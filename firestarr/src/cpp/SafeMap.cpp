// /* Copyright (c) Queen's Printer for Ontario, 2020. */

// /* SPDX-License-Identifier: AGPL-3.0-or-later */

// #include "stdafx.h"
// #include "SafeMap.h"
// #include "Statistics.h"
// namespace tbd::util
// {
// SafeMap::SafeMap(const SafeMap& rhs)
//   : values_(rhs.values_)
// {
// }
// SafeMap::SafeMap(SafeMap&& rhs) noexcept
//   : values_(std::move(rhs.values_))
// {
// }
// SafeMap& SafeMap::operator=(const SafeMap& rhs) noexcept
// {
//   try
//   {
//     lock_guard<mutex> lock(mutex_);
//     values_ = rhs.values_;
//     return *this;
//   }
//   catch (const std::exception& ex)
//   {
//     logging::fatal(ex);
//     std::terminate();
//   }
// }
// SafeMap& SafeMap::operator=(SafeMap&& rhs) noexcept
// {
//   try
//   {
//     lock_guard<mutex> lock(mutex_);
//     values_ = std::move(rhs.values_);
//     return *this;
//   }
//   catch (const std::exception& ex)
//   {
//     logging::fatal(ex);
//     std::terminate();
//   }
// }
// void SafeMap::addValue(const MathSize value)
// {
//   lock_guard<mutex> lock(mutex_);
//   static_cast<void>(insert_sorted(&values_, value));
// }
// vector<MathSize> SafeMap::getValues() const
// {
//   lock_guard<mutex> lock(mutex_);
//   return values_;
// }
// Statistics SafeMap::getStatistics() const
// {
//   return Statistics{getValues()};
// }
// size_t SafeMap::size() const noexcept
// {
//   return values_.size();
// }
// }
