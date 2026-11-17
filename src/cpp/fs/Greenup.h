/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_GREENUP_H
#define FS_GREENUP_H
#include "stdafx.h"
namespace fs::fuel
{
/**
 * \brief Calculate if green-up has occurred
 * \param nd Difference between date and the date of minimum foliar moisture content
 * \return Whether or no green-up has occurred
 */
[[nodiscard]] constexpr bool calculate_is_green(const int nd) { return nd >= 30; }
/**
 * \brief Use intersection of parabola with y = 120.0 line as point where grass greening starts
 * happening.
 */
static constexpr int START_GREENING = -43;
[[nodiscard]] constexpr int calculate_grass_curing(const int nd)
{
  const auto curing = (nd < START_GREENING)
                      ?   // we're before foliar moisture dip has started
                        100
                      : (nd >= 50)
                          ? 0   // foliar moisture is at 120.0, so grass should be totally uncured
                          // HACK: invent a formula that has 50% curing at the bottom of the foliar
                          // moisture dip foliar moisture above ranges between 120 and 85, with 85
                          // being at the point where we want 50% cured Curing:
                          // -43 => 100, 0 => 50, 50 => 0 least-squares best fit:
                          : static_cast<int>(52.5042 - 1.07324 * nd);
  return max(0, min(100, curing));
}
}
#endif
