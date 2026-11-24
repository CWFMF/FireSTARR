/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SAFE_VECTOR_H
#define FS_SAFE_VECTOR_H
#include "../stdafx.h"
namespace fs
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
  ~SafeVector() = default;
  /**
   * \brief Construct empty SafeVector
   */
  SafeVector() = default;
  SafeVector(const SafeVector& rhs);
  SafeVector(SafeVector&& rhs) noexcept;
  SafeVector& operator=(const SafeVector& rhs) noexcept;
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
#endif
