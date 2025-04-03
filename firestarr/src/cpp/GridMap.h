/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "Util.h"
#include "Grid.h"
#include "ConstantGrid.h"
#include "Settings.h"
namespace tbd::data
{
using topo::Location;
using topo::Position;
/**
 * \brief A GridData that uses an unordered_map for storage.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 */
template <class T, class V = T>
class GridMap
  : public GridData<T, V, map<Location, T>>
{
public:
  /**
   * \brief Determine if Location has a value
   * \param location Location to determine if present in GridMap
   * \return Whether or not a value is present for the Location
   */
  [[nodiscard]] bool contains(const Location& location) const
  {
    return this->data.end() != this->data.find(location);
  }
  template <class P>
  [[nodiscard]] bool contains(const Position<P>& position) const
  {
    return contains(Location{position.hash()});
  }
  /**
   * \brief Retrieve value at Location
   * \param location Location to get value for
   * \return Value at Location
   */
  [[nodiscard]] T at(const Location& location) const override
  {
    const auto value = this->data.find(location);
    if (value == this->data.end())
    {
      return this->nodataValue();
    }
    return get<1>(*value);
  }
  template <class P>
  [[nodiscard]] T at(const Position<P>& position) const
  {
    return at(Location{position.hash()});
  }
  /**
   * \brief Set value at Location
   * \param location Location to set value for
   * \param value Value to set at Location
   */
  void set(const Location& location, const T value) override
  {
    this->data[location] = value;
    assert(at(location) == value);
  }
  template <class P>
  void set(const Position<P>& position, const T value)
  {
    return set(Location{position.hash()}, value);
  }
  ~GridMap() = default;
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value that represents no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xllcorner Upper right corner X coordinate (m)
   * \param yllcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   */
  GridMap(const MathSize cell_size,
          const Idx rows,
          const Idx columns,
          T no_data,
          const int nodata,
          const MathSize xllcorner,
          const MathSize yllcorner,
          const MathSize xurcorner,
          const MathSize yurcorner,
          string&& proj4)
    : GridData<T, V, map<Location, T>>(cell_size,
                                       rows,
                                       columns,
                                       no_data,
                                       nodata,
                                       xllcorner,
                                       yllcorner,
                                       xurcorner,
                                       yurcorner,
                                       std::forward<string>(proj4),
                                       map<Location, T>())
  {
    constexpr auto max_hash = numeric_limits<HashSize>::max();
    // HACK: we don't want overflow errors, but we want to play with the hash size
    const auto max_columns = static_cast<MathSize>(max_hash) / static_cast<MathSize>(this->rows());
    logging::check_fatal(this->columns() >= max_columns,
                         "Grid is too big for cells to be hashed - "
                         "recompile with a larger HashSize value");
#ifdef DEBUG_GRIDS
    // enforce converting to an int and back produces same V
    const auto n0 = this->nodataInput();
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
    // HACK: reserve space for this based on how big our Idx is because that
    // tells us how many cells there could be
    // HACK: divide because we expect most perimeters to be fairly small, but we
    // want it to be reasonably large
    //    this->data.reserve(static_cast<size_t>(numeric_limits<Idx>::max() / 4));
    //    this->data.reserve(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS);
  }
  /**
   * \brief Construct empty GridMap with same extent as given Grid
   * \param grid Grid to use extent from
   */
  explicit GridMap(const Grid<T, V>& grid)
    : GridMap<T, V>(grid.cellSize(),
                    grid.noData(),
                    grid.nodata(),
                    grid.xllcorner(),
                    grid.yllcorner(),
                    grid.xurcorner(),
                    grid.yurcorner(),
                    grid.proj4())
  {
  }
  /**
   * \brief Construct empty GridMap with same extent as given Grid
   * \param grid_info Grid to use extent from
   * \param no_data Value to use for no data
   */
  GridMap(const GridBase& grid_info, T no_data)
    : GridMap<T, V>(grid_info.cellSize(),
                    static_cast<Idx>(grid_info.calculateRows()),
                    static_cast<Idx>(grid_info.calculateColumns()),
                    no_data,
                    static_cast<int>(no_data),
                    grid_info.xllcorner(),
                    grid_info.yllcorner(),
                    grid_info.xurcorner(),
                    grid_info.yurcorner(),
                    string(grid_info.proj4()))
  {
  }
  /**
   * \brief Move constructor
   * \param rhs GridMap to move from
   */
  GridMap(GridMap&& rhs) noexcept
    : GridData<T, V, map<Location, T>>(std::move(rhs))
  {
    this->data = std::move(rhs.data);
  }
  /**
   * \brief Copy constructor
   * \param rhs GridMap to copy from
   */
  GridMap(const GridMap& rhs)
    : GridData<T, V, map<Location, T>>(rhs)
  {
    this->data = rhs.data;
  }
  /**
   * \brief Move assignment
   * \param rhs GridMap to move from
   * \return This, after assignment
   */
  GridMap& operator=(GridMap&& rhs) noexcept
  {
    if (this != &rhs)
    {
      this->data = std::move(rhs.data);
    }
    return *this;
  }
  /**
   * \brief Copy assignment
   * \param rhs GridMap to copy from
   * \return This, after assignment
   */
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
  void clear() noexcept
  {
    //    this->data.clear();
    this->data = {};
    //    this->data.reserve(static_cast<size_t>(numeric_limits<Idx>::max() / 4));
  }
protected:
  tuple<Idx, Idx, Idx, Idx> dataBounds() const override
  {
    Idx min_row = this->rows();
    Idx max_row = 0;
    Idx min_column = this->columns();
    Idx max_column = 0;
    for (const auto& kv : this->data)
    {
      const Idx r = kv.first.row();
      const Idx c = kv.first.column();
      min_row = min(min_row, r);
      max_row = max(max_row, r);
      min_column = min(min_column, c);
      max_column = max(max_column, c);
    }
    // do this so that we take the center point when there's no data since it should
    // stay the same if the grid is centered on the fire
    if (min_row > max_row)
    {
      min_row = max_row = this->rows() / 2;
    }
    if (min_column > max_column)
    {
      min_column = max_column = this->columns() / 2;
    }
    return tuple<Idx, Idx, Idx, Idx>{
      min_column,
      min_row,
      max_column,
      max_row};
  }
public:
  /**
   * \brief Save GridMap contents to .asc file as probability
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param divisor Number of simulations to divide by to calculate probability per cell
   */
  template <class R>
  string saveToProbabilityFile(const string& dir,
                               const string& base_name,
                               const R divisor) const
  {
    auto div = [divisor](T value) -> R {
      return static_cast<R>(value / divisor);
    };
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
  [[nodiscard]] list<Location> makeEdge() const
  {
    list<Location> edge{};
    for (const auto& kv : this->data)
    {
      auto loc = kv.first;
      auto on_edge = false;
      for (Idx r = -1; !on_edge && r <= 1; ++r)
      {
        const Idx row_index = loc.row() + r;
        if (!(row_index < 0 || row_index >= this->rows()))
        {
          for (Idx c = -1; !on_edge && c <= 1; ++c)
          {
            const Idx col_index = loc.column() + c;
            if (!(col_index < 0 || col_index >= this->columns())
                && this->data.find(Location(row_index, col_index)) == this->data.end())
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
    logging::info("Created edge for perimeter with length %lu m",
                  static_cast<size_t>(this->cellSize() * edge.size()));
    return edge;
  }
  /**
   * \brief Make a list of all Locations that have a value
   * \return A list of all Locations that have a value
   */
  [[nodiscard]] list<Location> makeList() const
  {
    list<Location> result{this->data.size()};
    std::transform(this->data.begin(),
                   this->data.end(),
                   result.begin(),
                   [](const pair<const Location, const T>& kv) { return kv.first; });
    return result;
  }
};
}
