/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Observer.h"
namespace fs::sim
{
#ifndef MODE_BP_ONLY
string IObserver::makeName(const string& base_name, const string& suffix)
{
  if (base_name.length() > 0)
  {
    return base_name + "_" + suffix;
  }
  return suffix;
}
constexpr DurationSize NODATA_ARRIVAL = 0;
ArrivalObserver::ArrivalObserver(const Scenario& scenario)
  : MapObserver<DurationSize>(scenario, NODATA_ARRIVAL, "arrival")
{
#ifdef DEBUG_GRIDS
  // enforce converting to an int and back produces same V
  const auto n0 = NODATA_ARRIVAL;
  const auto n1 = static_cast<NodataIntType>(n0);
  const auto n2 = static_cast<DurationSize>(n1);
  const auto n3 = static_cast<NodataIntType>(n2);
  logging::check_equal(
    n1,
    n3,
    "nodata_value_ as int");
  logging::check_equal(
    n0,
    n2,
    "nodata_value_ from int");
#endif
}
DurationSize ArrivalObserver::getValue(const Event& event) const noexcept
{
#ifdef DEBUG_TEMPORARY
  if (abs(event.time() - 154.9987423154746) < 0.001)
  {
    printf("here\n");
  }
#endif
  return event.time();
}
SourceObserver::SourceObserver(const Scenario& scenario)
  : MapObserver<CellIndex>(scenario, static_cast<CellIndex>(255), "source")
{
}
CellIndex SourceObserver::getValue(const Event& event) const noexcept
{
  return event.source();
}
IntensityObserver::IntensityObserver(const Scenario& scenario) noexcept
  : MapObserver(scenario, NO_INTENSITY, "intensity")
{
}
[[nodiscard]] IntensitySize IntensityObserver::getValue(const Event& event) const noexcept
{
  return event.intensity();
}
void IntensityObserver::save(const string& dir, const string& base_name) const
{
  // FIX: IntensityObserver not tracking max right now?
  // MapObserver<IntensitySize>::save(dir, base_name);
  // FIX: save what scenario is tracking for now, but should be converted
  scenario_.saveIntensity(dir, base_name);
}
#endif
}
