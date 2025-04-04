/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "EnvironmentInfo.h"
#include "Environment.h"
#include "Settings.h"
namespace tbd
{
namespace topo
{
EnvironmentInfo::~EnvironmentInfo() = default;
EnvironmentInfo::EnvironmentInfo(string in_fuel,
                                 string in_elevation,
                                 data::GridBase&& fuel,
                                 data::GridBase&& elevation) noexcept
  : fuel_(std::move(fuel)),
    elevation_(std::move(elevation)),
    in_fuel_(std::move(in_fuel)),
    in_elevation_(std::move(in_elevation))
{
  logging::debug("fuel: %dx%d => (%f, %f)", fuel.calculateColumns(), fuel.calculateRows(), fuel.xllcorner(), fuel.yllcorner());
  logging::debug("elevation: %dx%d => (%f, %f)", elevation.calculateColumns(), elevation.calculateRows(), elevation.xllcorner(), elevation.yllcorner());
  logging::check_fatal(!(fuel.calculateRows() == elevation.calculateRows()
                         && fuel.calculateColumns() == elevation.calculateColumns()
                         && fuel.cellSize() == elevation.cellSize()
                         && fuel.xllcorner() == elevation.xllcorner()
                         && fuel.yllcorner() == elevation.yllcorner()),
                       "Grids are not aligned");
}
EnvironmentInfo::EnvironmentInfo(const string& in_fuel,
                                 const string& in_elevation)
  : EnvironmentInfo(in_fuel,
                    in_elevation,
                    data::read_header(in_fuel),
                    data::read_header(in_elevation))
{
}
unique_ptr<EnvironmentInfo> EnvironmentInfo::loadInfo(const string& in_fuel,
                                                      const string& in_elevation)
{
  if (sim::Settings::runAsync())
  {
    auto fuel_async = async(launch::async, [in_fuel]() { return data::read_header(in_fuel); });
    auto elevation_async = async(launch::async, [in_elevation]() { return data::read_header(in_elevation); });
    const auto e = new EnvironmentInfo(in_fuel,
                                       in_elevation,
                                       fuel_async.get(),
                                       elevation_async.get());
    return unique_ptr<EnvironmentInfo>(e);
  }
  const auto e = new EnvironmentInfo(in_fuel,
                                     in_elevation,
                                     data::read_header(in_fuel),
                                     data::read_header(in_elevation));
  return unique_ptr<EnvironmentInfo>(e);
}
Environment EnvironmentInfo::load(const string dir_out, const Point& point) const
{
  return Environment::load(dir_out, point, in_fuel_, in_elevation_);
}
unique_ptr<Coordinates> EnvironmentInfo::findCoordinates(
  const Point& point,
  const bool flipped) const
{
  return fuel_.findCoordinates(point, flipped);
}
unique_ptr<FullCoordinates> EnvironmentInfo::findFullCoordinates(
  const Point& point,
  const bool flipped) const
{
  return fuel_.findFullCoordinates(point, flipped);
}
}
}
