/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <tuple>
#include "Point.h"
namespace tbd
{
namespace topo
{
/**
 * \brief A Point that has sunrise and sunset times for each day.
 */
class StartPoint : public Point
{
  /**
   * \brief Array of tuple for sunrise/sunset times by day
   */
  array<tuple<DurationSize, DurationSize>, MAX_DAYS> days_;
public:
  /**
   * \brief Constructor
   * \param latitude Latitude (decimal degrees)
   * \param longitude Longitude (decimal degrees)
   */
  StartPoint(MathSize latitude, MathSize longitude) noexcept;
  ~StartPoint() noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs StartPoint to copy from
   */
  StartPoint(const StartPoint& rhs) noexcept = default;
  /**
   * \brief Move constructor
   * \param rhs StartPoint to move from
   */
  StartPoint(StartPoint&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs StartPoint to copy from
   * \return This, after assignment
   */
  StartPoint& operator=(const StartPoint& rhs) = default;
  /**
   * \brief Move assignment
   * \param rhs StartPoint to move from
   * \return This, after assignment
   */
  StartPoint& operator=(StartPoint&& rhs) noexcept;
  /**
   * \brief Sunrise time
   * \param day Day
   * \return Sunrise time on give day
   */
  [[nodiscard]] constexpr DurationSize dayStart(const size_t day) const
  {
    return get<0>(days_.at(day));
  }
  /**
   * \brief Sunset time
   * \param day Day
   * \return Sunset time on give day
   */
  [[nodiscard]] constexpr DurationSize dayEnd(const size_t day) const
  {
    return get<1>(days_.at(day));
  }
};
}
}
