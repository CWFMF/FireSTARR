/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_MODEL_H
#define FS_MODEL_H
#include "stdafx.h"
#include "Environment.h"
#include "FireWeather.h"
#include "Iteration.h"
#include "Perimeter.h"
#include "Settings.h"
#include "unstable.h"
namespace fs
{
using settings::Settings;
class StartPoint;
struct Event;
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
  explicit Semaphore(const int n) : used_{0}, limit_{n} { }
  Semaphore(const Semaphore& rhs) = delete;
  Semaphore(Semaphore&& rhs) = delete;
  Semaphore& operator=(const Semaphore& rhs) = delete;
  Semaphore& operator=(Semaphore&& rhs) = delete;
  void set_limit(int limit)
  {
    logging::debug("Changing Semaphore limit from {:d} to {:d}", limit_, limit);
    // NOTE: won't drop threads if set lower but won't give out more until below limit
    limit_ = limit;
  }
  size_t limit() { return static_cast<size_t>(limit_); }
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
 * \brief Indicates a section of code that is limited to a certain number of threads running at
 * once.
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
  explicit CriticalSection(Semaphore& ss) : s_{ss} { s_.wait(); }
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
      exit(logging::fatal(ex));
    }
  }
};
/**
 * \brief Contains all the immutable information regarding a simulation that is common between
 * Scenarios.
 */
class Model
{
public:
  /**
   * \brief Run Scenarios initialized from given inputs
   * \param output_directory Folder to save outputs to
   * \param weather_input Name of file to read weather from
   * \param yesterday FwiWeather yesterday used for startup indices
   * \param raster_root Directory to read raster inputs from
   * \param start_point StartPoint to use for sunrise/sunset
   * \param start_time Start time for simulation
   * \param perimeter Perimeter to initialize fire from, if there is one
   * \param size Size to start fire at if no Perimeter
   * \return
   */
  [[nodiscard]] static int runScenarios(
    const string_view output_directory,
    const LazyPath& weather_input,
    const FwiWeather& yesterday,
    const char* raster_root,
    const StartPoint& start_point,
    const tm& start_time,
    const LazyPath& perimeter,
    const size_t size
  );
  [[nodiscard]]
  Cell cell(const XYIdx xy) const
  {
    return env_->cell(xy);
  }
  [[nodiscard]] constexpr Idx height() const { return env_->height(); }
  [[nodiscard]] constexpr Idx width() const { return env_->width(); }
  /**
   * \brief Cell width and height (m)
   * \return Cell width and height (m)
   */
  [[nodiscard]] constexpr MathSize cellSize() const { return env_->cellSize(); }
  /**
   * \brief Environment simulation is occurring in
   * \return Environment simulation is occurring in
   */
  [[nodiscard]] constexpr const Environment& environment() const { return *env_; }
  /**
   * \brief Time that execution started
   * \return Time that execution started
   */
  [[nodiscard]] constexpr Clock::time_point runningSince() const { return running_since_; }
  /**
   * \brief Maximum amount of time simulation can run for before being stopped
   * \return Maximum amount of time simulation can run for before being stopped
   */
  [[nodiscard]] constexpr Clock::duration timeLimit() const { return time_limit_; }
  /**
   * \brief Time between generating interim outputs (s)
   * \return Time between generating interim outputs (s)
   */
  [[nodiscard]] constexpr Clock::duration interimTimeLimit() const
  {
    return interim_save_interval_;
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
   * \brief Whether or not simulation is under min simulation count
   * \return Whether or not simulation is under min simulation count
   */
  [[nodiscard]] bool isUnderSimulationCountMinimum() const noexcept;
  /**
   * \brief How many active simulations are still needed
   * \return How many active simulations are still needed
   */
  [[nodiscard]] size_t activeSimulationsStillRequired() const noexcept;
  /**
   * \brief Whether or not simulation is over max simulation count
   * \return Whether or not simulation is over max simulation count
   */
  [[nodiscard]] bool isOverSimulationCountMaximum() const noexcept;
  /**
   * \brief What year the weather is for
   * \return What year the weather is for
   */
  [[nodiscard]] int year() const noexcept { return year_; }
  /**
   * \brief How many ignition scenarios are being used
   * \return How many ignition scenarios are being used
   */
  [[nodiscard]] size_t ignitionScenarios() const noexcept { return starts_.size(); }
  /**
   * \brief How many Scenarios are in each Iteration
   * \return How many Scenarios are in each Iteration
   */
  [[nodiscard]] size_t scenarioCount() const noexcept { return wx_.size() * ignitionScenarios(); }
  /**
   * \brief Difference between date and the date of minimum foliar moisture content
   * \param time Date to get value for
   * \return Difference between date and the date of minimum foliar moisture content
   */
  [[nodiscard]] constexpr int nd(const DurationSize time) const
  {
    return nd_.at(static_cast<Day>(time));
  }
  [[nodiscard]] const char* outputDirectory() const { return output_directory_.c_str(); }
  /**
   * \brief Duration that model has run for
   * \return std::chrono::seconds  Duration model has been running for
   */
  [[nodiscard]] std::chrono::seconds runTime() const;
  [[nodiscard]] std::chrono::seconds timeSinceLastSave() const;
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
  [[nodiscard]] shared_ptr<ProbabilityMap> makeProbabilityMap(
    DurationSize time,
    DurationSize start_time,
    int min_value,
    int low_max,
    int med_max,
    int max_value
  ) const;
  ~Model() = default;
  /**
   * \brief Constructor
   * \param start_point StartPoint to use for sunrise/sunset times
   * \param env Environment to run simulations in
   */
  Model(
    const tm& start_time,
    const string_view output_directory,
    const StartPoint& start_point,
    Environment* env
  );
  Model(
    const tm& start_time,
    const string_view output_directory,
    const StartPoint& start_point,
    Environment* env,
    const Settings& settings
  );
  Model(Model&& rhs) noexcept = delete;
  Model(const Model& rhs) = delete;
  Model& operator=(Model&& rhs) noexcept = delete;
  Model& operator=(const Model& rhs) = delete;
  /**
   * \brief Set constant weather
   * \param weather FwiWeather to use as constant weather
   */
  void setWeather(const FwiWeather& weather, const Day start_day);
  /**
   * \brief Read weather used for Scenarios
   * \param yesterday FwiWeather for yesterday
   * \param latitude Latitude to calculate for
   * \param filename Weather file to read
   */
  void readWeather(const FwiWeather& yesterday, const MathSize latitude, const string& filename);
  /**
   * \brief Make starts based on desired point and where nearest combustible cells are
   * \param coordinates Coordinates in the Environment to try starting at
   * \param point Point Coordinates represent
   * \param perim Perimeter to start from, if there is one
   * \param size Size of fire to create if no input Perimeter
   */
  void makeStarts(Coordinates coordinates, const Point& point, const LazyPath& perim, size_t size);
  /**
   * \brief Create an Iteration by initializing Scenarios
   * \param start_point StartPoint to use for sunrise/sunset
   * \param start Start time for simulation
   * \param start_day Start date for simulation
   * \param last_date End date for simulation
   * \return Iteration containing initialized Scenarios
   */
  [[nodiscard]] Iteration readScenarios(
    const StartPoint& start_point,
    DurationSize start,
    Day start_day,
    Day last_date
  );
  /**
   * \brief Semaphore used to limit how many things run at once
   */
  static Semaphore task_limiter;
  /**
   * Conditions for yesterday (or constant weather)
   */
  ptr<const FwiWeather> yesterday() const noexcept { return &yesterday_; }
  /**
   * \brief Initial fire size at start of scenario
   */
  MathSize initial_size() const
  {
    // if more than one cell at start then we'd have a perimeter
    const auto num_cells = (nullptr == perimeter_) ? 1 : perimeter_->burned.size();
    return env_->to_hectares(num_cells);
  };

private:
  const string output_directory_;
  /**
   * \brief Add statistics for completed iterations
   * \param all_sizes All sizes that have simulations have produced
   * \param means Mean sizes per iteration
   * \param pct 95th percentile sizes per iteration
   * \param cur_sizes Sizes to add to statistics
   */
  [[nodiscard]] bool add_statistics(
    vector<MathSize>* all_sizes,
    vector<MathSize>* means,
    vector<MathSize>* pct,
    const SafeVector& sizes
  );
  /**
   * \brief Start time of simulation
   */
  tm start_time_{};
  /**
   * \brief Run Iterations until confidence is reached
   * \param start_point StartPoint to use for sunrise/sunset
   * \param start Start time for simulation
   * \param start_day Start day for simulation
   * \return Map of times to ProbabilityMap for that time
   */
  map<DurationSize, shared_ptr<ProbabilityMap>> runIterations(
    const StartPoint& start_point,
    DurationSize start,
    Day start_day
  );
  /**
   * \brief Find all Cell(s) that can burn in entire Environment
   */
  void findAllStarts();
  /**
   * Save probability rasters
   */
  DurationSize saveProbabilities(
    map<DurationSize, shared_ptr<ProbabilityMap>>& probabilities,
    const Day start_day,
    const bool is_interim
  );
  /**
   * \brief Find Cell(s) that can burn closest to Location
   * \param location Location to look for start Cells
   */
  void findStarts(const XYIdx& location);
  /**
   * \brief Differences between date and the date of minimum foliar moisture content
   */
  array<int, MAX_DAYS> nd_{};
  /**
   * \brief Map of scenario number to weather stream
   */
  map<size_t, FireWeather> wx_{};
  /**
   * \brief Map of scenario number to weather stream
   */
  map<size_t, FireWeather> wx_daily_{};
  /**
   * \brief Cell(s) that can burn closest to start Location
   */
  vector<XYIdx> starts_{};
  /**
   * \brief Time to use for simulation start
   */
  Clock::time_point running_since_;
  /**
   * \brief Maximum amount of time simulation can run for before being stopped
   */
  Clock::duration time_limit_;
  /**
   * \brief Time of last interim save
   */
  Clock::time_point no_interim_save_since_;
  /**
   * \briefTime between generating interim outputs
   */
  Clock::duration interim_save_interval_;
  /**
   * \brief Perimeter to use for initializing simulations
   */
  shared_ptr<Perimeter> perimeter_ = nullptr;
  /**
   * \brief Environment to use for Model
   */
  Environment* env_{nullptr};
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
  void outputWeather(map<size_t, FireWeather>& weather, const char* file_name);
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
   * \brief If simulation is past the time interval between interim outputs
   */
  bool should_output_interim_ = false;
  /**
   * \brief If simulation is being cancelled
   */
  bool is_being_cancelled_ = false;
  /**
   * \brief How many scenarios have been completed
   */
  atomic<size_t> scenarios_done_ = 0;
  /**
   * \brief How many scenarios out of first iteration have been completed
   */
  atomic<size_t> scenarios_required_done_ = 0;
  /**
   * \brief Scenarios completed at time of last interim save
   */
  atomic<size_t> scenarios_last_save_ = 0;
  /**
   * \brief Number of scenarios per iteration
   */
  size_t scenarios_per_iteration_ = 0;
  /**
   * \brief How many iterations have been completed
   */
  size_t iterations_done_ = 0;
  /**
   * \brief If interim outputs are different than last time they were saved
   */
  bool interim_changed_ = true;
  /**
   * \brief If simulation is undex min simulation count
   */
  bool is_under_simulation_minimum_{true};
  /**
   * \brief Number of active simulations still required
   */
  size_t active_simulations_still_required_{};
  /**
   * \brief If simulation is over max simulation count
   */
  bool is_over_simulation_count_ = false;
  /**
   * Conditions for yesterday (or constant weather)
   */
  FwiWeather yesterday_;
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
#endif
