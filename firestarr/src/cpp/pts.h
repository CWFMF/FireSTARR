/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "CellPoints.h"
#include "InnerPos.h"
#include "IntensityMap.h"

namespace fs::sim
{
class Pts
  : array_cellpts
{
public:
  Pts();
  Pts(
    const BurnedData& unburnable,
    const XYPos p);
  Pts(
    const BurnedData& unburnable,
    const XYSize x,
    const XYSize y)
    : Pts(unburnable, XYPos{x, y})
  {
  }
  Pts& insert(const XYPos p0)
  {
    return insert(p0.x(), p0.y());
  }
  Pts& insert(const XYSize x,
              const XYSize y);
  set<InnerPos> unique() const noexcept;
  void add_unique(const Location& loc, set<XYPos>& into) const noexcept;
  bool empty() const;
  size_t size() const noexcept;
  inline const array_dists& distances() const
  {
    return std::get<0>(*this);
  }
  inline const array_pts& points() const
  {
    return std::get<1>(*this);
  }
  inline array_dists& distances()
  {
    return std::get<0>(*this);
  }
  inline array_pts& points()
  {
    return std::get<1>(*this);
  }
  bool canBurn() const;
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
    const BurnedData& unburnable,
    const XYPos p0);
  Pts& insert(
    const BurnedData& unburnable,
    const XYSize x,
    const XYSize y)
  {
    return insert(unburnable, XYPos{x, y});
  }
  set<XYPos> unique() const noexcept;
  void add_unique(set<XYPos>& into) const noexcept;
  size_t size() const noexcept;
  map<HashSize, Pts> map_;
};
}
