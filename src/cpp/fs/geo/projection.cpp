/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "projection.h"
#include <proj.h>
#include "../log/Log.h"
#include "../sim/Settings.h"
#include "../types/Radians.h"
#include "../unstable.h"
#include "../util/Util.h"
#include "Point.h"
namespace fs
{
std::optional<FullCoordinates> to_proj4(
  const string& proj4,
  const fs::Point& point,
  MathSize* x,
  MathSize* y
)
{
  PJ_CONTEXT* C;
  PJ* P;
  PJ* norm;
  PJ_COORD a, b, c;
  /* or you may set C=PJ_DEFAULT_CTX if you are sure you will     */
  /* use PJ objects from only one thread                          */
  C = proj_context_create();
  P = proj_create_crs_to_crs(C, "EPSG:4326", proj4.c_str(), nullptr);
  if (nullptr == P)
  {
    std::cerr << "Failed to create transformation object.\n";
    return {};
  }
  /* This will ensure that the order of coordinates for the input CRS */
  /* will be longitude, latitude, whereas EPSG:4326 mandates latitude, */
  /* longitude */
  norm = proj_normalize_for_visualization(C, P);
  if (nullptr == norm)
  {
    std::cerr << "Failed to normalize transformation object.\n";
    return {};
  }
  proj_destroy(P);
  P = norm;
  /* Given that we have used proj_normalize_for_visualization(), the order */
  /* of coordinates is longitude, latitude, and values are expressed in */
  /* degrees. */
  a = proj_coord(point.longitude(), point.latitude(), 0, 0);
  /* transform to UTM, then back to geographical */
  b = proj_trans(P, PJ_FWD, a);
  *x = b.enu.e;
  *y = b.enu.n;
  c = proj_trans(P, PJ_INV, b);
  cout << std::format(
    "{} => easting: {:.3f}, northing: {:.3f} => x: {:.3f}, y: {:.3f} => longitude: {:g}, latitude: {:g}\n",
    point,
    b.enu.e,
    b.enu.n,
    b.xy.x,
    b.xy.y,
    c.lp.lam,
    c.lp.phi
  );
  /* Clean up */
  proj_destroy(P);
  proj_context_destroy(C); /* may be omitted in the single threaded case */
  return {};
}
PJ_CONTEXT* get_context()
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  auto pjc = proj_context_create();
  string db_path = settings.getBinaryDirectory() + "proj.db";
  // HACK: only do once
  static const auto showed_once = [&]() {
    logging::debug("Trying to set db path to {:s}", db_path);
    return true;
  }();
  std::ignore = showed_once;
  if (!proj_context_set_database_path(pjc, db_path.c_str(), nullptr, nullptr))
  {
    exit(logging::fatal("Can't set proj.db path"));
  }
  static const auto showed_success_once = [&]() {
    logging::verbose("Succeeded trying to set db path to {:s}", db_path);
    return true;
  }();
  std::ignore = showed_success_once;
  return pjc;
}
class Point;
PJ* normalized_context(PJ_CONTEXT* C, const string_view proj4_from, const string_view proj4_to)
{
  // FIX: this is WGS84, but do we need to support more than that for lat/long
  PJ* P = proj_create_crs_to_crs(C, string(proj4_from).c_str(), string(proj4_to).c_str(), nullptr);
  fs::logging::check_fatal(nullptr == P, "Failed to create transformation object");
  // This will ensure that the order of coordinates for the input CRS
  // will be longitude, latitude, whereas EPSG:4326 mandates latitude,
  // longitude
  PJ* P_norm = proj_normalize_for_visualization(C, P);
  fs::logging::check_fatal(nullptr == P_norm, "Failed to normalize transformation object");
  proj_destroy(P);
  return P_norm;
}
fs::XYPos from_lat_long(const string_view proj4, const fs::Point& point)
{
  // see https://proj.org/en/stable/development/quickstart.html
  // do this in a function so we can hide and clean up intial context
  PJ_CONTEXT* C = get_context();
  auto P = normalized_context(C, "EPSG:4326", proj4);
  // Given that we have used proj_normalize_for_visualization(), the order
  // of coordinates is longitude, latitude, and values are expressed in
  // degrees.
  const PJ_COORD a = proj_coord(point.longitude(), point.latitude(), 0, 0);
  // transform to UTM, then back to geographical
  const PJ_COORD b = proj_trans(P, PJ_FWD, a);
  XYPos result{XPos{b.enu.e}, YPos{b.enu.n}};
  // #ifdef DEBUG_PROJ
  PJ_COORD c = proj_trans(P, PJ_INV, b);
  fs::logging::verbose(
    "{} => easting: {:.3f}, northing: {:.3f} => x: {:.3f}, y: {:.3f} => longitude: {:g}, latitude: {:g}",
    point,
    b.enu.e,
    b.enu.n,
    b.xy.x,
    b.xy.y,
    c.lp.lam,
    c.lp.phi
  );
  // #endif
  proj_destroy(P);
  proj_context_destroy(C);
  return result;
}
fs::Point to_lat_long(const string_view proj4, const fs::XYPos xy)
{
  // see https://proj.org/en/stable/development/quickstart.html
  // do this in a function so we can hide and clean up intial context
  PJ_CONTEXT* C = get_context();
  auto P = normalized_context(C, proj4, "EPSG:4326");
  // Given that we have used proj_normalize_for_visualization(), the order
  // of coordinates is longitude, latitude, and values are expressed in
  // degrees.
  logging::verbose("proj_coord({:f}, {:f}, 0, 0)", xy.x.value, xy.y.value);
  const PJ_COORD a = proj_coord(xy.x.value, xy.y.value, 0, 0);
  // transform to  geographical
  const PJ_COORD b = proj_trans(P, PJ_FWD, a);
  // Point is (lat, lon)
  Point point{b.lp.phi, b.lp.lam};
  proj_destroy(P);
  proj_context_destroy(C);
  return point;
}
string try_fix_meridian(const string_view proj4)
{
  const auto proj = find_value("+proj=", proj4);
  if (0 != strcmp("tmerc", proj.c_str()) && 0 != strcmp("utm", proj.c_str()))
  {
    logging::debug("try_fix_meridian() has no effect on non utm/tmerc projections");
    return string(proj4);
  }
  const auto [lat_0, lon_0, k, x_0, y_0] =
    [&]() -> std::tuple<MathSize, MathSize, MathSize, MathSize, MathSize> {
    if (0 == strcmp("utm", proj.c_str()))
    {
      const auto zone = find_value("+zone=", proj4);
      // zone 15 is -93 and other zones are 6 degrees difference
      const auto z = stod(zone);
      const auto lon_0 = fs::utm_central_meridian(z);
      logging::debug("UTM zone {:s} == {:f} turned into meridian {:f}", zone, z, lon_0);
      return {0.0, lon_0, 0.9996, 500000.0, 0.0};
    }
    // HACK: only do once
    static const auto showed_once = [&]() {
      logging::debug("Using existing values for tmerc");
      return true;
    }();
    std::ignore = showed_once;
    const auto lat_0 = find_value("+lat_0=", proj4);
    const auto lon_0 = find_value("+lon_0=", proj4);
    const auto k = find_value("+k=", proj4);
    const auto x_0 = find_value("+x_0=", proj4);
    const auto y_0 = find_value("+y_0=", proj4);
    return {stod(lat_0), stod(lon_0), stod(k), stod(x_0), stod(y_0)};
  }();
  const auto ellps = find_value("+ellps=", proj4, "GRS80");
  const auto units = find_value("+units=", proj4, "m");
  return std::format(
    "+proj={} +lat_0={:0.9f} +lon_0={:0.9f} +k={:0.9f} +x_0={:0.9f} +y_0={:0.9f} +ellps={} +units={}",
    "tmerc",
    lat_0,
    lon_0,
    k,
    x_0,
    y_0,
    ellps,
    units
  );
}
Degrees find_north_south_deviation(const string_view proj4, const Point& p0)
{
  constexpr auto MAX_LATITUDE{89.9};
  constexpr auto LATITUDE_DISTANCE{1};
  const auto lat1 = min(MAX_LATITUDE, p0.latitude() + LATITUDE_DISTANCE);
  Point p1{lat1, p0.longitude()};
  auto grid0 = from_lat_long(proj4, p0);
  auto grid1 = from_lat_long(proj4, p1);
  logging::verbose(
    "Finding deviation between [{} => ({:f}, {:f})] and [{} => ({:f}, {:f})]",
    p0,
    grid0.x.value,
    grid0.y.value,
    p1,
    grid1.x.value,
    grid1.y.value
  );
  // angle is going to be how far off North we are
  // -90 to convert from math direction
  auto deviation =
    Radians::D_090().asDegrees()
    - Radians{atan2(grid1.y.value - grid0.y.value, grid1.x.value - grid0.x.value)}.asDegrees();
  logging::verbose(
    "Deviation for {} to {} with proj4 '{:s}' is {:f} degrees", p0, p1, proj4, deviation.value
  );
  return deviation;
};
bool check_deviation(
  const string_view what,
  const string_view proj4,
  const Point& p,
  const MathSize max_deviation
)
{
  auto deviation = find_north_south_deviation(proj4, p).value;
  if (abs(deviation) > max_deviation)
  {
    logging::error(
      "Grid North of {:s} {} deviates from true North by {:g} degrees which exceeds maximum of {:g} degrees",
      what,
      p,
      deviation,
      max_deviation
    );
    return false;
  }
  else if (abs(deviation * 2) > max_deviation)
  {
    // if close to max deviation then warn about it
    logging::warning(
      "Grid North of {:s} {} deviates from true North by {:g} degrees which is near maximum of {:f} degrees",
      what,
      p,
      deviation,
      max_deviation
    );
  }
  else
  {
    logging::debug(
      "Grid North of {:s} {} deviates from true North by {:g} degrees",
      what,
      p,
      deviation,
      max_deviation
    );
  }
  return true;
};
}
