/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <list>
#include <string>
#include "Location.h"
#include "Point.h"
namespace tbd
{
namespace wx
{
class FwiWeather;
}
namespace topo
{
class Environment;
/**
 * \brief Perimeter for an existing fire to initialize a simulation with.
 */
class Perimeter
{
public:
  /**
   * \brief Initialize perimeter from a file
   * \param perim File to read from
   * \param point Origin of fire
   * \param env Environment to apply Perimeter to
   */
  Perimeter(const string& perim, const Point& point, const Environment& env);
  /**
   * \brief Create a Perimeter of the given size at the given Location
   * \param location Location to center Perimeter on
   * \param size Size of Perimeter to create
   * \param env Environment to apply Perimeter to
   */
  Perimeter(const Location& location,
            size_t size,
            const Environment& env);
  template <class P>
  Perimeter(const Position<P>& position,
            size_t size,
            const Environment& env)
    : Perimeter(
        Location{position.hash()},
        size,
        env)
  {
  }
  // /**
  //  * \brief Map of all burned Locations
  //  * \return All Locations burned by this Perimeter
  //  */
  // [[nodiscard]] const BurnedMap& burned_map() const noexcept;
  /**
   * \brief List of all burned Locations
   * \return All Locations burned by this Perimeter
   */
  [[nodiscard]] const list<Location>& burned() const noexcept;
  /**
   * \brief List of all Locations along the edge of this Perimeter
   * \return All Locations along the edge of this Perimeter
   */
  [[nodiscard]] const list<Location>& edge() const noexcept;
private:
  // /**
  //  * @brief Map of burned cells
  //  *
  //  */
  // const BurnedMap burned_map_;
  // /**
  //  * \brief List of all burned Locations
  //  */
  list<Location> burned_;
  /**
   * \brief List of all Locations along the edge of this Perimeter
   */
  list<Location> edge_;
};
}
}
