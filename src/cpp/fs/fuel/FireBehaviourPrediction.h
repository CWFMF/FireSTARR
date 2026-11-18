/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FBP_H
#define FS_FBP_H
#include "../stdafx.h"
#include "../sim/Settings.h"
#include "FuelType.h"
#include "Greenup.h"
#include "StandardFuel.h"
namespace fs::fuel
{
using fs::fuel::calculate_grass_curing;
using fs::fuel::DEFAULT_GRASS_FUEL_LOAD;
using settings::Settings;
// Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 11]
[[nodiscard]] MathSize calculate_surface_fuel_consumption_jackpine(const LookupValue& bui) noexcept;
// Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 12]
[[nodiscard]] MathSize calculate_surface_fuel_consumption_pine(const LookupValue& bui) noexcept;
// Surface Fuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c1(const LookupValue& ffmc) noexcept;
// Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 10]
[[nodiscard]] MathSize calculate_surface_fuel_consumption_mixed_or_c2(const LookupValue& bui
) noexcept;
// Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c7_ffmc(const LookupValue& ffmc) noexcept;
// Woody Fuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c7_bui(const LookupValue& bui) noexcept;
// Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
[[nodiscard]] MathSize calculate_surface_fuel_consumption_d1(const LookupValue& bui) noexcept;
[[nodiscard]] MathSize calculate_surface_fuel_consumption_d2(const LookupValue& bui) noexcept;
// Base multiplier for rate of spread [GLC-X-10 eq 35a/35b]
[[nodiscard]] MathSize calculate_base_multiplier_curing(const LookupValue& curing) noexcept;
// Length to Breadth ratio [ST-X-3 eq 80/81]
[[nodiscard]] MathSize calculate_length_to_breadth_grass(const LookupValue& ws) noexcept;
/**
 * \brief A StandardFuel that is not made of multiple fuels.
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
class FuelNonMixed : public StandardFuel
{
public:
  FuelNonMixed() = delete;
  ~FuelNonMixed() override = default;
  FuelNonMixed(const FuelNonMixed& rhs) noexcept = delete;
  FuelNonMixed(FuelNonMixed&& rhs) noexcept = delete;
  FuelNonMixed& operator=(const FuelNonMixed& rhs) noexcept = delete;
  FuelNonMixed& operator=(FuelNonMixed&& rhs) noexcept = delete;

protected:
  FuelNonMixed(
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
  );
  FuelNonMixed(
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
  );

public:
  /**
   * \brief ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param spread SpreadInfo to use in calculations
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo& spread, const RoundedValue& isi)
    const noexcept override;
  /**
   * \brief Initial rate of spread (m/min) [ST-X-3 eq 26]
   * \param isi Initial Spread Index
   * \return Initial rate of spread (m/min) [ST-X-3 eq 26]
   */
  MathSize calculateRos(const int, const FwiWeather&, const RoundedValue& isi)
    const noexcept override;
};
/**
 * \brief A conifer fuel type.
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
class FuelConifer : public FuelNonMixed
{
public:
  FuelConifer() = delete;
  ~FuelConifer() override = default;
  FuelConifer(const FuelConifer& rhs) noexcept = delete;
  FuelConifer(FuelConifer&& rhs) noexcept = delete;
  FuelConifer& operator=(const FuelConifer& rhs) noexcept = delete;
  FuelConifer& operator=(FuelConifer&& rhs) noexcept = delete;

protected:
  /**
   * \brief A conifer FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  FuelConifer(
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
  );
  /**
   * \brief A conifer FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff Type of duff near the surface and deeper underground
   */
  FuelConifer(
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
  );
};
/**
 * \brief A fuel with jackpine as base fuel type.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 * \tparam Bui0 Average Build-up Index for the fuel type [ST-X-3 table 7]
 * \tparam Cbh Crown base height (m) [ST-X-3 table 8]
 * \tparam Cfl Crown fuel load (kg/m^2) [ST-X-3 table 8]
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
class FuelJackpine : public FuelConifer
{
public:
  FuelJackpine() = delete;
  ~FuelJackpine() override = default;
  FuelJackpine(const FuelJackpine& rhs) noexcept = delete;
  FuelJackpine(FuelJackpine&& rhs) noexcept = delete;
  FuelJackpine& operator=(const FuelJackpine& rhs) noexcept = delete;
  FuelJackpine& operator=(FuelJackpine&& rhs) noexcept = delete;
  FuelJackpine(
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
  );
  FuelJackpine(
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
  );
  /**
   * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 11]
   * \param spread SpreadInfo to use
   * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 11]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
};
/**
 * \brief A fuel with pine as the base fuel type.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 * \tparam Bui0 Average Build-up Index for the fuel type [ST-X-3 table 7]
 * \tparam Cbh Crown base height (m) [ST-X-3 table 8]
 * \tparam Cfl Crown fuel load (kg/m^2) [ST-X-3 table 8]
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
class FuelPine : public FuelConifer
{
public:
  FuelPine() = delete;
  ~FuelPine() override = default;
  FuelPine(const FuelPine& rhs) noexcept = delete;
  FuelPine(FuelPine&& rhs) noexcept = delete;
  FuelPine& operator=(const FuelPine& rhs) noexcept = delete;
  FuelPine& operator=(FuelPine&& rhs) noexcept = delete;
  FuelPine(
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
  );
  FuelPine(
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
  );
  /**
   * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 12]
   * \param spread SpreadInfo to use
   * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 12]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
};
/**
 * \brief FBP fuel type D-1.
 */
class FuelD1 : public FuelNonMixed
{
public:
  FuelD1() = delete;
  ~FuelD1() override = default;
  FuelD1(const FuelD1& rhs) noexcept = delete;
  FuelD1(FuelD1&& rhs) noexcept = delete;
  FuelD1& operator=(const FuelD1& rhs) noexcept = delete;
  FuelD1& operator=(FuelD1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type D-1
   * \param code Code to identify fuel with
   */
  explicit FuelD1(const FuelCodeSize& code) noexcept;
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param ros_multiplier Rate of spread multiplier [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize isfD1(
    const SpreadInfo& spread,
    const RoundedValue& ros_multiplier,
    const RoundedValue& isi
  ) const noexcept;
};
/**
 * \brief A mixedwood fuel type.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 * \tparam Bui0 Average Build-up Index for the fuel type [ST-X-3 table 7]
 * \tparam RosMultiplier Rate of spread multiplier * 10 [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
 * \tparam PercentMixed Percent conifer or dead fir
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam InorganicPercent Inorganic percent of Duff layer (%) [Anderson table 1]
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
class FuelMixed : public StandardFuel
{
  MathSize ros_multiplier_{};
  MathSize percent_mixed_{};

public:
  FuelMixed() = delete;
  ~FuelMixed() override = default;
  FuelMixed(const FuelMixed& rhs) noexcept = delete;
  FuelMixed(FuelMixed&& rhs) noexcept = delete;
  FuelMixed& operator=(const FuelMixed& rhs) noexcept = delete;
  FuelMixed& operator=(FuelMixed&& rhs) noexcept = delete;
  /**
   * \brief A mixed FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  FuelMixed(
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
  );
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 10]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 10]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66, pg 38]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66, pg 38]
   */
  [[nodiscard]] MathSize crownConsumption(const RoundedValue& cfb) const noexcept override;
  /**
   * \brief Calculate rate of spread (m/min) [ST-X-3 27/28, GLC-X-10 29/31]
   * \param isi Initial Spread Index
   * \return Calculate rate of spread (m/min) [ST-X-3 27/28, GLC-X-10 29/31]
   */
  [[nodiscard]] MathSize calculateRos(const int, const FwiWeather&, const RoundedValue& isi)
    const noexcept override;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 42]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 42]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo& spread, const RoundedValue& isi)
    const noexcept override;
  /**
   * \brief Percent Mixed (%)
   * \return Percent Mixed (%)
   */
  [[nodiscard]] MathSize percentMixed() const;
  /**
   * \brief Percent Conifer (% / 100)
   * \return Percent Conifer (% / 100)
   */
  [[nodiscard]] MathSize ratioConifer() const;
  /**
   * \brief Percent Deciduous (% / 100)
   * \return Percent Deciduous (% / 100)
   */
  [[nodiscard]] MathSize ratioDeciduous() const;

protected:
  /**
   * \brief Rate of spread multiplier [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
   * \return Rate of spread multiplier [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
   */
  [[nodiscard]] MathSize rosMultiplier() const;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize isfD1(const SpreadInfo& spread, const RoundedValue& isi) const noexcept;
};
/**
 * \brief A fuel made of dead fir and D1.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 * \tparam Bui0 Average Build-up Index for the fuel type [ST-X-3 table 7]
 * \tparam RosMultiplier Rate of spread multiplier * 10 [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
 * \tparam PercentDeadFir Percent dead fir in the stand.
 */
class FuelMixedDead : public FuelMixed
{
public:
  FuelMixedDead() = delete;
  ~FuelMixedDead() override = default;
  FuelMixedDead(const FuelMixedDead& rhs) noexcept = delete;
  FuelMixedDead(FuelMixedDead&& rhs) noexcept = delete;
  FuelMixedDead& operator=(const FuelMixedDead& rhs) noexcept = delete;
  FuelMixedDead& operator=(FuelMixedDead&& rhs) noexcept = delete;
  /**
   * \brief A mixed dead FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  FuelMixedDead(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const MathConstantSize& a,
    const MathConstantSize& b,
    const MathConstantSize& c,
    const MathConstantSize& bui0,
    const MathConstantSize& ros_multiplier,
    const MathConstantSize& percent_dead_fir
  );
};
/**
 * \brief A fuel composed of C2 and D1 mixed.
 * \tparam RosMultiplier Rate of spread multiplier * 10 [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
 * \tparam RatioMixed Percent conifer
 */
class FuelMixedWood : public FuelMixed
{
public:
  FuelMixedWood() = delete;
  ~FuelMixedWood() override = default;
  FuelMixedWood(const FuelMixedWood& rhs) noexcept = delete;
  FuelMixedWood(FuelMixedWood&& rhs) noexcept = delete;
  FuelMixedWood& operator=(const FuelMixedWood& rhs) noexcept = delete;
  FuelMixedWood& operator=(FuelMixedWood&& rhs) noexcept = delete;
  /**
   * \brief A mixedwood FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  FuelMixedWood(
    const FuelCodeSize& code,
    const char* name,
    const MathConstantSize& ros_multiplier,
    const MathConstantSize& percent_mixed
  );
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 17]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 17]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
};
/**
 * \brief A grass fuel type.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 */
class FuelGrass : public StandardFuel
{
public:
  FuelGrass() = delete;
  ~FuelGrass() override = default;
  FuelGrass(const FuelGrass& rhs) noexcept = delete;
  FuelGrass(FuelGrass&& rhs) noexcept = delete;
  FuelGrass& operator=(const FuelGrass& rhs) noexcept = delete;
  FuelGrass& operator=(FuelGrass&& rhs) noexcept = delete;
  /**
   * \brief A grass fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  FuelGrass(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const MathConstantSize& a,
    const MathConstantSize& b,
    const MathConstantSize& c
  );
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 pg 21]
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 pg 21]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const noexcept override;
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const FwiWeather& wx) const override;
  /**
   * \brief Calculate base rate of spread multiplier
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use for calculation
   * \return Base rate of spread multiplier
   */
  [[nodiscard]] MathSize baseMultiplier(const int nd, const FwiWeather& wx) const noexcept;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo& spread, const RoundedValue& isi)
    const noexcept override;
  /**
   * \brief Calculate rate of spread (m/min)
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use for calculation
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \return Rate of spread (m/min)
   */
  [[nodiscard]] MathSize calculateRos(const int nd, const FwiWeather& wx, const RoundedValue& isi)
    const noexcept override;

public:
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 80/81]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 80/81]
   */
  [[nodiscard]] MathSize lengthToBreadth(const RoundedValue& ws) const noexcept override;

public:
  /**
   * \brief Calculate rate of spread (m/min)
   * \param multiplier Rate of spread multiplier
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \return Rate of spread (m/min)
   */
  [[nodiscard]] MathSize calculateRos(const RoundedValue& multiplier, const RoundedValue& isi)
    const noexcept;
};
/**
 * \brief FBP fuel type C-1.
 */
class FuelC1 : public FuelConifer
{
public:
  FuelC1() = delete;
  ~FuelC1() override = default;
  FuelC1(const FuelC1& rhs) noexcept = delete;
  FuelC1(FuelC1&& rhs) noexcept = delete;
  FuelC1& operator=(const FuelC1& rhs) noexcept = delete;
  FuelC1& operator=(FuelC1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-1
   * \param code Code to identify fuel with
   */
  explicit FuelC1(const FuelCodeSize& code) noexcept;
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
};
/**
 * \brief FBP fuel type C-2.
 */
class FuelC2 : public FuelConifer
{
public:
  FuelC2() = delete;
  ~FuelC2() override = default;
  FuelC2(const FuelC2& rhs) noexcept = delete;
  FuelC2(FuelC2&& rhs) noexcept = delete;
  FuelC2& operator=(const FuelC2& rhs) noexcept = delete;
  FuelC2& operator=(FuelC2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-2
   * \param code Code to identify fuel with
   */
  explicit FuelC2(const FuelCodeSize& code) noexcept;
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 10]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 10]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
};
/**
 * \brief FBP fuel type C-3.
 */
class FuelC3 : public FuelJackpine
{
public:
  FuelC3() = delete;
  ~FuelC3() override = default;
  FuelC3(const FuelC3& rhs) noexcept = delete;
  FuelC3(FuelC3&& rhs) noexcept = delete;
  FuelC3& operator=(const FuelC3& rhs) noexcept = delete;
  FuelC3& operator=(FuelC3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-3
   * \param code Code to identify fuel with
   */
  explicit FuelC3(const FuelCodeSize& code) noexcept;
};
/**
 * \brief FBP fuel type C-4.
 */
class FuelC4 : public FuelJackpine
{
public:
  FuelC4() = delete;
  ~FuelC4() override = default;
  FuelC4(const FuelC4& rhs) noexcept = delete;
  FuelC4(FuelC4&& rhs) noexcept = delete;
  FuelC4& operator=(const FuelC4& rhs) noexcept = delete;
  FuelC4& operator=(FuelC4&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-4
   * \param code Code to identify fuel with
   */
  explicit FuelC4(const FuelCodeSize& code) noexcept;
};
/**
 * \brief FBP fuel type C-5.
 */
class FuelC5 : public FuelPine
{
public:
  FuelC5() = delete;
  ~FuelC5() override = default;
  FuelC5(const FuelC5& rhs) noexcept = delete;
  FuelC5(FuelC5&& rhs) noexcept = delete;
  FuelC5& operator=(const FuelC5& rhs) noexcept = delete;
  FuelC5& operator=(FuelC5&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-5
   * \param code Code to identify fuel with
   */
  explicit FuelC5(const FuelCodeSize& code) noexcept;
};
/**
 * \brief FBP fuel type C-6.
 */
class FuelC6 : public FuelPine
{
public:
  FuelC6() = delete;
  ~FuelC6() override = default;
  FuelC6(const FuelC6& rhs) noexcept = delete;
  FuelC6(FuelC6&& rhs) noexcept = delete;
  FuelC6& operator=(const FuelC6& rhs) noexcept = delete;
  FuelC6& operator=(FuelC6&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-6
   * \param code Code to identify fuel with
   */
  explicit FuelC6(const FuelCodeSize& code) noexcept;

protected:
  /**
   * \brief Final rate of spread (m/min)
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \return Final rate of spread (m/min)
   */
  [[nodiscard]] MathSize finalRos(
    const SpreadInfo& spread,
    const RoundedValue& isi,
    const RoundedValue& cfb,
    const RoundedValue& rss
  ) const noexcept override;
};
/**
 * \brief FBP fuel type C-7.
 */
class FuelC7 : public FuelConifer
{
public:
  FuelC7() = delete;
  ~FuelC7() override = default;
  FuelC7(const FuelC7& rhs) noexcept = delete;
  FuelC7(FuelC7&& rhs) noexcept = delete;
  FuelC7& operator=(const FuelC7& rhs) noexcept = delete;
  FuelC7& operator=(FuelC7&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-7
   * \param code Code to identify fuel with
   */
  explicit FuelC7(const FuelCodeSize& code) noexcept;
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 15]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 15]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
};
/**
 * \brief FBP fuel type D-2.
 */
class FuelD2 : public FuelNonMixed
{
public:
  FuelD2() = delete;
  ~FuelD2() override = default;
  FuelD2(const FuelD2& rhs) noexcept = delete;
  FuelD2(FuelD2&& rhs) noexcept = delete;
  FuelD2& operator=(const FuelD2& rhs) noexcept = delete;
  FuelD2& operator=(FuelD2&& rhs) noexcept = delete;
  // HACK: assume same bulk_density and inorganicContent as D1
  /**
   * \brief FBP fuel type D-2
   * \param code Code to identify fuel with
   */
  explicit FuelD2(const FuelCodeSize& code) noexcept;
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2)
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2)
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;
  /**
   * \brief Calculate rate of spread (m/min)
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use for calculation
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \return Rate of spread (m/min)
   */
  [[nodiscard]] MathSize calculateRos(int nd, const FwiWeather& wx, const RoundedValue& isi)
    const noexcept override;
};
/**
 * \brief FBP fuel type M-1.
 * \tparam PercentConifer Percent conifer
 */
class FuelM1 : public FuelMixedWood
{
public:
  FuelM1() = delete;
  ~FuelM1() override = default;
  FuelM1(const FuelM1& rhs) noexcept = delete;
  FuelM1(FuelM1&& rhs) noexcept = delete;
  FuelM1& operator=(const FuelM1& rhs) noexcept = delete;
  FuelM1& operator=(FuelM1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-1
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  FuelM1(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_conifer);
};
/**
 * \brief FBP fuel type M-2.
 * \tparam PercentConifer Percent conifer
 */
class FuelM2 : public FuelMixedWood
{
public:
  FuelM2() = delete;
  ~FuelM2() override = default;
  FuelM2(const FuelM2& rhs) noexcept = delete;
  FuelM2(FuelM2&& rhs) noexcept = delete;
  FuelM2& operator=(const FuelM2& rhs) noexcept = delete;
  FuelM2& operator=(FuelM2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-2
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  FuelM2(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_conifer);
};
/**
 * \brief FBP fuel type M-3.
 * \tparam PercentDeadFir Percent dead fir
 */
class FuelM3 : public FuelMixedDead
{
public:
  FuelM3() = delete;
  ~FuelM3() override = default;
  FuelM3(const FuelM3& rhs) noexcept = delete;
  FuelM3(FuelM3&& rhs) noexcept = delete;
  FuelM3& operator=(const FuelM3& rhs) noexcept = delete;
  FuelM3& operator=(FuelM3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-3
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  FuelM3(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_dead_fir);
};
/**
 * \brief FBP fuel type M-4.
 * \tparam PercentDeadFir Percent dead fir
 */
class FuelM4 : public FuelMixedDead
{
public:
  FuelM4() = delete;
  ~FuelM4() override = default;
  FuelM4(const FuelM4& rhs) noexcept = delete;
  FuelM4(FuelM4&& rhs) noexcept = delete;
  FuelM4& operator=(const FuelM4& rhs) noexcept = delete;
  FuelM4& operator=(FuelM4&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-4
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  FuelM4(const FuelCodeSize& code, const char* name, const MathConstantSize& percent_dead_fir);
};
/**
 * \brief FBP fuel type O-1a.
 */
class FuelO1A : public FuelGrass
{
public:
  FuelO1A() = delete;
  ~FuelO1A() override = default;
  FuelO1A(const FuelO1A& rhs) noexcept = delete;
  FuelO1A(FuelO1A&& rhs) noexcept = delete;
  FuelO1A& operator=(const FuelO1A& rhs) noexcept = delete;
  FuelO1A& operator=(FuelO1A&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type O-1a.
   * \param code Code to identify fuel with
   */
  explicit FuelO1A(const FuelCodeSize& code) noexcept;
};
/**
 * \brief FBP fuel type O-1b.
 */
class FuelO1B : public FuelGrass
{
public:
  FuelO1B() = delete;
  ~FuelO1B() override = default;
  FuelO1B(const FuelO1B& rhs) noexcept = delete;
  FuelO1B(FuelO1B&& rhs) noexcept = delete;
  FuelO1B& operator=(const FuelO1B& rhs) noexcept = delete;
  FuelO1B& operator=(FuelO1B&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type O-1b.
   * \param code Code to identify fuel with
   */
  explicit FuelO1B(const FuelCodeSize& code) noexcept;
};
/**
 * \brief A slash fuel type.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 * \tparam Bui0 Average Build-up Index for the fuel type [ST-X-3 table 7]
 * \tparam FfcA Forest Floor Consumption parameter a [ST-X-3 eq 19/21/23]
 * \tparam FfcB Forest Floor Consumption parameter b * 10000 [ST-X-3 eq 19/21/23]
 * \tparam WfcA Woody Fuel Consumption parameter a [ST-X-3 eq 20/22/24]
 * \tparam WfcB Woody Fuel Consumption parameter b * 10000 [ST-X-3 eq 20/22/24]
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 */
class FuelSlash : public FuelConifer
{
  MathSize ffc_a_{};
  MathSize ffc_b_{};
  MathSize wfc_a_{};
  MathSize wfc_b_{};

public:
  FuelSlash() = delete;
  ~FuelSlash() override = default;
  FuelSlash(const FuelSlash& rhs) noexcept = delete;
  FuelSlash(FuelSlash&& rhs) noexcept = delete;
  FuelSlash& operator=(const FuelSlash& rhs) noexcept = delete;
  FuelSlash& operator=(FuelSlash&& rhs) noexcept = delete;
  /**
   * \brief A slash fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  FuelSlash(
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
  );
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override;

private:
  /**
   * \brief Forest Floor Consumption parameter a [ST-X-3 eq 19/21/23]
   * \return Forest Floor Consumption parameter a [ST-X-3 eq 19/21/23]
   */
  [[nodiscard]] MathSize ffcA() const;
  /**
   * \brief Forest Floor Consumption parameter b [ST-X-3 eq 19/21/23]
   * \return Forest Floor Consumption parameter b [ST-X-3 eq 19/21/23]
   */
  [[nodiscard]] MathSize ffcB() const;
  /**
   * \brief Woody Fuel Consumption parameter a [ST-X-3 eq 20/22/24]
   * \return Woody Fuel Consumption parameter a [ST-X-3 eq 20/22/24]
   */
  [[nodiscard]] MathSize wfcA() const;
  /**
   * \brief Woody Fuel Consumption parameter b [ST-X-3 eq 20/22/24]
   * \return Woody Fuel Consumption parameter b [ST-X-3 eq 20/22/24]
   */
  [[nodiscard]] MathSize wfcB() const;
};
/**
 * \brief FBP fuel type S-1.
 */
class FuelS1 : public FuelSlash
{
public:
  FuelS1() = delete;
  ~FuelS1() override = default;
  FuelS1(const FuelS1& rhs) noexcept = delete;
  FuelS1(FuelS1&& rhs) noexcept = delete;
  FuelS1& operator=(const FuelS1& rhs) noexcept = delete;
  FuelS1& operator=(FuelS1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-1
   * \param code Code to identify fuel with
   */
  explicit FuelS1(const FuelCodeSize& code) noexcept;
};
/**
 * \brief FBP fuel type S-2.
 */
class FuelS2 : public FuelSlash
{
public:
  FuelS2() = delete;
  ~FuelS2() override = default;
  FuelS2(const FuelS2& rhs) noexcept = delete;
  FuelS2(FuelS2&& rhs) noexcept = delete;
  FuelS2& operator=(const FuelS2& rhs) noexcept = delete;
  FuelS2& operator=(FuelS2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-2
   * \param code Code to identify fuel with
   */
  explicit FuelS2(const FuelCodeSize& code) noexcept;
};
/**
 * \brief FBP fuel type S-3.
 */
class FuelS3 : public FuelSlash
{
public:
  FuelS3() = delete;
  ~FuelS3() override = default;
  FuelS3(const FuelS3& rhs) noexcept = delete;
  FuelS3(FuelS3&& rhs) noexcept = delete;
  FuelS3& operator=(const FuelS3& rhs) noexcept = delete;
  FuelS3& operator=(FuelS3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-3
   * \param code Code to identify fuel with
   */
  explicit FuelS3(const FuelCodeSize& code) noexcept;
};
class FuelVariable;
[[nodiscard]] MathSize compare_by_season(
  const FuelVariable& fuel,
  const function<MathSize(const FuelType&)>& fct
);
/**
 * \brief A fuel type that changes based on the season.
 * \tparam FuelSpring Fuel type to use in the spring
 * \tparam FuelSummer Fuel type to use in the summer
 */
class FuelVariable : public FuelType
{
public:
  // don't delete pointers since they're handled elsewhere
  ~FuelVariable() override = default;
  /**
   * \brief A slash fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param spring Fuel type to use in the spring
   * \param summer Fuel type to use in the summer
   */
  FuelVariable(
    const FuelCodeSize& code,
    const char* name,
    const FuelType* const spring,
    const FuelType* const summer
  );
  FuelVariable(FuelVariable&& rhs) noexcept = delete;
  FuelVariable(const FuelVariable& rhs) = delete;
  FuelVariable& operator=(FuelVariable&& rhs) noexcept = delete;
  FuelVariable& operator=(const FuelVariable& rhs) = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override;
  /**
   * \brief BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   * \param bui Build-up Index
   * \return BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   */
  [[nodiscard]] MathSize buiEffect(const RoundedValue& bui) const override;
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const FwiWeather& wx) const override;
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
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   */
  [[nodiscard]] MathSize crownConsumption(const RoundedValue& cfb) const override;
  /**
   * \brief Initial rate of spread (m/min) [ST-X-3 eq 26]
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use
   * \param isi Initial Spread Index
   * \return Initial rate of spread (m/min) [ST-X-3 eq 26]
   */
  [[nodiscard]] MathSize calculateRos(const int, const FwiWeather&, const RoundedValue&)
    const override;
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo&, const RoundedValue&) const override;
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const override;
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 79]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 79]
   */
  [[nodiscard]] MathSize lengthToBreadth(const RoundedValue& ws) const override;
  /**
   * \brief Final rate of spread (m/min)
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \return Final rate of spread (m/min)
   */
  [[nodiscard]] MathSize
  finalRos(const SpreadInfo&, const RoundedValue&, const RoundedValue&, const RoundedValue&)
    const override;
  /**
   * \brief Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   * \param spread SpreadInfo to use in calculation
   * \return Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo&) const override;
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
  [[nodiscard]] MathSize probabilityPeat(const RoundedValue& mc_fraction) const noexcept override;
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] MathSize survivalProbability(const FwiWeather& wx) const noexcept override;
  /**
   * \brief Fuel to use before green-up
   * \return Fuel to use before green-up
   */
  [[nodiscard]] const FuelType* spring() const noexcept override;
  /**
   * \brief Fuel to use after green-up
   * \return Fuel to use after green-up
   */
  [[nodiscard]] const FuelType* summer() const noexcept override;

private:
  /**
   * \brief Fuel to use before green-up
   */
  const FuelType* const spring_{nullptr};
  /**
   * \brief Fuel to use after green-up
   */
  const FuelType* const summer_{nullptr};
};
/**
 * \brief FBP fuel type D-1/D-2.
 */
class FuelD1D2 : public FuelVariable
{
public:
  FuelD1D2() = delete;
  ~FuelD1D2() override = default;
  FuelD1D2(const FuelD1D2& rhs) noexcept = delete;
  FuelD1D2(FuelD1D2&& rhs) noexcept = delete;
  FuelD1D2& operator=(const FuelD1D2& rhs) noexcept = delete;
  FuelD1D2& operator=(FuelD1D2&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between D-1/D-2 depending on green-up
   * \param code Code to identify fuel with
   * \param d1 D-1 fuel to use before green-up
   * \param d2 D-2 fuel to use after green-up
   */
  FuelD1D2(const FuelCodeSize& code, const FuelD1* d1, const FuelD2* d2) noexcept;
};
/**
 * \brief FBP fuel type M-1/M-2.
 * \tparam PercentConifer Percent conifer
 */
class FuelM1M2 : public FuelVariable
{
public:
  FuelM1M2() = delete;
  ~FuelM1M2() override = default;
  FuelM1M2(const FuelM1M2& rhs) noexcept = delete;
  FuelM1M2(FuelM1M2&& rhs) noexcept = delete;
  FuelM1M2& operator=(const FuelM1M2& rhs) noexcept = delete;
  FuelM1M2& operator=(FuelM1M2&& rhs) noexcept = delete;
  // HACK: it's up to you to make sure these match
  /**
   * \brief A fuel that changes between M-1/M-2 depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param m1 M-1 fuel to use before green-up
   * \param m2 M-2 fuel to use after green-up
   */
  FuelM1M2(
    const FuelCodeSize& code,
    const char* name,
    const FuelM1* m1,
    const FuelM2* m2,
    // HACK: to ensure they match for now
    const MathConstantSize& percent_conifer
  );
};
/**
 * \brief FBP fuel type M-3/M-4.
 * \tparam PercentDeadFir Percent dead fir
 */
class FuelM3M4 : public FuelVariable
{
public:
  FuelM3M4() = delete;
  ~FuelM3M4() override = default;
  FuelM3M4(const FuelM3M4& rhs) noexcept = delete;
  FuelM3M4(FuelM3M4&& rhs) noexcept = delete;
  FuelM3M4& operator=(const FuelM3M4& rhs) noexcept = delete;
  FuelM3M4& operator=(FuelM3M4&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between M-3/M-4 depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param m3 M-3 fuel to use before green-up
   * \param m4 M-4 fuel to use after green-up
   */
  FuelM3M4(
    const FuelCodeSize& code,
    const char* name,
    const FuelM3* m3,
    const FuelM4* m4,
    // HACK: to ensure they match for now
    const MathConstantSize& percent_dead_fir
  );
};
extern const array<const FuelType*, NUMBER_OF_FUELS> Fuels;
}
#endif
