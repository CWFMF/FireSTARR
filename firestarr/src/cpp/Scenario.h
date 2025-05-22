/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "EventCompare.h"
#include "FireWeather.h"
#include "IntensityMap.h"
#include "Model.h"
#include "Settings.h"
#include "StartPoint.h"
#include "InnerPos.h"
#include "FireSpread.h"
#include "CellPoints.h"

namespace fs::sim
{
class LogPoints;
class IObserver;
class Event;
using topo::Location;
using topo::Position;
using PointSet = vector<InnerPos>;
/**
 * \brief A single Scenario in an Iteration using a specific FireWeather stream.
 */
class Scenario
  : public logging::SelfLogger
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
  /**
   * \brief Weighted Danger Severity Rating
   * \return Weighted Danger Severity Rating
   */
  [[nodiscard]] constexpr size_t weightedDsr() const noexcept
  {
    return weather_->weightedDsr();
  }
  virtual ~Scenario();
  /**
   * \brief Constructor
   * \param model Model running this Scenario
   * \param id Identifier
   * \param weather Hourly weather stream to use
   * \param weather Weather stream to use for spread and extinction probability
   * \param start_time Start time for simulation
  //  * \param initial_intensity Intensity grid to start from
   * \param perimeter Perimeter to initialize with
   * \param start_point StartPoint to use sunrise/sunset times from
   * \param start_day First day of simulation
   * \param last_date Last day of simulation
   */
  Scenario(Model* model,
           size_t id,
           wx::FireWeather* weather,
           wx::FireWeather* weather_daily,
           DurationSize start_time,
           //  const shared_ptr<IntensityMap>& initial_intensity,
           const shared_ptr<topo::Perimeter>& perimeter,
           const topo::StartPoint& start_point,
           Day start_day,
           Day last_date);
  /**
   * \brief Constructor
   * \param model Model running this Scenario
   * \param id Identifier
   * \param weather Hourly weather stream to use
   * \param weather Weather stream to use for spread and extinction probability
   * \param start_time Start time for simulation
   * \param start_cell Cell to start ignition in
   * \param start_point StartPoint to use sunrise/sunset times from
   * \param start_day First day of simulation
   * \param last_date Last day of simulation
   */
  Scenario(Model* model,
           size_t id,
           wx::FireWeather* weather,
           wx::FireWeather* weather_daily,
           DurationSize start_time,
           const shared_ptr<topo::Cell>& start_cell,
           const topo::StartPoint& start_point,
           Day start_day,
           Day last_date);
  /**
   * \brief Move constructor
   * \param rhs Scenario to move from
   */
  Scenario(Scenario&& rhs) noexcept;
  Scenario(const Scenario& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs Scenario to move from
   * \return This, after assignment
   */
  Scenario& operator=(Scenario&& rhs) noexcept;
  Scenario& operator=(const Scenario& rhs) const = delete;
  // HACK: use for surface right now
  /**
   * \brief Assign start Cell, reset thresholds and set SafeVector to output results to
   * \param start_cell Cell to start ignition in
   * \param final_sizes SafeVector to output results to
   * \return This
   */
  [[nodiscard]] Scenario* reset_with_new_start(const shared_ptr<topo::Cell>& start_cell,
                                               util::SafeVector* final_sizes);
  /**
   * \brief Reset thresholds and set SafeVector to output results to
   * \param mt_extinction Used for extinction random numbers
   * \param mt_spread Used for spread random numbers
   * \param final_sizes SafeVector to output results to
   * \return This
   */
  [[nodiscard]] Scenario* reset(mt19937* mt_extinction,
                                mt19937* mt_spread,
                                util::SafeVector* final_sizes);
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
  /**
   * \brief Get Cell for given row and column
   * \param row Row
   * \param column Column
   * \return Cell for given row and column
   */
  [[nodiscard]]
#ifdef NDEBUG
  CONSTEXPR
#endif
    topo::Cell
    cell(const Idx row, const Idx column) const
  {
    return model_->cell(row, column);
  }
  /**
   * \brief Get Cell for given Location
   * \param location Location
   * \return Cell for given Location
   */
  template <class P>
  [[nodiscard]] constexpr topo::Cell cell(const Position<P>& position) const
  {
    return model_->cell(position);
  }
  /**
   * \brief Number of rows
   * \return Number of rows
   */
  [[nodiscard]] constexpr Idx rows() const
  {
    return model_->rows();
  }
  /**
   * \brief Number of columns
   * \return Number of columns
   */
  [[nodiscard]] constexpr Idx columns() const
  {
    return model_->columns();
  }
  /**
   * \brief Cell width and height (m)
   * \return Cell width and height (m)
   */
  [[nodiscard]] constexpr MathSize cellSize() const
  {
    return model_->cellSize();
  }
  /**
   * \brief Simulation number
   * \return Simulation number
   */
  [[nodiscard]] constexpr int64_t simulation() const
  {
    return simulation_;
  }
  /**
   * \brief StartPoint that provides sunrise/sunset times
   * \return StartPoint
   */
  [[nodiscard]] constexpr const topo::StartPoint& startPoint() const
  {
    return start_point_;
  }
  /**
   * \brief Simulation start time
   * \return Simulation start time
   */
  [[nodiscard]] constexpr DurationSize startTime() const
  {
    return start_time_;
  }
  /**
   * \brief Identifier
   * \return Identifier
   */
  [[nodiscard]] constexpr size_t id() const
  {
    return id_;
  }
  /**
   * \brief Model this Scenario is running in
   * \return Model this Scenario is running in
   */
  [[nodiscard]] constexpr const Model& model() const
  {
    return *model_;
  }
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
  [[nodiscard]] const wx::FwiWeather* weather(const DurationSize time) const
  {
    return weather_->at(time);
  }
  [[nodiscard]] const wx::FwiWeather* weather_daily(const DurationSize time) const
  {
    return weather_daily_->at(time);
  }
  /**
   * \brief Difference between date and the date of minimum foliar moisture content
   * \param time Time to get value for
   * \return Difference between date and the date of minimum foliar moisture content
   */
  [[nodiscard]] constexpr int nd(const DurationSize time) const
  {
    return model().nd(time);
  }
  /**
   * \brief Get extinction threshold for given time
   * \param time Time to get value for
   * \return Extinction threshold for given time
   */
  [[nodiscard]] ThresholdSize extinctionThreshold(const DurationSize time) const
  {
    return extinction_thresholds_.at(util::time_index(time - start_day_));
  }
  /**
   * \brief Get spread threshold for given time
   * \param time Time to get value for
   * \return Spread threshold for given time
   */
  [[nodiscard]] ThresholdSize spreadThresholdByRos(const DurationSize time) const
  {
    return spread_thresholds_by_ros_.at(util::time_index(time - start_day_));
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
    return isAtNight(time) ? Settings::minimumFfmcAtNight() : Settings::minimumFfmc();
  }
  /**
   * \brief Whether or not the given Location is surrounded by cells that are burnt
   * \param location Location to check if is surrounded
   * \return Whether or not the given Location is surrounded by cells that are burnt
   */
  [[nodiscard]] bool isSurrounded(const Location& location) const;
  template <class P>
  [[nodiscard]] bool isSurrounded(const Position<P>& position) const
  {
    return isSurrounded(Location{position.hash()});
  }
  /**
   * \brief Cell that InnerPos falls within
   * \param p InnerPos
   * \return Cell that InnerPos falls within
   */
  [[nodiscard]] topo::Cell cell(const InnerPos& p) const noexcept;
  /**
   * \brief Run the Scenario
   * \param probabilities map to update ProbabilityMap for times base on Scenario results
   * \return This
   */
  Scenario* run(map<DurationSize, ProbabilityMap*>* probabilities);
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
  [[nodiscard]] bool canBurn(const topo::Cell& location) const;
  /**
   * \brief Whether or not Cell with the given hash can burn
   * \param hash Hash for Cell to check
   * \return Whether or not Cell with the given hash can burn
   */
  //  [[nodiscard]] bool canBurn(HashSize hash) const;
  /**
   * \brief Whether or not Location has burned already
   * \param location Location to check
   * \return Whether or not Location has burned already
   */
  [[nodiscard]] bool hasBurned(const Location& location) const;
  template <class P>
  [[nodiscard]] bool hasBurned(const Position<P>& position) const
  {
    return hasBurned(Location{position.hash()});
  }
  /**
   * \brief Whether or not Location with given hash has burned already
   * \param hash Hash of Location to check
   * \return Whether or not Location with given hash has burned already
   */
  //  [[nodiscard]] bool hasBurned(HashSize hash) const;
  /**
   * \brief Add an Event to the queue
   * \param event Event to add
   */
  void addEvent(Event&& event);
  /**
   * \brief Evaluate next Event in the queue
  //  * \return Whether to continue simulation
   */
  // bool evaluateNextEvent();
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
   * \tparam V Type to use for time
   * \param time Time to add save point at
   */
  template <class V>
  void addSave(V time);
  /**
   * \brief Whether or not this Scenario has run already
   * \return Whether or not this Scenario has run already
   */
  [[nodiscard]] bool ran() const noexcept;
  /**
   * \brief Whether or not the fire survives the conditions
   * \param time Time to use weather from
   * \param cell Cell to use
   * \param time_at_location How long the fire has been in that Cell
   * \return Whether or not the fire survives the conditions
   */
  [[nodiscard]] bool survives(const DurationSize time,
                              const topo::Cell& cell,
                              const DurationSize time_at_location) const
  {
    try
    {
      const auto fire_wx = weather_;
      // // NOTE: Does using daily makes sense if we're looking at moisture?
      // // HACK: use daily with diurnal curves to be consistent with pre-hourly wx version
      // const auto fire_wx = weather_daily_;
      const auto wx = fire_wx->at(time);
      // use Mike's table
      const auto mc = wx->mcDmcPct();
      if (100 > mc
          || (109 >= mc && 5 > time_at_location)
          || (119 >= mc && 4 > time_at_location)
          || (131 >= mc && 3 > time_at_location)
          || (145 >= mc && 2 > time_at_location)
          || (218 >= mc && 1 > time_at_location))
      {
        return true;
      }
      // we can look by fuel type because the entire landscape shares the weather
      return extinctionThreshold(time) < fire_wx->survivalProbability(
               time,
               cell.fuelCode());
    }
    catch (const std::out_of_range& e)
    {
      // FIX: just ignore for now
      // std::cerr << e.what() << '\n';
      // logging::warning("Survival is checking for weather that doesn't exist at %f", time);
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
   * \brief Take whatever steps are necessary to process the given Event
   * \param event Event to process
   */
  void evaluate(const Event& event);
  /**
   * \brief Clear the Event list and all other data
   */
  void clear() noexcept;
protected:
  string add_log(const char* format) const noexcept override;
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
  Scenario(Model* model,
           size_t id,
           wx::FireWeather* weather,
           wx::FireWeather* weather_daily,
           DurationSize start_time,
           //  const shared_ptr<IntensityMap>& initial_intensity,
           const shared_ptr<topo::Perimeter>& perimeter,
           const shared_ptr<topo::Cell>& start_cell,
           topo::StartPoint start_point,
           Day start_day,
           Day last_date);
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
  /**
   * \brief Map of Cells to the PointSets within them
   */
  CellPointsMap points_;
  /**
   * \brief Contains information on cells that are not burnable
   */
  BurnedData* unburnable_;
  /**
   * \brief Event scheduler used for ordering events
   */
  set<Event, EventCompare> scheduler_;
  /**
   * \brief Map of what intensity each cell has burned at
   */
  unique_ptr<IntensityMap> intensity_;
  // /**
  //  * @brief Initial intensity map based off perimeter
  //  */
  // shared_ptr<IntensityMap> initial_intensity_;
  /**
   * \brief Perimeter used to start Scenario from
   */
  shared_ptr<topo::Perimeter> perimeter_;
  /**
   * \brief Calculated SpreadInfo for SpreadKey for current time
   */
  map<topo::SpreadKey, SpreadInfo> spread_info_{};
  /**
   * \brief Map of when Cell had first Point arrive in it
   */
  map<topo::Cell, DurationSize> arrival_{};
  /**
   * \brief Maximum rate of spread for current time
   */
  MathSize max_ros_;
  /**
   * \brief Cell that the Scenario starts from if no Perimeter
   */
  shared_ptr<topo::Cell> start_cell_;
  /**
   * \brief Hourly weather to use for this Scenario
   */
  wx::FireWeather* weather_;
  /**
   * \brief Weather stream to use for spread and extinction probability
   */
  wx::FireWeather* weather_daily_;
  /**
   * \brief Model this Scenario is being run in
   */
  Model* model_;
  /**
   * \brief Map of ProbabilityMaps by time snapshot for them was taken
   */
  map<DurationSize, ProbabilityMap*>* probabilities_;
  /**
   * \brief Where to append the final size of this Scenario when run is complete
   */
  util::SafeVector* final_sizes_;
  /**
   * \brief Origin of fire
   */
  topo::StartPoint start_point_;
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
  shared_ptr<LogPoints> log_points_;
  /**
   * \brief How many times point spread event has happened
   */
  size_t step_;
  /**
   * \brief How many times this scenario tried to spread out of bounds
   */
  size_t oob_spread_;
};
}
