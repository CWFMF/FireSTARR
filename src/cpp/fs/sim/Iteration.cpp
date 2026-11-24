/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Iteration.h"
#include "Scenario.h"
namespace fs
{
Iteration::~Iteration()
{
  for (auto& s : scenarios_)
  {
    delete s;
  }
}
Iteration::Iteration(vector<Scenario*> scenarios) noexcept : scenarios_(std::move(scenarios)) { }
Iteration* Iteration::reset_with_new_start(const XYIdx& start_cell)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  // HACK: ensure only called with surface
  logging::check_fatal(
    !settings.is_surface(), "Called reset_with_new_start() when not calculating surface"
  );
  // HACK: just copy code for now
  // FIX: remove duplicate code
  cancelled_ = false;
  final_sizes_ = {};
  auto i = 0;
  // could have multiple weather scenarios so this still makes sense to loop
  for (auto& scenario : scenarios_)
  {
    logging::extensive("Resetting scenario {:d}", i);
    std::ignore = scenario->reset_with_new_start(start_cell, &final_sizes_);
    ++i;
  }
  return this;
}
Iteration* Iteration::reset(mt19937_64* mt_extinction, mt19937_64* mt_spread)
{
  cancelled_ = false;
  final_sizes_ = {};
  for (auto& scenario : scenarios_)
  {
    static_cast<void>(scenario->reset(mt_extinction, mt_spread, &final_sizes_));
  }
  return this;
}
vector<DurationSize> Iteration::savePoints() const { return scenarios_.at(0)->savePoints(); }
DurationSize Iteration::startTime() const { return scenarios_.at(0)->startTime(); }
size_t Iteration::size() const noexcept { return scenarios_.size(); }
SafeVector Iteration::finalSizes() const { return final_sizes_; }
void Iteration::cancel(bool show_warning) noexcept
{
  cancelled_ = true;
  for (auto& s : scenarios_)
  {
    s->cancel(show_warning);
  }
}
}
