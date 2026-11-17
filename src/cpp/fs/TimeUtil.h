/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TIMEUTIL_H
#define FS_TIMEUTIL_H
#include "stdafx.h"
#include <ctime>
namespace fs
{
constexpr YearSize TM_YEAR_OFFSET = 1900;
constexpr int TM_MONTH_OFFSET = 1;
// FIX: don't use same name as ctime since we're changing behaviour
#ifdef _WIN32
#define timegm _mkgmtime
struct tm* gmtime_r(const time_t* timer, struct tm* result);
struct tm* localtime_r(const time_t* timer, struct tm* result);
#endif
/**
 * \brief Calculate tm fields from values already there
 * @param t tm object to update
 */
time_t fix_utc(tm* t);
// sim specific time functions
/**
 * \brief Convert day and hour to DurationSize representing time
 * \tparam T Type used for representing day
 * \param day Day
 * \param hour Hour
 * \return DurationSize representing time at day and hour
 */
template <typename T>
[[nodiscard]] DurationSize to_time(const T day, const int hour) noexcept
{
  return day + hour / (1.0 * DAY_HOURS);
}
/**
 * \brief Convert time index to DurationSize representing time
 * \param t_index index for array of times
 * \return DurationSize representing time at day and hour
 */
[[nodiscard]] constexpr DurationSize to_time(const size_t t_index) noexcept
{
  return static_cast<DurationSize>(t_index) / DAY_HOURS;
}
/**
 * \brief Convert day and hour into time index
 * \tparam T Type used for representing day
 * \param day Day
 * \param hour Hour
 * \return index for array of times
 */
template <typename T>
[[nodiscard]] size_t time_index(const T day, const int hour) noexcept
{
  return static_cast<size_t>(day) * DAY_HOURS + static_cast<size_t>(hour);
}
/**
 * \brief Convert day and hour into time index since min_date
 * \param day Day
 * \param hour Hour
 * \param min_date Date at time index 0
 * \return index for array of times
 */
template <typename T>
[[nodiscard]] size_t time_index(const T day, const int hour, const Day min_date) noexcept
{
  return time_index(day, hour) - static_cast<size_t>(DAY_HOURS) * min_date;
}
/**
 * \brief Convert DurationSize into time index
 * \param time time to get time index for
 * \return index for array of times
 */
[[nodiscard]] constexpr size_t time_index(const DurationSize time) noexcept
{
  return static_cast<size_t>(time * DAY_HOURS);
}
/**
 * \brief Convert DurationSize into time index since min_date
 * \param time Time to convert to time index
 * \param min_date Date at time index 0
 * \return index for array of times
 */
[[nodiscard]] constexpr size_t time_index(const DurationSize time, const Day min_date) noexcept
{
  return time_index(time) - static_cast<size_t>(DAY_HOURS) * min_date;
}
/**
 * \brief Convert date and time into tm struct
 * \param year year
 * \param month month
 * \param day day of month
 * \param hour hour
 * \param minute minute
 * \return tm representing date and time
 */
tm to_utc(const YearSize year, const int month, const int day, const int hour, const int minute);
string format_datetime(const tm& date);
string format_date(const tm& date);
string format_time(const tm& date);
tm parse_utc(const string value);
void add_time(tm& date, const string value);
/**
 * \brief Convert tm struct into internal represenation
 * \param t tm struct to convert
 * \return internal time representation
 */
DurationSize to_time(const tm& t);
/**
 * \brief Convert date and time into internal represenation
 * \param year year
 * \param month month
 * \param day day of month
 * \param hour hour
 * \param minute minute
 * \return internal time representation
 */
DurationSize to_time(
  const YearSize year,
  const int month,
  const int day,
  const int hour,
  const int minute
);
/**
 * \brief Read a date from the given stream
 * \param iss Stream to read from
 * \param str string to read date into
 * \param t tm to parse date into
 */
void read_date(istringstream* iss, string* str, tm* t);
/**
 * \brief Determine month and day that a day of the year represents
 * \param year Year to determine for
 * \param day_of_year Day of year to determine for
 * \param month Month that was determined
 * \param day_of_month Day of month that was determined
 */
void month_and_day(
  const YearSize year,
  const size_t day_of_year,
  size_t* month,
  size_t* day_of_month
);
/**
 * \brief Determine if year is a leap year
 * \param year Year to determine for
 * \return Whether or not the given year is a leap year
 */
[[nodiscard]] bool is_leap_year(const YearSize year);
/**
 * Make a nicely formatted timestamp string for the given simulation time
 * @param year Year time is for
 * @param time Simulation time (fractional day of year)
 * @return YYYY-mm-dd HH:00 time string for given time
 */
[[nodiscard]] string make_timestamp(const YearSize year, const DurationSize time);
}
#endif
