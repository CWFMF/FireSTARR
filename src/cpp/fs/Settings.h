/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SETTINGS_H
#define FS_SETTINGS_H
#include "stdafx.h"
#include "FuelLookup.h"
namespace fs::settings
{
class Settings;
Settings& instance() noexcept;
class OutputDateOffsets
{
public:
  OutputDateOffsets(const string value) noexcept;
  OutputDateOffsets() noexcept = default;
  OutputDateOffsets(const OutputDateOffsets& rhs) = default;
  OutputDateOffsets(OutputDateOffsets&& rhs) = default;
  OutputDateOffsets& operator=(const OutputDateOffsets& rhs) = default;
  OutputDateOffsets& operator=(OutputDateOffsets&& rhs) = default;
  // Original text that was parsed
  [[nodiscard]] string text() const noexcept { return text_; }
  // Days to output probability contours for (1 is start date, 2 is day after, etc.)
  [[nodiscard]] vector<int> offsets() const noexcept { return output_date_offsets_; }
  // Whatever the maximum value in the date offsets is
  [[nodiscard]] int max() const noexcept { return max_date_offset_; }

private:
  string text_{};
  // Days to output probability contours for (1 is start date, 2 is day after, etc.)
  vector<int> output_date_offsets_{};
  // Whatever the maximum value in the date offsets is
  int max_date_offset_{0};
};
class StaticCuring
{
public:
  StaticCuring() noexcept = default;
  StaticCuring(const int value) noexcept
    : value_{[&]() {
        // HACK: enforce bounds
        logging::check_fatal(
          0 > value || 100 < value, "Grass curing (%) must be in range [0-100] but got {:d}", value
        );
        return value;
      }()}
  { }
  int value() const noexcept { return value_.value(); };
  bool has_value() const noexcept { return value_.has_value(); }
  string as_string() const noexcept
  {
    string result = has_value() ? std::format("{:d}", value()) : "";
    return result;
  }

private:
  std::optional<int> value_{};
};
enum class Mode
{
  Simulation,
  Test,
  Surface
};
using namespace fuel;
/**
 * \brief Reads and provides access to settings for the simulation.
 */
class Settings
{
public:
  static void setRoot(const string dir_binary, const string dir_root) noexcept;
  const string getRoot() const noexcept;
  const string getBinaryDirectory() const noexcept;
  Settings() = delete;
  Settings(const string dir_binary, const string dir_root) noexcept;
  string create(const pushd& dir) const;
  // save to directory in same format parse expects
  void saveTo(const string& output_directory) const noexcept;
  bool found(const string& key) const noexcept
  {
    auto& s = settings_.second;
    return s.find(key) != s.end();
  }

public:
  // general settings
  Mode mode{Mode::Simulation};
  // directory to put simulation outputs in
  string output_directory{};
  // .csv with weather streams
  LazyPath wx_file_name{};
  // name to use for log file inside output_directory
  string log_file_name{"firestarr.log"};
  // full path for log file
  string log_file{};
  // HACK: should be output_directory, but if log_file_name starts with '/' it could be anything
  string log_directory() const
  {
    static const auto d = [&] {
      const auto last_index = log_file.find_last_of('/') + 1;
      return log_file.substr(0, last_index);
    }();
    return d;
  }
  // perimeter to use for igntion (empty if none)
  LazyPath perimeter{};
  // Whether or not to save individual grids
  bool save_individual{false};
  // Whether or not to run things asynchronously where possible
  bool run_async{true};
  // Whether or not to run deterministically (100% chance of spread & survival)
  bool deterministic{false};
  // Whether or not this is running in test mode
  constexpr bool is_test() const { return Mode::Test == mode; }
  // Whether or not this is running in surface mode
  constexpr bool is_surface() const { return Mode::Surface == mode; }
  // Whether or not to save grids as .asc
  bool save_as_ascii{false};
  // Whether or not to save grids as .tif
  bool save_as_tiff{true};
  // Whether or not to save points used for spread
  bool save_points{false};
  // Whether or not to save intensity grids
  bool save_intensity{true};
  // Whether or not to save probability grids
  bool save_probability{true};
  // Whether or not to save occurrence grids
  bool save_occurrence{false};
  // Whether or not to save simulation area grids
  bool save_simulation_area{false};
  // Whether or not to force greenup for all fires
  bool force_greenup{false};
  // Whether or not to force no greenup for all fires
  bool force_no_greenup{false};
  // Whether or not to search for a start location if start point is non-fuel
  bool no_search{false};
  // Static curing value to force for all fires
  StaticCuring static_curing{};
  // Minimum rate of spread before fire is considered to be spreading (m/min)
  MathSize minimum_ros{0.0};
  // Maximum distance that the fire is allowed to spread in one step (# of cells)
  MathSize maximum_spread_distance{0.0};
  // Minimum Fine Fuel Moisture Code required for spread during the day
  MathSize minimum_ffmc{0.0};
  // Minimum Fine Fuel Moisture Code required for spread during the night
  MathSize minimum_ffmc_at_night{0.0};
  // Offset from UTC to use for entire simulation (hours)
  std::optional<DurationSize> utc_offset{};
  // Offset from sunrise at which the day is considered to start (hours)
  DurationSize offset_sunrise{0.0};
  // Offset from sunrise at which the day is considered to end (hours)
  DurationSize offset_sunset{0.0};
  // Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
  int default_percent_conifer{0};
  // Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
  int default_percent_dead_fir{0};
  // The maximum fire intensity for the 'low' range of intensity (kW/m)
  int intensity_max_low{0};
  // The maximum fire intensity for the 'moderate' range of intensity (kW/m)
  int intensity_max_moderate{0};
  // Confidence required before simulation stops (% / 100)
  ThresholdSize confidence_level{0.0};
  // Salt to use for random seeds
  size_t salt{0};
  // Maximum time simulation can run before it is ended and whatever results it has are used (s)
  size_t maximum_time_seconds{0};
  // Time between generating interim outputs (s)
  size_t interim_output_interval_seconds{0};
  // Minimum number of simulations that must run before stopping
  size_t minimum_simulation_count{0};
  // Minimum number of simulations with any spread that must run before stopping
  size_t minimum_active_simulation_count{0};
  // Maximum number of simulations before stopping and whatever results it has are used
  size_t maximum_simulation_count{0};
  // Weight to give to Scenario part of thresholds=
  ThresholdSize threshold_scenario_weight{0.0};
  // Weight to give to daily part of thresholds
  ThresholdSize threshold_daily_weight{0.0};
  // Weight to give to hourly part of thresholds
  ThresholdSize threshold_hourly_weight{0.0};
  // Root directory that raster inputs are stored in
  LazyPath raster_root{};
  // Name of file that defines fuel lookup table
  LazyFuelLookup fuel_lookup{};
  // Days to output probability contours for (1 is start date, 2 is day after, etc.)
  OutputDateOffsets output_date_offsets{};
  FwiWeather get_weather() const;
  // FIX: need to get rain since noon yesterday to start of this hourly weather
  Precipitation apcp_prev{Precipitation::Zero()};

public:
  // normal mode only variables
  // initial fire size (ha)
  std::optional<size_t> initial_size{};

public:
  // normal or test mode variables
  std::optional<tm> start_date{};
  std::optional<MathSize> latitude{};
  std::optional<MathSize> longitude{};

public:
  // test mode only variables
  // name of fuel to use for test
  std::optional<string> fuel_name{};
  // test every combination of settings for tests
  std::optional<bool> test_all{};
  // number of hours to run tests for
  std::optional<MathSize> hours{};

public:
  // test/surface mode variables
  std::optional<Ffmc> ffmc{};
  std::optional<Dmc> dmc{};
  std::optional<Dc> dc{};
  std::optional<MathSize> wind_direction{};
  std::optional<MathSize> wind_speed{};
  std::optional<SlopeSize> slope{};
  std::optional<AspectSize> aspect{};

private:
  string dir_binary_;
  string dir_root_;
  // unparsed and found settings
  std::pair<string_map<string>, string_map<string>> settings_;
};
}
#endif
