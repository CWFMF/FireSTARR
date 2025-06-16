/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Observer.h"
namespace fs::sim
{
static constexpr array_pts NoPoints{};
string IObserver::makeName(const string& base_name, const string& suffix)
{
  if (base_name.length() > 0)
  {
    return base_name + "_" + suffix;
  }
  return suffix;
}

constexpr ROSSize NODATA_ROS = 0;
ROSSize FBPObserver::getValue(const Event& event) const noexcept
{
  ROSSize max_ros = 0.0;
  auto points_before = event.points_before();
  auto points_after = event.points_after();

  // Ensure points before is defined
  if (NoPoints == points_before)
  {
    return (max_ros);
  }

  for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
  {
     const auto& p0 = points_before[i];
     const auto& x0 = p0.first;
     const auto& y0 = p0.second;
     const auto& p1 = points_after[i];
     const auto& x1 = p1.first;
     const auto& y1 = p1.second;

     const auto d = ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1)) * 100.0; // TODO: this should be cell size
     const auto ros = d / event.duration(); // Is duration in min or sec?
     if (ros > max_ros)
     {
       max_ros = ros;
       //calculate direction between p0 and p1 (compass, not math)
       // calculate intensity from spread_info sfc + ros?
     }
   }

  return max_ros;
}
void FBPObserver::handleEvent(const Event& event) noexcept
{
  // Compare current ROS to past max
  // is there a value in map
  auto value = getValue(event);
  auto& cell = event.cell();
  auto& map = *ros_map_;
  if (!map.contains(cell))
  {
    map.set(cell, value);
  }
  else
  {
    auto value_old = map.at(cell);
    if (value > value_old)
    {
      map.set(cell, value);
    }
  }
}
/**
 * \brief Save observations
 * \param dir Directory to save to
 * \param base_name Base file name to save to
 */
void FBPObserver::save(const string& dir, const string& base_name) const
{
  ros_map_->saveToFile(dir, makeName(base_name, "ros"));
}
/**
 * \brief Clear all observations
 */
void FBPObserver::reset() noexcept
{
  ros_map_->clear();
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
}
