/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Perimeter.h"
#include "Environment.h"
#include "FuelLookup.h"
#include "Location.h"
namespace fs
{
BurnedMap::BurnedMap(const Grid<unsigned char, unsigned char>& perim_grid, const Environment& env)
  : GridMap<unsigned char, unsigned char>(env.makeMap<unsigned char>(static_cast<unsigned char>(0)))
{
  using fs::fuel::is_null_fuel;
  // HACK: fix offset if the perimeter raster is different from this one
  logging::check_fatal(
    0 != strcmp(perim_grid.proj4().c_str(), this->proj4().c_str()),
    "Invalid projection for input perimeter raster - {:s} instead of {:s}",
    perim_grid.proj4(),
    this->proj4()
  );
  logging::check_fatal(
    perim_grid.cellSize() != this->cellSize(),
    "Invalid cell size for input perimeter raster - {:f} instead of {:f}",
    perim_grid.cellSize(),
    this->cellSize()
  );
  const auto offset_x =
    static_cast<Idx>((this->xllcorner() - perim_grid.xllcorner()) / this->cellSize());
  const auto perim_origin =
    static_cast<Idx>(perim_grid.height() + perim_grid.yllcorner() / this->cellSize());
  const auto this_origin = static_cast<Idx>(this->height() + this->yllcorner() / this->cellSize());
  const auto offset_y = static_cast<Idx>((perim_origin - this_origin));
  // make sure we don't go out of bounds on grid
  const auto min_x = static_cast<Idx>(offset_x < 0 ? abs(offset_x) : 0);
  const auto max_width = min(this->width(), perim_grid.width());
  const auto min_y = static_cast<Idx>(offset_y < 0 ? abs(offset_y) : 0);
  const auto max_height =
    min(this->height(), static_cast<Idx>(perim_grid.height() - abs(offset_y)));
  logging::note("Correcting perimeter raster offset by {:d}{:d} cells", offset_x, offset_y);
  size_t count = 0;
  // since it was read in as a vector we need to check all the cells
  for (auto y = min_y; y < max_height; ++y)
  {
    for (auto x = min_x; x < max_width; ++x)
    {
      const XYIdx xy{x, y};
      const auto loc = env.cell(xy);
      const auto x0 = static_cast<Idx>(x + offset_x);
      const auto y0 = static_cast<Idx>(y + offset_y);
      const XYIdx fixed_loc{x0, y0};
      const auto value = perim_grid.at(fixed_loc);
      if (value != perim_grid.nodataValue() && !is_null_fuel(loc))
      {
        this->GridMap<unsigned char, unsigned char>::set(xy, value);
        ++count;
      }
    }
  }
  size_hectares_ = env.to_hectares(count);
#ifdef DEBUG_GRIDS
  for (auto& kv : data)
  {
    auto& loc = kv.first;
    logging::check_fatal(is_null_fuel(env.cell(loc)), "Null fuel in BurnedData");
  }
#endif
  logging::info("Loaded burned area of size {:d} ha", size_hectares_);
}
Perimeter::Perimeter(const BurnedMap& burned_map)
  : burned(burned_map.makeList()), edge(burned_map.makeEdge())
{ }
BurnedMap make_burned_map(const LazyPath& perim, const Point& point, const Environment& env)
{
  auto perim_grid = ConstantGrid<unsigned char>::readTiff(perim.canonical(), point);
  return BurnedMap(perim_grid, env);
}
Perimeter::Perimeter(const LazyPath& perim, const Point& point, const Environment& env)
  : Perimeter(make_burned_map(perim, point, env))
{ }
BurnedMap make_burned_map(const XYIdx& location, const size_t size, const Environment& env)
{
  // NOTE: FwiWeather is unused but could change this to try doing length to breadth ratio
  auto perim_grid = env.makeMap<unsigned char>(0);
  // want to find cells in the area that fill up the size we're looking for
  size_t count = 0;
  // convert into number of cells
  const auto num_cells = size / (100.0 * 100.0 / (perim_grid.cellSize() * perim_grid.cellSize()));
  auto max_distance = sqrt(num_cells / M_PI);
  perim_grid.set(location, 1);
  ++count;
  const auto& x_loc = location.x.value;
  const auto& y_loc = location.y.value;
  // HACK: assume fuel for origin matches the rest of the fire
  while (num_cells > count)
  {
    const auto range = static_cast<Idx>(ceil(max_distance));
    for (auto x = -range; x <= range && num_cells > count; ++x)
    {
      for (auto y = -range; y <= range && num_cells > count; ++y)
      {
        // look at any cell that's within the range
        if (sqrt(pow_int<2>(x) + pow_int<2>(y)) < max_distance)
        {
          const XYIdx xy{x_loc + x, y_loc + y};
          if (1 != perim_grid.at(xy) && !fs::fuel::is_null_fuel(env.cell(xy)))
          {
            perim_grid.set(xy, 1);
            ++count;
          }
        }
      }
    }
    max_distance += 0.1;
  }
  return BurnedMap(perim_grid, env);
}
Perimeter::Perimeter(const XYIdx& location, const size_t size, const Environment& env)
  : Perimeter(make_burned_map(location, size, env))
{ }
}
