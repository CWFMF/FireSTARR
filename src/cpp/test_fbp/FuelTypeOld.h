/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FUEL_TYPE_OLD_H
#define FS_FUEL_TYPE_OLD_H
#include "../fs/stdafx.h"
#include "../fs/fuel/Duff.h"
#include "../fs/fuel/FuelType.h"
#include "../fs/fuel/Survival.h"
namespace fs::fuelold
{
using fs::duff::Duff;
using fs::fuel::FuelType;
using fs::fuel::InvalidFuel;
using fs::fuel::simplify_fuel_name;
using fs::survival::probability_peat;
using fs::survival::survival_probability;
/**
 * \brief Base class for all FuelTypes.
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam InorganicPercent Inorganic percent of Duff layer (%) [Anderson table 1]
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
template <int BulkDensity, int InorganicPercent, int DuffDepth>
class FuelOldBase : public FuelType
{
public:
  ~FuelOldBase() override = default;
  /**
   * \brief Constructor
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel type can have a crown fire
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr FuelOldBase(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  )
    : FuelType(code, name, can_crown), duff_ffmc_(duff_ffmc), duff_dmc_(duff_dmc)
  { }
  FuelOldBase(FuelOldBase&& rhs) noexcept = delete;
  FuelOldBase(const FuelOldBase& rhs) = delete;
  FuelOldBase& operator=(FuelOldBase&& rhs) noexcept = delete;
  FuelOldBase& operator=(const FuelOldBase& rhs) = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override { return true; }
  /**
   * \brief Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \param rso Critical surface fire spread rate (RSO) [ST-X-3 eq 57]
   * \return Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   */
  [[nodiscard]] MathSize crownFractionBurned(const RoundedValue& rss, const RoundedValue& rso)
    const noexcept override
  {
    // can't burn crown if it doesn't exist
    return cfl() > 0 ? max(0.0, 1.0 - exp(-0.230 * (rss - rso))) : 0.0;
  }
  /**
   * \brief Calculate probability of burning [Anderson eq 1]
   * \param mc_fraction moisture content (% / 100)
   * \return Calculate probability of burning [Anderson eq 1]
   */
  [[nodiscard]] ThresholdSize probabilityPeat(const RoundedValue& mc_fraction
  ) const noexcept override
  {
    return probability_peat(bulkDensity(), inorganicPercent(), mc_fraction);
  }
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept override
  {
    return survival_probability(
      bulkDensity(), inorganicPercent(), *duffFfmcType(), *duffDmcType(), dmcRatio(), wx
    );
  }
  /**
   * \brief Duff Bulk Density (kg/m^3) [Anderson table 1]
   * \return Duff Bulk Density (kg/m^3) [Anderson table 1]
   */
  [[nodiscard]] static constexpr MathSize bulkDensity() { return BulkDensity / 1000.0; }
  /**
   * \brief Inorganic Percent (% / 100) [Anderson table 1]
   * \return Inorganic Percent (% / 100) [Anderson table 1]
   */
  [[nodiscard]] static constexpr MathSize inorganicPercent() { return InorganicPercent / 100.0; }
  /**
   * \brief DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   * \return DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   */
  [[nodiscard]] static constexpr MathSize duffDepth() { return DuffDepth / 10.0; }
  /**
   * \brief Type of duff deeper underground
   * \return Type of duff deeper underground
   */
  [[nodiscard]] constexpr const Duff* duffDmcType() const { return duff_dmc_; }
  /**
   * \brief Type of duff near the surface
   * \return Type of duff near the surface
   */
  [[nodiscard]] constexpr const Duff* duffFfmcType() const { return duff_ffmc_; }
  /**
   * \brief What fraction of the duff layer should use FFMC to determine moisture
   * \return What fraction of the duff layer should use FFMC to determine moisture
   */
  [[nodiscard]] static constexpr MathSize ffmcRatio() { return 1 - dmcRatio(); }
  /**
   * \brief What fraction of the duff layer should use DMC to determine moisture
   * \return What fraction of the duff layer should use DMC to determine moisture
   */
  [[nodiscard]] static constexpr MathSize dmcRatio()
  {
    return (duffDepth() - fs::survival::DUFF_FFMC_DEPTH) / duffDepth();
  }
  [[nodiscard]] const FuelType* summer() const noexcept override { return this; }
  [[nodiscard]] const FuelType* spring() const noexcept override { return this; }

private:
  /**
   * \brief Type of duff near the surface
   */
  const Duff* duff_ffmc_{nullptr};
  /**
   * \brief Type of duff deeper underground
   */
  const Duff* duff_dmc_{nullptr};
};
}
#endif
