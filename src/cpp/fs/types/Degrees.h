/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_DEGREES_H
#define FS_DEGREES_H
#include "../stdafx.h"
#include "StrictType.h"
namespace fs
{
struct Degrees : public StrictType<Degrees, units::CompassDegrees>
{
  using StrictType::StrictType;
  explicit constexpr Degrees(const DirectionSize degrees) noexcept
    : Degrees{static_cast<MathSize>(degrees)}
  { }
};
static constexpr Degrees INVALID_DIRECTION{std::numeric_limits<DirectionSize>::max()};
}
#endif
