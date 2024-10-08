/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

/*! \mainpage FireSTARR Documentation
 *
 * \section intro_sec Introduction
 *
 * FireSTARR is a probabilistic fire growth model.
 */
#include "stdafx.h"
#include <chrono>
#include "Model.h"
#include "Scenario.h"
#include "Test.h"
#include "TimeUtil.h"
#include "Log.h"
#include "version.h"
using tbd::logging::Log;
using tbd::sim::Settings;
static const char* BIN_NAME = nullptr;
static map<std::string, std::function<void()>> PARSE_FCT{};
static vector<std::pair<std::string, std::string>> PARSE_HELP{};
static map<std::string, bool> PARSE_REQUIRED{};
static map<std::string, bool> PARSE_HAVE{};
static int ARGC = 0;
static const char* const* ARGV = nullptr;
static int CUR_ARG = 0;
string get_args()
{
  std::string args(ARGV[0]);
  for (auto i = 1; i < ARGC; ++i)
  {
    args.append(" ");
    args.append(ARGV[i]);
  }
  return args;
}
void show_args()
{
  auto args = get_args();
  printf("Arguments are:\n%s\n", args.c_str());
}
void log_args()
{
  auto args = get_args();
  tbd::logging::note("Arguments are:\n%s\n", args.c_str());
}
void show_usage_and_exit(int exit_code)
{
  printf("Usage: %s <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]\n\n", BIN_NAME);
  printf("Run simulations and save output in the specified directory\n\n\n");
  printf("Usage: %s surface <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]\n\n", BIN_NAME);
  printf("Calculate probability surface and save output in the specified directory\n\n\n");
  printf("Usage: %s test <output_dir> <numHours> [slope [aspect [wind_speed [wind_direction]]]]\n\n", BIN_NAME);
  printf(" Run test cases and save output in the specified directory\n\n");
  printf(" Input Options\n");
  for (auto& kv : PARSE_HELP)
  {
    printf("   %-25s %s\n", kv.first.c_str(), kv.second.c_str());
  }
  exit(exit_code);
}
void show_usage_and_exit()
{
  show_args();
  show_usage_and_exit(-1);
}
void show_help_and_exit()
{
  // showing help isn't an error
  show_usage_and_exit(0);
}
const char* get_arg() noexcept
{
  // check if we don't have any more arguments
  tbd::logging::check_fatal(CUR_ARG + 1 >= ARGC, "Missing argument to --%s", ARGV[CUR_ARG]);
  // check if we have another flag right after
  tbd::logging::check_fatal('-' == ARGV[CUR_ARG + 1][0],
                            "Missing argument to --%s",
                            ARGV[CUR_ARG]);
  return ARGV[++CUR_ARG];
}
template <class T>
T parse(std::function<T()> fct)
{
  PARSE_HAVE.emplace(ARGV[CUR_ARG], true);
  return fct();
}
template <class T>
T parse_once(std::function<T()> fct)
{
  if (PARSE_HAVE.contains(ARGV[CUR_ARG]))
  {
    printf("\nArgument %s already specified\n\n", ARGV[CUR_ARG]);
    show_usage_and_exit();
  }
  return parse(fct);
}
bool parse_flag(bool not_inverse)
{
  return parse_once<bool>([not_inverse] { return not_inverse; });
}
MathSize parse_value()
{
  return parse_once<MathSize>([] { return stod(get_arg()); });
}
size_t parse_size_t()
{
  return parse_once<size_t>([] { return static_cast<size_t>(stoi(get_arg())); });
}
const char* parse_raw()
{
  return parse_once<const char*>(&get_arg);
}
string parse_string()
{
  return string(parse_raw());
}
template <class T>
T parse_index()
{
  // return T(parse_value());
  return parse_once<T>([] { return T(stod(get_arg())); });
}
// template <class T>
// T parse_int_index()
// {
//   return T(static_cast<int>(parse_size_t()));
//   // return parse_once<T>([] { return T(stoi(get_arg())); });
// }
void register_argument(string v, string help, bool required, std::function<void()> fct)
{
  PARSE_FCT.emplace(v, fct);
  PARSE_HELP.emplace_back(v, help);
  PARSE_REQUIRED.emplace(v, required);
}
template <class T>
void register_setter(std::function<void(T)> fct_set, string v, string help, bool required, std::function<T()> fct)
{
  register_argument(v, help, required, [fct_set, fct] { fct_set(fct()); });
}
template <class T>
void register_setter(T& variable, string v, string help, bool required, std::function<T()> fct)
{
  register_argument(v, help, required, [&variable, fct] { variable = fct(); });
}
void register_flag(std::function<void(bool)> fct, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [not_inverse, fct] { fct(parse_flag(not_inverse)); });
}
void register_flag(bool& variable, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [not_inverse, &variable] { variable = parse_flag(not_inverse); });
}
template <class T>
void register_index(T& index, string v, string help, bool required)
{
  register_argument(v, help, required, [&index] { index = parse_index<T>(); });
}
// template <class T>
// void register_int_index(T& index, string v, string help, bool required)
// {
//   register_argument(v, help, required, [&index] { index = parse_int_index<T>(); });
// }
int main(const int argc, const char* const argv[])
{
  printf("FireSTARR %s <%s>\n\n", VERSION, COMPILE_DATE);
  tbd::debug::show_debug_settings();
  ARGC = argc;
  ARGV = argv;
  auto bin = string(ARGV[CUR_ARG++]);
  replace(bin.begin(), bin.end(), '\\', '/');
  const auto end = max(static_cast<size_t>(0), bin.rfind('/') + 1);
  const auto bin_dir = bin.substr(0, end);
  const auto bin_name = bin.substr(end, bin.size() - end);
  // printf("Binary is %s in directory %s\n", bin_name.c_str(), bin_dir.c_str());
  BIN_NAME = bin.c_str();
  Settings::setRoot(bin_dir.c_str());
  // _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  Log::setLogLevel(tbd::logging::LOG_NOTE);
  register_argument("-h", "Show help", false, &show_help_and_exit);
  // auto start_time = tbd::Clock::now();
  // auto time = tbd::Clock::now();
  // constexpr size_t n_test = 100000000;
  // for (size_t i = 0; i < n_test; ++i)
  // {
  //     time = tbd::Clock::now();
  // }
  // const auto run_time = time - start_time;
  // const auto run_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(run_time);
  // printf("Calling Clock::now() %ld times took %ld seconds",
  //                 n_test, run_time_seconds.count());
  // Calling Clock::now() 100000000 times took 2 seconds
  // real    0m2.737s
  // user    0m2.660s
  // sys     0m0.011s
  // return 0;
  string wx_file_name;
  string log_file_name = "firestarr.log";
  string perim;
  size_t size = 0;
  tbd::wx::Ffmc ffmc;
  tbd::wx::Dmc dmc;
  tbd::wx::Dc dc;
  size_t wind_direction = 0;
  size_t wind_speed = 0;

  // FIX: need to get rain since noon yesterday to start of this hourly weather
  tbd::wx::Precipitation apcp_prev;
  // can be used multiple times
  register_argument("-v", "Increase output level", false, &Log::increaseLogLevel);
  // if they want to specify -v and -q then that's fine
  register_argument("-q", "Decrease output level", false, &Log::decreaseLogLevel);
  auto result = -1;
  if (ARGC > 1 && 0 == strcmp(ARGV[1], "test"))
  {
    if (ARGC <= 3)
    {
      show_usage_and_exit();
    }
    result = tbd::sim::test(ARGC, ARGV);
  }
  else
  {
    register_flag(&Settings::setSaveIndividual, true, "-i", "Save individual maps for simulations");
    register_flag(&Settings::setRunAsync, false, "-s", "Run in synchronous mode");
    // register_flag(&Settings::setDeterministic, true, "--deterministic", "Run deterministically (100% chance of spread & survival)");
    // register_flag(&Settings::setSurface, true, "--surface", "Create a probability surface based on igniting every possible location in grid");
    register_flag(&Settings::setSaveAsAscii, true, "--ascii", "Save grids as .asc");
    register_flag(&Settings::setSavePoints, true, "--points", "Save simulation points to file");
    register_flag(&Settings::setSaveIntensity, false, "--no-intensity", "Do not output intensity grids");
    register_flag(&Settings::setSaveProbability, false, "--no-probability", "Do not output probability grids");
    register_flag(&Settings::setSaveOccurrence, true, "--occurrence", "Output occurrence grids");
    register_flag(&Settings::setSaveSimulationArea, true, "--sim-area", "Output simulation area grids");
    register_flag(&Settings::setForceFuel, true, "--force-fuel", "Use first default fuel raster without checking coordinates");
    register_setter<string>(log_file_name, "--log", "Output log file", false, &parse_string);
    size_t SKIPPED_ARGS = 0;
    if (ARGC > 1 && 0 == strcmp(ARGV[1], "surface"))
    {
      tbd::logging::note("Running in probability surface mode");
      // skip 'surface' argument if present
      CUR_ARG += 1;
      SKIPPED_ARGS = 1;
      // probabalistic surface is computationally impossible at this point
      Settings::setDeterministic(true);
      Settings::setSurface(true);
      register_index<tbd::wx::Ffmc>(ffmc, "--ffmc", "Constant Fine Fuel Moisture Code", true);
      register_index<tbd::wx::Dmc>(dmc, "--dmc", "Constant Duff Moisture Code", true);
      register_index<tbd::wx::Dc>(dc, "--dc", "Constant Drought Code", true);
      // register_int_index<tbd::wx::Direction>(wind_direction, "--wd", "Constant wind direction", true);
      // register_setter<tbd::wx::Direction>(wind_direction, "--wd", "Constant wind direction", true, []() {
      //   return parse_once<tbd::wx::Direction>([] { return tbd::wx::Direction(stoi(get_arg()), false); });
      // });
      register_setter<size_t>(wind_direction, "--wd", "Constant wind direction", true, &parse_size_t);
      register_setter<size_t>(wind_speed, "--ws", "Constant wind speed", true, &parse_size_t);
    }
    else
    {
      register_setter<string>(wx_file_name, "--wx", "Input weather file", true, &parse_string);
      register_flag(&Settings::setDeterministic, true, "--deterministic", "Run deterministically (100% chance of spread & survival)");
      register_flag(&Settings::setRowColIgnition, true, "--rowcol-ignition", "Use row and col to specific start point. Assumes force-fuel is set.");
      register_setter<size_t>(&Settings::setIgnRow, "--ign-row", "Specify ignition row", false, &parse_size_t);
      register_setter<size_t>(&Settings::setIgnCol, "--ign-col", "Specify ignition column", false, &parse_size_t);
      register_setter<tbd::ThresholdSize>(&Settings::setConfidenceLevel, "--confidence", "Use specified confidence level", false, &parse_value);
      register_setter<string>(perim, "--perim", "Start from perimeter", false, &parse_string);
      register_setter<size_t>(size, "--size", "Start from size", false, &parse_size_t);
      // HACK: want different text for same flag so define here too
      register_index<tbd::wx::Ffmc>(ffmc, "--ffmc", "Startup Fine Fuel Moisture Code", true);
      register_index<tbd::wx::Dmc>(dmc, "--dmc", "Startup Duff Moisture Code", true);
      register_index<tbd::wx::Dc>(dc, "--dc", "Startup Drought Code", true);
      register_index<tbd::wx::Precipitation>(apcp_prev, "--apcp_prev", "Startup precipitation between 1200 yesterday and start of hourly weather", false);
    }
    register_setter<const char*>(&Settings::setOutputDateOffsets, "--output_date_offsets", "Override output date offsets", false, &parse_raw);
    if (2 == ARGC && 0 == strcmp(ARGV[CUR_ARG], "-h"))
    {
      // HACK: just do this for now
      show_help_and_exit();
    }
    else if (3 > (ARGC - SKIPPED_ARGS))
    {
      show_usage_and_exit();
    }
#ifdef NDEBUG
    try
    {
#endif
      if (6 <= (ARGC - SKIPPED_ARGS))
      {
        string output_directory(ARGV[CUR_ARG++]);
        replace(output_directory.begin(), output_directory.end(), '\\', '/');
        if ('/' != output_directory[output_directory.length() - 1])
        {
          output_directory += '/';
        }
        const char* dir_out = output_directory.c_str();
        struct stat info
        {
        };
        if (stat(dir_out, &info) != 0 || !(info.st_mode & S_IFDIR))
        {
          tbd::util::make_directory_recursive(dir_out);
        }
        // FIX: this just doesn't work because --log isn't parsed until later
        // if name starts with "/" then it's an absolute path, otherwise append to working directory
        const string log_file = log_file_name.starts_with("/") ? log_file_name : (output_directory + log_file_name);
        tbd::logging::check_fatal(!Log::openLogFile(log_file.c_str()),
                                  "Can't open log file %s",
                                  log_file.c_str());
        tbd::logging::note("Output directory is %s", dir_out);
        tbd::logging::note("Output log is %s", log_file.c_str());
        string date(ARGV[CUR_ARG++]);
        tm start_date{};
        start_date.tm_year = stoi(date.substr(0, 4)) - 1900;
        start_date.tm_mon = stoi(date.substr(5, 2)) - 1;
        start_date.tm_mday = stoi(date.substr(8, 2));
        const auto latitude = stod(ARGV[CUR_ARG++]);
        const auto longitude = stod(ARGV[CUR_ARG++]);
        const tbd::topo::StartPoint start_point(latitude, longitude);
        size_t num_days = 0;
        string arg(ARGV[CUR_ARG++]);
        tm start{};
        if (5 == arg.size() && ':' == arg[2])
        {
          try
          {
            // if this is a time then we aren't just running the weather
            start_date.tm_hour = stoi(arg.substr(0, 2));
            tbd::logging::check_fatal(start_date.tm_hour < 0 || start_date.tm_hour > 23,
                                      "Simulation start time has an invalid hour (%d)",
                                      start_date.tm_hour);
            start_date.tm_min = stoi(arg.substr(3, 2));
            tbd::logging::check_fatal(start_date.tm_min < 0 || start_date.tm_min > 59,
                                      "Simulation start time has an invalid minute (%d)",
                                      start_date.tm_min);
            tbd::logging::note("Simulation start time before fix_tm() is %d-%02d-%02d %02d:%02d",
                               start_date.tm_year + 1900,
                               start_date.tm_mon + 1,
                               start_date.tm_mday,
                               start_date.tm_hour,
                               start_date.tm_min);
            tbd::util::fix_tm(&start_date);
            tbd::logging::note("Simulation start time after fix_tm() is %d-%02d-%02d %02d:%02d",
                               start_date.tm_year + 1900,
                               start_date.tm_mon + 1,
                               start_date.tm_mday,
                               start_date.tm_hour,
                               start_date.tm_min);
            // we were given a time, so number of days is until end of year
            start = start_date;
            const auto start_t = mktime(&start);
            auto year_end = start;
            year_end.tm_mon = 11;
            year_end.tm_mday = 31;
            const auto seconds = difftime(mktime(&year_end), start_t);
            // start day counts too, so +1
            // HACK: but we don't want to go to Jan 1 so don't add 1
            num_days = static_cast<size_t>(seconds / tbd::DAY_SECONDS);
            tbd::logging::debug("Calculated number of days until end of year: %d",
                                num_days);
            // +1 because day 1 counts too
            // +2 so that results don't change when we change number of days
            num_days = min(num_days, static_cast<size_t>(Settings::maxDateOffset()) + 2);
          }
          catch (std::exception&)
          {
            show_usage_and_exit();
          }
          while (CUR_ARG < ARGC)
          {
            if (PARSE_FCT.find(ARGV[CUR_ARG]) != PARSE_FCT.end())
            {
              try
              {
                PARSE_FCT[ARGV[CUR_ARG]]();
              }
              catch (std::exception&)
              {
                printf("\n'%s' is not a valid value for argument %s\n\n", ARGV[CUR_ARG], ARGV[CUR_ARG - 1]);
                show_usage_and_exit();
              }
            }
            else
            {
              show_usage_and_exit();
            }
            ++CUR_ARG;
          }
        }
        else
        {
          show_usage_and_exit();
        }
        for (auto& kv : PARSE_REQUIRED)
        {
          if (kv.second && PARSE_HAVE.end() == PARSE_HAVE.find(kv.first))
          {
            tbd::logging::fatal("%s must be specified", kv.first.c_str());
          }
        }
        if (!PARSE_HAVE.contains("--apcp_prev"))
        {
          tbd::logging::warning("Assuming 0 precipitation between noon yesterday and weather start for startup indices");
          apcp_prev = tbd::wx::Precipitation::Zero;
        }
        // HACK: ISI for yesterday really doesn't matter so just use any wind
        // HACK: it's basically wrong to assign this precip to yesterday's object,
        // but don't want to add another argument right now
        const auto yesterday = tbd::wx::FwiWeather(tbd::wx::Temperature(0),
                                                   tbd::wx::RelativeHumidity(0),
                                                   tbd::wx::Wind(tbd::wx::Direction(wind_direction, false), tbd::wx::Speed(wind_speed)),
                                                   tbd::wx::Precipitation(apcp_prev),
                                                   ffmc,
                                                   dmc,
                                                   dc);
        tbd::util::fix_tm(&start_date);
        tbd::logging::note("Simulation start time after fix_tm() again is %d-%02d-%02d %02d:%02d",
                           start_date.tm_year + 1900,
                           start_date.tm_mon + 1,
                           start_date.tm_mday,
                           start_date.tm_hour,
                           start_date.tm_min);
        start = start_date;
        log_args();
        result = tbd::sim::Model::runScenarios(output_directory,
                                               wx_file_name.c_str(),
                                               yesterday,
                                               Settings::rasterRoot(),
                                               start_point,
                                               start,
                                               perim,
                                               size);
        Log::closeLogFile();
      }
      else
      {
        show_usage_and_exit();
      }
#ifdef NDEBUG
    }
    catch (const std::exception& ex)
    {
      tbd::logging::fatal(ex);
      std::terminate();
    }
#endif
  }
  return result;
}
