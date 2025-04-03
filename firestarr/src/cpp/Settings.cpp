/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include <filesystem>
#include "Settings.h"
#include "Trim.h"
namespace tbd::sim
{
template <class T>
static vector<T> parse_list(string str, T (*convert)(const string& s))
{
  vector<int> result{};
  // want format without spaces to work
  // OUTPUT_DATE_OFFSETS = [1,2,3,7,14]
  logging::check_fatal(str[0] != '[', "Expected list starting with '[");
  istringstream iss(str.substr(1));
  while (getline(iss, str, ','))
  {
    // need to make sure this isn't an empty list
    if (0 != strcmp("]", str.c_str()))
    {
      result.push_back(convert(str));
    }
  }
  return result;
}
/**
 * \brief Settings implementation class
 */
class SettingsImplementation
{
public:
  ~SettingsImplementation() = default;
  SettingsImplementation(const SettingsImplementation& rhs) = delete;
  SettingsImplementation(SettingsImplementation&& rhs) = delete;
  SettingsImplementation& operator=(const SettingsImplementation& rhs) = delete;
  SettingsImplementation& operator=(SettingsImplementation&& rhs) = delete;
  static SettingsImplementation& instance() noexcept;
  static SettingsImplementation& instance(bool check_loaded) noexcept;
  /**
   * \brief Set root directory and read settings from file
   * \param dirname Directory to use for settings and relative paths
   */
  void setRoot(const char* dirname) noexcept;
  /**
   * \brief Root directory that raster inputs are stored in
   * \return Root directory that raster inputs are stored in
   */
  [[nodiscard]] const char* rasterRoot() const noexcept
  {
    return raster_root_.c_str();
  }
  /**
   * \brief Fuel lookup table
   * \return Fuel lookup table
   */
  [[nodiscard]] const fuel::FuelLookup& fuelLookup() noexcept
  {
    if (nullptr == fuel_lookup_)
    {
      // do this here because it relies on instance being created already
      fuel_lookup_ = std::make_unique<fuel::FuelLookup>(fuel_lookup_table_file_.c_str());
      logging::check_fatal(nullptr == fuel_lookup_, "Fuel lookup table has not been loaded");
    }
    return *fuel_lookup_;
  }
  /**
   * \brief Minimum rate of spread before fire is considered to be spreading (m/min)
   * \return Minimum rate of spread before fire is considered to be spreading (m/min)
   */
  [[nodiscard]] MathSize minimumRos() const noexcept
  {
    return minimum_ros_;
  }
  void setMinimumRos(const MathSize value) noexcept
  {
    minimum_ros_ = value;
  }
  /**
   * \brief Maximum distance that the fire is allowed to spread in one step (# of cells)
   * \return Maximum distance that the fire is allowed to spread in one step (# of cells)
   */
  [[nodiscard]] constexpr MathSize maximumSpreadDistance() const noexcept
  {
    return maximum_spread_distance_;
  }
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the day
   * \return Minimum Fine Fuel Moisture Code required for spread during the day
   */
  [[nodiscard]] constexpr MathSize minimumFfmc() const noexcept
  {
    return minimum_ffmc_;
  }
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the night
   * \return Minimum Fine Fuel Moisture Code required for spread during the night
   */
  [[nodiscard]] constexpr MathSize minimumFfmcAtNight() const noexcept
  {
    return minimum_ffmc_at_night_;
  }
  /**
   * \brief Offset from sunrise at which the day is considered to start (hours)
   * \return Offset from sunrise at which the day is considered to start (hours)
   */
  [[nodiscard]] constexpr DurationSize offsetSunrise() const noexcept
  {
    return offset_sunrise_;
  }
  /**
   * \brief Offset from sunrise at which the day is considered to end (hours)
   * \return Offset from sunrise at which the day is considered to end (hours)
   */
  [[nodiscard]] constexpr DurationSize offsetSunset() const noexcept
  {
    return offset_sunset_;
  }
  /**
   * \brief Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
   * \return Percent of the stand that is composed of conifer (%)
   */
  [[nodiscard]] constexpr int defaultPercentConifer() const noexcept
  {
    return default_percent_conifer_;
  }
  /**
   * \brief Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
   * \return Percent of the stand that is composed of dead fir (NOT percent of the fir that is dead) (%)
   */
  [[nodiscard]] constexpr int defaultPercentDeadFir() const noexcept
  {
    return default_percent_dead_fir_;
  }
  /**
   * \brief The maximum fire intensity for the 'low' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'low' range of intensity (kW/m)
   */
  [[nodiscard]] constexpr int intensityMaxLow() const noexcept
  {
    return intensity_max_low_;
  }
  /**
   * \brief The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   */
  [[nodiscard]] constexpr int intensityMaxModerate() const noexcept
  {
    return intensity_max_moderate_;
  }
  /**
   * \brief Confidence required before simulation stops (% / 100)
   * \return Confidence required before simulation stops (% / 100)
   */
  [[nodiscard]] ThresholdSize confidenceLevel() const noexcept
  {
    return confidence_level_;
  }
  /**
   * \brief Set confidence required before simulation stops (% / 100)
   * \return Set confidence required before simulation stops (% / 100)
   */
  void setConfidenceLevel(const ThresholdSize value) noexcept
  {
    confidence_level_ = value;
  }
  void setRasterRoot(const char* dirname) noexcept
  {
    raster_root_ = dirname;
  }
  void setFuelLookupTable(const char* filename) noexcept
  {
    fuel_lookup_table_file_ = filename;
  }
  /**
   * \brief Static curing value
   * \return Static curing value
   */
  [[nodiscard]] int staticCuring() const noexcept
  {
    return static_curing_;
  }
  /**
   * \brief Set static curing value
   * \return Set static curing value
   */
  void setStaticCuring(const int value) noexcept
  {
    logging::check_fatal(0 > value || 100 < value, "Grass curing (%) must be in range [0-100] but got %d", value);
    static_curing_ = value;
    force_curing = true;
  }
  /**
   * \brief Maximum time simulation can run before it is ended and whatever results it has are used (s)
   * \return Maximum time simulation can run before it is ended and whatever results it has are used (s)
   */
  [[nodiscard]] size_t maximumTimeSeconds() const noexcept
  {
    return maximum_time_seconds_;
  }
  /**
   * \brief Set maximum time simulation can run before it is ended and whatever results it has are used (s)
   * \return Set maximum time simulation can run before it is ended and whatever results it has are used (s)
   */
  void setMaximumTimeSeconds(const size_t value) noexcept
  {
    maximum_time_seconds_ = value;
  }
  /**
   * \brief Maximum number of simulations that can run before it is ended and whatever results it has are used
   * \return Maximum number of simulations that can run before it is ended and whatever results it has are used
   */
  [[nodiscard]] constexpr size_t maximumCountSimulations() const noexcept
  {
    return maximum_count_simulations_;
  }
  /**
   * \brief Weight to give to Scenario part of thresholds
   * \return Weight to give to Scenario part of thresholds
   */
  [[nodiscard]] constexpr ThresholdSize thresholdScenarioWeight() const noexcept
  {
    return threshold_scenario_weight_;
  }
  /**
   * \brief Weight to give to daily part of thresholds
   * \return Weight to give to daily part of thresholds
   */
  [[nodiscard]] constexpr ThresholdSize thresholdDailyWeight() const noexcept
  {
    return threshold_daily_weight_;
  }
  /**
   * \brief Weight to give to hourly part of thresholds
   * \return Weight to give to hourly part of thresholds
   */
  [[nodiscard]] constexpr ThresholdSize thresholdHourlyWeight() const noexcept
  {
    return threshold_hourly_weight_;
  }
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  [[nodiscard]] vector<int> outputDateOffsets() const
  {
    return output_date_offsets_;
  }
  /**
   * \brief Set days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return None
   */
  void setOutputDateOffsets(const char* value)
  {
    output_date_offsets_ = parse_list<int>(value, [](const string& s) { return stoi(s); });
    max_date_offset_ = *std::max_element(output_date_offsets_.begin(), output_date_offsets_.end());
  }
  /**
   * \brief Whatever the maximum value in the date offsets is
   * \return Whatever the maximum value in the date offsets is
   */
  [[nodiscard]] constexpr int maxDateOffset() const noexcept
  {
    return max_date_offset_;
  }
private:
  /**
   * \brief Initialize object but don't load settings from file
   */
  explicit SettingsImplementation() noexcept;
  /**
   * \brief Directory used for settings and relative paths
   */
  string dir_root_;
  /**
   * \brief Mutex for parallel access
   */
  mutex mutex_;
  /**
   * \brief Root directory that raster inputs are stored in
   */
  string raster_root_;
  /**
   * \brief Name of file that defines fuel lookup table
   */
  string fuel_lookup_table_file_;
  /**
   * \brief fuel lookup table
   */
  unique_ptr<fuel::FuelLookup> fuel_lookup_ = nullptr;
  /**
   * \brief Minimum rate of spread before fire is considered to be spreading (m/min)
   */
  atomic<MathSize> minimum_ros_;
  /**
   * \brief Maximum distance that the fire is allowed to spread in one step (# of cells)
   */
  MathSize maximum_spread_distance_;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the day
   */
  MathSize minimum_ffmc_;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the night
   */
  MathSize minimum_ffmc_at_night_;
  /**
   * \brief Offset from sunrise at which the day is considered to start (hours)
   */
  DurationSize offset_sunrise_;
  /**
   * \brief Offset from sunrise at which the day is considered to end (hours)
   */
  DurationSize offset_sunset_;
  /**
   * \brief Confidence required before simulation stops (% / 100)
   */
  atomic<ThresholdSize> confidence_level_;
  /**
   * \brief Ignition position row
   */
  atomic<int> ign_row_ = 1;
  /**
   * \brief Ignition position col
   */
  atomic<int> ign_col_ = 1;
  /**
   * \brief Static curing value
   */
  atomic<int> static_curing_ = 75;
  /**
   * \brief Maximum time simulation can run before it is ended and whatever results it has are used (s)
   */
  atomic<size_t> maximum_time_seconds_;
  /**
   * @brief Maximum number of simulations that can run before it is ended and whatever results it has are used
   */
  size_t maximum_count_simulations_;
  /**
   * \brief Weight to give to Scenario part of thresholds
   */
  ThresholdSize threshold_scenario_weight_;
  /**
   * \brief Weight to give to daily part of thresholds
   */
  ThresholdSize threshold_daily_weight_;
  /**
   * \brief Weight to give to hourly part of thresholds
   */
  ThresholdSize threshold_hourly_weight_;
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  vector<int> output_date_offsets_;
  /**
   * \brief Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
   */
  int default_percent_conifer_;
  /**
   * \brief Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
   */
  int default_percent_dead_fir_;
  /**
   * \brief Whatever the maximum value in the date offsets is
   */
  int max_date_offset_;
  /**
   * \brief The maximum fire intensity for the 'low' range of intensity (kW/m)
   */
  int intensity_max_low_;
  /**
   * \brief The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   */
  int intensity_max_moderate_;
public:
  /**
   * \brief Whether or not to save individual grids
   * \return Whether or not to save individual grids
   */
  atomic<bool> save_individual = false;
  /**
   * \brief Whether or not to run things asynchronously where possible
   * \return Whether or not to run things asynchronously where possible
   */
  atomic<bool> run_async = true;
  /**
   * \brief Whether or not to run deterministically (100% chance of spread & survival)
   * \return Whether or not to run deterministically (100% chance of spread & survival)
   */
  atomic<bool> deterministic = false;
  /**
   * \brief Whether or not to create a probability surface
   * \return Whether or not to create a probability surface
   */
  atomic<bool> surface = false;
  /**
   * \brief Whether or not to save grids as .asc
   * \return Whether or not to save grids as .asc
   */
  atomic<bool> save_as_ascii = false;
  /**
   * \brief Whether or not to save points used for spread
   * \return Whether or not to save points used for spread
   */
  atomic<bool> save_points = false;
  /**
   * \brief Whether or not to save intensity grids
   * \return Whether or not to save intensity grids
   */
  atomic<bool> save_intensity = true;
  /**
   * \brief Whether or not to save probability grids
   * \return Whether or not to save probability grids
   */
  atomic<bool> save_probability = true;
  /**
   * \brief Whether or not to save occurrence grids
   * \return Whether or not to save occurrence grids
   */
  atomic<bool> save_occurrence = false;
  /**
   * \brief Whether or not to save simulation area grids
   * \return Whether or not to save simulation area grids
   */
  atomic<bool> save_simulation_area = false;
  /**
   * \brief Whether or not to force greenup for all fires
   * \return Whether or not to force greenup for all fires
   */
  atomic<bool> force_greenup = false;
  /**
   * \brief Whether or not to force no greenup for all fires
   * \return Whether or not to force no greenup for all fires
   */
  atomic<bool> force_no_greenup = false;
  /**
   * \brief Whether or not to force static curing value for all fires
   * \return Whether or not to force static curing value for all fires
   */
  atomic<bool> force_curing = false;
  /**
   * \brief Whether or not the start point is specified by row and column id of a forced fuel grid
   * \return Whether or not the start point is specified by row and column id of a forced fuel grid
   */
  atomic<bool> rowcol_ignition = false;
};
/**
 * \brief The singleton instance for this class
 * \param check_loaded Whether to ensure a file has been loaded already
 * \return The singleton instance for this class
 */
SettingsImplementation& SettingsImplementation::instance(bool check_loaded) noexcept
{
  static SettingsImplementation instance_{};
  if (check_loaded)
  {
    logging::check_fatal(instance_.dir_root_.empty(), "Expected settings to be loaded, but no root directory specified yet");
  }
  return instance_;
}
/**
 * \brief The singleton instance for this class
 * \return The singleton instance for this class
 */
SettingsImplementation& SettingsImplementation::instance() noexcept
{
  return instance(true);
}
string get_value(unordered_map<string, string>& settings, const string& key)
{
  const auto found = settings.find(key);
  if (found != settings.end())
  {
    auto result = found->second;
    settings.erase(found);
    return result;
  }
  logging::fatal("Missing setting for %s", key.c_str());
  // HACK: use return to avoid compiler warning
  static const string Invalid = "INVALID";
  return Invalid;
}
string get_path(const char* const dir_root, unordered_map<string, string>& settings, const string& key)
{
  auto path = get_value(settings, key);
  if (!path.starts_with("/"))
  {
    // not an absolute path
    // if binary path starts with ./ then ignore it
    std::filesystem::path p = (0 == strcmp("./", dir_root)
                               || 0 == strcmp(".\\", dir_root))
                              ? path
                              : (dir_root + path);
#ifdef _WIN32
    path = std::filesystem::canonical(p).generic_string();
#else
    path = std::filesystem::canonical(p).c_str();
#endif
    logging::info("Converted relative path to absolute path %s", path.c_str());
  }
  return path;
}
SettingsImplementation::SettingsImplementation() noexcept
{
  dir_root_ = "";
}

void SettingsImplementation::setRoot(const char* dirname) noexcept
{
  try
  {
    dir_root_ = dirname;
    const auto filename = dir_root_ + "settings.ini";
    unordered_map<string, string> settings{};
    ifstream in;
    in.open(filename.c_str());
    if (in.is_open())
    {
      string str;
      logging::info("Reading settings from '%s'", filename.c_str());
      while (getline(in, str))
      {
        istringstream iss(str);
        if (getline(iss, str, '#'))
        {
          iss = istringstream(str);
        }
        if (getline(iss, str, '='))
        {
          const auto key = util::trim_copy(str);
          getline(iss, str, '\n');
          const auto value = util::trim_copy(str);
          settings.emplace(key, value);
          logging::debug("%s: %s", key.c_str(), value.c_str());
        }
      }
      in.close();
    }
    raster_root_ = get_path(dir_root_.c_str(), settings, "RASTER_ROOT");
    fuel_lookup_table_file_ = get_path(dir_root_.c_str(), settings, "FUEL_LOOKUP_TABLE");
    // HACK: run into fuel consumption being too low if we don't have a minimum ros
    static const auto MinRos = 0.05;
    // HACK: make sure this is always > 0 so that we don't have to check
    // specifically for 0 to avoid div error
    minimum_ros_ = max(stod(get_value(settings, "MINIMUM_ROS")), MinRos);
    maximum_spread_distance_ = stod(get_value(settings, "MAX_SPREAD_DISTANCE"));
    minimum_ffmc_ = stod(get_value(settings, "MINIMUM_FFMC"));
    minimum_ffmc_at_night_ = stod(get_value(settings, "MINIMUM_FFMC_AT_NIGHT"));
    offset_sunrise_ = stod(get_value(settings, "OFFSET_SUNRISE"));
    offset_sunset_ = stod(get_value(settings, "OFFSET_SUNSET"));
    confidence_level_ = stod(get_value(settings, "CONFIDENCE_LEVEL"));
    maximum_time_seconds_ = stol(get_value(settings, "MAXIMUM_TIME"));
    maximum_count_simulations_ = stol(get_value(settings, "MAXIMUM_SIMULATIONS"));
    threshold_scenario_weight_ = stod(get_value(settings, "THRESHOLD_SCENARIO_WEIGHT"));
    threshold_daily_weight_ = stod(get_value(settings, "THRESHOLD_DAILY_WEIGHT"));
    threshold_hourly_weight_ = stod(get_value(settings, "THRESHOLD_HOURLY_WEIGHT"));
    setOutputDateOffsets(get_value(settings, "OUTPUT_DATE_OFFSETS").c_str());
    default_percent_conifer_ = stoi(get_value(settings, "DEFAULT_PERCENT_CONIFER"));
    default_percent_dead_fir_ = stoi(get_value(settings, "DEFAULT_PERCENT_DEAD_FIR"));
    intensity_max_low_ = stoi(get_value(settings, "INTENSITY_MAX_LOW"));
    intensity_max_moderate_ = stoi(get_value(settings, "INTENSITY_MAX_MODERATE"));
    if (!settings.empty())
    {
      logging::warning("Unused settings in settings file %s", filename.c_str());
      for (const auto& kv : settings)
      {
        logging::warning("%s = %s", kv.first.c_str(), kv.second.c_str());
      }
    }
  }
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
    std::terminate();
  }
}
void Settings::setRoot(const char* dirname) noexcept
{
  return SettingsImplementation::instance(false).setRoot(dirname);
}
void Settings::setRasterRoot(const char* dirname) noexcept
{
  return SettingsImplementation::instance().setRasterRoot(dirname);
}
const char* Settings::rasterRoot() noexcept
{
  return SettingsImplementation::instance().rasterRoot();
}
void Settings::setFuelLookupTable(const char* filename) noexcept
{
  return SettingsImplementation::instance().setFuelLookupTable(filename);
}
const fuel::FuelLookup& Settings::fuelLookup() noexcept
{
  return SettingsImplementation::instance().fuelLookup();
}
bool Settings::saveIndividual() noexcept
{
  return SettingsImplementation::instance().save_individual;
}
void Settings::setSaveIndividual(const bool value) noexcept
{
  SettingsImplementation::instance().save_individual = value;
}
bool Settings::runAsync() noexcept
{
  return SettingsImplementation::instance().run_async;
}
void Settings::setRunAsync(const bool value) noexcept
{
  SettingsImplementation::instance().run_async = value;
}
bool Settings::deterministic() noexcept
{
  return SettingsImplementation::instance().deterministic;
}
bool Settings::surface() noexcept
{
  return SettingsImplementation::instance().surface;
}
void Settings::setSurface(const bool value) noexcept
{
  SettingsImplementation::instance().surface = value;
}
void Settings::setDeterministic(const bool value) noexcept
{
  SettingsImplementation::instance().deterministic = value;
}
bool Settings::saveAsAscii() noexcept
{
  return SettingsImplementation::instance().save_as_ascii;
}
void Settings::setSaveAsAscii(const bool value) noexcept
{
  SettingsImplementation::instance().save_as_ascii = value;
}
bool Settings::savePoints() noexcept
{
  return SettingsImplementation::instance().save_points;
}
void Settings::setSavePoints(const bool value) noexcept
{
  SettingsImplementation::instance().save_points = value;
}
bool Settings::saveIntensity() noexcept
{
  return SettingsImplementation::instance().save_intensity;
}
void Settings::setSaveIntensity(const bool value) noexcept
{
  SettingsImplementation::instance().save_intensity = value;
}
bool Settings::saveProbability() noexcept
{
  return SettingsImplementation::instance().save_probability;
}
void Settings::setSaveProbability(const bool value) noexcept
{
  SettingsImplementation::instance().save_probability = value;
}
bool Settings::saveOccurrence() noexcept
{
  return SettingsImplementation::instance().save_occurrence;
}
void Settings::setSaveOccurrence(const bool value) noexcept
{
  SettingsImplementation::instance().save_occurrence = value;
}
bool Settings::saveSimulationArea() noexcept
{
  return SettingsImplementation::instance().save_simulation_area;
}
void Settings::setSaveSimulationArea(const bool value) noexcept
{
  SettingsImplementation::instance().save_simulation_area = value;
}
bool Settings::forceGreenup() noexcept
{
  return SettingsImplementation::instance().force_greenup;
}
void Settings::setForceGreenup(const bool value) noexcept
{
  SettingsImplementation::instance().force_greenup = value;
}
bool Settings::forceNoGreenup() noexcept
{
  return SettingsImplementation::instance().force_no_greenup;
}
void Settings::setForceNoGreenup(const bool value) noexcept
{
  SettingsImplementation::instance().force_no_greenup = value;
}
bool Settings::forceStaticCuring() noexcept
{
  return SettingsImplementation::instance().force_curing;
}
int Settings::staticCuring() noexcept
{
  return SettingsImplementation::instance().staticCuring();
}
void Settings::setStaticCuring(const int value) noexcept
{
  SettingsImplementation::instance().setStaticCuring(value);
}
MathSize Settings::minimumRos() noexcept
{
  return SettingsImplementation::instance().minimumRos();
}
void Settings::setMinimumRos(const MathSize value) noexcept
{
  SettingsImplementation::instance().setMinimumRos(value);
}
MathSize Settings::maximumSpreadDistance() noexcept
{
  return SettingsImplementation::instance().maximumSpreadDistance();
}
MathSize Settings::minimumFfmc() noexcept
{
  return SettingsImplementation::instance().minimumFfmc();
}
MathSize Settings::minimumFfmcAtNight() noexcept
{
  return SettingsImplementation::instance().minimumFfmcAtNight();
}
DurationSize Settings::offsetSunrise() noexcept
{
  return SettingsImplementation::instance().offsetSunrise();
}
DurationSize Settings::offsetSunset() noexcept
{
  return SettingsImplementation::instance().offsetSunset();
}
int Settings::defaultPercentConifer() noexcept
{
  return SettingsImplementation::instance().defaultPercentConifer();
}
int Settings::defaultPercentDeadFir() noexcept
{
  return SettingsImplementation::instance().defaultPercentDeadFir();
}
int Settings::intensityMaxLow() noexcept
{
  return SettingsImplementation::instance().intensityMaxLow();
}
int Settings::intensityMaxModerate() noexcept
{
  return SettingsImplementation::instance().intensityMaxModerate();
}
ThresholdSize Settings::confidenceLevel() noexcept
{
  return SettingsImplementation::instance().confidenceLevel();
}
void Settings::setConfidenceLevel(const ThresholdSize value) noexcept
{
  SettingsImplementation::instance().setConfidenceLevel(value);
}
size_t Settings::maximumTimeSeconds() noexcept
{
  return SettingsImplementation::instance().maximumTimeSeconds();
}
void Settings::setMaximumTimeSeconds(const size_t value) noexcept
{
  return SettingsImplementation::instance().setMaximumTimeSeconds(value);
}
size_t Settings::maximumCountSimulations() noexcept
{
  return SettingsImplementation::instance().maximumCountSimulations();
}
ThresholdSize Settings::thresholdScenarioWeight() noexcept
{
  return SettingsImplementation::instance().thresholdScenarioWeight();
}
ThresholdSize Settings::thresholdDailyWeight() noexcept
{
  return SettingsImplementation::instance().thresholdDailyWeight();
}
ThresholdSize Settings::thresholdHourlyWeight() noexcept
{
  return SettingsImplementation::instance().thresholdHourlyWeight();
}
vector<int> Settings::outputDateOffsets()
{
  return SettingsImplementation::instance().outputDateOffsets();
}
void Settings::setOutputDateOffsets(const char* value)
{
  SettingsImplementation::instance().setOutputDateOffsets(value);
}
int Settings::maxDateOffset() noexcept
{
  return SettingsImplementation::instance().maxDateOffset();
}
}
