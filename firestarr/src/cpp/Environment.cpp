/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Environment.h"
#include "EnvironmentInfo.h"
#include "FuelLookup.h"
#include "ProbabilityMap.h"
#include "Scenario.h"
#include "Settings.h"

namespace fs
{
namespace topo
{
using util::sq;
Environment::~Environment()
{
  delete cells_;
}
Environment Environment::load(const string dir_out,
                              const Point& point,
                              const string& in_fuel,
                              const string& in_elevation)
{
  logging::note("Fuel raster is %s", in_fuel.c_str());
  logging::debug("Loading grids async");
  auto fuel = async(launch::async, [&in_fuel, &point]() { return FuelGrid::readTiff(in_fuel, point, sim::Settings::fuelLookup()); });
  auto elevation = async(launch::async, [&in_elevation, &point]() { return ElevationGrid::readTiff(in_elevation, point); });
  logging::debug("Waiting for grids");
  return Environment(dir_out,
                     *unique_ptr<FuelGrid>(fuel.get()),
                     *unique_ptr<ElevationGrid>(elevation.get()),
                     point);
}
sim::ProbabilityMap* Environment::makeProbabilityMap(const DurationSize time,
                                                     const DurationSize start_time) const
{
  return new sim::ProbabilityMap(dir_out_,
                                 time,
                                 start_time,
                                 *cells_);
}
Environment Environment::loadEnvironment(const string dir_out,
                                         const string& path,
                                         const Point& point,
                                         const string& perimeter,
                                         const int year)
{
  logging::note("Using ignition point (%f, %f)", point.latitude(), point.longitude());
  logging::info("Running using inputs directory '%s'", path.c_str());
  auto rasters = util::find_rasters(path, year);
  auto best_score = numeric_limits<MathSize>::min();
  unique_ptr<const EnvironmentInfo> env_info = nullptr;
  unique_ptr<data::GridBase> for_info = nullptr;
  string best_fuel = "";
  string best_elevation = "";
  auto found_best = false;
  if (!perimeter.empty())
  {
    for_info = make_unique<data::GridBase>(data::read_header(perimeter));
    logging::info("Perimeter projection is %s", for_info->proj4().c_str());
  }
  for (const auto& raster : rasters)
  {
    auto fuel = raster;
    logging::verbose("Replacing directory separators in path for: %s\n", fuel.c_str());
    // make sure we're using a consistent directory separator
    std::replace(fuel.begin(), fuel.end(), '\\', '/');
    // HACK: assume there's only one instance of 'fuel' in the file name we want to change
    const auto find_what = string("fuel");
    const auto find_len = find_what.length();
    const auto find_start = fuel.find(find_what, fuel.find_last_of('/'));
    const auto elevation = string(fuel).replace(find_start, find_len, "dem");
    unique_ptr<const EnvironmentInfo> cur_info = EnvironmentInfo::loadInfo(
      fuel,
      elevation);
    // want the raster that's going to give us the most room to spread, so pick the one with the most
    //   cells between the ignition and the edge on the side where it's closest to the edge
    // FIX: need to pick raster that aligns with perimeter if we have one
    //      -  for now at least ensure the same projection
    if (nullptr != for_info && 0 != strcmp(for_info->proj4().c_str(), cur_info->proj4().c_str()))
    {
      continue;
    }
    // FIX: just worrying about distance from specified lat/long for now, but should pick based on bounds of perimeter
    // flipped because we're reading from a raster so change (left, top) to (left, bottom)
    const auto coordinates = cur_info->findFullCoordinates(point, true);
    if (nullptr != coordinates)
    {
      auto actual_rows = cur_info->calculateRows();
      auto actual_columns = cur_info->calculateColumns();
      const auto x = std::get<0>(*coordinates);
      const auto y = std::get<1>(*coordinates);
      logging::note("Coordinates before reading are (%d, %d => %f, %f)",
                    x,
                    y,
                    x + std::get<2>(*coordinates) / 1000.0,
                    y + std::get<3>(*coordinates) / 1000.0);
      // if it's not in the raster then this is not an option
      // FIX: are these +/-1 because of counting the cell itself and starting from 0?
      const auto dist_W = x;
      const auto dist_E = actual_columns - x;
      const auto dist_N = actual_rows - y;
      const auto dist_S = y;
      // FIX: should take size of cells into account too? But is largest areas or highest resolution the priority?
      logging::note(
        "Coordinates distance to bottom left is: (%d, %d) and top right is (%d, %d)",
        dist_W,
        dist_S,
        dist_E,
        dist_N);
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
    env_info = EnvironmentInfo::loadInfo(
      best_fuel,
      best_elevation);
  }
  logging::check_fatal(
    nullptr == env_info,
    "Could not find an environment to use for (%f, %f)",
    point.latitude(),
    point.longitude());
  logging::debug(
    "Best match for (%f, %f) has projection '%s'",
    point.latitude(),
    point.longitude(),
    env_info->proj4().c_str());
  // envInfo should get deleted automatically because it uses unique_ptr
  return env_info->load(dir_out, point);
}
unique_ptr<Coordinates> Environment::findCoordinates(const Point& point,
                                                     const bool flipped) const
{
  return cells_->findCoordinates(point, flipped);
}
}
}
