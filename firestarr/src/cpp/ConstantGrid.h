/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <algorithm>
#include <string>
#include <tiffio.h>
#include <geotiffio.h>
#include <utility>
#include <vector>
#include "Grid.h"
#include "Util.h"
#include "Settings.h"
#include "FuelType.h"
namespace tbd::data
{
/**
 * \brief A GridData<T, V, const vector<T>> that cannot change once initialized.
 * \tparam T The initialization value type.
 * \tparam V The initialized value type.
 */
template <class T, class V = T>
class ConstantGrid
  : public GridData<T, V, const vector<T>>
{
public:
  /**
   * \brief Value for grid at given Location.
   * \param location Location to get value for.
   * \return Value at grid Location.
   */
  [[nodiscard]] constexpr T at(const Location& location) const noexcept override
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(location.row() >= this->rows() || location.column() >= this->columns(), "Out of bounds (%d, %d)", location.row(), location.column());
#endif
#ifdef DEBUG_POINTS
    {
      const Location loc{location.row(), location.column()};
      logging::check_equal(
        loc.column(),
        location.column(),
        "column");
      logging::check_equal(
        loc.row(),
        location.row(),
        "row");
      // if we're going to use the hash then we need to make sure it actually matches
      logging::check_equal(
        loc.hash(),
        location.hash(),
        "hash");
    }
#endif
    // return at(location.hash());
    return this->data.at(location.hash());
  }
  template <class P>
  [[nodiscard]] constexpr T at(const Position<P>& position) const noexcept
  {
    return at(Location{position.hash()});
  }
  /**
   * \brief Value for grid at given Location.
   * \param hash HashSize hash for Location to get value for.
   * \return Value at grid Location.
   */
  //  [[nodiscard]] constexpr T at(const HashSize hash) const noexcept
  //  {
  //    return this->data.at(hash);
  //  }
  /**
   * \brief Throw an error because ConstantGrid can't change values.
   */
  // ! @cond Doxygen_Suppress
  void set(const Location&, const T) override
  // ! @endcond
  {
    throw runtime_error("Cannot change ConstantGrid");
  }
  ~ConstantGrid() = default;
  ConstantGrid(const ConstantGrid& rhs) noexcept = delete;
  ConstantGrid(ConstantGrid&& rhs) noexcept = delete;
  ConstantGrid& operator=(const ConstantGrid& rhs) noexcept = delete;
  ConstantGrid& operator=(ConstantGrid&& rhs) noexcept = delete;
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param data Data to set as grid data
   */
  ConstantGrid(const MathSize cell_size,
               const Idx rows,
               const Idx columns,
               const V nodata_input,
               const T nodata_value,
               const MathSize xllcorner,
               const MathSize yllcorner,
               const MathSize xurcorner,
               const MathSize yurcorner,
               string&& proj4,
               vector<T>&& data)
    : GridData<T, V, const vector<T>>(cell_size,
                                      rows,
                                      columns,
                                      nodata_input,
                                      nodata_value,
                                      xllcorner,
                                      yllcorner,
                                      xurcorner,
                                      yurcorner,
                                      std::forward<string>(proj4),
                                      std::move(data))
  {
  }
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param initialization_value Value to initialize entire grid with
   */
  ConstantGrid(const MathSize cell_size,
               const Idx rows,
               const Idx columns,
               const V nodata_input,
               const T nodata_value,
               const MathSize xllcorner,
               const MathSize yllcorner,
               const string& proj4,
               const T& initialization_value) noexcept
    : ConstantGrid(cell_size,
                   rows,
                   columns,
                   nodata_input,
                   nodata_value,
                   xllcorner,
                   yllcorner,
                   proj4,
                   std::move(vector<T>(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS,
                                       initialization_value)))
  {
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param tif Pointer to open TIFF denoted by filename
   * \param gtif Pointer to open geotiff denoted by filename
   * \param point Point to center ConstantGrid on
   * \param convert Function taking int and nodata int value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V>* readTiff(const string& filename,
                                                    TIFF* tif,
                                                    GTIF* gtif,
                                                    const topo::Point& point,
                                                    std::function<T(V, V)> convert)
  {
    logging::info("Reading file %s", filename.c_str());
#ifdef DEBUG_GRIDS
    // auto min_value = std::numeric_limits<int16_t>::max();
    // auto max_value = std::numeric_limits<int16_t>::min();
    auto min_value = std::numeric_limits<V>::max();
    auto max_value = std::numeric_limits<V>::min();
#endif
    logging::debug("Reading a raster where T = %s, V = %s",
                   typeid(T).name(),
                   typeid(V).name());
    logging::debug("Raster type V has limits %ld, %ld",
                   std::numeric_limits<V>::min(),
                   std::numeric_limits<V>::max());
    const GridBase grid_info = read_header(tif, gtif);
    uint32_t tile_width;
    uint32_t tile_length;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_length);
    void* data;
    uint32_t count;
    TIFFGetField(tif, TIFFTAG_GDAL_NODATA, &count, &data);
    logging::check_fatal(0 == count, "NODATA value is not set in input");
    logging::debug("NODATA value is '%s'", static_cast<char*>(data));
    const auto nodata_orig = stoi(string(static_cast<char*>(data)));
    logging::debug("NODATA value is originally parsed as %d", nodata_orig);
    const auto nodata_input = static_cast<V>(stoi(string(static_cast<char*>(data))));
    logging::debug("NODATA value is parsed as %d", nodata_input);
    auto actual_rows = grid_info.calculateRows();
    auto actual_columns = grid_info.calculateColumns();
    const auto coordinates = grid_info.findFullCoordinates(point, true);
    logging::note("Coordinates before reading are (%d, %d => %f, %f)",
                  std::get<0>(*coordinates),
                  std::get<1>(*coordinates),
                  std::get<0>(*coordinates) + std::get<2>(*coordinates) / 1000.0,
                  std::get<1>(*coordinates) + std::get<3>(*coordinates) / 1000.0);
    auto min_column = max(static_cast<FullIdx>(0),
                          static_cast<FullIdx>(std::get<1>(*coordinates) - static_cast<FullIdx>(MAX_COLUMNS) / static_cast<FullIdx>(2)));
    if (min_column + MAX_COLUMNS >= actual_columns)
    {
      min_column = max(static_cast<FullIdx>(0), actual_columns - MAX_COLUMNS);
    }
    // make sure we're at the start of a tile
    const auto tile_column = tile_width * static_cast<FullIdx>(min_column / tile_width);
    const auto max_column = static_cast<FullIdx>(min(min_column + MAX_COLUMNS - 1, actual_columns));
#ifdef DEBUG_GRIDS
    logging::check_fatal(min_column < 0, "Column can't be less than 0");
    logging::check_fatal(max_column - min_column > MAX_COLUMNS, "Can't have more than %d columns", MAX_COLUMNS);
    logging::check_fatal(max_column > actual_columns, "Can't have more than actual %d columns", actual_columns);
#endif
    auto min_row = max(static_cast<FullIdx>(0),
                       static_cast<FullIdx>(std::get<0>(*coordinates) - static_cast<FullIdx>(MAX_ROWS) / static_cast<FullIdx>(2)));
    if (min_row + MAX_COLUMNS >= actual_rows)
    {
      min_row = max(static_cast<FullIdx>(0), actual_rows - MAX_ROWS);
    }
    const auto tile_row = tile_width * static_cast<FullIdx>(min_row / tile_width);
    const auto max_row = static_cast<FullIdx>(min(min_row + MAX_ROWS - 1, actual_rows));
#ifdef DEBUG_GRIDS
    logging::check_fatal(min_row < 0, "Row can't be less than 0 but is %d", min_row);
    logging::check_fatal(max_row - min_row > MAX_ROWS, "Can't have more than %d rows but have %d", MAX_ROWS, max_row - min_row);
    logging::check_fatal(max_row > actual_rows, "Can't have more than actual %d rows", actual_rows);
#endif
    T nodata_value = convert(nodata_input, nodata_input);
    logging::check_fatal(
      convert(nodata_input, nodata_input) != nodata_value,
      "Expected nodata value to be returned from convert()");
    vector<T> values(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS, nodata_value);
    logging::verbose("%s: malloc start", filename.c_str());
    int bps = std::numeric_limits<V>::digits + (1 * std::numeric_limits<V>::is_signed);
    uint16_t bps_file;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_file);
    logging::check_fatal(bps != bps_file,
                         "Raster %s type is not expected type (%d bits instead of %d)",
                         filename.c_str(),
                         bps_file,
                         bps);
#ifdef DEBUG_GRIDS
    int bps_int16_t = std::numeric_limits<int16_t>::digits + (1 * std::numeric_limits<int16_t>::is_signed);
    logging::debug("Size of pointer to int is %ld vs %ld", sizeof(int16_t*), sizeof(V*));
    logging::debug("Raster %s calculated bps for type V is %ld; tif says bps is %ld; int16_t is %ld",
                   filename.c_str(),
                   bps,
                   bps_file,
                   bps_int16_t);
#endif
    const auto tile_size = TIFFTileSize(tif);
    logging::debug("Tile size for reading %s is %ld", filename.c_str(), tile_size);
    const auto buf = _TIFFmalloc(tile_size);
    logging::verbose("%s: read start", filename.c_str());
    const tsample_t smp{};
    logging::debug("Want to clip grid to (%d, %d) => (%d, %d) for a %dx%d raster",
                   min_row,
                   min_column,
                   max_row,
                   max_column,
                   actual_rows,
                   actual_columns);
    for (auto h = tile_row; h <= max_row; h += tile_length)
    {
      for (auto w = tile_column; w <= max_column; w += tile_width)
      {
        TIFFReadTile(tif, buf, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 0, smp);
        for (
          FullIdx y = 0;
          (y < static_cast<FullIdx>(tile_length)) && (y + h <= max_row);
          ++y)
        {
          // read in so that (0, 0) has a hash of 0
          const auto y_row = static_cast<HashSize>((h - min_row) + y);
          const auto actual_row = (max_row - min_row) - y_row;
          if (actual_row >= 0 && actual_row < MAX_ROWS)
          {
            for (
              auto x = 0;
              (x < static_cast<FullIdx>(tile_width)) && (x + w <= max_column);
              ++x)
            {
              const auto offset = y * tile_width + x;
              const auto actual_column = ((w - min_column) + x);
              if (actual_column >= 0 && actual_column < MAX_ROWS)
              {
                const auto cur_hash = actual_row * MAX_COLUMNS + actual_column;
                auto cur = *(static_cast<V*>(buf) + offset);
#ifdef DEBUG_GRIDS
                min_value = min(cur, min_value);
                max_value = max(cur, max_value);
#endif
                // try
                // {
                values.at(cur_hash) = convert(cur, nodata_input);
                // logging::check_fatal(Settings::fuelLookup().values.at(cur_hash));
                // }
                // // catch (const std::out_of_range& err)
                // catch (const std::exception& err)
                // {
                //   logging::error("Error trying to read tiff");
                //   logging::debug("cur = %d", cur);
                //   logging::debug("nodata_input = %d", nodata_input);
                //   logging::debug("T = %s, V = %s", typeid(T).name(), typeid(V).name());
                //   if constexpr (std::is_same_v<T, const tbd::fuel::FuelType*>)
                //   {
                //     auto f = static_cast<const tbd::fuel::FuelType*>(values.at(cur_hash));
                //     logging::debug("fuel %s has code %d",
                //         tbd::fuel::FuelType::safeName(f),
                //         tbd::fuel::FuelType::safeCode(f));
                //   }
                //   logging::warning(err.what());
                //   // logging::fatal(err.what());
                //   tbd::sim::Settings::fuelLookup().listFuels();
                //   throw err;
                // }
              }
            }
          }
        }
      }
    }
    logging::verbose("%s: read end", filename.c_str());
    _TIFFfree(buf);
    logging::verbose("%s: free end", filename.c_str());
    const auto new_xll = grid_info.xllcorner() + (static_cast<MathSize>(min_column) * grid_info.cellSize());
    const auto new_yll = grid_info.yllcorner()
                       + (static_cast<MathSize>(actual_rows) - static_cast<MathSize>(max_row))
                           * grid_info.cellSize();
#ifdef DEBUG_GRIDS
    logging::check_fatal(new_yll < grid_info.yllcorner(),
                         "New yllcorner is outside original grid");
#endif
    logging::verbose("Translated lower left is (%f, %f) from (%f, %f)",
                     new_xll,
                     new_yll,
                     grid_info.xllcorner(),
                     grid_info.yllcorner());
    const auto num_rows = max_row - min_row + 1;
    const auto num_columns = max_column - min_column + 1;
    auto result = new ConstantGrid<T, V>(grid_info.cellSize(),
                                         num_rows,
                                         num_columns,
                                         nodata_input,
                                         nodata_value,
                                         new_xll,
                                         new_yll,
                                         new_xll + (static_cast<MathSize>(num_columns) + 1) * grid_info.cellSize(),
                                         new_yll + (static_cast<MathSize>(num_rows) + 1) * grid_info.cellSize(),
                                         string(grid_info.proj4()),
                                         std::move(values));
    auto new_location = result->findCoordinates(point, false);
#ifdef DEBUG_GRIDS
    logging::check_fatal(nullptr == new_location, "Invalid location after reading");
#endif
    logging::note("Coordinates are (%d, %d => %f, %f)",
                  std::get<0>(*new_location),
                  std::get<1>(*new_location),
                  std::get<0>(*new_location) + std::get<2>(*new_location) / 1000.0,
                  std::get<1>(*new_location) + std::get<3>(*new_location) / 1000.0);
#ifdef DEBUG_GRIDS
    logging::note("Values for %s range from %d to %d",
                  filename.c_str(),
                  min_value,
                  max_value);
#endif
    return result;
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \param convert Function taking V and nodata V value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V>* readTiff(const string& filename,
                                                    const topo::Point& point,
                                                    std::function<T(V, V)> convert)
  {
    return with_tiff<ConstantGrid<T, V>*>(
      filename,
      [&filename, &convert, &point](TIFF* tif, GTIF* gtif) { return readTiff(filename, tif, gtif, point, convert); });
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, T>* readTiff(const string& filename,
                                                    const topo::Point& point)
  {
    return readTiff(filename, point, util::no_convert<T>);
  }
protected:
  tuple<Idx, Idx, Idx, Idx> dataBounds() const override
  {
    Idx min_row = 0;
    Idx max_row = this->rows();
    Idx min_column = 0;
    Idx max_column = this->columns();
    // const Location loc_min{min_row, min_column};
    // const Location loc_max{max_row, max_column};
    // // #ifdef DEBUG_POINTS
    // logging::check_equal(
    //   loc_min.column(),
    //   min_column,
    //   "min_column");
    // logging::check_equal(
    //   loc_min.row(),
    //   min_row,
    //   "min_row");
    // logging::check_equal(
    //   loc_max.column(),
    //   max_column,
    //   "max_column");
    // logging::check_equal(
    //   loc_max.row(),
    //   max_row,
    //   "max_row");
    // // #endif
    return tuple<Idx, Idx, Idx, Idx>{
      min_column,
      min_row,
      max_column,
      max_row};
  }
private:
  /**
   * \brief Constructor
   * \param grid_info GridBase defining Grid area
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param values Values to initialize grid with
   */
  ConstantGrid(const GridBase& grid_info,
               const V nodata_input,
               const T nodata_value,
               vector<T>&& values)
    : ConstantGrid(grid_info.cellSize(),
                   nodata_input,
                   nodata_value,
                   grid_info.xllcorner(),
                   grid_info.yllcorner(),
                   grid_info.xurcorner(),
                   grid_info.yurcorner(),
                   string(grid_info.proj4()),
                   std::move(values))
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(
      this->data.size() != static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS,
      "Invalid grid size");
#endif
  }
};
}
