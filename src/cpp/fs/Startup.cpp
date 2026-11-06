/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Startup.h"
#include "unstable.h"
namespace fs
{
Startup::Startup(
  string station,
  const tm& generated,
  const Point& point,
  const MathSize distance_from,
  const Ffmc& ffmc,
  const Dmc& dmc,
  const Dc& dc,
  const Precipitation& apcp_prev,
  const bool overridden
) noexcept
  : station_(std::move(station)), generated_(generated), point_(point),
    distance_from_(distance_from), ffmc(ffmc), dmc(dmc), dc(dc), apcp_prev(apcp_prev),
    is_overridden_(overridden)
{ }
}
