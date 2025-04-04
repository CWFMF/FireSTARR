/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <condition_variable>
#include <cstdio>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "Environment.h"
#include "Iteration.h"
#include "FireWeather.h"
namespace tbd
{
namespace topo
{
class StartPoint;
}
namespace sim
{
class Event;
class Scenario;
/**
 * \brief Provides the ability to limit number of threads running at once.
 */
class Semaphore
{
public:
  /**
   * \brief Create a Semaphore that limits number of concurrent things running
   * \param n Number of concurrent things running
   */
  explicit Semaphore(const int n)
    : used_{0},
      limit_{n}
  {
  }
  Semaphore(const Semaphore& rhs) = delete;
  Semaphore(Semaphore&& rhs) = delete;
  Semaphore& operator=(const Semaphore& rhs) = delete;
  Semaphore& operator=(Semaphore&& rhs) = delete;
  void set_limit(size_t limit)
  {
    logging::debug("Changing Semaphore limit from %d to %d", limit_, limit);
    // NOTE: won't drop threads if set lower but won't give out more until below limit
    limit_ = limit;
  }
  size_t limit()
  {
    return limit_;
  }
  /**
   * \brief Notify something that's waiting so it can run
   */
  void notify()
  {
    std::unique_lock<std::mutex> l(mutex_);
    --used_;
    cv_.notify_one();
  }
  /**
   * \brief Wait until allowed to run
   */
  void wait()
  {
    std::unique_lock<std::mutex> l(mutex_);
    cv_.wait(l, [this] { return used_ <= limit_; });
    ++used_;
  }
private:
  /**
   * \brief Mutex for parallel access
   */
  std::mutex mutex_;
  /**
   * \brief Condition variable to use for checking count
   */
  std::condition_variable cv_;
  /**
   * \brief Variable to keep count of threads in use
   */
  int used_;
  /**
   * \brief Limit for number of threads
   */
  int limit_;
};
/**
 * \brief Indicates a section of code that is limited to a certain number of threads running at once.
 */
class CriticalSection
{
  /**
   * \brief Semaphore that this keeps track of access for
   */
  Semaphore& s_;
public:
  /**
   * \brief Constructor
   * \param ss Semaphore to wait on
   */
  explicit CriticalSection(Semaphore& ss)
    : s_{ss}
  {
    s_.wait();
  }
  CriticalSection(const CriticalSection& rhs) = delete;
  CriticalSection(CriticalSection&& rhs) = delete;
  CriticalSection& operator=(const CriticalSection& rhs) = delete;
  CriticalSection& operator=(CriticalSection&& rhs) = delete;
  ~CriticalSection() noexcept
  {
    try
    {
      s_.notify();
    }
    catch (const std::exception& ex)
    {
      logging::fatal(ex);
      std::terminate();
    }
  }
};
/**
 * \brief Contains all the immutable information regarding a simulation that is common between Scenarios.
 */
class Model
{
public:
  /**
   * \brief Run Scenarios initialized from given inputs
   * \param dir_out Folder to save outputs to
   * \param weather_input Name of file to read weather from
   * \param yesterday FwiWeather yesterday used for startup indices
   * \param raster_root Directory to read raster inputs from
   * \param start_point StartPoint to use for sunrise/sunset
   * \param start_time Start time for simulation
   * \param perimeter Perimeter to initialize fire from, if there is one
   * \param size Size to start fire at if no Perimeter
   * \return
   */
  [[nodiscard]] static int runScenarios(const string dir_out,
                                        const char* weather_input,
                                        const wx::FwiWeather& yesterday,
                                        const char* raster_root,
                                        const topo::StartPoint& start_point,
                                        const tm& start_time,
                                        const string& perimeter,
                                        size_t size);
  /**
   * \brief Cell at the given row and column
   * \param row Row
   * \param column Column
   * \return Cell at the given row and column
   */
  [[nodiscard]]
#ifdef NDEBUG
  constexpr
#endif
    topo::Cell
    cell(const Idx row, const Idx column) const
  {
    return env_->cell(row, column);
  }
  /**
   * \brief Cell at the given Location
   * \param location Location to get Cell for
   * \return Cell at the given Location
   */
  template <class P>
  [[nodiscard]] constexpr topo::Cell cell(const Position<P>& position) const
  {
    return env_->cell(position);
  }
  /**
   * \brief Cell at the Location represented by the given hash
   * \param hash_size Hash size for Location to get Cell for
   * \return Cell at the Location represented by the given hash
   */
  //  [[nodiscard]] constexpr topo::Cell cell(const HashSize hash_size) const
  //  {
  //    return env_->cell(hash_size);
  //  }
  /**
   * \brief Number of rows in extent
   * \return Number of rows in extent
   */
  [[nodiscard]] constexpr Idx rows() const
  {
    return env_->rows();
  }
  /**
   * \brief Number of columns in extent
   * \return Number of columns in extent
   */
  [[nodiscard]] constexpr Idx columns() const
  {
    return env_->columns();
  }
  /**
   * \brief Cell width and height (m)
   * \return Cell width and height (m)
   */
  [[nodiscard]] constexpr MathSize cellSize() const
  {
    return env_->cellSize();
  }
  /**
   * \brief Environment simulation is occurring in
   * \return Environment simulation is occurring in
   */
  [[nodiscard]] constexpr const topo::Environment& environment() const
  {
    return *env_;
  }
  /**
   * \brief Time that execution started
   * \return Time that execution started
   */
  [[nodiscard]] constexpr Clock::time_point runningSince() const
  {
    return running_since_;
  }
  /**
   * \brief Maximum amount of time simulation can run for before being stopped
   * \return Maximum amount of time simulation can run for  before being stopped
   */
  [[nodiscard]] constexpr Clock::duration timeLimit() const
  {
    return time_limit_;
  }
  /**
   * \brief Whether or not simulation has exceeded any limits that mean it should stop
   * \return Whether or not simulation has exceeded any limits that mean it should stop
   */
  [[nodiscard]] bool shouldStop() const noexcept;
  /**
   * \brief Whether or not simulation has been running longer than maximum duration
   * \return Whether or not simulation has been running longer than maximum duration
   */
  [[nodiscard]] bool isOutOfTime() const noexcept;
  /**
   * \brief Whether or not simulation is over max simulation count
   * \return Whether or not simulation is over max simulation count
   */
  [[nodiscard]] bool isOverSimulationCountLimit() const noexcept;
  /**
   * \brief What year the weather is for
   * \return What year the weather is for
   */
  [[nodiscard]] int year() const noexcept
  {
    return year_;
  }
  /**
   * \brief How many ignition scenarios are being used
   * \return How many ignition scenarios are being used
   */
  [[nodiscard]] int ignitionScenarios() const noexcept
  {
    return starts_.size();
  }
  /**
   * \brief How many Scenarios are in each Iteration
   * \return How many Scenarios are in each Iteration
   */
  [[nodiscard]] int scenarioCount() const noexcept
  {
    return wx_.size() * ignitionScenarios();
  }
  /**
   * \brief Difference between date and the date of minimum foliar moisture content
   * \param time Date to get value for
   * \return Difference between date and the date of minimum foliar moisture content
   */
  [[nodiscard]] constexpr int nd(const DurationSize time) const
  {
    return nd_.at(static_cast<Day>(time));
  }
  [[nodiscard]] const char* outputDirectory() const
  {
    return dir_out_.c_str();
  }
  /**
   * \brief Duration that model has run for
   * \return std::chrono::seconds  Duration model has been running for
   */
  [[nodiscard]] std::chrono::seconds runTime() const;
  /**
   * \brief Create a ProbabilityMap with the same extent as this
   * \param time Time in simulation this ProbabilityMap represents
   * \param start_time Start time of simulation
   * \param min_value Lower bound of 'low' intensity range
   * \param low_max Upper bound of 'low' intensity range
   * \param med_max Upper bound of 'moderate' intensity range
   * \param max_value Upper bound of 'high' intensity range
   * \return ProbabilityMap with the same extent as this
   */
  [[nodiscard]] ProbabilityMap* makeProbabilityMap(DurationSize time,
                                                   DurationSize start_time,
                                                   int min_value,
                                                   int low_max,
                                                   int med_max,
                                                   int max_value) const;
  ~Model() = default;
  /**
   * \brief Constructor
   * \param start_point StartPoint to use for sunrise/sunset times
   * \param env Environment to run simulations in
   */
  Model(const string dir_out,
        const topo::StartPoint& start_point,
        topo::Environment* env);
  Model(Model&& rhs) noexcept = delete;
  Model(const Model& rhs) = delete;
  Model& operator=(Model&& rhs) noexcept = delete;
  Model& operator=(const Model& rhs) = delete;
  /**
   * \brief Set constant weather
   * \param weather FwiWeather to use as constant weather
   */
  void setWeather(const wx::FwiWeather& weather, const Day start_day);
  /**
   * \brief Read weather used for Scenarios
   * \param yesterday FwiWeather for yesterday
   * \param latitude Latitude to calculate for
   * \param filename Weather file to read
   */
  void readWeather(const wx::FwiWeather& yesterday,
                   const MathSize latitude,
                   const string& filename);
  /**
   * \brief Make starts based on desired point and where nearest combustible cells are
   * \param coordinates Coordinates in the Environment to try starting at
   * \param point Point Coordinates represent
   * \param perim Perimeter to start from, if there is one
   * \param size Size of fire to create if no input Perimeter
   */
  void makeStarts(Coordinates coordinates,
                  const topo::Point& point,
                  string perim,
                  size_t size);
  /**
   * \brief Create an Iteration by initializing Scenarios
   * \param start_point StartPoint to use for sunrise/sunset
   * \param start Start time for simulation
   * \param start_day Start date for simulation
   * \param last_date End date for simulation
   * \return Iteration containing initialized Scenarios
   */
  [[nodiscard]] Iteration readScenarios(const topo::StartPoint& start_point,
                                        DurationSize start,
                                        Day start_day,
                                        Day last_date);
  /**
   * \brief Acquire a BurnedData that has already burnt cells set
   * \return A BurnedData that has already burnt cells set
   */
  [[nodiscard]] BurnedData* getBurnedVector() const noexcept;
  /**
   * \brief Return a BurnedData so it can be used in the future
   * \param has_burned BurnedData to return to pool
   */
  void releaseBurnedVector(BurnedData* has_burned) const noexcept;
  /**
   * \brief Semaphore used to limit how many things run at once
   */
  static Semaphore task_limiter;
  /**
   * Conditions for yesterday (or constant weather)
   */
  const wx::FwiWeather* yesterday() const noexcept
  {
    return &yesterday_;
  }
private:
  const string dir_out_;
  /**
   * \brief Add statistics for completed iterations
   * \param all_sizes All sizes that have simulations have produced
   * \param means Mean sizes per iteration
   * \param pct 95th percentile sizes per iteration
   * \param cur_sizes Sizes to add to statistics
   */
  [[nodiscard]] bool add_statistics(vector<MathSize>* all_sizes,
                                    vector<MathSize>* means,
                                    vector<MathSize>* pct,
                                    const util::SafeVector& sizes);
  /**
   * \brief Mutex for parallel access
   */
  mutable mutex vector_mutex_;
  /**
   * \brief Start time of simulation
   */
  tm start_time_;
  /**
   * \brief Pool of BurnedData that can be reused
   */
  mutable vector<unique_ptr<BurnedData>> vectors_{};
  /**
   * \brief Run Iterations until confidence is reached
   * \param start_point StartPoint to use for sunrise/sunset
   * \param start Start time for simulation
   * \param start_day Start day for simulation
   * \return Map of times to ProbabilityMap for that time
   */
  map<DurationSize, ProbabilityMap*> runIterations(const topo::StartPoint& start_point,
                                                   DurationSize start,
                                                   Day start_day);
  /**
   * \brief Find all Cell(s) that can burn in entire Environment
   */
  void findAllStarts();
  /**
   * Save probability rasters
   */
  DurationSize saveProbabilities(map<DurationSize, ProbabilityMap*>& probabilities, const Day start_day, const bool is_interim);
  /**
   * \brief Find Cell(s) that can burn closest to Location
   * \param location Location to look for start Cells
   */
  void findStarts(Location location);
  /**
   * \brief Differences between date and the date of minimum foliar moisture content
   */
  array<int, MAX_DAYS> nd_{};
  /**
   * \brief Map of scenario number to weather stream
   */
  map<size_t, shared_ptr<wx::FireWeather>> wx_{};
  /**
   * \brief Map of scenario number to weather stream
   */
  map<size_t, shared_ptr<wx::FireWeather>> wx_daily_{};
  /**
   * \brief Cell(s) that can burn closest to start Location
   */
  vector<shared_ptr<topo::Cell>> starts_{};
  /**
   * \brief Time to use for simulation start
   */
  Clock::time_point running_since_;
  /**
   * \brief Maximum amount of time simulation can run for before being stopped
   */
  Clock::duration time_limit_;
  // /**
  //  * @brief Initial intensity map based off perimeter
  //  */
  // shared_ptr<IntensityMap> initial_intensity_ = nullptr;
  /**
   * \brief Perimeter to use for initializing simulations
   */
  shared_ptr<topo::Perimeter> perimeter_ = nullptr;
  /**
   * \brief Environment to use for Model
   */
  topo::Environment* env_;
#ifdef DEBUG_WEATHER
  /**
   * \brief Write weather that was loaded to an output file
   */
  void outputWeather();
  /**
   * \brief Write weather that was loaded to an output file
   * \param weather Weather to write
   * \param file_name Name of file to write to
   */
  void outputWeather(
    map<size_t, shared_ptr<wx::FireWeather>>& weather,
    const char* file_name);
#endif
  /**
   * \brief What year the weather is for
   */
  int year_;
  /**
   * \brief If simulation is out of time and should stop
   */
  bool is_out_of_time_ = false;
  /**
   * \brief If simulation is over max simulation count
   */
  bool is_over_simulation_count_ = false;
  /**
   * Conditions for yesterday (or constant weather)
   */
  wx::FwiWeather yesterday_;
  // /**
  //  * @brief Time when we last checked if simulation should end
  //  *
  //  */
  std::chrono::steady_clock::time_point last_checked_;
  /**
   * \brief Latitude to use for any calcualtions
   */
  MathSize latitude_;
  /**
   * \brief Longitude to use for any calcualtions
   */
  MathSize longitude_;
};
}
}
