/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_UNSTABLE_H
#define FS_UNSTABLE_H
#include <cfenv>
#include <stdexcept>
#define CHECK_ROUNDING 1
// #undef CHECK_ROUNDING
#define DEBUG_ROUNDING 1
#undef DEBUG_ROUNDING
#if defined(DEBUG_ROUNDING) || defined(CHECK_ROUNDING)
#include <cinttypes>
#endif
#include <cstdint>
#include <limits>
#define _USE_MATH_DEFINES
#include <cfloat>
#include <cmath>
// HACK: M_PI not showing up on Windows
#ifndef M_PI
#include <math.h>
#endif
inline void setfperrors()
{
  // FE_INEXACT happens in stoa()
  feenableexcept(FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID);
}
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
// Type used for math operations
using MathSize = _Float64;
using MathConstantSize = _Float64;
template <typename T>
concept HasValueFunction = requires(const T& t) {
  { t.value() } -> std::convertible_to<MathSize>;
};
template <typename T>
concept HasValue = requires(const T& t) {
  { t.value } -> std::convertible_to<fs::MathSize>;
};
template <typename T>
concept IsValue = requires(const T& t) {
  { t } -> std::convertible_to<MathSize>;
};
template <typename T>
concept IsFunction = requires(const T& t) {
  { t() } -> std::convertible_to<MathSize>;
};
template <class T>
  requires HasValueFunction<T>
constexpr MathSize as_value(const T& t)
{
  return t.value();
}
template <class T>
  requires HasValue<T>
constexpr MathSize as_value(const T& t)
{
  return t.value;
}
template <class T>
  requires IsValue<T>
constexpr MathSize as_value(const T& t)
{
  return t;
}
// if both then use .value
template <class T>
  requires IsValue<T> && HasValue<T>
constexpr MathSize as_value(const T& t)
{
  return t.value;
}
template <class T>
  requires IsFunction<T>
constexpr MathSize as_value(const T& t)
{
  return t();
}
/**
 * \brief Return base raised to the power N
 * \tparam N Power to raise to
 * \tparam T Type passed and returned
 * \param base Base to raise to power N
 * \return base raised to power N
 */
template <size_t N, class T>
[[nodiscard]] constexpr T pow_int(const T& base)
{
  // https://stackoverflow.com/questions/16443682/c-power-of-integer-template-meta-programming
  return N == 0     ? 1
       : N % 2 == 0 ? pow_int<N / 2, T>(base) * pow_int<N / 2, T>(base)
                    : base * pow_int<(N - 1) / 2, T>(base) * pow_int<(N - 1) / 2, T>(base);
}
constexpr MathSize round_value(const MathSize value) noexcept
{
  const auto i = static_cast<std::uint64_t>(value * 10);
  const auto v = static_cast<MathSize>(i) / static_cast<MathSize>(10);
  return v;
}
// HACK: float values seem to deviate at 18 digits, but round to have MaxFloatValue not overflow
using RoundedValue = MathSize;
// Can go 300k+ for S3. Don't want to use int but don't need high precision
using RoundedIntensity = MathSize;
// HACK: things that used old LookupTable rounding
using LookupValue = MathSize;
// HACK: override these so unstable code doesn't change results of functions
// in debug vs release version
MathSize cos(const MathSize angle) noexcept;
MathSize sin(const MathSize angle) noexcept;
}
#endif
