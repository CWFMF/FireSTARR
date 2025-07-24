/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#define LOG_POINTS_RELATIVE
// #undef LOG_POINTS_RELATIVE
#define LOG_POINTS_CELL
#undef LOG_POINTS_CELL

#include "Scenario.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
#include "IntensityMap.h"
#include "FuelType.h"
#include "MergeIterator.h"
#include "CellPoints.h"
#include "FuelLookup.h"
#include "Location.h"
#include "Cell.h"
#include "LogPoints.h"

namespace fs::sim
{
using topo::Position;
using topo::Cell;
using topo::Perimeter;
using topo::SpreadKey;
using topo::StartPoint;

constexpr auto CELL_CENTER = static_cast<InnerSize>(0.5);
constexpr auto PRECISION = static_cast<MathSize>(0.001);
static atomic<size_t> COUNT = 0;
static atomic<size_t> COMPLETED = 0;
static atomic<size_t> TOTAL_STEPS = 0;
static std::mutex MUTEX_SIM_COUNTS;
static map<size_t, size_t> SIM_COUNTS{};

template <typename T, typename F>
void do_each(T& for_list, F fct)
{
  std::for_each(
    std::execution::seq,
    for_list.begin(),
    for_list.end(),
    fct);
}

template <typename T, typename F>
void do_par(T& for_list, F fct)
{
  std::for_each(
    std::execution::par_unseq,
    for_list.begin(),
    for_list.end(),
    fct);
}
void Scenario::clear() noexcept
{
  //  scheduler_.clear();
  scheduler_ = set<Event, EventCompare>();
#ifdef USE_OLD_SPREAD
  //  points_.clear();
  points_ = {};
#endif
  spread_info_ = {};
  extinction_thresholds_.clear();
  spread_thresholds_by_ros_.clear();
  max_ros_ = 0;
#ifdef DEBUG_SIMULATION
  log_check_fatal(!scheduler_.empty(), "Scheduler isn't empty after clear()");
#endif
  step_ = 0;
  oob_spread_ = 0;
}
size_t Scenario::completed() noexcept
{
  return COMPLETED;
}
size_t Scenario::count() noexcept
{
  return COUNT;
}
size_t Scenario::total_steps() noexcept
{
  return TOTAL_STEPS;
}
Scenario::~Scenario()
{
  clear();
}
/*!
 * \page probability Probability of events
 *
 * Probability throughout the simulations is handled using pre-rolled random numbers
 * based on a fixed seed, so that simulation results are reproducible.
 *
 * Probability is stored as 'thresholds' for a certain event on a day-by-day and hour-by-hour
 * basis. If the calculated probability of that type of event matches or exceeds the threshold
 * then the event will occur.
 *
 * Each iteration of a scenario will have its own thresholds, and thus different behaviour
 * can occur with the same input indices.
 *
 * Thresholds are used to determine:
 * - extinction
 * - spread events
 */
static void make_threshold(vector<ThresholdSize>* thresholds,
                           mt19937* mt,
                           const Day start_day,
                           const Day last_date,
                           ThresholdSize (*convert)(double value))
{
  const auto total_weight = Settings::thresholdScenarioWeight() + Settings::thresholdDailyWeight() + Settings::thresholdHourlyWeight();
  uniform_real_distribution<ThresholdSize> rand(0.0, 1.0);
  const auto general = rand(*mt);
  for (size_t i = start_day; i < MAX_DAYS; ++i)
  {
    const auto daily = rand(*mt);
    for (auto h = 0; h < DAY_HOURS; ++h)
    {
      // generate no matter what so if we extend the time period the results
      // for the first days don't change
      const auto hourly = rand(*mt);
      // only save if we're going to use it
      // HACK: +1 so if it's exactly at the end time there's something there
      if (i <= static_cast<size_t>(last_date + 1))
      {
        // subtract from 1.0 because we want weight to make things more likely not less
        // ensure we stay between 0 and 1
        thresholds->at((i - start_day) * DAY_HOURS + h) =
          convert(
            max(0.0,
                min(1.0,
                    1.0 - (Settings::thresholdScenarioWeight() * general + Settings::thresholdDailyWeight() * daily + Settings::thresholdHourlyWeight() * hourly) / total_weight)));
        //        thresholds->at((i - start_day) * DAY_HOURS + h) = 0.0;
      }
    }
  }
}
template <class V>
constexpr V same(const V value) noexcept
{
  return value;
}
static void make_threshold(vector<ThresholdSize>* thresholds,
                           mt19937* mt,
                           const Day start_day,
                           const Day last_date)
{
  make_threshold(thresholds, mt, start_day, last_date, &same);
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   wx::FireWeather* weather_daily,
                   const DurationSize start_time,
                   //  const shared_ptr<IntensityMap>& initial_intensity,
                   const shared_ptr<Perimeter>& perimeter,
                   const StartPoint& start_point,
                   const Day start_day,
                   const Day last_date)
  : Scenario(model,
             id,
             weather,
             weather_daily,
             start_time,
             //  initial_intensity,
             perimeter,
             nullptr,
             start_point,
             start_day,
             last_date)
{
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   wx::FireWeather* weather_daily,
                   const DurationSize start_time,
                   const HashSize start_cell,
                   const StartPoint& start_point,
                   const Day start_day,
                   const Day last_date)
  : Scenario(model,
             id,
             weather,
             weather_daily,
             start_time,
             // make_unique<IntensityMap>(*model, nullptr),
             nullptr,
             make_shared<HashSize>(start_cell),
             start_point,
             start_day,
             last_date)
{
}
// HACK: just set next start point here for surface right now
Scenario* Scenario::reset_with_new_start(const shared_ptr<HashSize>& start_cell,
                                         util::SafeVector* final_sizes)
{
  start_cell_ = start_cell;
  // FIX: remove duplicated code
  // logging::extensive("Set cell; resetting");
  // return reset(nullptr, nullptr, final_sizes);
  cancelled_ = false;
  current_time_ = start_time_;
#ifdef USE_OLD_SPREAD
  intensity_ = nullptr;
#endif
#ifdef USE_NEW_SPREAD
  intensity_new_ = nullptr;
#endif
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
  ran_ = false;
  clear();
  current_time_ = start_time_ - 1;
#ifdef USE_OLD_SPREAD
  points_ = {};
#endif
#ifdef USE_OLD_SPREAD
  intensity_ = make_unique<IntensityMap>(model());
#endif
#ifdef USE_NEW_SPREAD
  intensity_new_ = make_unique<IntensityMap>(model());
#endif
  // HACK: never reset these if using a surface
  // if (!Settings::surface())
  {
    // these are reset in clear()
    // spread_info_ = {};
    // max_ros_ = 0;
    // surrounded_ = POOL_BURNED_DATA.acquire();
    current_time_index_ = numeric_limits<size_t>::max();
  }
  ++COUNT;
  {
    // want a global count of how many times this scenario ran
    std::lock_guard<std::mutex> lk(MUTEX_SIM_COUNTS);
    simulation_ = ++SIM_COUNTS[id_];
  }
  return this;
}
Scenario* Scenario::reset(mt19937* mt_extinction,
                          mt19937* mt_spread,
                          util::SafeVector* final_sizes)
{
  cancelled_ = false;
  current_time_ = start_time_;
#ifdef USE_OLD_SPREAD
  intensity_ = nullptr;
#endif
#ifdef USE_NEW_SPREAD
  intensity_new_ = nullptr;
#endif
  //  weather_(weather);
  //  model_(model);
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
  //  start_point_(std::move(start_point));
  //  id_(id);
  //  start_time_(start_time);
  //  simulation_(-1);
  //  start_day_(start_day);
  //  last_date_(last_date);
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
    make_threshold(&extinction_thresholds_, mt_extinction, start_day_, last_date_);
  }
  if (nullptr != mt_spread)
  {
    make_threshold(&spread_thresholds_by_ros_,
                   mt_spread,
                   start_day_,
                   last_date_,
                   &SpreadInfo::calculateRosFromThreshold);
  }
  //  std::fill(extinction_thresholds_.begin(), extinction_thresholds_.end(), 1.0 - abs(1.0 / (10 * id_)));
  //  std::fill(spread_thresholds_by_ros_.begin(), spread_thresholds_by_ros_.end(), 1.0 - abs(1.0 / (10 * id_)));
  // std::fill(extinction_thresholds_.begin(), extinction_thresholds_.end(), 0.5);
  //  std::fill(spread_thresholds_by_ros_.begin(), spread_thresholds_by_ros_.end(), SpreadInfo::calculateRosFromThreshold(0.5));
  current_time_ = start_time_ - 1;
#ifdef USE_OLD_SPREAD
  points_ = {};
#endif
// don't do this until we run so that we don't allocate memory too soon
// log_verbose("Applying initial intensity map");
// // HACK: if initial_intensity is null then perimeter must be too?
// intensity_ = (nullptr == initial_intensity_)
//   ? make_unique<IntensityMap>(model(), nullptr)
//   : make_unique<IntensityMap>(*initial_intensity_);
#ifdef USE_OLD_SPREAD
  intensity_ = make_unique<IntensityMap>(model());
#endif
#ifdef USE_NEW_SPREAD
  intensity_new_ = make_unique<IntensityMap>(model());
#endif
  spread_info_ = {};
  max_ros_ = 0;
  // surrounded_ = POOL_BURNED_DATA.acquire();
  current_time_index_ = numeric_limits<size_t>::max();
  ++COUNT;
  {
    // want a global count of how many times this scenario ran
    std::lock_guard<std::mutex> lk(MUTEX_SIM_COUNTS);
    simulation_ = ++SIM_COUNTS[id_];
  }
  return this;
}
#ifdef USE_NEW_SPREAD
#ifdef DEBUG_NEW_SPREAD
template <class V>
int compare(const V& a, const V& b)
{
  return a == b
         ? 0
       : a < b
         ? -1
         : 1;
}
[[nodiscard]] int compare_pts(
  const string& msg,
  const set<XYPos>& pts_old,
  const set<XYPos>& pts_new)
{
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  show_points<XYPos, XYSize, std::set<XYPos>>(pts_old, "%s Old points are", msg.c_str());
  show_points<XYPos, XYSize, std::set<XYPos>>(pts_new, "%s New points are", msg.c_str());
#endif
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::verbose(
    "Comparing %s %ld old points to %ld new points",
    msg.c_str(),
    pts_old.size(),
    pts_new.size());
#endif
  const auto n_new = pts_new.size();
  const auto n_old = pts_old.size();
  if (n_new != n_old)
  {
    logging::error(
      "Have %ld old points and %ld new points",
      pts_old.size(),
      pts_new.size());
    show_points<XYPos, XYSize, std::set<XYPos>>(pts_old, "Old points for %s", msg.c_str());
    show_points<XYPos, XYSize, std::set<XYPos>>(pts_new, "New points for %s", msg.c_str());
    vector<XYPos> diff{};
    std::set_difference(
      pts_old.begin(),
      pts_old.end(),
      pts_new.begin(),
      pts_new.end(),
      std::back_inserter(diff));
    show_points<XYPos, XYSize, vector<XYPos>>(
      diff,
      "Difference %s",
      msg.c_str());
    return compare(n_old, n_new);
  }
  else
  {
    auto it_old = pts_old.cbegin();
    auto it_new = pts_new.cbegin();
    while (it_old != pts_old.cend())
    {
      auto& p0 = *it_new;
      auto& p1 = *it_old;
#ifdef DEBUG_NEW_SPREAD_VERBOSE
      // logging::info("(%f, %f)", p0.x(), p0.y());
#endif
      if (!pts_new.contains(p0))
      {
        logging::error(
          "Point (%f, %f) doesn't exist in %s new points",
          msg.c_str(),
          p0.x(),
          p0.y());
        return compare(p0, p1);
      }
      if (!pts_old.contains(p1))
      {
        logging::error(
          "Point (%f, %f) doesn't exist in %s old points",
          msg.c_str(),
          p1.x(),
          p1.y());
        return compare(p0, p1);
      }
      if (p1.x() != p0.x() || p1.y() != p0.y())
      {
        logging::error(
          "%s (%f, %f) != (%f, %f)",
          msg.c_str(),
          p0.x(),
          p0.y(),
          p1.x(),
          p1.y());
        return compare(p0, p1);
      }
      ++it_old;
      ++it_new;
    }
  }
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::info(
    "Compared %s points are equal",
    msg.c_str());
#endif
  return 0;
}
[[nodiscard]] int compare_pts(
  const string& msg,
  const spreading_points& to_spread,
  const spreading_points_new& to_spread_new)
{
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::info(
    "Comparing %s spreading points",
    msg.c_str());
  logging::info(
    "Generating %s old spreading points",
    msg.c_str());
#endif
  set<XYPos> spread_old{};
  for (auto& kv0 : to_spread)
  {
    for (const auto& kv1 : kv0.second)
    {
      // auto& c = kv1.first;
      auto& d = kv1.second;
      auto u = d.unique();
      spread_old.insert(u.cbegin(), u.cend());
    }
  }
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::info(
    "Generating %s new spreading points",
    msg.c_str());
#endif
  set<XYPos> spread_new{};
  for (auto& kv0 : to_spread_new)
  {
    for (auto& kv1 : kv0.second)
    {
      auto& u = kv1.second;
      spread_new.insert(u.cbegin(), u.cend());
    }
  }
  return compare_pts(msg, spread_old, spread_new);
}
[[nodiscard]] int compare_pts(
  const string& msg,
  const spreading_points::mapped_type& to_spread,
  const spreading_points_new::mapped_type& to_spread_new)
{
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::verbose(
    "Comparing %s spreading points",
    msg.c_str());
  logging::verbose(
    "Generating %s old spreading points",
    msg.c_str());
#endif
  set<XYPos> spread_old{};
  for (auto& kv0 : to_spread)
  {
    auto& d = kv0.second;
    auto u = d.unique();
    spread_old.insert(u.cbegin(), u.cend());
  }
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::verbose(
    "Generating %s new spreading points",
    msg.c_str());
#endif
  set<XYPos> spread_new{};
  for (auto& kv0 : to_spread_new)
  {
    auto& u = kv0.second;
    spread_new.insert(u.cbegin(), u.cend());
  }
  return compare_pts(msg, spread_old, spread_new);
}

template <class A, class B>
void check_pts(
  const string msg,
  A pts_old,
  B pts_new)
{
  logging::check_fatal(
    0 != compare_pts(msg, pts_old, pts_new),
    "Points are different for %s",
    msg.c_str());
}
#endif
#endif
void Scenario::evaluate(const Event& event)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(event.time() < current_time_,
                  "Expected time to be > %f but got %f",
                  current_time_,
                  event.time());
#endif
  const auto& p = event.cell();
  const auto x = p.column() + CELL_CENTER;
  const auto y = p.row() + CELL_CENTER;
  const XYPos p0{x, y};
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "evaluate()",
    points_.unique(),
    points_new_.unique());
#endif
  switch (event.type())
  {
    case Event::FIRE_SPREAD:
      ++step_;
#ifdef DEBUG_POINTS
      // if (fs::logging::Log::getLogLevel() >= fs::logging::LOG_VERBOSE)
      {
        const auto ymd = fs::make_timestamp(model().year(), event.time());
        // log_verbose("Handling spread event for time %f representing %s with %ld points", event.time(), ymd.c_str(), points_.size());
      }
#endif
      scheduleFireSpread(event);
#ifdef DEBUG_NEW_SPREAD
      printf("*****************************************\n");
#endif
      break;
    case Event::SAVE:
      saveStats(event.time());
      break;
    case Event::NEW_FIRE:
// HACK: don't do this in constructor because scenario creates this in its constructor
// HACK: insert point as originating from itself
#ifdef USE_OLD_SPREAD
      points_.insert(
        *intensity_,
        p0);
#endif
#ifdef USE_NEW_SPREAD
      points_new_.insert(
        *intensity_new_,
        p0);
#endif
#ifdef DEBUG_NEW_SPREAD_CHECK
      check_pts(
        "NEW_FIRE",
        points_.unique(),
        points_new_.unique());
#endif
      if (fuel::is_null_fuel(event.cell()))
      {
        log_fatal("Trying to start a fire in non-fuel");
      }
      log_verbose("Starting fire at point (%f, %f) in fuel type %s at time %f",
                  x,
                  y,
                  fuel::FuelType::safeName(fuel::check_fuel(event.cell())),
                  event.time());
      if (!survives(event.time(), event.cell(), event.timeAtLocation()))
      {
        // const auto wx = weather(event.time());
        // HACK: show daily values since that's what survival uses
        const auto wx = weather_daily(event.time());
        log_info("Didn't survive ignition in %s with weather %f, %f",
                 fuel::FuelType::safeName(fuel::check_fuel(event.cell())),
                 wx->ffmc(),
                 wx->dmc());
        // HACK: we still want the fire to have existed, so set the intensity of the origin
      }
// fires start with intensity of 1
#ifdef USE_OLD_SPREAD
      intensity_->ignite(event.cell().hash());
#endif
#ifdef USE_NEW_SPREAD
      intensity_new_->ignite(event.cell().hash());
#endif
#if defined(USE_OLD_SPREAD) && defined(USE_NEW_SPREAD)
      logging::check_equal(
        intensity_->hasBurned(event.cell().hash()),
        intensity_new_->hasBurned(event.cell().hash()),
        "has burned");
#endif
      scheduleFireSpread(event);
      break;
    case Event::END_SIMULATION:
      log_verbose("End simulation event reached at %f", event.time());
      endSimulation();
      break;
    default:
      throw runtime_error("Invalid event type");
  }
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   wx::FireWeather* weather_daily,
                   const DurationSize start_time,
                   //  const shared_ptr<IntensityMap>& initial_intensity,
                   const shared_ptr<Perimeter>& perimeter,
                   const shared_ptr<HashSize>& start_cell,
                   StartPoint start_point,
                   const Day start_day,
                   const Day last_date)
  : current_time_(start_time),
#ifdef USE_OLD_SPREAD
    intensity_(nullptr),
#endif
#ifdef USE_NEW_SPREAD
    intensity_new_(nullptr),
#endif
    // initial_intensity_(initial_intensity),
    perimeter_(perimeter),
    // surrounded_(nullptr),
    max_ros_(0),
    start_cell_(start_cell),
    weather_(weather),
    weather_daily_(weather_daily),
    model_(model),
    probabilities_(nullptr),
    final_sizes_(nullptr),
    start_point_(std::move(start_point)),
    id_(id),
    start_time_(start_time),
    simulation_(-1),
    start_day_(start_day),
    last_date_(last_date),
    ran_(false),
    step_(0),
    oob_spread_(0)
{
  last_save_ = weather_->minDate();
  const auto wx = weather_->at(start_time_);
  logging::check_fatal(nullptr == wx,
                       "No weather for start time %s",
                       make_timestamp(model->year(), start_time_).c_str());
  const auto saves = Settings::outputDateOffsets();
  const auto last_save = start_day_ + saves[saves.size() - 1];
  logging::check_fatal(last_save > weather_->maxDate(),
                       "No weather for last save time %s",
                       make_timestamp(model->year(), last_save).c_str());
}
void Scenario::saveStats(const DurationSize time) const
{
#if defined(DEBUG_NEW_SPREAD) && defined(USE_OLD_SPREAD)
  check_pts(
    "saveStats()",
    points_.unique(),
    points_new_.unique());
#endif
  const auto& intensity =
#ifdef USE_NEW_SPREAD
    intensity_new_;
#else
    intensity_;
#endif
  probabilities_->at(time)->addProbability(*intensity);
  if (time == last_save_)
  {
    final_sizes_->addValue(intensity->fireSize());
  }
}
bool Scenario::ran() const noexcept
{
  return ran_;
}
Scenario::Scenario(Scenario&& rhs) noexcept
  : save_points_(std::move(rhs.save_points_)),
    extinction_thresholds_(std::move(rhs.extinction_thresholds_)),
    spread_thresholds_by_ros_(std::move(rhs.spread_thresholds_by_ros_)),
    current_time_(rhs.current_time_),
#ifdef USE_OLD_SPREAD
    points_(std::move(rhs.points_)),
#endif
#ifdef USE_NEW_SPREAD
    points_new_(std::move(rhs.points_new_)),
#endif
    scheduler_(std::move(rhs.scheduler_)),
#ifdef USE_OLD_SPREAD
    intensity_(std::move(rhs.intensity_)),
#endif
#ifdef USE_NEW_SPREAD
    intensity_new_(std::move(rhs.intensity_new_)),
#endif
    // initial_intensity_(std::move(rhs.initial_intensity_)),
    perimeter_(std::move(rhs.perimeter_)),
    spread_info_(std::move(rhs.spread_info_)),
    max_ros_(rhs.max_ros_),
    start_cell_(std::move(rhs.start_cell_)),
    weather_(rhs.weather_),
    weather_daily_(rhs.weather_daily_),
    model_(rhs.model_),
    probabilities_(rhs.probabilities_),
    final_sizes_(rhs.final_sizes_),
    start_point_(std::move(rhs.start_point_)),
    id_(rhs.id_),
    start_time_(rhs.start_time_),
    last_save_(rhs.last_save_),
    simulation_(rhs.simulation_),
    start_day_(rhs.start_day_),
    last_date_(rhs.last_date_),
    ran_(rhs.ran_)
{
}
Scenario& Scenario::operator=(Scenario&& rhs) noexcept
{
  if (this != &rhs)
  {
    save_points_ = std::move(rhs.save_points_);
    extinction_thresholds_ = std::move(rhs.extinction_thresholds_);
    spread_thresholds_by_ros_ = std::move(rhs.spread_thresholds_by_ros_);
#ifdef USE_OLD_SPREAD
    points_ = std::move(rhs.points_);
#endif
#ifdef USE_NEW_SPREAD
    points_new_ = std::move(rhs.points_new_);
#endif
    current_time_ = rhs.current_time_;
    scheduler_ = std::move(rhs.scheduler_);
#ifdef USE_OLD_SPREAD
    intensity_ = std::move(rhs.intensity_);
#endif
#ifdef USE_NEW_SPREAD
    intensity_new_ = std::move(rhs.intensity_new_);
#endif
    // initial_intensity_ = std::move(rhs.initial_intensity_);
    perimeter_ = std::move(rhs.perimeter_);
    // surrounded_ = rhs.surrounded_;
    start_cell_ = std::move(rhs.start_cell_);
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
  }
  return *this;
}
bool Scenario::isSurrounded(const HashSize hash_value) const
{
  const auto& intensity =
#ifdef USE_OLD_SPREAD
    intensity_;
#else
    intensity_new_;
#endif
  return intensity->isSurrounded(hash_value);
}
Cell Scenario::cell(const InnerPos& p) const noexcept
{
  return cell(p.y(), p.x());
}
string Scenario::add_log(const char* format) const noexcept
{
  const string tmp;
  stringstream iss(tmp);
  static char buffer[1024]{0};
  sxprintf(buffer, "Scenario %4ld.%04ld (%3f): ", id(), simulation(), current_time_);
  iss << buffer << format;
  return iss.str();
}
#ifdef DEBUG_PROBABILITY
void saveProbabilities(const string& dir,
                       const string& base_name,
                       vector<ThresholdSize>& thresholds)
{
  ofstream out;
  out.open(dir + base_name + ".csv");
  for (auto v : thresholds)
  {
    out << v << '\n';
  }
  out.close();
}
#endif
Scenario* Scenario::run(map<DurationSize, shared_ptr<ProbabilityMap>>* probabilities)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(ran(), "Scenario has already run");
#endif
  log_verbose("Starting");
  CriticalSection _(Model::task_limiter);
  logging::debug("Concurrent Scenario limit is %d", Model::task_limiter.limit());
  probabilities_ = probabilities;
  log_verbose("Setting save points");
  for (auto time : save_points_)
  {
    // NOTE: these happen in this order because of the way they sort based on type
    addEvent(Event::makeSave(static_cast<DurationSize>(time)));
  }
  if (nullptr == perimeter_)
  {
    addEvent(Event::makeNewFire(start_time_, cell(*start_cell_)));
  }
  else
  {
    log_verbose("Applying perimeter");
#ifdef USE_OLD_SPREAD
    intensity_->applyPerimeter(*perimeter_);
#endif
#ifdef USE_NEW_SPREAD
    intensity_new_->applyPerimeter(*perimeter_);
#endif
    log_verbose("Perimeter applied");
    const auto& env = model().environment();
    log_verbose("Igniting points");
    for (const auto& location : perimeter_->edge())
    {
      //      const auto cell = env.cell(location.hash());
      const auto cell = env.cell(location);
#ifdef DEBUG_SIMULATION
      log_check_fatal(fuel::is_null_fuel(cell), "Null fuel in perimeter");
#endif
      const auto x = cell.column() + CELL_CENTER;
      const auto y = cell.row() + CELL_CENTER;
      const XYPos p0{x, y};
      // log_extensive("Adding point (%d, %d)",
      log_extensive("Adding point (%f, %f)",
                    x,
                    y);
#ifdef USE_OLD_SPREAD
      points_.insert(
        *intensity_,
        p0);
#endif
#ifdef USE_NEW_SPREAD
      points_new_.insert(
        *intensity_new_,
        p0);
#endif
      // auto e = points_.try_emplace(cell, cell.column() + CELL_CENTER, cell.row() + CELL_CENTER);
      // log_check_fatal(!e.second,
      //                 "Excepted to add point to new cell but (%ld, %ld) is already in map",
      //                 cell.column(),
      //                 cell.row());
    }
    addEvent(Event::makeFireSpread(start_time_));
  }
  // HACK: make a copy of the event so that it still exists after it gets processed
  // NOTE: sorted so that EventSaveASCII is always just before this
  // Only run until last time we asked for a save for
  log_verbose("Creating simulation end event for %f", last_save_);
  addEvent(Event::makeEnd(last_save_));
// mark all original points as burned at start
#ifdef USE_OLD_SPREAD
  for (auto& kv : points_.map_)
  {
    const auto& hash_value = kv.first;
    const auto& location = cell(hash_value);
    // const auto& location = kv.first;
    // would be burned already if perimeter applied
    if (intensity_->hasNotBurned(hash_value))
    // if (!isUnburnable(hash_value))
    {
      const auto fake_event = Event::makeFireSpread(
        start_time_,
        location);
      intensity_->burn(fake_event);
    }
  }
#endif
#ifdef USE_NEW_SPREAD
  // mark all original points as burned at start
  for (const auto& hash_value : points_new_.keys())
  {
    const auto& location = cell(hash_value);
    // const auto& location = kv.first;
    // would be burned already if perimeter applied
    // if (hasNotBurned(hash_value))
    // if (!isUnburnable(hash_value))
    if (intensity_new_->canBurn(hash_value))
    {
      const auto fake_event = Event::makeFireSpread(
        start_time_,
        location);
      intensity_new_->burn(fake_event);
    }
  }
#endif
  while (!cancelled_ && !scheduler_.empty())
  {
    evaluateNextEvent();
    // // FIX: the timer thread can cancel these instead of having this check
    // if (!evaluateNextEvent())
    // {
    //   cancel(true);
    // }
  }
  ++TOTAL_STEPS;
  if (cancelled_)
  {
    return nullptr;
  }
  const auto completed = ++COMPLETED;
  const auto log_level = (0 == (completed % 1000)) ? logging::LOG_NOTE : logging::LOG_INFO;
  {
#ifdef NDEBUG
    // HACK: use + to pull value out of atomic
    const auto count = (+COUNT);
    log_output(log_level,
               "[% d of % d] Completed with final size % 0.1f ha",
               completed,
               count,
               currentFireSize());
#else
    // try to make output consistent if in debug mode
    log_output(log_level, "Completed with final size %0.1f ha", currentFireSize());
#endif
  }
  ran_ = true;
#ifdef DEBUG_PROBABILITY
  // nice to have this get output when debugging, but only need it in extreme cases
  if (logging::Log::getLogLevel() <= logging::LOG_EXTENSIVE)
  {
    char buffer[64]{0};
    sxprintf(buffer,
             "%03zu_%06ld_extinction",
             id(),
             simulation());
    saveProbabilities(model().outputDirectory(), string(buffer), extinction_thresholds_);
    sxprintf(buffer,
             "%03zu_%06ld_spread",
             id(),
             simulation());
    saveProbabilities(model().outputDirectory(), string(buffer), spread_thresholds_by_ros_);
  }
#endif
  if (oob_spread_ > 0)
  {
    log_warning("Tried to spread out of bounds %ld times", oob_spread_);
  }
  return this;
}
void apply_offsets_spreadkey(
#ifdef USE_NEW_SPREAD
  PtMap& points_new,
#endif
#if defined(DEBUG_NEW_SPREAD) || !defined(USE_OLD_SPREAD)
  spreading_points_new::mapped_type& pts_spreading_new,
#endif
#ifdef USE_OLD_SPREAD
  CellPointsMap& points,
#endif
  const Scenario& scenario,
  const DurationSize& arrival_time,
  const DurationSize& duration,
  const OffsetSet& offsets
#ifdef USE_OLD_SPREAD
  ,
  spreading_points::mapped_type& pts_spreading
#endif
)
{
// NOTE: really tried to do this in parallel, but not enough points
// in a cell for it to work well
// vector<XYPos> r1{};
#ifdef DEBUG_NEW_SPREAD_CHECK
  const auto u_old = points.unique();
  const auto u_new = points_new.unique();
  check_pts(
    "apply_offsets_spreadkey start",
    u_old,
    u_new);
  check_pts(
    "spreading points",
    pts_spreading,
    pts_spreading_new);
#endif
  OffsetSet offsets_after_duration{};
#ifdef DEBUG_CELLPOINTS
  logging::verbose("Applying %ld offsets", offsets.size());
#endif
  // // offsets_after_duration.resize(offsets.size());
  // std::transform(
  //   offsets.cbegin(),
  //   offsets.cend(),
  //   std::back_inserter(offsets_after_duration),
  offsets_after_duration.resize(offsets.size());
  std::transform(
    offsets.cbegin(),
    offsets.cend(),
    offsets_after_duration.begin(),
    [&duration, &arrival_time](const ROSOffset& r_p) {
      const auto& p = std::get<0>(r_p);
      // logging::verbose("ros %f; x %f; y %f; duration %f;",
      //               ros,
      //               p.x(),
      //               p.y(),
      //               duration);
      return ROSOffset(
        Offset(p.x() * duration, p.y() * duration));
    });
#ifdef DEBUG_CELLPOINTS
  logging::debug("Calculated %ld offsets after duration %f", offsets_after_duration.size(), duration);
#endif
#ifdef USE_OLD_SPREAD
  logging::debug("pts_spreading has %ld items", pts_spreading.size());
#endif
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::debug("pts_spreading_new has %ld items", pts_spreading_new.size());
#endif
// #endif
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "pts_spreading old vs new",
    pts_spreading,
    pts_spreading_new);
#endif
  size_t i = 0;
#ifdef USE_OLD_SPREAD
  for (auto& kv : pts_spreading)
#else
  for (auto& kv : pts_spreading_new)
#endif
  {
    ++i;
#ifdef USE_OLD_SPREAD
    CellPoints& cell_pts = std::get<1>(kv);
#else
    auto& cell_pts = kv.second;
#endif
#ifdef DEBUG_NEW_SPREAD
    Location src{kv.first};
    const auto hash_value = src.hash();
    logging::check_equal(
      hash_value,
      kv.first,
      "key hash");
#endif
    // auto cell_pts_new = points_new.map_.at(hash_value);
#ifdef DEBUG_CELLPOINTS
    scenario.log_verbose(
      "cell_pts for (%d, %d) has %ld items",
      src.column(),
      src.row(),
      cell_pts.size());
#endif
    if (cell_pts.empty())
    {
#ifdef DEBUG_CELLPOINTS
      scenario.log_verbose(
        "Cell (%d, %d) ignored because empty",
        src.column(),
        src.row());
#endif
      continue;
    }
// auto& pts = cell_pts.pts_.points();
#ifdef USE_OLD_SPREAD
    auto pts = cell_pts.unique();
#else
    auto& pts = cell_pts;
#endif
#ifdef DEBUG_NEW_SPREAD_CHECK
    auto& pair_new = pts_spreading_new[i - 1];
    logging::check_equal(
      pair_new.first,
      hash_value,
      "pair hash");
    auto pts_new = pair_new.second;
    check_pts(
      "apply_offsets_spreadkey before insert",
      pts,
      pts_new);
    check_pts(
      "apply_offsets_spreadkey before insert 0.5",
      points.unique(hash_value),
      points_new.unique(hash_value));
#endif
    // combine point and direction that lead to it so we can get unique values
    // c++23 is where zip() gets implemented
    // auto pt_dirs = std::ranges::views::zip(pts, dirs);
    for (const auto& pt : pts)
    {
      // const auto& cell_x = cell_pts.cell_x_y_.x();
      // const auto& cell_y = cell_pts.cell_x_y_.y();
      // // FIX: HACK: recompose into XYPos
      // const XYPos src{location.column() + cell_x, location.row() + cell_y};
      // const XYPos src{pt.x() + cell_x, pt.y() + cell_y};
      // apply offsets to point
      // should be quicker to loop over offsets in inner loop
      for (const ROSOffset& r_p : offsets_after_duration)
      {
        const auto& out = std::get<0>(r_p);
        const auto& x_o = out.x();
        const auto& y_o = out.y();
        {
          const auto new_x = x_o + pt.x();
          const auto new_y = y_o + pt.y();
          const XYPos p0{new_x, new_y};
#ifdef DEBUG_NEW_SPREAD_CHECK
          {
            auto u_all0 = points.unique();
            auto u_hash0 = points.unique(hash_value);
            auto u_all1 = points_new.unique();
            auto u_hash1 = points_new.unique(hash_value);
            scenario.log_verbose(
              "Size %ld vs %ld and %ld vs %ld",
              u_all0.size(),
              u_all1.size(),
              u_hash0.size(),
              u_hash1.size());
            check_pts(
              "apply_offsets_spreadkey after insert 2",
              u_all0,
              u_all1);
            check_pts(
              "apply_offsets_spreadkey after insert 2.5",
              u_hash0,
              u_hash1);
          }
#endif
#ifdef USE_OLD_SPREAD
          points.insert(
            *scenario.intensity_,
            p0);
#endif
#ifdef USE_NEW_SPREAD
          points_new.insert(
            *scenario.intensity_new_,
            p0);
#endif
#ifdef DEBUG_CELLPOINTS
          scenario.log_verbose("points is now %ld items", points.size());
#ifdef DEBUG_NEW_SPREAD
          scenario.log_verbose("points_new is now %ld items", points_new.size());
          const string msg =
            "apply_offsets_spreadkey after insert in "
            + string((*(scenario.unburnable_))[p0.hash()]
                       ? "unburnable"
                       : "burnable")
            + " cell";
          check_pts(
            msg,
            points.unique(),
            points_new.unique());
#endif
#endif
#ifdef DEBUG_NEW_SPREAD_CHECK
          auto u_all0 = points.unique();
          auto u_hash0 = points.unique(hash_value);
          auto u_all1 = points_new.unique();
          auto u_hash1 = points_new.unique(hash_value);
          scenario.log_verbose(
            "Size %ld vs %ld and %ld vs %ld",
            u_all0.size(),
            u_all1.size(),
            u_hash0.size(),
            u_hash1.size());
          check_pts(
            "apply_offsets_spreadkey after insert 2",
            u_all0,
            u_all1);
          check_pts(
            "apply_offsets_spreadkey after insert 2.5",
            u_hash0,
            u_hash1);
#endif
        }
#ifdef DEBUG_NEW_SPREAD_CHECK
        auto u_all0 = points.unique();
        auto u_all1 = points_new.unique();
        auto u_hash0 = points.unique(hash_value);
        auto u_hash1 = points_new.unique(hash_value);
        scenario.log_verbose(
          "Size %ld vs %ld and %ld vs %ld",
          u_all0.size(),
          u_all1.size(),
          u_hash0.size(),
          u_hash1.size());
        check_pts(
          "apply_offsets_spreadkey after insert 3",
          u_all0,
          u_all1);
        check_pts(
          "apply_offsets_spreadkey after insert 3.5",
          u_hash0,
          u_hash1);
#endif
      }
#ifdef DEBUG_NEW_SPREAD_CHECK
      auto u_all0 = points.unique();
      auto u_hash0 = points.unique(hash_value);
      auto u_all1 = points_new.unique();
      auto u_hash1 = points_new.unique(hash_value);
      scenario.log_verbose(
        "Size %ld vs %ld and %ld vs %ld",
        u_all0.size(),
        u_all1.size(),
        u_hash0.size(),
        u_hash1.size());
      check_pts(
        "apply_offsets_spreadkey after offset loop 4",
        u_all0,
        u_all1);
      check_pts(
        "apply_offsets_spreadkey after offset loop 4.5",
        u_hash0,
        u_hash1);
#endif
    }
#ifdef DEBUG_NEW_SPREAD_CHECK
    auto u_all0 = points.unique();
    auto u_hash0 = points.unique(hash_value);
    auto u_all1 = points_new.unique();
    auto u_hash1 = points_new.unique(hash_value);
    scenario.log_verbose(
      "Size %ld vs %ld and %ld vs %ld",
      u_all0.size(),
      u_all1.size(),
      u_hash0.size(),
      u_hash1.size());
    check_pts(
      "apply_offsets_spreadkey after points loop 5",
      u_all0,
      u_all1);
    check_pts(
      "apply_offsets_spreadkey after points loop 5.5",
      u_hash0,
      u_hash1);
#endif
  }
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "apply_offsets_spreadkey end",
    points.unique(),
    points_new.unique());
#endif
  // return r1;
}
void Scenario::scheduleFireSpread(const Event& event)
{
  const auto time = event.time();
  // HACK: if a surface then always use 1600 weather
  // keeps a bunch of things we don't need in it if we don't reset?
  // const auto this_time = Settings::surface() ? util::time_index(static_cast<Day>(time), 16) : util::time_index(time);
  const auto this_time = util::time_index(time);
  // const auto wx_time = Settings::surface() ? util::to_time(util::time_index(static_cast<Day>(time), 16)) : util::to_time(this_time);
  // const auto wx_time = util::to_time(this_time);
  const auto& wx =
    weather(time);
  const auto& wx_daily =
    weather_daily(time);
  // note("time is %f", time);
  current_time_ = time;
  logging::check_fatal(nullptr == wx, "No weather available for time %f", time);
  //  log_note("%d points", points_->size());
  const auto next_time = static_cast<DurationSize>(this_time + 1) / DAY_HOURS;
  // should be in minutes?
  const auto max_duration = (next_time - time) * DAY_MINUTES;
  // log_verbose("time is %f, next_time is %f, max_duration is %f",
  //      time,
  //      next_time,
  //      max_duration);
  const auto max_time = time + max_duration / DAY_MINUTES;
  // if (wx->ffmc().asValue() < minimumFfmcForSpread(time))
  // HACK: use the old ffmc for this check to be consistent with previous version
  if (wx_daily->ffmc().asValue() < minimumFfmcForSpread(time))
  {
    addEvent(Event::makeFireSpread(max_time));
    log_extensive("Waiting until %f because of FFMC", max_time);
    return;
  }
  // log_note("There are %ld spread offsets calculated", spread_info_.size());
  if (current_time_index_ != this_time)
  {
    // logging::check_fatal(Settings::surface() && current_time_index_ != numeric_limits<size_t>::max(),
    //                      "Expected to only pick weather time once");
    current_time_index_ = this_time;
    // seemed like it would be good to keep offsets but max_ros_ needs to reset or things slow to a crawl?
    {
      spread_info_ = {};
    }
    max_ros_ = 0.0;
  }
  // get once and keep
  const auto ros_min = Settings::minimumRos();
#ifdef USE_NEW_SPREAD
  spreading_points_new to_spread_new{};
#endif
#ifdef DEBUG_NEW_SPREAD
  log_info("Finding spreading points");
#endif
#ifdef USE_OLD_SPREAD
  spreading_points to_spread{};
#endif
  // make block to prevent it being visible beyond use
  {
// if we use an iterator this way we don't need to copy keys to erase things
#ifdef USE_OLD_SPREAD
    auto it = points_.map_.begin();
#else
    auto it = points_new_.map_.begin();
#endif
#ifdef DEBUG_NEW_SPREAD
    size_t i = 0;
#endif
#ifdef USE_OLD_SPREAD
    while (it != points_.map_.end())
#else
    while (it != points_new_.map_.end())
#endif
    {
#ifdef DEBUG_NEW_SPREAD_VERBOSE
      log_info("Point %d", i);
#endif
      const HashSize hash_value = it->first;
      const Location loc{hash_value};
      const auto& for_cell = cell(hash_value);
      const auto key = for_cell.key();
      ROSSize ros = 0;
      // HACK: need to lookup before emplace since might try to create Cell without fuel
      // if (!fuel::is_null_fuel(loc))
      // const auto h = for_cell.hash();
      // if (!(cannotSpread(hash_value)))
      // if (hasNotBurned(hash_value))
      // if (!isUnburnable(hash_value))
#ifdef USE_OLD_SPREAD
      logging::check_fatal(
        intensity_->isUnburnable(hash_value),
        "Unburnable cell (%d, %d) in burning cells",
        loc.column(),
        loc.row());
#endif
      {
        // const SpreadInfo tmp{*this, time, key, nd(time), wx};
        {
          const auto& origin_inserted = spread_info_.try_emplace(key, *this, time, key, nd(time), wx);
          // any cell that has the same fuel, slope, and aspect has the same spread
          const auto& origin = origin_inserted.first->second;
          // filter out things not spreading fast enough here so they get copied if they aren't
          // isNotSpreading() had better be true if ros is lower than minimum
          ros = origin.headRos();
        }
        if (ros >= ros_min)
        {
          max_ros_ = max(max_ros_, ros);
          // NOTE: shouldn't be Cell if we're looking up by just Location later
#ifdef USE_OLD_SPREAD
          to_spread[key].emplace_back(hash_value, std::move(it->second));
#endif
#ifdef USE_NEW_SPREAD
          to_spread_new[key].emplace_back(hash_value, it->second.unique());
#endif
#ifdef USE_OLD_SPREAD
          it = points_.map_.erase(it);
#ifdef USE_NW_SPREAD
          points_new_.erase(hash_value);
#endif
#else
#ifdef USE_OLD_SPREAD
          points_.map_.erase(hash_value);
#endif
          it = points_new_.map_.erase(it);
#endif
#ifdef DEBUG_NEW_SPREAD_CHECK
          check_pts(
            "emplace into to_spread",
            to_spread,
            to_spread_new);
#endif
#ifdef DEBUG_CELLPOINTS
          auto& v = to_spread[key];
          const auto n = v.size();
          const auto& p = v[n - 1].second;
          log_verbose(
            "added %ld items to to_spread[%d][(%d, %d)] when spreading",
            p.size(),
            key,
            loc.column(),
            loc.row());
#endif
        }
        else
        {
          ++it;
#ifdef DEBUG_CELLPOINTS
          log_extensive(
            "not spreading[%d][(%d, %d)]",
            key,
            loc.column(),
            loc.row());
#endif
        }
#ifdef DEBUG_CELLPOINTS
        check_pts(
          "during making to_spread 1",
          to_spread,
          to_spread_new);
#endif
#ifdef DEBUG_NEW_SPREAD_VERBOSE
        check_pts(
          "points during making to_spread 1",
          points_.unique(),
          points_new_.unique());
#endif
#ifdef DEBUG_NEW_SPREAD
        ++i;
#endif
      }
    }
  }
  // if nothing in to_spread then nothing is spreading
#ifdef USE_OLD_SPREAD
  if (to_spread.empty())
#else
  if (to_spread_new.empty())
#endif
  {
    // if no spread then we left everything back in points_ still
    log_verbose("Waiting until %f", max_time);
    addEvent(Event::makeFireSpread(max_time));
    return;
  }
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "after spread empty check",
    to_spread,
    to_spread_new);
#endif
  // note("Max spread is %f, max_ros is %f",
  //      Settings::maximumSpreadDistance() * cellSize(),
  //      max_ros_);
  const auto duration = ((max_ros_ > 0)
                           ? min(max_duration,
                                 Settings::maximumSpreadDistance() * cellSize() / max_ros_)
                           : max_duration);
  // note("Spreading for %f minutes", duration);
  const auto new_time = time + duration / DAY_MINUTES;
#ifdef USE_OLD_SPREAD
  CellPointsMap cell_pts{};
#endif
#ifdef USE_NEW_SPREAD
  PtMap cell_pts_new{};
#endif
#ifdef USE_OLD_SPREAD
  for (spreading_points::value_type& kv0 : to_spread)
#else
  for (auto& kv0 : to_spread_new)
#endif
  {
    auto& key = kv0.first;
    const auto& offsets = spread_info_[key].offsets();
#ifdef USE_OLD_SPREAD
    spreading_points::mapped_type& pts = kv0.second;
#ifdef DEBUG_NEW_SPREAD
    spreading_points_new::mapped_type& pts_spreading_new = to_spread_new[key];
#endif
#else
    spreading_points_new::mapped_type& pts_spreading_new = kv0.second;
#endif
#ifdef DEBUG_NEW_SPREAD_CHECK
    check_pts(
      "looping through to_spread start",
      to_spread,
      to_spread_new);
    check_pts(
      "pts vs pts_spreading_new",
      pts,
      pts_spreading_new);
    check_pts(
      "cell_pts vs new",
      cell_pts.unique(),
      cell_pts_new.unique());
#endif
    // FIX: just decomposing for now
    // auto r =
    apply_offsets_spreadkey(
#ifdef USE_NEW_SPREAD
      cell_pts_new,
#endif
#if defined(DEBUG_NEW_SPREAD) || !defined(USE_OLD_SPREAD)
      pts_spreading_new,
#endif
#ifdef USE_OLD_SPREAD
      cell_pts,
#endif
      *this,
      new_time,
      duration,
      offsets
#ifdef USE_OLD_SPREAD
      ,
      pts
#endif
    );
    // for (XYPos& p : r)
    // {
    //   // cell_pts.insert(p.x(), p.y());
    //   points_.insert(p.x(), p.y());
    // }
  }
#ifdef USE_OLD_SPREAD
  for (auto& p : points_.unique())
  {
    cell_pts.insert(*intensity_, p);
  }
  points_ = cell_pts;
#endif
  // check after inserting new points since cells that didn't spread could be surrounded now
#ifdef USE_NEW_SPREAD
  for (auto& p : points_new_.unique())
  {
    cell_pts_new.insert(*intensity_new_, p);
  }
  points_new_ = cell_pts_new;
#endif
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "pts before remove surrounded",
    points_.unique(),
    points_new_.unique());
#endif
#ifdef USE_OLD_SPREAD
  points_.remove_if(
    [this](
      const pair<HashSize, CellPoints>& kv) {
      const auto& hash_value = kv.first;
      const Location location{hash_value};
      // clear out if unburnable
      const auto do_clear = intensity_->cannotSpread(hash_value) || intensity_->isSurrounded(hash_value);
      // if (do_clear)
      // {
      //   logging::error(
      //     "Shouldn't have unburnable cell (%d, %d)",
      //     location.column(),
      //     location.row());
      // }
      return do_clear;
    });
#endif
  // if we move everything out of points_ we can parallelize this check?
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "spreading before burning cells",
    to_spread,
    to_spread_new);
#endif
#ifdef USE_NEW_SPREAD
  {
    auto it = points_new_.map_.begin();
    while (it != points_new_.map_.end())
    {
      auto& hash_value = it->first;
      if ((*intensity_new_->unburnable_)[hash_value] || intensity_new_->isSurrounded(hash_value))
      {
        it = points_new_.map_.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
#endif
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "pts before burning cells",
    points_.unique(),
    points_new_.unique());
#endif
#ifdef USE_OLD_SPREAD
  auto it = points_.map_.begin();
  while (it != points_.map_.end())
#else
  auto it = points_new_.map_.begin();
  while (it != points_new_.map_.end())
#endif
  {
    auto& kv = *it;
    // CellPoints& pts = kv.second;
    const auto& hash_value = kv.first;
    const auto for_cell = cell(hash_value);
#ifdef DEBUG_TEMPORARY
    const auto h_target = Location{2081, 1962}.hash();
    if (h_target == hash_value)
    {
      logging::info("Looking at cell (%d, %d)", kv.second.cell_x_y_.x(), kv.second.cell_x_y_.y());
    }
#endif
    // logging::check_fatal(pts.empty(), "Empty points for some reason");
    // ******************* CHECK THIS BECAUSE IF SOMETHING IS IN HERE SHOULD IT ALWAYS HAVE SPREAD????? *****************8
    const auto& seek_spread = spread_info_.find(for_cell.key());
    const auto max_intensity = (spread_info_.end() == seek_spread) ? 0 : seek_spread->second.maxIntensity();
    // // if we don't have empty cells anymore then intensity should always be >0?
    // logging::check_fatal(max_intensity <= 0,
    //                      "Expected max_intensity to be > 0 but got %f",
    //                      max_intensity);
    // HACK: just use side-effect to log and check bounds
    if (
#ifdef USE_OLD_SPREAD
      intensity_->hasNotBurned(hash_value)
#else
      intensity_new_->hasNotBurned(hash_value)
#endif
      // !isUnburnable(hash_value)
      && max_intensity > 0)
    {
      // // HACK: make sure it can't round down to 0
      // const auto intensity = static_cast<IntensitySize>(max(
      //   1.0,
      //   max_intensity));
      // HACK: just use the first cell as the source
      // FIX: HACK: only output spread within for now
      // const auto& spread = pts.spread_internal_;
      const auto fake_event = Event::makeFireSpread(
        new_time,
        for_cell);
#ifdef USE_OLD_SPREAD
      intensity_->burn(fake_event);
#endif
#ifdef USE_NEW_SPREAD
      intensity_new_->burn(fake_event);
#endif
    }
    auto& intensity =
#ifdef USE_OLD_SPREAD
      intensity_;
#else
      intensity_new_;
#endif
    if (!(intensity->cannotSpread(hash_value))
        // && hasNotBurned(for_cell)
        && ((survives(new_time, for_cell, new_time - intensity->arrival_[hash_value])
             && !intensity->isSurrounded(hash_value))))
    {
      // keep points because they could go somewhere
      ++it;
    }
    else
    {
#ifdef DEBUG_CELLPOINTS
      logging::info(
        "Removing %ld points because: cannot_spread=%d; does_survive=%d; is_surrounded=%d",
        it->second.unique().size(),
        cannotSpread(hash_value),
        survives(new_time, for_cell, new_time - arrival_[hash_value]),
        isSurrounded(hash_value));
#endif
      // just inserted false, so make sure unburnable gets updated
      // whether it went out or is surrounded just mark it as unburnable
#ifdef USE_OLD_SPREAD
      (*intensity_->unburnable_)[hash_value] = true;
      // FIX: old behaviour is to not remove these
      it = points_.map_.erase(it);
#endif
      // ++it;
#ifdef USE_NEW_SPREAD
#ifndef USE_OLD_SPREAD
      it = points_new_.map_.erase(it);
#else
      points_new_.erase(hash_value);
#endif
      (*intensity_new_->unburnable_)[hash_value] = true;
#endif
      // not swapping means these points get dropped
    }
  }
#ifdef DEBUG_NEW_SPREAD_CHECK
  check_pts(
    "pts after burning cells",
    points_.unique(),
    points_new_.unique());
#endif
#ifdef USE_OLD_SPREAD
  log_extensive("Spreading %d cells until %f", points_.map_.size(), new_time);
#endif
  addEvent(Event::makeFireSpread(new_time));
}
MathSize
  Scenario::currentFireSize() const
{
  const auto& intensity =
#ifdef USE_OLD_SPREAD
    intensity_;
#else
    intensity_new_;
#endif
  return intensity->fireSize();
}
// bool Scenario::canBurn(const HashSize hash_value) const
// {
//   return intensity_->canBurn(hash_value);
//   // return !isUnburnable(hash_value);
// }
void Scenario::endSimulation() noexcept
{
  log_verbose("Ending simulation");
  //  scheduler_.clear();
  scheduler_ = set<Event, EventCompare>();
}
void Scenario::addSaveByOffset(const int offset)
{
  // offset is from begging of the day the simulation starts
  // e.g. 1 is midnight, 2 is tomorrow at midnight
  addSave(static_cast<Day>(startTime()) + offset);
}
vector<DurationSize> Scenario::savePoints() const
{
  return save_points_;
}
template <class V>
void Scenario::addSave(V time)
{
  last_save_ = max(last_save_, static_cast<DurationSize>(time));
  save_points_.push_back(time);
}
void Scenario::addEvent(Event&& event)
{
  scheduler_.insert(std::move(event));
}
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
  // return !model_->isOutOfTime();
  // return cancelled_;
}
void Scenario::cancel(bool show_warning) noexcept
{
  // ignore if already cancelled
  if (!cancelled_)
  {
    cancelled_ = true;
    if (show_warning)
    {
      log_warning("Simulation cancelled");
    }
  }
}
}
