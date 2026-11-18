/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_ENVIRONMENT_INFO_H
#define FS_ENVIRONMENT_INFO_H
#include "../stdafx.h"
#include "Environment.h"
#include "Grid.h"
namespace fs
{
/**
 * \brief Information regarding an Environment, such as grids to read and location.
 */
class EnvironmentInfo
{
public:
  /**
   * \brief Load EnvironmentInfo from given rasters
   * \param in_fuel Fuel raster
   * \param in_elevation Elevation raster
   * \return EnvironmentInfo
   */
  [[nodiscard]] static unique_ptr<EnvironmentInfo> loadInfo(
    const string_view in_fuel,
    const string_view in_elevation
  );
  ~EnvironmentInfo();
  /**
   * \brief Construct from given rasters
   * \param in_fuel Fuel raster
   * \param in_elevation Elevation raster
   */
  EnvironmentInfo(const string_view in_fuel, const string_view in_elevation);
  EnvironmentInfo(EnvironmentInfo&& rhs) noexcept = default;
  EnvironmentInfo(const EnvironmentInfo& rhs) = delete;
  EnvironmentInfo& operator=(EnvironmentInfo&& rhs) noexcept = default;
  EnvironmentInfo& operator=(const EnvironmentInfo& rhs) = delete;
  /**
   * \brief Determine Coordinates in the grid for the Point
   * \param point Point to find Coordinates for
   * \param flipped Whether the grid data is flipped across the horizontal axis
   * \return Coordinates that would be at Point within this EnvironmentInfo, or nullptr if it is not
   */
  [[nodiscard]] std::optional<Coordinates> findCoordinates(const Point& point, bool flipped) const;
  /**
   * \brief Determine FullCoordinates in the grid for the Point
   * \param point Point to find FullCoordinates for
   * \param flipped Whether the grid data is flipped across the horizontal axis
   * \return Coordinates that would be at Point within this EnvironmentInfo, or nullptr if it is not
   */
  [[nodiscard]] std::optional<FullCoordinates> findFullCoordinates(const Point& point, bool flipped)
    const;
  [[nodiscard]] bool validate(const Point& point) const;
  /**
   * \brief Load the full Environment
   * \param point Origin Point
   * \return
   */
  [[nodiscard]] Environment load(const Point& point) const;
  [[nodiscard]] constexpr FullIdx calculateHeight() const { return fuel_.calculateHeight(); }
  [[nodiscard]] constexpr FullIdx calculateWidth() const { return fuel_.calculateWidth(); }
  /**
   * \brief UTM projection that this uses
   * \return UTM projection that this uses
   */
  [[nodiscard]] constexpr const string& proj4() const { return fuel_.proj4(); }

private:
  /**
   * \brief Information about fuel raster
   */
  GridBase fuel_;
  /**
   * \brief Information about elevation raster
   */
  GridBase elevation_;
  /**
   * \brief Fuel raster path
   */
  string in_fuel_;
  /**
   * \brief Elevation raster path
   */
  string in_elevation_;
  /**
   * \brief Constructor
   * \param in_fuel Fuel raster path
   * \param in_elevation Elevation raster path
   * \param fuel Information about fuel raster
   * \param elevation Information about elevation raster
   */
  EnvironmentInfo(
    const string_view in_fuel,
    const string_view in_elevation,
    GridBase&& fuel,
    GridBase&& elevation
  ) noexcept;
};
}
#endif
