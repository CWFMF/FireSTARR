/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FuelType.h"
#include "../log/Log.h"
#include "../sim/Settings.h"
#include "Greenup.h"
namespace fs::fuel
{
string simplify_fuel_name(const string_view fuel)
{
  string simple_fuel_name{fuel};
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '-'), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), ' '), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '('), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), ')'), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '/'), simple_fuel_name.end()
  );
  std::transform(
    simple_fuel_name.begin(), simple_fuel_name.end(), simple_fuel_name.begin(), ::toupper
  );
  // remove PDF & PC
  const auto pc = simple_fuel_name.find("PC");
  if (string::npos != pc)
  {
    simple_fuel_name.erase(pc);
  }
  const auto pdf = simple_fuel_name.find("PDF");
  if (string::npos != pdf)
  {
    simple_fuel_name.erase(pdf);
  }
  return simple_fuel_name;
}
[[nodiscard]] const FuelType* FuelType::find_fuel_by_season(const int nd) const noexcept
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  // if not green yet, then still in spring conditions
  return settings.force_greenup    ? summer()
       : settings.force_no_greenup ? spring()
       : calculate_is_green(nd)    ? summer()
                                   : spring();
}
MathSize InvalidFuel::grass_curing(const int, const FwiWeather&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::cbh() const { throw runtime_error("Invalid fuel type in fuel map"); }
MathSize InvalidFuel::cfl() const { throw runtime_error("Invalid fuel type in fuel map"); }
MathSize InvalidFuel::buiEffect(const RoundedValue&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::crownConsumption(const RoundedValue&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::calculateRos(const int, const FwiWeather&, const RoundedValue&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::calculateIsf(const SpreadInfo&, const RoundedValue&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::surfaceFuelConsumption(const SpreadInfo&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::lengthToBreadth(const RoundedValue&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::
  finalRos(const SpreadInfo&, const RoundedValue&, const RoundedValue&, const RoundedValue&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::criticalSurfaceIntensity(const SpreadInfo&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::crownFractionBurned(const RoundedValue&, const RoundedValue&) const noexcept
{
  return logging::fatal("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::probabilityPeat(const RoundedValue&) const noexcept
{
  return logging::fatal("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::survivalProbability(const FwiWeather&) const noexcept
{
  return logging::fatal("Invalid fuel type in fuel map");
}
}
