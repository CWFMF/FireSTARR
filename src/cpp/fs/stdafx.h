/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_STDAFX_H
#define FS_STDAFX_H
#define _USE_MATH_DEFINES
#include "debug_settings.h"
#include "unstable.h"
// #define VLD_FORCE_ENABLE
// #include "vld.h"
#define NOMINMAX
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <compare>
#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <execution>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <ranges>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include "unstable.h"
namespace fs
{
// HACK: avoid warning about unused include
using fs::debug::show_debug_settings;
using std::abs;
using std::array;
using std::async;
using std::atomic;
using std::cout;
using std::endl;
using std::fixed;
using std::function;
using std::future;
using std::get;
using std::getline;
using std::hash;
using std::ifstream;
using std::istringstream;
using std::launch;
using std::list;
using std::lock_guard;
using std::make_shared;
using std::make_tuple;
using std::make_unique;
using std::map;
using std::max;
using std::min;
using std::mt19937_64;
using std::mutex;
using std::numeric_limits;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::put_time;
using std::runtime_error;
using std::set;
using std::setprecision;
using std::shared_ptr;
using std::stod;
using std::stoi;
using std::stol;
using std::string;
using std::string_view;
using std::stringstream;
using std::to_string;
using std::to_wstring;
using std::tuple;
using std::uniform_real_distribution;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using std::wstring;
// HACK: use these for things we want to keep as smart pointers during refact
template <class T>
using ptr = T*;
template <class T>
using uptr = unique_ptr<T>;
template <class T>
using sptr = shared_ptr<T>;
// HACK: use aliases for types so we know what they're supposed to represent
// TODO: replace with strict types
/**
 * \brief Size of a calendar year
 */
using YearSize = int;
/**
 * \brief Difference minimum for MathSizes to be considered the same
 */
static const MathSize COMPARE_LIMIT = 1.0E-20f;
/**
 * \brief Size of the hash of a Cell
 */
using HashSize = uint32_t;
/**
 * \brief Size of the index for a Cell
 */
using CellIndex = uint8_t;
// want to be able to make a bitmask of all directions it came from
//  064  008  032
//  001  000  002
//  016  004  128
static constexpr CellIndex DIRECTION_NONE = 0b00000000;
static constexpr CellIndex DIRECTION_W = 0b00000001;
static constexpr CellIndex DIRECTION_E = 0b00000010;
static constexpr CellIndex DIRECTION_S = 0b00000100;
static constexpr CellIndex DIRECTION_N = 0b00001000;
static constexpr CellIndex DIRECTION_SW = 0b00010000;
static constexpr CellIndex DIRECTION_NE = 0b00100000;
static constexpr CellIndex DIRECTION_NW = 0b01000000;
static constexpr CellIndex DIRECTION_SE = 0b10000000;
// x or y index for a grid
using Idx = int16_t;
constexpr auto INVALID_INDEX = std::numeric_limits<Idx>::min();
// x or y index for a grid not in memory yet
using FullIdx = int64_t;
/**
 * \brief Type used for perimeter raster values (uses [0, 1])
 */
using PerimSize = uint8_t;
/**
 * \brief Type used for fuel values (uses [0 - 999]?)
 */
using FuelSize = uint16_t;
/**
 * \brief Size to use for representing fuel types
 */
using FuelCodeSize = uint8_t;
/**
 * \brief Type used for direction values (uses [0 - 359])
 */
using DirectionSize = uint16_t;
/**
 * \brief Type used for aspect values (uses [0 - 359])
 */
using AspectSize = DirectionSize;
/**
 * \brief Type used for elevation values (uses [? - 9800?])
 */
using ElevationSize = int16_t;
constexpr auto INVALID_ELEVATION = std::numeric_limits<ElevationSize>::max();
/**
 * \brief Type used for slope values (uses [0 - MAX_SLOPE_FOR_DISTANCE])
 */
using SlopeSize = uint16_t;
/**
 * \brief Type used for storing intensities
 */
using IntensitySize = uint32_t;
/**
 * \brief Type used for storing locations within cells
 */
using InnerSize = _Float64;
/**
 * \brief Type used for storing locations in environment
 */
using XYSize = _Float64;
/**
 * \brief Type used for probability thresholds
 */
using ThresholdSize = _Float64;
/**
 * \brief Type used for storing locations within cells
 */
using DurationSize = _Float64;
constexpr DurationSize INVALID_TIME = -1;
constexpr auto NO_INTENSITY = static_cast<IntensitySize>(0);
using ROSSize = MathSize;
constexpr auto NO_ROS = static_cast<ROSSize>(0.0);
/**
 * \brief A day (0 - 366)
 */
using Day = uint16_t;
static constexpr Day MAX_DAYS = 366;
static constexpr Idx MAX_WIDTH = 4096;
static constexpr Idx MAX_HEIGHT = MAX_WIDTH;
static constexpr Idx PREFERRED_TILE_WIDTH = 256;
static constexpr Idx TILE_WIDTH = min(MAX_WIDTH, static_cast<Idx>(PREFERRED_TILE_WIDTH));
/**
 * \brief Maximum aspect value (360 == 0)
 */
static constexpr DirectionSize MAX_ASPECT = 359;
/**
 * \brief Maximum slope that can be stored - this is used in the horizontal distance calculation
 */
static constexpr SlopeSize MAX_SLOPE_FOR_DISTANCE = 500;
/**
 * \brief Invalid angle value
 */
static constexpr DirectionSize INVALID_ANGLE = 361;
/**
 * \brief Invalid aspect value
 */
static constexpr auto INVALID_ASPECT = static_cast<AspectSize>(INVALID_ANGLE);
/**
 * \brief Invalid grass curing value
 */
static constexpr MathSize INVALID_CURING = -1;
/**
 * \brief Invalid slope value
 */
static constexpr SlopeSize INVALID_SLOPE = 511;
/**
 * \brief Invalid FuelType code value
 */
static constexpr FuelCodeSize INVALID_FUEL_CODE = 0;
/**
 * \brief Number of all possible fuels in simulation
 */
static constexpr size_t NUMBER_OF_FUELS = 141;
/**
 * \brief Number of hours in a day
 */
static constexpr int DAY_HOURS = 24;
/**
 * \brief Number of minutes in an hour
 */
static constexpr int HOUR_MINUTES = 60;
/**
 * \brief Number of seconds in a minute
 */
static constexpr int MINUTE_SECONDS = 60;
/**
 * \brief Number of seconds in an hour
 */
static constexpr int HOUR_SECONDS = HOUR_MINUTES * MINUTE_SECONDS;
/**
 * \brief Number of minutes in a day
 */
static constexpr int DAY_MINUTES = DAY_HOURS * HOUR_MINUTES;
/**
 * \brief Number of seconds in a day
 */
static constexpr int DAY_SECONDS = DAY_MINUTES * MINUTE_SECONDS;
/**
 * \brief Number of hours in a year
 */
static constexpr int YEAR_HOURS = MAX_DAYS * DAY_HOURS;
/**
 * \brief Array of results of a function for all possible integer percent slopes
 */
using SlopeTableArray = array<_Float64, INVALID_SLOPE + 1>;
/**
 * \brief Size of an angle in degrees
 */
using DegreesSize = uint16_t;
/**
 * \brief Array of results of a function for all possible integer angles in degrees
 */
using AngleTableArray = array<_Float64, 361>;
/**
 * \brief Size to use for representing sub-coordinates for location within a Cell
 */
using SubSize = uint16_t;
/**
 * \brief Coordinates ((x, y), (sub-x, sub-y))
 */
struct Coordinates
{
  Idx x;
  Idx y;
  SubSize x_sub;
  SubSize y_sub;
};
/**
 * \brief FullCoordinates ((x, y), (sub-x, sub-y))
 */
struct FullCoordinates
{
  FullIdx x;
  FullIdx y;
  SubSize x_sub;
  SubSize y_sub;
};
/**
 * \brief Type of clock to use for times
 */
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
}
#endif
