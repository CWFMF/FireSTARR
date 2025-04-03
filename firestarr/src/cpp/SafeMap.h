// /* Copyright (c) Queen's Printer for Ontario, 2020. */

// /* SPDX-License-Identifier: AGPL-3.0-or-later */

// #pragma once
// #include <vector>
// namespace tbd::util
// {
// /**
//  * \brief A vector with added thread safety.
//  */
// template <class K, class V>
// class SafeMap
// {
//   /**
//    * \brief Map of stored values
//    */
//   std::map<K, V> values_{};
//   /**
//    * \brief Mutex for parallel access
//    */
//   mutable mutex mutex_{};
// public:
//   /**-
//    * \brief Destructor
//    */
//   ~SafeMap() = default;
//   /**
//    * \brief Construct empty SafeMap
//    */
//   SafeMap() = default;
//   /**
//    * \brief Copy constructor
//    * \param rhs SafeMap to copy from
//    */
//   SafeMap(const SafeMap& rhs);
//   /**
//    * \brief Move constructor
//    * \param rhs SafeMap to move from
//    */
//   SafeMap(SafeMap&& rhs) noexcept;
//   /**
//    * \brief Copy assignment operator
//    * \param rhs SafeMap to copy from
//    * \return This, after assignment
//    */
//   SafeMap& operator=(const SafeMap& rhs) noexcept;
//   /**
//    * \brief Move assignment operator
//    * \param rhs SafeMap to move from
//    * \return This, after assignment
//    */
//   SafeMap& operator=(SafeMap&& rhs) noexcept;
//   /**
//    * \brief Add a value to the SafeMap
//    * \param key Key for value
//    * \param value Value to add
//    */
//   void addValue(K key, V value);
//   /**
//    * \brief Get a vector with the stored values
//    * \return A vector with the stored values
//    */
//   [[nodiscard]] std::vector<MathSize> getValues() const;
//   /**
//    * \brief Calculate Statistics for values in this SafeMap
//    * \return Statistics for values in this SafeMap
//    */
//   [[nodiscard]] Statistics getStatistics() const;
//   /**
//    * \brief Number of values in the SafeMap
//    * \return Size of the SafeMap
//    */
//   [[nodiscard]] size_t size() const noexcept;
// };
// }
