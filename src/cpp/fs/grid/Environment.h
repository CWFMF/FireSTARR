/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_ENVIRONMENT_H
#define FS_ENVIRONMENT_H
#include "../stdafx.h"
#include "../geo/Point.h"
#include "../sim/Event.h"
#include "BurnedData.h"
#include "Cell.h"
#include "ConstantGrid.h"
#include "GridMap.h"
#include "IntensityMap.h"
#include "ProbabilityMap.h"
namespace fs
{
/*!
 * \page environment Fire environment
 *
 * The fuel, slope, aspect, and elevation used in simulations is consistent through
 * all simulations. These attributes are loaded from rasters at the start of the
 * process. These rasters must be in a UTM projection, and all rasters for a zone
 * must be named consistently across the different attributes. The GIS scripts
 * provided in the FireSTARR project can generate these rasters for you.
 *
 * Elevation is only read at the ignition point and it is assumed that elevation
 * is the same wherever it is used in a calculation. Despite this, slope and aspect
 * are used for calculations in each cell, and it is only where elevation is
 * specifically used in a formula that this value is referenced.
 *
 * Fuel requires a .lut lookup table file, in the same format that Prometheus
 * expects.
 *
 * Grass curing and leaf-on/off are determined based on time of year, as described
 * elsewhere.
 *
 * Weather and ignition point(s) are the only things that vary between simulations
 * at this point.
 */
/**
 * \brief The area that a Model is run for, with Fuel, Slope, and Aspect grids.
 */
class Environment
{
public:
  /**
   * \brief Load from rasters in folder that have same projection as Perimeter
   * \param path Folder to read rasters from
   * \param point Origin point
   * \param perimeter Perimeter to use projection from
   * \param year Year to look for rasters for if available
   * \return Environment
   */
  [[nodiscard]] static Environment loadEnvironment(
    const string_view path,
    const Point& point,
    const LazyPath& perimeter,
    YearSize year
  );
  /**
   * \brief Load from rasters
   * \param point Origin point
   * \param in_fuel Fuel raster
   * \param in_elevation Elevation raster
   * \return Environment
   */
  [[nodiscard]] static Environment load(
    const Point& point,
    const string_view in_fuel,
    const string_view in_elevation
  );
  /**
   * \brief Determine Coordinates in the grid for the Point
   * \param point Point to find Coordinates for
   * \param flipped Whether the grid data is flipped across the horizontal axis
   * \return Coordinates that would be at Point within this EnvironmentInfo, or nullptr if it is not
   */
  [[nodiscard]] std::optional<Coordinates> findCoordinates(const Point& point, bool flipped) const;
  /**
   * \brief UTM projection that this uses
   * \return UTM projection that this uses
   */
  [[nodiscard]] constexpr string_view proj4() const { return cells_.proj4(); }
  [[nodiscard]] constexpr Idx height() const { return cells_.height(); }
  [[nodiscard]] constexpr Idx width() const { return cells_.width(); }
  /**
   * \brief Cell width and height (m)
   * \return Cell width and height (m)
   */
  [[nodiscard]] constexpr MathSize cellSize() const { return cells_.cellSize(); };
  /**
   * \brief Elevation of the origin Point
   * \return Elevation of the origin Point
   */
  [[nodiscard]] constexpr ElevationSize elevation() const { return elevation_; };
  [[nodiscard]] Cell cell(const XYIdx& xy) const { return cells_.at(xy); }
  [[nodiscard]] Cell offset(const Event& event, const Idx x, const Idx y) const;
  /**
   * \brief Make a ProbabilityMap that covers this Environment
   * \param time Time in simulation this ProbabilityMap represents
   * \param start_time Start time of simulation
   * \param min_value Lower bound of 'low' intensity range
   * \param low_max Upper bound of 'low' intensity range
   * \param med_max Upper bound of 'moderate' intensity range
   * \param max_value Upper bound of 'high' intensity range
   * \return ProbabilityMap with the same extent as this
   */
  [[nodiscard]] shared_ptr<ProbabilityMap> makeProbabilityMap(
    DurationSize time,
    DurationSize start_time,
    int min_value,
    int low_max,
    int med_max,
    int max_value,
    const shared_ptr<Perimeter> perimeter
  ) const;
  /**
   * \brief Create a GridMap<Other> covering this Environment
   * \tparam Other Type of GridMap
   * \param nodata Value that represents no data
   * \return GridMap<Other> covering this Environment
   */
  template <class Other>
  [[nodiscard]] GridMap<Other> makeMap(const Other nodata) const
  {
    return GridMap<Other>(cells_, nodata);
  }
  const BurnedData& unburnable() const;

protected:
  /**
   * \brief Combine rasters into CellGrid
   * \param elevation Elevation raster
   * \return
   */
  [[nodiscard]] static CellGrid makeCells(const FuelGrid& fuel, const ElevationGrid& elevation);
  /**
   * \brief Construct from cells and elevation
   * \param cells Cells representing Environment
   * \param elevation Elevation at origin Point
   */
  Environment(
    unique_ptr<FuelGrid> fuel_grid,
    unique_ptr<ElevationGrid> elevation_grid,
    CellGrid&& cells,
    const ElevationSize elevation
  ) noexcept;
  /**
   * \brief Load from rasters
   * \param fuel Fuel raster
   * \param elevation Elevation raster
   */
  Environment(const FuelGrid& fuel, const ElevationGrid& elevation, const Point& point);
  Environment(
    const FuelGrid& fuel,
    const ElevationGrid& elevation,
    const Point& point,
    const Settings& settings
  );

public:
  void saveToFile(const string_view output_directory) const;
  /*
   * \brief Convert number of cells to size in hectares
   * \return Size in hectares for given number of cells
   */
  MathSize to_hectares(const size_t num_cells) const;

private:
  /**
   * \brief Fuel grid (if saving simulation area)
   */
  unique_ptr<FuelGrid> fuel_grid_{};
  /**
   * \brief Elevation grid (if saving simulation area)
   */
  unique_ptr<ElevationGrid> elevation_grid_{};
  /**
   * \brief Cells representing Environment
   */
  CellGrid cells_{};
  /**
   * \brief Cells that are not burnable
   */
  BurnedData not_burnable_{};
  /**
   * \brief Elevation at StartPoint
   */
  ElevationSize elevation_{INVALID_ELEVATION};
};
}
#endif
