/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INTENSITY_MAP_H
#define FS_INTENSITY_MAP_H
#include "../stdafx.h"
#include "../types/Location.h"
#include "BurnedData.h"
#include "GridMap.h"
namespace fs
{
class Perimeter;
class Cell;
class ProbabilityMap;
class Model;
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
  explicit IntensityMap(const Model& model) noexcept;
  ~IntensityMap() noexcept = default;
  IntensityMap(const IntensityMap& rhs);
  IntensityMap(IntensityMap&& rhs) = delete;
  IntensityMap& operator=(const IntensityMap& rhs) = delete;
  IntensityMap& operator=(IntensityMap&& rhs) noexcept = delete;
  [[nodiscard]] Idx height() const { return is_burned_.height(); }
  [[nodiscard]] Idx width() const { return is_burned_.width(); }
  /**
   * \brief Set cells in the map to be burned based on Perimeter
   * \param perimeter Perimeter to burn cells based on
   */
  void applyPerimeter(const Perimeter& perimeter) noexcept;
  [[nodiscard]] bool canBurn(const XYIdx& location) const;
  [[nodiscard]] bool hasBurned(const XYIdx& location) const;
  [[nodiscard]] bool isSurrounded(const XYIdx& location) const;
  void ignite(const XYIdx& location);

public:
  /**
   * \brief Update Location with specified values
   * \param location Location to burn
   * \param intensity Intensity to burn with (kW/m)
   * \param ros Rate of spread to check against maximu (m/min)
   * \param raz Spread azimuth for ros
   */
  void burn(const XYIdx& location, IntensitySize intensity, MathSize ros, fs::Direction raz);
  /**
   * \brief Save contents to file
   * \param dir Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList save(const string_view dir, const string_view base_name) const;
  /**
   * \brief Size of the fire represented by this
   * \return Size of the fire represented by this
   */
  [[nodiscard]] MathSize fireSize() const { return is_burned_.fireSize(); }
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<XYIdx, IntensitySize>::const_iterator cbegin() const noexcept;
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<XYIdx, IntensitySize>::const_iterator cend() const noexcept;

private:
  /**
   * \brief Model map is for
   */
  const Model& model_;
  // Map of intensity that cells have burned  at
  GridMap<IntensitySize> intensity_max_{};
  // HACK: just add ROS/RAZ into this object for now
  // Map of rate of spread/direction that cells have burned with at max ros
  std::optional<GridMap<MathSize>> rate_of_spread_at_max_{};
  std::optional<GridMap<DegreesSize>> direction_of_spread_at_max_{};
  // bitset denoting cells that can no longer burn
  BurnedData is_burned_{};
};
}
#endif
