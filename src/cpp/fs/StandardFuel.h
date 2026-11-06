/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_STANDARDFUEL
#define FS_STANDARDFUEL
#include "stdafx.h"
#include "FuelType.h"
#include "LogValue.h"
#include "unstable.h"
namespace fs::fuel
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
[[nodiscard]] MathSize calculate_standard_foliar_moisture_fmc(const MathSize f) noexcept;
/**
 * \brief Crown fire spread rate (m/min) / Foliar Moisture Effect (RSC / (FME / FME_avg)) [ST-X-3 eq
 * 64]
 * \param isi Initial Spread Index
 * \return RSC / (FME / FME_avg) [ST-X-3 eq 64]
 */
[[nodiscard]] MathSize calculate_standard_foliar_moisture_isi(const MathSize i) noexcept;
/**
 * \brief Length to Breadth ratio [ST-X-3 eq 79]
 * \param ws Wind Speed (km/h)
 * \return Length to Breadth ratio [ST-X-3 eq 79]
 */
[[nodiscard]] MathSize calculate_standard_length_to_breadth(const MathSize w) noexcept;
/**
 * \brief A FuelBase made of a standard fuel type.
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
class StandardFuel : public FuelBase
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
  constexpr StandardFuel(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const LogValue log_q,
    const MathSize a,
    const MathSize b,
    const MathSize c,
    const MathSize bui0,
    const MathSize cbh,
    const MathSize cfl,
    const MathSize bulk_density,
    const MathSize inorganic_percent,
    const MathSize duff_depth,
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
  /**
   * \brief Constructor
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel type can have a crown fire
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff Type of duff near the surface and deeper underground
   */
  constexpr StandardFuel(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const LogValue log_q,
    const MathSize a,
    const MathSize b,
    const MathSize c,
    const MathSize bui0,
    const MathSize cbh,
    const MathSize cfl,
    const MathSize bulk_density,
    const MathSize inorganic_percent,
    const MathSize duff_depth,
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
  StandardFuel(StandardFuel&& rhs) noexcept = delete;
  StandardFuel(const StandardFuel& rhs) noexcept = delete;
  StandardFuel& operator=(StandardFuel&& rhs) noexcept = delete;
  StandardFuel& operator=(const StandardFuel& rhs) = delete;
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
         * log(max(
           SLOPE_LIMIT_ISI,
           static_cast<MathSize>(
             (rsf > static_cast<MathSize>(0.0)) ? (1.0 - pow((rsf / (mu * a())), (1.0 / c()))) : 1.0
           )
         ));
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
  ~StandardFuel() override = default;

public:
  /**
   * \brief Average Build-up Index for the fuel type [ST-X-3 table 7]
   * \return Average Build-up Index for the fuel type [ST-X-3 table 7]
   */
  [[nodiscard]] constexpr MathSize bui0() const noexcept { return bui0_; }
  /**
   * \brief Crown base height (m) [ST-X-3 table 8]
   * \return Crown base height (m) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cbh() const override { return cbh_; }
  /**
   * \brief Crown fuel load (kg/m^2) [ST-X-3 table 8]
   * \return Crown fuel load (kg/m^2) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cfl() const override { return cfl_ / 100.0; }
  /**
   * \brief Rate of spread parameter a [ST-X-3 table 6]
   * \return Rate of spread parameter a [ST-X-3 table 6]
   */
  [[nodiscard]] constexpr MathSize a() const noexcept { return a_; }
  /**
   * \brief Negative of rate of spread parameter b [ST-X-3 table 6]
   * \return Negative of rate of spread parameter b [ST-X-3 table 6]
   */
  [[nodiscard]] constexpr MathSize negB() const noexcept
  {
    // the only places this gets used it gets negated so just store it that way
    return -b_ / 10000.0;
  }
  /**
   * \brief Rate of spread parameter c [ST-X-3 table 6]
   * \return Rate of spread parameter c [ST-X-3 table 6]
   */
  [[nodiscard]] constexpr MathSize c() const noexcept { return c_ / 100.0; }

public:
  /**
   * \brief Crown fire spread rate (RSC) (m/min) [ST-X-3 eq 64]
   * \param isi Initial Spread Index
   * \param fmc Foliar Moisture Content
   * \return Crown fire spread rate (RSC) (m/min) [ST-X-3 eq 64]
   */
  [[nodiscard]] static MathSize crownRateOfSpread(const MathSize isi, const MathSize fmc) noexcept
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
  MathSize a_{};
  MathSize b_{};
  MathSize c_{};
  MathSize bui0_{};
  MathSize cbh_{};
  MathSize cfl_{};
};
}
#endif
