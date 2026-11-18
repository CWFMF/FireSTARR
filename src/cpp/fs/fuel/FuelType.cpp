/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FuelType.h"
#include "../log/Log.h"
#include "../sim/Settings.h"
#include "Greenup.h"
#include "Survival.h"
namespace fs::fuel
{
using namespace fs::survival;
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
MathSize fire_intensity(const RoundedValue& fc, const RoundedValue& ros)
{
  return 300.0 * fc * ros;
}
[[nodiscard]] FuelCodeSize FuelType::safeCode(const FuelType* fuel)
{
  return nullptr == fuel ? static_cast<FuelCodeSize>(INVALID_FUEL_CODE) : fuel->code();
}
[[nodiscard]] const char* FuelType::safeName(const FuelType* fuel)
{
  return nullptr == fuel ? "NULL" : fuel->name();
}
[[nodiscard]] MathSize FuelType::criticalRos(const RoundedValue& sfc, const RoundedIntensity& csi)
{
  return sfc > 0 ? csi / (300.0 * sfc) : 0.0;
}
[[nodiscard]] bool FuelType::isCrown(const RoundedIntensity& csi, const RoundedIntensity& sfi)
{
  return sfi > csi;
}
FuelType::FuelType(const FuelCodeSize& code, const char* name, const bool can_crown) noexcept
  : name_(name), can_crown_(can_crown), code_(code)
{ }
[[nodiscard]] bool FuelType::canCrown() const { return can_crown_; }
[[nodiscard]] MathSize FuelType::grass_curing(const int, const FwiWeather&) const
{
  // NOTE: grass overrides this but everything else doesn't have curing
  return INVALID_CURING;
}
[[nodiscard]] const char* FuelType::name() const { return name_; }
[[nodiscard]] FuelCodeSize FuelType::code() const { return code_; }
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
InvalidFuel::InvalidFuel(const FuelCodeSize& code, const char* name) noexcept
  : FuelType(code, name, false)
{ }
[[nodiscard]] bool InvalidFuel::isValid() const { return false; }
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
[[nodiscard]] const FuelType* InvalidFuel::summer() const noexcept { return this; }
[[nodiscard]] const FuelType* InvalidFuel::spring() const noexcept { return this; }
FuelBase::FuelBase(
  const FuelCodeSize& code,
  const char* name,
  const bool can_crown,
  const MathConstantSize& bulk_density,
  const MathConstantSize& inorganic_percent,
  const MathConstantSize& duff_depth,
  const Duff* duff_ffmc,
  const Duff* duff_dmc
)
  : FuelType(code, name, can_crown), bulk_density_(bulk_density),
    inorganic_percent_(inorganic_percent), duff_depth_(duff_depth), duff_ffmc_(duff_ffmc),
    duff_dmc_(duff_dmc)
{ }
[[nodiscard]] bool FuelBase::isValid() const { return true; }
[[nodiscard]] MathSize FuelBase::crownFractionBurned(
  const RoundedValue& rss,
  const RoundedValue& rso
) const noexcept
{
  // can't burn crown if it doesn't exist
  return cfl() > 0 ? max(0.0, 1.0 - exp(-0.230 * (rss - rso))) : 0.0;
}
[[nodiscard]] ThresholdSize FuelBase::probabilityPeat(const RoundedValue& mc_fraction
) const noexcept
{
  return probability_peat(bulkDensity(), inorganicPercent(), mc_fraction);
}
[[nodiscard]] ThresholdSize FuelBase::survivalProbability(const FwiWeather& wx) const noexcept
{
  return survival_probability(
    bulkDensity(), inorganicPercent(), *duffFfmcType(), *duffDmcType(), dmcRatio(), wx
  );
}
[[nodiscard]] MathSize FuelBase::bulkDensity() const
{
  return bulk_density_;
  // BulkDensity / 1000.0;
}
[[nodiscard]] MathSize FuelBase::inorganicPercent() const
{
  return inorganic_percent_;
  // InorganicPercent / 100.0;
}
[[nodiscard]] MathSize FuelBase::duffDepth() const
{
  return duff_depth_;
  // DuffDepth / 10.0;
}
[[nodiscard]] const Duff* FuelBase::duffDmcType() const { return duff_dmc_; }
[[nodiscard]] const Duff* FuelBase::duffFfmcType() const { return duff_ffmc_; }
[[nodiscard]] MathSize FuelBase::ffmcRatio() const { return 1 - dmcRatio(); }
[[nodiscard]] MathSize FuelBase::dmcRatio() const
{
  return (duffDepth() - fs::survival::DUFF_FFMC_DEPTH) / duffDepth();
}
[[nodiscard]] const FuelType* FuelBase::summer() const noexcept { return this; }
[[nodiscard]] const FuelType* FuelBase::spring() const noexcept { return this; }
}
