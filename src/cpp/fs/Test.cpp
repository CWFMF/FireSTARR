/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Test.h"
#include "FireSpread.h"
#include "FireWeather.h"
#include "FuelLookup.h"
#include "Location.h"
#include "Log.h"
#include "Model.h"
#include "Observer.h"
#include "SafeVector.h"
#include "Settings.h"
#include "Util.h"
namespace fs
{
using fs::fuel::FuelLookup;
using settings::Settings;
/**
 * \brief An Environment with no elevation and the same value in every Cell.
 */
class TestEnvironment : public Environment
{
public:
  /**
   * \brief Environment with the same data in every cell
   * \param cells Constant cells
   */
  explicit TestEnvironment(CellGrid&& cells) noexcept
    : Environment(nullptr, nullptr, std::move(cells), 0)
  { }
};
/**
 * \brief A Scenario run with constant fuel, weather, and topography.
 */
class TestScenario final : public Scenario
{
public:
  ~TestScenario() override = default;
  TestScenario(const TestScenario& rhs) = delete;
  TestScenario(TestScenario&& rhs) = delete;
  TestScenario& operator=(const TestScenario& rhs) = delete;
  TestScenario& operator=(TestScenario&& rhs) = delete;
  /**
   * \brief Constructor
   * \param model Model running this Scenario
   * \param start_cell Cell to start ignition in
   * \param start_point StartPoint represented by start_cell
   * \param start_date Start date of simulation
   * \param end_date End data of simulation
   * \param weather Constant weather to use for duration of simulation
   */
  TestScenario(
    Model* model,
    const XYIdx& start_xy,
    const StartPoint& start_point,
    const int start_date,
    const DurationSize end_date,
    const ptr<const FireWeather> weather,
    ptr<SafeVector> final_sizes
  )
    : Scenario(
        model,
        1,
        weather,
        weather,
        start_date,
        nullptr,
        start_xy,
        start_point,
        static_cast<Day>(start_date),
        static_cast<Day>(end_date)
      )
  {
    registerObserver(new IntensityObserver(*this));
    registerObserver(new ArrivalObserver(*this));
    registerObserver(new SourceObserver(*this));
    addEvent(Event{.time = end_date, .type = Event::Type::EndSimulation});
    last_save_ = end_date;
    // cast to avoid warning
    std::ignore = reset(nullptr, nullptr, final_sizes);
  }
};
void showSpread(const SpreadInfo& spread, ptr<const FwiWeather> w, const FuelType* fuel)
{
  // HACK: make two rows and then print so columns are aligned
  std::stringstream line_header{};
  std::stringstream line_data{};
  auto add_value = [&](const char* col, const string value) {
    line_data << " " << value;
    line_header << std::format(" {:>{}s}", col, value.size());
  };
  add_value("PREC", std::format("{:5.2f}", w->prec.value));
  add_value("TEMP", std::format("{:5.1f}", w->temperature.value));
  add_value("RH", std::format("{:3g}", w->rh.value));
  add_value("WS", std::format("{:5.1f}", w->wind.speed.value));
  add_value("WD", std::format("{:3g}", w->wind.direction.value));
  add_value("FFMC", std::format("{:5.1f}", w->ffmc.value));
  add_value("DMC", std::format("{:5.1f}", w->dmc.value));
  add_value("DC", std::format("{:5g}", w->dc.value));
  add_value("ISI", std::format("{:5.1f}", w->isi.value));
  add_value("BUI", std::format("{:5.1f}", w->bui.value));
  add_value("FWI", std::format("{:5.1f}", w->fwi.value));
  add_value("GS", std::format("{:3d}", spread.percentSlope()));
  add_value("SAZ", std::format("{:3d}", spread.slopeAzimuth()));
  const auto simple_fuel = simplify_fuel_name(fuel->name());
  add_value("FUEL", std::format("{:>7s}", simple_fuel));
  add_value("GC", std::format("{:3.0g}", fuel->grass_curing(spread.nd(), *w)));
  add_value("L:B", std::format("{:5.2f}", spread.lengthToBreadth()));
  add_value("CBH", std::format("{:4.1f}", fuel->cbh()));
  add_value("CFB", std::format("{:6.3f}", spread.crownFractionBurned()));
  add_value("CFC", std::format("{:6.3f}", spread.crownFuelConsumption()));
  add_value("FD", std::format("{:2c}", spread.fireDescription()));
  add_value("HFI", std::format("{:6d}", static_cast<size_t>(spread.maxIntensity())));
  add_value("RAZ", std::format("{:3d}", spread.headDirection().asDegreesSize()));
  add_value("ROS", std::format("{:6.4g}", spread.headRos()));
  add_value("SFC", std::format("{:6.4g}", spread.surfaceFuelConsumption()));
  add_value("TFC", std::format("{:6.4g}", spread.totalFuelConsumption()));
  cout << std::format("Calculated spread is:\n{:s}\n{:s}\n", line_header.str(), line_data.str());
}
string generate_test_name(
  const auto& fuel,
  const SlopeSize slope,
  const AspectSize aspect,
  const fs::Wind& wind
)
{
  // wind speed & direction can be decimal values, but slope and aspect are int
  return std::format(
    "{:s}_S{:03d}_A{:03d}_WD{:05.1f}_WS{:05.1f}",
    simplify_fuel_name(fuel),
    slope,
    aspect,
    wind.direction.asDegrees(),
    wind.speed.value
  );
};
string run_test(
  const string_view base_directory,
  const string_view fuel_name,
  const SlopeSize slope,
  const AspectSize aspect,
  const DurationSize num_hours,
  const Dc& dc,
  const Dmc& dmc,
  const Ffmc& ffmc,
  const Wind& wind,
  ptr<SafeVector> final_sizes,
  const bool ignore_existing
)
{
  // HACK: resolve once and fail if not set already
  static auto& settings = fs::settings::instance();
  static const auto& lookup = settings.fuel_lookup.lookup();
  string test_name = generate_test_name(fuel_name, slope, aspect, wind);
  logging::verbose("Queueing test for {:s}", &(test_name[0]));
  const string output_directory = string(base_directory) + test_name + "/";
  if (ignore_existing && directory_exists(output_directory.c_str()))
  {
    // skip if directory exists
    logging::warning("Skipping existing directory {:s}", output_directory);
    return output_directory;
  }
  // delay instantiation so things only get made when executed
  static Semaphore num_concurrent{10 * static_cast<int>(std::thread::hardware_concurrency())};
  CriticalSection _(num_concurrent);
  // logging::debug("Concurrent test limit is {:d}", num_concurrent.limit());
  logging::note("Running test for {:s}", output_directory);
  static const StartPoint ForPoint(settings.latitude.value(), settings.longitude.value());
  const auto start_date = settings.start_date.value().tm_yday;
  const auto end_date = start_date + static_cast<DurationSize>(num_hours) / DAY_HOURS;
  make_directory_recursive(output_directory);
  const auto fuel = lookup.bySimplifiedName(simplify_fuel_name(fuel_name));
  auto values = vector<Cell>();
  for (Idx y = 0; y < MAX_HEIGHT; ++y)
  {
    for (Idx x = 0; x < MAX_WIDTH; ++x)
    {
      values.emplace_back(slope, aspect, FuelType::safeCode(fuel));
    }
  }
  const Cell cell_nodata{};
  TestEnvironment env{CellGrid{
    TEST_GRID_SIZE,
    MAX_WIDTH,
    MAX_HEIGHT,
    cell_nodata.fullHash(),
    cell_nodata,
    TEST_XLLCORNER,
    TEST_YLLCORNER,
    TEST_XLLCORNER + TEST_GRID_SIZE * MAX_WIDTH,
    TEST_YLLCORNER + TEST_GRID_SIZE * MAX_HEIGHT,
    TEST_PROJ4,
    std::move(values)
  }};
  const XYIdx start_xy{static_cast<Idx>(MAX_WIDTH / 2), static_cast<Idx>(MAX_HEIGHT / 2)};
  Model model(settings.start_date.value(), output_directory, ForPoint, &env);
  const auto start_cell = model.cell(start_xy);
  FireWeather weather(fuel, start_date, dc, dmc, ffmc, wind);
  TestScenario scenario(&model, start_xy, ForPoint, start_date, end_date, &weather, final_sizes);
  const auto w = weather.at(start_date);
  auto info = SpreadInfo(scenario, start_date, start_cell.key(), model.nd(start_date), w);
  showSpread(info, w, fuel);
  map<DurationSize, shared_ptr<ProbabilityMap>> probabilities{};
  logging::debug("Starting simulation");
  // NOTE: don't want to reset first because TestScenabuirio handles what that does
  scenario.run(&probabilities);
  logging::note("Saving results for {:s} in {:s}", test_name, output_directory);
  std::ignore = scenario.saveObservers(output_directory, test_name);
  logging::note("Final Size: {:0.0f}, ROS: {:0.2f}", scenario.currentFireSize(), info.headRos());
  return string(output_directory);
}
string run_test_ignore_existing(
  const string_view output_directory,
  const string_view fuel_name,
  const SlopeSize slope,
  const AspectSize aspect,
  const DurationSize num_hours,
  const Dc& dc,
  const Dmc& dmc,
  const Ffmc& ffmc,
  const Wind& wind,
  ptr<SafeVector> final_sizes
)
{
  return run_test(
    output_directory, fuel_name, slope, aspect, num_hours, dc, dmc, ffmc, wind, final_sizes, true
  );
}
template <class V>
void show_options(const char* name, const vector<V>& values, std::function<string(V&)> convert)
{
  cout << std::format("\t{:d} {:s}: ", values.size(), name);
  // HACK: always print something before but avoid extra comma
  const char* prefix_open = "[";
  const char* prefix_comma = ", ";
  const char** p = &prefix_open;
  for (auto v : values)
  {
    cout << std::format("{:s}{:s}", *p, convert(v));
    p = &prefix_comma;
  }
  cout << "]\n";
};
template <class V>
void show_options(const char* name, const vector<V>& values)
{
  return show_options<V>(name, values, [](V& value) { return std::format("{:d}", value); });
};
void show_options(const char* name, const vector<string>& values)
{
  return show_options<string>(name, values, [](string& value) { return value; });
};
int test(Settings& settings)
{
  const auto output_directory{settings.output_directory};
  static const AspectSize ASPECT_INCREMENT = 90;
  static const SlopeSize SLOPE_INCREMENT = 60;
  static const int WS_INCREMENT = 5;
  static const int WD_INCREMENT = 45;
  static const int MAX_WIND = 50;
  static const DurationSize DEFAULT_HOURS = 10.0;
  static const SlopeSize DEFAULT_SLOPE = 0;
  static const AspectSize DEFAULT_ASPECT = 0;
  static const Speed DEFAULT_WIND_SPEED(20);
  static const Direction DEFAULT_WIND_DIRECTION{Degrees{180.0}};
  // static const Wind DEFAULT_WIND(DEFAULT_WIND_SPEED, DEFAULT_WIND_DIRECTION);
  static const Ffmc DEFAULT_FFMC(90);
  static const Dmc DEFAULT_DMC(35.5);
  static const Dc DEFAULT_DC(275);
  // const vector<string> FUEL_NAMES{"C-2", "O-1a", "M-1/M-2 (25 PC)", "S-1", "C-3"};
  // all possible fuel names that aren't invalid
  static auto FUEL_NAMES = []() -> vector<string> {
    auto it = std::views::transform(
      std::views::filter(
        FuelLookup::Fuels, [](const FuelType* f) -> bool { return nullptr != f && f->isValid(); }
      ),
      [](const auto* f) -> string { return simplify_fuel_name(FuelType::safeName(f)); }
    );
    return {it.begin(), it.end()};
  }();
  static const auto DEFAULT_FUEL_NAME = simplify_fuel_name("C-2");
  SafeVector final_sizes{};
  // FIX: I think this does a lot of the same things as the test code is doing because it was
  // derived from this code
  settings.deterministic = true;
  settings.minimum_ros = 0.0;
  settings.save_points = false;
  // make sure all tests run regardless of how long it takes
  settings.maximum_time_seconds = numeric_limits<size_t>::max();
  const auto hours{settings.hours.value_or(DEFAULT_HOURS)};
  const auto ffmc{settings.ffmc.value_or(DEFAULT_FFMC)};
  const auto dmc{settings.dmc.value_or(DEFAULT_DMC)};
  const auto dc{settings.dc.value_or(DEFAULT_DC)};
  const Direction wind_direction{settings.wind_direction.value_or(DEFAULT_WIND_DIRECTION.value)};
  const Speed wind_speed{settings.wind_speed.value_or(DEFAULT_WIND_SPEED.value)};
  const Wind wind{wind_speed, wind_direction};
  const auto slope{settings.slope.value_or(DEFAULT_SLOPE)};
  const auto aspect{settings.aspect.value_or(DEFAULT_ASPECT)};
  const auto fixed_fuel_name = simplify_fuel_name(settings.fuel_name.value_or(""));
  const auto fuel = (fixed_fuel_name.empty() ? DEFAULT_FUEL_NAME : fixed_fuel_name);
  try
  {
    if (settings.test_all.value_or(false))
    {
      size_t result = 0;
      // generate all options first so we can say how many there are at start
      auto fuel_names = vector<string>();
      if (fixed_fuel_name.empty())
      {
        for (auto f : FUEL_NAMES)
        {
          fuel_names.emplace_back(f);
        }
      }
      else
      {
        fuel_names.emplace_back(fuel);
      }
      auto slopes = vector<SlopeSize>();
      if (!settings.slope.has_value())
      {
        for (SlopeSize slope = 0; slope <= 100; slope += SLOPE_INCREMENT)
        {
          slopes.emplace_back(slope);
        }
      }
      else
      {
        slopes.emplace_back(slope);
      }
      auto aspects = vector<AspectSize>();
      if (!settings.aspect.has_value())
      {
        for (AspectSize aspect = 0; aspect < 360; aspect += ASPECT_INCREMENT)
        {
          aspects.emplace_back(aspect);
        }
      }
      else
      {
        aspects.emplace_back(aspect);
      }
      auto wind_directions = vector<DirectionSize>();
      if (!settings.wind_direction.has_value())
      {
        for (auto wind_direction = 0; wind_direction < 360; wind_direction += WD_INCREMENT)
        {
          wind_directions.emplace_back(wind_direction);
        }
      }
      else
      {
        wind_directions.emplace_back(static_cast<int>(wind_direction.value));
      }
      auto wind_speeds = vector<int>();
      if (!settings.wind_speed.has_value())
      {
        for (auto wind_speed = 0; wind_speed <= MAX_WIND; wind_speed += WS_INCREMENT)
        {
          wind_speeds.emplace_back(wind_speed);
        }
      }
      else
      {
        wind_speeds.emplace_back(static_cast<int>(wind_speed.value));
      }
      size_t values = 1;
      values *= fuel_names.size();
      values *= slopes.size();
      values *= aspects.size();
      values *= wind_directions.size();
      values *= wind_speeds.size();
      cout << std::format("There are {:d} options to try based on:\n", values);
      show_options("fuels", fuel_names);
      show_options("slopes", slopes);
      show_options("aspects", aspects);
      show_options("wind directions", wind_directions);
      show_options("wind speeds", wind_speeds);
      // do everything in parallel but not all at once because it uses too much memory for most
      // computers
      vector<std::future<string>> results{};
      for (const auto& fuel : fuel_names)
      {
        // do everything in parallel but not all at once because it uses too much memory for most
        // computers
        for (auto slope : slopes)
        {
          for (auto aspect : aspects)
          {
            for (auto wind_direction : wind_directions)
            {
              const Direction direction{Degrees{wind_direction}};
              for (const auto wind_speed : wind_speeds)
              {
                const Speed speed{static_cast<MathSize>(wind_speed)};
                const Wind wind{speed, direction};
                // need to make string now because it'll be another value if we wait
                results.push_back(async(
                  launch::async,
                  run_test_ignore_existing,
                  output_directory,
                  fuel,
                  slope,
                  aspect,
                  hours,
                  dc,
                  dmc,
                  ffmc,
                  wind,
                  &final_sizes
                ));
              }
            }
          }
        }
      }
      for (auto& r : results)
      {
        r.wait();
        auto output_directory = r.get();
        logging::check_fatal(
          !directory_exists(output_directory.c_str()),
          "Directory for test is missing: {:s}\n",
          output_directory
        );
        ++result;
      }
      auto directories = read_directory(output_directory, "*", false);
      logging::check_fatal(
        directories.size() != result,
        "Expected {:d} directories but have {:d}",
        result,
        directories.size()
      );
      logging::note("Successfully ran {:d} tests", result);
    }
    else
    {
      logging::note(
        "Running tests with constant inputs for {:f} hours:\n\tFBP Fuel:\t\t{:s}\n\tFFMC:\t\t\t{:f}\n\tDMC:\t\t\t{:f}\n\tDC:\t\t\t{:f}\n\tWind Speed:\t\t{:f}\n\tWind Direction:\t\t{:f}\n\tSlope:\t\t\t{:d}\n\tAspect:\t\t\t{:d}\n",
        hours,
        fuel,
        ffmc.value,
        dmc.value,
        dc.value,
        wind_speed.value,
        wind_direction.value,
        slope,
        aspect
      );
      auto dir_result = run_test(
        output_directory, fuel, slope, aspect, hours, dc, dmc, ffmc, wind, &final_sizes, false
      );
      logging::check_fatal(
        !directory_exists(dir_result.c_str()), "Directory for test is missing: {:s}\n", dir_result
      );
    }
  }
  catch (const runtime_error& err)
  {
    exit(logging::fatal(err));
  }
  return 0;
}
}
