/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Util.h"
#include "Log.h"
#include <regex>
#include <filesystem>
#ifdef _WIN32
#include <direct.h>
#endif
namespace fs = std::filesystem;

TIFF* GeoTiffOpen(const char* const filename, const char* const mode)
{
  TIFF* tif = XTIFFOpen(filename, mode);
  static const TIFFFieldInfo xtiffFieldInfo[] = {
    {TIFFTAG_GDAL_NODATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false, (char*)"GDALNoDataValue"},
    {TIFFTAG_GEOPIXELSCALE, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoPixelScale"},
    {TIFFTAG_GEOTRANSMATRIX, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoTransformationMatrix"},
    {TIFFTAG_GEOTIEPOINTS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoTiePoints"},
    {TIFFTAG_GEOKEYDIRECTORY, -1, -1, TIFF_SHORT, FIELD_CUSTOM, true, true, (char*)"GeoKeyDirectory"},
    {TIFFTAG_GEODOUBLEPARAMS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoDoubleParams"},
    {TIFFTAG_GEOASCIIPARAMS, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, true, (char*)"GeoASCIIParams"}};
  TIFFMergeFieldInfo(tif, xtiffFieldInfo, sizeof(xtiffFieldInfo) / sizeof(xtiffFieldInfo[0]));
  return tif;
}
int sxprintf(char* buffer, size_t N, const char* format, va_list* args)
{
  // printf("int sxprintf(char* buffer, size_t N, const char* format, va_list* args)\n");
  auto r = vsnprintf(buffer, N, format, *args);
  if (!(r < static_cast<int>(N)))
  {
    printf("**************** ERROR ****************\n");
    printf("\tTrying to write to buffer resulted in string being cut off at %ld characters\n", N);
    printf("Should have written:\n\t\"");
    vprintf(format, *args);
    printf("\"\n\t\"%s\"", buffer);
    // HACK: just loop
    printf("\n\t");
    for (size_t i = 0; i < (N - 1); ++i)
    {
      printf(" ");
    }
    printf("^-- cut off here at character %ld\n", N);
    printf("**************** ERROR ****************\n");
    throw std::runtime_error("String buffer overflow avoided");
  }
  return r;
}
int sxprintf(char* buffer, size_t N, const char* format, ...)
{
  // printf("int sxprintf(char* buffer, size_t N, const char* format, ...)\n");
  va_list args;
  va_start(args, format);
  auto r = sxprintf(buffer, N, format, &args);
  va_end(args);
  return r;
}
namespace tbd::util
{
void read_directory(const bool for_files, const string& name, vector<string>* v, const string& match)
{
  string full_match = ".*/" + match;
  logging::verbose(("Matching '" + full_match + "'").c_str());
  static const std::regex re(full_match, std::regex_constants::icase);
  for (const auto& entry : fs::directory_iterator(name))
  {
    logging::verbose(("Checking if file: " + entry.path().string()).c_str());
    if (
      (for_files && fs::is_regular_file(entry))
      || (!for_files && fs::is_directory(entry)))
    {
      logging::extensive(("Checking regex match: " + entry.path().string()).c_str());
      if (std::regex_match(entry.path().string(), re))
      {
#ifdef _WIN32
        v->push_back(entry.path().generic_string());
#else
        v->push_back(entry.path());
#endif
      }
    }
  }
}
void read_directory(const string& name, vector<string>* v, const string& match)
{
  read_directory(true, name, v, match);
}
void read_directory(bool for_files, const string& name, vector<string>* v)
{
  read_directory(for_files, name, v, "*");
}
void read_directory(const string& name, vector<string>* v)
{
  read_directory(name, v, "*");
}
vector<string> find_rasters(const string& dir, const int year)
{
  const auto for_year = dir + "/" + to_string(year) + "/";
  const auto for_default = dir + "/default/";
  // use first existing folder of dir/year, dir/default, or dir in that order
  const auto raster_root = directory_exists(for_year.c_str())
                           ? for_year
                           : (
                               directory_exists(for_default.c_str())
                                 ? for_default
                                 : dir);
  vector<string> results{};
  try
  {
    read_directory(raster_root, &results, "fuel.*\\.tif");
  }
  catch (const std::exception& ex)
  {
    logging::error("Unable to read directory %s", raster_root.c_str());
    logging::error("%s", ex.what());
  }
  return results;
}
bool directory_exists(const char* dir) noexcept
{
  struct stat dir_info{};
  return stat(dir, &dir_info) == 0 && dir_info.st_mode & S_IFDIR;
}
bool file_exists(const char* path) noexcept
{
  struct stat path_info{};
  // FIX: check that this works on symlinks
  return stat(path, &path_info) == 0 && path_info.st_mode & S_IFREG;
}
void make_directory(const char* dir) noexcept
{
#ifdef _WIN32
  if (-1 == _mkdir(dir) && errno != EEXIST)
#else
  if (-1 == mkdir(dir, 0777) && errno != EEXIST)
#endif
  {
    struct stat dir_info{};
    if (stat(dir, &dir_info) != 0)
    {
      logging::fatal("Cannot create directory %s", dir);
    }
    else if (dir_info.st_mode & S_IFDIR)
    {
      // everything is fine
    }
    else
    {
      logging::fatal("%s is not a directory\n", dir);
    }
  }
}
void make_directory_recursive(const char* dir) noexcept
{
  char tmp[256];
  sxprintf(tmp, "%s", dir);
  const auto len = strlen(tmp);
  if (tmp[len - 1] == '/')
    tmp[len - 1] = 0;
  for (auto p = tmp + 1; *p; ++p)
    if (*p == '/')
    {
      *p = 0;
      make_directory(tmp);
      *p = '/';
    }
  make_directory(tmp);
}
tm to_tm(const int year,
         const int month,
         const int day,
         const int hour,
         const int minute)
{
  // do this to calculate yday
  tm t{};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  mktime(&t);
  return t;
}
DurationSize to_time(const tm& t)
{
  return t.tm_yday
       + ((t.tm_hour + (static_cast<DurationSize>(t.tm_min) / HOUR_MINUTES)) / DAY_HOURS);
}
DurationSize to_time(const int year,
                     const int month,
                     const int day,
                     const int hour,
                     const int minute)
{
  return to_time(to_tm(year, month, day, hour, minute));
}
void read_date(istringstream* iss, string* str, tm* t)
{
  *t = {};
  // Date
  getline(iss, str, ',');
  string ds;
  istringstream dss(*str);
  getline(dss, ds, '-');
  t->tm_year = stoi(ds) - 1900;
  getline(dss, ds, '-');
  t->tm_mon = stoi(ds) - 1;
  getline(dss, ds, ' ');
  t->tm_mday = stoi(ds);
  getline(dss, ds, ':');
  t->tm_hour = stoi(ds);
  logging::verbose("Date is %4d-%02d-%02d %02d:00",
                   t->tm_year + 1900,
                   t->tm_mon + 1,
                   t->tm_mday,
                   t->tm_hour);
}
UsageCount::~UsageCount()
{
  logging::note("%s called %d times", for_what_.c_str(), count_.load());
}
UsageCount::UsageCount(string for_what) noexcept
  : count_(0), for_what_(std::move(for_what))
{
}
UsageCount& UsageCount::operator++() noexcept
{
  ++count_;
  return *this;
}
}
constexpr int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr int DAYS_IN_MONTH_LEAP[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void tbd::month_and_day(const int year, const size_t day_of_year, size_t* month, size_t* day_of_month)
{
  auto days = (is_leap_year(year) ? DAYS_IN_MONTH_LEAP : DAYS_IN_MONTH);
  *month = 1;
  int days_left = day_of_year;
  while (days[*month - 1] <= days_left)
  {
    days_left -= days[*month - 1];
    ++(*month);
  }
  *day_of_month = days_left + 1;
}
bool tbd::is_leap_year(const int year)
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
string tbd::make_timestamp(const int year, const DurationSize time)
{
  size_t day = floor(time);
  size_t hour = (time - day) * static_cast<DurationSize>(DAY_HOURS);
  size_t minute = round(((time - day) * static_cast<DurationSize>(DAY_HOURS) - hour) * HOUR_MINUTES);
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
  char buffer[128];
  sxprintf(buffer,
           "%4d-%02ld-%02ld %02ld:%02ld",
           year,
           month,
           day_of_month,
           hour,
           minute);
  return {buffer};
}
