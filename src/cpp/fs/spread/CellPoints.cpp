/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "CellPoints.h"
namespace fs
{
void CellPoints::insert_basic(
  CellPoints& cell_pts,
  const XYPos&,
  const SpreadData& spread_current,
  const XYPos& xy
) noexcept
{
  auto& spread_arrival = cell_pts.spread_arrival_;
  if (0 < spread_current.time && 0 > spread_arrival.time)
  {
    // record ros and time if nothing yet
    spread_arrival = spread_current;
  }
  const auto& cell_x_y = cell_pts.cell_x_y_;
  const DistanceSize x0{static_cast<DistanceSize>(xy.x.value - cell_x_y.x.value)};
  const DistanceSize y0{static_cast<DistanceSize>(xy.y.value - cell_x_y.y.value)};
  // CHECK: FIX: is this initializing everything to false or just one element?
  for (size_t i = 0; i < cell_pts.distances.size(); ++i)
  {
    // calculate with inner but keep XYPos
    const auto d = distance(x0, y0, POINTS_OUTER[i].first, POINTS_OUTER[i].second);
    auto& p_d = cell_pts.distances[i];
    auto& p_p = cell_pts.points[i];
    p_p = (d < p_d) ? xy : p_p;
    p_d = (d < p_d) ? d : p_d;
    // don't calculate FI/ROS/RAZ
  }
}
}
