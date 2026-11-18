/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FUEL_TYPE_H
#define FS_FUEL_TYPE_H
#include "../stdafx.h"
#include "../spread/FireSpread.h"
#include "../wx/FwiWeather.h"
#include "Duff.h"
namespace fs::fuel
{
using duff::Duff;
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
[[nodiscard]] MathSize fire_intensity(const RoundedValue& fc, const RoundedValue& ros);
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
  [[nodiscard]] static FuelCodeSize safeCode(const FuelType* fuel);
  /**
   * \brief Convert FuelType to its name, or 0 if nullptr
   * \param fuel FuelType to convert
   * \return Name for FuelType, or "NULL" if nullptr
   */
  [[nodiscard]] static const char* safeName(const FuelType* fuel);
  /**
   * \brief Critical rate of spread (m/min)
   * \param sfc Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   * \param csi Critical Surface Fire Intensity (CSI) (kW/m) [ST-X-3 eq 56]
   * \return Critical rate of spread (m/min)
   */
  [[nodiscard]] static MathSize criticalRos(const RoundedValue& sfc, const RoundedIntensity& csi);
  /**
   * \brief Whether or not this is a crown fire
   * \param csi Critical Surface Fire Intensity (CSI) (kW/m) [ST-X-3 eq 56]
   * \param sfi Surface Fire Intensity (kW/m)
   * \return Whether or not this is a crown fire
   */
  [[nodiscard]] static bool isCrown(const RoundedIntensity& csi, const RoundedIntensity& sfi);
  /**
   * \brief Crown fuel load (kg/m^2) [ST-X-3 table 8]
   * \return Crown fuel load (kg/m^2) [ST-X-3 table 8]
   */
  [[nodiscard]] virtual MathSize cfl() const = 0;
  virtual ~FuelType() noexcept = default;
  FuelType() noexcept = delete;
  /**
   * \brief Fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel can have a crown fire
   */
  FuelType(const FuelCodeSize& code, const char* name, const bool can_crown) noexcept;
  FuelType(FuelType&& rhs) noexcept = delete;
  FuelType(const FuelType& rhs) noexcept = delete;
  FuelType& operator=(FuelType&& rhs) noexcept = delete;
  FuelType& operator=(const FuelType& rhs) noexcept = delete;
  /**
   * \brief Whether or not this fuel can have a crown fire
   * \return Whether or not this fuel can have a crown fire
   */
  [[nodiscard]] bool canCrown() const;
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] virtual MathSize grass_curing(const int, const FwiWeather&) const;
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
  [[nodiscard]] virtual MathSize crownFractionBurned(
    const RoundedValue& rss,
    const RoundedValue& rso
  ) const noexcept = 0;
  /**
   * \brief Calculate probability of burning [Anderson eq 1]
   * \param mc_fraction moisture content (% / 100)
   * \return Calculate probability of burning [Anderson eq 1]
   */
  [[nodiscard]] virtual MathSize probabilityPeat(const RoundedValue& mc_fraction
  ) const noexcept = 0;
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
  [[nodiscard]] virtual MathSize buiEffect(const RoundedValue& bui) const = 0;
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   */
  [[nodiscard]] virtual MathSize crownConsumption(const RoundedValue& cfb) const = 0;
  /**
   * \brief Calculate rate of spread (m/min)
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use for calculation
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \return Rate of spread (m/min)
   */
  [[nodiscard]] virtual MathSize calculateRos(int nd, const FwiWeather& wx, const RoundedValue& isi)
    const = 0;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41/42]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41/42]
   */
  [[nodiscard]] virtual MathSize calculateIsf(const SpreadInfo& spread, const RoundedValue& isi)
    const = 0;
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
  [[nodiscard]] virtual MathSize lengthToBreadth(const RoundedValue& ws) const = 0;
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
    const RoundedValue& isi,
    const RoundedValue& cfb,
    const RoundedValue& rss
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
  [[nodiscard]] const char* name() const;
  /**
   * \brief Code for this fuel type
   * \return Code for this fuel type
   */
  [[nodiscard]] FuelCodeSize code() const;
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
  const bool can_crown_{false};
  /**
   * \brief Code to identify fuel with
   */
  FuelCodeSize code_{0};
};
/**
 * \brief Placeholder fuel that throws exceptions if it ever gets used.
 */
class InvalidFuel final : public FuelType
{
public:
  InvalidFuel() noexcept = delete;
  /**
   * \brief Placeholder fuel that throws exceptions if it ever gets used.
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  InvalidFuel(const FuelCodeSize& code, const char* name) noexcept;
  ~InvalidFuel() override = default;
  InvalidFuel(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel(InvalidFuel&& rhs) noexcept = delete;
  InvalidFuel& operator=(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel& operator=(InvalidFuel&& rhs) noexcept = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override;
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
  [[nodiscard]] MathSize buiEffect(const RoundedValue&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize crownConsumption(const RoundedValue&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize calculateRos(int, const FwiWeather&, const RoundedValue&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo&, const RoundedValue&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize lengthToBreadth(const RoundedValue&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize
  finalRos(const SpreadInfo&, const RoundedValue&, const RoundedValue&, const RoundedValue&)
    const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize crownFractionBurned(const RoundedValue&, const RoundedValue&)
    const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize probabilityPeat(const RoundedValue&) const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize survivalProbability(const FwiWeather&) const noexcept override;
  [[nodiscard]] const FuelType* summer() const noexcept override;
  [[nodiscard]] const FuelType* spring() const noexcept override;
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
  FuelBase(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const MathConstantSize& bulk_density,
    const MathConstantSize& inorganic_percent,
    const MathConstantSize& duff_depth,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  );
  FuelBase(FuelBase&& rhs) noexcept = delete;
  FuelBase(const FuelBase& rhs) = delete;
  FuelBase& operator=(FuelBase&& rhs) noexcept = delete;
  FuelBase& operator=(const FuelBase& rhs) = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override;
  /**
   * \brief Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \param rso Critical surface fire spread rate (RSO) [ST-X-3 eq 57]
   * \return Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   */
  [[nodiscard]] MathSize crownFractionBurned(const RoundedValue& rss, const RoundedValue& rso)
    const noexcept override;
  /**
   * \brief Calculate probability of burning [Anderson eq 1]
   * \param mc_fraction moisture content (% / 100)
   * \return Calculate probability of burning [Anderson eq 1]
   */
  [[nodiscard]] ThresholdSize probabilityPeat(const RoundedValue& mc_fraction
  ) const noexcept override;
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept override;
  /**
   * \brief Duff Bulk Density (kg/m^3) [Anderson table 1]
   * \return Duff Bulk Density (kg/m^3) [Anderson table 1]
   */
  [[nodiscard]] MathSize bulkDensity() const;
  /**
   * \brief Inorganic Percent (% / 100) [Anderson table 1]
   * \return Inorganic Percent (% / 100) [Anderson table 1]
   */
  [[nodiscard]] MathSize inorganicPercent() const;
  /**
   * \brief DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   * \return DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   */
  [[nodiscard]] MathSize duffDepth() const;
  /**
   * \brief Type of duff deeper underground
   * \return Type of duff deeper underground
   */
  [[nodiscard]] const Duff* duffDmcType() const;
  /**
   * \brief Type of duff near the surface
   * \return Type of duff near the surface
   */
  [[nodiscard]] const Duff* duffFfmcType() const;
  /**
   * \brief What fraction of the duff layer should use FFMC to determine moisture
   * \return What fraction of the duff layer should use FFMC to determine moisture
   */
  [[nodiscard]] MathSize ffmcRatio() const;
  /**
   * \brief What fraction of the duff layer should use DMC to determine moisture
   * \return What fraction of the duff layer should use DMC to determine moisture
   */
  [[nodiscard]] MathSize dmcRatio() const;
  [[nodiscard]] const FuelType* summer() const noexcept override;
  [[nodiscard]] const FuelType* spring() const noexcept override;

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
