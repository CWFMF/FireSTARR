/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <random>
#include <vector>
#include "SafeVector.h"
#include "IntensityMap.h"
namespace fs::sim
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
  /**
   * \brief Copy constructor
   * \param rhs Iteration to copy form
   */
  Iteration(const Iteration& rhs) = default;
  /**
   * \brief Move constructor
   * \param rhs Iteration to move from
   */
  Iteration(Iteration&& rhs) = default;
  /**
   * \brief Copy assignment
   * \param rhs Iteration to copy from
   * \return This, after assignment
   */
  Iteration& operator=(const Iteration& rhs) = default;
  /**
   * \brief Move assignment
   * \param rhs Iteration to move from
   * \return This, after assignment
   */
  Iteration& operator=(Iteration&& rhs) = default;
  /**
   * \brief Assign start Cell and create new thresholds for use in each Scenario
   * \param start_cell Cell to start ignition in
   * \return This
   */
  Iteration* reset_with_new_start(const shared_ptr<topo::Cell>& start_cell);
  /**
   * \brief Create new thresholds for use in each Scenario
   * \param mt_extinction Extinction thresholds
   * \param mt_spread Spread thresholds
   * \return This
   */
  Iteration* reset(mt19937* mt_extinction,
                   mt19937* mt_spread);
  /**
   * \brief List of Scenarios this Iteration contains
   * \return List of Scenarios this Iteration contains
   */
  [[nodiscard]] const vector<Scenario*>& getScenarios() const noexcept
  {
    return scenarios_;
  }
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
  [[nodiscard]] util::SafeVector finalSizes() const;
private:
  /**
   * \brief List of Scenarios this Iteration contains
   */
  vector<Scenario*> scenarios_;
  /**
   * \brief SafeVector of sizes that Scenarios have resulted in
   */
  util::SafeVector final_sizes_{};
  /**
   * \brief Whether this has been cancelled and should stop computing.
   */
  bool cancelled_ = false;
};
}
