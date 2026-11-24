/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_WEATHER_INDICES_H
#define FS_WEATHER_INDICES_H
#include "../types/Radians.h"
#include "../types/StrictType.h"
#include "../unstable.h"
namespace fs
{
struct Temperature : public StrictType<Temperature, units::Celsius>
{
  using StrictType::StrictType;
};
struct RelativeHumidity : public StrictType<RelativeHumidity, units::Percent>
{
  using StrictType::StrictType;
};
struct Speed : public StrictType<Speed, units::KilometresPerHour>
{
  using StrictType::StrictType;
};
struct Direction : public StrictType<Direction, units::CompassDegrees>
{
  using StrictType::StrictType;
  static consteval Direction Zero() { return Direction{0.0}; };
  static consteval Direction Invalid()
  {
    // NOTE: uint so -1 doesn't work
    return Direction{std::numeric_limits<DegreesSize>::max()};
  };
  constexpr Direction(const Degrees& degrees) : StrictType{degrees.value} { }
  constexpr Direction(const Radians& radians) : Direction{radians.asDegrees()} { }
  [[nodiscard]] constexpr MathSize asRadians() const
  {
    return Radians::from_degrees(asDegrees()).value;
  }
  [[nodiscard]] constexpr MathSize asDegrees() const { return value; }
  [[nodiscard]] constexpr Direction fix() const
  {
    auto r = asDegrees();
    while (r < 0)
    {
      r += 360;
    }
    while (r >= 360)
    {
      r -= 360;
    }
    return Direction{r};
  }
  [[nodiscard]] constexpr DegreesSize asDegreesSize() const
  {
    return static_cast<DegreesSize>(asDegrees());
  }
  [[nodiscard]] constexpr MathSize heading() const
  {
    return Radians::from_degrees(asDegrees()).to_heading().value;
  }
};
/**
 * \brief Wind with a Speed and Direction.
 */
struct Wind
{
  static constexpr auto units{std::tuple{Speed::units, Direction::units}};
  Speed speed{};
  Direction direction{};
  static consteval Wind Zero() { return {Speed::Zero(), Direction::Zero()}; };
  static consteval Wind Invalid() { return {Speed::Invalid(), Direction::Invalid()}; };
  auto operator<=>(const Wind& rhs) const = default;
  /**
   * \brief Direction wind is going towards
   * \return Direction wind is going towards
   */
  [[nodiscard]] constexpr MathSize heading() const noexcept { return direction.heading(); }
  /**
   * \brief X component of wind vector (km/h)
   * \return X component of wind vector (km/h)
   */
  [[nodiscard]] MathSize wsvX() const noexcept
  {
    // HACK: rounding error due to assignment before adding in old tests so keep for now
    volatile auto v = speed.value * sin(direction.heading());
    return v;
  }
  /**
   * \brief Y component of wind vector (km/h)
   * \return Y component of wind vector (km/h)
   */
  [[nodiscard]] MathSize wsvY() const noexcept
  {
    // HACK: rounding error due to assignment before adding in old tests so keep for now
    volatile auto v = speed.value * cos(direction.heading());
    return v;
  }
};
struct Precipitation : public StrictType<Precipitation, units::MillimetresAccumulated>
{
  using StrictType::StrictType;
};
/**
 * \brief Collection of weather indices used for calculating FwiWeather.
 */
struct Weather
{
  static constexpr auto units{
    std::tuple{Temperature::units, RelativeHumidity::units, Wind::units, Precipitation::units}
  };
  static consteval Weather Zero() { return {}; }
  static consteval Weather Invalid()
  {
    return {
      .temperature = Temperature::Invalid(),
      .rh = RelativeHumidity::Invalid(),
      .wind = Wind::Invalid(),
      .prec = Precipitation::Invalid()
    };
  }
  /**
   * \brief Temperature (Celsius)
   */
  Temperature temperature{};
  /**
   * \brief Relative Humidity (%)
   */
  RelativeHumidity rh{};
  /**
   * \brief Wind (km/h)
   */
  Wind wind{};
  /**
   * \brief Precipitation (1hr accumulation) (mm)
   */
  Precipitation prec{};
  auto operator<=>(const Weather& rhs) const = default;
};
}
#endif
