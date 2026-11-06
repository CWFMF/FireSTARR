/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "unstable.h"
namespace fs
{
MathSize cos(const MathSize angle) noexcept { return static_cast<MathSize>(std::cos(angle)); }
MathSize sin(const MathSize angle) noexcept { return static_cast<MathSize>(std::sin(angle)); }
}
