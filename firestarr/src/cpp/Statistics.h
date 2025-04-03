/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <algorithm>
#include <numeric>
#include <vector>
#include "Log.h"
#include "Settings.h"
#include "Util.h"
namespace tbd
{
namespace util
{
/**
 * \brief Student's T critical values
 */
static constexpr array<MathSize, 100> T_VALUES{
  3.078,
  1.886,
  1.638,
  1.533,
  1.476,
  1.440,
  1.415,
  1.397,
  1.383,
  1.372,
  1.363,
  1.356,
  1.350,
  1.345,
  1.341,
  1.337,
  1.333,
  1.330,
  1.328,
  1.325,
  1.323,
  1.321,
  1.319,
  1.318,
  1.316,
  1.315,
  1.314,
  1.313,
  1.311,
  1.310,
  1.309,
  1.309,
  1.308,
  1.307,
  1.306,
  1.306,
  1.305,
  1.304,
  1.304,
  1.303,
  1.303,
  1.302,
  1.302,
  1.301,
  1.301,
  1.300,
  1.300,
  1.299,
  1.299,
  1.299,
  1.298,
  1.298,
  1.298,
  1.297,
  1.297,
  1.297,
  1.297,
  1.296,
  1.296,
  1.296,
  1.296,
  1.295,
  1.295,
  1.295,
  1.295,
  1.295,
  1.294,
  1.294,
  1.294,
  1.294,
  1.294,
  1.293,
  1.293,
  1.293,
  1.293,
  1.293,
  1.293,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.290,
  1.290,
  1.290,
  1.290,
  1.290};
/**
 * \brief Provides statistics calculation for vectors of values.
 */
class Statistics
{
public:
  /**
   * \brief Minimum value
   * \return Minimum value
   */
  [[nodiscard]] MathSize min() const noexcept
  {
    return percentiles_[0];
  }
  /**
   * \brief Maximum value
   * \return Maximum value
   */
  [[nodiscard]] MathSize max() const noexcept
  {
    return percentiles_[100];
  }
  /**
   * \brief Median value
   * \return Median value
   */
  [[nodiscard]] MathSize median() const noexcept
  {
    return percentiles_[50];
  }
  /**
   * \brief Mean (average) value
   * \return Mean (average) value
   */
  [[nodiscard]] MathSize mean() const noexcept
  {
    return mean_;
  }
  /**
   * \brief Standard Deviation
   * \return Standard Deviation
   */
  [[nodiscard]] MathSize standardDeviation() const noexcept
  {
    return standard_deviation_;
  }
  /**
   * \brief Sample Variance
   * \return Sample Variance
   */
  [[nodiscard]] MathSize sampleVariance() const noexcept
  {
    return sample_variance_;
  }
  /**
   * \brief Number of data points in the set
   * \return Number of data points in the set
   */
  [[nodiscard]] size_t n() const noexcept
  {
    return n_;
  }
  /**
   * \brief Value for given percentile
   * \param i Percentile to retrieve value for
   * \return Value for given percentile
   */
  [[nodiscard]] MathSize percentile(const uint8_t i) const noexcept
  {
#ifdef DEBUG_STATISTICS
    logging::check_fatal(static_cast<size_t>(i) >= percentiles_.size(),
                         "Invalid percentile %d requested",
                         i);
#endif
    return percentiles_.at(i);
  }
  /**
   * \brief 80% Confidence Interval
   * \return 80% Confidence Interval
   */
  [[nodiscard]] MathSize confidenceInterval80() const
  {
    return confidenceInterval(1.28);
  }
  /**
   * \brief 90% Confidence Interval
   * \return 90% Confidence Interval
   */
  [[nodiscard]] MathSize confidenceInterval90() const
  {
    return confidenceInterval(1.645);
  }
  /**
   * \brief 95% Confidence Interval
   * \return 95% Confidence Interval
   */
  [[nodiscard]] MathSize confidenceInterval95() const
  {
    return confidenceInterval(1.96);
  }
  /**
   * \brief 98% Confidence Interval
   * \return 98% Confidence Interval
   */
  [[nodiscard]] MathSize confidenceInterval98() const
  {
    return confidenceInterval(2.33);
  }
  /**
   * \brief 99% Confidence Interval
   * \return 99% Confidence Interval
   */
  [[nodiscard]] MathSize confidenceInterval99() const
  {
    return confidenceInterval(2.58);
  }
  /**
   * \brief Calculates statistics on a vector of values
   * \param values Values to use for calculation
   */
  explicit Statistics(vector<MathSize> values)
  {
    // values should already be sorted
    //  std::sort(values.begin(), values.end());
    n_ = values.size();
    min_ = values[0];
    max_ = values[n_ - 1];
    median_ = values[n_ / 2];
    const auto total_sum = std::accumulate(values.begin(),
                                           values.end(),
                                           0.0,
                                           [](const MathSize t, const MathSize x) { return t + x; });
    mean_ = total_sum / n_;
    for (size_t i = 0; i < percentiles_.size(); ++i)
    {
      const auto pos = std::min(n_ - 1,
                                static_cast<size_t>(truncl(
                                  (static_cast<MathSize>(i) / (percentiles_.size() - 1)) * n_)));
      // note("For %d values %dth percentile is at %d", n_, i, pos);
      percentiles_[i] = values[pos];
    }
    const auto total = std::accumulate(values.begin(),
                                       values.end(),
                                       0.0,
                                       [this](const MathSize t, const MathSize x) { return t + pow_int<2>(x - mean_); });
    standard_deviation_ = sqrt(total / n_);
    sample_variance_ = total / (n_ - 1);
#ifdef DEBUG_STATISTICS
    logging::check_equal(min_, percentiles_[0], "min");
    logging::check_equal(max_, percentiles_[100], "max");
    logging::check_equal(median_, percentiles_[50], "median");
#endif
  }
  /**
   * \brief Calculate Student's T value
   * \return Student's T value
   */
  [[nodiscard]] MathSize studentsT() const noexcept
  {
    const auto result = T_VALUES[std::min(T_VALUES.size(), n()) - 1]
                      * sqrt(sampleVariance() / n()) / abs(mean());
    // printf("%ld %f %f %f\n", n(), mean(), sampleVariance(), result);
    return result;
  }
  /**
   * \brief Whether or not we have less than the relative error and can be confident in the results
   * \param relative_error Relative Error that is required
   * \return If Student's T value is less than the relative error
   */
  [[nodiscard]] bool isConfident(const MathSize relative_error) const noexcept
  {
    const auto st = studentsT();
    const auto re = relative_error / (1 + relative_error);
    // printf("%f <= %f is %s\n", st, re, ((st <= re) ? "true" : "false"));
    return st <= re;
  }
  /**
   * \brief Estimate how many more runs are required to achieve desired confidence
   * \param cur_runs Current number of runs completed
   * \param relative_error Relative Error to achieve to be confident
   * \return Number of runs still required
   */
  [[nodiscard]] size_t runsRequired(
    // const size_t cur_runs,
    const MathSize relative_error) const
  {
    const auto re = relative_error / (1 + relative_error);
    const std::function<MathSize(size_t)> fct = [this](const size_t i) noexcept {
      return T_VALUES[std::min(T_VALUES.size(), i) - 1]
           * sqrt(sampleVariance() / i) / abs(mean());
    };
    const auto cur_runs = n();
    return binary_find_checked(cur_runs, 10 * cur_runs, re, fct) - cur_runs;
  }
private:
  /**
   * \brief Calculate Confidence Interval for given z value
   * \param z Z value to calculate for
   * \return Confidence Interval
   */
  [[nodiscard]] MathSize confidenceInterval(const MathSize z) const
  {
    return z * mean_ / sqrt(n_);
  }
  /**
   * \brief Number of values
   */
  size_t n_;
  /**
   * \brief Minimum value
   */
  MathSize min_;
  /**
   * \brief Maximum value
   */
  MathSize max_;
  /**
   * \brief Mean (average) value
   */
  MathSize mean_;
  /**
   * \brief Median value
   */
  MathSize median_;
  /**
   * \brief Standard Deviation
   */
  MathSize standard_deviation_;
  /**
   * \brief Sample variance
   */
  MathSize sample_variance_;
  /**
   * \brief Array of all integer percentile values
   */
  array<MathSize, 101> percentiles_{};
};
}
}
