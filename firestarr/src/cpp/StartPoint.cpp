/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "StartPoint.h"
#include "Settings.h"
#include "unstable.h"
namespace fs::topo
{
template <typename T>
static T fix_range(T value, T min_value, T max_value) noexcept
{
  while (value < min_value)
  {
    value += max_value;
  }
  while (value >= max_value)
  {
    value -= max_value;
  }
  return value;
}
template <typename T>
static T fix_degrees(T value) noexcept
{
  return fix_range(value, 0.0, 360.0);
}
template <typename T>
static T fix_hours(T value) noexcept
{
  return fix_range(value, 0.0, 24.0);
}
static DurationSize sunrise_sunset(const int jd,
                                   const MathSize latitude,
                                   const MathSize longitude,
                                   const bool for_sunrise) noexcept
{
  static const auto Zenith = util::to_radians(96);
  static const auto LocalOffset = -5;
  const auto t_hour = for_sunrise ? 6 : 18;
  // http://edwilliams.org/sunrise_sunset_algorithm.htm
  const auto lng_hour = longitude / 15;
  const auto t = jd + (t_hour - lng_hour) / 24;
  const auto m = 0.9856 * t - 3.289;
  const auto l = fix_degrees(
    m + 1.916 * _sin(util::to_radians(m)) + 0.020 * _sin(util::to_radians(2 * m)) + 282.634);
  auto ra = fix_degrees(util::to_degrees(atan(0.91764 * tan(util::to_radians(l)))));
  const auto l_quadrant = floor(l / 90) * 90;
  const auto ra_quadrant = floor(ra / 90) * 90;
  ra += l_quadrant - ra_quadrant;
  ra /= 15;
  const auto sin_dec = 0.39782 * _sin(util::to_radians(l));
  const auto cos_dec = _cos(asin(sin_dec));
  const auto cos_h = (_cos(Zenith) - sin_dec * _sin(util::to_radians(latitude))) / (cos_dec * _cos(util::to_radians(latitude)));
  if (cos_h > 1)
  {
    // sun never rises
    return for_sunrise ? -1 : 25;
  }
  if (cos_h < -1)
  {
    // sun never sets
    return for_sunrise ? 25 : -1;
  }
  auto h = util::to_degrees(acos(cos_h));
  if (for_sunrise)
  {
    h = 360 - h;
  }
  h /= 15;
  const auto mean_t = h + ra - 0.06571 * t - 6.622;
  const auto ut = mean_t - lng_hour;
  return fix_hours(ut + LocalOffset);
}
static DurationSize sunrise(const int jd,
                            const MathSize latitude,
                            const MathSize longitude) noexcept
{
  return sunrise_sunset(jd, latitude, longitude, true);
}
static DurationSize sunset(const int jd, const MathSize latitude, const MathSize longitude) noexcept
{
  return sunrise_sunset(jd, latitude, longitude, false);
}
static array<tuple<DurationSize, DurationSize>, MAX_DAYS> make_days(
  const MathSize latitude,
  const MathSize longitude) noexcept
{
  array<tuple<DurationSize, DurationSize>, MAX_DAYS> days{};
  array<DurationSize, MAX_DAYS> day_length_hours{};
  for (size_t i = 0; i < day_length_hours.size(); ++i)
  {
    days[i] = make_tuple(
      fix_hours(
        sunrise(static_cast<int>(i), latitude, longitude) + sim::Settings::offsetSunrise()),
      fix_hours(
        sunset(static_cast<int>(i),
               latitude,
               longitude)
        - sim::Settings::offsetSunset()));
    day_length_hours[i] = get<1>(days[i]) - get<0>(days[i]);
  }
  return days;
}
StartPoint::StartPoint(const MathSize latitude, const MathSize longitude) noexcept
  : Point(latitude, longitude), days_(make_days(latitude, longitude))
{
}
StartPoint& StartPoint::operator=(StartPoint&& rhs) noexcept
{
  if (this != &rhs)
  {
    Point::operator=(rhs);
    for (size_t i = 0; i < days_.size(); ++i)
    {
      days_[i] = rhs.days_[i];
    }
  }
  return *this;
}
}
