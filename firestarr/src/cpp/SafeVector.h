/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <vector>
namespace tbd::util
{
class Statistics;
/**
 * \brief A vector with added thread safety.
 */
class SafeVector
{
  /**
   * \brief Vector of stored values
   */
  std::vector<MathSize> values_{};
  /**
   * \brief Mutex for parallel access
   */
  mutable mutex mutex_{};
public:
  /**
   * \brief Destructor
   */
  ~SafeVector() = default;
  /**
   * \brief Construct empty SafeVector
   */
  SafeVector() = default;
  /**
   * \brief Copy constructor
   * \param rhs SafeVector to copy from
   */
  SafeVector(const SafeVector& rhs);
  /**
   * \brief Move constructor
   * \param rhs SafeVector to move from
   */
  SafeVector(SafeVector&& rhs) noexcept;
  /**
   * \brief Copy assignment operator
   * \param rhs SafeVector to copy from
   * \return This, after assignment
   */
  SafeVector& operator=(const SafeVector& rhs) noexcept;
  /**
   * \brief Move assignment operator
   * \param rhs SafeVector to move from
   * \return This, after assignment
   */
  SafeVector& operator=(SafeVector&& rhs) noexcept;
  /**
   * \brief Add a value to the SafeVector
   * \param value Value to add
   */
  void addValue(MathSize value);
  /**
   * \brief Get a vector with the stored values
   * \return A vector with the stored values
   */
  [[nodiscard]] std::vector<MathSize> getValues() const;
  /**
   * \brief Calculate Statistics for values in this SafeVector
   * \return Statistics for values in this SafeVector
   */
  [[nodiscard]] Statistics getStatistics() const;
  /**
   * \brief Number of values in the SafeVector
   * \return Size of the SafeVector
   */
  [[nodiscard]] size_t size() const noexcept;
};
}
