/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "../stdafx.h"
#include "Scenario.h"
#include "../fuel/FuelLookup.h"
#include "../grid/BurnedData.h"
#include "../grid/Cell.h"
#include "../grid/IntensityMap.h"
#include "../grid/Perimeter.h"
#include "../grid/ProbabilityMap.h"
#include "../log/Log.h"
#include "../spread/CellPoints.h"
#include "../spread/FireSpread.h"
#include "../spread/PointSpread.h"
#include "../types/Location.h"
#include "Observer.h"
#include "rng.h"
#include "Settings.h"
namespace fs
{
using namespace fuel;
using std::cout;
// constexpr auto PRECISION = static_cast<MathSize>(0.001);
static atomic<size_t> COUNT = 0;
static atomic<size_t> COMPLETED = 0;
static atomic<size_t> TOTAL_STEPS = 0;
static std::mutex MUTEX_SIM_COUNTS;
static map<size_t, size_t> SIM_COUNTS{};
template <typename T, typename F>
void do_each(T& for_list, F fct)
{
  std::for_each(
#if !defined(__APPLE__) || !defined(__clang__)
    // apple clang doesn't support this?
    std::execution::seq,
#endif
    for_list.begin(),
    for_list.end(),
    fct
  );
}
template <typename T, typename F>
void do_par(T& for_list, F fct)
{
  std::for_each(
#if !defined(__APPLE__) || !defined(__clang__)
    // apple clang doesn't support this?
    std::execution::par_unseq,
#endif
    for_list.begin(),
    for_list.end(),
    fct
  );
}
void IObserver_deleter::operator()(IObserver* ptr) const
{
  // cout << "Deleting\n";
  delete ptr;
}
void Scenario::clear() noexcept
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  unburnable_.clear();
  scheduler_ = set<Event>();
  arrival_ = {};
  points_ = {};
  if (!settings.is_surface())
  {
    spread_info_ = {};
  }
  extinction_thresholds_.clear();
  spread_thresholds_by_ros_.clear();
  max_ros_ = 0;
#ifdef DEBUG_SIMULATION
  logging::check_fatal(
    !scheduler_.empty(), "{:s} Scheduler isn't empty after clear()", log_prefix_
  );
#endif
  step_ = 0;
  oob_spread_ = 0;
}
size_t Scenario::completed() noexcept { return COMPLETED; }
size_t Scenario::count() noexcept { return COUNT; }
size_t Scenario::total_steps() noexcept { return TOTAL_STEPS; }
Scenario::~Scenario() { clear(); }
// HACK: just set next start point here for surface right now
Scenario* Scenario::reset_with_new_start(const XYIdx& start_xy, ptr<SafeVector> final_sizes)
{
  unburnable_.clear();
  start_xy_ = start_xy;
  cancelled_ = false;
  current_time_ = start_time_;
  intensity_ = nullptr;
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
  ran_ = false;
  clear();
  for (const auto& o : observers_)
  {
    o->reset();
  }
  current_time_ = start_time_ - 1;
  points_ = {};
  intensity_ = make_unique<IntensityMap>(model());
  // HACK: never reset these if using a surface
  {
    current_time_index_ = numeric_limits<size_t>::max();
  }
  ++COUNT;
  {
    // want a global count of how many times this scenario ran
    scoped_lock lk{MUTEX_SIM_COUNTS};
    simulation_ = static_cast<int64_t>(++SIM_COUNTS[id_]);
  }
  return this;
}
Scenario* Scenario::reset(
  mt19937_64* mt_extinction,
  mt19937_64* mt_spread,
  ptr<SafeVector> final_sizes
)
{
  unburnable_.clear();
  cancelled_ = false;
  current_time_ = start_time_;
  intensity_ = nullptr;
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
  ran_ = false;
  // track this here because reset is always called before use
  // HACK: +2 so there's something there if we land exactly on the end date
  const auto num = (static_cast<size_t>(last_date_) - start_day_ + 2) * DAY_HOURS;
  clear();
  extinction_thresholds_.resize(num);
  spread_thresholds_by_ros_.resize(num);
  // if these are null then all probability thresholds remain 0
  if (nullptr != mt_extinction)
  {
    rng::make_threshold(&extinction_thresholds_, mt_extinction, start_day_, last_date_);
  }
  if (nullptr != mt_spread)
  {
    rng::make_threshold(
      &spread_thresholds_by_ros_,
      mt_spread,
      start_day_,
      last_date_,
      &SpreadInfo::calculateRosFromThreshold
    );
  }
  for (const auto& o : observers_)
  {
    o->reset();
  }
  current_time_ = start_time_ - 1;
  points_ = {};
  intensity_ = make_unique<IntensityMap>(model());
  spread_info_ = {};
  arrival_ = {};
  max_ros_ = 0;
  current_time_index_ = numeric_limits<size_t>::max();
  ++COUNT;
  {
    // want a global count of how many times this scenario ran
    scoped_lock lock{MUTEX_SIM_COUNTS};
    simulation_ = static_cast<int64_t>(++SIM_COUNTS[id_]);
  }
  return this;
}
inline string get_log_prefix(const Scenario& scenario) noexcept
{
  return std::format(
    "Scenario {:4d}.{:04d} ({:3f}): ", scenario.id(), scenario.simulation(), scenario.current_time()
  );
}
void Scenario::evaluate(const Event& event)
{
  // log prefix is static if scenario time doesn't change
  log_prefix_ = get_log_prefix(*this);
#ifdef DEBUG_SIMULATION
  logging::check_fatal(
    event.time < current_time_,
    "{:s} Expected time to be > {:f} but got {:f}",
    log_prefix_,
    current_time_,
    event.time
  );
#endif
  // HACK: handle NewFire separately since can't create variables in switch
  if (Event::Type::NewFire == event.type)
  {
    const auto p0 = event.xy.center();
    const auto& x = p0.x;
    const auto& y = p0.y;
    const auto for_cell = model_->cell(event.xy);
    points_log_.log(step_, STAGE_NEW, event.time, x.value, y.value);
    // HACK: don't do this in constructor because scenario creates this in its constructor
    // HACK: insert point as originating from itself
    points_.insert(
      p0,
      SpreadData{event.time, NO_INTENSITY, NO_ROS, Direction::Invalid(), Direction::Invalid()},
      p0
    );
    if (is_null_fuel(for_cell))
    {
      exit(logging::fatal("{:s} Trying to start a fire in non-fuel", log_prefix_));
    }
    logging::verbose(
      "{:s} Starting fire at point ({:f}, {:f}) in fuel type {:s} at time {:f}",
      log_prefix_,
      x.value,
      y.value,
      FuelType::safeName(check_fuel(for_cell)),
      event.time
    );
    if (!survives(event.time, for_cell, event.time_at_location))
    {
      if (should_log(logging::level::info))
      {
        // HACK: show daily values since that's what survival uses
        const auto wx = weather_daily(event.time);
        logging::info(
          "{:s} Didn't survive ignition in {:s} with weather {:f}, {:f}",
          log_prefix_,
          FuelType::safeName(check_fuel(for_cell)),
          wx.ffmc().value,
          wx.dmc().value
        );
      }
      // HACK: we still want the fire to have existed, so set the intensity of the origin
    }
    // fires start with intensity of 1
    burn(event);
    scheduleFireSpread(event);
  }
  else
  {
    switch (event.type)
    {
      case Event::Type::FireSpread:
        ++step_;
#ifdef DEBUG_POINTS
        {
          const auto ymd = fs::make_timestamp(model().year(), event.time);
        }
#endif
        scheduleFireSpread(event);
        break;
      case Event::Type::Save:
        std::ignore = saveObservers(model_->outputDirectory(), event.time);
        saveStats(event.time);
        break;
      case Event::Type::NewFire:
        throw runtime_error("Error handling event");
        break;
      case Event::Type::EndSimulation:
        logging::verbose("{:s} End simulation event reached at {:f}", log_prefix_, event.time);
        endSimulation();
        break;
      default:
        throw runtime_error("Invalid event type");
    }
  }
}
using namespace fuel;
SurvivalMap make_survival(const FireWeather& fire_wx)
{
  static const auto& settings = fs::settings::instance();
  static const auto& lookup = settings.fuel_lookup.lookup();
  const auto& used_fuels = lookup.usedFuels();
  const auto& weather_by_hour_by_day = fire_wx.getWeather();
  const auto min_date = fire_wx.minDate();
  const auto max_date = fire_wx.maxDate();
  SurvivalMap result{};
  for (const auto& in_fuel : used_fuels)
  {
    const auto code = FuelType::safeCode(in_fuel);
    if (nullptr != in_fuel && INVALID_FUEL_CODE != code)
    {
      auto by_fuel = vector<float>{};
      by_fuel.resize((static_cast<size_t>(max_date) - min_date + 2) * DAY_HOURS);
      // calculate the entire stream for this fuel
      for (auto day = min_date; day <= max_date; ++day)
      {
        for (auto h = 0; h < DAY_HOURS; ++h)
        {
          const auto wx = weather_by_hour_by_day.at(time_index(day, h, min_date));
          const auto i = time_index(day, h, min_date);
          by_fuel.at(i) =
            static_cast<float>(wx.isNull() ? 0.0 : (in_fuel->survivalProbability(wx)));
        }
      }
      result.at(code) = std::move(by_fuel);
    }
  }
  return result;
}
Scenario::Scenario(
  Model* model,
  const size_t id,
  const ptr<const FireWeather> weather,
  const ptr<const FireWeather> weather_daily,
  const DurationSize start_time,
  const shared_ptr<Perimeter>& perimeter,
  const XYIdx& start_cell,
  StartPoint start_point,
  const Day start_day,
  const Day last_date
)
  : current_time_(start_time), unburnable_{}, intensity_(nullptr), perimeter_(perimeter),
    max_ros_(0), start_xy_(start_cell), weather_(weather), weather_daily_(weather_daily),
    model_(model), probabilities_(nullptr), final_sizes_(nullptr),
    start_point_(std::move(start_point)), id_(id), start_time_(start_time),
    last_save_(weather_->minDate()), simulation_(-1), start_day_(start_day), last_date_(last_date),
    ran_(false), step_(0),
    points_log_(
      LogPoints{model_->outputDirectory(), settings::instance().save_points, id_, start_time_}
    ),
    survival_probability_{make_survival(*weather_)}
{
  const auto wx = weather_->at(start_time_);
  logging::check_fatal(
    wx.isNull(), "No weather for start time {:s}", make_timestamp(model->year(), start_time_)
  );
  const auto saves = settings::instance().output_date_offsets.offsets();
  const auto last_save = start_day_ + saves[saves.size() - 1];
  logging::check_fatal(
    last_save > weather_->maxDate(),
    "No weather for last save time {:s}",
    make_timestamp(model->year(), last_save)
  );
}
void Scenario::saveStats(const DurationSize time) const
{
  probabilities_->at(time)->addProbability(*intensity_);
  if (time == last_save_)
  {
    final_sizes_->addValue(intensity_->fireSize());
  }
}
void Scenario::registerObserver(IObserver* observer)
{
  observers_.push_back(unique_ptr<IObserver, IObserver_deleter>(observer));
}
void Scenario::notify(const Event& event) const
{
  for (const auto& o : observers_)
  {
    o->handleEvent(event);
  }
}
FileList Scenario::saveObservers(const string_view output_directory, const string_view base_name)
  const
{
  FileList files{};
  for (const auto& o : observers_)
  {
    files.append_range(o->save(output_directory, base_name));
  }
  return files;
}
FileList Scenario::saveObservers(const string_view output_directory, const DurationSize time) const
{
  return saveObservers(
    output_directory,
    std::format("{:03d}_{:06d}_{:03d}", id(), simulation(), static_cast<int>(time))
  );
}
FileList Scenario::saveIntensity(const string_view output_directory, const string_view base_name)
  const
{
  return intensity_->save(output_directory, base_name);
}
bool Scenario::ran() const noexcept { return ran_; }
Scenario::Scenario(Scenario&& rhs) noexcept
  : observers_(std::move(rhs.observers_)), save_points_(std::move(rhs.save_points_)),
    extinction_thresholds_(std::move(rhs.extinction_thresholds_)),
    spread_thresholds_by_ros_(std::move(rhs.spread_thresholds_by_ros_)),
    current_time_(rhs.current_time_), points_(std::move(rhs.points_)),
    unburnable_(std::move(rhs.unburnable_)), scheduler_(std::move(rhs.scheduler_)),
    intensity_(std::move(rhs.intensity_)), perimeter_(std::move(rhs.perimeter_)),
    spread_info_(std::move(rhs.spread_info_)), arrival_(std::move(rhs.arrival_)),
    max_ros_(rhs.max_ros_), start_xy_(std::move(rhs.start_xy_)), weather_(rhs.weather_),
    weather_daily_(rhs.weather_daily_), model_(rhs.model_), probabilities_(rhs.probabilities_),
    final_sizes_(rhs.final_sizes_), start_point_(std::move(rhs.start_point_)), id_(rhs.id_),
    start_time_(rhs.start_time_), last_save_(rhs.last_save_), simulation_(rhs.simulation_),
    start_day_(rhs.start_day_), last_date_(rhs.last_date_), ran_(rhs.ran_),
    survival_probability_(rhs.survival_probability_)
{ }
Scenario& Scenario::operator=(Scenario&& rhs) noexcept
{
  if (this != &rhs)
  {
    observers_ = std::move(rhs.observers_);
    save_points_ = std::move(rhs.save_points_);
    extinction_thresholds_ = std::move(rhs.extinction_thresholds_);
    spread_thresholds_by_ros_ = std::move(rhs.spread_thresholds_by_ros_);
    points_ = std::move(rhs.points_);
    current_time_ = rhs.current_time_;
    scheduler_ = std::move(rhs.scheduler_);
    intensity_ = std::move(rhs.intensity_);
    perimeter_ = std::move(rhs.perimeter_);
    start_xy_ = std::move(rhs.start_xy_);
    weather_ = rhs.weather_;
    weather_daily_ = rhs.weather_daily_;
    model_ = rhs.model_;
    probabilities_ = rhs.probabilities_;
    final_sizes_ = rhs.final_sizes_;
    start_point_ = std::move(rhs.start_point_);
    id_ = rhs.id_;
    start_time_ = rhs.start_time_;
    last_save_ = rhs.last_save_;
    simulation_ = rhs.simulation_;
    start_day_ = rhs.start_day_;
    last_date_ = rhs.last_date_;
    ran_ = rhs.ran_;
    survival_probability_ = rhs.survival_probability_;
  }
  return *this;
}
void Scenario::burn(const Event& event)
{
#ifdef DEBUG_SIMULATION
  logging::check_fatal(
    intensity_->hasBurned(event.xy),
    "{:s} Re-burning cell ({:d}, {:d})",
    log_prefix_,
    event.xy.x_value(),
    event.xy.y_value()
  );
#endif
  // #ifdef DEBUG_POINTS
  //   logging::check_fatal((*unburnable_)[event.xy.hash()], [&]() {
  //     return std::format(
  //       "{:s} Burning unburnable cell ({:d}, {:d})", log_prefix_, event.xy.x(), event.xy.y()
  //     );
  //   });
  // #endif
  //  Observers only care about cells burning so do it here
  notify(event);
  intensity_->burn(event.xy, event.intensity, event.ros, event.raz);
#ifdef DEBUG_GRIDS
  logging::check_fatal(
    !intensity_->hasBurned(event.xy), "{:s} Wasn't marked as burned after burn", log_prefix_
  );
#endif
  arrival_[event.xy] = event.time;
}
bool Scenario::isSurrounded(const XYIdx& location) const
{
  return intensity_->isSurrounded(location);
}
#ifdef DEBUG_PROBABILITY
void saveProbabilities(
  const string_view dir,
  const string_view base_name,
  vector<ThresholdSize>& thresholds
)
{
  ofstream out{string(dir) + string(base_name) + ".csv"};
  for (auto v : thresholds)
  {
    out << v << '\n';
  }
  out.close();
}
#endif
Scenario* Scenario::run(map<DurationSize, shared_ptr<ProbabilityMap>>* probabilities)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
#ifdef DEBUG_SIMULATION
  logging::check_fatal(ran(), "{:s} Scenario has already run", log_prefix_);
#endif
  logging::verbose("{:s} Starting", log_prefix_);
  CriticalSection _(Model::task_limiter);
  // HACK: only do once
  static const auto showed_once = [&]() {
    logging::debug("Concurrent Scenario limit is {:d}", Model::task_limiter.limit());
    return true;
  }();
  std::ignore = showed_once;
  unburnable_ = model_->environment().unburnable();
  probabilities_ = probabilities;
  logging::verbose("{:s} Setting save points", log_prefix_);
  for (auto time : save_points_)
  {
    // NOTE: these happen in this order because of the way they sort based on type
    addEvent(Event{.time = time, .type = Event::Type::Save});
  }
  if (nullptr == perimeter_)
  {
    addEvent(Event{.time = start_time_, .type = Event::Type::NewFire, .xy = start_xy_});
  }
  else
  {
    logging::verbose("{:s} Applying perimeter", log_prefix_);
    intensity_->applyPerimeter(*perimeter_);
    logging::verbose("{:s} Perimeter applied", log_prefix_);
    logging::verbose("{:s} Igniting points", log_prefix_);
    for (const auto& location : perimeter_->edge)
    {
#ifdef DEBUG_SIMULATION
      const auto for_cell = model_->cell(location);
      logging::check_fatal(is_null_fuel(for_cell), "{:s} Null fuel in perimeter", log_prefix_);
#endif
      const auto p0 = location.center();
      const auto& x = p0.x;
      const auto& y = p0.y;
      // log_verbose(*this, "Adding point ({:d}, {:d})",
      logging::extensive("{:s} Adding point ({:f}, {:f})", log_prefix_, x.value, y.value);
      points_.insert(
        p0,
        SpreadData{start_time_, NO_INTENSITY, NO_ROS, Direction::Invalid(), Direction::Invalid()},
        p0
      );
    }
    addEvent(Event{.time = start_time_, .type = Event::Type::FireSpread});
  }
  // HACK: make a copy of the event so that it still exists after it gets processed
  // NOTE: sorted so that EventSaveASCII is always just before this
  // Only run until last time we asked for a save for
  logging::verbose("{:s} Creating simulation end event for {:f}", log_prefix_, last_save_);
  addEvent(Event{.time = last_save_, .type = Event::Type::EndSimulation});
  // mark all original points as burned at start
  for (auto& [loc, pts] : points_.cells_)
  {
    assert(loc == pts.pos());
    // would be burned already if perimeter applied
    if (canBurn(loc))
    {
      const Event fake_event{.time = start_time_, .xy = loc, .ros = 0.0};
      burn(fake_event);
    }
  }
  while (!cancelled_ && !scheduler_.empty())
  {
    evaluateNextEvent();
  }
  ++TOTAL_STEPS;
  unburnable_.clear();
  if (cancelled_)
  {
    return nullptr;
  }
  const auto completed = ++COMPLETED;
  // HACK: use + to pull value out of atomic
  const auto count = (settings.is_surface()) ? model_->scenarioCount() : (+COUNT);
  const auto log_level = (0 == (completed % 1000)) ? logging::level::note : logging::level::info;
  if (logging::should_log(log_level))
  {
    if (settings.is_surface())
    {
      const auto ratio_done = static_cast<MathSize>(completed) / static_cast<MathSize>(count);
      const auto s = model_->runTime().count();
      const auto r =
        static_cast<size_t>(static_cast<MathSize>(s) / ratio_done) - static_cast<size_t>(s);
      std::ignore = logging::output_no_check(
        log_level,
        "{:s} [{:d} of {:d}] ({:0.2f}%) <{:d} : {:d} remaining> Completed with final size {:0.1f} ha",
        log_prefix_,
        completed,
        count,
        100 * ratio_done,
        s,
        r,
        currentFireSize()
      );
    }
    else
    {
#ifdef NDEBUG
      std::ignore = logging::output_no_check(
        log_level,
        "{:s} [{:d} of {:d}] Completed with final size {:0.1f} ha",
        log_prefix_,
        completed,
        count,
        currentFireSize()
      );
#else
      // try to make output consistent if in debug mode
      std::ignore = logging::output_no_check(
        log_level, "{:s} Completed with final size {:0.1f} ha", log_prefix_, currentFireSize()
      );
#endif
    }
  }
  ran_ = true;
#ifdef DEBUG_PROBABILITY
  // nice to have this get output when debugging, but only need it in extreme cases
  if (should_log(logging::level::extensive))
  {
    saveProbabilities(
      model().outputDirectory(),
      std::format("{:03d}_{:06d}_extinction", id(), simulation()),
      extinction_thresholds_
    );
    saveProbabilities(
      model().outputDirectory(),
      std::format("{:03d}_{:06d}_spread", id(), simulation()),
      spread_thresholds_by_ros_
    );
  }
#endif
  if (oob_spread_ > 0)
  {
    logging::warning("{:s} Tried to spread out of bounds {:d} times", log_prefix_, oob_spread_);
  }
  return this;
}
void Scenario::scheduleFireSpread(const Event& event)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  const auto time = event.time;
  const auto this_time = time_index(time);
  const auto wx = settings.is_surface() ? model_->yesterday() : weather(time);
  const auto wx_daily = settings.is_surface() ? model_->yesterday() : weather_daily(time);
  current_time_ = time;
  log_prefix_ = get_log_prefix(*this);
  logging::check_fatal(wx.isNull(), "No weather available for time {:f}", time);
  const auto next_time = static_cast<DurationSize>(this_time + 1) / DAY_HOURS;
  // should be in minutes?
  const auto max_duration = (next_time - time) * DAY_MINUTES;
  const auto max_time = time + max_duration / DAY_MINUTES;
  // HACK: use the old ffmc for this check to be consistent with previous version
  if (wx_daily.ffmc().value < minimumFfmcForSpread(time))
  {
    addEvent(Event{.time = max_time, .type = Event::Type::FireSpread});
    logging::extensive("{:s} Waiting until {:f} because of FFMC", log_prefix_, max_time);
    return;
  }
  if (current_time_index_ != this_time)
  {
    current_time_index_ = this_time;
    // seemed like it would be good to keep offsets but max_ros_ needs to reset or things slow to
    // a crawl?
    if (!settings.is_surface())
    {
      spread_info_ = {};
    }
    max_ros_ = 0.0;
  }
  auto new_time = do_spread(max_ros_, points_, spread_info_, this, unburnable_, time, max_duration);
  if (-1 == new_time)
  {
    // if no spread then we left everything back in points still
    logging::verbose("{:s} Waiting until {:f}", log_prefix_, max_time);
    addEvent(Event{.time = max_time, .type = Event::Type::FireSpread});
    return;
  }
  // if we move everything out of points_ we can parallelize this check?
  do_each(points_.cells_, [&](CellPointsMap::map_value& kv) {
    auto& [loc, pts] = kv;
    const auto for_cell = cell(loc);
    // ******************* CHECK THIS BECAUSE IF SOMETHING IS IN HERE SHOULD IT ALWAYS HAVE
    // SPREAD????? *****************8
    const auto max_intensity = spread_info_.maxIntensity(for_cell.key());
    // HACK: just use side-effect to log and check bounds
    points_log_.log(step_, STAGE_SPREAD, new_time, pts);
    if (canBurn(loc) && max_intensity > 0)
    {
      const auto& spread = pts.spread_arrival();
      const Event fake_event{
        .time = new_time,
        .xy = loc,
        .ros = spread.ros,
        .intensity = spread.intensity,
        .raz = spread.direction,
        .source = pts.sources()
      };
      burn(fake_event);
    }
    if (!unburnable_.at(loc)
        // && canBurn(for_cell)
        && ((survives(new_time, for_cell, new_time - arrival_[loc]) && !isSurrounded(loc))))
    {
      points_log_.log(step_, STAGE_CONDENSE, new_time, pts);
      // CHECK: this is already pointed at the right thing, no?
      // std::swap(pts0.second, pts);
    }
    else
    {
      // just inserted false, so make sure unburnable gets updated
      // whether it went out or is surrounded just mark it as unburnable
      unburnable_.set(loc);
      // not swapping means these points get dropped
    }
  });
  logging::extensive(
    "{:s} Spreading {:d} cells until {:f}", log_prefix_, points_.cells_.size(), new_time
  );
  addEvent(Event{.time = new_time, .type = Event::Type::FireSpread});
}
MathSize Scenario::currentFireSize() const { return intensity_->fireSize(); }
bool Scenario::canBurn(const XYIdx& location) const { return intensity_->canBurn(location); }
bool Scenario::hasBurned(const XYIdx& location) const { return intensity_->hasBurned(location); }
void Scenario::endSimulation() noexcept
{
  logging::verbose("{:s} Ending simulation", log_prefix_);
  scheduler_ = set<Event>();
}
void Scenario::addSaveByOffset(const int offset)
{
  // offset is from beginning of the day the simulation starts
  // e.g. 1 is midnight, 2 is tomorrow at midnight
  addSave(static_cast<Day>(startTime()) + offset);
}
vector<DurationSize> Scenario::savePoints() const { return save_points_; }
void Scenario::addSave(const DurationSize time)
{
  last_save_ = max(last_save_, time);
  save_points_.push_back(time);
}
void Scenario::addEvent(Event&& event) { scheduler_.insert(std::move(event)); }
// bool Scenario::evaluateNextEvent()
void Scenario::evaluateNextEvent()
{
  // make sure to actually copy it before we erase it
  const auto& event = *scheduler_.begin();
  evaluate(event);
  if (!scheduler_.empty())
  {
    scheduler_.erase(event);
  }
}
void Scenario::cancel(bool show_warning) noexcept
{
  // ignore if already cancelled
  if (!cancelled_)
  {
    cancelled_ = true;
    if (show_warning)
    {
      logging::warning("{:s} Simulation cancelled", log_prefix_);
    }
  }
}
}
