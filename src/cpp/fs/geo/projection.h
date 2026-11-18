/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_PROJECTION_H
#define FS_PROJECTION_H
#include "../stdafx.h"
#include "../types/Location.h"
#include "Point.h"
namespace fs
{
using fs::FullCoordinates;
using fs::MathSize;
std::optional<FullCoordinates> to_proj4(
  const string& proj4,
  const fs::Point& point,
  MathSize* x,
  MathSize* y
);
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
fs::XYPos from_lat_long(const string_view proj4, const fs::Point& point);
fs::Point to_lat_long(const string_view proj4, const fs::XYPos xy);
string try_fix_meridian(const string_view proj4);
Degrees find_north_south_deviation(const string_view proj4, const Point& p0);
bool check_deviation(
  const string_view what,
  const string_view proj4,
  const Point& p,
  const MathSize max_deviation
);
}
#endif
