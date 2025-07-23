/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "pts.h"
#ifdef USE_NEW_SPREAD
#include "Log.h"
#include "Location.h"
#include "Scenario.h"

namespace fs::sim
{
constexpr InnerSize DIST_22_5 = static_cast<InnerSize>(0.2071067811865475244008443621048490392848359376884740365883398689);
constexpr InnerSize P_0_5 = static_cast<InnerSize>(0.5) + DIST_22_5;
constexpr InnerSize M_0_5 = static_cast<InnerSize>(0.5) - DIST_22_5;
//   static constexpr auto INVALID_DISTANCE = std::numeric_limits<InnerSize>::max();
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const auto INVALID_DISTANCE = static_cast<DistanceSize>(MAX_ROWS * MAX_ROWS);
static const XYPos INVALID_XY_POSITION{};
static const pair<DistanceSize, XYPos> INVALID_XY_PAIR{INVALID_DISTANCE, INVALID_XY_POSITION};
static const XYSize INVALID_XY_LOCATION = INVALID_XY_PAIR.second.x();
static const InnerPos INVALID_INNER_POSITION{};
static const pair<DistanceSize, InnerPos> INVALID_INNER_PAIR{INVALID_DISTANCE, {}};
static const InnerSize INVALID_INNER_LOCATION = INVALID_INNER_PAIR.second.x();
static const SpreadData INVALID_SPREAD_DATA{
  INVALID_TIME};

using DISTANCE_PAIR = pair<DistanceSize, DistanceSize>;
#define D_PTS(x, y) (DISTANCE_PAIR{static_cast<DistanceSize>(x), static_cast<DistanceSize>(y)})
constexpr std::array<DISTANCE_PAIR, NUM_DIRECTIONS> POINTS_OUTER{
  D_PTS(0.5, 1.0),
  // north-northeast is closest to point (0.5 + 0.207, 1.0)
  D_PTS(P_0_5, 1.0),
  // northeast is closest to point (1.0, 1.0)
  D_PTS(1.0, 1.0),
  // east-northeast is closest to point (1.0, 0.5 + 0.207)
  D_PTS(1.0, P_0_5),
  // east is closest to point (1.0, 0.5)
  D_PTS(1.0, 0.5),
  // east-southeast is closest to point (1.0, 0.5 - 0.207)
  D_PTS(1.0, M_0_5),
  // southeast is closest to point (1.0, 0.0)
  D_PTS(1.0, 0.0),
  // south-southeast is closest to point (0.5 + 0.207, 0.0)
  D_PTS(P_0_5, 0.0),
  // south is closest to point (0.5, 0.0)
  D_PTS(0.5, 0.0),
  // south-southwest is closest to point (0.5 - 0.207, 0.0)
  D_PTS(M_0_5, 0.0),
  // southwest is closest to point (0.0, 0.0)
  D_PTS(0.0, 0.0),
  // west-southwest is closest to point (0.0, 0.5 - 0.207)
  D_PTS(0.0, M_0_5),
  // west is closest to point (0.0, 0.5)
  D_PTS(0.0, 0.5),
  // west-northwest is closest to point (0.0, 0.5 + 0.207)
  D_PTS(0.0, P_0_5),
  // northwest is closest to point (0.0, 1.0)
  D_PTS(0.0, 1.0),
  // north-northwest is closest to point (0.5 - 0.207, 1.0)
  D_PTS(M_0_5, 1.0)};

constexpr inline auto dist_line(const auto& a, const auto& b)
{
  return ((a - b) * (a - b));
}
Pts::Pts()
  : cell_x_y_(INVALID_INDEX, INVALID_INDEX)
{
  // // actually only need to set first value to denote empty
  // distances()[0] = INVALID_DIRECTION;
  std::fill_n(
    &(distances()[0]),
    NUM_DIRECTIONS,
    INVALID_DISTANCE);
  std::fill_n(
    &(points()[0]),
    NUM_DIRECTIONS,
    INVALID_INNER_POSITION);
}
Pts::Pts(const IntensityMap& intensity_map, const XYPos p)
  : Pts()
{
  cell_x_y_ =
    {static_cast<Idx>(p.x()),
     static_cast<Idx>(p.y())};
  // HACK: assign value of first item if burnable
  if (!(*intensity_map.unburnable_)[p.hash()])
  {
    distances()[0] = INVALID_DISTANCE - 1;
  }
  logging::check_equal(
    canBurn(),
    !(*intensity_map.unburnable_)[p.hash()],
    "can burn");
  insert(p);
#ifdef DEBUG_NEW_SPREAD_CHECK
  logging::check_equal(
    static_cast<size_t>(
      canBurn()
        ? 1
        : 0),
    unique().size(),
    "Initial size");
#endif
  // if (!intensity_map.unburnable_[p.hash()])
  // {
  //   XYSize integral;
  //   // need to calculate distances, but we know everything is the same point
  //   const auto x0 = static_cast<DistanceSize>(modf(p.x(), &integral));
  //   const auto y0 = static_cast<DistanceSize>(modf(p.y(), &integral));
  //   const InnerPos p0{x0, y0};
  //   auto& pts = points();
  //   auto& dists = distances();
  //   std::fill_n(&(pts[0]), NUM_DIRECTIONS, p0);
  //   for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
  //   {
  //     const auto& p1 = POINTS_OUTER[i];
  //     const auto& x1 = p1.first;
  //     const auto& y1 = p1.second;
  //     dists[i] = dist_line(x0, x1) + dist_line(y0, y1);
  //   }
  // }
  // // else
  // // {
  // //   // actually only need to set first value
  // //   distances()[0] = INVALID_DIRECTION;
  // //   // std::fill_n(
  // //   //   &(distances()[0]),
  // //   //   NUM_DIRECTIONS,
  // //   //   INVALID_DIRECTION);
  // // }
}
Pts& Pts::insert(const XYSize x,
                 const XYSize y)
{
#ifdef DEBUG_CELLPOINTS
  const auto n0 = size();
  {
    const set<XYPos> u = unique();
    show_points<XYPos, XYSize, std::set<XYPos>>(
      u,
      "Pts: Adding (%f, %f) to %ld points",
      x,
      y,
      n0);
  }
#endif
  logging::verbose(
    "Pts: (%d, %d) %s burn",
    cell_x_y_.x(),
    cell_x_y_.y(),
    canBurn() ? "can" : "cannot");
  if (canBurn())
  {
    XYSize integral;
    // need to calculate distances, but we know everything is the same point
    const auto x0 = static_cast<DistanceSize>(modf(x, &integral));
    const auto y0 = static_cast<DistanceSize>(modf(y, &integral));
    const InnerPos p0{x0, y0};
    // std::fill_n(&(points()[0]), NUM_DIRECTIONS, p0);
    auto& dists = distances();
    auto& pts = points();
    for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
    {
      const auto& p1 = POINTS_OUTER[i];
      const auto& x1 = p1.first;
      const auto& y1 = p1.second;
      const auto d = dist_line(x0, x1) + dist_line(y0, y1);
      auto& p_d = dists[i];
      auto& p_p = pts[i];
#ifdef DEBUG_NEW_SPREAD_VERBOSE
      if (d < p_d)
      {
        logging::verbose(
          "Pts: Inserting (%f, %f) into (%d, %d) as %s: (%f, %f)",
          x,
          y,
          cell_x_y_.x(),
          cell_x_y_.y(),
          DIRECTION_NAMES[i],
          p0.x(),
          p0.y());
      }
#endif
      p_p = (d < p_d) ? p0 : p_p;
      p_d = (d < p_d) ? d : p_d;
    }
  }
#ifdef DEBUG_CELLPOINTS
  const auto n1 = size();
  {
    const set<XYPos> u = unique();
    show_points<XYPos, XYSize, std::set<XYPos>>(
      u,
      "Pts: Adding (%f, %f) to %ld points gives %ld",
      x,
      y,
      n0,
      n1);
  }
#endif
  return *this;
}
inline bool Pts::canBurn() const
{
  return (INVALID_DISTANCE != distances()[0]);
}
bool Pts::empty() const
{
  // NOTE: if anything is invalid then everything must be
  //   return (INVALID_DISTANCE == distances()[0]);
  return !canBurn();
}

Pts& PtMap::insert(const IntensityMap& intensity_map, const XYPos p0)
{
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::verbose(
    "Pts::insert: (%f, %f) %s burn",
    p0.x(),
    p0.y(),
    !intensity_map.unburnable_[p0.hash()] ? "can" : "cannot");
#endif
  auto p = map_.try_emplace(
    p0.hash(),
    intensity_map,
    p0);
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::verbose(
    "Pts::insert: %s (%f, %f)",
    p.second ? "emplaced" : "inserting",
    p0.x(),
    p0.y());
#endif
  auto& pts = p.first->second;
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  const set<XYPos> u0 = unique();
  const auto n0 = u0.size();
  show_points<XYPos, XYSize, std::set<XYPos>>(
    u0,
    "Pts: Adding (%f, %f) to %ld points",
    p0.x(),
    p0.y(),
    n0);
#endif
  // HACK: insert the same thing twice should do nothing?
  // if (!p.second)
  // {
  //   logging::info("already existed");
  // }
  if (!p.second)
  {
    pts.insert(p0);
  }
#if defined(DEBUG_CELLPOINTS) || defined(DEBUG_NEW_SPREAD_VERBOSE)
  {
    const auto n1 = size();
    const set<XYPos> u1 = unique();
    show_points<XYPos, XYSize, std::set<XYPos>>(
      u1,
      "Pts: Adding (%f, %f) to %ld points gives %ld points",
      p0.x(),
      p0.y(),
      n0,
      n1);
  }
#endif
  return pts;
}
set<XYPos> Pts::unique() const noexcept
{
  set<XYPos> r{};
  Location loc{cell_x_y_.hash()};
  add_unique(loc, r);
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  {
    show_points<XYPos, XYSize, std::set<XYPos>>(
      r,
      "Generated unique points as:");
  }
#endif
  return r;
}
void Pts::add_unique(const Location& loc, set<XYPos>& into) const noexcept
{
  logging::check_equal(
    loc.column(),
    cell_x_y_.x(),
    "Pts::add_unique: x");
  logging::check_equal(
    loc.row(),
    cell_x_y_.y(),
    "Pts::add_unique: y");
// if any point is invalid then they all have to be
#ifdef DEBUG_NEW_SPREAD_VERBOSE
  logging::verbose(
    "Pts::add_unique: (%d, %d) %s burn",
    cell_x_y_.x(),
    cell_x_y_.y(),
    canBurn() ? "can" : "cannot");
#endif
  if (canBurn())
  {
#ifdef DEBUG_NEW_SPREAD_VERBOSE
    logging::verbose(
      "Pts::add_unique: starting with %ld points",
      into.size());
#endif
    const auto& pts = points();
    for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
    {
      const auto& p = pts[i];
#ifdef DEBUG_NEW_SPREAD_VERBOSE
      logging::verbose(
        "Pts::add_unique: (%f, %f) as %s",
        p.x(),
        p.y(),
        DIRECTION_NAMES[i]);
#endif
      into.insert(
        {cell_x_y_.x(),
         cell_x_y_.y(),
         p.x(),
         p.y()});
#ifdef DEBUG_NEW_SPREAD_VERBOSE
      logging::verbose(
        "Pts::add_unique: now %ld points",
        into.size());
#endif
    }
    // std::transform(
    //   points().cbegin(),
    //   points().cend(),
    //   std::insert_iterator(into, end(into)),
    //   [this](auto& p) -> XYPos {
    //     return {
    //       cell_x_y_.x(),
    //       cell_x_y_.y(),
    //       p.x(),
    //       p.y()};
    //   });
    logging::check_fatal(
      0 == into.size(),
      "Pts::add_unique: Can burn but no points");
    // logging::note(
    //   "Pts: Have %ld points",
    //   into.size());
  }
  // const auto pts = unique();
  // //   logging::verbose("Have %ld points", pts.size());
  // //   for (auto& p : pts)
  // //   {
  // //     logging::verbose("(%f, %f)", p.x(), p.y());
  // //   }
  // std::transform(
  //   pts.cbegin(),
  //   pts.cend(),
  //   std::insert_iterator(into, end(into)),
  //   [loc](auto& p1) -> XYPos {
  //     XYPos p{loc.column(), loc.row(), p1.x(), p1.y()};
  //     //   logging::verbose("generated point is (%f, %f)", p.x(), p.y());
  //     return p;
  //   });
}
size_t Pts::size() const noexcept
{
  return unique().size();
}
set<XYPos> PtMap::unique(const HashSize hash_value) const noexcept
{
  set<XYPos> r{};
  for (auto& kv : map_)
  {
    if (kv.first == hash_value)
    {
      const auto& loc = Location{kv.first};
      logging::verbose(
        "PtMap::unique: Adding unique for cell (%d, %d)",
        loc.column(),
        loc.row());
      auto& pts = kv.second;
      pts.add_unique(loc, r);
    }
  }
  return r;
}
set<XYPos> PtMap::unique() const noexcept
{
  set<XYPos> r{};
  add_unique(r);
  return r;
}
void PtMap::add_unique(set<XYPos>& into) const noexcept
{
  for (auto& kv : map_)
  {
    const auto& loc = Location{kv.first};
    auto& pts = kv.second;
    pts.add_unique(loc, into);
  }
}
set<HashSize> PtMap::keys() const noexcept
{
  set<HashSize> r{};
  for (auto& kv : map_)
  {
    r.insert(kv.first);
  }
  return r;
}
size_t PtMap::size() const noexcept
{
  return unique().size();
}
size_t PtMap::erase(const HashSize hash_value) noexcept
{
  return map_.erase(hash_value);
}
}
#endif
