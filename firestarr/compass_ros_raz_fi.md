Scenario.cpp
`void Scenario::notify(const Event& event) const` will send an event to the observers
- modify Event to have the points that were in the cell pre/post spreading

Observer.h
Implement something like
```
class RosObserver final
  : public MapObserver<ROSSize>
```
except use `[[nodiscard]] ROSSize getValue(const Event& event) const noexcept override` to calculate the distance between compass points pre/post spread

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
