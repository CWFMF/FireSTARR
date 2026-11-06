/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "StartPoint.h"
namespace fs::rng
{
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
template <class V>
constexpr V same(const V value) noexcept
{
  return value;
}
void make_threshold(
  vector<ThresholdSize>* thresholds,
  mt19937_64* mt,
  const Day start_day,
  const Day last_date,
  const ThresholdSize threshold_scenario_weight,
  const ThresholdSize threshold_daily_weight,
  const ThresholdSize threshold_hourly_weight,
  ThresholdSize (*convert)(_Float64 value) = same
);
void make_threshold(
  vector<ThresholdSize>* thresholds,
  mt19937_64* mt,
  const Day start_day,
  const Day last_date,
  ThresholdSize (*convert)(_Float64 value) = same
);
std::seed_seq make_seed(
  const char* name,
  const StartPoint& start_point,
  const Day start_day,
  const size_t salt,
  const size_t base_salt
);
};
