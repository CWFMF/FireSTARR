/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_UNSTABLE_H
#define FS_UNSTABLE_H
#define _USE_MATH_DEFINES
#include <cmath>
// HACK: M_PI not showing up on Windows
#include <cfloat>
#include <math.h>
namespace fs
{
/**
 * \brief Type used for math operations
 */
// use first type that's 64-bit
#if FLT_DIG > 6
using MathSize = float;
#elif DBL_DIG > 6
using MathSize = double;
#else
using MathSize = long double;
#endif
static_assert(8 == sizeof(MathSize), "64-bit required for MathSize");
// HACK: override these so unstable code doesn't change results of functions
// in debug vs release version
MathSize cos(const MathSize angle) noexcept;
MathSize sin(const MathSize angle) noexcept;
}
#endif
