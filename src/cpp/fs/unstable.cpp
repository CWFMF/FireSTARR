/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "unstable.h"
#include <cfenv>
#define USE_FP_ENV 1
#undef USE_FP_ENV
namespace fs
{
MathSize cos(const MathSize angle) noexcept { return static_cast<MathSize>(std::cos(angle)); }
MathSize sin(const MathSize angle) noexcept { return static_cast<MathSize>(std::sin(angle)); }
MathSize acos(const MathSize angle) noexcept
{
#ifdef USE_FP_ENV
  // FIX: change to use RAII
  std::fenv_t env{};
  std::feholdexcept(&env);
#endif
  const auto v = std::acos(angle);
#ifdef USE_FP_ENV
  // clear FE_INVALID but not other exceptions
  std::feclearexcept(FE_INVALID);
  std::feupdateenv(&env);
#endif
  return v;
}
}
void setfperrors()
{
#ifdef USE_FP_ENV
  // FE_INEXACT happens in stoa()
  fexcept_t excepts{};
  fesetexceptflag(&excepts, FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW);
#endif
  // HACK: set rounding here too
  fesetround(FE_TONEAREST);
}
