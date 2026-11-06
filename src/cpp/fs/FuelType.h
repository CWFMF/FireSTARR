/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FUELTYPE_H
#define FS_FUELTYPE_H
#include "stdafx.h"
#include "Duff.h"
#include "FireSpread.h"
#include "FWI.h"
#include "Survival.h"
#include "unstable.h"
namespace fs::fuel
{
using duff::Duff;
using namespace fs::survival;
string simplify_fuel_name(const string_view fuel);
// References
// Forestry Canada
// Development and Structure of the Canadian Forest Fire Behaviour Prediction System (ST-X-3)
// https://cfs.nrcan.gc.ca/pubwarehouse/pdfs/10068.pdf
//
// Wotton, B.M., Alexander, M.E., Taylor, S.W.
// Updates and revision to the 1992 Canadian Forest Fire Behavior Prediction System (GLC-X-10)
// https://cfs.nrcan.gc.ca/pubwarehouse/pdfs/31414.pdf
//
// Anderson, Kerry
// Incorporating Smoldering Into Fire Growth Modelling
// https://www.cfs.nrcan.gc.ca/pubwarehouse/pdfs/19950.pdf
//
// default grass fuel load (kg/m^2)
static constexpr MathSize DEFAULT_GRASS_FUEL_LOAD = 0.35;
/**
 * \brief Fire Intensity (kW/m) [ST-X-3 eq 69]
 * \param fc Fuel consumption (kg/m^2)
 * \param ros Rate of spread (m/min)
 * \return Fire Intensity (kW/m) [ST-X-3 eq 69]
 */
[[nodiscard]] constexpr MathSize fire_intensity(const MathSize fc, const MathSize ros)
{
  return 300.0 * fc * ros;
}
/**
 * \brief An FBP fuel type.
 */
class FuelType
{
public:
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] virtual bool isValid() const = 0;
  /**
   * \brief Convert FuelType to its code, or 0 if nullptr
   * \param fuel FuelType to convert
   * \return Code for FuelType, or 0 if nullptr
   */
  [[nodiscard]] static constexpr FuelCodeSize safeCode(const FuelType* fuel)
  {
    return nullptr == fuel ? static_cast<FuelCodeSize>(INVALID_FUEL_CODE) : fuel->code();
  }
  /**
   * \brief Convert FuelType to its name, or 0 if nullptr
   * \param fuel FuelType to convert
   * \return Name for FuelType, or "NULL" if nullptr
   */
  [[nodiscard]] static constexpr const char* safeName(const FuelType* fuel)
  {
    return nullptr == fuel ? "NULL" : fuel->name();
  }
  /**
   * \brief Critical rate of spread (m/min)
   * \param sfc Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   * \param csi Critical Surface Fire Intensity (CSI) (kW/m) [ST-X-3 eq 56]
   * \return Critical rate of spread (m/min)
   */
  [[nodiscard]] static constexpr MathSize criticalRos(const MathSize sfc, const MathSize csi)
  {
    return sfc > 0 ? csi / (300.0 * sfc) : 0.0;
  }
  /**
   * \brief Whether or not this is a crown fire
   * \param csi Critical Surface Fire Intensity (CSI) (kW/m) [ST-X-3 eq 56]
   * \param sfi Surface Fire Intensity (kW/m)
   * \return Whether or not this is a crown fire
   */
  [[nodiscard]] static constexpr bool isCrown(const MathSize csi, const MathSize sfi)
  {
    return sfi > csi;
  }
  /**
   * \brief Crown fuel load (kg/m^2) [ST-X-3 table 8]
   * \return Crown fuel load (kg/m^2) [ST-X-3 table 8]
   */
  [[nodiscard]] virtual MathSize cfl() const = 0;
  virtual ~FuelType() noexcept = default;
  /**
   * \brief Fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel can have a crown fire
   */
  constexpr FuelType(const FuelCodeSize& code, const char* name, const bool can_crown) noexcept
    : name_(name), can_crown_(can_crown), code_(code)
  { }
  FuelType(FuelType&& rhs) noexcept = delete;
  FuelType(const FuelType& rhs) noexcept = delete;
  FuelType& operator=(FuelType&& rhs) noexcept = delete;
  FuelType& operator=(const FuelType& rhs) noexcept = delete;
  /**
   * \brief Whether or not this fuel can have a crown fire
   * \return Whether or not this fuel can have a crown fire
   */
  [[nodiscard]] constexpr bool canCrown() const { return can_crown_; }
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] virtual MathSize grass_curing(const int, const FwiWeather&) const
  {
    // NOTE: grass overrides this but everything else doesn't have curing
    return INVALID_CURING;
  }
  /**
   * \brief Crown base height (m) [ST-X-3 table 8]
   * \return Crown base height (m) [ST-X-3 table 8]
   */
  [[nodiscard]] virtual MathSize cbh() const = 0;
  /**
   * \brief Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \param rso Critical surface fire spread rate (RSO) [ST-X-3 eq 57]
   * \return Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   */
  [[nodiscard]] virtual MathSize crownFractionBurned(MathSize rss, MathSize rso) const noexcept = 0;
  /**
   * \brief Calculate probability of burning [Anderson eq 1]
   * \param mc_fraction moisture content (% / 100)
   * \return Calculate probability of burning [Anderson eq 1]
   */
  [[nodiscard]] virtual MathSize probabilityPeat(MathSize mc_fraction) const noexcept = 0;
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] virtual ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept = 0;
  /**
   * \brief BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   * \param bui Build-up Index
   * \return BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   */
  [[nodiscard]] virtual MathSize buiEffect(MathSize bui) const = 0;
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   */
  [[nodiscard]] virtual MathSize crownConsumption(MathSize cfb) const = 0;
  /**
   * \brief Calculate rate of spread (m/min)
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use for calculation
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \return Rate of spread (m/min)
   */
  [[nodiscard]] virtual MathSize calculateRos(int nd, const FwiWeather& wx, MathSize isi) const = 0;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41/42]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41/42]
   */
  [[nodiscard]] virtual MathSize calculateIsf(const SpreadInfo& spread, MathSize isi) const = 0;
  /**
   * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   * \param spread SpreadInfo to use
   * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   */
  [[nodiscard]] virtual MathSize surfaceFuelConsumption(const SpreadInfo& spread) const = 0;
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 79]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 79]
   */
  [[nodiscard]] virtual MathSize lengthToBreadth(MathSize ws) const = 0;
  /**
   * \brief Final rate of spread (m/min)
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \return Final rate of spread (m/min)
   */
  [[nodiscard]] virtual MathSize finalRos(
    const SpreadInfo& spread,
    MathSize isi,
    MathSize cfb,
    MathSize rss
  ) const = 0;
  /**
   * \brief Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   * \param spread SpreadInfo to use in calculation
   * \return Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   */
  [[nodiscard]] virtual MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const = 0;
  /**
   * \brief Name of the fuel
   * \return Name of the fuel
   */
  [[nodiscard]] constexpr const char* name() const { return name_; }
  /**
   * \brief Code for this fuel type
   * \return Code for this fuel type
   */
  [[nodiscard]] constexpr FuelCodeSize code() const { return code_; }
  [[nodiscard]] virtual const FuelType* summer() const noexcept = 0;
  [[nodiscard]] virtual const FuelType* spring() const noexcept = 0;
  [[nodiscard]] const FuelType* find_fuel_by_season(const int nd) const noexcept;

private:
  /**
   * \brief Name of the fuel
   */
  const char* name_{nullptr};
  /**
   * \brief Whether or not this fuel can have a crown fire
   */
  const bool can_crown_;
  /**
   * \brief Code to identify fuel with
   */
  FuelCodeSize code_;
};
/**
 * \brief Placeholder fuel that throws exceptions if it ever gets used.
 */
class InvalidFuel final : public FuelType
{
public:
  InvalidFuel() noexcept : InvalidFuel(0, nullptr) { }
  /**
   * \brief Placeholder fuel that throws exceptions if it ever gets used.
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr InvalidFuel(const FuelCodeSize& code, const char* name) noexcept
    : FuelType(code, name, false)
  { }
  ~InvalidFuel() override = default;
  InvalidFuel(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel(InvalidFuel&& rhs) noexcept = delete;
  InvalidFuel& operator=(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel& operator=(InvalidFuel&& rhs) noexcept = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override { return false; }
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const FwiWeather& wx) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize cbh() const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize cfl() const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize buiEffect(MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize crownConsumption(MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize calculateRos(int, const FwiWeather&, MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo&, MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize lengthToBreadth(MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize finalRos(const SpreadInfo&, MathSize, MathSize, MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize crownFractionBurned(MathSize, MathSize) const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize probabilityPeat(MathSize) const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize survivalProbability(const FwiWeather&) const noexcept override;
  [[nodiscard]] const FuelType* summer() const noexcept override { return this; }
  [[nodiscard]] const FuelType* spring() const noexcept override { return this; }
};
/**
 * \brief Base class for all FuelTypes.
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam InorganicPercent Inorganic percent of Duff layer (%) [Anderson table 1]
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
class FuelBase : public FuelType
{
private:
  MathSize bulk_density_{};
  MathSize inorganic_percent_{};
  MathSize duff_depth_{};

public:
  ~FuelBase() override = default;
  /**
   * \brief Constructor
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel type can have a crown fire
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr FuelBase(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const MathSize bulk_density,
    const MathSize inorganic_percent,
    const MathSize duff_depth,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  )
    : FuelType(code, name, can_crown), bulk_density_(bulk_density),
      inorganic_percent_(inorganic_percent), duff_depth_(duff_depth), duff_ffmc_(duff_ffmc),
      duff_dmc_(duff_dmc)
  { }
  FuelBase(FuelBase&& rhs) noexcept = delete;
  FuelBase(const FuelBase& rhs) = delete;
  FuelBase& operator=(FuelBase&& rhs) noexcept = delete;
  FuelBase& operator=(const FuelBase& rhs) = delete;
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
  [[nodiscard]] MathSize crownFractionBurned(const MathSize rss, const MathSize rso)
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
  [[nodiscard]] ThresholdSize probabilityPeat(const MathSize mc_fraction) const noexcept override
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
  [[nodiscard]] constexpr MathSize bulkDensity() const
  {
    return bulk_density_;
    // BulkDensity / 1000.0;
  }
  /**
   * \brief Inorganic Percent (% / 100) [Anderson table 1]
   * \return Inorganic Percent (% / 100) [Anderson table 1]
   */
  [[nodiscard]] constexpr MathSize inorganicPercent() const
  {
    return inorganic_percent_;
    // InorganicPercent / 100.0;
  }
  /**
   * \brief DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   * \return DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   */
  [[nodiscard]] constexpr MathSize duffDepth() const
  {
    return duff_depth_;
    // DuffDepth / 10.0;
  }
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
  [[nodiscard]] constexpr MathSize ffmcRatio() const { return 1 - dmcRatio(); }
  /**
   * \brief What fraction of the duff layer should use DMC to determine moisture
   * \return What fraction of the duff layer should use DMC to determine moisture
   */
  [[nodiscard]] constexpr MathSize dmcRatio() const
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
