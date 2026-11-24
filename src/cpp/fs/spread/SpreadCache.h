/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SPREAD_CACHE_H
#define FS_SPREAD_CACHE_H
#include "../stdafx.h"
#include "../grid/Cell.h"
#include "FireSpread.h"
namespace fs
{
class Scenario;
class SpreadCache
{
public:
  using SpreadCacheMap = map<SpreadKey, SpreadInfo>;
  SpreadCache() noexcept = default;
  SpreadCache(const SpreadCache& rhs) noexcept = delete;
  SpreadCache(SpreadCache&& rhs) noexcept = default;
  SpreadCache& operator=(const SpreadCache& rhs) noexcept = delete;
  SpreadCache& operator=(SpreadCache&& rhs) noexcept = default;
  ptr<const SpreadInfo> add_spread(
    const SpreadKey& key,
    ptr<const Scenario> scenario,
    DurationSize time
  ) noexcept;
  MathSize maxIntensity(const SpreadKey& key) const noexcept;
  const OffsetSet& offsets(const SpreadKey& key) const noexcept;

private:
  /**
   * \brief Calculated SpreadInfo for SpreadKey for current time
   */
  SpreadCacheMap spread_info_{};
};
}
#endif
