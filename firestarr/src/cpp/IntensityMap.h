/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <memory>
#include <string>
#include <bitset>
#include "GridMap.h"
#include "Location.h"
namespace fs
{
namespace topo
{
class Perimeter;
class Cell;
}
namespace sim
{
using fs::topo::Position;
class ProbabilityMap;
class Model;
using BurnedData = std::bitset<static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS>;
/**
 * \brief Represents a map of intensities that cells have burned at for a single Scenario.
 */
class IntensityMap
{
  /**
   * \brief Mutex for parallel access
   */
  mutable mutex mutex_{};
public:
  /**
   * \brief Constructor
   * \param model Model to use extent from
   */
  // IntensityMap(const Model& model, topo::Perimeter* perimeter) noexcept;
  explicit IntensityMap(const Model& model) noexcept;
  ~IntensityMap() noexcept;
  IntensityMap(const IntensityMap& rhs);
  IntensityMap(IntensityMap&& rhs) = delete;
  // IntensityMap(IntensityMap&& rhs);
  IntensityMap& operator=(const IntensityMap& rhs) = delete;
  IntensityMap& operator=(IntensityMap&& rhs) noexcept = delete;
  /**
   * \brief Number of rows in this extent
   * \return Number of rows in this extent
   */
  [[nodiscard]] Idx rows() const
  {
    return intensity_max_->rows();
  }
  /**
   * \brief Number of columns in this extent
   * \return Number of columns in this extent
   */
  [[nodiscard]] Idx columns() const
  {
    return intensity_max_->columns();
  }
  /**
   * \brief Set cells in the map to be burned based on Perimeter
   * \param perimeter Perimeter to burn cells based on
   */
  void applyPerimeter(const topo::Perimeter& perimeter) noexcept;
  /**
   * \brief Whether or not the Cell with the given hash can burn
   * \param hash Hash for Cell to check
   * \return Whether or not the Cell with the given hash can burn
   */
  [[nodiscard]] bool canBurn(const Location& location) const;
  template <class P>
  [[nodiscard]] bool canBurn(const Position<P>& position) const
  {
    return canBurn(Location{position.hash()});
  }
  /**
   * \brief Whether or not the Location with the given hash can burn
   * \param hash Hash for Location to check
   * \return Whether or not the Location with the given hash can burn
   */
  [[nodiscard]] bool hasBurned(const Location& location) const;
  template <class P>
  [[nodiscard]] bool hasBurned(const Position<P>& position) const
  {
    return hasBurned(Location{position.hash()});
  }
  /**
   * \brief Whether or not all Locations surrounding the given Location are burned
   * \param location Location to check
   * \return Whether or not all Locations surrounding the given Location are burned
   */
  [[nodiscard]] bool isSurrounded(const Location& location) const;
  template <class P>
  [[nodiscard]] bool isSurrounded(const Position<P>& position) const
  {
    return isSurrounded(Location{position.hash()});
  }
  /**
   * \brief Mark given location as burned
   * \param location Location to burn
   */
  void ignite(const Location& location);
  template <class P>
  void ignite(const Position<P>& position)
  {
    ignite(Location{position.hash()});
  }
public:
  /**
   * \brief Update Location with specified values
   * \param location Location to burn
   * \param intensity Intensity to burn with (kW/m)
   * \param ros Rate of spread to check against maximu (m/min)
   * \param raz Spread azimuth for ros
   */
  void burn(const Location& location);
  template <class P>
  void burn(const Position<P>& position)
  {
    burn(
      Location{position.hash()});
  }
  /**
   * \brief Size of the fire represented by this
   * \return Size of the fire represented by this
   */
  [[nodiscard]] MathSize fireSize() const;
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<Location, IntensitySize>::const_iterator
    cbegin() const noexcept;
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<Location, IntensitySize>::const_iterator
    cend() const noexcept;
private:
  /**
   * \brief Model map is for
   */
  const Model& model_;
  /**
   * \brief Map of intensity that cells have burned  at
   */
  unique_ptr<data::GridMap<IntensitySize>> intensity_max_;
  /**
   * \brief bitset denoting cells that can no longer burn
   */
  BurnedData* is_burned_;
};
}
}
