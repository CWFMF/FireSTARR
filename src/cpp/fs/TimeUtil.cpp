/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "TimeUtil.h"
#include "Log.h"
#include "Util.h"
namespace fs
{
#ifdef _WIN32
struct tm* gmtime_r(const time_t* timer, struct tm* result)
{
  if (gmtime_s(result, timer))
  {
    return nullptr;
  }
  return result;
}
struct tm* localtime_r(const time_t* timer, struct tm* result)
{
  if (localtime_s(result, timer))
  {
    return nullptr;
  }
  return result;
}
#endif
time_t fix_utc(tm* t)
{
  // HACK: just use UTC since tz is not used
  const time_t t_t = timegm(t);
  if (!gmtime_r(&t_t, t))
  {
    logging::fatal("Unable to convert time from {}", t_t);
  }
  return t_t;
}
tm to_utc(const YearSize year, const int month, const int day, const int hour, const int minute)
{
  // do this to calculate yday
  tm t{};
  t.tm_year = year - TM_YEAR_OFFSET;
  t.tm_mon = month - TM_MONTH_OFFSET;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  fs::fix_utc(&t);
  return t;
}
string format_date(const tm& date)
{
  return std::format(
    "{:4d}-{:02d}-{:02d}",
    date.tm_year + TM_YEAR_OFFSET,
    date.tm_mon + TM_MONTH_OFFSET,
    date.tm_mday
  );
}
string format_time(const tm& date)
{
  return std::format("{:02d}:{:02d}", date.tm_hour, date.tm_min);
}
string format_datetime(const tm& date)
{
  // make sure parts match whole
  return std::format("{} {}", format_date(date), format_time(date));
}
tm parse_utc(const string value)
{
  tm date{};
  date.tm_year = stoi(value.substr(0, 4)) - TM_YEAR_OFFSET;
  date.tm_mon = stoi(value.substr(5, 2)) - TM_MONTH_OFFSET;
  date.tm_mday = stoi(value.substr(8, 2));
  fs::fix_utc(&date);
  return date;
}
void add_time(tm& date, const string value)
{
  // if this is a time then we aren't just running the weather
  date.tm_hour = stoi(value.substr(0, 2));
  fs::logging::check_fatal(
    date.tm_hour < 0 || date.tm_hour > 23,
    "Simulation start time has an invalid hour ({:d})",
    date.tm_hour
  );
  date.tm_min = stoi(value.substr(3, 2));
  fs::logging::check_fatal(
    date.tm_min < 0 || date.tm_min > 59,
    "Simulation start time has an invalid minute ({:d})",
    date.tm_min
  );
  fs::logging::debug("Simulation start time before fix_tm() is {:s}", format_datetime(date));
  fs::fix_utc(&date);
  fs::logging::debug("Simulation start time after fix_tm() is {:s}", format_datetime(date));
}
DurationSize to_time(const tm& t)
{
  return t.tm_yday
       + ((t.tm_hour + (static_cast<DurationSize>(t.tm_min) / HOUR_MINUTES)) / DAY_HOURS);
}
DurationSize to_time(
  const YearSize year,
  const int month,
  const int day,
  const int hour,
  const int minute
)
{
  return to_time(to_utc(year, month, day, hour, minute));
}
void read_date(istringstream* iss, string* str, tm* t)
{
  *t = {};
  // Date
  getline(iss, str, ',');
  string ds;
  istringstream dss(*str);
  getline(dss, ds, '-');
  t->tm_year = stoi(ds) - TM_YEAR_OFFSET;
  getline(dss, ds, '-');
  t->tm_mon = stoi(ds) - TM_MONTH_OFFSET;
  getline(dss, ds, ' ');
  t->tm_mday = stoi(ds);
  getline(dss, ds, ':');
  t->tm_hour = stoi(ds);
  logging::verbose("Date is {:s}", format_datetime(*t));
  fs::fix_utc(t);
}
constexpr int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr int DAYS_IN_MONTH_LEAP[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
void month_and_day(
  const YearSize year,
  const size_t day_of_year,
  size_t* month,
  size_t* day_of_month
)
{
  auto days = (is_leap_year(year) ? DAYS_IN_MONTH_LEAP : DAYS_IN_MONTH);
  *month = 1;
  auto days_left = static_cast<int>(day_of_year);
  while (days[*month - 1] <= days_left)
  {
    days_left -= days[*month - 1];
    ++(*month);
  }
  *day_of_month = static_cast<size_t>(days_left) + 1;
}
bool is_leap_year(const YearSize year)
{
  if (year % 400 == 0)
  {
    return true;
  }
  if (year % 100 == 0)
  {
    return false;
  }
  return (year % 4 == 0);
}
string make_timestamp(const YearSize year, const DurationSize time)
{
  auto day = static_cast<size_t>(floor(time));
  auto hour = static_cast<size_t>(
    (time - static_cast<DurationSize>(day)) * static_cast<DurationSize>(DAY_HOURS)
  );
  auto minute = static_cast<size_t>(round(
    ((time - static_cast<DurationSize>(day)) * static_cast<DurationSize>(DAY_HOURS)
     - static_cast<DurationSize>(hour))
    * HOUR_MINUTES
  ));
  if (60 == minute)
  {
    minute = 0;
    hour += 1;
  }
  if (24 == hour)
  {
    day += 1;
    hour = 0;
  }
  size_t month;
  size_t day_of_month;
  month_and_day(year, day, &month, &day_of_month);
  return std::format("{:4d}-{:02d}-{:02d} {:02d}:{:02d}", year, month, day_of_month, hour, minute);
}
}
