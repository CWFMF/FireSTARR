/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "../stdafx.h"
#include "StartPoint.h"
#include "../unstable.h"
#include "Settings.h"
namespace fs
{
using settings::Settings;
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
static Degrees fix_degrees(const Degrees& degrees) noexcept
{
  return Degrees{fix_degrees(degrees.value)};
}
template <typename T>
static T fix_hours(T value) noexcept
{
  return fix_range(value, 0.0, 24.0);
}
/**
 * Sunrise/sunset time for UTC at location for julian day
 */
static std::pair<DurationSize, DurationSize> sunrise_sunset(
  const int jd,
  const MathSize latitude,
  const MathSize longitude
) noexcept
{
  static const auto Zenith = Radians::from_degrees(96.0);
  const auto lng_hour = longitude / 15;
  auto find_time = [=](const bool for_sunrise) -> DurationSize {
    const auto t_hour = for_sunrise ? 6 : 18;
    // http://edwilliams.org/sunrise_sunset_algorithm.htm
    const auto t = jd + (t_hour - lng_hour) / 24;
    const auto m = 0.9856 * t - 3.289;
    const auto l = fix_degrees(
      m + 1.916 * sin(Radians::from_degrees(m)) + 0.020 * sin(Radians::from_degrees(2 * m))
      + 282.634
    );
    auto ra =
      fix_degrees(Radians{atan(0.91764 * tan(Radians{Radians::from_degrees(l)}))}.asDegrees())
        .value;
    const auto l_quadrant = floor(l / 90) * 90;
    const auto ra_quadrant = floor(ra / 90) * 90;
    ra += l_quadrant - ra_quadrant;
    ra /= 15;
    const auto sin_dec = 0.39782 * sin(Radians::from_degrees(l));
    const auto cos_dec = cos(asin(sin_dec));
    const auto cos_h = (cos(Zenith) - sin_dec * sin(Radians::from_degrees(latitude)))
                     / (cos_dec * cos(Radians::from_degrees(latitude)));
    MathSize h = Radians{acos(cos_h)}.asDegrees().value;
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
    if (for_sunrise)
    {
      h = 360 - h;
    }
    h /= 15;
    const auto mean_t = h + ra - 0.06571 * t - 6.622;
    const auto ut = mean_t - lng_hour;
    return fix_hours(ut);
  };
  return {find_time(true), find_time(false)};
}
static array<tuple<DurationSize, DurationSize>, MAX_DAYS> make_days(
  const MathSize latitude,
  const MathSize longitude
) noexcept
{
  static const Settings& settings = settings::instance();
  array<tuple<DurationSize, DurationSize>, MAX_DAYS> days{};
  array<DurationSize, MAX_DAYS> day_length_hours{};
  constexpr auto DEGREES_PER_TZ_HOUR = 15.0;
  const auto tz_calculated = floor(longitude / DEGREES_PER_TZ_HOUR);
  auto utc_offset = settings.utc_offset.value_or(tz_calculated);
  if (!settings.utc_offset.has_value())
  {
    logging::note("Using calculated timezone offset of ({:0.1g}) since not specified", utc_offset);
  }
  else if (1 < abs(tz_calculated - utc_offset))
  {
    logging::warning(
      "Specified timezone ({:0.1g}) differs from calculated timezone ({:0.1g}) by {:0.1g} hours",
      utc_offset,
      tz_calculated,
      (tz_calculated - utc_offset)
    );
  }
  for (size_t i = 0; i < day_length_hours.size(); ++i)
  {
    const auto [sunrise, sunset] = sunrise_sunset(static_cast<int>(i), latitude, longitude);
    days[i] = make_tuple(
      fix_hours(sunrise + utc_offset + settings.offset_sunrise),
      fix_hours(sunset + utc_offset - settings.offset_sunset)
    );
    day_length_hours[i] = get<1>(days[i]) - get<0>(days[i]);
  }
  return days;
}
StartPoint::StartPoint(const MathSize latitude, const MathSize longitude) noexcept
  : Point(latitude, longitude), days_(make_days(latitude, longitude))
{ }
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
