/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_CONSTANTGRID_H
#define FS_CONSTANTGRID_H
#include "stdafx.h"
#include "Grid.h"
#include "Location.h"
#include "Util.h"
namespace fs
{
template <class T, class V>
class ConstantGrid;
;
[[nodiscard]] ConstantGrid<int, int> readTiffInt(const string_view filename, const Point& point);
/**
 * \brief A GridData<T, V, vector<T>> that cannot change once initialized.
 * \tparam T The initialization value type.
 * \tparam V The initialized value type.
 */
template <class T, class V = T>
class ConstantGrid : public GridData<T, V, vector<T>>
{
public:
  ConstantGrid() = default;
  [[nodiscard]] constexpr T at(const XYIdx& location) const noexcept override
  {
    // constant grids would use index not hash
    return this->data.at(to_index(location));
  }
  /**
   * \brief Throw an error because ConstantGrid can't change values.
   */
  // ! @cond Doxygen_Suppress
  void set(const XYIdx&, const T) override
  // ! @endcond
  {
    throw runtime_error("Cannot change ConstantGrid");
  }
  ~ConstantGrid() override = default;
  ConstantGrid(const ConstantGrid& rhs) noexcept = default;
  ConstantGrid(ConstantGrid&& rhs) noexcept = default;
  ConstantGrid& operator=(const ConstantGrid& rhs) noexcept = default;
  ConstantGrid& operator=(ConstantGrid&& rhs) noexcept = default;
  ConstantGrid(
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
    vector<T>&& data
  )
    : GridData<T, V, vector<T>>(
        cell_size,
        width,
        height,
        nodata_input,
        nodata_value,
        xllcorner,
        yllcorner,
        xurcorner,
        yurcorner,
        proj4,
        std::move(data)
      )
  { }
  ConstantGrid(
    const MathSize cell_size,
    const Idx width,
    const Idx height,
    const V nodata_input,
    const T nodata_value,
    const MathSize xllcorner,
    const MathSize yllcorner,
    const string_view proj4,
    const T& initialization_value
  ) noexcept
    : ConstantGrid(
        cell_size,
        width,
        height,
        nodata_input,
        nodata_value,
        xllcorner,
        yllcorner,
        proj4,
        std::move(vector<T>(static_cast<size_t>(MAX_HEIGHT) * MAX_WIDTH, initialization_value))
      )
  { }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \param convert Function taking int and nodata int value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V> readTiff(
    const string_view filename,
    const Point& point,
    std::function<T(V, V)> convert
  )
  {
    logging::debug("Reading a raster where T = {:s}, V = {:s}", typeid(T).name(), typeid(V).name());
    auto grid = readTiffInt(filename, point);
    const auto nodata_input = grid.nodataInput();
    T nodata_value = convert(nodata_input, nodata_input);
    logging::check_fatal(
      convert(nodata_input, nodata_input) != nodata_value,
      "Expected nodata value to be returned from convert()"
    );
    vector<T> values(static_cast<size_t>(MAX_HEIGHT) * MAX_WIDTH, nodata_value);
    std::transform(grid.data.begin(), grid.data.end(), values.begin(), [&](const int v) {
      return convert(static_cast<V>(v), nodata_input);
    });
    ConstantGrid<T, V> result{
      grid.cellSize(),
      grid.width(),
      grid.height(),
      static_cast<V>(grid.nodataInput()),
      nodata_value,
      grid.xllcorner(),
      grid.yllcorner(),
      grid.xurcorner(),
      grid.yurcorner(),
      grid.proj4(),
      std::move(values)
    };
    return result;
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, T> readTiff(const string_view filename, const Point& point)
  {
    return readTiff(filename, point, no_convert<T>);
  }

protected:
  tuple<Idx, Idx, Idx, Idx> dataBounds() const override
  {
    const Idx min_y = 0;
    const Idx max_y = this->height();
    const Idx min_x = 0;
    const Idx max_x = this->width();
    return {min_x, min_y, max_x, max_y};
  }

private:
  /**
   * \brief Constructor
   * \param grid_info GridBase defining Grid area
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param values Values to initialize grid with
   */
  ConstantGrid(
    const GridBase& grid_info,
    const V nodata_input,
    const T nodata_value,
    vector<T>&& values
  )
    : ConstantGrid(
        grid_info.cellSize(),
        nodata_input,
        nodata_value,
        grid_info.xllcorner(),
        grid_info.yllcorner(),
        grid_info.xurcorner(),
        grid_info.yurcorner(),
        string(grid_info.proj4()),
        std::move(values)
      )
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(
      this->data.size() != static_cast<size_t>(MAX_HEIGHT) * MAX_WIDTH, "Invalid grid size"
    );
#endif
  }
};
namespace fuel
{
class FuelType;
}
using FuelGrid = ConstantGrid<const fuel::FuelType*, FuelSize>;
class Cell;
using ElevationGrid = ConstantGrid<ElevationSize>;
using CellGrid = ConstantGrid<Cell, SpreadKey>;
}
#endif
