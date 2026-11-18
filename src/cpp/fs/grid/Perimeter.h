/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_PERIMETER_H
#define FS_PERIMETER_H
#include "../stdafx.h"
#include "../geo/Point.h"
#include "GridMap.h"
namespace fs
{
class Environment;
/**
 * \brief A map of locations which have burned in a Scenario.
 * Use this class so that we can filter by fuel cells but not expose the members
 */
class BurnedMap final : public GridMap<unsigned char>
{
public:
  /**
   * \brief Constructor
   * \param perim_grid Grid representing Perimeter to initialize from
   * \param env Environment to use as base
   */
  BurnedMap(const Grid<unsigned char, unsigned char>& perim_grid, const Environment& env);
  size_t size_hectares() const { return size_hectares_; }

private:
  /**
   * \brief Size in hectares
   */
  size_t size_hectares_;
};
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
  Perimeter(const LazyPath& perim, const Point& point, const Environment& env);
  Perimeter(const XYIdx& location, const size_t size, const Environment& env);
  /**
   * \brief List of all burned Locations
   */
  const list<XYIdx> burned;
  /**
   * \brief List of all Locations along the edge of this Perimeter
   */
  const list<XYIdx> edge;

private:
  Perimeter(const BurnedMap& burned_map);
};
}
#endif
