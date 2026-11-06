/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FBPOld.h"
#include "unstable.h"
namespace fs::fuelold
{
MathSize calculate_surface_fuel_consumption_mixed_or_c2(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 5.0 * (1.0 - exp(-0.0115 * bui));
}
MathSize calculate_surface_fuel_consumption_d1(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 1.5 * (1.0 - exp(-0.0183 * bui));
}
MathSize calculate_base_multiplier_curing(const MathSize c) noexcept
{
  const auto curing = round_value(c);
  return (curing >= 58.8) ? (0.176 + 0.02 * (curing - 58.8)) : (0.005 * expm1(0.061 * curing));
}
MathSize calculate_length_to_breadth_grass(const MathSize w) noexcept
{
  const auto ws = round_value(w);
  return ws < 1.0 ? 1.0 : (1.1 * pow(ws, 0.464));
}
MathSize calculate_surface_fuel_consumption_pine(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 5.0 * pow(1.0 - exp(-0.0149 * bui), 2.48);
}
MathSize calculate_surface_fuel_consumption_jackpine(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 5.0 * pow(1.0 - exp(-0.0164 * bui), 2.24);
}
MathSize FuelOldD1::isfD1(
  const SpreadInfo& spread,
  const MathSize ros_multiplier,
  const MathSize isi
) const noexcept
{
  return limitIsf(
    ros_multiplier,
    spread.slopeFactor() * (ros_multiplier * a()) * pow(1.0 - exp(negB() * isi), c())
  );
}
/**
 * \brief Surface Fuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 * \param ffmc Fine Fuel Moisture Code
 * \return Surface Fuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c1(const MathSize f) noexcept
{
  const auto ffmc = round_value(f);
  return max(0.0, 0.75 + ((ffmc > 84) ? 0.75 : -0.75) * sqrt(1 - exp(-0.23 * abs(ffmc - 84))));
}
MathSize FuelOldC1::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c1(spread.weather->ffmc.value);
}
MathSize FuelOldC2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_mixed_or_c2(spread.weather->bui.value);
}
MathSize FuelOldC6::finalRos(
  const SpreadInfo& spread,
  const MathSize isi,
  const MathSize cfb,
  const MathSize rss
) const noexcept
{
  const auto rsc = crownRateOfSpread(isi, spread.foliarMoisture());
  // using max with 0 is the same as ensuring rsc > rss
  return rss + cfb * max(0.0, rsc - rss);
}
/**
 * \brief Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 * \param ffmc Fine Fuel Moisture Code
 * \return Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c7_ffmc(const MathSize f) noexcept
{
  const auto ffmc = round_value(f);
  return (ffmc > 70) ? 2.0 * (1.0 - exp(-0.104 * (ffmc - 70.0))) : 0.0;
}
/**
 * \brief Woody Fuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 * \return Woody Fuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c7_bui(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 1.5 * (1.0 - exp(-0.0201 * bui));
}
MathSize FuelOldC7::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c7_ffmc(spread.weather->ffmc.value)
       + calculate_surface_fuel_consumption_c7_bui(spread.weather->bui.value);
}
[[nodiscard]] MathSize calculate_surface_fuel_consumption_d2(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return bui >= 80 ? 1.5 * (1.0 - exp(-0.0183 * bui)) : 0.0;
}
MathSize FuelOldD2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_d2(spread.weather->bui.value);
}
MathSize FuelOldD2::calculateRos(const int, const FwiWeather& wx, const MathSize isi) const noexcept
{
  return (wx.bui.value >= 80) ? rosBasic(isi) : 0.0;
}
}
