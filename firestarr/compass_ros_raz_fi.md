Scenario.cpp
`void Scenario::notify(const Event& event) const` will send an event to the observers
- modify Event to have the points that were in the cell pre/post spreading

add 2 to Event:
using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;

keep this `  CellPointsMap points_;` in Scenario and then compare between that and what it ends up as with

`run()` is initialization of Scenario
spread is in `void Scenario::scheduleFireSpread(const Event& event)`
`it = points_.map_.erase(it);` is modifying points in place, so just make a copy before `while (it != points_.map_.end())`

decides what duration to use for this time step based on max ROS and max distance per step
```
  const auto duration = ((max_ros_ > 0)
                           ? min(max_duration,
                                 Settings::maximumSpreadDistance() * cellSize() / max_ros_)
                           : max_duration);
```
```
# maximum distance that head can spread per step (* cell size)
MAX_SPREAD_DISTANCE = 0.4
```

this does the spreading:
```
  CellPointsMap cell_pts{};
  auto spread = std::views::transform(
    to_spread,
    [this, &duration, &new_time](
      spreading_points::value_type& kv0) -> CellPointsMap {
      auto& key = kv0.first;
      const auto& offsets = spread_info_[key].offsets();
      spreading_points::mapped_type& cell_pts = kv0.second;
      auto r = apply_offsets_spreadkey(new_time, duration, offsets, cell_pts);
      return r;
    });
```

`CellPointsMap` is all cells and the `CellPoints` in each of them, as a map based on Cell location `map<Location, CellPoints> map_;`

```
CellPointsMap& CellPointsMap::merge(
  const BurnedData& unburnable,
  const CellPointsMap& rhs) noexcept
```
is used to take what was spread into each cell and combine them all together again with the hope that it could compile to do that in parallel


`CellIndex src_;` in `CellPoints` is a mask of all the direction sources that things spread into the cell from

You just need to get the `CellPointArrays pts_;` out of each `CellPoints` pre/post spread and compare those



Observer.h
Implement something like
```
class RosObserver final
  : public MapObserver<ROSSize>
```
except use `[[nodiscard]] ROSSize getValue(const Event& event) const noexcept override` to calculate the distance between compass points pre/post spread

extend `const Event& event` to have pre/post points

```
        const auto fake_event = Event::makeFireSpread(
          new_time,
          spread.intensity(),
          spread.ros(),
          spread.direction(),
          for_cell,
          pts.sources());
```
is used to make an Event that has the attributes that should be recorded by the observers that care about them
So if you add pre/post points to that constructor then the Event can have them and you'd define `getValue()` to return the ROS/FI/RAZ you calculate from the difference between pre/post points

All Events have these attributes regardless of Event
```
  /**
   * \brief Time to schedule for
   */
  DurationSize time_;
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   */
  DurationSize time_at_location_;
  /**
   * \brief Cell to spread in
   */
  Cell cell_;
  /**
   * \brief Type of Event
   */
  Type type_;
  IntensitySize intensity_;
  ROSSize ros_;
  Direction raz_;
  // /**
  //  * \brief Spread information at time and place of event
  //  */
  // const SpreadInfo* spread_info_;
  /**
   * \brief CellIndex for relative Cell that spread into from
   */
  CellIndex source_;
```


CellPoints.h
`class CellPointArrays` is the data that's inside a Cell about the points in it and the distance of them to the compass point they're the closest point to

CellPoints.cpp
```
CellPoints& CellPoints::insert(
  const XYPos& src,
  const SpreadData& spread_current,
  const XYSize x,
  const XYSize y) noexcept
```
is where the distance comparison code is right now
```
    const auto& x1 = p1.first;
    const auto& y1 = p1.second;
    const auto d = ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
```
is the distance between two points. Take that code and apply it over two CellPointArrays. e.g.
```
for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
{
    const auto& p0 = points_pre[i];
    const auto& x0 = p0.first;
    const auto& y0 = p0.second;
    const auto& p1 = points_post[i];
    const auto& x1 = p1.first;
    const auto& y1 = p1.second;
    d[i] = ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}
```
The distance in `d` would be part of a vector, because it's the change in where the point closest to that compass direction is. You can use the distance as the ROS if you divide it by how long the time step was. Should be able to calculate the angle between p0 & p1 also - make sure to convert it to a compass angle and not a math angle.

Record `d / time` or whatever the calculated ROS is as the ROS for each direction. You probably don't need the angles yet, because you'd go through `d` and find the maximum, and then you only care about the direction for that value of `i` because that's the direction it spread the furthest/fastest.

So you'd have distance & direction from that. Keep those and then when you do the next step in the iteration you do the same thing, but only keep it if it's above the max ROS/direction you already have.

FI comes from SFC & ROS I think? Just calculate it from the SpreadInfo


Considerations:
- sometimes a point will spread into another cell, so you wouldn't be moving the compass point within the cell and you need to compare to where it landed outside the cell
    - this probably doesn't work if you clip a corner, so maybe it's just points within the cell that matter?
if you compare the place it lands to the points along the edge of the cell, then if it clipped a corner it would be really far from that corner on the next step, and the closest thing to that corner would still be where it came from
but if e.g. you go from the center to [0.8, 0.8] and then the next step is [1.01, 1.01] (just outside cell) then that would still be closer to [1.0, 1.0] (the NE(?) cell)
