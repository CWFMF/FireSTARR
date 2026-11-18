/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_START_POINT_H
#define FS_START_POINT_H
#include "../stdafx.h"
#include "../geo/Point.h"
#include "Settings.h"
namespace fs
{
using settings::Settings;
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
  StartPoint(const StartPoint& rhs) noexcept = default;
  StartPoint(StartPoint&& rhs) noexcept = default;
  StartPoint& operator=(const StartPoint& rhs) = default;
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
  // private:
  //   /**
  //    * \brief Offset from UTC (hours)
  //    */
  //   MathSize utc_offset_;
};
}
#endif
