/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_CELLPOINTS_H
#define FS_CELLPOINTS_H
#include "stdafx.h"
#include <algorithm>
#include <compare>
#include "BurnedData.h"
#include "Cell.h"
#include "FireSpread.h"
#include "Location.h"
#include "unstable.h"
#include "Weather.h"
namespace fs
{
using fs::Direction;
// Type used for storing distances within cells
using DistanceSize = _Float64;
struct SpreadData
{
  DurationSize time{INVALID_TIME};
  IntensitySize intensity{static_cast<IntensitySize>(0)};
  ROSSize ros{INVALID_ROS};
  Direction direction{Direction::Invalid()};
  Direction direction_previous{Direction::Invalid()};
};
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const auto INVALID_DISTANCE = static_cast<DistanceSize>(MAX_HEIGHT * MAX_HEIGHT);
static constexpr size_t FURTHEST_N = 0;
static constexpr size_t FURTHEST_NNE = 1;
static constexpr size_t FURTHEST_NE = 2;
static constexpr size_t FURTHEST_ENE = 3;
static constexpr size_t FURTHEST_E = 4;
static constexpr size_t FURTHEST_ESE = 5;
static constexpr size_t FURTHEST_SE = 6;
static constexpr size_t FURTHEST_SSE = 7;
static constexpr size_t FURTHEST_S = 8;
static constexpr size_t FURTHEST_SSW = 9;
static constexpr size_t FURTHEST_SW = 10;
static constexpr size_t FURTHEST_WSW = 11;
static constexpr size_t FURTHEST_W = 12;
static constexpr size_t FURTHEST_WNW = 13;
static constexpr size_t FURTHEST_NW = 14;
static constexpr size_t FURTHEST_NNW = 15;
static constexpr size_t NUM_DIRECTIONS = 16;
static constexpr auto MASK_NE = DIRECTION_N & DIRECTION_NE & DIRECTION_E;
static constexpr auto MASK_SE = DIRECTION_S & DIRECTION_SE & DIRECTION_E;
static constexpr auto MASK_SW = DIRECTION_S & DIRECTION_SW & DIRECTION_W;
static constexpr auto MASK_NW = DIRECTION_N & DIRECTION_NW & DIRECTION_W;
template <class T>
using CompassArray = std::array<T, NUM_DIRECTIONS>;
// mask of sides that would need to be burned for direction to not matter
static constexpr CompassArray<CellIndex> DIRECTION_MASKS{
  DIRECTION_N,
  MASK_NE,
  MASK_NE,
  MASK_NE,
  DIRECTION_E,
  MASK_SE,
  MASK_SE,
  MASK_SE,
  DIRECTION_S,
  MASK_SW,
  MASK_SW,
  MASK_SW,
  DIRECTION_W,
  MASK_NW,
  MASK_NW,
  MASK_NW
};
// use direction distance towards compass point as tie-breaker when ros is the same
static constexpr auto DIRECTION_ANGLES = []() {
  CompassArray<Direction> result{};
  constexpr auto angle = Degrees{Radians::D_360().asDegrees().value / result.size()};
  for (size_t i = 0; i < result.size(); ++i)
  {
    // divide circle into equal wedges
    result[i] = angle * i;
  }
  return result;
}();
static inline const SpreadData& find_better(
  const size_t for_direction,
  const SpreadData& lhs,
  const SpreadData& rhs
) noexcept
{
  assert(for_direction < NUM_DIRECTIONS);
  // either faster or higher direction since we're trying to make this reproducible
  // regardless of order points are inserted
  if (lhs.ros < rhs.ros)
  {
    return rhs;
  }
  else if (lhs.ros > rhs.ros)
  {
    return lhs;
  }
  // if same speed then look at intensity
  if (lhs.intensity < rhs.intensity)
  {
    return rhs;
  }
  else if (lhs.intensity > rhs.intensity)
  {
    return lhs;
  }
  // if same intensity then look at direction
  const auto& dir = DIRECTION_ANGLES[for_direction];
  auto d0 = lhs.direction - dir;
  auto d1 = rhs.direction - dir;
  // if closer to compass direction or positive in the case of the same distance
  if (abs(d0) > abs(d1) || (abs(d0) == abs(d1) && d0 < d1))
  {
    return rhs;
  }
  return lhs;
}
/**
 * Points in a cell furthest in each direction
 */
class CellPoints
{
private:
  static bool should_save() noexcept
  {
    // HACK: resolve once and fail if not set already
    static auto& settings = fs::settings::instance();
    static auto r = settings.save_individual || settings.save_intensity;
    return r;
  }
  static inline constexpr DistanceSize distance(
    const DistanceSize x2,
    const DistanceSize y2
  ) noexcept
  {
    return x2 * x2 + y2 * y2;
  }
  static inline constexpr DistanceSize distance(
    const DistanceSize x0,
    const DistanceSize y0,
    const DistanceSize x1,
    const DistanceSize y1
  ) noexcept
  {
    return distance(x0 - x1, y0 - y1);
  }
  static inline constexpr DistanceSize distance(
    const XYIdx& cell_x_y,
    const XYPos& xy,
    const DistanceSize x1,
    const DistanceSize y1
  ) noexcept
  {
    return distance(
      static_cast<DistanceSize>(xy.x.value - cell_x_y.x.value),
      static_cast<DistanceSize>(xy.y.value - cell_x_y.y.value),
      x1,
      y1
    );
  }
  static inline constexpr DistanceSize distance(
    const XYIdx& cell_x_y,
    const XYPos& xy,
    const size_t i
  ) noexcept
  {
    return distance(
      static_cast<DistanceSize>(xy.x.value - cell_x_y.x.value),
      static_cast<DistanceSize>(xy.y.value - cell_x_y.y.value),
      POINTS_OUTER[i].first,
      POINTS_OUTER[i].second
    );
  }
  static void insert_calc(
    CellPoints& cell_pts,
    const XYPos& src,
    const SpreadData& spread_current,
    const XYPos& xy
  ) noexcept
  {
#ifdef DEBUG_CELLPOINTS
    logging::note(
      "Insert ({:f}, {:f}) at time {:f} with ROS {:f}, Intensity {:d}, RAZ {:f}",
      x,
      y,
      arrival_time,
      ros,
      intensity,
      raz.asDegrees()
    );
#endif
    // initial burn will have an invalid direction, so needs to burn everywhere
    const auto is_initial = Direction::Invalid() == spread_current.direction_previous;
    // only spread in a direction that's in front of the normal to the angle it came from
    // i.e. the 90 degrees on either side of the raz
    const auto dir_diff =
      abs((spread_current.direction - spread_current.direction_previous).fix().asDegrees());
    constexpr auto MAX_DEGREES = 90.0;
    // NOTE: there should be no change in the extent of the fire if we exclude things behind the
    // normal to the direction it came from
    //       - but if we exclude too much then it can change how things spread, even if it is a
    //       more representative angle for the grids
    const auto is_not_backwards = MAX_DEGREES >= dir_diff;
    const auto consider_for_spread = is_initial || is_not_backwards;
    auto& spread_arrival = cell_pts.spread_arrival_;
    // count things as the same time if within a tolerance
    constexpr auto TIME_EPSILON_SECONDS = 1.0 * MINUTE_SECONDS;
    constexpr auto TIME_EPSILON = TIME_EPSILON_SECONDS / DAY_SECONDS;
    if (0 < spread_current.time && 0 > spread_arrival.time)
    {
#ifdef DEBUG_CELLPOINTS
      logging::extensive(
        "No time so setting ros to {:f} at time {:f}", spread_current.ros, spread_current.time
      );
#endif
      // record ros and time if nothing yet
      spread_arrival = spread_current;
    }
    // no point in any of this if not outputting individual or intensity
    else
    {
      if (consider_for_spread)
      {
        if (abs(spread_current.time - spread_arrival.time) <= TIME_EPSILON)
        // else if (arrival_time == arrival_time_)
        {
#ifdef DEBUG_CELLPOINTS
          logging::verbose(
            "Same time so setting ros to max({:f}, {:f}) at time {:f}",
            spread_current.ros,
            spread_arrival.ros,
            spread_current.time
          );
#endif
          // the same time so pick higher ros
          if (
          (spread_arrival.ros < spread_current.ros)
          || (spread_arrival.ros == spread_current.ros
              && spread_current.intensity > spread_arrival.intensity))
          {
            // NOTE: keep track of original time so this doesn't just always happen
            spread_arrival = {
              spread_arrival.time,
              spread_current.intensity,
              spread_current.ros,
              spread_current.direction,
              spread_current.direction_previous
            };
          }
        }
      }
    }
    // NOTE: use location inside cell so smaller types can be more precise
    // since digits aren't wasted on cell
    const auto& cell_x_y = cell_pts.cell_x_y_;
    auto& directions = cell_pts.directions;
    const DistanceSize x0{static_cast<DistanceSize>(xy.x.value - cell_x_y.x_value())};
    const DistanceSize y0{static_cast<DistanceSize>(xy.y.value - cell_x_y.y_value())};
    // CHECK: FIX: is this initializing everything to false or just one element?
    CompassArray<bool> closer{};
    std::fill(closer.begin(), closer.end(), false);
    // even if !consider_for_spread, update points
    for (size_t i = 0; i < closer.size(); ++i)
    {
      const auto d = distance(x0, y0, POINTS_OUTER[i].first, POINTS_OUTER[i].second);
      auto& p_d = cell_pts.distances[i];
      auto& p_p = cell_pts.points[i];
      auto& p_a = directions[i];
      closer[i] = (d < p_d);
      p_p = (d < p_d) ? xy : p_p;
      // always want to update with how we got here
      p_a = (d < p_d) ? spread_current.direction.asDegrees() : p_a;
      p_d = (d < p_d) ? d : p_d;
    }
#ifdef DEBUG_CELLPOINTS
    logging::note("now have {:d} points", size());
#endif
    const auto dst_xy = cell_pts.pos();
    const XYIdx src_xy{src};
    // adds 0 if the same so try without checking
    {
      // we inserted a pair of (src, dst), which means we've never
      // calculated the relativeIndex for this so add it to main map
      cell_pts.add_source(src_xy.relativeIndex(dst_xy));
    }
    // FIX: this should be aware of where it came from
    // have to check src vs dst is same so we know it didn't come in from outside
    // CHECK: FIX: should check for things that spread out of cell also?
    if ((src_xy == dst_xy)
        // if this spread isn't backwards then figure out if it's faster than previous internal
        && consider_for_spread)
    {
      // if we spread from this cell to this cell again then ros could be considered for max
      // need to make sure we're not spreading back towards where we came from because that doesn't
      // matter
      // HACK: for now look at source for this cell and exclude points in those directions
      const auto srcs = cell_pts.sources();
      for (size_t i = 0; i < closer.size(); ++i)
      {
        const auto mask = DIRECTION_MASKS[i];
        if (mask != (srcs & mask))
        {
          // at least one of the cells in this direction is not a source, so consider them
          if (closer[i])
          {
            auto& spread_old = cell_pts.spread_internal_[i];
            spread_old = find_better(i, spread_old, spread_current);
          }
        }
      }
    }
  }
  static void insert_basic(
    CellPoints& cell_pts,
    const XYPos&,
    const SpreadData& spread_current,
    const XYPos& xy
  ) noexcept;
  static CellPoints& insert(
    CellPoints& cell_pts,
    const XYPos& src,
    const SpreadData& spread_current,
    const XYPos& xy
  ) noexcept
  {
    static const auto fct = [&]() { return (should_save()) ? &insert_calc : &insert_basic; }();
    fct(cell_pts, src, spread_current, xy);
    return cell_pts;
  }

private:
  // HACK: repeat for now
  CellPoints(const XYIdx& cell_xy) noexcept : cell_x_y_{cell_xy}
  {
    std::ranges::fill(distances, INVALID_DISTANCE);
    // FIX: thought invalid would work, but does this need to be 0?
    std::ranges::fill(points, XYPos::Invalid());
    if (should_save())
    {
      std::ranges::fill(directions, INVALID_DIRECTION.value);
    }
#ifdef DEBUG_CELLPOINTS
    logging::note("CellPoints is size {:d} after creation and should be empty", size());
#endif
  }
  CellPoints(const XYPos& p) noexcept : CellPoints(XYIdx{p}) { }

private:
  // HACK: define constants so we don't have to cast
  static constexpr auto I_0_0 = static_cast<DistanceSize>(0.0);
  static constexpr auto I_0_5 = static_cast<DistanceSize>(0.5);
  static constexpr auto I_1_0 = static_cast<DistanceSize>(1.0);
  static constexpr auto DIST_22_5 =
    static_cast<DistanceSize>(0.2071067811865475244008443621048490392848359376884740365883398689);
  static constexpr auto P_0_5 = static_cast<DistanceSize>(0.5) + DIST_22_5;
  static constexpr auto M_0_5 = static_cast<DistanceSize>(0.5) - DIST_22_5;
  using d = std::pair<DistanceSize, DistanceSize>;
  static constexpr CompassArray<d> POINTS_OUTER{
    d{I_0_5, I_1_0},
    // north-northeast is closest to point (0.5 + 0.207, 1.0)
    d{P_0_5, I_1_0},
    // northeast is closest to point (1.0, 1.0)
    d{I_1_0, I_1_0},
    // east-northeast is closest to point (1.0, 0.5 + 0.207)
    d{I_1_0, P_0_5},
    // east is closest to point (1.0, 0.5)
    d{I_1_0, I_0_5},
    // east-southeast is closest to point (1.0, 0.5 - 0.207)
    d{I_1_0, M_0_5},
    // southeast is closest to point (1.0, 0.0)
    d{I_1_0, I_0_0},
    // south-southeast is closest to point (0.5 + 0.207, 0.0)
    d{P_0_5, I_0_0},
    // south is closest to point (0.5, 0.0)
    d{I_0_5, I_0_0},
    // south-southwest is closest to point (0.5 - 0.207, 0.0)
    d{M_0_5, I_0_0},
    // southwest is closest to point (0.0, 0.0)
    d{I_0_0, I_0_0},
    // west-southwest is closest to point (0.0, 0.5 - 0.207)
    d{I_0_0, M_0_5},
    // west is closest to point (0.0, 0.5)
    d{I_0_0, I_0_5},
    // west-northwest is closest to point (0.0, 0.5 + 0.207)
    d{I_0_0, P_0_5},
    // northwest is closest to point (0.0, 1.0)
    d{I_0_0, I_1_0},
    // north-northwest is closest to point (0.5 - 0.207, 1.0)
    d{M_0_5, I_1_0}
  };

public:
  using spreading_points = map<SpreadKey, vector<pair<XYIdx, CellPoints>>>;
  constexpr CellPoints() noexcept = default;
  // HACK: so we can emplace with nullptr
  CellPoints(const CellPoints* rhs) noexcept
  {
    logging::check_fatal(nullptr == rhs, "Initializing CellPoints from nullptr");
    *this = *rhs;
  }
  CellPoints& insert(const XYPos& src, const SpreadData& spread_current, const XYPos& xy) noexcept
  {
    return CellPoints::insert(*this, src, spread_current, xy);
  }
  CellPoints(const XYPos& src, const SpreadData& spread_current, const XYPos& xy) noexcept
    : CellPoints(XYIdx{xy})
  {
    insert(src, spread_current, xy);
  }
  CellPoints(CellPoints&& rhs) noexcept = default;
  CellPoints(const CellPoints& rhs) noexcept = default;
  CellPoints& operator=(CellPoints&& rhs) noexcept = default;
  CellPoints& operator=(const CellPoints& rhs) noexcept = default;
  void add_source(const CellIndex src) { src_ |= src; }
  CellIndex sources() const { return src_; }
  CellPoints& merge(const CellPoints& rhs)
  {
#ifdef DEBUG_CELLPOINTS
    const auto n0 = size();
    const auto n1 = rhs.size();
#endif
    // either both invalid or lower one is valid
    cell_x_y_ = min(cell_x_y_, rhs.cell_x_y_);
    auto& d0 = distances;
    auto& d1 = rhs.distances;
    auto& p0 = points;
    auto& p1 = rhs.points;
    auto& a0 = directions;
    auto& a1 = rhs.directions;
    // we know distances in each direction so just pick closer
    for (size_t i = 0; i < d0.size(); ++i)
    {
      if (d1[i] < d0[i])
      {
        d0[i] = d1[i];
        p0[i] = p1[i];
        a0[i] = a1[i];
      }
      spread_internal_[i] = find_better(i, spread_internal_[i], rhs.spread_internal_[i]);
    }
    add_source(rhs.src_);
    // if valid time and earlier then that would be the arrival time
    if (INVALID_TIME != rhs.spread_arrival_.time
        && (INVALID_TIME == spread_arrival_.time || rhs.spread_arrival_.time < spread_arrival_.time))
    {
      spread_arrival_ = rhs.spread_arrival_;
    }
#ifdef DEBUG_CELLPOINTS
    logging::note("Merging {:d} with {:d} gives {:d} pts", n0, n1, size());
#endif
    return *this;
  }
  set<XYPos> unique() const noexcept
  {
    // if any point is invalid then they all have to be
    if (XPos::Invalid().value == points[0].x.value)
    {
      return {};
    }
    return {points.begin(), points.end()};
  }
  auto operator==(const CellPoints& rhs) const noexcept
  {
    return cell_x_y_ == rhs.cell_x_y_ && points == rhs.points;
  }
  std::partial_ordering operator<=>(const CellPoints& rhs) const noexcept
  {
    if (auto cmp = cell_x_y_ <=> rhs.cell_x_y_; 0 != cmp)
    {
      return cmp;
    }
    return points <=> rhs.points;
  }
  [[nodiscard]] constexpr const XYIdx& pos() const noexcept { return cell_x_y_; }
  void clear();
  bool empty() const
  {
    // NOTE: if anything is invalid then everything must be
    return (XPos::Invalid().value == points[0].x.value);
  }
  auto point_directions() const noexcept
  {
    CompassArray<std::pair<XYPos, MathSize>> pt_dirs{};
    auto& pts = points;
    auto& dirs = directions;
    for (size_t i = 0; i < pts.size(); ++i)
    {
      pt_dirs[i] = {pts[i], dirs[i]};
    }
    return pt_dirs;
  }
  const SpreadData& spread_arrival() const noexcept { return spread_arrival_; }

private:
  // any way to get rid of this since we're using it as the map key?
  XYIdx cell_x_y_{};
  CompassArray<XYPos> points{};
  CompassArray<DistanceSize> distances{};
  CompassArray<MathSize> directions{};
  CellIndex src_{DIRECTION_NONE};
  SpreadData spread_arrival_{};
  // need to keep each direction separate so filtering by direction masks when merging works
  CompassArray<SpreadData> spread_internal_{SpreadData{}};
};
using spreading_points = CellPoints::spreading_points;
// map that merges items when try_emplace doesn't insert
class CellPointsMap
{
public:
  CellPointsMap() noexcept = default;
  CellPointsMap& merge(const BurnedData& unburnable, const CellPointsMap& rhs) noexcept
  {
    // FIX: if we iterate through both they should be sorted
    for (const auto& [location, pts] : rhs.cells_)
    {
      if (!unburnable.at(location))
      {
        auto e = cells_.try_emplace(location, pts);
        CellPoints& cell_pts = e.first->second;
        if (!e.second)
        {
          // couldn't insert
          cell_pts.merge(pts);
        }
#ifdef DEBUG_CELLPOINTS
        logging::note(
          "insert with size {:d} of ({:f}, {:f}) at time {:f} with ROS {:f} gives size {:d}",
          n0,
          x,
          y,
          arrival_time,
          ros,
          size()
        );
#endif
      }
    }
    return *this;
  }
  set<XYPos> unique() const noexcept
  {
    set<XYPos> r{};
    for (auto& [loc, pts] : cells_)
    {
      for (auto& p : pts.unique())
      {
        r.insert(p);
      }
    }
    return r;
  }
  using map_type = std::map<XYIdx, CellPoints>;
  using map_value = map_type::value_type;
  // apply function to each CellPoints within and remove matches
  void remove_if(std::function<bool(const map_value&)> F) noexcept { std::erase_if(cells_, F); }
  // FIX: public for debugging right now
  // private:
  // CellPoints contains XYIdx so no need for pair
  map_type cells_{};
};
static inline CellPoints& insert(
  CellPointsMap& cell_pts_map,
  const XYPos& src,
  const SpreadData& spread_current,
  const XYPos& xy
) noexcept
{
#ifdef DEBUG_CELLPOINTS
  const auto n0 = size();
#endif
  const XYIdx location{xy};
  auto& lhs = cell_pts_map.cells_;
  auto e = lhs.try_emplace(location, src, spread_current, xy);
  CellPoints& cell_pts = e.first->second;
  if (!e.second)
  {
    // FIX: should use max of whatever ROS has entered during this time and not just first ros
    // tried to add new CellPoints but already there
    cell_pts.insert(src, spread_current, xy);
#ifdef DEBUG_CELLPOINTS
    logging::note(
      "insert with size {:d} of ({:f}, {:f}) at time {:f} with ROS {:f} gives size {:d}",
      n0,
      x,
      y,
      arrival_time,
      ros,
      size()
    );
#endif
  }
  return cell_pts;
}
}
#endif
