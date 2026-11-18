/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "SafeVector.h"
#include "../log/Log.h"
#include "../util/Statistics.h"
namespace fs
{
SafeVector::SafeVector(const SafeVector& rhs) : values_(rhs.values_) { }
SafeVector::SafeVector(SafeVector&& rhs) noexcept : values_(std::move(rhs.values_)) { }
SafeVector& SafeVector::operator=(const SafeVector& rhs) noexcept
{
  try
  {
    scoped_lock lock{mutex_};
    values_ = rhs.values_;
    return *this;
  }
  catch (const std::exception& ex)
  {
    exit(logging::fatal(ex));
  }
}
SafeVector& SafeVector::operator=(SafeVector&& rhs) noexcept
{
  try
  {
    scoped_lock lock{mutex_};
    values_ = std::move(rhs.values_);
    return *this;
  }
  catch (const std::exception& ex)
  {
    exit(logging::fatal(ex));
  }
}
void SafeVector::addValue(const MathSize value)
{
  scoped_lock lock{mutex_};
  static_cast<void>(insert_sorted(&values_, value));
}
vector<MathSize> SafeVector::getValues() const
{
  scoped_lock lock{mutex_};
  return values_;
}
Statistics SafeVector::getStatistics() const { return Statistics{getValues()}; }
size_t SafeVector::size() const noexcept { return values_.size(); }
}
