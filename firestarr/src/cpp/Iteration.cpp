/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Iteration.h"
#include "ProbabilityMap.h"
#include "Scenario.h"
namespace fs::sim
{
Iteration::~Iteration()
{
  for (auto& s : scenarios_)
  {
    delete s;
  }
}
Iteration::Iteration(vector<Scenario*> scenarios) noexcept
  : scenarios_(std::move(scenarios))
{
}
Iteration* Iteration::reset(mt19937* mt_extinction, mt19937* mt_spread)
{
  cancelled_ = false;
  final_sizes_ = {};
  for (auto& scenario : scenarios_)
  {
    static_cast<void>(scenario->reset(mt_extinction, mt_spread, &final_sizes_));
  }
  return this;
}
//
// Iteration* Iteration::run(map<DurationSize, ProbabilityMap*>* probabilities)
//{
//  // sort in run so that they still get the same extinction thresholds as when unsorted
//		std::sort(scenarios_.begin(),
//			scenarios_.end(),
//			[](Scenario* lhs, Scenario* rhs) noexcept
//		{
//			// sort so that scenarios with highest DSRs are at the front
//		  //return lhs->weightedDsr() > rhs->weightedDsr();
//		});
//	if (Settings::runAsync())
//  {
//    vector<future<Scenario*>> results{};
//    // make a local copy so that we don't have mutex competition with other Iterations
//    map<DurationSize, ProbabilityMap*> local_probabilities{};
//    for (auto& kv : *probabilities)
//    {
//      local_probabilities[kv.first] = kv.second->copyEmpty();
//    }
//    for (auto& scenario : scenarios_)
//    {
//      results.push_back(async(launch::async,
//                              &Scenario::run,
//                              scenario,
//                              &local_probabilities));
//    }
//    for (auto& scenario : results)
//    {
//      auto s = scenario.get();
//      s->clear();
//    }
//    for (auto& kv : *probabilities)
//    {
//      kv.second->addProbabilities(*local_probabilities[kv.first]);
//      delete local_probabilities[kv.first];
//    }
//  }
//  else
//  {
//    for (auto& scenario : scenarios_)
//    {
//      scenario->run(probabilities);
//    }
//  }
//  return this;
//}
vector<DurationSize> Iteration::savePoints() const
{
  return scenarios_.at(0)->savePoints();
}
DurationSize Iteration::startTime() const
{
  return scenarios_.at(0)->startTime();
}
size_t Iteration::size() const noexcept
{
  return scenarios_.size();
}
util::SafeVector Iteration::finalSizes() const
{
  return final_sizes_;
}
void Iteration::cancel(bool show_warning) noexcept
{
  cancelled_ = true;
  for (auto& s : scenarios_)
  {
    s->cancel(show_warning);
  }
}
}
