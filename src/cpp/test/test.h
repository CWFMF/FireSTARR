/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TEST_H
#define FS_TEST_H
#include <concepts>
#include "../fs/log/Log.h"
#include "../fs/unstable.h"
namespace fs::testing
{
template <class T>
auto check_range(
  const char* name_fct,
  const char* name_param,
  const auto fct_a,
  const auto fct_b,
  const MathSize epsilon,
  const T& it
)
{
  logging::debug("Checking {:s}", name_fct);
  std::for_each(
#if !defined(__APPLE__) || !defined(__clang__)
    // apple clang doesn't support this?
    std::execution::par_unseq,
#endif
    it.begin(),
    it.end(),
    [&](const auto& v) {
      const auto msg = std::format("{} ({} = {})", name_fct, name_param, v);
      logging::check_tolerance(epsilon, fct_a(v), fct_b(v), msg.c_str());
      logging::verbose("{:s}", msg);
    }
  );
}
template <typename T>
concept IsValue = requires(const T& t) {
  { t } -> std::convertible_to<MathSize>;
};
template <typename T>
concept IsFunction = requires(const T& t) {
  { t() } -> std::convertible_to<MathSize>;
};
template <class T>
  requires IsValue<T>
MathSize value_of(const T& t)
{
  return t;
}
template <class T>
  requires IsFunction<T>
MathSize value_of(const T& t)
{
  return t();
}
template <class TypeA, class TypeB>
int compare_duff(
  const string name,
  const TypeA& a,
  const TypeB& b,
  const logging::level log_level = logging::level::info
)
{
  static constexpr int RESOLUTION = 10000;
  static constexpr MathSize RANGE = 250.0;
  // check %, so 1 decimal is fine
  static constexpr auto EPSILON = static_cast<MathSize>(1e-1);
  std::ignore = logging::output(log_level, "Checking {:s}", name);
  logging::check_equal_verbose(logging::level::debug, value_of(a.ash), value_of(b.ash), "ash");
  logging::check_equal_verbose(logging::level::debug, value_of(a.rho), value_of(b.rho), "rho");
  logging::check_equal_verbose(logging::level::debug, value_of(a.b0), value_of(b.b0), "b0");
  logging::check_equal_verbose(logging::level::debug, value_of(a.b1), value_of(b.b1), "b1");
  logging::check_equal_verbose(logging::level::debug, value_of(a.b2), value_of(b.b2), "b2");
  logging::check_equal_verbose(logging::level::debug, value_of(a.b3), value_of(b.b3), "b3");
  for (auto i = 0; i < RESOLUTION; ++i)
  {
    const MathSize mc = RANGE * i / RESOLUTION;
    const auto msg = std::format("probability of survival (mc = {})", mc);
    logging::check_tolerance(
      EPSILON, a.probabilityOfSurvival(mc), b.probabilityOfSurvival(mc), msg.c_str()
    );
  }
  return 0;
}
// FIX: this was used to compare to the old template version, but now just
//      compares each Duff to itself
int test_duff(const int argc, const char* const argv[]);
}
#endif
