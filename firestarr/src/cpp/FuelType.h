/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "Duff.h"
#include "FWI.h"
namespace tbd
{
namespace sim
{
class SpreadInfo;
}
namespace data
{
class LogValue;
}
using sim::SpreadInfo;
using data::LogValue;
namespace fuel
{
constexpr FuelCodeSize INVALID_FUEL_CODE = 0;
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
// amount of duff to apply ffmc moisture to (cm) (1.2 cm is from Kerry's paper)
static constexpr MathSize DUFF_FFMC_DEPTH = 1.2;
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
  constexpr FuelType(const FuelCodeSize& code,
                     const char* name,
                     const bool can_crown) noexcept
    : name_(name), can_crown_(can_crown), code_(code)
  {
  }
  FuelType(FuelType&& rhs) noexcept = delete;
  FuelType(const FuelType& rhs) noexcept = delete;
  FuelType& operator=(FuelType&& rhs) noexcept = delete;
  FuelType& operator=(const FuelType& rhs) noexcept = delete;
  /**
   * \brief Whether or not this fuel can have a crown fire
   * \return Whether or not this fuel can have a crown fire
   */
  [[nodiscard]] constexpr bool canCrown() const
  {
    return can_crown_;
  }
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] virtual MathSize grass_curing(const int, const wx::FwiWeather&) const
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
   * \brief Survival probability calculated using probability of ony survival based on multiple formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] virtual ThresholdSize survivalProbability(
    const wx::FwiWeather& wx) const noexcept = 0;
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
  [[nodiscard]] virtual MathSize calculateRos(int nd,
                                              const wx::FwiWeather& wx,
                                              MathSize isi) const = 0;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41/42]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41/42]
   */
  [[nodiscard]] virtual MathSize calculateIsf(const SpreadInfo& spread,
                                              MathSize isi) const = 0;
  /**
   * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   * \param spread SpreadInfo to use
   * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   */
  [[nodiscard]] virtual MathSize surfaceFuelConsumption(
    const SpreadInfo& spread) const = 0;
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
  [[nodiscard]] virtual MathSize finalRos(const SpreadInfo& spread,
                                          MathSize isi,
                                          MathSize cfb,
                                          MathSize rss) const = 0;
  /**
   * \brief Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   * \param spread SpreadInfo to use in calculation
   * \return Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   */
  [[nodiscard]] virtual MathSize criticalSurfaceIntensity(
    const SpreadInfo& spread) const = 0;
  /**
   * \brief Name of the fuel
   * \return Name of the fuel
   */
  [[nodiscard]] constexpr const char* name() const
  {
    return name_;
  }
  /**
   * \brief Code for this fuel type
   * \return Code for this fuel type
   */
  [[nodiscard]] constexpr FuelCodeSize code() const
  {
    return code_;
  }
private:
  /**
   * \brief Name of the fuel
   */
  const char* name_;
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
 * \brief Base class for all FuelTypes.
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam InorganicPercent Inorganic percent of Duff layer (%) [Anderson table 1]
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
template <int BulkDensity, int InorganicPercent, int DuffDepth>
class FuelBase
  : public FuelType
{
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
  constexpr FuelBase(const FuelCodeSize& code,
                     const char* name,
                     const bool can_crown,
                     const Duff* duff_ffmc,
                     const Duff* duff_dmc)
    : FuelType(code, name, can_crown),
      duff_ffmc_(duff_ffmc),
      duff_dmc_(duff_dmc)
  {
  }
  FuelBase(FuelBase&& rhs) noexcept = delete;
  FuelBase(const FuelBase& rhs) = delete;
  FuelBase& operator=(FuelBase&& rhs) noexcept = delete;
  FuelBase& operator=(const FuelBase& rhs) = delete;
  /**
   * \brief Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \param rso Critical surface fire spread rate (RSO) [ST-X-3 eq 57]
   * \return Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   */
  [[nodiscard]] MathSize crownFractionBurned(const MathSize rss,
                                             const MathSize rso) const noexcept override
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
    // Anderson table 1
    constexpr auto pb = bulkDensity();
    // Anderson table 1
    constexpr auto fi = inorganicPercent();
    constexpr auto pi = fi * pb;
    // Inorganic ratio
    constexpr auto ri = fi / (1 - fi);
    constexpr auto const_part = -19.329 + 1.7170 * ri + 23.059 * pi;
    // Anderson eq 1
    return 1 / (1 + exp(17.047 * mc_fraction / (1 - fi) + const_part));
  }
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] ThresholdSize survivalProbability(const wx::FwiWeather& wx) const noexcept
    override
  {
    // divide by 100 since we need moisture ratio
    //    IFERROR(((1 / (1 + EXP($G$43 + $I$43 *
    //            (Q$44 * $O$43 + $N$43)))) -
    //            (1 / (1 + EXP($G$43 + $I$43 * (2.5 * $O$43 + $N$43)))))
    //            / (1 / (1 + EXP($G$43 + $I$43 * $N$43))), 0)
    // HACK: use same constants for all fuels because they seem to work nicer than
    // using the ratios, but they change anyway because of the other fuel attributes
    static const auto WFfmc = 0.25;
    static const auto WDmc = 1.0;
    static const auto RatioHartford = 0.5;
    static const auto RatioFrandsen = 1.0 - RatioHartford;
    static const auto RatioAspen = 0.5;
    static const auto RatioFuel = 1.0 - RatioAspen;
    const auto mc_ffmc = wx.mcFfmc() * WFfmc + WDmc;
    static const auto McFfmcSaturated = 2.5 * WFfmc + WDmc;
    static const auto McDmc = WDmc;
    const auto prob_ffmc_peat = probabilityPeat(mc_ffmc);
    const auto prob_ffmc_peat_saturated = probabilityPeat(McFfmcSaturated);
    const auto prob_ffmc_peat_zero = probabilityPeat(McDmc);
    const auto prob_ffmc_peat_weighted = (prob_ffmc_peat - prob_ffmc_peat_saturated) / prob_ffmc_peat_zero;
    const auto prob_ffmc = duffFfmcType()->probabilityOfSurvival(mc_ffmc * 100);
    const auto prob_ffmc_saturated = duffFfmcType()->probabilityOfSurvival(
      McFfmcSaturated * 100);
    const auto prob_ffmc_zero = duffFfmcType()->probabilityOfSurvival(McDmc);
    const auto prob_ffmc_weighted = (prob_ffmc - prob_ffmc_saturated) / prob_ffmc_zero;
    const auto term_otway = exp(-3.11 + 0.12 * wx.dmc().asValue());
    const auto prob_otway = term_otway / (1 + term_otway);
    const auto mc_pct = wx.mcDmcPct() * dmcRatio() + wx.mcFfmcPct() * ffmcRatio();
    const auto prob_weight_ffmc = duffFfmcType()->probabilityOfSurvival(mc_pct);
    const auto prob_weight_ffmc_peat = probabilityPeat(mc_pct / 100);
    const auto prob_weight_dmc = duffDmcType()->probabilityOfSurvival(wx.mcDmcPct());
    const auto prob_weight_dmc_peat = probabilityPeat(wx.mcDmc());
    // chance of survival is 1 - chance of it not surviving in every fuel
    const auto tot_prob = 1 - (1 - prob_ffmc_peat_weighted) * (1 - prob_ffmc_weighted) * ((1 - prob_otway) * RatioAspen + ((1 - prob_weight_ffmc_peat) * RatioHartford + (1 - prob_weight_ffmc) * RatioFrandsen) * ((1 - prob_weight_dmc_peat) * RatioHartford + (1 - prob_weight_dmc) * RatioFrandsen) * RatioFuel);
    return tot_prob;
  }
  /**
   * \brief Duff Bulk Density (kg/m^3) [Anderson table 1]
   * \return Duff Bulk Density (kg/m^3) [Anderson table 1]
   */
  [[nodiscard]] static constexpr MathSize bulkDensity()
  {
    return BulkDensity / 1000.0;
  }
  /**
   * \brief Inorganic Percent (% / 100) [Anderson table 1]
   * \return Inorganic Percent (% / 100) [Anderson table 1]
   */
  [[nodiscard]] static constexpr MathSize inorganicPercent()
  {
    return InorganicPercent / 100.0;
  }
  /**
   * \brief DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   * \return DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   */
  [[nodiscard]] static constexpr MathSize duffDepth()
  {
    return DuffDepth / 10.0;
  }
  /**
   * \brief Type of duff deeper underground
   * \return Type of duff deeper underground
   */
  [[nodiscard]] constexpr const Duff* duffDmcType() const
  {
    return duff_dmc_;
  }
  /**
   * \brief Type of duff near the surface
   * \return Type of duff near the surface
   */
  [[nodiscard]] constexpr const Duff* duffFfmcType() const
  {
    return duff_ffmc_;
  }
  /**
   * \brief What fraction of the duff layer should use FFMC to determine moisture
   * \return What fraction of the duff layer should use FFMC to determine moisture
   */
  [[nodiscard]] static constexpr MathSize ffmcRatio()
  {
    return 1 - dmcRatio();
  }
  /**
   * \brief What fraction of the duff layer should use DMC to determine moisture
   * \return What fraction of the duff layer should use DMC to determine moisture
   */
  [[nodiscard]] static constexpr MathSize dmcRatio()
  {
    return (duffDepth() - DUFF_FFMC_DEPTH) / duffDepth();
  }
private:
  /**
   * \brief Type of duff near the surface
   */
  const Duff* duff_ffmc_;
  /**
   * \brief Type of duff deeper underground
   */
  const Duff* duff_dmc_;
};
/**
 * \brief Placeholder fuel that throws exceptions if it ever gets used.
 */
class InvalidFuel final
  : public FuelType
{
public:
  InvalidFuel() noexcept
    : InvalidFuel(0, nullptr)
  {
  }
  /**
   * \brief Placeholder fuel that throws exceptions if it ever gets used.
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr InvalidFuel(const FuelCodeSize& code, const char* name) noexcept
    : FuelType(code, name, false)
  {
  }
  ~InvalidFuel() override = default;
  InvalidFuel(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel(InvalidFuel&& rhs) noexcept = delete;
  InvalidFuel& operator=(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel& operator=(InvalidFuel&& rhs) noexcept = delete;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const wx::FwiWeather& wx) const override;
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
  [[nodiscard]] MathSize calculateRos(int, const wx::FwiWeather&, MathSize) const
    override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo&, MathSize) const
    override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const
    override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize lengthToBreadth(MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize finalRos(const SpreadInfo&,
                                  MathSize,
                                  MathSize,
                                  MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo&) const
    override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize crownFractionBurned(MathSize,
                                             MathSize) const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize probabilityPeat(MathSize) const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize survivalProbability(const wx::FwiWeather&) const noexcept
    override;
};
}
}
