/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SIMPLE_STANDARDFUEL
#define FS_SIMPLE_STANDARDFUEL
#include "stdafx.h"
#include "LogValue.h"
#include "SimpleFuelType.h"
namespace fs::simplefbp
{
/**
 * \brief Limit to slope when calculating ISI
 */
static constexpr MathSize SLOPE_LIMIT_ISI = 0.01;
/**
 * \brief Calculate standard foliar moisture effect (FME) based on FMC [ST-X-3 eq 61]
 * \param fmc Foliar Moisture Content (FMC)
 * \return Standard foliar moisture effect (FME) based on FMC [ST-X-3 eq 61]
 */
[[nodiscard]] static MathSize calculate_standard_foliar_moisture_fmc(const MathSize f) noexcept
{
  const auto fmc = round_value(f);
  return pow_int<4>(1.5 - 0.00275 * fmc) / (460.0 + 25.9 * fmc) / 0.778 * 1000.0;
}
/**
 * \brief Crown fire spread rate (m/min) / Foliar Moisture Effect (RSC / (FME / FME_avg)) [ST-X-3 eq
 * 64]
 * \param isi Initial Spread Index
 * \return RSC / (FME / FME_avg) [ST-X-3 eq 64]
 */
[[nodiscard]] static MathSize calculate_standard_foliar_moisture_isi(const MathSize i) noexcept
{
  const auto isi = round_value(i);
  return 60.0 * (1.0 - exp(-0.0497 * isi));
}
/**
 * \brief Length to Breadth ratio [ST-X-3 eq 79]
 * \param ws Wind Speed (km/h)
 * \return Length to Breadth ratio [ST-X-3 eq 79]
 */
[[nodiscard]] static MathSize calculate_standard_length_to_breadth(const MathSize w) noexcept
{
  const auto ws = round_value(w);
  return 1.0 + 8.729 * pow(1.0 - exp(-0.030 * ws), 2.155);
}
/**
 * \brief A SimpleFuelBase made of a standard fuel type.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 * \tparam Bui0 Average Build-up Index for the fuel type [ST-X-3 table 7]
 * \tparam Cbh Crown base height (m) [ST-X-3 table 8]
 * \tparam Cfl Crown fuel load (kg/m^2) [ST-X-3 table 8]
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam InorganicPercent Inorganic percent of Duff layer (%) [Anderson table 1]
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
template <
  int A,
  int B,
  int C,
  int Bui0,
  int Cbh,
  int Cfl,
  int BulkDensity,
  int InorganicPercent,
  int DuffDepth>
class SimpleStandardFuel : public SimpleFuelBase<BulkDensity, InorganicPercent, DuffDepth>
{
public:
  /**
   * \brief Constructor
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel type can have a crown fire
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr SimpleStandardFuel(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const LogValue log_q,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  ) noexcept
    : SimpleFuelBase<BulkDensity, InorganicPercent, DuffDepth>(
        code,
        name,
        can_crown,
        duff_ffmc,
        duff_dmc
      ),
      log_q_(log_q)
  { }
  /**
   * \brief Constructor
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel type can have a crown fire
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff Type of duff near the surface and deeper underground
   */
  constexpr SimpleStandardFuel(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const LogValue log_q,
    const Duff* duff
  ) noexcept
    : SimpleStandardFuel(code, name, can_crown, log_q, duff, duff)
  { }
  SimpleStandardFuel(SimpleStandardFuel&& rhs) noexcept = delete;
  SimpleStandardFuel(const SimpleStandardFuel& rhs) noexcept = delete;
  SimpleStandardFuel& operator=(SimpleStandardFuel&& rhs) noexcept = delete;
  SimpleStandardFuel& operator=(const SimpleStandardFuel& rhs) = delete;
  /**
   * \brief Initial rate of spread (m/min) [ST-X-3 eq 26]
   * \param isi Initial Spread Index
   * \return Initial rate of spread (m/min) [ST-X-3 eq 26]
   */
  [[nodiscard]] MathSize rosBasic(const MathSize isi) const noexcept
  {
    return a() * pow(1.0 - exp(negB() * isi), c());
  }
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   */
  MathSize crownConsumption(const MathSize cfb) const noexcept override { return cfl() * cfb; }
  /**
   * \brief ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param mu Multiplier
   * \param rsf Slope-adjusted zero wind rate of spread (RSF) [ST-X-3 eq 40]
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize limitIsf(const MathSize mu, const MathSize rsf) const noexcept
  {
    return (1.0 / negB())
         * log(
             max(SLOPE_LIMIT_ISI, (rsf > 0.0) ? (1.0 - pow((rsf / (mu * a())), (1.0 / c()))) : 1.0)
         );
  }
  /**
   * \brief Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   * \param spread SpreadInfo to use in calculation
   * \return Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const noexcept override
  {
    return 0.001 * pow(cbh(), 1.5) * pow(460.0 + 25.9 * spread.foliarMoisture(), 1.5);
  }
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 79]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 79]
   */
  [[nodiscard]] MathSize lengthToBreadth(const MathSize ws) const noexcept override
  {
    return calculate_standard_length_to_breadth(ws);
  }
  /**
   * \brief Final rate of spread (m/min)
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \return Final rate of spread (m/min)
   */
  MathSize finalRos(const SpreadInfo&, MathSize, MathSize, const MathSize rss)
    const noexcept override
  {
    return rss;
  }
  /**
   * \brief BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   * \param bui Build-up Index
   * \return BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   */
  [[nodiscard]] MathSize buiEffect(const MathSize bui) const noexcept override
  {
    return (0 < bui) ? exp(50.0 * log_q_.value * ((1.0 / bui) - (1.0 / bui0()))) : 1.0;
  }

protected:
  ~SimpleStandardFuel() override = default;

public:
  /**
   * \brief Average Build-up Index for the fuel type [ST-X-3 table 7]
   * \return Average Build-up Index for the fuel type [ST-X-3 table 7]
   */
  [[nodiscard]] static constexpr MathSize bui0() noexcept { return Bui0; }
  /**
   * \brief Crown base height (m) [ST-X-3 table 8]
   * \return Crown base height (m) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cbh() const override { return Cbh; }
  /**
   * \brief Crown fuel load (kg/m^2) [ST-X-3 table 8]
   * \return Crown fuel load (kg/m^2) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cfl() const override { return Cfl / 100.0; }
  /**
   * \brief Rate of spread parameter a [ST-X-3 table 6]
   * \return Rate of spread parameter a [ST-X-3 table 6]
   */
  [[nodiscard]] static constexpr MathSize a() noexcept { return A; }
  /**
   * \brief Negative of rate of spread parameter b [ST-X-3 table 6]
   * \return Negative of rate of spread parameter b [ST-X-3 table 6]
   */
  [[nodiscard]] static constexpr MathSize negB() noexcept
  {
    // the only places this gets used it gets negated so just store it that way
    return -B / 10000.0;
  }
  /**
   * \brief Rate of spread parameter c [ST-X-3 table 6]
   * \return Rate of spread parameter c [ST-X-3 table 6]
   */
  [[nodiscard]] static constexpr MathSize c() noexcept { return C / 100.0; }

public:
  /**
   * \brief Crown fire spread rate (RSC) (m/min) [ST-X-3 eq 64]
   * \param isi Initial Spread Index
   * \param fmc Foliar Moisture Content
   * \return Crown fire spread rate (RSC) (m/min) [ST-X-3 eq 64]
   */
  [[nodiscard]] static constexpr MathSize crownRateOfSpread(
    const MathSize isi,
    const MathSize fmc
  ) noexcept
  {
    return calculate_standard_foliar_moisture_isi(isi)
         * calculate_standard_foliar_moisture_fmc(fmc);
  }
  [[nodiscard]] constexpr MathSize logQ() const noexcept { return log_q_.value; }

private:
  /**
   * \brief Log value of q [ST-X-3 table 7]
   */
  LogValue log_q_;
  static_assert(-negB() < 1);
  static_assert(c() < 10 && c() > 1);
};
}
#endif
