/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
namespace tbd::data
{
/**
 * \brief A wrapper around a MathSize to ensure correct types are used.
 * \tparam T The derived class that this Index represents.
 */
template <class T>
class Index
{
  /**
   * \brief Value represented by this
   */
  MathSize value_;
public:
  /**
   * \brief Destructor
   */
  ~Index() = default;
  /**
   * \brief Construct with a value of 0
   */
  constexpr Index() noexcept
    : value_(0)
  {
  }
  /**
   * \brief Construct with given value
   * \param value Value to assign
   */
  constexpr explicit Index(const MathSize value) noexcept
    : value_(value)
  {
  }
  /**
   * \brief Move constructor
   * \param rhs Index to move from
   */
  constexpr Index(Index<T>&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Index to copy from
   */
  constexpr Index(const Index<T>& rhs) noexcept = default;
  /**
   * \brief Move assignment
   * \param rhs Index to move from
   * \return This, after assignment
   */
  Index<T>& operator=(Index<T>&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs Index to copy from
   * \return This, after assignment
   */
  Index<T>& operator=(const Index<T>& rhs) noexcept = default;
  /**
   * \brief Equality operator
   * \param rhs Index to compare to
   * \return Whether or not these are equivalent
   */
  [[nodiscard]] constexpr bool operator==(const Index<T>& rhs) const noexcept
  {
    return value_ == rhs.value_;
  }
  /**
   * \brief Not equals operator
   * \param rhs Index to compare to
   * \return Whether or not these are not equivalent
   */
  [[nodiscard]] constexpr bool operator!=(const Index<T>& rhs) const noexcept
  {
    return !(*this == rhs);
  }
  /**
   * \brief Returns value as a MathSize
   * \return MathSize value for Index
   */
  [[nodiscard]] constexpr MathSize asValue() const noexcept
  {
    return value_;
  }
  /**
   * \brief Less than operator
   * \param rhs Index to compare to
   * \return Whether or not this is less than the provided Index
   */
  constexpr bool operator<(const Index<T> rhs) const noexcept
  {
    return value_ < rhs.value_;
  }
  /**
   * \brief Greater than operator
   * \param rhs Index to compare to
   * \return Whether or not this is greater than the provided Index
   */
  [[nodiscard]] constexpr bool operator>(const Index<T> rhs) const noexcept
  {
    return value_ > rhs.value_;
  }
  /**
   * \brief Less than or equal to operator
   * \param rhs Index to compare to
   * \return Whether or not this is less than or equal to the provided Index
   */
  [[nodiscard]] constexpr bool operator<=(const Index<T> rhs) const noexcept
  {
    return value_ <= rhs.value_;
  }
  /**
   * \brief Greater than or equal to operator
   * \param rhs Index to compare to
   * \return Whether or not this is greater than or equal to the provided Index
   */
  [[nodiscard]] constexpr bool operator>=(const Index<T> rhs) const noexcept
  {
    return value_ >= rhs.value_;
  }
  /**
   * \brief Addition operator
   * \param rhs Index to add value from
   * \return The value of this plus the value of the provided index
   */
  [[nodiscard]] constexpr Index<T> operator+(const Index<T> rhs) const noexcept
  {
    return Index<T>(value_ + rhs.value_);
  }
  /**
   * \brief Subtraction operator
   * \param rhs Index to add value from
   * \return The value of this minus the value of the provided index
   */
  [[nodiscard]] constexpr Index<T> operator-(const Index<T> rhs) const noexcept
  {
    return Index<T>(value_ - rhs.value_);
  }
  /**
   * \brief Addition assignment operator
   * \param rhs Index to add value from
   * \return This, plus the value of the provided Index
   */
  constexpr Index<T>& operator+=(const Index<T> rhs) noexcept
  {
    value_ += rhs.value_;
    return *this;
  }
  /**
   * \brief Subtraction assignment operator
   * \param rhs Index to add value from
   * \return This, minus the value of the provided Index
   */
  constexpr Index<T>& operator-=(const Index<T> rhs) noexcept
  {
    value_ -= rhs.value_;
    return *this;
  }
};
/**
 * \brief A result of calling log(x) for some value of x, pre-calculated at compile time.
 */
class LogValue
  : public Index<LogValue>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
};
static constexpr LogValue LOG_0_70{-0.35667494393873245};
static constexpr LogValue LOG_0_75{-0.2876820724517809};
static constexpr LogValue LOG_0_80{-0.2231435513142097};
static constexpr LogValue LOG_0_85{-0.16251892949777494};
static constexpr LogValue LOG_0_90{-0.10536051565782628};
static constexpr LogValue LOG_1_00{0.0};
#ifndef _WIN32
// windows won't use constexpr with log but we can MathSize check numbers are right when compiling elsewhere
static constexpr LogValue LOG_0_70_CALC{log(0.7)};
static constexpr LogValue LOG_0_75_CALC{log(0.75)};
static constexpr LogValue LOG_0_80_CALC{log(0.8)};
static constexpr LogValue LOG_0_85_CALC{log(0.85)};
static constexpr LogValue LOG_0_90_CALC{log(0.9)};
static constexpr LogValue LOG_1_00_CALC{log(1.0)};
static_assert(abs((LOG_0_70 - LOG_0_70_CALC).asValue()) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_0_75 - LOG_0_75_CALC).asValue()) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_0_80 - LOG_0_80_CALC).asValue()) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_0_90 - LOG_0_90_CALC).asValue()) < numeric_limits<MathSize>::epsilon());
static_assert(abs((LOG_1_00 - LOG_1_00_CALC).asValue()) < numeric_limits<MathSize>::epsilon());
#endif
}
