/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LOCATION_H
#define FS_LOCATION_H
#include "../stdafx.h"
#include "../util/Util.h"
#include "StrictType.h"
namespace fs
{
constexpr auto CELL_CENTER = static_cast<InnerSize>(0.5);
// Number of bits to use for storing one coordinate of Position data
static constexpr HashSize XYBits = std::bit_width<HashSize>(MAX_HEIGHT - 1);
static_assert(pow_int<XYBits, size_t>(2) == MAX_HEIGHT);
static_assert(pow_int<XYBits, size_t>(2) == MAX_WIDTH);
static constexpr HashSize ColumnMask = bit_mask<XYBits, HashSize>();
// // Number of bits to use for storing Position data
// static constexpr HashSize PositionBits = XYBits * 2;
// // Hash mask for bits being used for Position data
// static constexpr Topo HashMask = bit_mask<PositionBits, Topo>();
// static_assert(HashMask >= static_cast<size_t>(MAX_WIDTH) * MAX_HEIGHT - 1);
// static_assert(HashMask <= std::numeric_limits<HashSize>::max());
// HACK: use same invalid value as Idx versions
// decimal cell position in the X direction
struct XPos : public StrictType<XPos, units::Unitless, XYSize, std::numeric_limits<Idx>::max()>
{
  using StrictType::StrictType;
  auto operator<=>(const XPos& rhs) const = default;
};
// decimal cell position in the Y direction
struct YPos : public StrictType<YPos, units::Unitless, XYSize, std::numeric_limits<Idx>::max()>
{
  using StrictType::StrictType;
  auto operator<=>(const YPos& rhs) const = default;
};
struct XIdx : public StrictType<XIdx, units::Unitless, Idx, std::numeric_limits<Idx>::max()>
{
  using StrictType::StrictType;
  explicit constexpr XIdx(const XPos& x) : XIdx(static_cast<Idx>(x.value)) { }
  auto operator<=>(const XIdx& rhs) const = default;
};
struct YIdx : public StrictType<YIdx, units::Unitless, Idx, std::numeric_limits<Idx>::max()>
{
  using StrictType::StrictType;
  explicit constexpr YIdx(const YPos& y) : YIdx(static_cast<Idx>(y.value)) { }
  auto operator<=>(const YIdx& rhs) const = default;
};
// HACK: don't actually have bounds for now
// The position within the Environment that a spreading point has.
struct XYPos
{
  XPos x{};
  YPos y{};
  static constexpr XYPos Invalid() noexcept { return XYPos{XPos::Invalid(), YPos::Invalid()}; }
  auto operator<=>(const XYPos& rhs) const = default;
};
// a point that is within a set of min/max bounds
template <class S, int XMin, int XMax, int YMin, int YMax>
struct BoundedPoint;
//  Offset from a position
using Offset = BoundedPoint<XYSize, -1, 1, -1, 1>;
// The position within the Environment that a spreading point has.
using CellPos = BoundedPoint<Idx, 0, MAX_WIDTH, 0, MAX_HEIGHT>;
template <class S, int XMin, int XMax, int YMin, int YMax>
struct BoundedPoint
{
  S x{XMin - 1};
  S y{YMin - 1};
  auto operator==(const BoundedPoint& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
  auto operator<=>(const BoundedPoint& rhs) const noexcept
  {
    if (auto cmp = x <=> rhs.x; 0 != cmp)
    {
      return cmp;
    }
    return y <=> rhs.y;
  }
  using class_type = BoundedPoint<S, XMin, XMax, YMin, YMax>;
  static constexpr auto INVALID_X = XMin - 1;
  static constexpr auto INVALID_Y = YMin - 1;
};
struct XYIdx
{
public:
  // access value for format
  constexpr inline Idx x_value() const noexcept { return x.value; }
  constexpr inline Idx y_value() const noexcept { return y.value; }
  constexpr XYIdx() = default;
  explicit constexpr XYIdx(const XIdx& x0, const YIdx& y0) : x{x0}, y{y0}
  {
#ifdef DEBUG_POINTS
    // HACK: don't use logging since breaks constexpr
    if (-1 >= x.value || MAX_WIDTH <= x.value || -1 >= y.value || MAX_HEIGHT <= y.value)
    {
      cout << std::format("xy out of bounds: ({}, {})", x.value, y.value);
      exit(-1);
    }
#endif
  }
  template <class T>
  explicit constexpr XYIdx(const T& x0, const T& y0)
    : XYIdx(XIdx{static_cast<Idx>(x0)}, YIdx{static_cast<Idx>(y0)})
  { }
  template <class S, int XMin, int XMax, int YMin, int YMax>
  explicit constexpr XYIdx(const BoundedPoint<S, XMin, XMax, YMin, YMax>& pt) noexcept
    : XYIdx(pt.x, pt.y)
  { }
  explicit constexpr XYIdx(const XYPos& xy) noexcept : XYIdx(XIdx{xy.x}, YIdx{xy.y}) { }
  constexpr auto operator==(const XYIdx& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
  constexpr auto operator<=>(const XYIdx& rhs) const noexcept
  {
    // maintain same order as hash was, regardless of how actual object is stored
    if (const auto cmp = y <=> rhs.y; 0 != cmp)
    {
      return cmp;
    }
    return x <=> rhs.x;
  }
  XYPos operator+(const XYPos& rhs) const noexcept
  {
    return XYPos{XPos{rhs.x.value + x.value}, YPos{rhs.y.value + y.value}};
  }
  XYPos center() const noexcept { return *this + XYPos{XPos{CELL_CENTER}, YPos{CELL_CENTER}}; }
  XYIdx operator+(const XYIdx& rhs) const noexcept { return XYIdx{x + rhs.x, y + rhs.y}; }
  XYIdx operator+(const XIdx& rhs) const noexcept { return XYIdx{x + rhs, y}; }
  XYIdx operator+(const YIdx& rhs) const noexcept { return XYIdx{x, y + rhs}; }
  /**
   * Determine the direction that a given cell is in from another cell. This is the
   * same convention as wind (i.e. the direction it is coming from, not the direction
   * it is going towards).
   * @param src The cell to find directions relative to
   * @param dst The cell to find the direction of
   * @return Direction that you would have to go in to get to dst from src
   */
  inline constexpr CellIndex relativeIndex(const XYIdx& rhs) const noexcept
  {
    constexpr CellIndex DIRECTIONS[9] = {
      DIRECTION_SW,
      DIRECTION_S,
      DIRECTION_SE,
      DIRECTION_W,
      DIRECTION_NONE,
      DIRECTION_E,
      DIRECTION_NW,
      DIRECTION_N,
      DIRECTION_NE
    };
    return DIRECTIONS[((x - rhs.x).value + 1) + 3 * ((y - rhs.y).value + 1)];
  }
  XIdx x{};
  YIdx y{};
};
inline XYPos operator+(const XYPos& lhs, const XYIdx& rhs) noexcept { return rhs + lhs; }
static inline constexpr std::pair<XIdx, YIdx> hash_to_xy(const XYIdx& xy) noexcept
{
  return {xy.x, xy.y};
}
static inline constexpr std::pair<Idx, Idx> hash_to_xy_value(const XYIdx& xy) noexcept
{
  return {xy.x_value(), xy.y_value()};
}
static inline constexpr size_t to_index(const XYIdx& xy) noexcept
{
  // NOTE: do this here since fixed grid is the only reason for this?
  return (static_cast<HashSize>(xy.y_value()) << XYBits) + static_cast<HashSize>(xy.x_value());
}
/**
 * Determine the direction that a given cell is in from another cell. This is the
 * same convention as wind (i.e. the direction it is coming from, not the direction
 * it is going towards).
 * @param src The cell to find directions relative to
 * @param dst The cell to find the direction of
 * @return Direction that you would have to go in to get to dst from src
 */
inline constexpr CellIndex relativeIndex(const XYIdx& src, const XYIdx& dst) noexcept
{
  return src.relativeIndex(dst);
}
}
#endif
