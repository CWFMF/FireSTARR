/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Model.h"
#include "FBP.h"
#include "FireWeather.h"
#include "FWI.h"
#include "Input.h"
#include "Location.h"
#include "Log.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
#include "rng.h"
#include "Scenario.h"
#include "Settings.h"
#include "TimeUtil.h"
namespace fs
{
// // HACK: assume using half the CPUs probably means that faster cores are being used?
// constexpr MathSize PCT_CPU = 0.5;
Semaphore Model::task_limiter{static_cast<int>(std::thread::hardware_concurrency())};
Model::Model(
  const tm& start_time,
  const string_view output_directory,
  const StartPoint& start_point,
  Environment* env
)
  : Model(start_time, output_directory, start_point, env, settings::instance())
{ }
Model::Model(
  const tm& start_time,
  const string_view output_directory,
  const StartPoint& start_point,
  Environment* env,
  const Settings& settings
)
  : output_directory_(output_directory), start_time_(start_time), running_since_(Clock::now()),
    time_limit_(settings.maximum_time_seconds), no_interim_save_since_(Clock::now()),
    interim_save_interval_(settings.interim_output_interval_seconds), env_(env),
    active_simulations_still_required_(settings.minimum_active_simulation_count),
    latitude_(start_point.latitude()), longitude_(start_point.longitude())
{
  logging::debug("Calculating for {}", start_point);
  const auto nd_for_point = calculate_nd_ref_for_point(env->elevation(), start_point);
  for (auto day = 0; day < MAX_DAYS; ++day)
  {
    nd_.at(static_cast<size_t>(day)) = static_cast<int>(day - nd_for_point);
    logging::verbose(
      "Day {:d} has nd {:d}, is{:s} green, {:d}% curing",
      day,
      nd_.at(static_cast<size_t>(day)),
      calculate_is_green(nd_.at(static_cast<size_t>(day))) ? "" : " not",
      calculate_grass_curing(nd_.at(static_cast<size_t>(day)))
    );
  }
}
void Model::setWeather(const FwiWeather& weather, const Day start_day)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  static const auto& lookup = settings.fuel_lookup.lookup();
  yesterday_ = weather;
  const auto& f = lookup.usedFuels();
  wx_.emplace(
    0,
    FireWeather{
      f, static_cast<Day>(start_day - 1), weather.dc, weather.dmc, weather.ffmc, weather.wind
    }
  );
  wx_daily_.emplace(
    0,
    FireWeather{
      f, static_cast<Day>(start_day - 1), weather.dc, weather.dmc, weather.ffmc, weather.wind
    }
  );
}
void Model::readWeather(
  const FwiWeather& yesterday,
  const MathSize latitude,
  const string& filename
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  static const auto& lookup = settings.fuel_lookup.lookup();
  map<size_t, vector<FwiWeather>> wx{};
  map<size_t, map<Day, FwiWeather>> wx_daily{};
  map<Day, struct tm> dates{};
  Day min_date = numeric_limits<Day>::max();
  Day max_date = numeric_limits<Day>::min();
  time_t prev_time = numeric_limits<time_t>::min();
  ifstream in{filename};
  logging::check_fatal(!in.is_open(), "Could not open input weather file {:s}", filename);
  if (in.is_open())
  {
#ifdef DEBUG_WEATHER
    const auto file_out = string(output_directory_) + "/wx_hourly_out_read.csv";
    ofstream out{file_out};
    logging::check_fatal(!out.is_open(), "Cannot open file {:s} for output", file_out.c_str());
    out << "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\r\n";
#endif
    string str;
    logging::info("Reading scenarios from '{:s}'", filename);
    // read header line
    getline(in, str);
    // get rid of whitespace
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    constexpr auto expected_header = "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI";
    logging::check_fatal(
      expected_header != str,
      "Input CSV must have columns in this order:\n'{:s}'\n but got:\n'{:s}'",
      expected_header,
      str
    );
    auto prev = &yesterday;
    // HACK: adding to original object if we don't do this?
    auto apcp_24h = yesterday.prec.value;
    while (getline(in, str))
    {
      istringstream iss(str);
      if (getline(iss, str, ',') && !str.empty())
      {
        // HACK: ignore date and just worry about relative order??
        // Scenario
        logging::verbose("Scenario is {:s}", str);
        size_t cur = 0;
        try
        {
          cur = static_cast<size_t>(stoi(str));
        }
        catch (const std::exception& ex)
        {
          // HACK: somehow stoi() is still getting empty strings
          logging::fatal(
            ex, "Error reading weather file {:s}: {:s} is not a valid integer", filename, str
          );
        }
        if (wx.find(cur) == wx.end())
        {
          logging::debug("Loading scenario {:d}...", cur);
          wx.emplace(cur, vector<FwiWeather>{});
          prev_time = std::numeric_limits<time_t>::min();
          logging::check_fatal(
            wx_daily.find(cur) != wx_daily.end(),
            "Somehow have daily weather for scenario {:d} before hourly weather",
            cur
          );
          wx_daily.emplace(cur, map<Day, FwiWeather>());
          prev = &yesterday;
          logging::extensive(
            "Resetting new scenario precip to {:f} from {:f}", yesterday.prec.value, apcp_24h
          );
          apcp_24h = yesterday.prec.value;
        }
        auto& s = wx.at(cur);
        struct tm t{};
        read_date(&iss, &str, &t);
        year_ = t.tm_year + TM_YEAR_OFFSET;
        const auto ticks = fix_utc(&t);
        if (1 == cur)
        {
          logging::debug("Date '{:s}' is {:d} and calculated jd is {:d}", str, ticks, t.tm_yday);
          if (!s.empty() && t.tm_yday < min_date)
          {
            exit(
              logging::fatal("Weather input file crosses year boundary or dates are not sequential")
            );
          }
        }
        min_date = min(min_date, static_cast<Day>(t.tm_yday));
        max_date = max(max_date, static_cast<Day>(t.tm_yday));
        time_t cur_time = fix_utc(&t);
        if (prev_time != std::numeric_limits<time_t>::min())
        {
          auto seconds_diff = (cur_time - prev_time);
          logging::check_fatal(
            seconds_diff != HOUR_SECONDS,
            "Expected sequential hours in weather input but rows are {:f} hours away from each other",
            seconds_diff / static_cast<MathSize>(HOUR_SECONDS)
          );
        }
        prev_time = cur_time;
        const auto for_time = (t.tm_yday - min_date) * DAY_HOURS + t.tm_hour;
        // HACK: can be up until rest of year since start date
        const size_t new_size = (max_date - min_date + 1) * DAY_HOURS;
        const auto old_size = s.size();
        if (old_size != new_size)
        {
          s.resize(new_size);
        }
        logging::verbose("for_time == {:d}", for_time);
        FwiWeather w{read_fwi_weather(&iss, &str)};
        s.at(for_time) = w;
        logging::check_fatal(
          0 > w.prec.value, "Hourly weather precip {:f} is negative", w.prec.value
        );
        apcp_24h += w.prec.value;
        logging::extensive(
          "Adding {:f} to precip results in accumulation of {:f}", w.prec.value, apcp_24h
        );
        if (12 == t.tm_hour)
        {
          // we just hit noon on a new day, so add the daily value
          auto& s_daily = wx_daily.at(cur);
          const auto day = static_cast<Day>(t.tm_yday);
          logging::check_fatal(s_daily.find(day) != s_daily.end(), "Day already exists");
          const auto month = t.tm_mon + 1;
          s_daily.emplace(
            day,
            FwiWeather{*prev, month, latitude, w.temperature, w.rh, w.wind, Precipitation(apcp_24h)}
          );
          // new 24 hour period
          logging::extensive("Resetting daily precip to {:f} from {:f}", 0.0, apcp_24h);
          apcp_24h = 0;
          prev = &s_daily.at(static_cast<Day>(t.tm_yday));
        }
#ifdef DEBUG_WEATHER
        const auto month = t.tm_mon + 1;
        const auto fmt_line = std::format(
          "{:d},{:d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f}",
          cur,
          year_,
          month,
          t.tm_mday,
          t.tm_hour,
          t.tm_min,
          t.tm_sec,
          w.prec.value,
          w.temperature.value,
          w.rh.value,
          w.wind.speed.value,
          w.wind.direction.value,
          w.ffmc.value,
          w.dmc.value,
          w.dc.value,
          w.isi.value,
          w.bui.value,
          w.fwi.value
        );
        logging::debug(fmt_line.c_str());
        out << fmt_line << "\r\n";
#endif
      }
    }
#ifdef DEBUG_WEATHER
    out.close();
    logging::check_fatal(out.fail(), "Could not close file {:s}", file_out.c_str());
#endif
    in.close();
  }
  const auto& f = lookup.usedFuels();
  // loop through and try to find duplicates
  for (const auto& kv : wx)
  {
    const auto k = kv.first;
    const auto& s = kv.second;
    // FIX: this is just looking for duplicate scenario ids, not weather?
    if (wx_.find(k) == wx_.end())
    {
      wx_.emplace(k, FireWeather{f, min_date, max_date, s});
      // calculate daily indices
      auto& s_daily = wx_daily.at(k);
      // HACK: set yesterday to match today
      s_daily.emplace(min_date - 1, s_daily.at(min_date));
      wx_daily_.emplace(k, FireWeather{f, s_daily});
    }
  }
}
void Model::findStarts(const XYIdx& location)
{
  logging::error("Trying to start a fire in non-fuel");
  Idx range = 1;
  auto [x_loc, y_loc] = hash_to_xy(location);
  const auto width = env_->width();
  const auto height = env_->height();
  while (starts_.empty() && (range < (MAX_WIDTH / 2)))
  {
    for (Idx x = -range; x <= range; ++x)
    {
      for (Idx y = -range; y <= range; ++y)
      {
        // make sure we only look at the outside of the box
        if (1 == range || abs(x) == range || abs(y) == range)
        {
          // is this going to work if negative?
          // const auto xy = location + XIdx{x} + YIdx{y};
          const XIdx x1{x_loc + XIdx{x}};
          if (0 <= x1.value && x1.value < width)
          {
            const YIdx y1{y_loc + YIdx{y}};
            if (0 <= y1.value && y1.value < height)
            {
              const XYIdx xy{x1, y1};
              const auto for_cell = env_->cell(xy);
              if (!is_null_fuel(for_cell))
              {
                starts_.push_back(xy);
              }
            }
          }
        }
      }
    }
    ++range;
  }
  logging::check_fatal(starts_.empty(), "Fuel grid is empty");
  if (should_log(logging::level::info))
  {
    std::ignore = logging::output_no_check(
      logging::level::info, "Using {:d} start locations:", ignitionScenarios()
    );
    for (const auto& s : starts_)
    {
      std::ignore =
        logging::output_no_check(logging::level::info, "\t{:d}, {:d}", s.x_value(), s.y_value());
    }
  }
}
void Model::findAllStarts()
{
  logging::note("Running scenarios for every possible start location");
  for (Idx x = 0; x < env_->width(); ++x)
  {
    for (Idx y = 0; y < env_->height(); ++y)
    {
      const XYIdx xy{x, y};
      const auto for_cell = env_->cell(xy);
      if (!is_null_fuel(for_cell))
      {
        starts_.push_back(xy);
      }
    }
  }
  logging::info("Using {:d} start locations:", ignitionScenarios());
}
void Model::makeStarts(
  Coordinates coordinates,
  const Point& point,
  const LazyPath& perim,
  size_t size
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  XYIdx location{coordinates.x, coordinates.y};
  if (!perim.empty())
  {
    logging::note("Initializing from perimeter {:s}", perim.canonical());
    perimeter_ = make_shared<Perimeter>(perim, point, *env_);
    // HACK: if perimeter is only one cell then use position not perimeter so it can bounce if
    // non-fuel
    const auto& burned = perimeter_->burned;
    const auto s = burned.size();
    if (1 >= s)
    {
      logging::note(
        "Converting perimeter into point since size is {:d} cell{:s}", s, (1 == s ? "" : "s")
      );
      // use whatever the one cell is instead of the lat/long
      if (1 == s)
      {
        location = *(burned.begin());
      }
      // HACK: use 0 for 0 or 1 so it'll assign by point
      size = 0;
      perimeter_ = nullptr;
    }
  }
  // use if instead of else if in case perimeter was a single point and got switched
  if (size > 0)
  {
    logging::note("Initializing from size {:f} ha", env_->to_hectares(size));
    perimeter_ = make_shared<Perimeter>(location, size, *env_);
  }
  // figure out where the fire can exist
  if (nullptr != perimeter_ && !perimeter_->burned.empty())
  {
    logging::check_fatal(size != 0 && !perim.empty(), "Can't specify size and perimeter");
    // we have a perimeter to start from
    // HACK: make sure this isn't empty
    starts_.push_back(location);
    logging::note(
      "Fire starting with size {:0.1f} ha", env_->to_hectares(perimeter_->burned.size())
    );
  }
  else
  {
    if (nullptr != perimeter_)
    {
      logging::check_fatal(
        !perimeter_->burned.empty(), "Not using perimeter so it should be empty"
      );
      logging::note("Using fire perimeter results in empty fire - changing to use point");
      perimeter_ = nullptr;
    }
    if (settings.is_surface())
    {
      findAllStarts();
    }
    else
    {
      logging::note("Fire starting with size {:0.1f} ha", env_->to_hectares(1));
      const auto for_cell = cell(location);
      if (0 == size && is_null_fuel(for_cell))
      {
        if (!settings.no_search)
        {
          findStarts(location);
        }
        else
        {
          logging::warning("Start location is non-fuel and --no-search was specified");
        }
      }
      else
      {
        starts_.push_back(location);
      }
    }
  }
  logging::note(
    "Creating {:d} streams x {:d} location{:s} = {:d} scenarios",
    wx_.size(),
    ignitionScenarios(),
    ignitionScenarios() > 1 ? "s" : "",
    wx_.size() * ignitionScenarios()
  );
}
Iteration Model::readScenarios(
  const StartPoint& start_point,
  const DurationSize start,
  const Day start_day,
  const Day last_date
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  // FIX: this is going to do a lot of work to set up each scenario if we're making a surface
  vector<Scenario*> result{};
  const auto saves = settings.output_date_offsets.offsets();
  const auto setup_scenario = [&](Scenario* scenario) {
    if (settings.save_individual)
    {
      scenario->registerObserver(new IntensityObserver(*scenario));
      scenario->registerObserver(new ArrivalObserver(*scenario));
      scenario->registerObserver(new SourceObserver(*scenario));
    }
    // FIX: this should be relative to the start date, not the weather start date
    for (const auto& i : saves)
    {
      scenario->addSaveByOffset(i);
    }
    result.push_back(scenario);
  };
  if (settings.is_surface())
  {
    setup_scenario(new Scenario(
      this,
      0,
      &wx_.at(0),
      &wx_daily_.at(0),
      start,
      nullptr,
      starts_.at(0),
      start_point,
      start_day,
      last_date
    ));
  }
  else
  {
    for (const auto& kv : wx_)
    {
      const auto id = kv.first;
      const auto* cur_wx = &kv.second;
      const auto* cur_daily = &wx_daily_.at(id);
      // FIX: this should simplify to the loop and passing both location & perim
      if (nullptr != perimeter_)
      {
        logging::check_equal(
          starts_.size(), static_cast<size_t>(1), "start locations with perimeter"
        );
        const auto& cur_start = starts_.at(0);
        setup_scenario(new Scenario(
          this,
          id,
          cur_wx,
          cur_daily,
          start,
          perimeter_,
          cur_start,
          start_point,
          start_day,
          last_date
        ));
      }
      else
      {
        for (const auto& cur_start : starts_)
        {
          // should always have at least the day before the fire in the weather stream
          setup_scenario(new Scenario(
            this,
            id,
            cur_wx,
            cur_daily,
            start,
            perimeter_,
            cur_start,
            start_point,
            start_day,
            last_date
          ));
        }
      }
    }
  }
  return Iteration(result);
}
std::chrono::seconds at_least_zero(const std::chrono::seconds& t)
{
  constexpr auto zero = std::chrono::seconds{0};
  if (t < zero)
  {
    return zero;
  }
  return t;
}
[[nodiscard]] std::chrono::seconds Model::runTime() const
{
  const auto run_time = last_checked_ - runningSince();
  const auto run_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(run_time);
  return at_least_zero(run_time_seconds);
}
[[nodiscard]] std::chrono::seconds Model::timeSinceLastSave() const
{
  return at_least_zero(
    std::chrono::duration_cast<std::chrono::seconds>(last_checked_ - no_interim_save_since_)
  );
}
bool Model::shouldStop() const noexcept
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  return (!settings.is_surface()) && (isOutOfTime() || isOverSimulationCountMaximum());
}
bool Model::isOutOfTime() const noexcept { return is_out_of_time_; }
bool Model::isUnderSimulationCountMinimum() const noexcept { return is_under_simulation_minimum_; }
size_t Model::activeSimulationsStillRequired() const noexcept
{
  return active_simulations_still_required_;
}
bool Model::isOverSimulationCountMaximum() const noexcept { return is_over_simulation_count_; }
shared_ptr<ProbabilityMap> Model::makeProbabilityMap(
  const DurationSize time,
  const DurationSize start_time,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value
) const
{
  return env_->makeProbabilityMap(
    time, start_time, min_value, low_max, med_max, max_value, perimeter_
  );
}
static void show_probabilities(const map<ThresholdSize, shared_ptr<ProbabilityMap>>& probabilities)
{
  for (const auto& kv : probabilities)
  {
    kv.second->show();
  }
}
map<DurationSize, shared_ptr<ProbabilityMap>> make_prob_map(
  const Model& model,
  const vector<DurationSize>& saves,
  const DurationSize started,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value
)
{
  map<DurationSize, shared_ptr<ProbabilityMap>> result{};
  for (const auto& time : saves)
  {
    result.emplace(
      time, model.makeProbabilityMap(time, started, min_value, low_max, med_max, max_value)
    );
  }
  return result;
}
map<DurationSize, SafeVector*> make_size_map(const vector<DurationSize>& saves)
{
  map<DurationSize, SafeVector*> result{};
  for (const auto& time : saves)
  {
    result.emplace(time, new SafeVector());
  }
  return result;
}
bool Model::add_statistics(
  vector<MathSize>* all_sizes,
  vector<MathSize>* means,
  vector<MathSize>* pct,
  const SafeVector& sizes
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  const auto i = pct->size();
  const auto cur_sizes = sizes.getValues();
  logging::check_fatal(cur_sizes.empty(), "No sizes at end of simulation");
  const Statistics s{cur_sizes};
  static_cast<void>(insert_sorted(pct, s.percentile(95)));
  static_cast<void>(insert_sorted(means, s.mean()));
  // NOTE: Used to just look at mean and percentile of each iteration, but should probably look at
  // all the sizes together?
  for (const auto& size : cur_sizes)
  {
    static_cast<void>(insert_sorted(all_sizes, size));
  }
  if (settings.is_surface())
  {
    return true;
  }
  is_under_simulation_minimum_ = all_sizes->size() < settings.minimum_simulation_count;
  is_over_simulation_count_ = all_sizes->size() >= settings.maximum_simulation_count;
  if (isOverSimulationCountMaximum())
  {
    logging::note(
      "Stopping after {:d} iterations. Simulation limit of {:d} simulations has been reached.",
      i,
      +settings.maximum_simulation_count
    );
    return false;
  }
  active_simulations_still_required_ = [&]() {
    size_t num_left = settings.minimum_active_simulation_count;
    for (const auto s : *all_sizes)
    {
      if (s > initial_size())
      {
        --num_left;
      }
      if (0 == num_left)
      {
        logging::note("Found enough active simulations to meet minimum");
        return num_left;
      }
    }
    logging::note(
      "Not enough active simulations out of {:d} results to meet minimum", all_sizes->size()
    );
    return num_left;
  }();
  if (0 < activeSimulationsStillRequired())
  {
    return true;
  }
  if (isUnderSimulationCountMinimum())
  {
    return true;
  }
  if (isOutOfTime())
  {
    logging::note(
      "Stopping after {:d} iterations. Time limit of {:d} seconds has been reached.",
      i,
      +settings.maximum_time_seconds
    );
    return false;
  }
  return true;
}
/*!
 * \page ending Simulation stop conditions
 *
 * Simulations will continue to run until a stop condition is reached.
 *
 * 1) the program has reached the time defined in the settings file as the maximum
 * run duration.
 *
 * 2) the amount of variability in the output statistics has decreased to a point
 * that is less than the confidence level defined in the settings file
 */
size_t runs_required(
  const size_t i,
  const vector<MathSize>* all_sizes,
  const vector<MathSize>* means,
  const vector<MathSize>* pct,
  const Model& model
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  if (settings.deterministic)
  {
    logging::note("Stopping after iteration {:d} because running in deterministic mode", i);
    return 0;
  }
  if (model.isOverSimulationCountMaximum())
  {
    logging::note(
      "Stopping after {:d} iterations. Simulation limit of {:d} simulations has been reached.",
      i,
      +settings.maximum_simulation_count
    );
    return 0;
  }
  if (model.isOutOfTime())
  {
    logging::note(
      "Stopping after {:d} iterations. Time limit of {:d} seconds has been reached.",
      i,
      +settings.maximum_time_seconds
    );
    return 0;
  }
  const auto max_sims_left = settings.maximum_simulation_count - i;
  if (model.isUnderSimulationCountMinimum())
  {
    logging::debug(
      "Continuing after {:d} iterations. Simulation minimum of {:d} simulations has not been reached.",
      i,
      +settings.minimum_simulation_count
    );
    return min(max_sims_left, settings.minimum_simulation_count - i);
  }
  const auto active_still_required = model.activeSimulationsStillRequired();
  if (0 < active_still_required)
  {
    logging::debug(
      "Continuing after {:d} iterations. Active simulation minimum of {:d} simulations has not been reached.",
      i,
      +settings.minimum_active_simulation_count
    );
    // HACK: if we have n active sims so far then expect that many per i simulations?
    return min(max_sims_left, i * active_still_required);
  }
  // HACK: statistics don't work if only one value, so need at least 2
  const auto min_values = min(min(all_sizes->size(), means->size()), pct->size());
  if (1 >= min_values)
  {
    logging::note("Cannot calculate statistics with only {:d} value", min_values);
    return 1;
  }
  const auto for_sizes = Statistics{*all_sizes};
  const auto for_means = Statistics{*means};
  const auto for_pct = Statistics{*pct};
  if (!(!for_means.isConfident(settings.confidence_level)
        || !for_pct.isConfident(settings.confidence_level)
        || !for_sizes.isConfident(settings.confidence_level)))
  {
    return 0;
  }
  const auto runs_for_means = for_means.runsRequired(settings.confidence_level);
  const auto runs_for_pct = for_pct.runsRequired(settings.confidence_level);
  const auto runs_for_sizes = for_sizes.runsRequired(settings.confidence_level);
  logging::debug(
    "Runs required based on criteria: {{ means: {:d}, pct: {:d}, sizes: {:d} }}",
    runs_for_means,
    runs_for_pct,
    runs_for_sizes
  );
  logging::debug(
    "Number of values based on criteria: {{ means: {:d}, pct: {:d}, sizes: {:d} }}",
    for_means.n(),
    for_pct.n(),
    for_sizes.n()
  );
  const auto left = max(max(runs_for_means, runs_for_pct), runs_for_sizes);
  return left;
}
DurationSize Model::saveProbabilities(
  map<DurationSize, shared_ptr<ProbabilityMap>>& probabilities,
  const Day start_day,
  const bool is_interim
)
{
  auto final_time = numeric_limits<DurationSize>::min();
  const ProcessingStatus processing_status =
    !is_interim ? processed : (0 == scenarios_done_ ? unprocessed : processing);
  if ((processing_status == processing) && (scenarios_last_save_ == scenarios_done_))
  {
    logging::error("No change since last call to saveProbabilities");
  }
  else if ((processing_status != processing) || should_output_interim_)
  {
    // HACK: use max as "never saved" and replace with 0 on first save
    if (is_being_cancelled_)
    {
      logging::info(
        "Saving{:s} results for ({:d} of {:d}) required scenarios{:s}",
        is_interim ? " interim" : "",
        +scenarios_required_done_,
        +scenarios_per_iteration_,
        is_being_cancelled_ ? " because cancelling" : ""
      );
    }
    else
    {
      logging::info(
        "Saving{:s} results for {:d} scenarios ({:d} new in {:d} since last save)",
        is_interim ? " interim" : "",
        +scenarios_done_,
        +scenarios_done_ - scenarios_last_save_,
        timeSinceLastSave().count()
      );
    }
    for (const auto& [t, prob] : probabilities)
    {
      const auto time = prob->time;
      final_time = max(final_time, time);
      std::ignore = prob->saveAll(outputDirectory(), this->start_time_, time, processing_status);
      if (processing_status == processed)
      {
        if (should_log(logging::level::note))
        {
          const auto day = static_cast<int>(round(time));
          const auto n = nd(day);
          logging::note(
            "Fuels for day {:d} are {:s} green-up and grass has {:d}% curing",
            day - static_cast<int>(start_day),
            calculate_is_green(n) ? "after" : "before",
            calculate_grass_curing(n)
          );
        }
      }
    }
    logging::debug("Done saving probability grids");
    interim_changed_ = false;
    should_output_interim_ = false;
    scenarios_last_save_ = +scenarios_done_;
    if (!is_interim)
    {
      ProbabilityMap::deleteInterim();
    }
    else
    {
      no_interim_save_since_ = Clock::now();
    }
  }
  else
  {
    interim_changed_ = true;
  }
  return final_time;
}
map<DurationSize, shared_ptr<ProbabilityMap>> Model::runIterations(
  const StartPoint& start_point,
  const DurationSize start,
  const Day start_day
)
{
  using rng::make_seed;
  if (starts_.empty())
  {
    logging::warning("No start locations so not running");
    return {};
  }
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  auto last_date = start_day;
  for (const auto& i : settings.output_date_offsets.offsets())
  {
    last_date = max(static_cast<Day>(start_day + i), last_date);
  }
  auto seed_spread = make_seed("spread", start_point, start_day, 0, settings.salt);
  auto seed_extinction = make_seed("extinction", start_point, start_day, 1, settings.salt);
  mt19937_64 mt_spread(seed_spread);
  mt19937_64 mt_extinction(seed_extinction);
  vector<MathSize> all_sizes{};
  vector<MathSize> means{};
  vector<MathSize> pct{};
  iterations_done_ = 0;
  scenarios_done_ = 0;
  scenarios_required_done_ = 0;
  vector<Iteration> all_iterations{};
  logging::verbose("Reading scenarios");
  all_iterations.push_back(readScenarios(start_point, start, start_day, last_date));
  // HACK: reference from vector so timer can cancel everything in vector
  auto& iteration = all_iterations[0];
  scenarios_per_iteration_ = iteration.size();
  // put probability maps into map
  logging::verbose("Setting save points");
  const auto saves = iteration.savePoints();
  const auto started = iteration.startTime();
  auto probabilities = make_prob_map(
    *this,
    saves,
    started,
    0,
    settings.intensity_max_low,
    settings.intensity_max_moderate,
    numeric_limits<int>::max()
  );
  vector<map<DurationSize, shared_ptr<ProbabilityMap>>> all_probabilities{};
  all_probabilities.push_back(make_prob_map(
    *this,
    saves,
    started,
    0,
    settings.intensity_max_low,
    settings.intensity_max_moderate,
    numeric_limits<int>::max()
  ));
  logging::verbose("Setting up initial intensity map with perimeter");
  auto runs_left = 1;
  bool is_being_cancelled = false;
  // HACK: use initial value for type
  auto timer = std::thread([&]() {
    constexpr auto CHECK_INTERVAL = 1s;
    bool keep_checking{true};
    const bool is_limited = 0 != settings.maximum_time_seconds;
    const bool with_interim = 0 != interim_save_interval_.count();
    if (!is_limited)
    {
      logging::note("No time limit being enforced since MAXIMUM_TIME = 0");
    }
    if (!with_interim)
    {
      logging::note("No interim outputs being generated since INTERIM_OUTPUT_INTERVAL = 0");
    }
    while (keep_checking)
    {
      this->last_checked_ = Clock::now();
      // think we need to check regularly instead of just sleeping so that we can see
      // if we've done enough runs and need to stop for that reason
      std::this_thread::sleep_for(CHECK_INTERVAL);
      // set bool so other things don't need to check clock
      is_out_of_time_ = is_limited && runTime().count() >= timeLimit().count();
      should_output_interim_ =
        with_interim && timeSinceLastSave().count() >= interimTimeLimit().count();
      if (should_output_interim_ && interim_changed_)
      {
        saveProbabilities(all_probabilities[0], start_day, true);
      }
      logging::verbose("Checking clock [{:d} of {:d}]", runTime().count(), timeLimit().count());
      keep_checking = (runs_left > 0 && !shouldStop());
    }
    if (isOutOfTime())
    {
      logging::warning("Ran out of time - cancelling simulations");
    }
    if (0 == iterations_done_)
    {
      logging::warning(
        "Ran out of time, but haven't finished any iterations, so cancelling all but first"
      );
    }
    size_t i = 0;
    for (auto& iter : all_iterations)
    {
      // don't cancel first iteration if no iterations are done
      if (0 != iterations_done_ || 0 != i)
      {
        // if not over limit then just did all the runs so no warning
        iter.cancel(shouldStop());
      }
      ++i;
    }
    if (0 == iterations_done_)
    {
      is_being_cancelled = true;
      if (scenarios_required_done_ > 0)
      {
        logging::info(
          "Saving interim results for ({:d} of {:d}) scenarios in timer thread",
          +scenarios_required_done_,
          +scenarios_per_iteration_
        );
        saveProbabilities(all_probabilities[0], start_day, true);
      }
    }
    const auto run_time_seconds = runTime().count();
    const auto time_left = settings.maximum_time_seconds - run_time_seconds;
    logging::debug(
      "Ending timer after {:d} seconds with {:d} seconds left", run_time_seconds, time_left
    );
  });
  auto threads = list<std::thread>{};
  const auto finalize_probabilities = [&]() {
    // assume timer is cancelling everything
    for (auto& t : threads)
    {
      if (t.joinable())
      {
        t.join();
      }
    }
    if (timer.joinable())
    {
      timer.join();
    }
    return probabilities;
  };
  // if using surface just run each start through in a loop here
  size_t cur_start = 0;
  auto reset_iter = [&](Iteration& iter) {
    if (settings.is_surface())
    {
      if (cur_start >= starts_.size())
      {
        return false;
      }
      auto start_cell = starts_[cur_start];
      iter.reset_with_new_start(start_cell);
      ++cur_start;
    }
    else
    {
      iter.reset(&mt_extinction, &mt_spread);
    }
    return true;
  };
  if (settings.run_async)
  {
    const auto HARDWARE_THREADS = static_cast<size_t>(std::thread::hardware_concurrency());
    // maybe a bit slower but prefer to run all scenarios at the same time
    const auto MAX_THREADS = max(HARDWARE_THREADS, scenarios_per_iteration_);
    if (MAX_THREADS > HARDWARE_THREADS)
    {
      logging::note(
        "Increasing to use at least one thread for each of {:d} scenarios",
        +scenarios_per_iteration_
      );
      Model::task_limiter.set_limit(static_cast<int>(MAX_THREADS));
    }
    // no point in running multiple iterations if deterministic
    const auto concurrent_iterations = 1;
    for (size_t x = 1; x < concurrent_iterations; ++x)
    {
      all_iterations.push_back(readScenarios(start_point, start, start_day, last_date));
      all_probabilities.push_back(make_prob_map(
        *this,
        saves,
        started,
        0,
        settings.intensity_max_low,
        settings.intensity_max_moderate,
        numeric_limits<int>::max()
      ));
    }
    auto run_scenario = [&](Scenario* s, size_t i, bool is_required) {
      const auto result = s->run(&all_probabilities[i]);
      ++scenarios_done_;
      logging::extensive(
        "Done {:d} scenarios in iteration {:d} which {:s} required",
        +scenarios_done_,
        i,
        (is_required ? "is" : "is not")
      );
      if (is_required)
      {
        logging::verbose(
          "Done {:d} scenarios in iteration {:d} which {:s} required",
          +scenarios_done_,
          i,
          (is_required ? "is" : "is not")
        );
        ++scenarios_required_done_;
        logging::debug(
          "Have ({:d} of {:d}) scenarios and {:s} being cancelled",
          +scenarios_required_done_,
          +scenarios_per_iteration_,
          (is_being_cancelled ? "is" : "not")
        );
        if (is_being_cancelled)
        {
          // no point in saving interim if final is done
          if (scenarios_per_iteration_ != scenarios_required_done_)
          {
            logging::info(
              "Saving interim results for ({:d} of {:d}) scenarios",
              +scenarios_required_done_,
              +scenarios_per_iteration_
            );
            saveProbabilities(all_probabilities[0], start_day, true);
          }
        }
      }
      return result;
    };
    logging::debug("Created {:d} iterations to run concurrently", all_iterations.size());
    size_t cur_iter = 0;
    for (auto& iter : all_iterations)
    {
      if (reset_iter(iter))
      {
        auto& scenarios = iter.getScenarios();
        for (auto s : scenarios)
        {
          threads.emplace_back(run_scenario, s, cur_iter, 0 == cur_iter);
        }
        ++cur_iter;
      }
    }
    cur_iter = 0;
    while (runs_left > 0)
    {
      // should have completed one iteration, so add it
      auto& iteration = all_iterations[cur_iter];
      // so now try to loop through and add iterations as they finish
      // FIX: look at converting so that new threads get started as others complete
      // - would have to have multiple Iterations so we keep the data from them separate?
      size_t k = 0;
      while (k < scenarios_per_iteration_)
      {
        threads.front().join();
        threads.pop_front();
        ++k;
      }
      auto final_sizes = iteration.finalSizes();
      ++iterations_done_;
      for (auto& kv : all_probabilities[cur_iter])
      {
        probabilities[kv.first]->addProbabilities(*kv.second);
        // clear so we don't double count
        kv.second->reset();
      }
      if (!add_statistics(&all_sizes, &means, &pct, final_sizes))
      {
        // ran out of time but timer should cancel everything
        return finalize_probabilities();
      }
      {
        if (settings.is_surface())
        {
          runs_left = ignitionScenarios() - iterations_done_;
        }
        else
        {
          runs_left = runs_required(iterations_done_, &all_sizes, &means, &pct, *this);
          logging::note("Need another {:d} iterations", runs_left);
        }
      }
      if (runs_left > 0)
      {
        if (reset_iter(iteration))
        {
          auto& scenarios = iteration.getScenarios();
          for (auto s : scenarios)
          {
            threads.emplace_back(run_scenario, s, cur_iter, false);
          }
          ++cur_iter;
          // loop around to start if required
          cur_iter %= all_iterations.size();
        }
      }
      else
      {
        // no runs required, so stop
        return finalize_probabilities();
      }
    }
    // everything should be done when this section ends
  }
  else
  {
    logging::note("Running in synchronous mode");
    while (runs_left > 0)
    {
      logging::note("Running iteration {:d}", iterations_done_ + 1);
      if (reset_iter(iteration))
      {
        for (auto s : iteration.getScenarios())
        {
          s->run(&probabilities);
        }
        ++iterations_done_;
        if (!add_statistics(&all_sizes, &means, &pct, iteration.finalSizes()))
        {
          // ran out of time but timer should cance everything
          return finalize_probabilities();
        }
        if (settings.is_surface())
        {
          runs_left = ignitionScenarios() - iterations_done_;
        }
        else
        {
          runs_left = runs_required(iterations_done_, &all_sizes, &means, &pct, *this);
          logging::note("Need another {:d} iterations", runs_left);
        }
      }
    }
  }
  return finalize_probabilities();
}
int Model::runScenarios(
  const string_view output_directory,
  const LazyPath& weather_input,
  const FwiWeather& yesterday,
  const char* const raster_root,
  const StartPoint& start_point,
  const tm& start_time,
  const LazyPath& perimeter,
  const size_t size
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  fs::logging::note(
    "Simulation start time at start of runScenarios() is {:s}", format_datetime(start_time)
  );
  auto env = Environment::loadEnvironment(
    raster_root, start_point, perimeter, start_time.tm_year + TM_YEAR_OFFSET
  );
  env.saveToFile(output_directory);
  logging::debug("Environment loaded");
  // don't flip for Environment because that already happened
  const auto position = env.findCoordinates(start_point, false);
#ifndef NDEBUG
  logging::check_fatal(
    position->y > MAX_HEIGHT || position->x > MAX_WIDTH,
    "Location loaded outside of grid at position ({:d}, {:d})",
    position->y,
    position->x
  );
#endif
  logging::info("Position is ({:d}, {:d})", position->x, position->y);
  const XYIdx location{position->x, position->y};
  Model model(start_time, output_directory, start_point, &env);
  logging::note("Grid has size ({:d}, {:d})", env.width(), env.height());
  logging::note("Fire start position is cell ({:d}, {:d})", location.x_value(), location.y_value());
  auto start_hour =
    ((start_time.tm_hour + (static_cast<DurationSize>(start_time.tm_min) / 60)) / DAY_HOURS);
  logging::note(
    "Simulation start time is {:d}-{:02d}-{:02d} {:02d}:{:02d}",
    start_time.tm_year + TM_YEAR_OFFSET,
    start_time.tm_mon + TM_MONTH_OFFSET,
    start_time.tm_mday,
    start_time.tm_hour,
    start_time.tm_min
  );
  const auto start = start_time.tm_yday + start_hour;
  const auto start_day = static_cast<Day>(start);
  if (settings.is_surface())
  {
    // yesterday should have constants to use
    model.setWeather(yesterday, start_day);
    model.year_ = start_time.tm_year + 1900;
  }
  else
  {
    model.readWeather(yesterday, start_point.latitude(), weather_input.canonical());
    if (model.wx_.empty())
    {
      exit(logging::fatal("No weather provided"));
    }
    const auto& w = model.wx_.begin()->second;
    logging::debug("Have weather from day {:d} to {:d}", w.minDate(), w.maxDate());
    const auto numDays = (w.maxDate() - w.minDate() + 1);
    const auto needDays = settings.output_date_offsets.max();
    if (numDays < needDays)
    {
      exit(logging::fatal(
        "Not enough weather to proceed - have {:d} days but looking for {:d}", numDays, needDays
      ));
    }
// want to output internal representation of weather to file
#ifdef DEBUG_WEATHER
    if (!settings.is_surface())
    {
      model.outputWeather();
    }
#endif
    // want to check that start time is in the range of the weather data we have
    logging::check_fatal(start < w.minDate(), "Start time is before weather streams start");
    logging::check_fatal(start > w.maxDate(), "Start time is after weather streams end");
  }
  logging::note(
    "Simulation start time of {:f} is {:s}", start, make_timestamp(model.year(), start)
  );
  model.makeStarts(*position, start_point, perimeter, size);
  auto probabilities = model.runIterations(start_point, start, start_day);
  logging::note("Ran {:d} simulations", Scenario::completed());
  const auto run_time_seconds = model.runTime();
  const size_t time_left = settings.maximum_time_seconds - run_time_seconds.count();
  logging::debug(
    "Finished successfully after {:d} seconds with {:d} seconds left",
    run_time_seconds.count(),
    time_left
  );
  logging::debug("Processed {:d} spread events between all scenarios", Scenario::total_steps());
  show_probabilities(probabilities);
  // auto final_time =
  model.saveProbabilities(probabilities, start_day, false);
  // HACK: update last checked time to use in calculation
  model.last_checked_ = Clock::now();
  logging::note("Total simulation time was {:d} seconds", model.runTime().count());
  return 0;
}
#ifdef DEBUG_WEATHER
void Model::outputWeather()
{
  outputWeather(wx_, "wx_hourly_out.csv");
  outputWeather(wx_daily_, "wx_daily_out.csv");
}
void Model::outputWeather(map<size_t, FireWeather>& weather, const char* file_name)
{
  const auto file_out = string(output_directory_) + file_name;
  const auto file_out_fbp = string(output_directory_) + string("fbp_") + file_name;
  ofstream out{file_out};
  logging::check_fatal(!out.is_open(), "Unable to write to {:s}", file_out);
  ofstream out_fbp{file_out_fbp};
  logging::check_fatal(!out_fbp.is_open(), "Unable to write to {:s}", file_out);
  constexpr auto HEADER_FWI = "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI";
  constexpr auto HEADER_FBP_PRIMARY = "CFB,CFC,FD,HFI,RAZ,ROS,SFC,TFC";
  out << HEADER_FWI << "\r\n";
  out_fbp << HEADER_FWI << "," << HEADER_FBP_PRIMARY << "\r\n";
  size_t i = 0;
  for (auto& kv : weather)
  {
    auto& s = kv.second;
    // do we need to index this by hour and day?
    // was assuming it started at 0 for first hour and day
    auto& wx = s.getWeather();
    size_t min_hour = s.minDate() * DAY_HOURS;
    size_t wx_size = wx.size();
    size_t hour = min_hour;
    for (size_t j = 0; j < wx_size; ++j)
    {
      size_t day = hour / 24;
      auto w = wx.at(hour - min_hour);
      size_t month;
      size_t day_of_month;
      month_and_day(year_, day, &month, &day_of_month);
      if (nullptr != w)
      {
        const auto fmt_line = std::format(
          "{:d},{:d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f},{:1.6f}",
          i,
          year_,
          static_cast<uint8_t>(month),
          static_cast<uint8_t>(day_of_month),
          static_cast<uint8_t>(hour - day * DAY_HOURS),
          0,
          0,
          w->prec.value,
          w->temperature.value,
          w->rh.value,
          w->wind.speed.value,
          w->wind.direction.value,
          w->ffmc.value,
          w->dmc.value,
          w->dc.value,
          w->isi.value,
          w->bui.value,
          w->fwi.value
        );
        out << fmt_line << "\r\n";
        SlopeSize SLOPE_MAX = MAX_SLOPE_FOR_DISTANCE;
        SlopeSize SLOPE_INCREMENT = 200;
        AspectSize ASPECT_MAX = 360;
        AspectSize ASPECT_INCREMENT = 450;
        static const auto lookup = settings::instance().fuel_lookup.lookup();
        const auto fuel = lookup.byName("C-2");
        for (SlopeSize slope = 0; slope < SLOPE_MAX; slope += SLOPE_INCREMENT)
        {
          for (AspectSize aspect = 0; aspect < ASPECT_MAX; aspect += ASPECT_INCREMENT)
          {
            // for (auto fuel_name : FUELS)
            {
              const auto fuel_name = fuel->name();
              // calculate and output fbp
              // const auto spread = SpreadInfo(year_,
              Point start_point{latitude_, longitude_};
              const SpreadInfo spread(
                year_,
                month,
                day_of_month,
                hour,
                0,
                start_point,
                env_->elevation(),
                slope,
                aspect,
                fuel_name,
                w
              );
              const auto fmt_line = std::format(
                "{:d},{:d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:c},{:1.6g},{:1.6g},{:1.6g},{:1.6g},{:1.6g}\r\n",
                i,
                year_,
                static_cast<uint8_t>(month),
                static_cast<uint8_t>(day_of_month),
                static_cast<uint8_t>(hour - day * DAY_HOURS),
                0,
                0,
                w->prec.value,
                w->temperature.value,
                w->rh.value,
                w->wind.speed.value,
                w->wind.direction.value,
                w->ffmc.value,
                w->dmc.value,
                w->dc.value,
                w->isi.value,
                w->bui.value,
                w->fwi.value,
                spread.crownFractionBurned(),
                spread.crownFuelConsumption(),
                spread.fireDescription(),
                spread.maxIntensity(),
                spread.headDirection().asDegrees(),
                spread.headRos(),
                spread.surfaceFuelConsumption(),
                spread.totalFuelConsumption()
              );
              cout << fmt_line;
              out_fbp << fmt_line;
            }
          }
        }
      }
      ++hour;
    }
    ++i;
  }
  out.close();
  out_fbp.close();
  logging::check_fatal(out.fail(), [&]() {
    return std::format("Could not close file {:s}", file_out.c_str());
  });
  logging::check_fatal(out_fbp.fail(), [&]() {
    return std::format("Could not close file {:s}", file_out_fbp.c_str());
  });
}
#endif
}
