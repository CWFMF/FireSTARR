/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_POINT_SPREAD_H
#define FS_POINT_SPREAD_H
#include "../stdafx.h"
#include "../sim/Scenario.h"
#include "../spread/CellPoints.h"
#include "SpreadCache.h"
namespace fs
{
class Scenario;
// time spread went to or -1 if no spread
DurationSize do_spread(
  MathSize& max_ros,
  CellPointsMap& points,
  SpreadCache& spread_info,
  ptr<const Scenario> scenario,
  const BurnedData& unburnable,
  const DurationSize time,
  const DurationSize max_duration
) noexcept;
}
#endif
