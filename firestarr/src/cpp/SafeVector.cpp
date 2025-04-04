/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "SafeVector.h"
#include "Statistics.h"
namespace fs::util
{
SafeVector::SafeVector(const SafeVector& rhs)
  : values_(rhs.values_)
{
}
SafeVector::SafeVector(SafeVector&& rhs) noexcept
  : values_(std::move(rhs.values_))
{
}
SafeVector& SafeVector::operator=(const SafeVector& rhs) noexcept
{
  try
  {
    lock_guard<mutex> lock(mutex_);
    values_ = rhs.values_;
    return *this;
  }
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
    std::terminate();
  }
}
SafeVector& SafeVector::operator=(SafeVector&& rhs) noexcept
{
  try
  {
    lock_guard<mutex> lock(mutex_);
    values_ = std::move(rhs.values_);
    return *this;
  }
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
    std::terminate();
  }
}
void SafeVector::addValue(const MathSize value)
{
  lock_guard<mutex> lock(mutex_);
  static_cast<void>(insert_sorted(&values_, value));
}
vector<MathSize> SafeVector::getValues() const
{
  lock_guard<mutex> lock(mutex_);
  return values_;
}
Statistics SafeVector::getStatistics() const
{
  return Statistics{getValues()};
}
size_t SafeVector::size() const noexcept
{
  return values_.size();
}
}
