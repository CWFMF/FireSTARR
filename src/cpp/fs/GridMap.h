/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_GRIDMAP_H
#define FS_GRIDMAP_H
#include "stdafx.h"
#include <cstdint>
#include "Grid.h"
#include "Location.h"
namespace fs
{
/**
 * \brief A GridData that uses an unordered_map for storage.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 */
template <class T, class V = T>
class GridMap : public GridData<T, V, map<XYIdx, T>>
{
public:
  [[nodiscard]] bool contains(const XYIdx& location) const
  {
    return this->data.end() != this->data.find(location);
  }
  [[nodiscard]] T at(const XYIdx& location) const override
  {
    const auto value = this->data.find(location);
    if (value == this->data.end())
    {
      return this->nodataValue();
    }
    return get<1>(*value);
  }
  /**
   * \brief Set value at Location
   * \param location Location to set value for
   * \param value Value to set at Location
   */
  void set(const XYIdx& location, const T value) override
  {
    this->data[location] = value;
    assert(at(location) == value);
  }
  GridMap() noexcept = default;
  ~GridMap() override = default;
  GridMap(
    const MathSize cell_size,
    const Idx width,
    const Idx height,
    T no_data,
    const int nodata,
    const MathSize xllcorner,
    const MathSize yllcorner,
    const MathSize xurcorner,
    const MathSize yurcorner,
    const string_view proj4
  )
    : GridData<T, V, map<XYIdx, T>>(
        cell_size,
        width,
        height,
        no_data,
        static_cast<uint8_t>(nodata),
        xllcorner,
        yllcorner,
        xurcorner,
        yurcorner,
        proj4,
        map<XYIdx, T>()
      )
  {
    constexpr auto max_hash = numeric_limits<HashSize>::max();
    // HACK: we don't want overflow errors, but we want to play with the hash size
    const auto MAX_WIDTH = static_cast<MathSize>(max_hash) / static_cast<MathSize>(this->height());
    logging::check_fatal(
      this->width() >= MAX_WIDTH,
      "Grid is too big for cells to be hashed - "
      "recompile with a larger HashSize value"
    );
#ifdef DEBUG_GRIDS
    // enforce converting to an int and back produces same V
    const auto n0 = this->nodataInput();
    const auto n1 = static_cast<NodataIntType>(n0);
    const auto n2 = static_cast<V>(n1);
    const auto n3 = static_cast<NodataIntType>(n2);
    logging::check_equal(n1, n3, "nodata_input_ as int");
    logging::check_equal(n0, n2, "nodata_input_ from int");
#endif
  }
  /**
   * \brief Construct empty GridMap with same extent as given Grid
   * \param grid Grid to use extent from
   */
  explicit GridMap(const Grid<T, V>& grid)
    : GridMap<T, V>(
        grid.cellSize(),
        grid.noData(),
        grid.nodata(),
        grid.xllcorner(),
        grid.yllcorner(),
        grid.xurcorner(),
        grid.yurcorner(),
        grid.proj4()
      )
  { }
  /**
   * \brief Construct empty GridMap with same extent as given Grid
   * \param grid_info Grid to use extent from
   * \param no_data Value to use for no data
   */
  GridMap(const GridBase& grid_info, T no_data)
    : GridMap<T, V>(
        grid_info.cellSize(),
        static_cast<Idx>(grid_info.calculateWidth()),
        static_cast<Idx>(grid_info.calculateHeight()),
        no_data,
        static_cast<int>(no_data),
        grid_info.xllcorner(),
        grid_info.yllcorner(),
        grid_info.xurcorner(),
        grid_info.yurcorner(),
        string(grid_info.proj4())
      )
  { }
  GridMap(GridMap&& rhs) noexcept : GridData<T, V, map<XYIdx, T>>(std::move(rhs))
  {
    this->data = std::move(rhs.data);
  }
  GridMap(const GridMap& rhs) : GridData<T, V, map<XYIdx, T>>(rhs) { this->data = rhs.data; }
  GridMap& operator=(GridMap&& rhs) noexcept
  {
    if (this != &rhs)
    {
      this->data = std::move(rhs.data);
    }
    return *this;
  }
  GridMap& operator=(const GridMap& rhs)
  {
    if (this != &rhs)
    {
      this->data = rhs.data;
    }
    return *this;
  }
  /**
   * \brief Clear data from GridMap
   */
  void clear() noexcept { this->data = {}; }

protected:
  tuple<Idx, Idx, Idx, Idx> dataBounds() const override
  {
    Idx min_y = this->height();
    Idx max_y = 0;
    Idx min_x = this->width();
    Idx max_x = 0;
    for (const auto& kv : this->data)
    {
      const Idx r = kv.first.y_value();
      const Idx c = kv.first.x_value();
      min_y = min(min_y, r);
      max_y = max(max_y, r);
      min_x = min(min_x, c);
      max_x = max(max_x, c);
    }
    // do this so that we take the center point when there's no data since it should
    // stay the same if the grid is centered on the fire
    if (min_y > max_y)
    {
      min_y = max_y = this->height() / 2;
    }
    if (min_x > max_x)
    {
      min_x = max_x = this->width() / 2;
    }
    return {min_x, min_y, max_x, max_y};
  }

public:
  /**
   * \brief Save GridMap contents to .asc file as probability
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param divisor Number of simulations to divide by to calculate probability per cell
   * \return FileList of file names saved to
   */
  template <class R>
  [[nodiscard]] FileList saveToProbabilityFile(
    const string_view dir,
    const string_view base_name,
    const R divisor
  ) const
  {
    auto div = [=](T value) -> R { return static_cast<R>(value) / divisor; };
    return this->template saveToFile<R>(dir, base_name, div);
  }
  /**
   * \brief Calculate area for cells that have a value (ha)
   * \return Area for cells that have a value (ha)
   */
  [[nodiscard]] MathSize fireSize() const noexcept
  {
    // we know that every cell is a key, so we convert that to hectares
    const MathSize per_width = (this->cellSize() / 100.0);
    // cells might have 0 as a value, but those shouldn't affect size
    return static_cast<MathSize>(this->data.size()) * per_width * per_width;
  }
  /**
   * \brief Make a list of all Locations that are on the edge of cells with a value
   * \return A list of all Locations that are on the edge of cells with a value
   */
  [[nodiscard]] list<XYIdx> makeEdge() const
  {
    list<XYIdx> edge{};
    for (const auto& kv : this->data)
    {
      auto loc = kv.first;
      // FIX: any way to avoid decomposing?
      auto [x_loc, y_loc] = hash_to_xy(loc);
      auto on_edge = false;
      for (Idx r = -1; !on_edge && r <= 1; ++r)
      {
        const Idx y = y_loc.value + r;
        if (!(y < 0 || y >= this->height()))
        {
          for (Idx c = -1; !on_edge && c <= 1; ++c)
          {
            const Idx x = x_loc.value + c;
            if (!(x < 0 || x >= this->width()) && this->data.find(XYIdx{x, y}) == this->data.end())
            {
              on_edge = true;
            }
          }
        }
      }
      if (on_edge)
      {
        edge.push_back(loc);
      }
    }
    logging::info(
      "Created edge for perimeter with length {:d} m",
      static_cast<size_t>(this->cellSize() * static_cast<MathSize>(edge.size()))
    );
    return edge;
  }
  /**
   * \brief Make a list of all Locations that have a value
   * \return A list of all Locations that have a value
   */
  [[nodiscard]] list<XYIdx> makeList() const
  {
    list<XYIdx> result{this->data.size()};
    std::transform(
      this->data.begin(),
      this->data.end(),
      result.begin(),
      [](const pair<const XYIdx, const T>& kv) { return kv.first; }
    );
    return result;
  }
};
}
#endif
