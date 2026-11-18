/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_STANDARD_FUEL_H
#define FS_STANDARD_FUEL_H
#include "../stdafx.h"
#include "../util/LogValue.h"
#include "FuelType.h"
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
[[nodiscard]] MathSize calculate_standard_foliar_moisture_fmc(const LookupValue& fmc) noexcept;
/**
 * \brief Crown fire spread rate (m/min) / Foliar Moisture Effect (RSC / (FME / FME_avg)) [ST-X-3 eq
 * 64]
 * \param isi Initial Spread Index
 * \return RSC / (FME / FME_avg) [ST-X-3 eq 64]
 */
[[nodiscard]] MathSize calculate_standard_foliar_moisture_isi(const LookupValue& isi) noexcept;
/**
 * \brief Length to Breadth ratio [ST-X-3 eq 79]
 * \param ws Wind Speed (km/h)
 * \return Length to Breadth ratio [ST-X-3 eq 79]
 */
[[nodiscard]] MathSize calculate_standard_length_to_breadth(const LookupValue& ws) noexcept;
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
  StandardFuel(
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
  ) noexcept;
  /**
   * \brief Constructor
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel type can have a crown fire
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff Type of duff near the surface and deeper underground
   */
  StandardFuel(
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
  ) noexcept;
  StandardFuel(StandardFuel&& rhs) noexcept = delete;
  StandardFuel(const StandardFuel& rhs) noexcept = delete;
  StandardFuel& operator=(StandardFuel&& rhs) noexcept = delete;
  StandardFuel& operator=(const StandardFuel& rhs) = delete;
  /**
   * \brief Initial rate of spread (m/min) [ST-X-3 eq 26]
   * \param isi Initial Spread Index
   * \return Initial rate of spread (m/min) [ST-X-3 eq 26]
   */
  [[nodiscard]] MathSize rosBasic(const RoundedValue& isi) const noexcept;
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   */
  MathSize crownConsumption(const RoundedValue& cfb) const noexcept override;
  /**
   * \brief ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param mu Multiplier
   * \param rsf Slope-adjusted zero wind rate of spread (RSF) [ST-X-3 eq 40]
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize limitIsf(const RoundedValue& mu, const RoundedValue& rsf) const noexcept;
  /**
   * \brief Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   * \param spread SpreadInfo to use in calculation
   * \return Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const noexcept override;
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 79]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 79]
   */
  [[nodiscard]] MathSize lengthToBreadth(const RoundedValue& ws) const noexcept override;
  /**
   * \brief Final rate of spread (m/min)
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \return Final rate of spread (m/min)
   */
  MathSize finalRos(
    const SpreadInfo&,
    const RoundedValue&,
    const RoundedValue&,
    const RoundedValue& rss
  ) const noexcept override;
  /**
   * \brief BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   * \param bui Build-up Index
   * \return BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   */
  [[nodiscard]] MathSize buiEffect(const RoundedValue& bui) const noexcept override;

protected:
  ~StandardFuel() override = default;

public:
  /**
   * \brief Average Build-up Index for the fuel type [ST-X-3 table 7]
   * \return Average Build-up Index for the fuel type [ST-X-3 table 7]
   */
  [[nodiscard]] MathSize bui0() const noexcept;
  /**
   * \brief Crown base height (m) [ST-X-3 table 8]
   * \return Crown base height (m) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cbh() const override;
  /**
   * \brief Crown fuel load (kg/m^2) [ST-X-3 table 8]
   * \return Crown fuel load (kg/m^2) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cfl() const override;
  /**
   * \brief Rate of spread parameter a [ST-X-3 table 6]
   * \return Rate of spread parameter a [ST-X-3 table 6]
   */
  [[nodiscard]] MathSize a() const noexcept;
  /**
   * \brief Negative of rate of spread parameter b [ST-X-3 table 6]
   * \return Negative of rate of spread parameter b [ST-X-3 table 6]
   */
  [[nodiscard]] MathSize negB() const noexcept;
  /**
   * \brief Rate of spread parameter c [ST-X-3 table 6]
   * \return Rate of spread parameter c [ST-X-3 table 6]
   */
  [[nodiscard]] MathSize c() const noexcept;

public:
  /**
   * \brief Crown fire spread rate (RSC) (m/min) [ST-X-3 eq 64]
   * \param isi Initial Spread Index
   * \param fmc Foliar Moisture Content
   * \return Crown fire spread rate (RSC) (m/min) [ST-X-3 eq 64]
   */
  [[nodiscard]] static MathSize crownRateOfSpread(
    const RoundedValue& isi,
    const RoundedValue& fmc
  ) noexcept;
  [[nodiscard]] MathSize logQ() const noexcept;

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
