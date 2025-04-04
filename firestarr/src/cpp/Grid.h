/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include "Location.h"
#include "Cell.h"
#include "Log.h"
#include "Point.h"
#include "Settings.h"

using fs::topo::Location;
using fs::topo::Position;
using NodataIntType = int64_t;
/**
 * \brief Provides hash function for Location.
 */
template <>
struct std::hash<Location>
{
  /**
   * \brief Get hash value for a Location
   * \param location Location to get value for
   * \return Hash value for a Location
   */
  [[nodiscard]] constexpr fs::HashSize operator()(
    const Location& location) const noexcept
  {
    return location.hash();
  }
};
namespace fs::data
{
/**
 * \brief The base class with information for a grid of data with geographic coordinates.
 */
class GridBase
{
public:
  virtual ~GridBase() = default;
  /**
   * \brief Move constructor
   * \param rhs GridBase to move from
   */
  GridBase(GridBase&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs GridBase to copy from
   */
  GridBase(const GridBase& rhs) = default;
  /**
   * \brief Copy assignment
   * \param rhs GridBase to copy from
   * \return This, after assignment
   */
  GridBase& operator=(const GridBase& rhs) = default;
  /**
   * \brief Move assignment
   * \param rhs GridBase to move from
   * \return This, after assignment
   */
  GridBase& operator=(GridBase&& rhs) noexcept = default;
  /**
   * \brief Cell size used for GridBase.
   * \return Cell height and width in meters.
   */
  [[nodiscard]] constexpr MathSize cellSize() const noexcept
  {
    return cell_size_;
  }
  /**
   * \brief Number of rows in the GridBase.
   * \return Number of rows in the GridBase.
   */
  [[nodiscard]] constexpr FullIdx calculateRows() const noexcept
  {
    return static_cast<FullIdx>((yurcorner() - yllcorner()) / cellSize()) - 1;
    // // HACK: just get rid of -1 for now because it seems weird
    // return static_cast<FullIdx>((yurcorner() - yllcorner()) / cellSize());
  }
  /**
   * \brief Number of columns in the GridBase.
   * \return Number of columns in the GridBase.
   */
  [[nodiscard]] constexpr FullIdx calculateColumns() const noexcept
  {
    return static_cast<FullIdx>((xurcorner() - xllcorner()) / cellSize()) - 1;
    // // HACK: just get rid of -1 for now because it seems weird
    // return static_cast<FullIdx>((xurcorner() - xllcorner()) / cellSize());
  }
  /**
   * \brief Lower left corner X coordinate in meters.
   * \return Lower left corner X coordinate in meters.
   */
  [[nodiscard]] constexpr MathSize xllcorner() const noexcept
  {
    return xllcorner_;
  }
  /**
   * \brief Lower left corner Y coordinate in meters.
   * \return Lower left corner Y coordinate in meters.
   */
  [[nodiscard]] constexpr MathSize yllcorner() const noexcept
  {
    return yllcorner_;
  }
  /**
   * \brief Upper right corner X coordinate in meters.
   * \return Upper right corner X coordinate in meters.
   */
  [[nodiscard]] constexpr MathSize xurcorner() const noexcept
  {
    return xurcorner_;
  }
  /**
   * \brief Upper right corner Y coordinate in meters.
   * \return Upper right corner Y coordinate in meters.
   */
  [[nodiscard]] constexpr MathSize yurcorner() const noexcept
  {
    return yurcorner_;
  }
  /**
   * \brief Proj4 string defining coordinate system for this grid. Must be a UTM projection.
   * \return Proj4 string defining coordinate system for this grid.
   */
  [[nodiscard]] constexpr const string& proj4() const noexcept
  {
    return proj4_;
  }
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   */
  GridBase(MathSize cell_size,
           MathSize xllcorner,
           MathSize yllcorner,
           MathSize xurcorner,
           MathSize yurcorner,
           string&& proj4) noexcept;
  /**
   * \brief Default constructor
   */
  GridBase() noexcept;
  /**
   * \brief Create .prj file in directory with base name for file
   * \param dir Directory to create in
   * \param base_name base file name for .prj file
   */
  void createPrj(const string& dir, const string& base_name) const;
  /**
   * \brief Find Coordinates for Point
   * \param point Point to translate to Grid Coordinate
   * \param flipped Whether or not Grid data is flipped along x axis
   * \return Coordinates for Point translated to Grid
   */
  [[nodiscard]] unique_ptr<Coordinates> findCoordinates(
    const topo::Point& point,
    bool flipped) const;
  /**
   * \brief Find FullCoordinates for Point
   * \param point Point to translate to Grid Coordinate
   * \param flipped Whether or not Grid data is flipped along x axis
   * \return Coordinates for Point translated to Grid
   */
  [[nodiscard]] unique_ptr<FullCoordinates> findFullCoordinates(
    const topo::Point& point,
    bool flipped) const;
private:
  /**
   * \brief Proj4 string defining projection.
   */
  string proj4_;
  /**
   * \brief Cell height and width in meters.
   */
  MathSize cell_size_;
  /**
   * \brief Lower left corner X coordinate in meters.
   */
  MathSize xllcorner_;
  /**
   * \brief Lower left corner Y coordinate in meters.
   */
  MathSize yllcorner_;
  /**
   * \brief Upper right corner X coordinate in meters.
   */
  MathSize xurcorner_;
  /**
   * \brief Upper right corner Y coordinate in meters.
   */
  MathSize yurcorner_;
};
void write_ascii_header(ofstream& out,
                        MathSize num_columns,
                        MathSize num_rows,
                        MathSize xll,
                        MathSize yll,
                        MathSize cell_size,
                        MathSize no_data);
template <class R>
[[nodiscard]] R with_tiff(const string& filename, function<R(TIFF*, GTIF*)> fct)
{
  logging::debug("Reading file %s", filename.c_str());
  // suppress warnings about geotiff tags that aren't found
  TIFFSetWarningHandler(nullptr);
  auto tif = GeoTiffOpen(filename.c_str(), "r");
  logging::check_fatal(!tif, "Cannot open file %s as a TIF", filename.c_str());
  auto gtif = GTIFNew(tif);
  logging::check_fatal(!gtif, "Cannot open file %s as a GEOTIFF", filename.c_str());
  //  try
  //  {
  R result = fct(tif, gtif);
  if (tif)
  {
    XTIFFClose(tif);
  }
  if (gtif)
  {
    GTIFFree(gtif);
  }
  GTIFDeaccessCSV();
  return result;
  //  }
  //  catch (std::exception&)
  //  {
  //    return logging::fatal<R>("Unable to process file %s", filename.c_str());
  //  }
}
GridBase read_header(TIFF* tif, GTIF* gtif);
GridBase read_header(const string& filename);
/**
 * \brief A GridBase with an associated type of data.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 */
template <class T, class V = T>
class Grid
  : public GridBase
{
public:
  /**
   * \brief Number of rows in the GridBase.
   * \return Number of rows in the GridBase.
   */
  [[nodiscard]] constexpr Idx rows() const noexcept
  {
    return rows_;
  }
  /**
   * \brief Number of columns in the GridBase.
   * \return Number of columns in the GridBase.
   */
  [[nodiscard]] constexpr Idx columns() const noexcept
  {
    return columns_;
  }
  /**
   * \brief Value used for grid locations that have no data.
   * \return Value used for grid locations that have no data.
   */
  [[nodiscard]] constexpr V nodataInput() const noexcept
  {
    return nodata_input_;
  }
  /**
   * \brief Value representing no data
   * \return Value representing no data
   */
  constexpr T nodataValue() const noexcept
  {
    return nodata_value_;
  }   // NOTE: only use this for simple types because it's returning by value
  /**
   * \brief Value for grid at given Location.
   * \param location Location to get value for.
   * \return Value at grid Location.
   */
  [[nodiscard]] virtual T at(const Location& location) const = 0;
  template <class P>
  [[nodiscard]] T at(const Position<P>& position) const
  {
    return at(Location{position.hash()});
  }
  // NOTE: use set instead of at to avoid issues with bool
  /**
   * \brief Set value for grid at given Location.
   * \param location Location to set value for.
   * \param value Value to set at grid Location.
   * \return None
   */
  virtual void set(const Location& location, T value) = 0;
  template <class P>
  void set(const Position<P>& position, const T value)
  {
    set(Location{position.hash()});
  }
protected:
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   */
  Grid(const MathSize cell_size,
       const Idx rows,
       const Idx columns,
       const V nodata_input,
       const T nodata_value,
       const MathSize xllcorner,
       const MathSize yllcorner,
       const MathSize xurcorner,
       const MathSize yurcorner,
       string&& proj4) noexcept
    : GridBase(cell_size,
               xllcorner,
               yllcorner,
               xurcorner,
               yurcorner,
               std::forward<string>(proj4)),
      nodata_input_(nodata_input),
      nodata_value_(nodata_value),
      rows_(rows),
      columns_(columns)
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(rows > MAX_ROWS, "Too many rows (%d > %d)", rows, MAX_ROWS);
    logging::check_fatal(columns > MAX_COLUMNS, "Too many columns (%d > %d)", columns, MAX_COLUMNS);
#endif
#ifdef DEBUG_GRIDS
    // enforce converting to an int and back produces same V
    const auto n0 = this->nodata_input_;
    const auto n1 = static_cast<NodataIntType>(n0);
    const auto n2 = static_cast<V>(n1);
    const auto n3 = static_cast<NodataIntType>(n2);
    logging::check_equal(
      n1,
      n3,
      "nodata_input_ as int");
    logging::check_equal(
      n0,
      n2,
      "nodata_input_ from int");
#endif
  }
  /**
   * \brief Construct based on GridBase and no data value
   * \param grid_info GridBase defining Grid area
   * \param no_data Value that represents no data
   */
  Grid(const GridBase& grid_info, V no_data) noexcept
    : Grid(grid_info.cellSize(),
           static_cast<Idx>(grid_info.calculateRows()),
           static_cast<Idx>(grid_info.calculateColumns()),
           no_data,
           to_string(no_data),
           grid_info.xllcorner(),
           grid_info.yllcorner(),
           grid_info.xurcorner(),
           grid_info.yurcorner(),
           grid_info.proj4())
  {
  }
private:
  /**
   * \brief Value used to represent no data at a Location.
   */
  V nodata_input_;
  /**
   * \brief Value to use for representing no data at a Location.
   */
  T nodata_value_;
  /**
   * \brief Number of rows in the grid.
   */
  Idx rows_{};
  /**
   * \brief Number of columns in the grid.
   */
  Idx columns_{};
};
/**
 * \brief A Grid that defines the data structure used for storing values.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 * \tparam D The data type that stores the values.
 */
template <class T, class V, class D>
class GridData
  : public Grid<T, V>
{
public:
  using Grid<T, V>::Grid;
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
   * \param data Data to populate GridData with
   */
  GridData(const MathSize cell_size,
           const Idx rows,
           const Idx columns,
           const V nodata_input,
           const T nodata_value,
           const MathSize xllcorner,
           const MathSize yllcorner,
           const MathSize xurcorner,
           const MathSize yurcorner,
           string&& proj4,
           D&& data)
    : Grid<T, V>(cell_size,
                 rows,
                 columns,
                 nodata_input,
                 nodata_value,
                 xllcorner,
                 yllcorner,
                 xurcorner,
                 yurcorner,
                 std::forward<string>(proj4)),
      data(std::forward<D>(data))
  {
  }
  ~GridData() = default;
  /**
   * \brief Copy constructor
   * \param rhs GridData to copy from
   */
  GridData(const GridData& rhs)
    : Grid<T, V>(rhs), data(rhs.data)
  {
  }
  /**
   * \brief Move constructor
   * \param rhs GridData to move from
   */
  GridData(GridData&& rhs) noexcept
    : Grid<T, V>(rhs), data(std::move(rhs.data))
  {
  }
  /**
   * \brief Copy assignment
   * \param rhs GridData to copy from
   * \return This, after assignment
   */
  GridData& operator=(const GridData& rhs) noexcept
  {
    if (this != &rhs)
    {
      Grid<T, V>::operator=(rhs);
      data = rhs.data;
    }
    return *this;
  }
  /**
   * \brief Move assignment
   * \param rhs GridData to copy from
   * \return This, after assignment
   */
  GridData& operator=(GridData&& rhs) noexcept
  {
    if (this != &rhs)
    {
      Grid<T, V>::operator=(rhs);
      data = std::move(rhs.data);
    }
    return *this;
  }
  /**
   * \brief Size of data structure storing values
   * \return Size of data structure storing values
   */
  [[nodiscard]] size_t size() const
  {
    return data.size();
  }
  // HACK: use public access so that we can get to the keys
  /**
   * \brief Structure that holds data represented by this GridData
   */
  D data;
protected:
  virtual tuple<Idx, Idx, Idx, Idx> dataBounds() const = 0;
  /**
   * \brief Save GridMap contents to .asc file
   * \tparam R Type to be written to .asc file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   * \param no_data Value to use as nodata in output
   */
  template <class R>
  string saveToAsciiFile(const string& dir,
                         const string& base_name,
                         std::function<R(T value)> convert,
                         const R no_data) const
  {
    tuple<Idx, Idx, Idx, Idx> bounds = dataBounds();
    auto min_column = std::get<0>(bounds);
    auto min_row = std::get<1>(bounds);
    auto max_column = std::get<2>(bounds);
    auto max_row = std::get<3>(bounds);
#ifdef DEBUG_GRIDS
    logging::debug(
      "Bounds are (%d, %d), (%d, %d)",
      min_column,
      min_row,
      max_column,
      max_row);
#endif
    logging::extensive("Lower left corner is (%d, %d)", min_column, min_row);
    logging::extensive("Upper right corner is (%d, %d)", max_column, max_row);
    const MathSize xll = this->xllcorner() + min_column * this->cellSize();
    // offset is different for y since it's flipped
    const MathSize yll = this->yllcorner() + (min_row) * this->cellSize();
    logging::extensive("Lower left corner is (%f, %f)", xll, yll);
    // HACK: make sure it's always at least 1
    const auto num_rows = static_cast<MathSize>(max_row) - min_row + 1;
    const auto num_columns = static_cast<MathSize>(max_column) - min_column + 1;
    ofstream out;
    string filename = dir + base_name + ".asc";
    out.open(filename.c_str());
    write_ascii_header(
      out,
      num_columns,
      num_rows,
      xll,
      yll,
      this->cellSize(),
      static_cast<MathSize>(no_data));
    for (Idx ro = 0; ro < num_rows; ++ro)
    {
      // HACK: do this so that we always get at least one pixel in output
      // need to output in reverse order since (0,0) is bottom left
      const Idx r = static_cast<Idx>(max_row) - ro;
      for (Idx co = 0; co < num_columns; ++co)
      {
        const Location idx(static_cast<Idx>(r), static_cast<Idx>(min_column + co));
        // HACK: use + here so that it gets promoted to a printable number
        //       prevents char type being output as characters
        out << +(convert(this->at(idx)))
            << " ";
      }
      out << "\n";
    }
    out.close();
    this->createPrj(dir, base_name);
    return filename;
  }
  /**
   * \brief Save GridMap contents to .tif file
   * \tparam R Type to be written to .tif file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   */
  template <class R>
  string saveToTiffFile(const string& dir,
                        const string& base_name,
                        std::function<R(T value)> convert,
                        const R no_data) const
  {
    uint32_t tileWidth = min((int)(this->columns()), 256);
    uint32_t tileHeight = min((int)(this->rows()), 256);
    tuple<Idx, Idx, Idx, Idx> bounds = dataBounds();
    auto min_column = std::get<0>(bounds);
    auto min_row = std::get<1>(bounds);
    auto max_column = std::get<2>(bounds);
    auto max_row = std::get<3>(bounds);
    logging::check_fatal(
      min_column > max_column,
      "Invalid bounds for columns with %d => %d",
      min_column,
      max_column);
    logging::check_fatal(
      min_row > max_row,
      "Invalid bounds for rows with %d => %d",
      min_row,
      max_row);
#ifdef DEBUG_GRIDS
    logging::debug(
      "Bounds are (%d, %d), (%d, %d) initially",
      min_column,
      min_row,
      max_column,
      max_row);
#endif
    Idx c_min = 0;
    while (c_min + static_cast<Idx>(tileWidth) <= min_column)
    {
      c_min += static_cast<Idx>(tileWidth);
    }
    Idx c_max = c_min + static_cast<Idx>(tileWidth);
    while (c_max < max_column)
    {
      c_max += static_cast<Idx>(tileWidth);
    }
    min_column = c_min;
    max_column = c_max;
    Idx r_min = 0;
    while (r_min + static_cast<Idx>(tileHeight) <= min_row)
    {
      r_min += static_cast<Idx>(tileHeight);
    }
    Idx r_max = r_min + static_cast<Idx>(tileHeight);
    while (r_max < max_row)
    {
      r_max += static_cast<Idx>(tileHeight);
    }
    min_row = r_min;
    max_row = r_max;
    logging::check_fatal(
      min_column >= max_column,
      "Invalid bounds for columns with %d => %d",
      min_column,
      max_column);
    logging::check_fatal(
      min_row >= max_row,
      "Invalid bounds for rows with %d => %d",
      min_row,
      max_row);
#ifdef DEBUG_GRIDS
    logging::debug(
      "Bounds are (%d, %d), (%d, %d) after correction",
      min_column,
      min_row,
      max_column,
      max_row);
#endif
    logging::extensive("(%d, %d) => (%d, %d)", min_column, min_row, max_column, max_row);
    logging::check_fatal((max_row - min_row) % tileHeight != 0, "Invalid start and end rows");
    logging::check_fatal((max_column - min_column) % tileHeight != 0, "Invalid start and end columns");
    logging::extensive("Lower left corner is (%d, %d)", min_column, min_row);
    logging::extensive("Upper right corner is (%d, %d)", max_column, max_row);
    const MathSize xll = this->xllcorner() + min_column * this->cellSize();
    // offset is different for y since it's flipped
    const MathSize yll = this->yllcorner() + (min_row) * this->cellSize();
    logging::extensive("Lower left corner is (%f, %f)", xll, yll);
    const auto num_rows = static_cast<size_t>(max_row - min_row);
    const auto num_columns = static_cast<size_t>(max_column - min_column);
    // ensure this is always divisible by tile size
    logging::check_fatal(0 != (num_rows % tileWidth), "%d rows not divisible by tiles", num_rows);
    logging::check_fatal(0 != (num_columns % tileHeight), "%d columns not divisible by tiles", num_columns);
    string filename = dir + base_name + ".tif";
    TIFF* tif = GeoTiffOpen(filename.c_str(), "w");
    auto gtif = GTIFNew(tif);
    logging::check_fatal(!gtif, "Cannot open file %s as a GEOTIFF", filename.c_str());
    const double xul = xll;
    const double yul = this->yllcorner() + (this->cellSize() * max_row);
    double tiePoints[6] = {
      0.0,
      0.0,
      0.0,
      xul,
      yul,
      0.0};
    double pixelScale[3] = {
      this->cellSize(),
      this->cellSize(),
      0.0};
    uint32_t bps = sizeof(R) * 8;
    // make sure to use floating point if values are
    if (std::is_floating_point<R>::value)
    {
      logging::extensive("Writing %s with float data type for %s",
                         filename.c_str(),
                         typeid(R).name());
      TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    }
    else
    {
      logging::extensive("Writing %s with int data type for %s",
                         filename.c_str(),
                         typeid(R).name());
    }
    // FIX: was using double, and that usually doesn't make sense, but sometime it might?
    // use buffer big enought to fit any (V  + '.000\0') + 1
    constexpr auto n = std::numeric_limits<V>::digits10;
    static_assert(n > 0);
    char str[n + 6]{0};
    const auto nodata_as_int = static_cast<int>(this->nodataInput());
    sxprintf(str, "%d.000", nodata_as_int);
    logging::extensive(
      "%s using nodata string '%s' for nodata value of (%d, %f)",
      typeid(this).name(),
      str,
      nodata_as_int,
      static_cast<MathSize>(no_data));
    TIFFSetField(tif, TIFFTAG_GDAL_NODATA, str);
    logging::extensive("%s takes %d bits", base_name.c_str(), bps);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, num_columns);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, num_rows);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps);
    TIFFSetField(tif, TIFFTAG_TILEWIDTH, tileWidth);
    TIFFSetField(tif, TIFFTAG_TILELENGTH, tileHeight);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    GTIFSetFromProj4(gtif, this->proj4().c_str());
    TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, tiePoints);
    TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixelScale);
    size_t tileSize = tileWidth * tileHeight;
    const auto buf_size = tileSize * sizeof(R);
    logging::extensive("%s has buffer size %d", base_name.c_str(), buf_size);
    R* buf = (R*)_TIFFmalloc(buf_size);
    for (size_t co = 0; co < num_columns; co += tileWidth)
    {
      for (size_t ro = 0; ro < num_rows; ro += tileHeight)
      {
        std::fill_n(
          &buf[0],
          tileWidth * tileHeight,
          no_data);
        // NOTE: shouldn't need to check if writing outside of tile because we made bounds on tile edges above
        // need to put data from grid into buffer, but flipped vertically
        for (size_t x = 0; x < tileWidth; ++x)
        {
          for (size_t y = 0; y < tileHeight; ++y)
          {
            const Idx r = static_cast<Idx>(max_row) - (ro + y + 1);
            const Idx c = static_cast<Idx>(min_column) + co + x;
            const Location idx(r, c);
            // might be out of bounds if not divisible by number of tiles
            if (!(this->rows() <= r
                  || 0 > r
                  || this->columns() <= c
                  || 0 > c))
            {
              // HACK: was getting invalid rasters if assigning directly into buf
              const R value = convert(this->at(idx));
              buf[x + y * tileWidth] = value;
            }
          }
        }
        logging::check_fatal(TIFFWriteTile(tif, buf, co, ro, 0, 0) < 0, "Cannot write tile to %s", filename.c_str());
      }
    }
    GTIFWriteKeys(gtif);
    if (gtif)
    {
      GTIFFree(gtif);
    }
    _TIFFfree(buf);
    TIFFClose(tif);
    return filename;
  }
public:
  /**
   * \brief Save GridMap contents to file based on settings
   * \tparam R Type to be written to file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   * \param no_data Value to use as nodata in output
   */
  template <class R>
  string saveToFileWithoutRetry(const string& dir,
                                const string& base_name,
                                std::function<R(T value)> convert,
                                const R no_data) const
  {
    // NOTE: do this instead of function pointer because it's using templates
    if (fs::sim::Settings::saveAsAscii())
    {
      return this->template saveToAsciiFile<R>(
        dir,
        base_name,
        convert,
        no_data);
    }
    else
    {
      return this->template saveToTiffFile<R>(
        dir,
        base_name,
        convert,
        no_data);
    }
  }
  /**
   * \brief Save GridMap contents to file based on settings
   * \tparam R Type to be written to file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   * \param no_data Value to use as nodata in output
   */
  template <class R>
  string saveToFileWithRetry(const string& dir,
                             const string& base_name,
                             std::function<R(T value)> convert,
                             const R no_data) const
  {
    // HACK: (hopefully) ensure that write works
    try
    {
      // HACK: use different function name to prevent infinite recursion warning
      return this->template saveToFileWithoutRetry<R>(dir, base_name, convert, no_data);
    }
    catch (const std::exception& err)
    {
      logging::error("Error trying to write %s to %s so retrying\n%s",
                     base_name.c_str(),
                     dir.c_str(),
                     err.what());
      try
      {
        return this->template saveToFileWithoutRetry<R>(dir, base_name, convert, no_data);
      }
      catch (const std::exception& err_fatal)
      {
        // will exit if not supposed to retry
        return logging::fatal<string>(
          "Error trying to write %s to %s\n%s",
          base_name.c_str(),
          dir.c_str(),
          err_fatal.what());
      }
    }
  }
  /**
   * \brief Save GridMap contents to file based on settings
   * \tparam R Type to be written to file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   * \param no_data Value to use as nodata in output
   */
  template <class R>
  string saveToFile(const string& dir,
                    const string& base_name,
                    std::function<R(T value)> convert,
                    const R no_data) const
  {
    return this->template saveToFileWithRetry<R>(dir, base_name, convert, no_data);
  }
  /**
   * \brief Save GridMap contents to file based on settings
   * \tparam R Type to be written to file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   */
  template <class R>
  string saveToFile(const string& dir,
                    const string& base_name,
                    std::function<R(T value)> convert) const
  {
    return this->template saveToFile<R>(dir, base_name, convert, static_cast<R>(this->nodataInput()));
  }
  /**
   * \brief Save GridMap contents to file based on settings
   * \param dir Directory to save into
   * \param base_name File base name to use
   */
  template <class R = V>
  string saveToFile(const string& dir, const string& base_name) const
  {
    return this->template saveToFile<R>(
      dir,
      base_name,
      [](T value) -> R {
        return static_cast<R>(value);
      });
  }
};
}
