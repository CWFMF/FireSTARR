/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "SpreadCache.h"
#include "../log/Log.h"
#include "../sim/Scenario.h"
#include "FireSpread.h"
namespace fs
{
static MathSize find_min_ros(const Scenario& scenario, const DurationSize time)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  const MathSize min_ros = settings.minimum_ros;
  return settings.deterministic ? min_ros : std::max(scenario.spreadThresholdByRos(time), min_ros);
}
ptr<const SpreadInfo> SpreadCache::add_spread(
  const SpreadKey& key,
  ptr<const Scenario> scenario,
  DurationSize time
) noexcept
{
  return &(spread_info_
             .try_emplace(
               key,
               time,
               find_min_ros(*scenario, time),
               scenario->cellSize(),
               key,
               scenario->nd(time),
               scenario->weather(time),
               scenario->weather_daily(time)
             )
             .first->second);
}
MathSize SpreadCache::maxIntensity(const SpreadKey& key) const noexcept
{
  auto seek_spread = spread_info_.find(key);
  const auto max_intensity =
    (spread_info_.end() == seek_spread) ? 0 : seek_spread->second.maxIntensity();
  return max_intensity;
}
const OffsetSet& SpreadCache::offsets(const SpreadKey& key) const noexcept
{
  return spread_info_.at(key).offsets();
}
}
