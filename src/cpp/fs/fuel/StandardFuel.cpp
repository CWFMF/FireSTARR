/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "StandardFuel.h"
namespace fs::fuel
{
MathSize calculate_standard_foliar_moisture_fmc(const LookupValue& fmc_) noexcept
{
  const auto fmc = round_value(fmc_);
  return pow_int<4>(1.5 - 0.00275 * fmc) / (460.0 + 25.9 * fmc) / 0.778 * 1000.0;
}
MathSize calculate_standard_foliar_moisture_isi(const LookupValue& isi_) noexcept
{
  const auto isi = round_value(isi_);
  return 60.0 * (1.0 - exp(-0.0497 * isi));
}
MathSize calculate_standard_length_to_breadth(const LookupValue& ws_) noexcept
{
  const auto ws = round_value(ws_);
  return 1.0 + 8.729 * pow(1.0 - exp(-0.030 * ws), 2.155);
}
MathSize calculate_standard_rsc(const RoundedValue& isi, const RoundedValue& fmc) noexcept
{
  // HACK: convert back to double to instantiate as LookupValue
  return calculate_standard_foliar_moisture_isi(as_value(isi))
       * calculate_standard_foliar_moisture_fmc(as_value(fmc));
}
MathSize calculate_standard_final_ros(
  const RoundedValue&,
  const RoundedValue&,
  const RoundedValue&,
  const RoundedValue& rss
)
{
  return rss;
}
MathSize calculate_bui_effect(
  const MathConstantSize& log_q,
  const MathConstantSize& bui0,
  const RoundedValue& bui
) noexcept
{
  return (0 < bui) ? exp(50.0 * log_q * ((1.0 / bui) - (1.0 / bui0))) : 1.0;
}
MathSize calculate_ros_basic(
  const MathConstantSize& a,
  const MathConstantSize& neg_b,
  const MathConstantSize& c,
  const RoundedValue& isi
) noexcept
{
  return a * pow(1.0 - exp(neg_b * isi), c);
}
StandardFuel::StandardFuel(
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
) noexcept
  : FuelBase(
      code,
      name,
      can_crown,
      // FIX: change these to actual numbers when we get to derived class
      bulk_density / 1000.0,
      inorganic_percent / 100.0,
      duff_depth / 10.0,
      duff_ffmc,
      duff_dmc
    ),
    log_q_(log_q), a_(a), b_(b), c_(c), bui0_(bui0), cbh_(cbh), cfl_(cfl)
{
  assert(-negB() < 1);
  assert(StandardFuel::c() < 10 && StandardFuel::c() > 1);
}
StandardFuel::StandardFuel(
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
) noexcept
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
      duff,
      duff
    )
{ }
[[nodiscard]] MathSize StandardFuel::rosBasic(const RoundedValue& isi) const noexcept
{
  return calculate_ros_basic(a(), negB(), c(), isi);
}
MathSize StandardFuel::crownConsumption(const RoundedValue& cfb) const noexcept
{
  return cfl() * cfb;
}
[[nodiscard]] MathSize StandardFuel::limitIsf(const RoundedValue& mu, const RoundedValue& rsf)
  const noexcept
{
  return (1.0 / negB())
       * log(max(
         SLOPE_LIMIT_ISI,
         static_cast<MathSize>(
           (rsf > static_cast<MathSize>(0.0)) ? (1.0 - pow((rsf / (mu * a())), (1.0 / c()))) : 1.0
         )
       ));
}
[[nodiscard]] MathSize StandardFuel::criticalSurfaceIntensity(const SpreadInfo& spread
) const noexcept
{
  return 0.001 * pow(cbh(), 1.5) * pow(460.0 + 25.9 * spread.foliarMoisture(), 1.5);
}
[[nodiscard]] MathSize StandardFuel::lengthToBreadth(const RoundedValue& ws) const noexcept
{
  return calculate_standard_length_to_breadth(as_value(ws));
}
MathSize StandardFuel::finalRos(
  const RoundedValue& foliar_moisture,
  const RoundedValue& isi,
  const RoundedValue& cfb,
  const RoundedValue& rss
) const
{
  return calculate_standard_final_ros(foliar_moisture, isi, cfb, rss);
}
[[nodiscard]] MathSize StandardFuel::buiEffect(const RoundedValue& bui) const noexcept
{
  return calculate_bui_effect(logQ(), bui0(), bui);
}
[[nodiscard]] MathSize StandardFuel::bui0() const noexcept { return bui0_; }
[[nodiscard]] MathSize StandardFuel::cbh() const { return cbh_; }
[[nodiscard]] MathSize StandardFuel::cfl() const { return cfl_ / 100.0; }
[[nodiscard]] MathSize StandardFuel::a() const noexcept { return a_; }
[[nodiscard]] MathSize StandardFuel::negB() const noexcept
{
  // the only places this gets used it gets negated so just store it that way
  return -b_ / 10000.0;
}
[[nodiscard]] MathSize StandardFuel::c() const noexcept { return c_ / 100.0; }
[[nodiscard]] MathSize StandardFuel::crownRateOfSpread(
  const RoundedValue& isi,
  const RoundedValue& fmc
) const noexcept
{
  return calculate_standard_rsc(isi, fmc);
}
[[nodiscard]] MathSize StandardFuel::logQ() const noexcept { return as_value(log_q_); }
}
