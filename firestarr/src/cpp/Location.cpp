/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "Location.h"
namespace tbd::topo
{
CellIndex relativeIndex(const Location& src, const Location& dst) noexcept
{
  static constexpr CellIndex DIRECTIONS[9] =
    {
      DIRECTION_SW,
      DIRECTION_S,
      DIRECTION_SE,
      DIRECTION_W,
      DIRECTION_NONE,
      DIRECTION_E,
      DIRECTION_NW,
      DIRECTION_N,
      DIRECTION_NE};
  return DIRECTIONS[((src.column() - dst.column()) + 1)
                    + 3 * ((src.row() - dst.row()) + 1)];
}
}
