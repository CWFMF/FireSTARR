/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "EnvironmentInfo.h"
#include "../geo/Point.h"
#include "../log/Log.h"
#include "../sim/Settings.h"
#include "Environment.h"
#include "Grid.h"
namespace fs
{
EnvironmentInfo::~EnvironmentInfo() = default;
EnvironmentInfo::EnvironmentInfo(
  const string_view in_fuel,
  const string_view in_elevation,
  GridBase&& fuel,
  GridBase&& elevation
) noexcept
  : fuel_(fuel), elevation_(elevation), in_fuel_(std::move(in_fuel)),
    in_elevation_(std::move(in_elevation))
{
  logging::debug(
    "fuel: {:d}{:d} => ({:f}, {:f})",
    fuel.calculateWidth(),
    fuel.calculateHeight(),
    fuel.xllcorner(),
    fuel.yllcorner()
  );
  logging::debug(
    "elevation: {:d}{:d} => ({:f}, {:f})",
    elevation.calculateWidth(),
    elevation.calculateHeight(),
    elevation.xllcorner(),
    elevation.yllcorner()
  );
  logging::check_fatal(
    !(fuel.calculateHeight() == elevation.calculateHeight()
      && fuel.calculateWidth() == elevation.calculateWidth()
      && fuel.cellSize() == elevation.cellSize() && fuel.xllcorner() == elevation.xllcorner()
      && fuel.yllcorner() == elevation.yllcorner()),
    "Grids are not aligned"
  );
}
EnvironmentInfo::EnvironmentInfo(const string_view in_fuel, const string_view in_elevation)
  : EnvironmentInfo(in_fuel, in_elevation, read_header(in_fuel), read_header(in_elevation))
{ }
unique_ptr<EnvironmentInfo> EnvironmentInfo::loadInfo(
  const string_view in_fuel,
  const string_view in_elevation
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  if (settings.run_async)
  {
    auto fuel_async = async(launch::async, [in_fuel]() { return read_header(in_fuel); });
    auto elevation_async =
      async(launch::async, [in_elevation]() { return read_header(in_elevation); });
    const auto e =
      new EnvironmentInfo(in_fuel, in_elevation, fuel_async.get(), elevation_async.get());
    return unique_ptr<EnvironmentInfo>(e);
  }
  const auto e =
    new EnvironmentInfo(in_fuel, in_elevation, read_header(in_fuel), read_header(in_elevation));
  return unique_ptr<EnvironmentInfo>(e);
}
Environment EnvironmentInfo::load(const Point& point) const
{
  if (!validate(point))
  {
    logging::fatal("Grid does not meet requirements for simulation to be valid");
  }
  return Environment::load(point, in_fuel_, in_elevation_);
}
std::optional<Coordinates> EnvironmentInfo::findCoordinates(const Point& point, const bool flipped)
  const
{
  return fuel_.findCoordinates(point, flipped);
}
std::optional<FullCoordinates> EnvironmentInfo::findFullCoordinates(
  const Point& point,
  const bool flipped
) const
{
  return fuel_.findFullCoordinates(point, flipped);
}
[[nodiscard]] bool EnvironmentInfo::validate(const Point& point) const
{
  return fuel_.validate(point);
}
}
