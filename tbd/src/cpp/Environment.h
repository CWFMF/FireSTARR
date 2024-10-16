/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "Cell.h"
#include "ConstantGrid.h"
#include "Event.h"
#include "FuelType.h"
#include "GridMap.h"
#include "IntensityMap.h"
#include "Point.h"
#include "Settings.h"

namespace tbd::topo
{
using FuelGrid = data::ConstantGrid<const fuel::FuelType*, FuelSize>;
using ElevationGrid = data::ConstantGrid<ElevationSize>;
using CellGrid = data::ConstantGrid<Cell, Topo>;
/*!
 * \page environment Fire environment
 *
 * The fuel, slope, aspect, and elevation used in simulations is consistent through
 * all simulations. These attributes are loaded from rasters at the start of the
 * process. These rasters must be in a UTM projection, and all rasters for a zone
 * must be named consistently across the different attributes. The GIS scripts
 * provided in the TBD project can generate these rasters for you.
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
   * \param dir_out Folder to save outputs to
   * \param path Folder to read rasters from
   * \param point Origin point
   * \param perimeter Perimeter to use projection from
   * \param year Year to look for rasters for if available
   * \return Environment
   */
  [[nodiscard]] static Environment loadEnvironment(
    const string dir_out,
    const string& path,
    const Point& point,
    const string& perimeter,
    int year);
  /**
   * \brief Load from rasters
   * \param point Origin point
   * \param in_fuel Fuel raster
   * \param in_elevation Elevation raster
   * \return Environment
   */
  [[nodiscard]] static Environment load(const string dir_out,
                                        const Point& point,
                                        const string& in_fuel,
                                        const string& in_elevation);
  ~Environment();
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
   * \brief Move constructor
   * \param rhs Environment to move from
   */
  Environment(Environment&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Environment to copy from
   */
  Environment(const Environment& rhs) noexcept = default;
  /**
   * \brief Move assignment
   * \param rhs Environment to move from
   * \return This, after assignment
   */
  Environment& operator=(Environment&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs Environment to copy from
   * \return This, after assignment
   */
  Environment& operator=(const Environment& rhs) noexcept = default;
  /**
   * \brief UTM projection that this uses
   * \return UTM projection that this uses
   */
  [[nodiscard]] constexpr const string& proj4() const
  {
    return cells_->proj4();
  }
  /**
   * \brief Number of rows in grid
   * \return Number of rows in grid
   */
  [[nodiscard]] constexpr Idx rows() const
  {
    return cells_->rows();
  }
  /**
   * \brief Number of columns in grid
   * \return Number of columns in grid
   */
  [[nodiscard]] constexpr Idx columns() const
  {
    return cells_->columns();
  }
  /**
   * \brief Cell width and height (m)
   * \return Cell width and height (m)
   */
  [[nodiscard]] constexpr MathSize cellSize() const
  {
    return cells_->cellSize();
  }
  /**
   * \brief Elevation of the origin Point
   * \return Elevation of the origin Point
   */
  [[nodiscard]] constexpr ElevationSize elevation() const
  {
    return elevation_;
  }
  /**
   * \brief Cell at given row and column
   * \param row Row
   * \param column Column
   * \return Cell at given row and column
   */
  [[nodiscard]]
#ifdef NDEBUG
  constexpr
#endif
    Cell
    cell(const Idx row, const Idx column) const
  {
    return cells_->at(Location(row, column));
  }
  /**
   * \brief Cell at given Location
   * \param location Location
   * \return Cell at given Location
   */
  template <class P>
  [[nodiscard]] constexpr Cell cell(const Position<P>& position) const
  {
    return cells_->at(position);
  }
  /**
   * \brief Cell at Location with given hash
   * \param hash_size Hash
   * \return Cell at Location with given hash
   */
  //  [[nodiscard]] constexpr Cell cell(const HashSize hash_size) const
  //  {
  //    return cells_->at(hash_size);
  //  }
  /**
   * \brief Cell at Location with offset of row and column from Location of Event
   * \param event Event to use for base Location
   * \param row
   * \param column
   * \return
   */
  [[nodiscard]]
#ifdef NDEBUG
  constexpr
#endif
    Cell
    offset(const sim::Event& event,
           const Idx row,
           const Idx column) const
  {
    const auto& p = event.cell();
    // return cell(p.hash() + column + static_cast<HashSize>(MAX_COLUMNS) * row);
    return cell(Location(p.row() + row, p.column() + column));
  }
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
  [[nodiscard]] sim::ProbabilityMap* makeProbabilityMap(DurationSize time,
                                                        DurationSize start_time,
                                                        int min_value,
                                                        int low_max,
                                                        int med_max,
                                                        int max_value) const;
  /**
   * \brief Create a GridMap<Other> covering this Environment
   * \tparam Other Type of GridMap
   * \param nodata Value that represents no data
   * \return GridMap<Other> covering this Environment
   */
  template <class Other>
  [[nodiscard]] unique_ptr<data::GridMap<Other>> makeMap(const Other nodata) const
  {
    return make_unique<data::GridMap<Other>>(*cells_, nodata);
  }
  /**
   * \brief Create BurnedData and set burned bits based on Perimeter
   * \return BurnedData with all initially burned locations set
   */
  [[nodiscard]] unique_ptr<sim::BurnedData> makeBurnedData() const
  {
    auto result = make_unique<sim::BurnedData>(*not_burnable_);
    return result;
  }
  /**
   * \brief Reset with known non-fuel cells
   * \param data BurnedData to reset
   */
  void resetBurnedData(sim::BurnedData* data) const noexcept
  {
    // setting to {} probably makes a bitset of the same size on the stack?
    // *data = {};
    *data = *not_burnable_;
  }
protected:
  /**
   * \brief Combine rasters into ConstantGrid<Cell, Topo>
   * \param elevation Elevation raster
   * \return
   */
  [[nodiscard]] static CellGrid* makeCells(
    const FuelGrid& fuel,
    const ElevationGrid& elevation)
  {
    logging::check_equal(fuel.yllcorner(), elevation.yllcorner(), "yllcorner");
    static Cell nodata{};
    auto values = vector<Cell>{fuel.data.size()};
    vector<HashSize> hashes{};
    //    for (HashSize h = 0; h < static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS; ++h)
    //    {
    //      hashes.emplace_back(h);
    //    }
    for (Idx r = 0; r < fuel.rows(); ++r)
    {
      for (Idx c = 0; c < fuel.columns(); ++c)
      {
        const auto h = hashes.emplace_back(Location(r, c).hash());
        //        hashes.emplace_back(Location(r, c).hash());
        //      }
        //    }
        //    std::for_each(
        //      std::execution::par_unseq,
        //      hashes.begin(),
        //      hashes.end(),
        //      [&fuel, &values, &elevation](auto&& h)
        //      {
        const topo::Location loc{r, c, h};
        //        const topo::Location loc{static_cast<Idx>(h / MAX_COLUMNS),
        //                                 static_cast<Idx>(h % MAX_COLUMNS),
        //                                 h};
        //        const auto r = loc.row();
        //        const auto c = loc.column();
        //        const auto f = fuel::FuelType::safeCode(fuel.at(h));
        if (r >= 0 && r < fuel.rows() && c >= 0 && c < fuel.columns())
        {
          // NOTE: this needs to translate to internal codes?
          const auto f = fuel::FuelType::safeCode(fuel.at(loc));
          auto s = static_cast<SlopeSize>(INVALID_SLOPE);
          auto a = static_cast<AspectSize>(INVALID_ASPECT);
          // HACK: don't calculate for outside box of cells
          if (r > 0 && r < fuel.rows() - 1 && c > 0 && c < fuel.columns() - 1)
          {
            MathSize dem[9];
            bool valid = true;
            for (int i = -1; i < 2; ++i)
            {
              for (int j = -1; j < 2; ++j)
              {
                // grid is (0, 0) at bottom left, but want [0] in array to be NW corner
                auto actual_row = static_cast<Idx>(r - i);
                auto actual_column = static_cast<Idx>(c + j);
                auto cur_loc = Location{actual_row, actual_column};
                //                auto cur_h = cur_loc.hash();
                //              dem[3 * (i + 1) + (j + 1)] = 1.0 * elevation.at(cur_h);
                const auto v = elevation.at(cur_loc);
                // can't calculate slope & aspect if any surrounding cell is nodata
                if (elevation.nodataValue() == v)
                {
                  valid = false;
                  break;
                }
                dem[3 * (i + 1) + (j + 1)] = 1.0 * v;
              }
              if (!valid)
              {
                break;
              }
            }
            if (valid)
            {
              // Horn's algorithm
              const MathSize dx = ((dem[2] + dem[5] + dem[5] + dem[8])
                                   - (dem[0] + dem[3] + dem[3] + dem[6]))
                                / elevation.cellSize();
              const MathSize dy = ((dem[6] + dem[7] + dem[7] + dem[8])
                                   - (dem[0] + dem[1] + dem[1] + dem[2]))
                                / elevation.cellSize();
              const MathSize key = (dx * dx + dy * dy);
              auto slope_pct = static_cast<float>(100 * (sqrt(key) / 8.0));
              s = min(static_cast<SlopeSize>(MAX_SLOPE_FOR_DISTANCE), static_cast<SlopeSize>(round(slope_pct)));
              static_assert(std::numeric_limits<SlopeSize>::max() >= MAX_SLOPE_FOR_DISTANCE);
              MathSize aspect_azimuth = 0.0;

              if (s > 0 && (dx != 0 || dy != 0))
              {
                aspect_azimuth = atan2(dy, -dx) * M_RADIANS_TO_DEGREES;
                // NOTE: need to change this out of 'math' direction into 'real' direction (i.e. N is 0, not E)
                aspect_azimuth = (aspect_azimuth > 90.0) ? (450.0 - aspect_azimuth) : (90.0 - aspect_azimuth);
                if (aspect_azimuth == 360.0)
                {
                  aspect_azimuth = 0.0;
                }
              }

              a = static_cast<AspectSize>(round(aspect_azimuth));
            }
          }
          const auto cell = Cell{h, s, a, f};
          values.at(h) = cell;
#ifdef DEBUG_GRIDS
#ifndef VLD_RPTHOOK_INSTALL
          logging::check_fatal(cell.row() != r, "Cell row %d not %d", cell.row(), r);
          logging::check_fatal(cell.column() != c, "Cell column %d not %d", cell.column(), c);
          logging::check_fatal(cell.slope() != s, "Cell slope %d not %d", cell.slope(), s);
          logging::check_fatal(cell.aspect() != a, "Cell aspect %d not %d", cell.aspect(), a);
          logging::check_fatal(cell.fuelCode() != f, "Cell fuel %d not %d", cell.fuelCode(), f);
          const auto v = values.at(h);
          logging::check_fatal(v.row() != r, "Row %d not %d", v.row(), r);
          logging::check_fatal(v.column() != c, "Column %d not %d", v.column(), c);
          logging::check_fatal(v.slope() != s, "Slope %d not %d", v.slope(), s);
          if (0 != s)
          {
            logging::check_fatal(v.aspect() != a, "Aspect %d not %d", v.aspect(), a);
          }
          else
          {
            logging::check_fatal(v.aspect() != 0, "Aspect %d not %d", v.aspect(), 0);
          }
          logging::check_fatal(v.fuelCode() != f, "Fuel %d not %d", v.fuelCode(), f);
#endif
#endif
        }
        //      });
      }
    }
    return new topo::CellGrid(
      fuel.cellSize(),
      fuel.rows(),
      fuel.columns(),
      nodata.fullHash(),
      nodata,
      fuel.xllcorner(),
      fuel.yllcorner(),
      fuel.xurcorner(),
      fuel.yurcorner(),
      string(fuel.proj4()),
      std::move(values));
  }
  /**
   * \brief Creates a map of areas that are not burnable either because of fuel or the initial perimeter.
   */
  shared_ptr<sim::BurnedData> initializeNotBurnable(const CellGrid& cells) const
  {
    // shared_ptr<sim::BurnedData> result{};
    //     std::fill(not_burnable_.begin(), not_burnable_.end(), false);
    //  make a template we can copy to reset things
    auto result = make_shared<sim::BurnedData>();
    for (Idx r = 0; r < rows(); ++r)
    {
      for (Idx c = 0; c < columns(); ++c)
      {
        const Location location(r, c);
        // HACK: just mark outside edge as unburnable so we never need to check
        bool is_outer = 0 == r
                     || 0 == c
                     || (rows() - 1) == r
                     || (columns() - 1) == c;
        // (*result)[location.hash()] = (nullptr == fuel::fuel_by_code(cells.at(location).fuelCode()));
        (*result)[location.hash()] = is_outer || fuel::is_null_fuel(cells.at(location));
        //        if (fuel::is_null_fuel(cell(location)))
        //        {
        //          not_burnable_[location.hash()] = true;
        //        }
      }
    }
    return result;
  }
  /**
   * \brief Construct from cells and elevation
   * \param cells Cells representing Environment
   * \param elevation Elevation at origin Point
   */
  Environment(const string dir_out,
              CellGrid* cells,
              const ElevationSize elevation) noexcept
    : dir_out_(dir_out),
      cells_(cells),
      not_burnable_(initializeNotBurnable(*cells)),
      elevation_(elevation)
  {
    //    try
    //    {
    //      initializeNotBurnable();
    //    }
    //    catch (...)
    //    {
    //      std::terminate();
    //    }
  }
  /**
   * \brief Load from rasters
   * \param fuel Fuel raster
   * \param elevation Elevation raster
   */
  Environment(
    const string dir_out,
    const FuelGrid& fuel,
    const ElevationGrid& elevation,
    const Point& point)
    : Environment(dir_out,
                  makeCells(fuel,
                            elevation),
                  elevation.at(Location(*elevation.findCoordinates(point, false).get())))
  {
    // take elevation at point so that if max grid size changes elevation doesn't
    logging::note("Start elevation is %d", elevation_);
    if (sim::Settings::saveSimulationArea())
    {
      logging::debug("Saving simulation area");
      const auto lookup = sim::Settings::fuelLookup();
      auto convert_to_slope =
        [&elevation](
          const Cell& v) -> SlopeSize {
        return v.slope();
      };
      auto convert_to_aspect =
        [&elevation](
          const Cell& v) -> AspectSize {
        return v.aspect();
      };
      auto convert_to_area =
        [&elevation](
          const ElevationSize v) -> ElevationSize {
        // need to still be nodata if it was
        return (v == elevation.nodataValue()) ? v : 3;
      };
      auto convert_to_fuelcode =
        [&lookup](
          const fuel::FuelType* const value) -> FuelSize {
        return lookup.fuelToCode(value);
      };
      fuel.saveToFile<FuelSize>(
        dir_out_,
        "fuel",
        convert_to_fuelcode);
      elevation.saveToFile(
        dir_out_,
        "dem");
      // save slope & aspect grids
      cells_->saveToFile<SlopeSize>(
        dir_out_,
        "slope",
        convert_to_slope,
        static_cast<SlopeSize>(INVALID_SLOPE));
      cells_->saveToFile<AspectSize>(
        dir_out_,
        "aspect",
        convert_to_aspect,
        static_cast<AspectSize>(INVALID_ASPECT));
      // HACK: make a grid with "3" as the value so if we merge max with it it'll cover up anything else
      elevation.saveToFile<ElevationSize>(
        dir_out_,
        "simulation_area",
        convert_to_area);
      logging::debug("Done saving fuel grid");
    }
  }
private:
  const string dir_out_;
  /**
   * \brief Cells representing Environment
   */
  CellGrid* cells_;
  /**
   * \brief BurnedData of cells that are not burnable
   */
  shared_ptr<const sim::BurnedData> not_burnable_;
  /**
   * \brief Elevation at StartPoint
   */
  ElevationSize elevation_;
};
}
