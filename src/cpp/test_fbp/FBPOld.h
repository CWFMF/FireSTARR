/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FBP_OLD_H
#define FS_FBP_OLD_H
#include "../fs/Duff.h"
#include "../fs/Greenup.h"
#include "../fs/Settings.h"
#include "../fs/stdafx.h"
#include "StandardFuelOld.h"
#ifdef DEBUG_FUEL_VARIABLE
#include "Log.h"
#endif
namespace fs::fuelold
{
[[nodiscard]] MathSize calculate_surface_fuel_consumption_mixed_or_c2(const MathSize b) noexcept;
[[nodiscard]] MathSize calculate_surface_fuel_consumption_d1(const MathSize b) noexcept;
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
class FuelOldNonMixed
  : public StandardFuelOld<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>
{
public:
  FuelOldNonMixed() = delete;
  ~FuelOldNonMixed() override = default;
  FuelOldNonMixed(const FuelOldNonMixed& rhs) noexcept = delete;
  FuelOldNonMixed(FuelOldNonMixed&& rhs) noexcept = delete;
  FuelOldNonMixed& operator=(const FuelOldNonMixed& rhs) noexcept = delete;
  FuelOldNonMixed& operator=(FuelOldNonMixed&& rhs) noexcept = delete;

protected:
  using StandardFuelOld<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>::
    StandardFuelOld;

public:
  /**
   * \brief ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param spread SpreadInfo to use in calculations
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo& spread, const MathSize isi)
    const noexcept override
  {
    return this->limitIsf(
      1.0, calculateRos(spread.nd(), *spread.weather, isi) * spread.slopeFactor()
    );
  }
  /**
   * \brief Initial rate of spread (m/min) [ST-X-3 eq 26]
   * \param isi Initial Spread Index
   * \return Initial rate of spread (m/min) [ST-X-3 eq 26]
   */
  MathSize calculateRos(const int, const FwiWeather&, const MathSize isi) const noexcept override
  {
    return this->rosBasic(isi);
  }
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
class FuelOldConifer
  : public FuelOldNonMixed<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>
{
public:
  FuelOldConifer() = delete;
  ~FuelOldConifer() override = default;
  FuelOldConifer(const FuelOldConifer& rhs) noexcept = delete;
  FuelOldConifer(FuelOldConifer&& rhs) noexcept = delete;
  FuelOldConifer& operator=(const FuelOldConifer& rhs) noexcept = delete;
  FuelOldConifer& operator=(FuelOldConifer&& rhs) noexcept = delete;

protected:
  /**
   * \brief A conifer FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr FuelOldConifer(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  )
    : FuelOldNonMixed<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>(
        code,
        name,
        true,
        log_q,
        duff_ffmc,
        duff_dmc
      )
  { }
  /**
   * \brief A conifer FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff Type of duff near the surface and deeper underground
   */
  constexpr FuelOldConifer(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const Duff* duff
  )
    : FuelOldConifer(code, name, log_q, duff, duff)
  { }
};
/**
 * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 11]
 * \param bui Build-up Index
 * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 11]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_jackpine(const MathSize b) noexcept;
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
template <int A, int B, int C, int Bui0, int Cbh, int Cfl, int BulkDensity, int DuffDepth>
class FuelOldJackpine : public FuelOldConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>
{
public:
  FuelOldJackpine() = delete;
  ~FuelOldJackpine() override = default;
  FuelOldJackpine(const FuelOldJackpine& rhs) noexcept = delete;
  FuelOldJackpine(FuelOldJackpine&& rhs) noexcept = delete;
  FuelOldJackpine& operator=(const FuelOldJackpine& rhs) noexcept = delete;
  FuelOldJackpine& operator=(FuelOldJackpine&& rhs) noexcept = delete;
  using FuelOldConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>::FuelOldConifer;
  /**
   * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 11]
   * \param spread SpreadInfo to use
   * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 11]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override
  {
    return calculate_surface_fuel_consumption_jackpine(spread.weather->bui.value);
  }
};
/**
 * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 12]
 * \param bui Build-up Index
 * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 12]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_pine(const MathSize b) noexcept;
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
template <int A, int B, int C, int Bui0, int Cbh, int Cfl, int BulkDensity, int DuffDepth>
class FuelOldPine : public FuelOldConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>
{
public:
  FuelOldPine() = delete;
  ~FuelOldPine() override = default;
  FuelOldPine(const FuelOldPine& rhs) noexcept = delete;
  FuelOldPine(FuelOldPine&& rhs) noexcept = delete;
  FuelOldPine& operator=(const FuelOldPine& rhs) noexcept = delete;
  FuelOldPine& operator=(FuelOldPine&& rhs) noexcept = delete;
  using FuelOldConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>::FuelOldConifer;
  /**
   * \brief Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 12]
   * \param spread SpreadInfo to use
   * \return Surface fuel consumption (SFC) (kg/m^2) [ST-X-3 eq 12]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override
  {
    return calculate_surface_fuel_consumption_pine(spread.weather->bui.value);
  }
};
/**
 * \brief FBP fuel type D-1.
 */
class FuelOldD1 : public FuelOldNonMixed<30, 232, 160, 32, 0, 0, 61, 59, 24>
{
public:
  FuelOldD1() = delete;
  ~FuelOldD1() override = default;
  FuelOldD1(const FuelOldD1& rhs) noexcept = delete;
  FuelOldD1(FuelOldD1&& rhs) noexcept = delete;
  FuelOldD1& operator=(const FuelOldD1& rhs) noexcept = delete;
  FuelOldD1& operator=(FuelOldD1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type D-1
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldD1(const FuelCodeSize& code) noexcept
    : FuelOldNonMixed(code, "D-1", false, LOG_0_90, &duff::Peat)
  { }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override
  {
    return calculate_surface_fuel_consumption_d1(spread.weather->bui.value);
  }
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param ros_multiplier Rate of spread multiplier [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize isfD1(const SpreadInfo& spread, MathSize ros_multiplier, MathSize isi)
    const noexcept;
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
template <
  int A,
  int B,
  int C,
  int Bui0,
  int RosMultiplier,
  int PercentMixed,
  int BulkDensity,
  int InorganicPercent,
  int DuffDepth>
class FuelOldMixed
  : public StandardFuelOld<A, B, C, Bui0, 6, 80, BulkDensity, InorganicPercent, DuffDepth>
{
public:
  FuelOldMixed() = delete;
  ~FuelOldMixed() override = default;
  FuelOldMixed(const FuelOldMixed& rhs) noexcept = delete;
  FuelOldMixed(FuelOldMixed&& rhs) noexcept = delete;
  FuelOldMixed& operator=(const FuelOldMixed& rhs) noexcept = delete;
  FuelOldMixed& operator=(FuelOldMixed&& rhs) noexcept = delete;
  /**
   * \brief A mixed FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  constexpr FuelOldMixed(const FuelCodeSize& code, const char* name, const LogValue log_q)
    : StandardFuelOld<A, B, C, Bui0, 6, 80, BulkDensity, InorganicPercent, DuffDepth>(
        code,
        name,
        true,
        log_q,
        &duff::Peat,
        &duff::Peat
      )
  { }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 10]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 10]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override
  {
    return calculate_surface_fuel_consumption_mixed_or_c2(spread.weather->bui.value);
  }
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66, pg 38]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66, pg 38]
   */
  [[nodiscard]] MathSize crownConsumption(const MathSize cfb) const noexcept override
  {
    return ratioConifer()
         * StandardFuelOld<A, B, C, Bui0, 6, 80, BulkDensity, InorganicPercent, DuffDepth>::
             crownConsumption(cfb);
  }
  /**
   * \brief Calculate rate of spread (m/min) [ST-X-3 27/28, GLC-X-10 29/31]
   * \param isi Initial Spread Index
   * \return Calculate rate of spread (m/min) [ST-X-3 27/28, GLC-X-10 29/31]
   */
  [[nodiscard]] MathSize calculateRos(const int, const FwiWeather&, const MathSize isi)
    const noexcept override
  {
    static const FuelOldD1 F{14};
    return ratioConifer() * this->rosBasic(isi)
         + rosMultiplier() * ratioDeciduous() * F.rosBasic(isi);
  }
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 42]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 42]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo& spread, const MathSize isi)
    const noexcept override
  {
    return ratioConifer() * this->limitIsf(1.0, spread.slopeFactor() * this->rosBasic(isi))
         + ratioDeciduous() * isfD1(spread, isi);
  }
  /**
   * \brief Percent Conifer (% / 100)
   * \return Percent Conifer (% / 100)
   */
  [[nodiscard]] static constexpr MathSize ratioConifer() { return PercentMixed / 100.0; }
  /**
   * \brief Percent Deciduous (% / 100)
   * \return Percent Deciduous (% / 100)
   */
  [[nodiscard]] static constexpr MathSize ratioDeciduous() { return 1.0 - (PercentMixed / 100.0); }

protected:
  /**
   * \brief Rate of spread multiplier [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
   * \return Rate of spread multiplier [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
   */
  [[nodiscard]] static constexpr MathSize rosMultiplier() { return RosMultiplier / 10.0; }
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) for D-1 [ST-X-3 eq 41]
   */
  [[nodiscard]] static MathSize isfD1(const SpreadInfo& spread, const MathSize isi) noexcept
  {
    static const FuelOldD1 F{14};
    return F.isfD1(spread, rosMultiplier(), isi);
  }
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
template <int A, int B, int C, int Bui0, int RosMultiplier, int PercentDeadFir>
class FuelOldMixedDead
  : public FuelOldMixed<A, B, C, Bui0, RosMultiplier, PercentDeadFir, 61, 15, 75>
{
public:
  FuelOldMixedDead() = delete;
  ~FuelOldMixedDead() override = default;
  FuelOldMixedDead(const FuelOldMixedDead& rhs) noexcept = delete;
  FuelOldMixedDead(FuelOldMixedDead&& rhs) noexcept = delete;
  FuelOldMixedDead& operator=(const FuelOldMixedDead& rhs) noexcept = delete;
  FuelOldMixedDead& operator=(FuelOldMixedDead&& rhs) noexcept = delete;
  /**
   * \brief A mixed dead FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  constexpr FuelOldMixedDead(const FuelCodeSize& code, const char* name, const LogValue log_q)
    : FuelOldMixed<A, B, C, Bui0, RosMultiplier, PercentDeadFir, 61, 15, 75>(code, name, log_q)
  { }
};
/**
 * \brief A fuel composed of C2 and D1 mixed.
 * \tparam RosMultiplier Rate of spread multiplier * 10 [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
 * \tparam RatioMixed Percent conifer
 */
template <int RosMultiplier, int RatioMixed>
class FuelOldMixedWood
  : public FuelOldMixed<110, 282, 150, 50, RosMultiplier, RatioMixed, 108, 25, 50>
{
public:
  FuelOldMixedWood() = delete;
  ~FuelOldMixedWood() override = default;
  FuelOldMixedWood(const FuelOldMixedWood& rhs) noexcept = delete;
  FuelOldMixedWood(FuelOldMixedWood&& rhs) noexcept = delete;
  FuelOldMixedWood& operator=(const FuelOldMixedWood& rhs) noexcept = delete;
  FuelOldMixedWood& operator=(FuelOldMixedWood&& rhs) noexcept = delete;
  /**
   * \brief A mixedwood FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr FuelOldMixedWood(const FuelCodeSize& code, const char* name)
    : FuelOldMixed<110, 282, 150, 50, RosMultiplier, RatioMixed, 108, 25, 50>(code, name, LOG_0_80)
  { }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 17]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 17]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override
  {
    return this->ratioConifer()
           * FuelOldMixed<110, 282, 150, 50, RosMultiplier, RatioMixed, 108, 25, 50>::
               surfaceFuelConsumption(spread)
         + this->ratioDeciduous()
             * calculate_surface_fuel_consumption_d1(spread.weather->bui.value);
  }
};
/**
 * \brief Length to Breadth ratio [ST-X-3 eq 80/81]
 */
[[nodiscard]] MathSize calculate_length_to_breadth_grass(const MathSize w) noexcept;
/**
 * \brief Base multiplier for rate of spread [GLC-X-10 eq 35a/35b]
 * \param curing Grass fuel curing rate (%)
 * \return Base multiplier for rate of spread [GLC-X-10 eq 35a/35b]
 */
[[nodiscard]] MathSize calculate_base_multiplier_curing(const MathSize c) noexcept;
/**
 * \brief A grass fuel type.
 * \tparam A Rate of spread parameter a [ST-X-3 table 6]
 * \tparam B Rate of spread parameter b * 10000 [ST-X-3 table 6]
 * \tparam C Rate of spread parameter c * 100 [ST-X-3 table 6]
 */
template <int A, int B, int C>
class FuelOldGrass
  : public StandardFuelOld<
      A,
      B,
      C,
      1,
      0,
      0,
      0,
      0,
      static_cast<int>(fs::survival::DUFF_FFMC_DEPTH * 10.0)>
{
public:
  FuelOldGrass() = delete;
  ~FuelOldGrass() override = default;
  FuelOldGrass(const FuelOldGrass& rhs) noexcept = delete;
  FuelOldGrass(FuelOldGrass&& rhs) noexcept = delete;
  FuelOldGrass& operator=(const FuelOldGrass& rhs) noexcept = delete;
  FuelOldGrass& operator=(FuelOldGrass&& rhs) noexcept = delete;
  /**
   * \brief A grass fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  constexpr FuelOldGrass(const FuelCodeSize& code, const char* name, const LogValue log_q)
    // HACK: grass assumes no duff (total duff depth == ffmc depth => dmc depth is 0)
    : StandardFuelOld<
        A,
        B,
        C,
        1,
        0,
        0,
        0,
        0,
        static_cast<int>(fs::survival::DUFF_FFMC_DEPTH * 10.0)>(
        code,
        name,
        false,
        log_q,
        &duff::PeatMuck,
        &duff::PeatMuck
      )
  { }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 pg 21]
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 pg 21]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const noexcept override
  {
    return fs::fuel::DEFAULT_GRASS_FUEL_LOAD;
  }
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const FwiWeather& wx) const override
  {
    // HACK: resolve once and fail if not set already
    static const auto& settings = fs::settings::instance();
    if (settings.static_curing.has_value())
    {
      return settings.static_curing.value();
    }
    const auto is_drought = wx.dc.value > 500;
    return is_drought ? 100 : fs::fuel::calculate_grass_curing(nd);
  }
  /**
   * \brief Calculate base rate of spread multiplier
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use for calculation
   * \return Base rate of spread multiplier
   */
  [[nodiscard]] MathSize baseMultiplier(const int nd, const FwiWeather& wx) const noexcept
  {
    return calculate_base_multiplier_curing(grass_curing(nd, wx));
  }
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo& spread, const MathSize isi)
    const noexcept override
  {
    const auto mu = baseMultiplier(spread.nd(), *spread.weather);
    // prevent divide by 0
    const auto mu_not_zero = max(0.001, mu);
    return this->limitIsf(mu_not_zero, calculateRos(mu, isi) * spread.slopeFactor());
  }
  /**
   * \brief Calculate rate of spread (m/min)
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use for calculation
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \return Rate of spread (m/min)
   */
  [[nodiscard]] MathSize calculateRos(const int nd, const FwiWeather& wx, const MathSize isi)
    const noexcept override
  {
    return calculateRos(baseMultiplier(nd, wx), isi);
  }

public:
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 80/81]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 80/81]
   */
  [[nodiscard]] MathSize lengthToBreadth(const MathSize ws) const noexcept override
  {
    return calculate_length_to_breadth_grass(ws);
  }

public:
  /**
   * \brief Calculate rate of spread (m/min)
   * \param multiplier Rate of spread multiplier
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \return Rate of spread (m/min)
   */
  [[nodiscard]] MathSize calculateRos(const MathSize multiplier, const MathSize isi) const noexcept
  {
    return multiplier * this->rosBasic(isi);
  }
};
/**
 * \brief FBP fuel type C-1.
 */
class FuelOldC1 : public FuelOldConifer<90, 649, 450, 72, 2, 75, 45, 5, 34>
{
public:
  FuelOldC1() = delete;
  ~FuelOldC1() override = default;
  FuelOldC1(const FuelOldC1& rhs) noexcept = delete;
  FuelOldC1(FuelOldC1&& rhs) noexcept = delete;
  FuelOldC1& operator=(const FuelOldC1& rhs) noexcept = delete;
  FuelOldC1& operator=(FuelOldC1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-1
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldC1(const FuelCodeSize& code) noexcept
    : FuelOldConifer(code, "C-1", LOG_0_90, &duff::Reindeer, &duff::Peat)
  { }
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
class FuelOldC2 : public FuelOldConifer<110, 282, 150, 64, 3, 80, 34, 0, 100>
{
public:
  FuelOldC2() = delete;
  ~FuelOldC2() override = default;
  FuelOldC2(const FuelOldC2& rhs) noexcept = delete;
  FuelOldC2(FuelOldC2&& rhs) noexcept = delete;
  FuelOldC2& operator=(const FuelOldC2& rhs) noexcept = delete;
  FuelOldC2& operator=(FuelOldC2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-2
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldC2(const FuelCodeSize& code) noexcept
    : FuelOldConifer(code, "C-2", LOG_0_70, &duff::SphagnumUpper)
  { }
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
class FuelOldC3 : public FuelOldJackpine<110, 444, 300, 62, 8, 115, 20, 65>
{
public:
  FuelOldC3() = delete;
  ~FuelOldC3() override = default;
  FuelOldC3(const FuelOldC3& rhs) noexcept = delete;
  FuelOldC3(FuelOldC3&& rhs) noexcept = delete;
  FuelOldC3& operator=(const FuelOldC3& rhs) noexcept = delete;
  FuelOldC3& operator=(FuelOldC3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-3
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldC3(const FuelCodeSize& code) noexcept
    : FuelOldJackpine(code, "C-3", LOG_0_75, &duff::FeatherMoss, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type C-4.
 */
class FuelOldC4 : public FuelOldJackpine<110, 293, 150, 66, 4, 120, 31, 62>
{
public:
  FuelOldC4() = delete;
  ~FuelOldC4() override = default;
  FuelOldC4(const FuelOldC4& rhs) noexcept = delete;
  FuelOldC4(FuelOldC4&& rhs) noexcept = delete;
  FuelOldC4& operator=(const FuelOldC4& rhs) noexcept = delete;
  FuelOldC4& operator=(FuelOldC4&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-4
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldC4(const FuelCodeSize& code) noexcept
    : FuelOldJackpine(code, "C-4", LOG_0_80, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type C-5.
 */
class FuelOldC5 : public FuelOldPine<30, 697, 400, 56, 18, 120, 93, 46>
{
public:
  FuelOldC5() = delete;
  ~FuelOldC5() override = default;
  FuelOldC5(const FuelOldC5& rhs) noexcept = delete;
  FuelOldC5(FuelOldC5&& rhs) noexcept = delete;
  FuelOldC5& operator=(const FuelOldC5& rhs) noexcept = delete;
  FuelOldC5& operator=(FuelOldC5&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-5
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldC5(const FuelCodeSize& code) noexcept
    : FuelOldPine(code, "C-5", LOG_0_80, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type C-6.
 */
class FuelOldC6 : public FuelOldPine<30, 800, 300, 62, 7, 180, 50, 50>
{
public:
  FuelOldC6() = delete;
  ~FuelOldC6() override = default;
  FuelOldC6(const FuelOldC6& rhs) noexcept = delete;
  FuelOldC6(FuelOldC6&& rhs) noexcept = delete;
  FuelOldC6& operator=(const FuelOldC6& rhs) noexcept = delete;
  FuelOldC6& operator=(FuelOldC6&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-6
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldC6(const FuelCodeSize& code) noexcept
    : FuelOldPine(code, "C-6", LOG_0_80, &duff::PineSeney)
  { }

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
    MathSize isi,
    MathSize cfb,
    MathSize rss
  ) const noexcept override;
};
/**
 * \brief FBP fuel type C-7.
 */
class FuelOldC7 : public FuelOldConifer<45, 305, 200, 106, 10, 50, 20, 15, 50>
{
public:
  FuelOldC7() = delete;
  ~FuelOldC7() override = default;
  FuelOldC7(const FuelOldC7& rhs) noexcept = delete;
  FuelOldC7(FuelOldC7&& rhs) noexcept = delete;
  FuelOldC7& operator=(const FuelOldC7& rhs) noexcept = delete;
  FuelOldC7& operator=(FuelOldC7&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-7
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldC7(const FuelCodeSize& code) noexcept
    : FuelOldConifer(code, "C-7", LOG_0_85, &duff::SprucePine)
  { }
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
class FuelOldD2 : public FuelOldNonMixed<6, 232, 160, 32, 0, 0, 61, 59, 24>
{
public:
  FuelOldD2() = delete;
  ~FuelOldD2() override = default;
  FuelOldD2(const FuelOldD2& rhs) noexcept = delete;
  FuelOldD2(FuelOldD2&& rhs) noexcept = delete;
  FuelOldD2& operator=(const FuelOldD2& rhs) noexcept = delete;
  FuelOldD2& operator=(FuelOldD2&& rhs) noexcept = delete;
  // HACK: assume same bulk_density and inorganicContent as D1
  /**
   * \brief FBP fuel type D-2
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldD2(const FuelCodeSize& code) noexcept
    : FuelOldNonMixed(code, "D-2", false, LOG_0_90, &duff::Peat)
  { }
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
  [[nodiscard]] MathSize calculateRos(int nd, const FwiWeather& wx, MathSize isi)
    const noexcept override;
};
/**
 * \brief FBP fuel type M-1.
 * \tparam PercentConifer Percent conifer
 */
template <int PercentConifer>
class FuelOldM1 : public FuelOldMixedWood<10, PercentConifer>
{
public:
  FuelOldM1() = delete;
  ~FuelOldM1() override = default;
  FuelOldM1(const FuelOldM1& rhs) noexcept = delete;
  FuelOldM1(FuelOldM1&& rhs) noexcept = delete;
  FuelOldM1& operator=(const FuelOldM1& rhs) noexcept = delete;
  FuelOldM1& operator=(FuelOldM1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-1
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr FuelOldM1(const FuelCodeSize& code, const char* name)
    : FuelOldMixedWood<10, PercentConifer>(code, name)
  { }
};
/**
 * \brief FBP fuel type M-2.
 * \tparam PercentConifer Percent conifer
 */
template <int PercentConifer>
class FuelOldM2 : public FuelOldMixedWood<2, PercentConifer>
{
public:
  FuelOldM2() = delete;
  ~FuelOldM2() override = default;
  FuelOldM2(const FuelOldM2& rhs) noexcept = delete;
  FuelOldM2(FuelOldM2&& rhs) noexcept = delete;
  FuelOldM2& operator=(const FuelOldM2& rhs) noexcept = delete;
  FuelOldM2& operator=(FuelOldM2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-2
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr FuelOldM2(const FuelCodeSize& code, const char* name)
    : FuelOldMixedWood<2, PercentConifer>(code, name)
  { }
};
/**
 * \brief FBP fuel type M-3.
 * \tparam PercentDeadFir Percent dead fir
 */
template <int PercentDeadFir>
class FuelOldM3 : public FuelOldMixedDead<120, 572, 140, 50, 10, PercentDeadFir>
{
public:
  FuelOldM3() = delete;
  ~FuelOldM3() override = default;
  FuelOldM3(const FuelOldM3& rhs) noexcept = delete;
  FuelOldM3(FuelOldM3&& rhs) noexcept = delete;
  FuelOldM3& operator=(const FuelOldM3& rhs) noexcept = delete;
  FuelOldM3& operator=(FuelOldM3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-3
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr FuelOldM3(const FuelCodeSize& code, const char* name)
    : FuelOldMixedDead<120, 572, 140, 50, 10, PercentDeadFir>(code, name, LOG_0_80)
  { }
};
/**
 * \brief FBP fuel type M-4.
 * \tparam PercentDeadFir Percent dead fir
 */
template <int PercentDeadFir>
class FuelOldM4 : public FuelOldMixedDead<100, 404, 148, 50, 2, PercentDeadFir>
{
public:
  FuelOldM4() = delete;
  ~FuelOldM4() override = default;
  FuelOldM4(const FuelOldM4& rhs) noexcept = delete;
  FuelOldM4(FuelOldM4&& rhs) noexcept = delete;
  FuelOldM4& operator=(const FuelOldM4& rhs) noexcept = delete;
  FuelOldM4& operator=(FuelOldM4&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-4
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr FuelOldM4(const FuelCodeSize& code, const char* name)
    : FuelOldMixedDead<100, 404, 148, 50, 2, PercentDeadFir>(code, name, LOG_0_80)
  { }
};
/**
 * \brief FBP fuel type O-1a.
 */
class FuelOldO1A : public FuelOldGrass<190, 310, 140>
{
public:
  FuelOldO1A() = delete;
  ~FuelOldO1A() override = default;
  FuelOldO1A(const FuelOldO1A& rhs) noexcept = delete;
  FuelOldO1A(FuelOldO1A&& rhs) noexcept = delete;
  FuelOldO1A& operator=(const FuelOldO1A& rhs) noexcept = delete;
  FuelOldO1A& operator=(FuelOldO1A&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type O-1a.
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldO1A(const FuelCodeSize& code) noexcept
    : FuelOldGrass(code, "O-1a", LOG_1_00)
  { }
};
/**
 * \brief FBP fuel type O-1b.
 */
class FuelOldO1B : public FuelOldGrass<250, 350, 170>
{
public:
  FuelOldO1B() = delete;
  ~FuelOldO1B() override = default;
  FuelOldO1B(const FuelOldO1B& rhs) noexcept = delete;
  FuelOldO1B(FuelOldO1B&& rhs) noexcept = delete;
  FuelOldO1B& operator=(const FuelOldO1B& rhs) noexcept = delete;
  FuelOldO1B& operator=(FuelOldO1B&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type O-1b.
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldO1B(const FuelCodeSize& code) noexcept
    : FuelOldGrass(code, "O-1b", LOG_1_00)
  { }
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
template <int A, int B, int C, int Bui0, int FfcA, int FfcB, int WfcA, int WfcB, int BulkDensity>
class FuelOldSlash : public FuelOldConifer<A, B, C, Bui0, 0, 0, BulkDensity, 15, 74>
{
public:
  FuelOldSlash() = delete;
  ~FuelOldSlash() override = default;
  FuelOldSlash(const FuelOldSlash& rhs) noexcept = delete;
  FuelOldSlash(FuelOldSlash&& rhs) noexcept = delete;
  FuelOldSlash& operator=(const FuelOldSlash& rhs) noexcept = delete;
  FuelOldSlash& operator=(FuelOldSlash&& rhs) noexcept = delete;
  /**
   * \brief A slash fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr FuelOldSlash(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  )
    : FuelOldConifer<A, B, C, Bui0, 0, 0, BulkDensity, 15, 74>(
        code,
        name,
        log_q,
        duff_ffmc,
        duff_dmc
      )
  { }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 25]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override
  {
    return ffcA() * (1.0 - exp(ffcB() * spread.weather->bui.value))
         + wfcA() * (1.0 - exp(wfcB() * spread.weather->bui.value));
  }

private:
  /**
   * \brief Forest Floor Consumption parameter a [ST-X-3 eq 19/21/23]
   * \return Forest Floor Consumption parameter a [ST-X-3 eq 19/21/23]
   */
  [[nodiscard]] static constexpr MathSize ffcA() { return FfcA; }
  /**
   * \brief Forest Floor Consumption parameter b [ST-X-3 eq 19/21/23]
   * \return Forest Floor Consumption parameter b [ST-X-3 eq 19/21/23]
   */
  [[nodiscard]] static constexpr MathSize ffcB() { return FfcB / 10000.0; }
  /**
   * \brief Woody Fuel Consumption parameter a [ST-X-3 eq 20/22/24]
   * \return Woody Fuel Consumption parameter a [ST-X-3 eq 20/22/24]
   */
  [[nodiscard]] static constexpr MathSize wfcA() { return WfcA; }
  /**
   * \brief Woody Fuel Consumption parameter b [ST-X-3 eq 20/22/24]
   * \return Woody Fuel Consumption parameter b [ST-X-3 eq 20/22/24]
   */
  [[nodiscard]] static constexpr MathSize wfcB() { return WfcB / 10000.0; }
};
/**
 * \brief FBP fuel type S-1.
 */
class FuelOldS1 : public FuelOldSlash<75, 297, 130, 38, 4, -250, 4, -340, 78>
{
public:
  FuelOldS1() = delete;
  ~FuelOldS1() override = default;
  FuelOldS1(const FuelOldS1& rhs) noexcept = delete;
  FuelOldS1(FuelOldS1&& rhs) noexcept = delete;
  FuelOldS1& operator=(const FuelOldS1& rhs) noexcept = delete;
  FuelOldS1& operator=(FuelOldS1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-1
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldS1(const FuelCodeSize& code) noexcept
    : FuelOldSlash(code, "S-1", LOG_0_75, &duff::FeatherMoss, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type S-2.
 */
class FuelOldS2 : public FuelOldSlash<40, 438, 170, 63, 10, -130, 6, -600, 132>
{
public:
  FuelOldS2() = delete;
  ~FuelOldS2() override = default;
  FuelOldS2(const FuelOldS2& rhs) noexcept = delete;
  FuelOldS2(FuelOldS2&& rhs) noexcept = delete;
  FuelOldS2& operator=(const FuelOldS2& rhs) noexcept = delete;
  FuelOldS2& operator=(FuelOldS2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-2
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldS2(const FuelCodeSize& code) noexcept
    : FuelOldSlash(code, "S-2", LOG_0_75, &duff::FeatherMoss, &duff::WhiteSpruce)
  { }
};
/**
 * \brief FBP fuel type S-3.
 */
class FuelOldS3 : public FuelOldSlash<55, 829, 320, 31, 12, -166, 20, -210, 100>
{
public:
  FuelOldS3() = delete;
  ~FuelOldS3() override = default;
  FuelOldS3(const FuelOldS3& rhs) noexcept = delete;
  FuelOldS3(FuelOldS3&& rhs) noexcept = delete;
  FuelOldS3& operator=(const FuelOldS3& rhs) noexcept = delete;
  FuelOldS3& operator=(FuelOldS3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-3
   * \param code Code to identify fuel with
   */
  explicit constexpr FuelOldS3(const FuelCodeSize& code) noexcept
    : FuelOldSlash(code, "S-3", LOG_0_75, &duff::FeatherMoss, &duff::PineSeney)
  { }
};
template <class FuelOldSpring, class FuelOldSummer>
class FuelOldVariable;
template <class FuelOldSpring, class FuelOldSummer>
[[nodiscard]] const FuelType& find_fuel_by_season(
  const int nd,
  const FuelOldVariable<FuelOldSpring, FuelOldSummer>& fuel
) noexcept
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  // if not green yet, then still in spring conditions
  return settings.force_greenup           ? fuel.summer()
       : settings.force_no_greenup        ? fuel.spring()
       : fs::fuel::calculate_is_green(nd) ? fuel.summer()
                                          : fuel.spring();
}
template <class FuelOldSpring, class FuelOldSummer>
[[nodiscard]] MathSize compare_by_season(
  const FuelOldVariable<FuelOldSpring, FuelOldSummer>& fuel,
  const function<MathSize(const FuelType&)>& fct
)
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
/**
 * \brief A fuel type that changes based on the season.
 * \tparam FuelSpring Fuel type to use in the spring
 * \tparam FuelSummer Fuel type to use in the summer
 */
template <class FuelOldSpring, class FuelOldSummer>
class FuelOldVariable : public FuelType
{
public:
  // don't delete pointers since they're handled elsewhere
  ~FuelOldVariable() override = default;
  /**
   * \brief A slash fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel can have a crown fire
   * \param spring Fuel type to use in the spring
   * \param summer Fuel type to use in the summer
   */
  constexpr FuelOldVariable(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const FuelOldSpring* const spring,
    const FuelOldSummer* const summer
  )
    : FuelType(code, name, can_crown), spring_(spring), summer_(summer)
  { }
  FuelOldVariable(FuelOldVariable&& rhs) noexcept = delete;
  FuelOldVariable(const FuelOldVariable& rhs) = delete;
  FuelOldVariable& operator=(FuelOldVariable&& rhs) noexcept = delete;
  FuelOldVariable& operator=(const FuelOldVariable& rhs) = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override { return true; }
  /**
   * \brief BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   * \param bui Build-up Index
   * \return BUI Effect on surface fire rate of spread [ST-X-3 eq 54]
   */
  [[nodiscard]] MathSize buiEffect(MathSize bui) const override
  {
    return compare_by_season(*this, [bui](const FuelType& fuel) { return fuel.buiEffect(bui); });
  }
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const FwiWeather& wx) const override
  {
    return compare_by_season(*this, [&](const FuelType& fuel) {
      return fuel.grass_curing(nd, wx);
    });
  }
  /**
   * \brief Crown base height (m) [ST-X-3 table 8]
   * \return Crown base height (m) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cbh() const override
  {
    return compare_by_season(*this, [](const FuelType& fuel) { return fuel.cbh(); });
  }
  /**
   * \brief Crown fuel load (kg/m^2) [ST-X-3 table 8]
   * \return Crown fuel load (kg/m^2) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cfl() const override
  {
    return compare_by_season(*this, [](const FuelType& fuel) { return fuel.cfl(); });
  }
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   */
  [[nodiscard]] MathSize crownConsumption(const MathSize cfb) const override
  {
    return compare_by_season(*this, [cfb](const FuelType& fuel) {
      return fuel.crownConsumption(cfb);
    });
  }
  /**
   * \brief Initial rate of spread (m/min) [ST-X-3 eq 26]
   * \param nd Difference between date and the date of minimum foliar moisture content
   * \param wx FwiWeather to use
   * \param isi Initial Spread Index
   * \return Initial rate of spread (m/min) [ST-X-3 eq 26]
   */
  [[nodiscard]] MathSize calculateRos(const int, const FwiWeather&, const MathSize) const override
  {
    throw runtime_error("FuelVariable not resolved to specific type");
  }
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo&, const MathSize) const override
  {
    throw runtime_error("FuelVariable not resolved to specific type");
  }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const override
  {
    throw runtime_error("FuelVariable not resolved to specific type");
  }
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 79]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 79]
   */
  [[nodiscard]] MathSize lengthToBreadth(const MathSize ws) const override
  {
    return compare_by_season(*this, [ws](const FuelType& fuel) {
      return fuel.lengthToBreadth(ws);
    });
  }
  /**
   * \brief Final rate of spread (m/min)
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index (may differ from wx because of slope)
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \return Final rate of spread (m/min)
   */
  [[nodiscard]] MathSize finalRos(const SpreadInfo&, const MathSize, const MathSize, const MathSize)
    const override
  {
    throw runtime_error("FuelVariable not resolved to specific type");
  }
  /**
   * \brief Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   * \param spread SpreadInfo to use in calculation
   * \return Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo&) const override
  {
    throw runtime_error("FuelVariable not resolved to specific type");
  }
  /**
   * \brief Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \param rso Critical surface fire spread rate (RSO) [ST-X-3 eq 57]
   * \return Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   */
  [[nodiscard]] MathSize crownFractionBurned(const MathSize rss, const MathSize rso)
    const noexcept override
  {
    return spring()->crownFractionBurned(rss, rso);
  }
  /**
   * \brief Calculate probability of burning [Anderson eq 1]
   * \param mc_fraction moisture content (% / 100)
   * \return Calculate probability of burning [Anderson eq 1]
   */
  [[nodiscard]] MathSize probabilityPeat(const MathSize mc_fraction) const noexcept override
  {
    return spring()->probabilityPeat(mc_fraction);
  }
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] MathSize survivalProbability(const FwiWeather& wx) const noexcept override
  {
    return spring()->survivalProbability(wx);
  }
  /**
   * \brief Fuel to use before green-up
   * \return Fuel to use before green-up
   */
  [[nodiscard]] const FuelType* spring() const noexcept override { return spring_; }
  /**
   * \brief Fuel to use after green-up
   * \return Fuel to use after green-up
   */
  [[nodiscard]] const FuelType* summer() const noexcept override { return summer_; }

private:
  /**
   * \brief Fuel to use before green-up
   */
  const FuelOldSpring* const spring_{nullptr};
  /**
   * \brief Fuel to use after green-up
   */
  const FuelOldSummer* const summer_{nullptr};
};
/**
 * \brief FBP fuel type D-1/D-2.
 */
class FuelOldD1D2 : public FuelOldVariable<FuelOldD1, FuelOldD2>
{
public:
  FuelOldD1D2() = delete;
  ~FuelOldD1D2() override = default;
  FuelOldD1D2(const FuelOldD1D2& rhs) noexcept = delete;
  FuelOldD1D2(FuelOldD1D2&& rhs) noexcept = delete;
  FuelOldD1D2& operator=(const FuelOldD1D2& rhs) noexcept = delete;
  FuelOldD1D2& operator=(FuelOldD1D2&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between D-1/D-2 depending on green-up
   * \param code Code to identify fuel with
   * \param d1 D-1 fuel to use before green-up
   * \param d2 D-2 fuel to use after green-up
   */
  constexpr FuelOldD1D2(const FuelCodeSize& code, const FuelOldD1* d1, const FuelOldD2* d2) noexcept
    : FuelOldVariable(code, "D-1/D-2", false, d1, d2)
  { }
};
/**
 * \brief FBP fuel type M-1/M-2.
 * \tparam PercentConifer Percent conifer
 */
template <int PercentConifer>
class FuelOldM1M2 : public FuelOldVariable<FuelOldM1<PercentConifer>, FuelOldM2<PercentConifer>>
{
public:
  FuelOldM1M2() = delete;
  ~FuelOldM1M2() override = default;
  FuelOldM1M2(const FuelOldM1M2& rhs) noexcept = delete;
  FuelOldM1M2(FuelOldM1M2&& rhs) noexcept = delete;
  FuelOldM1M2& operator=(const FuelOldM1M2& rhs) noexcept = delete;
  FuelOldM1M2& operator=(FuelOldM1M2&& rhs) noexcept = delete;
  // HACK: it's up to you to make sure these match
  /**
   * \brief A fuel that changes between M-1/M-2 depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param m1 M-1 fuel to use before green-up
   * \param m2 M-2 fuel to use after green-up
   */
  constexpr FuelOldM1M2(
    const FuelCodeSize& code,
    const char* name,
    const FuelOldM1<PercentConifer>* m1,
    const FuelOldM2<PercentConifer>* m2
  )
    : FuelOldVariable<FuelOldM1<PercentConifer>, FuelOldM2<PercentConifer>>(
        code,
        name,
        true,
        m1,
        m2
      )
  { }
};
/**
 * \brief FBP fuel type M-3/M-4.
 * \tparam PercentDeadFir Percent dead fir
 */
template <int PercentDeadFir>
class FuelOldM3M4 : public FuelOldVariable<FuelOldM3<PercentDeadFir>, FuelOldM4<PercentDeadFir>>
{
public:
  FuelOldM3M4() = delete;
  ~FuelOldM3M4() override = default;
  FuelOldM3M4(const FuelOldM3M4& rhs) noexcept = delete;
  FuelOldM3M4(FuelOldM3M4&& rhs) noexcept = delete;
  FuelOldM3M4& operator=(const FuelOldM3M4& rhs) noexcept = delete;
  FuelOldM3M4& operator=(FuelOldM3M4&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between M-3/M-4 depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param m3 M-3 fuel to use before green-up
   * \param m4 M-4 fuel to use after green-up
   */
  constexpr FuelOldM3M4(
    const FuelCodeSize& code,
    const char* name,
    const FuelOldM3<PercentDeadFir>* m3,
    const FuelOldM4<PercentDeadFir>* m4
  )
    : FuelOldVariable<FuelOldM3<PercentDeadFir>, FuelOldM4<PercentDeadFir>>(
        code,
        name,
        true,
        m3,
        m4
      )
  { }
};
/**
 * \brief FBP fuel type O-1.
 */
class FuelOldO1 : public FuelOldVariable<FuelOldO1A, FuelOldO1B>
{
public:
  FuelOldO1() = delete;
  ~FuelOldO1() override = default;
  FuelOldO1(const FuelOldO1& rhs) noexcept = delete;
  FuelOldO1(FuelOldO1&& rhs) noexcept = delete;
  FuelOldO1& operator=(const FuelOldO1& rhs) noexcept = delete;
  FuelOldO1& operator=(FuelOldO1&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between O-1a/O-1b depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param o1a O1-a fuel to use before green-up
   * \param o1b O1-b fuel to use after green-up
   */
  constexpr FuelOldO1(
    const FuelCodeSize& code,
    const char* name,
    const FuelOldO1A* o1a,
    const FuelOldO1B* o1b
  )
    : FuelOldVariable<FuelOldO1A, FuelOldO1B>(code, name, true, o1a, o1b)
  { }
};
}
#endif
