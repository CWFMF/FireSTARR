/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_UNSTABLE_H
#define FS_UNSTABLE_H
#define _USE_MATH_DEFINES
#include <cfloat>
#include <cmath>
// HACK: M_PI not showing up on Windows
#ifndef M_PI
#include <math.h>
#endif
namespace fs
{
// fixed-size float types aren't defined until C++23
#ifndef _Float32
#if __STDCPP_FLOAT32_T__
#define _Float32 float32_t
#elif 24 == FLT_MANT_DIG
#define _Float32 float
#elif 24 == DBL_MANT_DIG
#define _Float32 double
#else
#define _Float32 long double
#endif
#endif
#ifndef _Float64
#if __STDCPP_FLOAT64_T__
#define _Float64 float64_t
#elif 53 == FLT_MANT_DIG
#define _Float64 float
#elif 53 == DBL_MANT_DIG
#define _Float64 double
#else
#define _Float64 long double
#endif
#endif
static_assert(4 == sizeof(_Float32), "Could not determine 32-bit float type");
static_assert(8 == sizeof(_Float64), "Could not determine 64-bit float type");
// Type used for math operations
using MathSize = _Float64;
constexpr MathSize round_value(const MathSize value) noexcept
{
  // HACK: use for replicating old LookupTable rounding
  const auto i = static_cast<size_t>(value * 10);
  const auto v = i / static_cast<MathSize>(10);
  return v;
}
// HACK: override these so unstable code doesn't change results of functions
// in debug vs release version
MathSize cos(const MathSize angle) noexcept;
MathSize sin(const MathSize angle) noexcept;
}
#endif
