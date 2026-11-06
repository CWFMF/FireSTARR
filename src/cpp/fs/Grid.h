/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_GRID_H
#define FS_GRID_H
#include "stdafx.h"
#include "Location.h"
#include "Log.h"
#include "Point.h"
#include "Settings.h"
#include "tiff.h"
namespace fs
{
using namespace logging;
using NodataIntType = int64_t;
// NOTE: use types specifically for libtiff
using TiffFloat = float;
using TiffDouble = double;
string create_file_name(
  const string_view dir,
  const string_view base_name,
  const string_view extension
);
/**
 * \brief The base class with information for a grid of data with geographic coordinates.
 */
class GridBase
{
public:
  virtual ~GridBase() = default;
  GridBase(GridBase&& rhs) noexcept = default;
  GridBase(const GridBase& rhs) = default;
  GridBase& operator=(const GridBase& rhs) = default;
  GridBase& operator=(GridBase&& rhs) noexcept = default;
  /**
   * \brief Cell size used for GridBase.
   * \return Cell height and width in metres.
   */
  [[nodiscard]] constexpr MathSize cellSize() const noexcept { return cell_size_; }
  [[nodiscard]] constexpr FullIdx calculateHeight() const noexcept
  {
    return static_cast<FullIdx>((yurcorner() - yllcorner()) / cellSize()) - 1;
  }
  [[nodiscard]] constexpr FullIdx calculateWidth() const noexcept
  {
    return static_cast<FullIdx>((xurcorner() - xllcorner()) / cellSize()) - 1;
  }
  /**
   * \brief Lower left corner X coordinate in metres.
   * \return Lower left corner X coordinate in metres.
   */
  [[nodiscard]] constexpr MathSize xllcorner() const noexcept { return xllcorner_; }
  /**
   * \brief Lower left corner Y coordinate in metres.
   * \return Lower left corner Y coordinate in metres.
   */
  [[nodiscard]] constexpr MathSize yllcorner() const noexcept { return yllcorner_; }
  /**
   * \brief Upper right corner X coordinate in metres.
   * \return Upper right corner X coordinate in metres.
   */
  [[nodiscard]] constexpr MathSize xurcorner() const noexcept { return xurcorner_; }
  /**
   * \brief Upper right corner Y coordinate in metres.
   * \return Upper right corner Y coordinate in metres.
   */
  [[nodiscard]] constexpr MathSize yurcorner() const noexcept { return yurcorner_; }
  /**
   * \brief Proj4 string defining coordinate system for this grid. Must be a UTM projection.
   * \return Proj4 string defining coordinate system for this grid.
   */
  [[nodiscard]] constexpr const string& proj4() const noexcept { return proj4_; }
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   */
  GridBase(
    MathSize cell_size,
    MathSize xllcorner,
    MathSize yllcorner,
    MathSize xurcorner,
    MathSize yurcorner,
    const string_view proj4
  ) noexcept;
  GridBase() noexcept = default;
  /**
   * \brief Create .prj file in directory with base name for file
   * \param dir Directory to create in
   * \param base_name base file name for .prj file
   */
  void createPrj(const string_view dir, const string_view base_name) const;
  /**
   * \brief Find Coordinates for Point
   * \param point Point to translate to Grid Coordinate
   * \param flipped Whether or not Grid data is flipped along x axis
   * \return Coordinates for Point translated to Grid
   */
  [[nodiscard]] std::optional<Coordinates> findCoordinates(const Point& point, bool flipped) const;
  /**
   * \brief Find FullCoordinates for Point
   * \param point Point to translate to Grid Coordinate
   * \param flipped Whether or not Grid data is flipped along x axis
   * \return Coordinates for Point translated to Grid
   */
  [[nodiscard]] std::optional<FullCoordinates> findFullCoordinates(const Point& point, bool flipped)
    const;
  [[nodiscard]] bool validate(const Point& point) const;
  string saveToTiffFileInt(
    const Idx width,
    const Idx height,
    const tuple<Idx, Idx, Idx, Idx> bounds,
    const string_view dir,
    const string_view base_name,
    const uint16_t bits_per_sample,
    const bool is_unsigned,
    std::function<int(const XYIdx&)> value_at,
    const int nodata_as_int
  ) const;
  string saveToTiffFileFloat(
    const Idx width,
    const Idx height,
    const tuple<Idx, Idx, Idx, Idx> bounds,
    const string_view dir,
    const string_view base_name,
    // const uint16_t bits_per_sample,
    // const uint16_t sample_format,
    std::function<TiffDouble(const XYIdx&)> value_at,
    const int nodata_as_int
  ) const;

private:
  /**
   * \brief Proj4 string defining projection.
   */
  string proj4_{};
  /**
   * \brief Cell height and width in metres.
   */
  MathSize cell_size_{-1};
  /**
   * \brief Lower left corner X coordinate in metres.
   */
  MathSize xllcorner_{-1};
  /**
   * \brief Lower left corner Y coordinate in metres.
   */
  MathSize yllcorner_{-1};
  /**
   * \brief Upper right corner X coordinate in metres.
   */
  MathSize xurcorner_{-1};
  /**
   * \brief Upper right corner Y coordinate in metres.
   */
  MathSize yurcorner_{-1};
};
void write_ascii_header(
  ofstream& out,
  MathSize width,
  MathSize height,
  MathSize xll,
  MathSize yll,
  MathSize cell_size,
  MathSize no_data
);
GridBase read_header(GeoTiff& geotiff);
GridBase read_header(const string_view filename);
/**
 * \brief A GridBase with an associated type of data.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 */
template <class T, class V = T>
class Grid : public GridBase
{
public:
  [[nodiscard]] constexpr Idx height() const noexcept { return height_; }
  [[nodiscard]] constexpr Idx width() const noexcept { return width_; }
  /**
   * \brief Value used for grid locations that have no data.
   * \return Value used for grid locations that have no data.
   */
  [[nodiscard]] constexpr V nodataInput() const noexcept { return nodata_input_; }
  /**
   * \brief Value representing no data
   * \return Value representing no data
   */
  constexpr T nodataValue() const noexcept
  {
    return nodata_value_;
  }   // NOTE: only use this for simple types because it's returning by value
  [[nodiscard]] virtual T at(const XYIdx& location) const = 0;
  // NOTE: use set instead of at to avoid issues with bool
  virtual void set(const XYIdx& location, T value) = 0;

protected:
  Grid() = default;
  Grid(
    const MathSize cell_size,
    const Idx width,
    const Idx height,
    const V nodata_input,
    const T nodata_value,
    const MathSize xllcorner,
    const MathSize yllcorner,
    const MathSize xurcorner,
    const MathSize yurcorner,
    const string_view proj4
  ) noexcept
    : GridBase(cell_size, xllcorner, yllcorner, xurcorner, yurcorner, proj4),
      nodata_input_(nodata_input), nodata_value_(nodata_value), height_(height), width_(width)
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(height > MAX_HEIGHT, "Height too large ({:d} > {:d})", height, MAX_HEIGHT);
    logging::check_fatal(width > MAX_WIDTH, "Width too large ({:d} > {:d})", width, MAX_WIDTH);
#endif
#ifdef DEBUG_GRIDS
    // enforce converting to an int and back produces same V
    const auto n0 = this->nodata_input_;
    const auto n1 = static_cast<NodataIntType>(n0);
    const auto n2 = static_cast<V>(n1);
    const auto n3 = static_cast<NodataIntType>(n2);
    logging::check_equal(n1, n3, "nodata_input_ as int");
    logging::check_equal(n0, n2, "nodata_input_ from int");
#endif
  }
  /**
   * \brief Construct based on GridBase and no data value
   * \param grid_info GridBase defining Grid area
   * \param no_data Value that represents no data
   */
  Grid(const GridBase& grid_info, V no_data) noexcept
    : Grid(
        grid_info.cellSize(),
        static_cast<Idx>(grid_info.calculateHeight()),
        static_cast<Idx>(grid_info.calculateWidth()),
        no_data,
        to_string(no_data),
        grid_info.xllcorner(),
        grid_info.yllcorner(),
        grid_info.xurcorner(),
        grid_info.yurcorner(),
        grid_info.proj4()
      )
  { }

private:
  /**
   * \brief Value used to represent no data at a Location.
   */
  V nodata_input_;
  /**
   * \brief Value to use for representing no data at a Location.
   */
  T nodata_value_;
  Idx height_{};
  Idx width_{};
};
/**
 * \brief A Grid that defines the data structure used for storing values.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 * \tparam D The data type that stores the values.
 */
template <class T, class V, class D>
class GridData : public Grid<T, V>
{
public:
  using Grid<T, V>::Grid;
  GridData(
    const MathSize cell_size,
    const Idx width,
    const Idx height,
    const V nodata_input,
    const T nodata_value,
    const MathSize xllcorner,
    const MathSize yllcorner,
    const MathSize xurcorner,
    const MathSize yurcorner,
    const string_view proj4,
    D&& data
  )
    : Grid<T, V>(
        cell_size,
        width,
        height,
        nodata_input,
        nodata_value,
        xllcorner,
        yllcorner,
        xurcorner,
        yurcorner,
        proj4
      ),
      data(std::forward<D>(data))
  { }
  ~GridData() override = default;
  GridData(const GridData& rhs) : Grid<T, V>(rhs), data(rhs.data) { }
  GridData(GridData&& rhs) noexcept : Grid<T, V>(rhs), data(std::move(rhs.data)) { }
  GridData& operator=(const GridData& rhs) noexcept
  {
    if (this != &rhs)
    {
      Grid<T, V>::operator=(rhs);
      data = rhs.data;
    }
    return *this;
  }
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
  [[nodiscard]] size_t size() const { return data.size(); }
  // HACK: use public access so that we can get to the keys
  /**
   * \brief Structure that holds data represented by this GridData
   */
  D data{};

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
  string saveToAsciiFile(
    const string_view dir,
    const string_view base_name,
    std::function<R(T value)> convert,
    const R no_data
  ) const
  {
    const auto [min_x, min_y, max_x, max_y] = dataBounds();
    // auto bounds = dataBounds();
    // auto min_x = std::get<0>(bounds);
    // auto min_y = std::get<1>(bounds);
    // auto max_x = std::get<2>(bounds);
    // auto max_y = std::get<3>(bounds);
#ifdef DEBUG_GRIDS
    logging::debug("Bounds are ({:d}, {:d}), ({:d}, {:d})", min_x, min_y, max_x, max_y);
#endif
    logging::extensive("Lower left corner is ({:d}, {:d})", min_x, min_y);
    logging::extensive("Upper right corner is ({:d}, {:d})", max_x, max_y);
    const MathSize xll = this->xllcorner() + min_x * this->cellSize();
    // offset is different for y since it's flipped
    const MathSize yll = this->yllcorner() + (min_y) * this->cellSize();
    logging::extensive("Lower left corner is ({:f}, {:f})", xll, yll);
    // HACK: make sure it's always at least 1
    const auto height_calc = static_cast<MathSize>(max_y) - min_y + 1;
    const auto width_calc = static_cast<MathSize>(max_x) - min_x + 1;
    const auto filename = create_file_name(dir, base_name, "asc");
    ofstream out{filename};
    write_ascii_header(
      out, width_calc, height_calc, xll, yll, this->cellSize(), static_cast<MathSize>(no_data)
    );
    for (Idx y0 = 0; y0 < height_calc; ++y0)
    {
      // HACK: do this so that we always get at least one pixel in output
      // need to output in reverse order since (0,0) is bottom left
      const Idx y1 = static_cast<Idx>(max_y) - y0;
      for (Idx x0 = 0; x0 < width_calc; ++x0)
      {
        const XYIdx idx{static_cast<Idx>(min_x + x0), static_cast<Idx>(y1)};
        // HACK: use + here so that it gets promoted to a printable number
        //       prevents char type being output as characters
        out << +(convert(this->at(idx))) << " ";
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
  string saveToTiffFile(
    const string_view dir,
    const string_view base_name,
    std::function<R(T value)> convert,
    const R no_data
  ) const
    requires(std::is_floating_point_v<R>)
  {
    // constexpr auto bps = 16;
    // auto bps = sizeof(R) * 8;
    // logging::check_fatal(
    //   bps != 16, "Only float16_t supported for output tiff but BITSPERSAMPLE is {:d}", bps
    // );
    // HACK: output is using internal nodata representation right now, but that shouldn't change
    // anything FIX: use original input nodata value
    // logging::check_equal(
    //   static_cast<int>(no_data), static_cast<int>(this->nodataInput()), "integer nodata"
    // );
    return GridBase::saveToTiffFileFloat(
      this->width(),
      this->height(),
      this->dataBounds(),
      dir,
      base_name,
      // bps,
      // SAMPLEFORMAT_IEEEFP,
      [&](const XYIdx& idx) { return static_cast<TiffDouble>(convert(this->at(idx))); },
      no_data
    );
  }
  template <class R>
  string saveToTiffFile(
    const string_view dir,
    const string_view base_name,
    std::function<R(T value)> convert,
    const R no_data
  ) const
    requires(std::is_integral_v<R>)
  {
    auto bps = sizeof(R) * 8;
    // HACK: output is using internal nodata representation right now, but that shouldn't change
    // anything FIX: use original input nodata value
    // logging::check_equal(
    //   static_cast<int>(no_data), static_cast<int>(this->nodataInput()), "integer nodata"
    // );
    return GridBase::saveToTiffFileInt(
      this->width(),
      this->height(),
      this->dataBounds(),
      dir,
      base_name,
      bps,
      std::is_unsigned_v<R>,
      [&](const XYIdx& idx) { return static_cast<int>(convert(this->at(idx))); },
      no_data
    );
  }

public:
  /**
   * \brief Save GridMap contents to file based on settings
   * \tparam R Type to be written to file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   * \param no_data Value to use as nodata in output
   * \return FileList of file names saved to
   */
  template <class R>
  [[nodiscard]] FileList saveToFileWithoutRetry(
    const string_view dir,
    const string_view base_name,
    std::function<R(T value)> convert,
    const R no_data
  ) const
  {
    static const auto& settings = settings::instance();
    FileList files{};
    // NOTE: do this instead of function pointer because it's using templates
    if (settings.save_as_ascii)
    {
      files.emplace_back(this->template saveToAsciiFile<R>(dir, base_name, convert, no_data));
    }
    // always save at least something
    if (!settings.save_as_ascii || settings.save_as_tiff)
    {
      files.emplace_back(this->template saveToTiffFile<R>(dir, base_name, convert, no_data));
    }
    return files;
  }
  /**
   * \brief Save GridMap contents to file based on settings
   * \tparam R Type to be written to file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   * \param no_data Value to use as nodata in output
   * \return FileList of file names saved to
   */
  template <class R>
  [[nodiscard]] FileList saveToFileWithRetry(
    const string_view dir,
    const string_view base_name,
    std::function<R(T value)> convert,
    const R no_data
  ) const
  {
    // HACK: (hopefully) ensure that write works
    try
    {
      // HACK: use different function name to prevent infinite recursion warning
      return this->template saveToFileWithoutRetry<R>(dir, base_name, convert, no_data);
    }
    catch (const std::exception& err)
    {
      logging::error(
        "Error trying to write {:s} to {:s} so retrying\n{:s}", base_name, dir, err.what()
      );
      try
      {
        return this->template saveToFileWithoutRetry<R>(dir, base_name, convert, no_data);
      }
      catch (const std::exception& err_fatal)
      {
        // will exit if not supposed to retry
        exit(logging::fatal(
          "Error trying to write {:s} to {:s}\n{:s}", base_name, dir, err_fatal.what()
        ));
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
   * \return FileList of file names saved to
   */
  template <class R>
  [[nodiscard]] FileList saveToFile(
    const string_view dir,
    const string_view base_name,
    std::function<R(T value)> convert,
    const R no_data
  ) const
  {
    return this->template saveToFileWithRetry<R>(dir, base_name, convert, no_data);
  }
  /**
   * \brief Save GridMap contents to file based on settings
   * \tparam R Type to be written to file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   * \return FileList of file names saved to
   */
  template <class R>
  [[nodiscard]] FileList saveToFile(
    const string_view dir,
    const string_view base_name,
    std::function<R(T value)> convert
  ) const
  {
    return this->template saveToFile<R>(
      dir, base_name, convert, static_cast<R>(this->nodataInput())
    );
  }
  /**
   * \brief Save GridMap contents to file based on settings
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \return FileList of file names saved to
   */
  template <class R = V>
  [[nodiscard]] FileList saveToFile(const string_view dir, const string_view base_name) const
  {
    return this->template saveToFile<R>(dir, base_name, [](T value) -> R {
      return static_cast<R>(value);
    });
  }
};
}
#endif
