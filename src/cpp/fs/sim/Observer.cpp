/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Observer.h"
namespace fs
{
string IObserver::makeName(const string_view base_name, const string_view suffix)
{
  if (base_name.length() > 0)
  {
    return string(base_name) + "_" + string(suffix);
  }
  return string(suffix);
}
constexpr DurationSize NODATA_ARRIVAL = 0;
ArrivalObserver::ArrivalObserver(const Scenario& scenario)
  : MapObserver<DurationSize>(scenario, NODATA_ARRIVAL, "arrival")
{
#ifdef DEBUG_GRIDS
  using NodataIntType = int64_t;
  // enforce converting to an int and back produces same V
  const auto n0 = NODATA_ARRIVAL;
  const auto n1 = static_cast<NodataIntType>(n0);
  const auto n2 = static_cast<DurationSize>(n1);
  const auto n3 = static_cast<NodataIntType>(n2);
  logging::check_equal(n1, n3, "nodata_value_ as int");
  logging::check_equal(n0, n2, "nodata_value_ from int");
#endif
}
DurationSize ArrivalObserver::getValue(const Event& event) const noexcept { return event.time; }
SourceObserver::SourceObserver(const Scenario& scenario)
  : MapObserver<CellIndex>(scenario, static_cast<CellIndex>(255), "source")
{ }
CellIndex SourceObserver::getValue(const Event& event) const noexcept { return event.source; }
IntensityObserver::IntensityObserver(const Scenario& scenario) noexcept
  : MapObserver(scenario, NO_INTENSITY, "intensity")
{ }
[[nodiscard]] IntensitySize IntensityObserver::getValue(const Event& event) const noexcept
{
  return event.intensity;
}
FileList IntensityObserver::save(const string_view dir, const string_view base_name) const
{
  // FIX: save what scenario is tracking for now, but should be converted
  return scenario_.saveIntensity(dir, base_name);
}
}
