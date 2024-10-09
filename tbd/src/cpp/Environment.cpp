/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Environment.h"
#include "EnvironmentInfo.h"
#include "FuelLookup.h"
#include "ProbabilityMap.h"
#include "Scenario.h"
#include "Settings.h"

namespace tbd
{
namespace topo
{
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
  if (sim::Settings::runAsync())
  {
    logging::debug("Loading grids async");
    auto fuel = async(launch::async, [&in_fuel, &point]() { return FuelGrid::readTiff(in_fuel, point, sim::Settings::fuelLookup()); });
    auto elevation = async(launch::async, [&in_elevation, &point]() { return ElevationGrid::readTiff(in_elevation, point); });
    logging::debug("Waiting for grids");
    return Environment(dir_out,
                       *unique_ptr<FuelGrid>(fuel.get()),
                       *unique_ptr<ElevationGrid>(elevation.get()),
                       point);
  }
  logging::warning("Loading grids synchronously");
  // HACK: need to copy strings since closures do that above
  return Environment(dir_out,
                     *unique_ptr<FuelGrid>(
                       FuelGrid::readTiff(string(in_fuel), point, sim::Settings::fuelLookup())),
                     *unique_ptr<ElevationGrid>(
                       ElevationGrid::readTiff(string(in_elevation), point)),
                     point);
}
sim::ProbabilityMap* Environment::makeProbabilityMap(const DurationSize time,
                                                     const DurationSize start_time,
                                                     const int min_value,
                                                     const int low_max,
                                                     const int med_max,
                                                     const int max_value) const
{
  return new sim::ProbabilityMap(dir_out_,
                                 time,
                                 start_time,
                                 min_value,
                                 low_max,
                                 med_max,
                                 max_value,
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
  auto best_x = numeric_limits<MathSize>::max();
  auto best_y = numeric_limits<MathSize>::max();
  unique_ptr<const EnvironmentInfo> env_info = nullptr;
  unique_ptr<data::GridBase> for_info = nullptr;
  string best_fuel = "";
  string best_elevation = "";
  MathSize best_meridian = numeric_limits<MathSize>::max();
  auto found_best = false;
  if (!perimeter.empty())
  {
    for_info = make_unique<data::GridBase>(data::read_header(perimeter));
    logging::info("Perimeter projection is %s", for_info->proj4().c_str());
  }
  for (const auto& raster : rasters)
  {
    auto fuel = raster;
    printf("Replacing directory separators in path for: %s\n", fuel.c_str());
    // make sure we're using a consistent directory separator
    std::replace(fuel.begin(), fuel.end(), '\\', '/');
    // HACK: assume there's only one instance of 'fuel' in the file name we want to change
    const auto find_what = string("fuel");
    const auto find_len = find_what.length();
    const auto find_start = fuel.find(find_what, fuel.find_last_of('/'));
    const auto elevation = string(fuel).replace(find_start, find_len, "dem");
    // figure out best raster based on meridian by filename first
    const auto find_zone_start = find_start + find_len + 1;
    const auto find_suffix = fuel.find_last_of(".");
    auto zone_guess = fuel.substr(find_zone_start, find_suffix - find_zone_start);
    std::replace(zone_guess.begin(), zone_guess.end(), '_', '.');
    logging::debug("Assuming file %s is for zone %s", fuel.c_str(), zone_guess.c_str());
    MathSize zone;
    MathSize meridian;
    unique_ptr<const EnvironmentInfo> cur_info;
    if (sscanf(zone_guess.c_str(), "%lf", &zone) && !sim::Settings::forceFuel())
    {
      meridian = (zone - 15.0) * 6.0 - 93.0;
    }
    else
    {
      cur_info = EnvironmentInfo::loadInfo(
        fuel,
        elevation);
      zone = cur_info->zone();
      meridian = cur_info->meridian();
    }
    if (sim::Settings::forceFuel())
    {
      logging::verbose("FORCING ENV");
      env_info = std::move(cur_info);
      break;
    }
    const auto cur_x = abs(point.longitude() - meridian);
    logging::verbose("Zone %0.1f meridian is %0.2f degrees from point",
                     zone,
                     cur_x);
    // HACK: assume floating point is going to always be exactly the same result
    if (cur_x < best_x)
    {
      logging::verbose("SWITCH X");
      best_x = cur_x;
      best_fuel = fuel;
      best_elevation = elevation;
      best_meridian = meridian;
      found_best = true;
      // if already loaded then keep
      if (nullptr != cur_info)
      {
        logging::verbose("CHECK Y");
        const auto coordinates = cur_info->findFullCoordinates(point, false);
        const auto cur_y = static_cast<FullIdx>(abs(
          std::get<0>(*coordinates) - cur_info->calculateRows() / static_cast<FullIdx>(2)));
        logging::verbose(("Current y value is " + std::to_string(cur_y)).c_str());
        if (cur_y < best_y)
        {
          logging::verbose("SWITCH Y");
          env_info = std::move(cur_info);
          best_y = cur_y;
        }
      }
      else
      {
        logging::verbose("NULLPTR");
      }
    }
  }
  if (nullptr == env_info && found_best)
  {
    env_info = EnvironmentInfo::loadInfo(
      best_fuel,
      best_elevation);
  }
  logging::check_fatal(nullptr == env_info,
                       "Could not find an environment to use for (%f, %f)",
                       point.latitude(),
                       point.longitude());
  if (!sim::Settings::forceFuel())
  {
    logging::check_fatal(best_meridian != env_info->meridian(),
                         "Thought file with best match was for meridian %ld but is actually %ld",
                         best_meridian,
                         env_info->meridian());
  }
  logging::debug("Best match for (%f, %f) is zone %0.2f with a meridian of %0.2f",
                 point.latitude(),
                 point.longitude(),
                 env_info->zone(),
                 env_info->meridian());
  logging::note("Projection is %s", env_info->proj4().c_str());
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
