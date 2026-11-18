/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FBPOld.h"
#include "../fs/unstable.h"
namespace fs::fuelold
{
MathSize calculate_surface_fuel_consumption_jackpine(const LookupValue& bui_) noexcept
{
  const auto bui = round_value(bui_);
  return 5.0 * pow(1.0 - exp(-0.0164 * bui), 2.24);
}
MathSize calculate_surface_fuel_consumption_pine(const LookupValue& bui_) noexcept
{
  const auto bui = round_value(bui_);
  return 5.0 * pow(1.0 - exp(-0.0149 * bui), 2.48);
}
MathSize calculate_surface_fuel_consumption_c1(const LookupValue& ffmc_) noexcept
{
  const auto ffmc = round_value(ffmc_);
  return max(0.0, 0.75 + ((ffmc > 84) ? 0.75 : -0.75) * sqrt(1 - exp(-0.23 * abs(ffmc - 84))));
}
MathSize calculate_surface_fuel_consumption_mixed_or_c2(const LookupValue& bui_) noexcept
{
  const auto bui = round_value(bui_);
  return 5.0 * (1.0 - exp(-0.0115 * bui));
}
MathSize calculate_surface_fuel_consumption_c7_ffmc(const LookupValue& ffmc_) noexcept
{
  const auto ffmc = round_value(ffmc_);
  return (ffmc > 70) ? 2.0 * (1.0 - exp(-0.104 * (ffmc - 70.0))) : 0.0;
}
MathSize calculate_surface_fuel_consumption_c7_bui(const LookupValue& bui_) noexcept
{
  const auto bui = round_value(bui_);
  return 1.5 * (1.0 - exp(-0.0201 * bui));
}
MathSize calculate_surface_fuel_consumption_d1(const LookupValue& bui_) noexcept
{
  const auto bui = round_value(bui_);
  return 1.5 * (1.0 - exp(-0.0183 * bui));
}
MathSize calculate_surface_fuel_consumption_d2(const LookupValue& bui_) noexcept
{
  const auto bui = round_value(bui_);
  return bui >= 80 ? 1.5 * (1.0 - exp(-0.0183 * bui)) : 0.0;
}
MathSize calculate_base_multiplier_curing(const LookupValue& curing_) noexcept
{
  const auto curing = round_value(curing_);
  return (curing >= 58.8) ? (0.176 + 0.02 * (curing - 58.8)) : (0.005 * expm1(0.061 * curing));
}
MathSize calculate_length_to_breadth_grass(const LookupValue& ws_) noexcept
{
  const auto ws = round_value(ws_);
  return ws < 1.0 ? 1.0 : (1.1 * pow(ws, 0.464));
}
MathSize FuelOldD1::isfD1(
  const SpreadInfo& spread,
  const RoundedValue& ros_multiplier,
  const RoundedValue& isi
) const noexcept
{
  return limitIsf(
    ros_multiplier,
    spread.slopeFactor() * (ros_multiplier * a()) * pow(1.0 - exp(negB() * isi), c())
  );
}
MathSize FuelOldC1::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c1(spread.weather.ffmc().value);
}
MathSize FuelOldC2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_mixed_or_c2(spread.weather.bui().value);
}
MathSize FuelOldC6::finalRos(
  const SpreadInfo& spread,
  const RoundedValue& isi,
  const RoundedValue& cfb,
  const RoundedValue& rss
) const noexcept
{
  const auto rsc = crownRateOfSpread(isi, spread.foliarMoisture());
  // using max with 0 is the same as ensuring rsc > rss
  return rss + cfb * max(0.0, rsc - rss);
}
MathSize FuelOldC7::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c7_ffmc(spread.weather.ffmc().value)
       + calculate_surface_fuel_consumption_c7_bui(spread.weather.bui().value);
}
MathSize FuelOldD2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_d2(spread.weather.bui().value);
}
MathSize FuelOldD2::calculateRos(const int, const FwiWeather& wx, const RoundedValue& isi)
  const noexcept
{
  return (wx.bui().value >= 80) ? rosBasic(isi) : 0.0;
}
}
