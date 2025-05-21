/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "UTM.h"
#include "Point.h"
#include "Log.h"
#include "Util.h"
#include "unstable.h"

#include <proj.h>
#include "stdafx.h"
#include "Util.h"
namespace fs::topo
{
class Point;
PJ* normalized_context(PJ_CONTEXT* C, const string& proj4_from, const string& proj4_to)
{
  // FIX: this is WGS84, but do we need to support more than that for lat/long
  PJ* P = proj_create_crs_to_crs(
    C,
    proj4_from.c_str(),
    proj4_to.c_str(),
    nullptr);
  fs::logging::check_fatal(0 == P, "Failed to create transformation object");
  // This will ensure that the order of coordinates for the input CRS
  // will be longitude, latitude, whereas EPSG:4326 mandates latitude,
  // longitude
  PJ* P_norm = proj_normalize_for_visualization(C, P);
  fs::logging::check_fatal(0 == P_norm, "Failed to normalize transformation object");
  proj_destroy(P);
  return P_norm;
}
void from_lat_long(
  const string& proj4,
  const fs::topo::Point& point,
  MathSize* x,
  MathSize* y)
{
  // see https://proj.org/en/stable/development/quickstart.html
  // do this in a function so we can hide and clean up intial context
  PJ_CONTEXT* C = proj_context_create();
  auto P = normalized_context(C, "EPSG:4326", proj4);
  // Given that we have used proj_normalize_for_visualization(), the order
  // of coordinates is longitude, latitude, and values are expressed in
  // degrees.
  const PJ_COORD a = proj_coord(point.longitude(), point.latitude(), 0, 0);
  // transform to UTM, then back to geographical
  const PJ_COORD b = proj_trans(P, PJ_FWD, a);
  *x = b.enu.e;
  *y = b.enu.n;
  // #ifdef DEBUG_PROJ
  PJ_COORD c = proj_trans(P, PJ_INV, b);
  fs::logging::verbose(
    "longitude: %f, latitude: %f => easting: %.3f, northing: %.3f => x: %.3f, y: %.3f => longitude: %g, latitude: %g",
    point.longitude(),
    point.latitude(),
    b.enu.e,
    b.enu.n,
    b.xy.x,
    b.xy.y,
    c.lp.lam,
    c.lp.phi);
  // #endif
  proj_destroy(P);
  proj_context_destroy(C);
}
fs::topo::Point to_lat_long(
  const string& proj4,
  const MathSize x,
  const MathSize y)
{
  // see https://proj.org/en/stable/development/quickstart.html
  // do this in a function so we can hide and clean up intial context
  PJ_CONTEXT* C = proj_context_create();
  auto P = normalized_context(C, proj4, "EPSG:4326");
  // Given that we have used proj_normalize_for_visualization(), the order
  // of coordinates is longitude, latitude, and values are expressed in
  // degrees.
  logging::verbose("proj_coord(%f, %f, 0, 0)", x, y);
  const PJ_COORD a = proj_coord(x, y, 0, 0);
  // transform to  geographical
  const PJ_COORD b = proj_trans(P, PJ_FWD, a);
  // Point is (lat, lon)
  Point point{b.lp.phi, b.lp.lam};
  proj_destroy(P);
  proj_context_destroy(C);
  return point;
}
string&& try_fix_meridian(string&& proj4)
{
  const auto zone_pos = proj4.find("+zone=");
  // if proj4 is defined by zone then convert to be defined by meridian
  if (string::npos != zone_pos && string::npos != proj4.find("+proj=utm"))
  {
    // NOTE: using proj for actual projections, but we want proj4 strings to use meridian
    //       and not zone so outputs are consistent regardless of input
    // convert from utm zone to tmerc
    const auto zone_str = proj4.substr(zone_pos + 6);
    const auto zone = stoi(zone_str);
    // zone 15 is -93 and other zones are 6 degrees difference
    const auto degrees = fs::topo::utm_central_meridian(zone);
    // HACK: assume utm zone is at start
    proj4 = string(
      "+proj=tmerc +lat_0=0.000000000 +lon_0=" + to_string(degrees) + " +k=0.999600 +x_0=500000.000 +y_0=0.000");
    logging::verbose("Adjusted proj4 is %s\n", proj4.c_str());
  }
  return std::forward<string>(proj4);
}
}
