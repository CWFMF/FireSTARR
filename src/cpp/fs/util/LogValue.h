/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LOG_VALUE_H
#define FS_LOG_VALUE_H
#include <compare>
#include "../unstable.h"
namespace fs
{
/**
 * \brief A result of calling log(x) for some value of x, pre-calculated at compile time.
 */
struct LogValue
{
  MathSize value{0};
  std::partial_ordering operator<=>(const LogValue& rhs) const = default;
};
static constexpr LogValue LOG_0_70{-0.35667494393873245};
static constexpr LogValue LOG_0_75{-0.2876820724517809};
static constexpr LogValue LOG_0_80{-0.2231435513142097};
static constexpr LogValue LOG_0_85{-0.16251892949777494};
static constexpr LogValue LOG_0_90{-0.10536051565782628};
static constexpr LogValue LOG_1_00{0.0};
#ifndef _WIN32
#if __cpp_lib_constexpr_cmath >= 202202L   // C++23
// windows won't use constexpr with log but we can double check numbers are right when compiling
// elsewhere
static constexpr LogValue LOG_0_70_CALC{log(0.7)};
static constexpr LogValue LOG_0_75_CALC{log(0.75)};
static constexpr LogValue LOG_0_80_CALC{log(0.8)};
static constexpr LogValue LOG_0_85_CALC{log(0.85)};
static constexpr LogValue LOG_0_90_CALC{log(0.9)};
static constexpr LogValue LOG_1_00_CALC{log(1.0)};
static_assert(abs((LOG_0_70.value - LOG_0_70_CALC.value)) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_0_75.value - LOG_0_75_CALC.value)) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_0_80.value - LOG_0_80_CALC.value)) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_0_90.value - LOG_0_90_CALC.value)) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_1_00.value - LOG_1_00_CALC.value)) < numeric_limits<MathSize>::epsilon());
#endif
#endif
}
#endif
