/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Environment.h"
#include "EnvironmentInfo.h"
#include "FuelLookup.h"
#include "Grid.h"
#include "Location.h"
#include "Log.h"
#include "Point.h"
#include "ProbabilityMap.h"
#include "Radians.h"
#include "Settings.h"
#include "Util.h"
namespace fs
{
Environment Environment::load(
  const Point& point,
  const string_view in_fuel,
  const string_view in_elevation
)
{
  logging::note("Fuel raster is {:s}", string(in_fuel));
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  static const auto& lookup = settings.fuel_lookup.lookup();
  if (settings.run_async)
  {
    logging::debug("Loading grids async");
    auto fuel = async(launch::async, [&]() { return FuelGrid::readTiff(in_fuel, point, lookup); });
    auto elevation =
      async(launch::async, [&]() { return ElevationGrid::readTiff(in_elevation, point); });
    logging::debug("Waiting for grids");
    return Environment(fuel.get(), elevation.get(), point);
  }
  logging::warning("Loading grids synchronously");
  // HACK: need to copy strings since closures do that above
  return Environment(
    FuelGrid::readTiff(in_fuel, point, lookup), ElevationGrid::readTiff(in_elevation, point), point
  );
}
shared_ptr<ProbabilityMap> Environment::makeProbabilityMap(
  const DurationSize time,
  const DurationSize start_time,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value,
  const shared_ptr<Perimeter> perimeter
) const
{
  return make_shared<ProbabilityMap>(
    time, start_time, min_value, low_max, med_max, max_value, cells_, perimeter
  );
}
Environment Environment::loadEnvironment(
  const string_view path,
  const Point& point,
  const LazyPath& perimeter,
  const YearSize year
)
{
  logging::note("Using ignition point {}", point);
  logging::info("Running using inputs directory '{:s}'", string(path));
  auto rasters = find_rasters(path, year);
  auto best_score = numeric_limits<MathSize>::min();
  unique_ptr<const EnvironmentInfo> env_info = nullptr;
  unique_ptr<GridBase> for_info = nullptr;
  string best_fuel;
  string best_elevation;
  auto found_best = false;
  if (!perimeter.empty())
  {
    for_info = make_unique<GridBase>(read_header(perimeter.canonical()));
    logging::info("Perimeter projection is {:s}", for_info->proj4());
  }
  for (const auto& raster : rasters)
  {
    auto fuel = raster;
    logging::verbose("Replacing directory separators in path for: {:s}\n", fuel);
    // make sure we're using a consistent directory separator
    std::replace(fuel.begin(), fuel.end(), '\\', '/');
    // HACK: assume there's only one instance of 'fuel' in the file name we want to change
    const auto find_what = string("fuel");
    const auto find_len = find_what.length();
    const auto find_start = fuel.find(find_what, fuel.find_last_of('/'));
    const auto elevation = string(fuel).replace(find_start, find_len, "dem");
    unique_ptr<const EnvironmentInfo> cur_info = EnvironmentInfo::loadInfo(fuel, elevation);
    // want the raster that's going to give us the most room to spread, so pick the one with the
    // most
    //   cells between the ignition and the edge on the side where it's closest to the edge
    // FIX: need to pick raster that aligns with perimeter if we have one
    //      -  for now at least ensure the same projection
    if (nullptr != for_info && 0 != strcmp(for_info->proj4().c_str(), cur_info->proj4().c_str()))
    {
      continue;
    }
    // FIX: just worrying about distance from specified lat/long for now, but should pick based on
    // bounds of perimeter flipped because we're reading from a raster so change (left, top) to
    // (left, bottom)
    const auto coordinates = cur_info->findFullCoordinates(point, true);
    if (coordinates.has_value())
    {
      auto actual_height = cur_info->calculateHeight();
      auto actual_width = cur_info->calculateWidth();
      const auto x = coordinates->x;
      const auto y = coordinates->y;
      logging::debug(
        "Coordinates before reading are ({:d}, {:d} => {:f}, {:f})",
        x,
        y,
        x + coordinates->x_sub / 1000.0,
        y + coordinates->y_sub / 1000.0
      );
      // if it's not in the raster then this is not an option
      // FIX: are these +/-1 because of counting the cell itself and starting from 0?
      const auto dist_W = x;
      const auto dist_E = actual_width - x;
      const auto dist_N = actual_height - y;
      const auto dist_S = y;
      // FIX: should take size of cells into account too? But is largest areas or highest resolution
      // the priority?
      logging::debug(
        "Coordinates distance to bottom left is: ({:d}, {:d}) and top right is ({:d}, {:d})",
        dist_W,
        dist_S,
        dist_E,
        dist_N
      );
      // shortest hypoteneuse is the closest corner to the origin, so want highest value for this
      const auto cur_score = sq(min(dist_W, dist_E)) + sq(min(dist_N, dist_S));
      if (cur_score > best_score)
      {
        best_score = cur_score;
        best_fuel = fuel;
        best_elevation = elevation;
        found_best = true;
      }
    }
  }
  if (nullptr == env_info && found_best)
  {
    logging::note("Loading info for fuel {:s}", best_fuel);
    env_info = EnvironmentInfo::loadInfo(best_fuel, best_elevation);
  }
  logging::check_fatal(nullptr == env_info, "Could not find an environment to use for {}", point);
  logging::debug("Best match for {} has projection '{:s}'", point, env_info->proj4());
  logging::note("Projection is {:s}", env_info->proj4());
  // envInfo should get deleted automatically because it uses unique_ptr
  return env_info->load(point);
}
std::optional<Coordinates> Environment::findCoordinates(const Point& point, const bool flipped)
  const
{
  return cells_.findCoordinates(point, flipped);
}
const BurnedData& Environment::unburnable() const { return not_burnable_; }
CellGrid Environment::makeCells(const FuelGrid& fuel, const ElevationGrid& elevation)
{
  logging::check_equal(fuel.yllcorner(), elevation.yllcorner(), "yllcorner");
  static Cell nodata{};
  auto values = vector<Cell>{fuel.data.size()};
  for (Idx y = 0; y < fuel.height(); ++y)
  {
    for (Idx x = 0; x < fuel.width(); ++x)
    {
      const XYIdx loc{x, y};
      if (y >= 0 && y < fuel.height() && x >= 0 && x < fuel.width())
      {
        // NOTE: this needs to translate to internal codes?
        const auto f = FuelType::safeCode(fuel.at(loc));
        auto s = static_cast<SlopeSize>(INVALID_SLOPE);
        auto a = static_cast<AspectSize>(INVALID_ASPECT);
        // HACK: don't calculate for outside box of cells
        if (y > 0 && y < fuel.height() - 1 && x > 0 && x < fuel.width() - 1)
        {
          MathSize dem[9];
          bool valid = true;
          for (int i = -1; i < 2; ++i)
          {
            for (int j = -1; j < 2; ++j)
            {
              // grid is (0, 0) at bottom left, but want [0] in array to be NW corner
              auto actual_y = static_cast<Idx>(y - i);
              auto actual_x = static_cast<Idx>(x + j);
              XYIdx cur_loc{actual_x, actual_y};
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
            const MathSize dx =
              ((dem[2] + dem[5] + dem[5] + dem[8]) - (dem[0] + dem[3] + dem[3] + dem[6]))
              / elevation.cellSize();
            const MathSize dy =
              ((dem[6] + dem[7] + dem[7] + dem[8]) - (dem[0] + dem[1] + dem[1] + dem[2]))
              / elevation.cellSize();
            const MathSize key = (dx * dx + dy * dy);
            auto slope_pct = static_cast<_Float32>(100 * (sqrt(key) / 8.0));
            s = min(
              static_cast<SlopeSize>(MAX_SLOPE_FOR_DISTANCE),
              static_cast<SlopeSize>(round(static_cast<MathSize>(slope_pct)))
            );
            static_assert(std::numeric_limits<SlopeSize>::max() >= MAX_SLOPE_FOR_DISTANCE);
            MathSize aspect_azimuth = 0.0;
            if (s > 0 && (dx != 0 || dy != 0))
            {
              aspect_azimuth = Radians{atan2(dy, -dx)}.asDegrees().value;
              // NOTE: need to change this out of 'math' direction into 'real' direction (i.e. N
              // is 0, not E)
              aspect_azimuth =
                (aspect_azimuth > 90.0) ? (450.0 - aspect_azimuth) : (90.0 - aspect_azimuth);
              if (aspect_azimuth == 360.0)
              {
                aspect_azimuth = 0.0;
              }
            }
            a = static_cast<AspectSize>(round(aspect_azimuth));
          }
        }
        const auto cell = Cell{s, a, f};
        // NOTE: this is going to be a vector that's the same size as the max size, despite the
        //       actual size of the contents
        values.at(to_index(loc)) = cell;
#ifdef DEBUG_GRIDS
#ifndef VLD_RPTHOOK_INSTALL
        const auto h = to_index(loc);
        const auto v = values.at(h);
        if (!(INVALID_SLOPE == cell.slope() || INVALID_ASPECT == cell.aspect()
              || INVALID_FUEL_CODE == cell.fuelCode()))
        {
          logging::check_equal(cell.slope(), s, "Cell slope");
          logging::check_equal(v.slope(), s, "Slope");
          if (0 != s)
          {
            logging::check_equal(cell.aspect(), a, "Cell aspect");
            logging::check_equal(v.aspect(), a, "Aspect");
          }
          else
          {
            logging::check_equal(
              cell.aspect(), static_cast<AspectSize>(a), "Cell aspect when slope is 0"
            );
            logging::check_equal(v.aspect(), static_cast<AspectSize>(0), "Aspect when slope is 0");
          }
          logging::check_equal(v.fuelCode(), f, "Fuel");
          logging::check_equal(cell.fuelCode(), f, "Cell fuel");
        }
        else
        {
          logging::check_equal(cell.slope(), INVALID_SLOPE, "Invalid Cell slope");
          logging::check_equal(cell.aspect(), INVALID_ASPECT, "Invalid Cell aspect");
          logging::check_equal(cell.fuelCode(), INVALID_FUEL_CODE, "Invalid Cell fuel");
          logging::check_equal(v.slope(), INVALID_SLOPE, "Invalid slope");
          logging::check_equal(v.aspect(), INVALID_ASPECT, "Invalid aspect");
          logging::check_equal(v.fuelCode(), INVALID_FUEL_CODE, "Invalid fuel");
        }
#endif
#endif
      }
    }
  }
  return CellGrid(
    fuel.cellSize(),
    fuel.width(),
    fuel.height(),
    nodata.fullHash(),
    nodata,
    fuel.xllcorner(),
    fuel.yllcorner(),
    fuel.xurcorner(),
    fuel.yurcorner(),
    string(fuel.proj4()),
    std::move(values)
  );
}
Environment::Environment(const FuelGrid& fuel, const ElevationGrid& elevation, const Point& point)
  : Environment(fuel, elevation, point, settings::instance())
{ }
Environment::Environment(
  const FuelGrid& fuel,
  const ElevationGrid& elevation,
  const Point& point,
  const Settings& settings
)
  : Environment(
      (settings.save_simulation_area ? make_unique<FuelGrid>(fuel) : nullptr),
      (settings.save_simulation_area ? make_unique<ElevationGrid>(elevation) : nullptr),
      makeCells(fuel, elevation),
      elevation.at([&]() {
        auto loc = elevation.findCoordinates(point, false);
        return XYIdx{loc->x, loc->y};
      }())
    )
{
  // take elevation at point so that if max grid size changes elevation doesn't
  logging::note("Start elevation is {:d}", elevation_);
}
Cell Environment::offset(const Event& event, const Idx x, const Idx y) const
{
  const auto& p = event.xy;
  // return cell(XYIdx{static_cast<Idx>(p.x().value + x), static_cast<Idx>(p.y().value + y)});
  return cell(p + XIdx{x} + YIdx{y});
}
void Environment::saveToFile(const string_view output_directory) const
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  static const auto& lookup = settings.fuel_lookup.lookup();
  if (settings.save_simulation_area)
  {
    logging::debug("Saving simulation area");
    auto convert_to_slope = [](const Cell& v) -> SlopeSize { return v.slope(); };
    auto convert_to_aspect = [](const Cell& v) -> AspectSize { return v.aspect(); };
    auto convert_to_area = [&](const Cell& v) -> SlopeSize {
      // need to still be nodata if it was
      return (v.slope() == INVALID_SLOPE) ? INVALID_SLOPE : 3;
    };
    // HACK: use original FuelGrid instead of cell value to ensure codes match input
    auto convert_to_fuelcode = [&](const FuelType* const value) -> FuelSize {
      return lookup.fuelToCode(value);
    };
    std::ignore = fuel_grid_->saveToFile<FuelSize>(output_directory, "fuel", convert_to_fuelcode);
    std::ignore = elevation_grid_->saveToFile<ElevationSize>(output_directory, "dem");
    // save slope & aspect grids
    std::ignore = cells_.saveToFile<SlopeSize>(
      output_directory, "slope", convert_to_slope, static_cast<SlopeSize>(INVALID_SLOPE)
    );
    std::ignore = cells_.saveToFile<AspectSize>(
      output_directory, "aspect", convert_to_aspect, static_cast<AspectSize>(INVALID_ASPECT)
    );
    // HACK: make a grid with "3" as the value so if we merge max with it it'll cover up anything
    // else
    std::ignore = cells_.saveToFile<ElevationSize>(
      output_directory, "simulation_area", convert_to_area, static_cast<SlopeSize>(INVALID_SLOPE)
    );
    logging::debug("Done saving simulation area grids");
  }
}
MathSize Environment::to_hectares(const size_t num_cells) const
{
  // we know that every cell is square, so figure out how many cells per ha
  const MathSize hectares_per_width = (this->cellSize() / 100.0);
  const MathSize hectares_per_cell = hectares_per_width * hectares_per_width;
  return num_cells * hectares_per_cell;
}
Environment::Environment(
  unique_ptr<FuelGrid> fuel_grid,
  unique_ptr<ElevationGrid> elevation_grid,
  CellGrid&& cells,
  const ElevationSize elevation
) noexcept
  : fuel_grid_(std::move(fuel_grid)), elevation_grid_(std::move(elevation_grid)), cells_(cells),
    not_burnable_{cells_}, elevation_(elevation)
{ }
}
