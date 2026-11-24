/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_ITERATION_H
#define FS_ITERATION_H
#include "../stdafx.h"
#include "../data/SafeVector.h"
#include "../types/Location.h"
namespace fs
{
class ProbabilityMap;
class Scenario;
/**
 * \brief Represents a full set of simulations using all available weather streams.
 */
class Iteration
{
public:
  ~Iteration();
  /**
   * \brief Constructor
   * \param scenarios List of Scenarios to wrap into Iteration
   */
  explicit Iteration(vector<Scenario*> scenarios) noexcept;
  Iteration(const Iteration& rhs) = default;
  Iteration(Iteration&& rhs) = default;
  Iteration& operator=(const Iteration& rhs) = default;
  Iteration& operator=(Iteration&& rhs) = default;
  /**
   * \brief Assign start Cell and create new thresholds for use in each Scenario
   * \param start_cell Cell to start ignition in
   * \return This
   */
  Iteration* reset_with_new_start(const XYIdx& start_cell);
  /**
   * \brief Create new thresholds for use in each Scenario
   * \param mt_extinction Extinction thresholds
   * \param mt_spread Spread thresholds
   * \return This
   */
  Iteration* reset(mt19937_64* mt_extinction, mt19937_64* mt_spread);
  /**
   * \brief List of Scenarios this Iteration contains
   * \return List of Scenarios this Iteration contains
   */
  [[nodiscard]] const vector<Scenario*>& getScenarios() const noexcept { return scenarios_; }
  /**
   * Mark as cancelled so it stops computing on next event.
   * \param Whether to log a warning about this being cancelled
   */
  void cancel(bool show_warning) noexcept;
  /**
   * \brief Points in time that ProbabilityMaps get saved for
   * \return Points in time that ProbabilityMaps get saved for
   */
  [[nodiscard]] vector<DurationSize> savePoints() const;
  /**
   * \brief Time that simulations start
   * \return Time that simulations start
   */
  [[nodiscard]] DurationSize startTime() const;
  /**
   * \brief Number of Scenarios in this Iteration
   * \return Number of Scenarios in this Iteration
   */
  [[nodiscard]] size_t size() const noexcept;
  /**
   * \brief SafeVector of sizes that Scenarios have resulted in
   * \return SafeVector of sizes that Scenarios have resulted in
   */
  [[nodiscard]] SafeVector finalSizes() const;

private:
  /**
   * \brief List of Scenarios this Iteration contains
   */
  vector<Scenario*> scenarios_;
  /**
   * \brief SafeVector of sizes that Scenarios have resulted in
   */
  SafeVector final_sizes_{};
  /**
   * \brief Whether this has been cancelled and should stop computing.
   */
  bool cancelled_ = false;
};
}
#endif
