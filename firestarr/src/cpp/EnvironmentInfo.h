/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <memory>
#include <string>
#include "Environment.h"
#include "Grid.h"
namespace tbd::topo
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
  [[nodiscard]] static unique_ptr<EnvironmentInfo> loadInfo(const string& in_fuel,
                                                            const string& in_elevation);
  ~EnvironmentInfo();
  /**
   * \brief Construct from given rasters
   * \param in_fuel Fuel raster
   * \param in_elevation Elevation raster
   */
  EnvironmentInfo(const string& in_fuel,
                  const string& in_elevation);
  /**
   * \brief Move constructor
   * \param rhs EnvironmentInfo to move from
   */
  EnvironmentInfo(EnvironmentInfo&& rhs) noexcept = default;
  EnvironmentInfo(const EnvironmentInfo& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs EnvironmentInfo to move from
   * \return This, after assignment
   */
  EnvironmentInfo& operator=(EnvironmentInfo&& rhs) noexcept = default;
  EnvironmentInfo& operator=(const EnvironmentInfo& rhs) = delete;
  /**
   * \brief Determine Coordinates in the grid for the Point
   * \param point Point to find Coordinates for
   * \param flipped Whether the grid data is flipped across the horizontal axis
   * \return Coordinates that would be at Point within this EnvironmentInfo, or nullptr if it is not
   */
  [[nodiscard]] unique_ptr<Coordinates> findCoordinates(
    const Point& point,
    bool flipped) const;
  /**
   * \brief Determine FullCoordinates in the grid for the Point
   * \param point Point to find FullCoordinates for
   * \param flipped Whether the grid data is flipped across the horizontal axis
   * \return Coordinates that would be at Point within this EnvironmentInfo, or nullptr if it is not
   */
  [[nodiscard]] unique_ptr<FullCoordinates> findFullCoordinates(
    const Point& point,
    bool flipped) const;
  /**
   * \brief Load the full Environment using the given FuelLookup to determine fuels
   * \param dir_out Folder to save outputs to
   * \param point Origin Point
   * \return
   */
  [[nodiscard]] Environment load(const string dir_out,
                                 const Point& point) const;
  /**
   * \brief Number of rows in grid
   * \return Number of rows in grid
   */
  [[nodiscard]] constexpr FullIdx calculateRows() const
  {
    return fuel_.calculateRows();
  }
  /**
   * \brief Number of columns in grid
   * \return Number of columns in grid
   */
  [[nodiscard]] constexpr FullIdx calculateColumns() const
  {
    return fuel_.calculateColumns();
  }
  /**
   * \brief UTM projection that this uses
   * \return UTM projection that this uses
   */
  [[nodiscard]] constexpr const string& proj4() const
  {
    return fuel_.proj4();
  }
private:
  /**
   * \brief Information about fuel raster
   */
  data::GridBase fuel_;
  /**
   * \brief Information about elevation raster
   */
  data::GridBase elevation_;
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
  EnvironmentInfo(string in_fuel,
                  string in_elevation,
                  data::GridBase&& fuel,
                  data::GridBase&& elevation) noexcept;
};
}
