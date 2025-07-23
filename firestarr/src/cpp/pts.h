/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#ifdef USE_OLD_SPREAD
#include "CellPoints.h"
#endif
#include "InnerPos.h"
#include "IntensityMap.h"
#ifdef USE_NEW_SPREAD
namespace fs::sim
{
#ifndef USE_OLD_SPREAD
using topo::SpreadKey;
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
// mask of sides that would need to be burned for direction to not matter
static constexpr std::array<CellIndex, NUM_DIRECTIONS> DIRECTION_MASKS{
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
  MASK_NW};
static constexpr std::array<const char*, NUM_DIRECTIONS> DIRECTION_NAMES{
  "N",
  "NNE",
  "NE",
  "ENE",
  "E",
  "ESE",
  "SE",
  "SSE",
  "S",
  "SSW",
  "SW",
  "WSW",
  "W",
  "WNW",
  "NW",
  "NNW"};
using array_dists = std::array<DistanceSize, NUM_DIRECTIONS>;
using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;
using array_cellpts = std::tuple<array_dists, array_pts>;
#endif
using spreading_points_new = map<SpreadKey, vector<pair<HashSize, set<XYPos>>>>;
class Pts
{
public:
  Pts();
  Pts(
    const IntensityMap& intensity_map,
    const XYPos p);
  Pts(
    const IntensityMap& intensity_map,
    const XYSize x,
    const XYSize y)
    : Pts(intensity_map, XYPos{x, y})
  {
  }
  Pts& insert(const XYPos p0)
  {
    return insert(p0.x(), p0.y());
  }
  Pts& insert(const XYSize x,
              const XYSize y);
  set<XYPos> unique() const noexcept;
  void add_unique(const Location& loc, set<XYPos>& into) const noexcept;
  bool empty() const;
#ifdef DEBUG_NEW_SPREAD
  size_t size() const noexcept;
#endif
  inline const array_dists& distances() const
  {
    return std::get<0>(cell_pts_);
  }
  inline const array_pts& points() const
  {
    return std::get<1>(cell_pts_);
  }
  inline array_dists& distances()
  {
    return std::get<0>(cell_pts_);
  }
  inline array_pts& points()
  {
    return std::get<1>(cell_pts_);
  }
  bool canBurn() const;
private:
  array_cellpts cell_pts_;
  CellPos cell_x_y_;
};
class PtMap
{
public:
  PtMap() = default;
  PtMap(const PtMap& rhs) noexcept = default;
  PtMap(PtMap&& rhs) noexcept = default;
  PtMap& operator=(const PtMap& rhs) noexcept = default;
  PtMap& operator=(PtMap&& rhs) noexcept = default;
  Pts& insert(
    const IntensityMap& intensity_map,
    const XYPos p0);
  Pts& insert(
    const IntensityMap& intensity_map,
    const XYSize x,
    const XYSize y)
  {
    return insert(intensity_map, XYPos{x, y});
  }
  set<XYPos> unique(const HashSize hash_value) const noexcept;
  set<XYPos> unique() const noexcept;
  void add_unique(set<XYPos>& into) const noexcept;
  set<HashSize> keys() const noexcept;
  size_t size() const noexcept;
  size_t erase(const HashSize hash_value) noexcept;
  // private:
  map<HashSize, Pts> map_;
};
}
#endif
