/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "../stdafx.h"
#include "FireBehaviourPrediction.h"
#include "../spread/FireSpread.h"
#include "../unstable.h"
#include "../wx/FireWeatherIndices.h"
#include "Duff.h"
#include "Survival.h"
#ifdef DEBUG_FUEL_VARIABLE
#include "../log/Log.h"
#endif
namespace fs::fuel
{
using settings::Settings;
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
FuelNonMixed::FuelNonMixed(
  const FuelCodeSize& code,
  const char* name,
  const bool can_crown,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& inorganic_percent,
  const MathConstantSize& duff_depth,
  const Duff* duff_ffmc,
  const Duff* duff_dmc
)
  : StandardFuel(
      code,
      name,
      can_crown,
      log_q,
      a,
      b,
      c,
      bui0,
      cbh,
      cfl,
      bulk_density,
      inorganic_percent,
      duff_depth,
      duff_ffmc,
      duff_dmc
    )
{ }
FuelNonMixed::FuelNonMixed(
  const FuelCodeSize& code,
  const char* name,
  const bool can_crown,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& inorganic_percent,
  const MathConstantSize& duff_depth,
  const Duff* duff
)
  : FuelNonMixed(
      code,
      name,
      can_crown,
      log_q,
      a,
      b,
      c,
      bui0,
      cbh,
      cfl,
      bulk_density,
      inorganic_percent,
      duff_depth,
      duff,
      duff
    )
{ }
[[nodiscard]] MathSize FuelNonMixed::calculateIsf(const SpreadInfo& spread, const RoundedValue& isi)
  const noexcept
{
  return this->limitIsf(1.0, calculateRos(spread.nd(), spread.weather, isi) * spread.slopeFactor());
}
MathSize FuelNonMixed::calculateRos(const int, const FwiWeather&, const RoundedValue& isi)
  const noexcept
{
  return this->rosBasic(isi);
}
FuelConifer::FuelConifer(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& inorganic_percent,
  const MathConstantSize& duff_depth,
  const Duff* duff_ffmc,
  const Duff* duff_dmc
)
  : FuelNonMixed(
      code,
      name,
      true,
      log_q,
      a,
      b,
      c,
      bui0,
      cbh,
      cfl,
      bulk_density,
      inorganic_percent,
      duff_depth,
      duff_ffmc,
      duff_dmc
    )
{ }
FuelConifer::FuelConifer(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& inorganic_percent,
  const MathConstantSize& duff_depth,
  const Duff* duff
)
  : FuelConifer(
      code,
      name,
      log_q,
      a,
      b,
      c,
      bui0,
      cbh,
      cfl,
      bulk_density,
      inorganic_percent,
      duff_depth,
      duff,
      duff
    )
{ }
FuelJackpine::FuelJackpine(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& duff_depth,
  const Duff* duff_ffmc,
  const Duff* duff_dmc
)
  : FuelConifer(
      code,
      name,
      log_q,
      a,
      b,
      c,
      bui0,
      cbh,
      cfl,
      bulk_density,
      15,
      duff_depth,
      duff_ffmc,
      duff_dmc
    )
{ }
FuelJackpine::FuelJackpine(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& duff_depth,
  const Duff* duff
)
  : FuelJackpine(code, name, log_q, a, b, c, bui0, cbh, cfl, bulk_density, duff_depth, duff, duff)
{ }
[[nodiscard]] MathSize FuelJackpine::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_jackpine(as_value(spread.weather.bui()));
}
FuelPine::FuelPine(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& duff_depth,
  const Duff* duff_ffmc,
  const Duff* duff_dmc
)
  : FuelConifer(
      code,
      name,
      log_q,
      a,
      b,
      c,
      bui0,
      cbh,
      cfl,
      bulk_density,
      15,
      duff_depth,
      duff_ffmc,
      duff_dmc
    )
{ }
FuelPine::FuelPine(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& cbh,
  const MathConstantSize& cfl,
  const MathConstantSize& bulk_density,
  const MathConstantSize& duff_depth,
  const Duff* duff
)
  : FuelPine(code, name, log_q, a, b, c, bui0, cbh, cfl, bulk_density, duff_depth, duff, duff)
{ }
[[nodiscard]] MathSize FuelPine::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_pine(as_value(spread.weather.bui()));
}
FuelD1::FuelD1(const FuelCodeSize& code) noexcept
  : FuelNonMixed(code, "D-1", false, LOG_0_90, 30, 232, 160, 32, 0, 0, 61, 59, 24, &duff::Peat)
{ }
[[nodiscard]] MathSize FuelD1::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_d1(as_value(spread.weather.bui()));
}
MathSize FuelD1::isfD1(
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
FuelMixed::FuelMixed(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& ros_multiplier,
  const MathConstantSize& percent_mixed,
  const MathConstantSize& bulk_density,
  const MathConstantSize& inorganic_percent,
  const MathConstantSize& duff_depth
)
  : StandardFuel(
      code,
      name,
      true,
      log_q,
      a,
      b,
      c,
      bui0,
      6,
      80,
      bulk_density,
      inorganic_percent,
      duff_depth,
      &duff::Peat,
      &duff::Peat
    ),
    ros_multiplier_(ros_multiplier), percent_mixed_(percent_mixed)
{ }
[[nodiscard]] MathSize FuelMixed::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_mixed_or_c2(as_value(spread.weather.bui()));
}
[[nodiscard]] MathSize FuelMixed::crownConsumption(const RoundedValue& cfb) const noexcept
{
  return ratioConifer() * StandardFuel::crownConsumption(cfb);
}
[[nodiscard]] MathSize FuelMixed::calculateRos(
  const int,
  const FwiWeather&,
  const RoundedValue& isi
) const noexcept
{
  static const FuelD1 F{14};
  return ratioConifer() * this->rosBasic(isi)
       + rosMultiplier() * ratioDeciduous() * F.rosBasic(isi);
}
[[nodiscard]] MathSize FuelMixed::calculateIsf(const SpreadInfo& spread, const RoundedValue& isi)
  const noexcept
{
  return ratioConifer() * this->limitIsf(1.0, spread.slopeFactor() * this->rosBasic(isi))
       + ratioDeciduous() * isfD1(spread, isi);
}
[[nodiscard]] MathSize FuelMixed::percentMixed() const { return percent_mixed_; }
[[nodiscard]] MathSize FuelMixed::ratioConifer() const { return percent_mixed_ / 100.0; }
[[nodiscard]] MathSize FuelMixed::ratioDeciduous() const { return 1.0 - (percent_mixed_ / 100.0); }
[[nodiscard]] MathSize FuelMixed::rosMultiplier() const { return ros_multiplier_ / 10.0; }
[[nodiscard]] MathSize FuelMixed::isfD1(const SpreadInfo& spread, const RoundedValue& isi)
  const noexcept
{
  static const FuelD1 F{14};
  return F.isfD1(spread, rosMultiplier(), isi);
}
FuelMixedDead::FuelMixedDead(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& ros_multiplier,
  const MathConstantSize& percent_dead_fir
)
  : FuelMixed(code, name, log_q, a, b, c, bui0, ros_multiplier, percent_dead_fir, 61, 15, 75)
{ }
FuelMixedWood::FuelMixedWood(
  const FuelCodeSize& code,
  const char* name,
  const MathConstantSize& ros_multiplier,
  const MathConstantSize& percent_mixed
)
  : FuelMixed(code, name, LOG_0_80, 110, 282, 150, 50, ros_multiplier, percent_mixed, 108, 25, 50)
{ }
[[nodiscard]] MathSize FuelMixedWood::surfaceFuelConsumption(const SpreadInfo& spread
) const noexcept
{
  return this->ratioConifer() * FuelMixed::surfaceFuelConsumption(spread)
       + this->ratioDeciduous()
           * calculate_surface_fuel_consumption_d1(as_value(spread.weather.bui()));
}
FuelGrass::FuelGrass(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c
)
  // HACK: grass assumes no duff (total duff depth == ffmc depth => dmc depth is 0)
  : StandardFuel(
      code,
      name,
      false,
      log_q,
      a,
      b,
      c,
      1,
      0,
      0,
      0,
      0,
      static_cast<int>(fs::survival::DUFF_FFMC_DEPTH * 10.0),
      &duff::PeatMuck,
      &duff::PeatMuck
    )
{ }
[[nodiscard]] MathSize FuelGrass::surfaceFuelConsumption(const SpreadInfo&) const noexcept
{
  return DEFAULT_GRASS_FUEL_LOAD;
}
[[nodiscard]] MathSize FuelGrass::grass_curing(const int nd, const FwiWeather& wx) const
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  if (settings.static_curing.has_value())
  {
    return settings.static_curing.value();
  }
  const auto is_drought = as_value(wx.dc()) > 500;
  return is_drought ? 100 : calculate_grass_curing(nd);
}
[[nodiscard]] MathSize FuelGrass::baseMultiplier(const int nd, const FwiWeather& wx) const noexcept
{
  return calculate_base_multiplier_curing(grass_curing(nd, wx));
}
[[nodiscard]] MathSize FuelGrass::calculateIsf(const SpreadInfo& spread, const RoundedValue& isi)
  const noexcept
{
  const auto mu = baseMultiplier(spread.nd(), spread.weather);
  // prevent divide by 0
  const auto mu_not_zero = max(static_cast<MathSize>(0.001), mu);
  return this->limitIsf(mu_not_zero, calculateRos(mu, isi) * spread.slopeFactor());
}
[[nodiscard]] MathSize FuelGrass::calculateRos(
  const int nd,
  const FwiWeather& wx,
  const RoundedValue& isi
) const noexcept
{
  return calculateRos(baseMultiplier(nd, wx), isi);
}
[[nodiscard]] MathSize FuelGrass::lengthToBreadth(const RoundedValue& ws) const noexcept
{
  return calculate_length_to_breadth_grass(as_value(ws));
}
[[nodiscard]] MathSize FuelGrass::calculateRos(
  const RoundedValue& multiplier,
  const RoundedValue& isi
) const noexcept
{
  return multiplier * this->rosBasic(isi);
}
FuelC1::FuelC1(const FuelCodeSize& code) noexcept
  : FuelConifer(
      code,
      "C-1",
      LOG_0_90,
      90,
      649,
      450,
      72,
      2,
      75,
      45,
      5,
      34,
      &duff::Reindeer,
      &duff::Peat
    )
{ }
MathSize FuelC1::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c1(as_value(spread.weather.ffmc()));
}
FuelC2::FuelC2(const FuelCodeSize& code) noexcept
  : FuelConifer(code, "C-2", LOG_0_70, 110, 282, 150, 64, 3, 80, 34, 0, 100, &duff::SphagnumUpper)
{ }
MathSize FuelC2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_mixed_or_c2(as_value(spread.weather.bui()));
}
FuelC3::FuelC3(const FuelCodeSize& code) noexcept
  : FuelJackpine(
      code,
      "C-3",
      LOG_0_75,
      110,
      444,
      300,
      62,
      8,
      115,
      20,
      65,
      &duff::FeatherMoss,
      &duff::PineSeney
    )
{ }
FuelC4::FuelC4(const FuelCodeSize& code) noexcept
  : FuelJackpine(code, "C-4", LOG_0_80, 110, 293, 150, 66, 4, 120, 31, 62, &duff::PineSeney)
{ }
FuelC5::FuelC5(const FuelCodeSize& code) noexcept
  : FuelPine(code, "C-5", LOG_0_80, 30, 697, 400, 56, 18, 120, 93, 46, &duff::PineSeney)
{ }
FuelC6::FuelC6(const FuelCodeSize& code) noexcept
  : FuelPine(code, "C-6", LOG_0_80, 30, 800, 300, 62, 7, 180, 50, 50, &duff::PineSeney)
{ }
MathSize FuelC6::finalRos(
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
FuelC7::FuelC7(const FuelCodeSize& code) noexcept
  : FuelConifer(code, "C-7", LOG_0_85, 45, 305, 200, 106, 10, 50, 20, 15, 50, &duff::SprucePine)
{ }
MathSize FuelC7::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c7_ffmc(as_value(spread.weather.ffmc()))
       + calculate_surface_fuel_consumption_c7_bui(as_value(spread.weather.bui()));
}
FuelD2::FuelD2(const FuelCodeSize& code) noexcept
  : FuelNonMixed(code, "D-2", false, LOG_0_90, 6, 232, 160, 32, 0, 0, 61, 59, 24, &duff::Peat)
{ }
MathSize FuelD2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_d2(as_value(spread.weather.bui()));
}
MathSize FuelD2::calculateRos(const int, const FwiWeather& wx, const RoundedValue& isi)
  const noexcept
{
  return (as_value(wx.bui()) >= 80) ? rosBasic(isi) : 0.0;
}
FuelM1::FuelM1(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_conifer)
  : FuelMixedWood(code, name, 10, percent_conifer)
{ }
FuelM2::FuelM2(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_conifer)
  : FuelMixedWood(code, name, 2, percent_conifer)
{ }
FuelM3::FuelM3(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_dead_fir)
  : FuelMixedDead(code, name, LOG_0_80, 120, 572, 140, 50, 10, percent_dead_fir)
{ }
FuelM4::FuelM4(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_dead_fir)
  : FuelMixedDead(code, name, LOG_0_80, 100, 404, 148, 50, 2, percent_dead_fir)
{ }
FuelO1A::FuelO1A(const FuelCodeSize& code) noexcept
  : FuelGrass(code, "O-1a", LOG_1_00, 190, 310, 140)
{ }
MathSize compare_by_season(const FuelVariable& fuel, const function<MathSize(const FuelType&)>& fct)
{
  // HACK: no way to tell which is which, so let's assume they have to be the same??
  // HACK: use a function so that DEBUG section doesn't get out of sync
  const auto for_spring = fct(*fuel.spring());
#ifdef DEBUG_FUEL_VARIABLE
  const auto for_summer = fct(*fuel.summer());
  logging::check_fatal(for_spring != for_summer, "Expected spring and summer cfb to be identical");
#endif
  return for_spring;
}
FuelO1B::FuelO1B(const FuelCodeSize& code) noexcept
  : FuelGrass(code, "O-1b", LOG_1_00, 250, 350, 170)
{ }
FuelSlash::FuelSlash(
  const FuelCodeSize& code,
  const char* name,
  const LogValue log_q,
  const MathConstantSize& a,
  const MathConstantSize& b,
  const MathConstantSize& c,
  const MathConstantSize& bui0,
  const MathConstantSize& ffc_a,
  const MathConstantSize& ffc_b,
  const MathConstantSize& wfc_a,
  const MathConstantSize& wfc_b,
  const MathConstantSize& bulk_density,
  const Duff* duff_ffmc,
  const Duff* duff_dmc
)
  : FuelConifer(code, name, log_q, a, b, c, bui0, 0, 0, bulk_density, 15, 74, duff_ffmc, duff_dmc),
    ffc_a_(ffc_a), ffc_b_(ffc_b), wfc_a_(wfc_a), wfc_b_(wfc_b)
{ }
[[nodiscard]] MathSize FuelSlash::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return ffcA() * (1.0 - exp(ffcB() * as_value(spread.weather.bui())))
       + wfcA() * (1.0 - exp(wfcB() * as_value(spread.weather.bui())));
}
[[nodiscard]] MathSize FuelSlash::ffcA() const { return ffc_a_; }
[[nodiscard]] MathSize FuelSlash::ffcB() const { return ffc_b_ / 10000.0; }
[[nodiscard]] MathSize FuelSlash::wfcA() const { return wfc_a_; }
[[nodiscard]] MathSize FuelSlash::wfcB() const { return wfc_b_ / 10000.0; }
FuelS1::FuelS1(const FuelCodeSize& code) noexcept
  : FuelSlash(
      code,
      "S-1",
      LOG_0_75,
      75,
      297,
      130,
      38,
      4,
      -250,
      4,
      -340,
      78,
      &duff::FeatherMoss,
      &duff::PineSeney
    )
{ }
FuelS2::FuelS2(const FuelCodeSize& code) noexcept
  : FuelSlash(
      code,
      "S-2",
      LOG_0_75,
      40,
      438,
      170,
      63,
      10,
      -130,
      6,
      -600,
      132,
      &duff::FeatherMoss,
      &duff::WhiteSpruce
    )
{ }
FuelS3::FuelS3(const FuelCodeSize& code) noexcept
  : FuelSlash(
      code,
      "S-3",
      LOG_0_75,
      55,
      829,
      320,
      31,
      12,
      -166,
      20,
      -210,
      100,
      &duff::FeatherMoss,
      &duff::PineSeney
    )
{ }
FuelVariable::FuelVariable(
  const FuelCodeSize& code,
  const char* name,
  const FuelType* const spring,
  const FuelType* const summer
)
  : FuelType(code, name, spring->canCrown()), spring_(spring), summer_(summer)
{
  assert(spring->canCrown() == summer->canCrown());
}
[[nodiscard]] bool FuelVariable::isValid() const { return true; }
[[nodiscard]] MathSize FuelVariable::buiEffect(const RoundedValue& bui) const
{
  return compare_by_season(*this, [bui](const FuelType& fuel) { return fuel.buiEffect(bui); });
}
[[nodiscard]] MathSize FuelVariable::grass_curing(const int nd, const FwiWeather& wx) const
{
  return compare_by_season(*this, [&](const FuelType& fuel) { return fuel.grass_curing(nd, wx); });
}
[[nodiscard]] MathSize FuelVariable::cbh() const
{
  return compare_by_season(*this, [](const FuelType& fuel) { return fuel.cbh(); });
}
[[nodiscard]] MathSize FuelVariable::cfl() const
{
  return compare_by_season(*this, [](const FuelType& fuel) { return fuel.cfl(); });
}
[[nodiscard]] MathSize FuelVariable::crownConsumption(const RoundedValue& cfb) const
{
  return compare_by_season(*this, [cfb](const FuelType& fuel) {
    return fuel.crownConsumption(cfb);
  });
}
[[nodiscard]] MathSize FuelVariable::calculateRos(const int, const FwiWeather&, const RoundedValue&)
  const
{
  throw runtime_error("FuelVariable not resolved to specific type");
}
[[nodiscard]] MathSize FuelVariable::calculateIsf(const SpreadInfo&, const RoundedValue&) const
{
  throw runtime_error("FuelVariable not resolved to specific type");
}
[[nodiscard]] MathSize FuelVariable::surfaceFuelConsumption(const SpreadInfo&) const
{
  throw runtime_error("FuelVariable not resolved to specific type");
}
[[nodiscard]] MathSize FuelVariable::lengthToBreadth(const RoundedValue& ws) const
{
  return compare_by_season(*this, [ws](const FuelType& fuel) { return fuel.lengthToBreadth(ws); });
}
[[nodiscard]] MathSize FuelVariable::
  finalRos(const SpreadInfo&, const RoundedValue&, const RoundedValue&, const RoundedValue&) const
{
  throw runtime_error("FuelVariable not resolved to specific type");
}
[[nodiscard]] MathSize FuelVariable::criticalSurfaceIntensity(const SpreadInfo&) const
{
  throw runtime_error("FuelVariable not resolved to specific type");
}
[[nodiscard]] MathSize FuelVariable::crownFractionBurned(
  const RoundedValue& rss,
  const RoundedValue& rso
) const noexcept
{
  return spring()->crownFractionBurned(rss, rso);
}
[[nodiscard]] MathSize FuelVariable::probabilityPeat(const RoundedValue& mc_fraction) const noexcept
{
  return spring()->probabilityPeat(mc_fraction);
}
[[nodiscard]] MathSize FuelVariable::survivalProbability(const FwiWeather& wx) const noexcept
{
  return spring()->survivalProbability(wx);
}
[[nodiscard]] const FuelType* FuelVariable::spring() const noexcept { return spring_; }
[[nodiscard]] const FuelType* FuelVariable::summer() const noexcept { return summer_; }
MathSize FuelVariable::crownRateOfSpread(const RoundedValue&, const RoundedValue&) const noexcept
{
  return logging::fatal("FuelVariable not resolved to specific type");
}
FuelD1D2::FuelD1D2(const FuelCodeSize& code, const FuelD1* d1, const FuelD2* d2) noexcept
  : FuelVariable(code, "D-1/D-2", d1, d2)
{ }
FuelM1M2::FuelM1M2(
  const FuelCodeSize& code,
  const char* name,
  const FuelM1* m1,
  const FuelM2* m2,
  // HACK: to ensure they match for now
  const MathConstantSize&
#ifndef NDEBUG
    percent_conifer
#endif
)
  : FuelVariable(code, name, m1, m2)
{
  assert(m1->percentMixed() == m2->percentMixed());
  assert(m1->percentMixed() == percent_conifer);
}
FuelM3M4::FuelM3M4(
  const FuelCodeSize& code,
  const char* name,
  const FuelM3* m3,
  const FuelM4* m4,
  // HACK: to ensure they match for now
  const MathConstantSize&
#ifndef NDEBUG
    percent_dead_fir
#endif
)
  : FuelVariable(code, name, m3, m4)
{
  assert(m3->percentMixed() == m4->percentMixed());
  assert(m3->percentMixed() == percent_dead_fir);
}
// FIX: ensure actual code use in compilation doesn't matter and don't need to be speicified
// manually in sequence
static_assert(0 == INVALID_FUEL_CODE);
static fs::fuel::InvalidFuel NULL_FUEL{INVALID_FUEL_CODE, "Non-fuel"};
static fs::fuel::InvalidFuel INVALID{1, "Invalid"};
static FuelC1 C1{2};
static FuelC2 C2{3};
static FuelC3 C3{4};
static FuelC4 C4{5};
static FuelC5 C5{6};
static FuelC6 C6{7};
static FuelC7 C7{8};
static FuelD1 D1{9};
static FuelD2 D2{10};
static FuelO1A O1_A{11};
static FuelO1B O1_B{12};
static FuelS1 S1{13};
static FuelS2 S2{14};
static FuelS3 S3{15};
static FuelD1D2 D1_D2{16, &D1, &D2};
static FuelM1 M1_05{17, "M-1 (05 PC)", 5};
static FuelM1 M1_10{18, "M-1 (10 PC)", 10};
static FuelM1 M1_15{19, "M-1 (15 PC)", 15};
static FuelM1 M1_20{20, "M-1 (20 PC)", 20};
static FuelM1 M1_25{21, "M-1 (25 PC)", 25};
static FuelM1 M1_30{22, "M-1 (30 PC)", 30};
static FuelM1 M1_35{23, "M-1 (35 PC)", 35};
static FuelM1 M1_40{24, "M-1 (40 PC)", 40};
static FuelM1 M1_45{25, "M-1 (45 PC)", 45};
static FuelM1 M1_50{26, "M-1 (50 PC)", 50};
static FuelM1 M1_55{27, "M-1 (55 PC)", 55};
static FuelM1 M1_60{28, "M-1 (60 PC)", 60};
static FuelM1 M1_65{29, "M-1 (65 PC)", 65};
static FuelM1 M1_70{30, "M-1 (70 PC)", 70};
static FuelM1 M1_75{31, "M-1 (75 PC)", 75};
static FuelM1 M1_80{32, "M-1 (80 PC)", 80};
static FuelM1 M1_85{33, "M-1 (85 PC)", 85};
static FuelM1 M1_90{34, "M-1 (90 PC)", 90};
static FuelM1 M1_95{35, "M-1 (95 PC)", 95};
static FuelM2 M2_05{36, "M-2 (05 PC)", 5};
static FuelM2 M2_10{37, "M-2 (10 PC)", 10};
static FuelM2 M2_15{38, "M-2 (15 PC)", 15};
static FuelM2 M2_20{39, "M-2 (20 PC)", 20};
static FuelM2 M2_25{40, "M-2 (25 PC)", 25};
static FuelM2 M2_30{41, "M-2 (30 PC)", 30};
static FuelM2 M2_35{42, "M-2 (35 PC)", 35};
static FuelM2 M2_40{43, "M-2 (40 PC)", 40};
static FuelM2 M2_45{44, "M-2 (45 PC)", 45};
static FuelM2 M2_50{45, "M-2 (50 PC)", 50};
static FuelM2 M2_55{46, "M-2 (55 PC)", 55};
static FuelM2 M2_60{47, "M-2 (60 PC)", 60};
static FuelM2 M2_65{48, "M-2 (65 PC)", 65};
static FuelM2 M2_70{49, "M-2 (70 PC)", 70};
static FuelM2 M2_75{50, "M-2 (75 PC)", 75};
static FuelM2 M2_80{51, "M-2 (80 PC)", 80};
static FuelM2 M2_85{52, "M-2 (85 PC)", 85};
static FuelM2 M2_90{53, "M-2 (90 PC)", 90};
static FuelM2 M2_95{54, "M-2 (95 PC)", 95};
static FuelM1M2 M1_M2_05{55, "M-1/M-2 (05 PC)", &M1_05, &M2_05, 5};
static FuelM1M2 M1_M2_10{56, "M-1/M-2 (10 PC)", &M1_10, &M2_10, 10};
static FuelM1M2 M1_M2_15{57, "M-1/M-2 (15 PC)", &M1_15, &M2_15, 15};
static FuelM1M2 M1_M2_20{58, "M-1/M-2 (20 PC)", &M1_20, &M2_20, 20};
static FuelM1M2 M1_M2_25{59, "M-1/M-2 (25 PC)", &M1_25, &M2_25, 25};
static FuelM1M2 M1_M2_30{60, "M-1/M-2 (30 PC)", &M1_30, &M2_30, 30};
static FuelM1M2 M1_M2_35{61, "M-1/M-2 (35 PC)", &M1_35, &M2_35, 35};
static FuelM1M2 M1_M2_40{62, "M-1/M-2 (40 PC)", &M1_40, &M2_40, 40};
static FuelM1M2 M1_M2_45{63, "M-1/M-2 (45 PC)", &M1_45, &M2_45, 45};
static FuelM1M2 M1_M2_50{64, "M-1/M-2 (50 PC)", &M1_50, &M2_50, 50};
static FuelM1M2 M1_M2_55{65, "M-1/M-2 (55 PC)", &M1_55, &M2_55, 55};
static FuelM1M2 M1_M2_60{66, "M-1/M-2 (60 PC)", &M1_60, &M2_60, 60};
static FuelM1M2 M1_M2_65{67, "M-1/M-2 (65 PC)", &M1_65, &M2_65, 65};
static FuelM1M2 M1_M2_70{68, "M-1/M-2 (70 PC)", &M1_70, &M2_70, 70};
static FuelM1M2 M1_M2_75{69, "M-1/M-2 (75 PC)", &M1_75, &M2_75, 75};
static FuelM1M2 M1_M2_80{70, "M-1/M-2 (80 PC)", &M1_80, &M2_80, 80};
static FuelM1M2 M1_M2_85{71, "M-1/M-2 (85 PC)", &M1_85, &M2_85, 85};
static FuelM1M2 M1_M2_90{72, "M-1/M-2 (90 PC)", &M1_90, &M2_90, 90};
static FuelM1M2 M1_M2_95{73, "M-1/M-2 (95 PC)", &M1_95, &M2_95, 95};
static FuelM3 M3_05{74, "M-3 (05 PDF)", 5};
static FuelM3 M3_10{75, "M-3 (10 PDF)", 10};
static FuelM3 M3_15{76, "M-3 (15 PDF)", 15};
static FuelM3 M3_20{77, "M-3 (20 PDF)", 20};
static FuelM3 M3_25{78, "M-3 (25 PDF)", 25};
static FuelM3 M3_30{79, "M-3 (30 PDF)", 30};
static FuelM3 M3_35{80, "M-3 (35 PDF)", 35};
static FuelM3 M3_40{81, "M-3 (40 PDF)", 40};
static FuelM3 M3_45{82, "M-3 (45 PDF)", 45};
static FuelM3 M3_50{83, "M-3 (50 PDF)", 50};
static FuelM3 M3_55{84, "M-3 (55 PDF)", 55};
static FuelM3 M3_60{85, "M-3 (60 PDF)", 60};
static FuelM3 M3_65{86, "M-3 (65 PDF)", 65};
static FuelM3 M3_70{87, "M-3 (70 PDF)", 70};
static FuelM3 M3_75{88, "M-3 (75 PDF)", 75};
static FuelM3 M3_80{89, "M-3 (80 PDF)", 80};
static FuelM3 M3_85{90, "M-3 (85 PDF)", 85};
static FuelM3 M3_90{91, "M-3 (90 PDF)", 90};
static FuelM3 M3_95{92, "M-3 (95 PDF)", 95};
static FuelM3 M3_100{93, "M-3 (100 PDF)", 100};
static FuelM4 M4_05{94, "M-4 (05 PDF)", 5};
static FuelM4 M4_10{95, "M-4 (10 PDF)", 10};
static FuelM4 M4_15{96, "M-4 (15 PDF)", 15};
static FuelM4 M4_20{97, "M-4 (20 PDF)", 20};
static FuelM4 M4_25{98, "M-4 (25 PDF)", 25};
static FuelM4 M4_30{99, "M-4 (30 PDF)", 30};
static FuelM4 M4_35{100, "M-4 (35 PDF)", 35};
static FuelM4 M4_40{101, "M-4 (40 PDF)", 40};
static FuelM4 M4_45{102, "M-4 (45 PDF)", 45};
static FuelM4 M4_50{103, "M-4 (50 PDF)", 50};
static FuelM4 M4_55{104, "M-4 (55 PDF)", 55};
static FuelM4 M4_60{105, "M-4 (60 PDF)", 60};
static FuelM4 M4_65{106, "M-4 (65 PDF)", 65};
static FuelM4 M4_70{107, "M-4 (70 PDF)", 70};
static FuelM4 M4_75{108, "M-4 (75 PDF)", 75};
static FuelM4 M4_80{109, "M-4 (80 PDF)", 80};
static FuelM4 M4_85{110, "M-4 (85 PDF)", 85};
static FuelM4 M4_90{111, "M-4 (90 PDF)", 90};
static FuelM4 M4_95{112, "M-4 (95 PDF)", 95};
static FuelM4 M4_100{113, "M-4 (100 PDF)", 100};
static FuelM3M4 M3_M4_05{114, "M-3/M-4 (05 PDF)", &M3_05, &M4_05, 5};
static FuelM3M4 M3_M4_10{115, "M-3/M-4 (10 PDF)", &M3_10, &M4_10, 10};
static FuelM3M4 M3_M4_15{116, "M-3/M-4 (15 PDF)", &M3_15, &M4_15, 15};
static FuelM3M4 M3_M4_20{117, "M-3/M-4 (20 PDF)", &M3_20, &M4_20, 20};
static FuelM3M4 M3_M4_25{118, "M-3/M-4 (25 PDF)", &M3_25, &M4_25, 25};
static FuelM3M4 M3_M4_30{119, "M-3/M-4 (30 PDF)", &M3_30, &M4_30, 30};
static FuelM3M4 M3_M4_35{120, "M-3/M-4 (35 PDF)", &M3_35, &M4_35, 35};
static FuelM3M4 M3_M4_40{121, "M-3/M-4 (40 PDF)", &M3_40, &M4_40, 40};
static FuelM3M4 M3_M4_45{122, "M-3/M-4 (45 PDF)", &M3_45, &M4_45, 45};
static FuelM3M4 M3_M4_50{123, "M-3/M-4 (50 PDF)", &M3_50, &M4_50, 50};
static FuelM3M4 M3_M4_55{124, "M-3/M-4 (55 PDF)", &M3_55, &M4_55, 55};
static FuelM3M4 M3_M4_60{125, "M-3/M-4 (60 PDF)", &M3_60, &M4_60, 60};
static FuelM3M4 M3_M4_65{126, "M-3/M-4 (65 PDF)", &M3_65, &M4_65, 65};
static FuelM3M4 M3_M4_70{127, "M-3/M-4 (70 PDF)", &M3_70, &M4_70, 70};
static FuelM3M4 M3_M4_75{128, "M-3/M-4 (75 PDF)", &M3_75, &M4_75, 75};
static FuelM3M4 M3_M4_80{129, "M-3/M-4 (80 PDF)", &M3_80, &M4_80, 80};
static FuelM3M4 M3_M4_85{130, "M-3/M-4 (85 PDF)", &M3_85, &M4_85, 85};
static FuelM3M4 M3_M4_90{131, "M-3/M-4 (90 PDF)", &M3_90, &M4_90, 90};
static FuelM3M4 M3_M4_95{132, "M-3/M-4 (95 PDF)", &M3_95, &M4_95, 95};
static FuelM3M4 M3_M4_100{133, "M-3/M-4 (100 PDF)", &M3_100, &M4_100, 100};
static FuelM1 M1_00{134, "M-1 (00 PC)", 0};
static FuelM2 M2_00{135, "M-2 (00 PC)", 0};
static FuelM1M2 M1_M2_00{136, "M-1/M-2 (00 PC)", &M1_00, &M2_00, 0};
static FuelM3 M3_00{137, "M-3 (00 PDF)", 0};
static FuelM4 M4_00{138, "M-4 (00 PDF)", 0};
static FuelM3M4 M3_M4_00{139, "M-3/M-4 (00 PDF)", &M3_00, &M4_00, 0};
static FuelVariable O1{140, "O-1", &O1_A, &O1_B};
const array<const FuelType*, NUMBER_OF_FUELS> Fuels{
  &NULL_FUEL, &INVALID,  &C1,       &C2,        &C3,       &C4,       &C5,       &C6,
  &C7,        &D1,       &D2,       &O1_A,      &O1_B,     &S1,       &S2,       &S3,
  &D1_D2,     &M1_05,    &M1_10,    &M1_15,     &M1_20,    &M1_25,    &M1_30,    &M1_35,
  &M1_40,     &M1_45,    &M1_50,    &M1_55,     &M1_60,    &M1_65,    &M1_70,    &M1_75,
  &M1_80,     &M1_85,    &M1_90,    &M1_95,     &M2_05,    &M2_10,    &M2_15,    &M2_20,
  &M2_25,     &M2_30,    &M2_35,    &M2_40,     &M2_45,    &M2_50,    &M2_55,    &M2_60,
  &M2_65,     &M2_70,    &M2_75,    &M2_80,     &M2_85,    &M2_90,    &M2_95,    &M1_M2_05,
  &M1_M2_10,  &M1_M2_15, &M1_M2_20, &M1_M2_25,  &M1_M2_30, &M1_M2_35, &M1_M2_40, &M1_M2_45,
  &M1_M2_50,  &M1_M2_55, &M1_M2_60, &M1_M2_65,  &M1_M2_70, &M1_M2_75, &M1_M2_80, &M1_M2_85,
  &M1_M2_90,  &M1_M2_95, &M3_05,    &M3_10,     &M3_15,    &M3_20,    &M3_25,    &M3_30,
  &M3_35,     &M3_40,    &M3_45,    &M3_50,     &M3_55,    &M3_60,    &M3_65,    &M3_70,
  &M3_75,     &M3_80,    &M3_85,    &M3_90,     &M3_95,    &M3_100,   &M4_05,    &M4_10,
  &M4_15,     &M4_20,    &M4_25,    &M4_30,     &M4_35,    &M4_40,    &M4_45,    &M4_50,
  &M4_55,     &M4_60,    &M4_65,    &M4_70,     &M4_75,    &M4_80,    &M4_85,    &M4_90,
  &M4_95,     &M4_100,   &M3_M4_00, &M3_M4_05,  &M3_M4_10, &M3_M4_15, &M3_M4_20, &M3_M4_25,
  &M3_M4_30,  &M3_M4_35, &M3_M4_40, &M3_M4_45,  &M3_M4_50, &M3_M4_55, &M3_M4_60, &M3_M4_65,
  &M3_M4_70,  &M3_M4_75, &M3_M4_80, &M3_M4_85,  &M3_M4_90, &M3_M4_95, &M1_00,    &M2_00,
  &M1_M2_00,  &M3_00,    &M4_00,    &M3_M4_100, &O1,
};
}
