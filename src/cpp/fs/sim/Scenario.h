/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SCENARIO_H
#define FS_SCENARIO_H
#include "../stdafx.h"
#include "../grid/IntensityMap.h"
#include "../log/LogPoints.h"
#include "../spread/CellPoints.h"
#include "../spread/SpreadCache.h"
#include "../types/Location.h"
#include "../util/TimeUtil.h"
#include "../wx/FireWeather.h"
#include "Model.h"
#include "Settings.h"
#include "StartPoint.h"
namespace fs
{
class IObserver;
struct Event;
/**
 * \brief Deleter for IObserver to get around incomplete class with unique_ptr
 */
struct IObserver_deleter
{
  void operator()(IObserver*) const;
};
// use an array instead of a map since number of values is so small and access should be faster
using SurvivalMap = array<vector<float>, NUMBER_OF_FUELS>;
/**
 * \brief A single Scenario in an Iteration using a specific FireWeather stream.
 */
class Scenario
{
public:
  /**
   * \brief Number of Scenarios that have completed running
   * \return Number of Scenarios that have completed running
   */
  [[nodiscard]] static size_t completed() noexcept;
  /**
   * \brief Number of Scenarios that have been initialized
   * \return Number of Scenarios that have been initialized
   */
  [[nodiscard]] static size_t count() noexcept;
  /**
   * \brief Total number of spread events for all Scenarios
   * \return Total number of spread events for all Scenarios
   */
  [[nodiscard]] static size_t total_steps() noexcept;
  virtual ~Scenario();
  Scenario(Scenario&& rhs) noexcept;
  Scenario(const Scenario& rhs) = delete;
  Scenario& operator=(Scenario&& rhs) noexcept;
  Scenario& operator=(const Scenario& rhs) const = delete;
  // HACK: use for surface right now
  /**
   * \brief Assign start Cell, reset thresholds and set SafeVector to output results to
   * \param start_cell Cell to start ignition in
   * \param final_sizes SafeVector to output results to
   * \return This
   */
  [[nodiscard]] Scenario* reset_with_new_start(
    const XYIdx& start_cell,
    ptr<SafeVector> final_sizes
  );
  /**
   * \brief Reset thresholds and set SafeVector to output results to
   * \param mt_extinction Used for extinction random numbers
   * \param mt_spread Used for spread random numbers
   * \param final_sizes SafeVector to output results to
   * \return This
   */
  [[nodiscard]] Scenario* reset(
    mt19937_64* mt_extinction,
    mt19937_64* mt_spread,
    ptr<SafeVector> final_sizes
  );
  /**
   * \brief Burn cell that Event takes place in
   * \param event Event with cell location
   */
  void burn(const Event& event);
  /**
   * Mark as cancelled so it stops computing on next event.
   * \param Whether to log a warning about this being cancelled
   */
  void cancel(bool show_warning) noexcept;
  [[nodiscard]]
  Cell cell(const XYIdx xy) const
  {
    return model_->cell(xy);
  }
  [[nodiscard]] constexpr Idx height() const { return model_->height(); }
  [[nodiscard]] constexpr Idx width() const { return model_->width(); }
  /**
   * \brief Cell width and height (m)
   * \return Cell width and height (m)
   */
  [[nodiscard]] constexpr MathSize cellSize() const { return model_->cellSize(); }
  /**
   * \brief Simulation number
   * \return Simulation number
   */
  [[nodiscard]] constexpr int64_t simulation() const { return simulation_; }
  [[nodiscard]] constexpr DurationSize current_time() const { return current_time_; }
  /**
   * \brief StartPoint that provides sunrise/sunset times
   * \return StartPoint
   */
  [[nodiscard]] constexpr const StartPoint& startPoint() const { return start_point_; }
  /**
   * \brief Simulation start time
   * \return Simulation start time
   */
  [[nodiscard]] constexpr DurationSize startTime() const { return start_time_; }
  /**
   * \brief Identifier
   * \return Identifier
   */
  [[nodiscard]] constexpr size_t id() const { return id_; }
  /**
   * \brief Model this Scenario is running in
   * \return Model this Scenario is running in
   */
  [[nodiscard]] constexpr const Model& model() const { return *model_; }
  /**
   * \brief Sunrise time for given day
   * \param for_day Day to get sunrise time for
   * \return Sunrise time for given day
   */
  [[nodiscard]] constexpr DurationSize dayStart(const size_t for_day) const
  {
    return start_point_.dayStart(for_day);
  }
  /**
   * \brief Sunset time for given day
   * \param for_day Day to get sunset time for
   * \return Sunset time for given day
   */
  [[nodiscard]] constexpr DurationSize dayEnd(const size_t for_day) const
  {
    return start_point_.dayEnd(for_day);
  }
  /**
   * \brief FwiWeather for given time
   * \param time Time to get weather for (decimal days)
   * \return FwiWeather for given time
   */
  [[nodiscard]] FwiWeather weather(const DurationSize time) const { return weather_->at(time); }
  [[nodiscard]] FwiWeather weather_daily(const DurationSize time) const
  {
    return weather_daily_->at(time);
  }
  /**
   * \brief Difference between date and the date of minimum foliar moisture content
   * \param time Time to get value for
   * \return Difference between date and the date of minimum foliar moisture content
   */
  [[nodiscard]] constexpr int nd(const DurationSize time) const { return model().nd(time); }
  /**
   * \brief Get extinction threshold for given time
   * \param time Time to get value for
   * \return Extinction threshold for given time
   */
  [[nodiscard]] ThresholdSize extinctionThreshold(const DurationSize time) const
  {
    return extinction_thresholds_.at(time_index(time - start_day_));
  }
  /**
   * \brief Get spread threshold for given time
   * \param time Time to get value for
   * \return Spread threshold for given time
   */
  [[nodiscard]] ThresholdSize spreadThresholdByRos(const DurationSize time) const
  {
    return spread_thresholds_by_ros_.at(time_index(time - start_day_));
  }
  /**
   * \brief Whether or not time is after sunrise and before sunset
   * \param time Time to determine for
   * \return Whether or not time is after sunrise and before sunset
   */
  [[nodiscard]] constexpr bool isAtNight(const DurationSize time) const
  {
    const auto day = static_cast<Day>(time);
    const auto hour_part = 24 * (time - day);
    return hour_part < dayStart(day) || hour_part > dayEnd(day);
  }
  /**
   * \brief Minimum Fine Fuel Moisture Code for spread to be possible
   * \param time Time to determine for
   * \return Minimum Fine Fuel Moisture Code for spread to be possible
   */
  [[nodiscard]] MathSize minimumFfmcForSpread(const DurationSize time) const noexcept
  {
    // HACK: resolve once and fail if not set already
    static const auto& settings = fs::settings::instance();
    return isAtNight(time) ? settings.minimum_ffmc_at_night : settings.minimum_ffmc;
  }
  /**
   * \brief Whether or not the given Location is surrounded by cells that are burnt
   * \param location Location to check if is surrounded
   * \return Whether or not the given Location is surrounded by cells that are burnt
   */
  [[nodiscard]] bool isSurrounded(const XYIdx& location) const;
  /**
   * \brief Run the Scenario
   * \param probabilities map to update ProbabilityMap for times base on Scenario results
   * \return This
   */
  Scenario* run(map<DurationSize, shared_ptr<ProbabilityMap>>* probabilities);
  /**
   * \brief Schedule a fire spread Event
   * \param event Event to schedule
   */
  void scheduleFireSpread(const Event& event);
  /**
   * \brief Current fire size (ha)
   * \return Current fire size (ha)
   */
  [[nodiscard]] MathSize currentFireSize() const;
  /**
   * \brief Whether or not a Cell can burn
   * \param location Cell
   * \return Whether or not a Cell can burn
   */
  [[nodiscard]] bool canBurn(const XYIdx& location) const;
  /**
   * \brief Whether or not Location has burned already
   * \param location Location to check
   * \return Whether or not Location has burned already
   */
  [[nodiscard]] bool hasBurned(const XYIdx& location) const;
  /**
   * \brief Add an Event to the queue
   * \param event Event to add
   */
  void addEvent(Event&& event);
  /**
   * \brief Evaluate next Event in the queue
   * \return Whether to continue simulation
   */
  void evaluateNextEvent();
  /**
   * \brief End the simulation
   */
  void endSimulation() noexcept;
  /**
   * \brief Add a save point for simulation data at the given offset
   * \param offset Offset from start of simulation (days)
   */
  void addSaveByOffset(int offset);
  /**
   * \brief Add a save point for simulation data at given time
   * \param time Time to add save point at
   */
  void addSave(const DurationSize time);
  /**
   * \brief Tell Observers to save their data with base file name
   * \param output_directory Directory to save to
   * \param base_name Base file name
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveObservers(
    const string_view output_directory,
    const string_view base_name
  ) const;
  /**
   * \brief Tell Observers to save their data for the given time
   * \param output_directory Directory to save to
   * \param time Time to save data for
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveObservers(const string_view output_directory, DurationSize time) const;
  /**
   * \brief Save burn intensity information
   * \param output_directory Directory to save to
   * \param base_name Base file name
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveIntensity(
    const string_view output_directory,
    const string_view base_name
  ) const;
  /**
   * \brief Whether or not this Scenario has run already
   * \return Whether or not this Scenario has run already
   */
  [[nodiscard]] bool ran() const noexcept;
  ThresholdSize survivalProbability(const DurationSize time, const FuelCodeSize& in_fuel) const
  {
    return static_cast<ThresholdSize>(
      survival_probability_.at(in_fuel).at(time_index(time, weather_->minDate()))
    );
  }
  /**
   * \brief Whether or not the fire survives the conditions
   * \param time Time to use weather from
   * \param cell Cell to use
   * \param time_at_location How long the fire has been in that Cell
   * \return Whether or not the fire survives the conditions
   */
  [[nodiscard]] bool survives(
    const DurationSize time,
    const Cell& cell,
    const DurationSize time_at_location
  ) const
  {
    // HACK: resolve once and fail if not set already
    static const auto& settings = fs::settings::instance();
    if (settings.deterministic)
    {
      // always survive if deterministic
      return true;
    }
    try
    {
      const auto fire_wx = weather_;
      const auto wx = fire_wx->at(time);
      // use Mike's table
      // days at location     equivalent DMC
      //                *     54.40794725611335
      //                5     49.77788646253319
      //                4     45.15330890865478
      //                3     40.184467357005346
      //                2     35.025698388961054
      //                1     15.049926856936347
      const auto mc = wx.mcDmcPct();
      if (100 > mc || (109 >= mc && 5 > time_at_location) || (119 >= mc && 4 > time_at_location)
          || (131 >= mc && 3 > time_at_location) || (145 >= mc && 2 > time_at_location)
          || (218 >= mc && 1 > time_at_location))
      {
        return true;
      }
      // we can look by fuel type because the entire landscape shares the weather
      return extinctionThreshold(time) < survivalProbability(time, cell.fuelCode());
    }
    catch (const std::out_of_range&)
    {
      // no weather, so don't survive
      return false;
    }
  }
  /**
   * \brief List of what times the simulation will save
   * \return List of what times the simulation will save
   */
  [[nodiscard]] vector<DurationSize> savePoints() const;
  /**
   * \brief Save state of Scenario at given time
   * \param time
   */
  void saveStats(DurationSize time) const;
  /**
   * \brief Register an IObserver that will be notified when Cells burn
   * \param observer Observer to add to notification list
   */
  void registerObserver(IObserver* observer);
  /**
   * \brief Notify IObservers that a Cell has burned
   * \param event Event to notify IObservers of
   */
  void notify(const Event& event) const;
  /**
   * \brief Take whatever steps are necessary to process the given Event
   * \param event Event to process
   */
  void evaluate(const Event& event);
  /**
   * \brief Clear the Event list and all other data
   */
  void clear() noexcept;

protected:
  string log_prefix_{};

public:
  /**
   * \brief Constructor
   * \param model Model running this Scenario
   * \param id Identifier
   * \param weather Hourly weather stream to use
   * \param weather Weather stream to use for spread and extinction probability
   * \param start_time Start time for simulation
   * \param start_point StartPoint to use sunrise/sunset times from
   * \param start_day First day of simulation
   * \param last_date Last day of simulation
   */
  Scenario(
    Model* model,
    size_t id,
    const ptr<const FireWeather> weather,
    const ptr<const FireWeather> weather_daily,
    DurationSize start_time,
    const shared_ptr<Perimeter>& perimeter,
    const XYIdx& start_xy,
    StartPoint start_point,
    Day start_day,
    Day last_date
  );

protected:
  /**
   * \brief Observers to be notified when cells burn
   */
  list<unique_ptr<IObserver, IObserver_deleter>> observers_{};
  /**
   * \brief List of times to save simulation
   */
  vector<DurationSize> save_points_;
  /**
   * \brief Thresholds used to determine if extinction occurs
   */
  vector<ThresholdSize> extinction_thresholds_{};
  /**
   * \brief Thresholds used to determine if spread occurs
   */
  vector<ThresholdSize> spread_thresholds_by_ros_{};
  /**
   * \brief Current time for this Scenario
   */
  DurationSize current_time_;
  CellPointsMap points_;
  /**
   * \brief Contains information on cells that are not burnable
   */
  BurnedData unburnable_{};
  /**
   * \brief Event scheduler used for ordering events
   */
  set<Event> scheduler_;
  /**
   * \brief Map of what intensity each cell has burned at
   */
  unique_ptr<IntensityMap> intensity_{nullptr};
  /**
   * \brief Perimeter used to start Scenario from
   */
  shared_ptr<Perimeter> perimeter_{nullptr};
  /**
   * \brief Calculated SpreadInfo for SpreadKey for current time
   */
  SpreadCache spread_info_{};
  /**
   * \brief Map of when Cell had first Point arrive in it
   */
  map<XYIdx, DurationSize> arrival_{};
  /**
   * \brief Maximum rate of spread for current time
   */
  MathSize max_ros_;
  /**
   * \brief Cell that the Scenario starts from if no Perimeter
   */
  XYIdx start_xy_{};
  /**
   * \brief Hourly weather to use for this Scenario
   */
  ptr<const FireWeather> weather_{nullptr};
  /**
   * \brief Weather stream to use for spread and extinction probability
   */
  ptr<const FireWeather> weather_daily_{nullptr};
  /**
   * \brief Model this Scenario is being run in
   */
  Model* model_{nullptr};
  /**
   * \brief Map of ProbabilityMaps by time snapshot for them was taken
   */
  map<DurationSize, shared_ptr<ProbabilityMap>>* probabilities_{nullptr};
  /**
   * \brief Where to append the final size of this Scenario when run is complete
   */
  ptr<SafeVector> final_sizes_{nullptr};
  /**
   * \brief Origin of fire
   */
  StartPoint start_point_;
  /**
   * \brief Identifier
   */
  size_t id_;
  /**
   * \brief Start time (decimal days)
   */
  DurationSize start_time_;
  /**
   * \brief Which save point is the last one
   */
  DurationSize last_save_;
  /**
   * \brief Time index for current time
   */
  size_t current_time_index_ = numeric_limits<size_t>::max();
  /**
   * \brief Simulation number
   */
  int64_t simulation_;
  /**
   * \brief First day of simulation
   */
  Day start_day_;
  /**
   * \brief Last day of simulation
   */
  Day last_date_;
  /**
   * \brief Whether or not this Scenario has completed running
   */
  bool ran_;
  /**
   * \brief Whether this has been cancelled.
   */
  bool cancelled_ = false;
  /**
   * \brief How many times point spread event has happened
   */
  size_t step_;
  /**
   * \brief Point logging
   */
  LogPoints points_log_{};
  /**
   * \brief How many times this scenario tried to spread out of bounds
   */
  size_t oob_spread_{};
  /**
   * \brief Probability of survival for fuels fuel at each time
   */
  SurvivalMap survival_probability_;
};
}
#endif
