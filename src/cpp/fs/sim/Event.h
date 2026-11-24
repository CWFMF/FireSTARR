/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_EVENT_H
#define FS_EVENT_H
#include "../stdafx.h"
#include <compare>
#include "../grid/Cell.h"
#include "../types/Location.h"
#include "../wx/WeatherIndices.h"
namespace fs
{
using fs::Direction;
/**
 * \brief A specific Event scheduled in a specific Scenario.
 */
struct Event
{
  /**
   * \brief Type of Event
   */
  enum class Type
  {
    Invalid,
    Save,
    EndSimulation,
    NewFire,
    FireSpread,
  };
  /**
   * \brief Cell representing no location
   */
  static constexpr Cell NoLocation{};
  /**
   * \brief Time of Event (decimal days)
   */
  DurationSize time{INVALID_TIME};
  /**
   * \brief Type of Event
   */
  Type type{Type::Invalid};
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   */
  DurationSize time_at_location{0.0};
  XYIdx xy{};
  /**
   * \brief Head fire rate of spread (m/min)
   */
  ROSSize ros{};
  /**
   * \brief Burn Intensity (kW/m)
   */
  IntensitySize intensity{};
  /**
   * \brief Head fire spread direction
   */
  Direction raz{INVALID_DIRECTION};
  /**
   * \brief CellIndex for relative Cell that spread into from
   */
  CellIndex source{};
  std::partial_ordering operator<=>(const Event& rhs) const
  {
    if (const auto cmp = time <=> rhs.time; 0 != cmp)
    {
      return cmp;
    }
    if (const auto cmp = type <=> rhs.type; 0 != cmp)
    {
      return cmp;
    }
    return xy <=> rhs.xy;
  }
};
}
#endif
