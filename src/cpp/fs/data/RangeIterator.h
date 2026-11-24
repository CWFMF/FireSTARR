/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_RANGE_ITERATOR_H
#define FS_RANGE_ITERATOR_H
#include "../stdafx.h"
// HACK: don't use until logging causing issues with constexpr is fixed
#undef DEBUG_ITERATOR
#ifdef DEBUG_ITERATOR
#include "../log/Log.h"
#endif
namespace fs
{
template <class T = MathSize>
class RangeIterator
{
  friend class iterable;

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = int;
  using value_type = T;
  RangeIterator(
    const value_type start,
    const value_type end,
    const value_type increment,
    const bool inclusive = true
  )
    : start_(start), end_(end), increment_(increment), inclusive_(inclusive)
  {
#ifdef DEBUG_ITERATOR
    logging::verbose(
      "Range is from {:f} to {:f} with step {:f} {:s}",
      start_,
      end_,
      increment_,
      inclusive_ ? "inclusive" : "exclusive"
    );
#endif
  }
  constexpr RangeIterator() = default;
  constexpr RangeIterator(const RangeIterator& rhs) = default;
  constexpr RangeIterator(RangeIterator&& rhs) = default;
  RangeIterator& operator=(const RangeIterator& rhs) = default;
  RangeIterator& operator=(RangeIterator&& rhs) = default;

public:
  constexpr inline value_type operator*() const
  {
    return start_ + static_cast<value_type>(step_) * increment_;
  }
#ifndef DEBUG_ITERATOR
  constexpr
#endif
    inline RangeIterator&
    operator++()
  {
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*(*this) < start_, [&]() {
      return std::format(
        "operator++() {:g} less than start value {:g} ({:+g}) for step {:d}",
        *(*this),
        start_,
        (*(*this) - start_),
        step_
      );
    });
#endif
    step_++;
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*this > end(), [&]() {
      return std::format(
        "operator++() {:g} more than end value {:g} ({:+g}) for step {:d}",
        *(*this),
        *end(),
        (*(*this) - *end()),
        step_
      );
    });
#endif
    return *this;
  }
#ifndef DEBUG_ITERATOR
  constexpr
#endif
    inline RangeIterator
    operator++(int)
  {
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*(*this) < start_, [&]() {
      return std::format(
        "operator++(int) {:g} less than start value {:g} ({:+g}) for step {:d}",
        *(*this),
        start_,
        step_
      );
    });
#endif
    RangeIterator oTmp = *(*this);
    *(*this) += increment_;
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*this > end(), [&]() {
      return std::format(
        "operator++(int) {:g} more than end value {:g} ({:+g}) for step {:d}",
        *(*this),
        *end(),
        step_
      );
    });
#endif
    return oTmp;
  }
#ifndef DEBUG_ITERATOR
  constexpr
#endif
    inline RangeIterator&
    operator--()
  {
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*this > end(), [&]() {
      return std::format(
        "operator--() {:g} more than end value {:g} ({:+g}) for step {:d}", *(*this), *end(), step_
      );
    });
#endif
    *(*this) -= increment_;
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*(*this) < start_, [&]() {
      return std::format(
        "operator--() {:g} less than start value {:g} ({:+g}) for step {:d}",
        *(*this),
        start_,
        step_
      );
    });
#endif
    return *this;
  }
#ifndef DEBUG_ITERATOR
  constexpr
#endif
    inline RangeIterator
    operator--(int)
  {
    RangeIterator oTmp = *(*this);
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*this > end(), [&]() {
      return std::format(
        "operator--(int) {:g} more than end value {:g} ({:+g}) for step {:d}",
        *(*this),
        *end(),
        step_
      );
    });
#endif
    *(*this) -= increment_;
#ifdef DEBUG_ITERATOR
    logging::check_fatal(*(*this) < start_, [&]() {
      return std::format(
        "operator--(int) {:g} less than start value {:g} ({:+g}) for step {:d}",
        *(*this),
        start_,
        step_
      );
    });
#endif
    return oTmp;
  }
  constexpr inline difference_type operator-(const RangeIterator& rhs) const
  {
    return static_cast<difference_type>(step_ - rhs.step_);
  }
  auto operator<=>(const RangeIterator<value_type>& rhs) const = default;
  // inline bool operator==(const RangeIterator& rhs) const { return *(*this) == rhs; }
  // inline bool operator!=(const RangeIterator& rhs) const { return !(*(*this) == rhs); }
  // inline bool operator>(const RangeIterator& rhs) const { return *(*this) < *rhs; }
  // inline bool operator<(const RangeIterator& rhs) const { return *(*this) > *rhs; }
  // inline bool operator>=(const RangeIterator& rhs) const { return *(*this) <= *rhs; }
  // inline bool operator<=(const RangeIterator& rhs) const { return *(*this) >= *rhs; }
  constexpr auto begin() const { return RangeIterator<value_type>(this, start_); }
  constexpr auto end() const
  {
    // if inclusive then end is slightly past end value so end is included
    return RangeIterator<value_type>(
      this, end_ + (inclusive_ ? increment_ : static_cast<value_type>(0))
    );
  }
  difference_type size() const { return end() - begin(); }

private:
  constexpr RangeIterator(const RangeIterator* rhs, const T value)
    : start_(rhs->start_), end_(rhs->end_), increment_(rhs->increment_),
      step_(static_cast<size_t>((static_cast<value_type>(value) - start_) / increment_)),
      inclusive_(rhs->inclusive_)
  { }
  value_type start_{};
  value_type end_{};
  value_type increment_{};
  // CHECK: should be difference_type?
  size_t step_{0};
  bool inclusive_{};
};
static_assert(
  std::bidirectional_iterator<RangeIterator<MathSize>>,
  "iterator must be an iterator!"
);
static_assert(std::bidirectional_iterator<RangeIterator<int>>, "iterator must be an iterator!");
inline auto range(
  const MathSize start,
  const MathSize end,
  const MathSize step,
  const bool inclusive = true
)
{
  std::ignore = inclusive;
  auto r = RangeIterator(start, end, step);
  return vector<MathSize>{r.begin(), r.end()};
}
inline auto range_int(const int start, const int end, const int step, const bool inclusive = true)
{
  std::ignore = inclusive;
  auto r = RangeIterator<int>(start, end, step);
  return vector<int>{r.begin(), r.end()};
}
}
#endif
