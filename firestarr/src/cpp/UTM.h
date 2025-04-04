/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once

#include <proj.h>
#include "stdafx.h"
#include "Util.h"
namespace fs::topo
{
class Point;
/**
 * \brief Calculate the UTM zone for the given meridian
 * \param meridian A MathSize designating the meridian to calculate the UTM zone for (degrees)
 * \return UTM zone for given meridian
 */
[[nodiscard]] constexpr MathSize meridian_to_zone(const MathSize meridian) noexcept
{
  return (meridian + 183.0) / 6.0;
}
/**
 * \brief Determines the central meridian for the given UTM zone.
 * \param zone A MathSize designating the UTM zone, range [1,60].
 * \return The central meridian for the given UTM zone (degrees)
 */
[[nodiscard]] constexpr MathSize utm_central_meridian(const MathSize zone) noexcept
{
  return -183.0 + zone * 6.0;
}
void from_lat_long(
  const string& proj4,
  const fs::topo::Point& point,
  MathSize* x,
  MathSize* y);
fs::topo::Point to_lat_long(
  const string& proj4,
  const MathSize x,
  const MathSize y);
string&& try_fix_meridian(string&& proj4);
}
