/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Settings.h"
#include "FuelLookup.h"
#include "Log.h"
#include "TimeUtil.h"
#include "Trim.h"
#include "Util.h"
namespace fs::settings
{
static mutex MUTEX{};
unique_ptr<Settings> INSTANCE{nullptr};
Settings& instance() noexcept
{
  scoped_lock lock{MUTEX};
  static const bool not_initialized = nullptr == INSTANCE;
  if (not_initialized)
  {
    exit(logging::fatal("Expected settings to be loaded, but no root directory specified yet"));
  }
  return *INSTANCE;
};
template <class T>
static vector<T> parse_list(string str, T (*convert)(const string s))
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
OutputDateOffsets::OutputDateOffsets(const string value) noexcept
  : text_(value),
    output_date_offsets_{parse_list<int>(value, [](const string s) { return stoi(s); })},
    max_date_offset_{*std::max_element(output_date_offsets_.begin(), output_date_offsets_.end())}
{ }
string get_value(
  std::pair<string_map<string>, string_map<string>>& settings,
  const string_view key,
  const bool required = true
)
{
  auto& settings_unparsed = settings.first;
  auto& settings_found = settings.second;
  const auto found = settings_unparsed.find(key);
  if (found != settings_unparsed.end())
  {
    auto result = found->second;
    settings_unparsed.erase(found);
    // if empty then same as no value
    if ("" != result)
    {
      settings_found.emplace(key, result);
      return result;
    }
  }
  if (required)
  {
    exit(logging::fatal("Missing setting for {:s}", string(key).c_str()));
  }
  // HACK: use return to avoid compiler warning
  static const auto Invalid = "INVALID";
  return Invalid;
}
bool get_flag(
  const bool default_value,
  std::pair<string_map<string>, string_map<string>>& settings,
  const string key
)
{
  constexpr auto required = false;
  auto value = get_value(settings, key, required);
  if ("INVALID" == value)
  {
    return default_value;
  }
  if ("1" == value)
  {
    return true;
  }
  if ("0" == value)
  {
    return false;
  }
  exit(
    logging::fatal("Only valid values for {:s} are 0 (false) or 1 (true) but got {:s}", key, value)
  );
}
constexpr string to_string(const Mode mode)
{
  switch (mode)
  {
    case Mode::Simulation:
      return "SIMULATION";
    case Mode::Test:
      return "TEST";
    case Mode::Surface:
      return "SURFACE";
  }
  exit(logging::fatal("Mode not handled"));
};
string ModeOptions()
{
  static const auto MODE_OPTIONS{std::format(
    "[{}, {}, {}]", to_string(Mode::Simulation), to_string(Mode::Test), to_string(Mode::Surface)
  )};
  return MODE_OPTIONS;
}
Mode get_mode(
  const Mode default_value,
  std::pair<string_map<string>, string_map<string>>& settings,
  const string key
)
{
  constexpr auto required = false;
  auto value = tolower(get_value(settings, key, required));
  if ("invalid" == value)
  {
    return default_value;
  }
  if ("simulation" == value)
  {
    return Mode::Simulation;
  }
  if ("surface" == value)
  {
    return Mode::Surface;
  }
  if ("test" == value)
  {
    return Mode::Test;
  }
  exit(logging::fatal(
    "Only valid value for {:s} is one of {:s} but got {:s}", key, ModeOptions(), value
  ));
}
const string Settings::getRoot() const noexcept { return dir_root_; }
const string Settings::getBinaryDirectory() const noexcept { return dir_binary_; }
void Settings::setRoot(const string dir_binary, const string dir_root) noexcept
{
  scoped_lock lock{MUTEX};
  logging::check_fatal(nullptr != INSTANCE, "Settings already initialized from file");
  INSTANCE = make_unique<Settings>(dir_binary, dir_root.empty() ? "." : dir_root);
}
Settings::Settings(const string dir_binary, const string dir_root) noexcept
  : dir_binary_{dir_binary}, dir_root_{dir_root}
{
  try
  {
    auto dir_settings = dir_root_;
    auto filename = dir_root_ + "settings.ini";
    if (!std::filesystem::exists(filename))
    {
      logging::debug("No exisiting settings file to read at {:s}", filename);
      // ensure directory exists and then copy settings.ini from beside binary to start
      const auto default_settings = dir_binary + "settings.ini";
      if (!std::filesystem::exists(default_settings))
      {
        exit(logging::fatal("No default settings found at {:s}", default_settings));
      }
      dir_settings = dir_binary;
      filename = default_settings;
      // logging::note(
      //   "Copying default settings from {:s} to {:s}", default_settings.c_str(), filename.c_str()
      // );
      // make_directory_recursive(dir_root_.c_str());
      // std::filesystem::copy(default_settings, filename);
    }
    if (!std::filesystem::exists(filename))
    {
      exit(logging::fatal("Unable to read settings from {:s}", filename));
    }
    logging::debug("Initializing settings from {:s}", filename);
    settings_ =
      make_pair<string_map<string>, string_map<string>>(string_map<string>{}, string_map<string>{});
    ;
    ifstream in;
    in.open(filename.c_str());
    if (in.is_open())
    {
      string str;
      logging::info("Reading settings from '{:s}'", filename);
      while (getline(in, str))
      {
        istringstream iss(str);
        if (getline(iss, str, '#'))
        {
          iss = istringstream(str);
        }
        if (getline(iss, str, '='))
        {
          const auto key = trim_copy(str);
          getline(iss, str, '\n');
          const auto value = trim_copy(str);
          settings_.first.emplace(key, value);
          logging::debug("{:s}: {:s}", key, value);
        }
      }
      in.close();
    }
    if (settings_.first.empty())
    {
      // could work if all arguments were specified as cli args (not currently possible)
      logging::warning("Settings file {:s} is empty", filename);
    }
    if (const auto value = get_value(settings_, "OUTPUT_DIRECTORY", false); "INVALID" != value)
    {
      output_directory = value;
    }
    else
    {
      if (dir_root_.empty())
      {
        dir_root_ = std::filesystem::current_path().generic_string();
      }
      output_directory = std::filesystem::absolute(dir_root_).lexically_normal().generic_string();
    }
    // HACK: resolve path when used to avoid failure when invalid but unused
    raster_root = LazyPath(dir_settings, get_value(settings_, "RASTER_ROOT"));
    fuel_lookup = LazyFuelLookup(dir_settings, get_value(settings_, "FUEL_LOOKUP_TABLE"));
    // HACK: run into fuel consumption being too low if we don't have a minimum ros
    static const auto MinRos = 0.05;
    // HACK: make sure this is always > 0 so that we don't have to check
    // specifically for 0 to avoid div error
    minimum_ros = max(stod(get_value(settings_, "MINIMUM_ROS")), MinRos);
    maximum_spread_distance = stod(get_value(settings_, "MAX_SPREAD_DISTANCE"));
    minimum_ffmc = stod(get_value(settings_, "MINIMUM_FFMC"));
    minimum_ffmc_at_night = stod(get_value(settings_, "MINIMUM_FFMC_AT_NIGHT"));
    offset_sunrise = stod(get_value(settings_, "OFFSET_SUNRISE"));
    offset_sunset = stod(get_value(settings_, "OFFSET_SUNSET"));
    confidence_level = stod(get_value(settings_, "CONFIDENCE_LEVEL"));
    maximum_time_seconds = stoul(get_value(settings_, "MAXIMUM_TIME"));
    interim_output_interval_seconds = stoul(get_value(settings_, "INTERIM_OUTPUT_INTERVAL"));
    minimum_simulation_count = stoul(get_value(settings_, "MINIMUM_SIMULATIONS"));
    minimum_active_simulation_count = stoul(get_value(settings_, "MINIMUM_ACTIVE_SIMULATIONS"));
    maximum_simulation_count = stoul(get_value(settings_, "MAXIMUM_SIMULATIONS"));
    threshold_scenario_weight = stod(get_value(settings_, "THRESHOLD_SCENARIO_WEIGHT"));
    threshold_daily_weight = stod(get_value(settings_, "THRESHOLD_DAILY_WEIGHT"));
    threshold_hourly_weight = stod(get_value(settings_, "THRESHOLD_HOURLY_WEIGHT"));
    output_date_offsets = OutputDateOffsets{get_value(settings_, "OUTPUT_DATE_OFFSETS")};
    default_percent_conifer = stoi(get_value(settings_, "DEFAULT_PERCENT_CONIFER"));
    default_percent_dead_fir = stoi(get_value(settings_, "DEFAULT_PERCENT_DEAD_FIR"));
    intensity_max_low = stoi(get_value(settings_, "INTENSITY_MAX_LOW"));
    intensity_max_moderate = stoi(get_value(settings_, "INTENSITY_MAX_MODERATE"));
    save_individual = get_flag(false, settings_, "SAVE_INDIVIDUAL");
    run_async = get_flag(true, settings_, "RUN_ASYNC");
    deterministic = get_flag(false, settings_, "DETERMINISTIC");
    mode = get_mode(Mode::Simulation, settings_, "MODE");
    save_as_ascii = get_flag(false, settings_, "SAVE_AS_ASCII");
    save_as_tiff = get_flag(true, settings_, "SAVE_AS_TIFF");
    save_points = get_flag(false, settings_, "SAVE_POINTS");
    save_intensity = get_flag(true, settings_, "SAVE_INTENSITY");
    save_probability = get_flag(true, settings_, "SAVE_PROBABILITY");
    save_occurrence = get_flag(false, settings_, "SAVE_OCCURRENCE");
    save_simulation_area = get_flag(false, settings_, "SAVE_SIMULATION_AREA");
    // FIX: have single GREENUP = setting ?
    force_greenup = get_flag(false, settings_, "FORCE_GREENUP");
    force_no_greenup = get_flag(false, settings_, "FORCE_NO_GREENUP");
    if (const auto value = get_value(settings_, "CURING", false); "INVALID" != value)
    {
      static_curing = stoi(value);
    }
    if (const auto value = get_value(settings_, "TZ", false); "INVALID" != value)
    {
      utc_offset = stod(value);
    }
    if (const auto value = get_value(settings_, "SALT", false); "INVALID" != value)
    {
      const int v = stoi(value);
      salt = static_cast<size_t>(abs(v));
      if (v < 0)
      {
        logging::warning("Negative salt value '{:d}' converted to positive value {:d}", v, salt);
      }
    }
    if (const auto value = get_value(settings_, "WX", false); "INVALID" != value)
    {
      wx_file_name = value;
    }
    if (const auto value = get_value(settings_, "LOG_FILE_NAME", false); "INVALID" != value)
    {
      log_file_name = value;
    }
    if (const auto value = get_value(settings_, "PERIMETER", false); "INVALID" != value)
    {
      perimeter = LazyPath{dir_settings, value};
    }
    if (const auto value = get_value(settings_, "FFMC", false); "INVALID" != value)
    {
      ffmc = Ffmc{stod(value)};
    }
    if (const auto value = get_value(settings_, "DMC", false); "INVALID" != value)
    {
      dmc = Dmc{stod(value)};
    }
    if (const auto value = get_value(settings_, "DC", false); "INVALID" != value)
    {
      dc = Dc{stod(value)};
    }
    if (const auto value = get_value(settings_, "TZ", false); "INVALID" != value)
    {
      utc_offset = stod(value);
    }
    if (const auto value = get_value(settings_, "WD", false); "INVALID" != value)
    {
      wind_direction = stod(value);
    }
    if (const auto value = get_value(settings_, "WS", false); "INVALID" != value)
    {
      wind_speed = stod(value);
    }
    // HACK: check for value that should only exist for specific mode
    auto get_value_if_mode =
      [&](const char* key, const Mode mode_required, const bool inverse = false) {
        const auto value = get_value(settings_, key, false);
        if ("INVALID" != value)
        {
          auto have_mode = mode_required == mode;
          if (inverse)
          {
            have_mode = !have_mode;
          }
          logging::check_fatal(
            !have_mode,
            "Value for {:s} found but mode is {:s} and needs to be{:s} {:s}",
            key,
            to_string(mode),
            inverse ? " not" : "",
            to_string(mode_required)
          );
        }
        return value;
      };
    auto get_value_if_not_mode = [&](const char* key, const Mode mode_required) {
      return get_value_if_mode(key, mode_required, true);
    };
    if (const auto value = get_value_if_mode("SIZE", Mode::Simulation); "INVALID" != value)
    {
      initial_size = stoi(value);
    }
    if (const auto value = get_value_if_mode("FUEL", Mode::Test); "INVALID" != value)
    {
      fuel_name = value;
    }
    if (const auto value = get_value_if_mode("TEST_ALL", Mode::Test); "INVALID" != value)
    {
      // FIX: duplicates effort
      test_all = get_flag(true, settings_, "TEST_ALL");
    }
    if (const auto value = get_value_if_mode("HOURS", Mode::Simulation); "INVALID" != value)
    {
      hours = stod(value);
    }
    if (const auto value = get_value_if_not_mode("START_DATE", Mode::Test); "INVALID" != value)
    {
      start_date = parse_utc(value);
    }
    if (const auto value = get_value_if_not_mode("START_TIME", Mode::Test); "INVALID" != value)
    {
      // NOTE: required START_DATE to be parsed previously
      add_time(start_date.value(), value);
    }
    if (const auto value = get_value_if_not_mode("LATITUDE", Mode::Test); "INVALID" != value)
    {
      latitude = stod(value);
    }
    if (const auto value = get_value_if_not_mode("LONGITUDE", Mode::Test); "INVALID" != value)
    {
      longitude = stod(value);
    }
    if (!settings_.first.empty())
    {
      if (logging::should_log(logging::level::warning))
      {
        logging::warning("Unused settings in settings file {:s}", filename);
        for (const auto& kv : settings_.first)
        {
          logging::warning("{:s} = {:s}", kv.first, kv.second);
        }
      }
    }
  }
  catch (const std::exception&)
  {
    // failed to read settings but just use defaults
  }
}
string format_section_header(const string& section, int width = 80, int side_width = 5)
{
  // const string hr = std::format("{:#^80}", "#");
  // const string side = std::format("{:#^5}", "#");
  string hr = std::format("{:#^{}}", "#", width);
  string side = std::format("{:#^{}}", "#", side_width);
  return std::format(
    "{}\n{}{: ^{}}{}\n{}\n", hr, side, section, width - (2 * side_width), side, hr
  );
}
string Settings::create(const pushd& dir) const
{
  stringstream buffer{};
  auto add_comment = [&](const string& comment) { buffer << "#" << comment << "\n"; };
  auto add_section = [&](const string& section) {
    auto header = format_section_header(section);
    buffer << header;
  };
  auto put = [&](const string& key, const string& comment, const auto& value) {
    // FIX: use original string instead of parsed and stringified input so nothing can change
    static constexpr auto MAX_PRECISION = std::numeric_limits<_Float64>::digits10 + 1;
    // make sure we don't lose precision or results will differ when run with settings file
    add_comment(" " + comment);
    buffer << key << " = " << std::setprecision(MAX_PRECISION) << value << "\n";
  };
  // HACK: just hardcode how this works since it's the opposite of parsing
  // // FIX: this should always just be whatever folder the settings file is in?
  // auto abs = std::filesystem::absolute(output_directory).lexically_normal();
  auto abs = dir.current_directory;
  auto relative = [&](const string& path) {
    // return std::filesystem::relative(path.c_str(), abs.c_str());
    return std::filesystem::relative(path.c_str()).generic_string();
    // return path;
  };
  add_section("SIMULATION INPUTS");
  // put("OUTPUT_DIRECTORY", "output directory", relative(output_directory).c_str());
  // HACK: output full path so if it isn't the same on read we can warn
  put(
    "OUTPUT_DIRECTORY",
    "output directory (warning if not the same as path for this file)",
    abs.c_str()
  );
  static const auto mode_help = std::format("simulation mode (one of {})", ModeOptions());
  put("MODE", mode_help, to_string(mode));
  if (!wx_file_name.empty())
  {
    auto wx_file = relative(wx_file_name.canonical());
    if (!file_exists(wx_file.c_str()))
    {
      throw std::runtime_error(
        std::format("Weather input file {} not found", wx_file_name.intended_path())
      );
    }
    put("WX", "weather file path", wx_file.c_str());
  }
  put("LOG_FILE_NAME", "log file name (created in output directoy)", log_file_name.c_str());
  if (!perimeter.empty())
  {
    put("PERIMETER", "perimeter to use for ignition", relative(perimeter.canonical()).c_str());
  }
  put(
    "RASTER_ROOT", "root directory to read rasters from", relative(raster_root.canonical()).c_str()
  );
  put(
    "FUEL_LOOKUP_TABLE",
    "lookup table for fuels (prometheus format)",
    relative(fuel_lookup.canonical()).c_str()
  );
  put(
    "DEFAULT_PERCENT_CONIFER",
    "default Percent Conifer (PC) for M1/M2 fuels where none is specified (%)",
    default_percent_conifer
  );
  put(
    "DEFAULT_PERCENT_DEAD_FIR",
    "default Percent Dead Fir (PDF) for M3/M4 fuels where none is specified (%)",
    default_percent_dead_fir
  );
  put("FORCE_GREENUP", "force greenup for entire simulation (0 = off, 1 = on)", force_greenup);
  put(
    "FORCE_NO_GREENUP", "force no greenup for entire simulation (0 = off, 1 = on)", force_no_greenup
  );
  put(
    "CURING",
    "static curing value to force for entire simulation (no value = off)",
    static_curing.as_string()
  );
  put("TZ", "offset from UTC to use for determining sunrise/sunset (hours)", utc_offset);
  put("SALT", "salt to use for random seeds", salt);
  if (Mode::Simulation == mode)
  {
    put("SIZE", "initial fire size (ha) (if no perimeter)", initial_size);
  }
  if (Mode::Test != mode)
  {
    put("START_DATE", "ignition start date (yyyy-mm-dd)", format_date(start_date.value()));
    put("START_TIME", "ignition start time (HH:MM)", format_time(start_date.value()));
    put(
      "LATITUDE", "latitude of area center & ignition if no perimeter (decimal degrees)", latitude
    );
    put(
      "LONGITUDE",
      "longitude of area center & ignition if no perimeter (decimal degrees)",
      longitude
    );
  }
  if (Mode::Test == mode)
  {
    put("TEST_ALL", "test every combination of settings for tests", test_all);
    put("FUEL", "fbp fuel type", fuel_name);
    put("HOURS", "number of hours to run tests for", hours);
  }
  // always need these for startup or constant indices
  put("FFMC", "fine fuel moisture code", ffmc);
  put("DMC", "duff moisture code", dmc);
  put("DC", "drought code", dc);
  if (Mode::Simulation != mode)
  {
    put("WD", "wind direction (degrees)", wind_direction);
    put("WS", "wind speed (km/h)", wind_speed);
  }
  if (Mode::Test == mode)
  {
    put("SLOPE", "ground slope (%)", slope);
    put("ASPECT", "ground aspect (degrees)", aspect);
  }
  /////////////////////////////////////////////////////////////////////////////
  add_section("SIMULATION OPTIONS");
  put(
    "MINIMUM_ROS",
    "minimum rate of spread before fire is considered to actually be spreading",
    minimum_ros
  );
  put(
    "MAX_SPREAD_DISTANCE",
    "maximum distance that the fire is allowed to spread in one step (# of cells)",
    maximum_spread_distance
  );
  put("MINIMUM_FFMC", "minimum ffmc for fire to spread", minimum_ffmc);
  put("MINIMUM_FFMC_AT_NIGHT", "minimum ffmc for fire to spread at night", minimum_ffmc_at_night);
  put(
    "OFFSET_SUNRISE",
    "offset from sunrise at which the day is considered to start (hours)",
    offset_sunrise
  );
  put(
    "OFFSET_SUNSET",
    "offset from sunrise at which the day is considered to end (hours)",
    offset_sunset
  );
  put(
    "THRESHOLD_SCENARIO_WEIGHT",
    "weight given to scenario when generating random thresholds",
    threshold_scenario_weight
  );
  put(
    "THRESHOLD_DAILY_WEIGHT",
    "weight given to daily when generating random thresholds",
    threshold_daily_weight
  );
  put(
    "THRESHOLD_HOURLY_WEIGHT",
    "weight given to hourly when generating random thresholds",
    threshold_hourly_weight
  );
  /////////////////////////////////////////////////////////////////////////////
  add_section("OUTPUT OPTIONS");
  put("OUTPUT_DATE_OFFSETS", "days to output probability contours for", output_date_offsets.text());
  put("SAVE_AS_ASCII", "save grids as .asc (0 = off, 1 = on)", save_as_ascii);
  put("SAVE_AS_TIFF", "to save grids as .tif (0 = off, 1 = on)", save_as_tiff);
  put("SAVE_POINTS", "save points used for spread (0 = off, 1 = on)", save_points);
  put("SAVE_INTENSITY", "save intensity grids (0 = off, 1 = on)", save_intensity);
  put("SAVE_PROBABILITY", "save probability grids (0 = off, 1 = on)", save_probability);
  put("SAVE_OCCURRENCE", "save occurrence grids (0 = off, 1 = on)", save_occurrence);
  put("SAVE_SIMULATION_AREA", "save simulation area grids (0 = off, 1 = on)", save_simulation_area);
  put(
    "INTENSITY_MAX_LOW",
    "maximum fire intensity for the 'low' range of intensity (kW/m)",
    intensity_max_low
  );
  put(
    "INTENSITY_MAX_MODERATE",
    "maximum fire intensity for the 'moderate' range of intensity (kW/m)",
    intensity_max_moderate
  );
  put("SAVE_INDIVIDUAL", "save individual grids (0 = off, 1 = on)", save_individual);
  /////////////////////////////////////////////////////////////////////////////
  add_section("EXECUTION OPTIONS");
  put("RUN_ASYNC", "run things asynchronously where possible (0 = off, 1 = on)", run_async);
  put(
    "DETERMINISTIC",
    "run deterministically (100% chance of spread & survival)  (0 = off, 1 = on)",
    deterministic
  );
  put(
    "CONFIDENCE_LEVEL",
    "confidence required before simulation stops (1.0 - (% / 100))",
    confidence_level
  );
  put(
    "INTERIM_OUTPUT_INTERVAL",
    "time between generating interim outputs (seconds) (0 is no interim outputs)",
    interim_output_interval_seconds
  );
  put(
    "MAXIMUM_TIME",
    "maximum amount of time to take for simulation (seconds) (0 is unlimited)",
    maximum_time_seconds
  );
  put(
    "MINIMUM_SIMULATIONS",
    "minimum number of simulations (0 == 1 simulation per scenario)",
    minimum_simulation_count
  );
  put(
    "MINIMUM_ACTIVE_SIMULATIONS",
    "minimum number of simulations where any spread occurs",
    minimum_active_simulation_count
  );
  put("MAXIMUM_SIMULATIONS", "maximum number of simulations to do", maximum_simulation_count);
  return buffer.str();
}
void Settings::saveTo(const string& output_directory) const noexcept
{
  try
  {
    make_directory_recursive(output_directory);
    // HACK: create() might depend on being in directory
    const pushd dir{output_directory};
    const auto contents = create(dir);
    const auto filename = "settings.ini";
    try
    {
      ofstream out{filename};
      out << contents;
    }
    catch (std::exception& ex)
    {
      if (file_exists(filename))
      {
        std::filesystem::remove(filename);
      }
      throw ex;
    }
  }
  catch (std::exception& ex)
  {
    exit(logging::fatal(ex, "Unable to output settings file"));
  }
}
FwiWeather Settings::get_weather() const
{
  return {
    Weather{
      Temperature::Zero(),
      RelativeHumidity::Zero(),
      Wind{
        Speed{wind_speed.value_or(Speed::Invalid().value)},
        Direction{Degrees{wind_direction.value_or(Direction::Invalid().value)}}
      },
      {apcp_prev}
    },
    ffmc.value_or(Ffmc::Invalid()),
    dmc.value_or(Dmc::Invalid()),
    dc.value_or(Dc::Invalid())
  };
}
}
