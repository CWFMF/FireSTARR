/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FIRE_WEATHER_H
#define FS_FIRE_WEATHER_H
#include "../stdafx.h"
#include "FireWeatherIndices.h"
#include "FwiWeather.h"
namespace fs
{
/**
 * \brief A stream of weather that gets used by a Scenario every Iteration.
 */
class FireWeather
{
public:
  virtual ~FireWeather() = default;
  /**
   * \brief Constructor
   * \param used_fuels set of FuelTypes that are used in the simulation
   * \param data map of Day to FwiWeather to use for weather stream with diurnal formula
   */
  FireWeather(const map<Day, FwiWeather>& data);
  FireWeather(Day min_date, Day max_date, vector<FwiWeather> weather_by_hour_by_day);
  /**
   * \brief A Constant weather stream with only one possible fuel
   * \param fuel Fuel to use
   * \param start_date Start date for stream
   * \param dc Drought Code
   * \param bui Build-up Index
   * \param dmc Duff Moisture Code
   * \param ffmc Fine Fuel Moisture Code
   * \param wind Wind
   */
  FireWeather(
    const Day start_date,
    const Dc& dc,
    const Dmc& dmc,
    const Ffmc& ffmc,
    const Wind& wind
  );
  FireWeather(FireWeather&& rhs) = default;
  FireWeather(const FireWeather& rhs) = delete;
  FireWeather& operator=(FireWeather&& rhs) noexcept = default;
  FireWeather& operator=(const FireWeather& rhs) = delete;
  /**
   * \brief Get FwiWeather for given time
   * \param time Time to get weather for
   * \return FwiWeather for given time
   */
  [[nodiscard]] FwiWeather at(const DurationSize time) const;
  /**
   * \brief Probability of survival in given fuel at given time
   * \param time Time to get survival probability for
   * \param in_fuel FuelCodeSize of FuelType to use
   * \return Probability of survival in given fuel at given time
   */
  [[nodiscard]] ThresholdSize survivalProbability(
    const DurationSize time,
    const FuelCodeSize& in_fuel
  ) const;
  /**
   * \brief Minimum date present in FireWeather
   * \return Minimum date present in FireWeather
   */
  [[nodiscard]] constexpr Day minDate() const { return min_date_; }
  /**
   * \brief Maximum date present in FireWeather
   * \return Maximum date present in FireWeather
   */
  [[nodiscard]] constexpr Day maxDate() const { return max_date_; }
  /**
   * \brief Weather by hour by day
   * \return Weather by hour by day
   */
  [[nodiscard]] const vector<FwiWeather>& getWeather() const noexcept
  {
    return weather_by_hour_by_day_;
  }

private:
  /**
   * \brief FwiWeather by hour by Day
   */
  vector<FwiWeather> weather_by_hour_by_day_{};
  /**
   * \brief Minimum date present in stream
   */
  Day min_date_;
  /**
   * \brief Maximum date present in stream
   */
  Day max_date_;
};
/**
 * \brief Equality operator
 * \param lhs First FireWeather
 * \param rhs Second FireWeather
 * \return Whether or not the two FireWeathers are equal
 */
[[nodiscard]] constexpr bool operator==(const FireWeather& lhs, const FireWeather& rhs)
{
  if (!(lhs.maxDate() == rhs.maxDate() && lhs.minDate() == rhs.minDate()))
  {
    return false;
  }
  // FIX: why is this a warning?
  for (Day day = lhs.minDate() + static_cast<Day>(1); day <= lhs.maxDate(); ++day)
  {
    for (auto hour = 0; hour < DAY_HOURS; ++hour)
    {
      const auto time = static_cast<DurationSize>(day) + hour / 24.0;
      if (lhs.at(time) != rhs.at(time))
      {
        return false;
      }
    }
  }
  return true;
}
/**
 * \brief Inequality operator
 * \param lhs First FireWeather
 * \param rhs Second FireWeather
 * \return Whether or not they are not equivalent
 */
[[nodiscard]] constexpr bool operator!=(const FireWeather& lhs, const FireWeather& rhs)
{
  return !(lhs == rhs);
}
}
#endif
