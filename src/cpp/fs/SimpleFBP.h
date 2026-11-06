/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SIMPLE_FBP_H
#define FS_SIMPLE_FBP_H
#include "stdafx.h"
#include "Duff.h"
#include "Settings.h"
#include "SimpleStandardFuel.h"
#ifdef DEBUG_FUEL_VARIABLE
#include "Log.h"
#endif
namespace fs::simplefbp
{
using settings::Settings;
/**
 * \brief Calculate if green-up has occurred
 * \param nd Difference between date and the date of minimum foliar moisture content
 * \return Whether or no green-up has occurred
 */
[[nodiscard]] constexpr bool calculate_is_green(const int nd) { return nd >= 30; }
/**
 * \brief Use intersection of parabola with y = 120.0 line as point where grass greening starts
 * happening.
 */
static constexpr int START_GREENING = -43;
[[nodiscard]] constexpr int calculate_grass_curing(const int nd)
{
  const auto curing = (nd < START_GREENING)
                      ?   // we're before foliar moisture dip has started
                        100
                      : (nd >= 50)
                          ? 0   // foliar moisture is at 120.0, so grass should be totally uncured
                          // HACK: invent a formula that has 50% curing at the bottom of the foliar
                          // moisture dip foliar moisture above ranges between 120 and 85, with 85
                          // being at the point where we want 50% cured Curing:
                          // -43 => 100, 0 => 50, 50 => 0 least-squares best fit:
                          : static_cast<int>(52.5042 - 1.07324 * nd);
  return max(0, min(100, curing));
}
[[nodiscard]] MathSize calculate_surface_fuel_consumption_mixed_or_c2(const MathSize b) noexcept;
[[nodiscard]] MathSize calculate_surface_fuel_consumption_d1(const MathSize b) noexcept;
/**
 * \brief A SimpleStandardFuel that is not made of multiple fuels.
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
class SimpleFuelNonMixed
  : public SimpleStandardFuel<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>
{
public:
  SimpleFuelNonMixed() = delete;
  ~SimpleFuelNonMixed() override = default;
  SimpleFuelNonMixed(const SimpleFuelNonMixed& rhs) noexcept = delete;
  SimpleFuelNonMixed(SimpleFuelNonMixed&& rhs) noexcept = delete;
  SimpleFuelNonMixed& operator=(const SimpleFuelNonMixed& rhs) noexcept = delete;
  SimpleFuelNonMixed& operator=(SimpleFuelNonMixed&& rhs) noexcept = delete;

protected:
  using SimpleStandardFuel<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>::
    SimpleStandardFuel;

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
class SimpleFuelConifer
  : public SimpleFuelNonMixed<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>
{
public:
  SimpleFuelConifer() = delete;
  ~SimpleFuelConifer() override = default;
  SimpleFuelConifer(const SimpleFuelConifer& rhs) noexcept = delete;
  SimpleFuelConifer(SimpleFuelConifer&& rhs) noexcept = delete;
  SimpleFuelConifer& operator=(const SimpleFuelConifer& rhs) noexcept = delete;
  SimpleFuelConifer& operator=(SimpleFuelConifer&& rhs) noexcept = delete;

protected:
  /**
   * \brief A conifer FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr SimpleFuelConifer(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  )
    : SimpleFuelNonMixed<A, B, C, Bui0, Cbh, Cfl, BulkDensity, InorganicPercent, DuffDepth>(
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
  constexpr SimpleFuelConifer(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const Duff* duff
  )
    : SimpleFuelConifer(code, name, log_q, duff, duff)
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
class SimpleFuelJackpine
  : public SimpleFuelConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>
{
public:
  SimpleFuelJackpine() = delete;
  ~SimpleFuelJackpine() override = default;
  SimpleFuelJackpine(const SimpleFuelJackpine& rhs) noexcept = delete;
  SimpleFuelJackpine(SimpleFuelJackpine&& rhs) noexcept = delete;
  SimpleFuelJackpine& operator=(const SimpleFuelJackpine& rhs) noexcept = delete;
  SimpleFuelJackpine& operator=(SimpleFuelJackpine&& rhs) noexcept = delete;
  using SimpleFuelConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>::SimpleFuelConifer;
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
class FuelPine : public SimpleFuelConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>
{
public:
  FuelPine() = delete;
  ~FuelPine() override = default;
  FuelPine(const FuelPine& rhs) noexcept = delete;
  FuelPine(FuelPine&& rhs) noexcept = delete;
  FuelPine& operator=(const FuelPine& rhs) noexcept = delete;
  FuelPine& operator=(FuelPine&& rhs) noexcept = delete;
  using SimpleFuelConifer<A, B, C, Bui0, Cbh, Cfl, BulkDensity, 15, DuffDepth>::SimpleFuelConifer;
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
class SimpleFuelD1 : public SimpleFuelNonMixed<30, 232, 160, 32, 0, 0, 61, 59, 24>
{
public:
  SimpleFuelD1() = delete;
  ~SimpleFuelD1() override = default;
  SimpleFuelD1(const SimpleFuelD1& rhs) noexcept = delete;
  SimpleFuelD1(SimpleFuelD1&& rhs) noexcept = delete;
  SimpleFuelD1& operator=(const SimpleFuelD1& rhs) noexcept = delete;
  SimpleFuelD1& operator=(SimpleFuelD1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type D-1
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelD1(const FuelCodeSize& code) noexcept
    : SimpleFuelNonMixed(code, "D-1", false, LOG_0_90, &duff::Peat)
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
class SimpleFuelMixed
  : public SimpleStandardFuel<A, B, C, Bui0, 6, 80, BulkDensity, InorganicPercent, DuffDepth>
{
public:
  SimpleFuelMixed() = delete;
  ~SimpleFuelMixed() override = default;
  SimpleFuelMixed(const SimpleFuelMixed& rhs) noexcept = delete;
  SimpleFuelMixed(SimpleFuelMixed&& rhs) noexcept = delete;
  SimpleFuelMixed& operator=(const SimpleFuelMixed& rhs) noexcept = delete;
  SimpleFuelMixed& operator=(SimpleFuelMixed&& rhs) noexcept = delete;
  /**
   * \brief A mixed FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  constexpr SimpleFuelMixed(const FuelCodeSize& code, const char* name, const LogValue log_q)
    : SimpleStandardFuel<A, B, C, Bui0, 6, 80, BulkDensity, InorganicPercent, DuffDepth>(
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
         * SimpleStandardFuel<A, B, C, Bui0, 6, 80, BulkDensity, InorganicPercent, DuffDepth>::
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
    static const SimpleFuelD1 F{14};
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
    static const SimpleFuelD1 F{14};
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
class SimpleFuelMixedDead
  : public SimpleFuelMixed<A, B, C, Bui0, RosMultiplier, PercentDeadFir, 61, 15, 75>
{
public:
  SimpleFuelMixedDead() = delete;
  ~SimpleFuelMixedDead() override = default;
  SimpleFuelMixedDead(const SimpleFuelMixedDead& rhs) noexcept = delete;
  SimpleFuelMixedDead(SimpleFuelMixedDead&& rhs) noexcept = delete;
  SimpleFuelMixedDead& operator=(const SimpleFuelMixedDead& rhs) noexcept = delete;
  SimpleFuelMixedDead& operator=(SimpleFuelMixedDead&& rhs) noexcept = delete;
  /**
   * \brief A mixed dead FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  constexpr SimpleFuelMixedDead(const FuelCodeSize& code, const char* name, const LogValue log_q)
    : SimpleFuelMixed<A, B, C, Bui0, RosMultiplier, PercentDeadFir, 61, 15, 75>(code, name, log_q)
  { }
};
/**
 * \brief A fuel composed of C2 and D1 mixed.
 * \tparam RosMultiplier Rate of spread multiplier * 10 [ST-X-3 eq 27/28, GLC-X-10 eq 29/30]
 * \tparam RatioMixed Percent conifer
 */
template <int RosMultiplier, int RatioMixed>
class SimpleFuelMixedWood
  : public SimpleFuelMixed<110, 282, 150, 50, RosMultiplier, RatioMixed, 108, 25, 50>
{
public:
  SimpleFuelMixedWood() = delete;
  ~SimpleFuelMixedWood() override = default;
  SimpleFuelMixedWood(const SimpleFuelMixedWood& rhs) noexcept = delete;
  SimpleFuelMixedWood(SimpleFuelMixedWood&& rhs) noexcept = delete;
  SimpleFuelMixedWood& operator=(const SimpleFuelMixedWood& rhs) noexcept = delete;
  SimpleFuelMixedWood& operator=(SimpleFuelMixedWood&& rhs) noexcept = delete;
  /**
   * \brief A mixedwood FBP fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr SimpleFuelMixedWood(const FuelCodeSize& code, const char* name)
    : SimpleFuelMixed<110, 282, 150, 50, RosMultiplier, RatioMixed, 108, 25, 50>(
        code,
        name,
        LOG_0_80
      )
  { }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 17]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 17]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const noexcept override
  {
    return this->ratioConifer()
           * SimpleFuelMixed<110, 282, 150, 50, RosMultiplier, RatioMixed, 108, 25, 50>::
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
class SimpleFuelGrass
  : public SimpleStandardFuel<A, B, C, 1, 0, 0, 0, 0, static_cast<int>(DUFF_FFMC_DEPTH * 10.0)>
{
public:
  SimpleFuelGrass() = delete;
  ~SimpleFuelGrass() override = default;
  SimpleFuelGrass(const SimpleFuelGrass& rhs) noexcept = delete;
  SimpleFuelGrass(SimpleFuelGrass&& rhs) noexcept = delete;
  SimpleFuelGrass& operator=(const SimpleFuelGrass& rhs) noexcept = delete;
  SimpleFuelGrass& operator=(SimpleFuelGrass&& rhs) noexcept = delete;
  /**
   * \brief A grass fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   */
  constexpr SimpleFuelGrass(const FuelCodeSize& code, const char* name, const LogValue log_q)
    // HACK: grass assumes no duff (total duff depth == ffmc depth => dmc depth is 0)
    : SimpleStandardFuel<A, B, C, 1, 0, 0, 0, 0, static_cast<int>(DUFF_FFMC_DEPTH * 10.0)>(
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
    return DEFAULT_GRASS_FUEL_LOAD;
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
    return is_drought ? 100 : calculate_grass_curing(nd);
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
class SimpleFuelC1 : public SimpleFuelConifer<90, 649, 450, 72, 2, 75, 45, 5, 34>
{
public:
  SimpleFuelC1() = delete;
  ~SimpleFuelC1() override = default;
  SimpleFuelC1(const SimpleFuelC1& rhs) noexcept = delete;
  SimpleFuelC1(SimpleFuelC1&& rhs) noexcept = delete;
  SimpleFuelC1& operator=(const SimpleFuelC1& rhs) noexcept = delete;
  SimpleFuelC1& operator=(SimpleFuelC1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-1
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelC1(const FuelCodeSize& code) noexcept
    : SimpleFuelConifer(code, "C-1", LOG_0_90, &duff::Reindeer, &duff::Peat)
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
class SimpleFuelC2 : public SimpleFuelConifer<110, 282, 150, 64, 3, 80, 34, 0, 100>
{
public:
  SimpleFuelC2() = delete;
  ~SimpleFuelC2() override = default;
  SimpleFuelC2(const SimpleFuelC2& rhs) noexcept = delete;
  SimpleFuelC2(SimpleFuelC2&& rhs) noexcept = delete;
  SimpleFuelC2& operator=(const SimpleFuelC2& rhs) noexcept = delete;
  SimpleFuelC2& operator=(SimpleFuelC2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-2
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelC2(const FuelCodeSize& code) noexcept
    : SimpleFuelConifer(code, "C-2", LOG_0_70, &duff::SphagnumUpper)
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
class SimpleFuelC3 : public SimpleFuelJackpine<110, 444, 300, 62, 8, 115, 20, 65>
{
public:
  SimpleFuelC3() = delete;
  ~SimpleFuelC3() override = default;
  SimpleFuelC3(const SimpleFuelC3& rhs) noexcept = delete;
  SimpleFuelC3(SimpleFuelC3&& rhs) noexcept = delete;
  SimpleFuelC3& operator=(const SimpleFuelC3& rhs) noexcept = delete;
  SimpleFuelC3& operator=(SimpleFuelC3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-3
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelC3(const FuelCodeSize& code) noexcept
    : SimpleFuelJackpine(code, "C-3", LOG_0_75, &duff::FeatherMoss, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type C-4.
 */
class SimpleFuelC4 : public SimpleFuelJackpine<110, 293, 150, 66, 4, 120, 31, 62>
{
public:
  SimpleFuelC4() = delete;
  ~SimpleFuelC4() override = default;
  SimpleFuelC4(const SimpleFuelC4& rhs) noexcept = delete;
  SimpleFuelC4(SimpleFuelC4&& rhs) noexcept = delete;
  SimpleFuelC4& operator=(const SimpleFuelC4& rhs) noexcept = delete;
  SimpleFuelC4& operator=(SimpleFuelC4&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-4
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelC4(const FuelCodeSize& code) noexcept
    : SimpleFuelJackpine(code, "C-4", LOG_0_80, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type C-5.
 */
class SimpleFuelC5 : public FuelPine<30, 697, 400, 56, 18, 120, 93, 46>
{
public:
  SimpleFuelC5() = delete;
  ~SimpleFuelC5() override = default;
  SimpleFuelC5(const SimpleFuelC5& rhs) noexcept = delete;
  SimpleFuelC5(SimpleFuelC5&& rhs) noexcept = delete;
  SimpleFuelC5& operator=(const SimpleFuelC5& rhs) noexcept = delete;
  SimpleFuelC5& operator=(SimpleFuelC5&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-5
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelC5(const FuelCodeSize& code) noexcept
    : FuelPine(code, "C-5", LOG_0_80, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type C-6.
 */
class SimpleFuelC6 : public FuelPine<30, 800, 300, 62, 7, 180, 50, 50>
{
public:
  SimpleFuelC6() = delete;
  ~SimpleFuelC6() override = default;
  SimpleFuelC6(const SimpleFuelC6& rhs) noexcept = delete;
  SimpleFuelC6(SimpleFuelC6&& rhs) noexcept = delete;
  SimpleFuelC6& operator=(const SimpleFuelC6& rhs) noexcept = delete;
  SimpleFuelC6& operator=(SimpleFuelC6&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-6
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelC6(const FuelCodeSize& code) noexcept
    : FuelPine(code, "C-6", LOG_0_80, &duff::PineSeney)
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
class SimpleFuelC7 : public SimpleFuelConifer<45, 305, 200, 106, 10, 50, 20, 15, 50>
{
public:
  SimpleFuelC7() = delete;
  ~SimpleFuelC7() override = default;
  SimpleFuelC7(const SimpleFuelC7& rhs) noexcept = delete;
  SimpleFuelC7(SimpleFuelC7&& rhs) noexcept = delete;
  SimpleFuelC7& operator=(const SimpleFuelC7& rhs) noexcept = delete;
  SimpleFuelC7& operator=(SimpleFuelC7&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type C-7
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelC7(const FuelCodeSize& code) noexcept
    : SimpleFuelConifer(code, "C-7", LOG_0_85, &duff::SprucePine)
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
class SimpleFuelD2 : public SimpleFuelNonMixed<6, 232, 160, 32, 0, 0, 61, 59, 24>
{
public:
  SimpleFuelD2() = delete;
  ~SimpleFuelD2() override = default;
  SimpleFuelD2(const SimpleFuelD2& rhs) noexcept = delete;
  SimpleFuelD2(SimpleFuelD2&& rhs) noexcept = delete;
  SimpleFuelD2& operator=(const SimpleFuelD2& rhs) noexcept = delete;
  SimpleFuelD2& operator=(SimpleFuelD2&& rhs) noexcept = delete;
  // HACK: assume same bulk_density and inorganicContent as D1
  /**
   * \brief FBP fuel type D-2
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelD2(const FuelCodeSize& code) noexcept
    : SimpleFuelNonMixed(code, "D-2", false, LOG_0_90, &duff::Peat)
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
class SimpleFuelM1 : public SimpleFuelMixedWood<10, PercentConifer>
{
public:
  SimpleFuelM1() = delete;
  ~SimpleFuelM1() override = default;
  SimpleFuelM1(const SimpleFuelM1& rhs) noexcept = delete;
  SimpleFuelM1(SimpleFuelM1&& rhs) noexcept = delete;
  SimpleFuelM1& operator=(const SimpleFuelM1& rhs) noexcept = delete;
  SimpleFuelM1& operator=(SimpleFuelM1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-1
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr SimpleFuelM1(const FuelCodeSize& code, const char* name)
    : SimpleFuelMixedWood<10, PercentConifer>(code, name)
  { }
};
/**
 * \brief FBP fuel type M-2.
 * \tparam PercentConifer Percent conifer
 */
template <int PercentConifer>
class SimpleFuelM2 : public SimpleFuelMixedWood<2, PercentConifer>
{
public:
  SimpleFuelM2() = delete;
  ~SimpleFuelM2() override = default;
  SimpleFuelM2(const SimpleFuelM2& rhs) noexcept = delete;
  SimpleFuelM2(SimpleFuelM2&& rhs) noexcept = delete;
  SimpleFuelM2& operator=(const SimpleFuelM2& rhs) noexcept = delete;
  SimpleFuelM2& operator=(SimpleFuelM2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-2
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr SimpleFuelM2(const FuelCodeSize& code, const char* name)
    : SimpleFuelMixedWood<2, PercentConifer>(code, name)
  { }
};
/**
 * \brief FBP fuel type M-3.
 * \tparam PercentDeadFir Percent dead fir
 */
template <int PercentDeadFir>
class SimpleFuelM3 : public SimpleFuelMixedDead<120, 572, 140, 50, 10, PercentDeadFir>
{
public:
  SimpleFuelM3() = delete;
  ~SimpleFuelM3() override = default;
  SimpleFuelM3(const SimpleFuelM3& rhs) noexcept = delete;
  SimpleFuelM3(SimpleFuelM3&& rhs) noexcept = delete;
  SimpleFuelM3& operator=(const SimpleFuelM3& rhs) noexcept = delete;
  SimpleFuelM3& operator=(SimpleFuelM3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-3
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr SimpleFuelM3(const FuelCodeSize& code, const char* name)
    : SimpleFuelMixedDead<120, 572, 140, 50, 10, PercentDeadFir>(code, name, LOG_0_80)
  { }
};
/**
 * \brief FBP fuel type M-4.
 * \tparam PercentDeadFir Percent dead fir
 */
template <int PercentDeadFir>
class SimpleFuelM4 : public SimpleFuelMixedDead<100, 404, 148, 50, 2, PercentDeadFir>
{
public:
  SimpleFuelM4() = delete;
  ~SimpleFuelM4() override = default;
  SimpleFuelM4(const SimpleFuelM4& rhs) noexcept = delete;
  SimpleFuelM4(SimpleFuelM4&& rhs) noexcept = delete;
  SimpleFuelM4& operator=(const SimpleFuelM4& rhs) noexcept = delete;
  SimpleFuelM4& operator=(SimpleFuelM4&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type M-4
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr SimpleFuelM4(const FuelCodeSize& code, const char* name)
    : SimpleFuelMixedDead<100, 404, 148, 50, 2, PercentDeadFir>(code, name, LOG_0_80)
  { }
};
/**
 * \brief FBP fuel type O-1a.
 */
class SimpleFuelO1A : public SimpleFuelGrass<190, 310, 140>
{
public:
  SimpleFuelO1A() = delete;
  ~SimpleFuelO1A() override = default;
  SimpleFuelO1A(const SimpleFuelO1A& rhs) noexcept = delete;
  SimpleFuelO1A(SimpleFuelO1A&& rhs) noexcept = delete;
  SimpleFuelO1A& operator=(const SimpleFuelO1A& rhs) noexcept = delete;
  SimpleFuelO1A& operator=(SimpleFuelO1A&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type O-1a.
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelO1A(const FuelCodeSize& code) noexcept
    : SimpleFuelGrass(code, "O-1a", LOG_1_00)
  { }
};
/**
 * \brief FBP fuel type O-1b.
 */
class SimpleFuelO1B : public SimpleFuelGrass<250, 350, 170>
{
public:
  SimpleFuelO1B() = delete;
  ~SimpleFuelO1B() override = default;
  SimpleFuelO1B(const SimpleFuelO1B& rhs) noexcept = delete;
  SimpleFuelO1B(SimpleFuelO1B&& rhs) noexcept = delete;
  SimpleFuelO1B& operator=(const SimpleFuelO1B& rhs) noexcept = delete;
  SimpleFuelO1B& operator=(SimpleFuelO1B&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type O-1b.
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelO1B(const FuelCodeSize& code) noexcept
    : SimpleFuelGrass(code, "O-1b", LOG_1_00)
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
class SimpleFuelSlash : public SimpleFuelConifer<A, B, C, Bui0, 0, 0, BulkDensity, 15, 74>
{
public:
  SimpleFuelSlash() = delete;
  ~SimpleFuelSlash() override = default;
  SimpleFuelSlash(const SimpleFuelSlash& rhs) noexcept = delete;
  SimpleFuelSlash(SimpleFuelSlash&& rhs) noexcept = delete;
  SimpleFuelSlash& operator=(const SimpleFuelSlash& rhs) noexcept = delete;
  SimpleFuelSlash& operator=(SimpleFuelSlash&& rhs) noexcept = delete;
  /**
   * \brief A slash fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param log_q Log value of q [ST-X-3 table 7]
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr SimpleFuelSlash(
    const FuelCodeSize& code,
    const char* name,
    const LogValue log_q,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  )
    : SimpleFuelConifer<A, B, C, Bui0, 0, 0, BulkDensity, 15, 74>(
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
class SimpleFuelS1 : public SimpleFuelSlash<75, 297, 130, 38, 4, -250, 4, -340, 78>
{
public:
  SimpleFuelS1() = delete;
  ~SimpleFuelS1() override = default;
  SimpleFuelS1(const SimpleFuelS1& rhs) noexcept = delete;
  SimpleFuelS1(SimpleFuelS1&& rhs) noexcept = delete;
  SimpleFuelS1& operator=(const SimpleFuelS1& rhs) noexcept = delete;
  SimpleFuelS1& operator=(SimpleFuelS1&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-1
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelS1(const FuelCodeSize& code) noexcept
    : SimpleFuelSlash(code, "S-1", LOG_0_75, &duff::FeatherMoss, &duff::PineSeney)
  { }
};
/**
 * \brief FBP fuel type S-2.
 */
class SimpleFuelS2 : public SimpleFuelSlash<40, 438, 170, 63, 10, -130, 6, -600, 132>
{
public:
  SimpleFuelS2() = delete;
  ~SimpleFuelS2() override = default;
  SimpleFuelS2(const SimpleFuelS2& rhs) noexcept = delete;
  SimpleFuelS2(SimpleFuelS2&& rhs) noexcept = delete;
  SimpleFuelS2& operator=(const SimpleFuelS2& rhs) noexcept = delete;
  SimpleFuelS2& operator=(SimpleFuelS2&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-2
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelS2(const FuelCodeSize& code) noexcept
    : SimpleFuelSlash(code, "S-2", LOG_0_75, &duff::FeatherMoss, &duff::WhiteSpruce)
  { }
};
/**
 * \brief FBP fuel type S-3.
 */
class SimpleFuelS3 : public SimpleFuelSlash<55, 829, 320, 31, 12, -166, 20, -210, 100>
{
public:
  SimpleFuelS3() = delete;
  ~SimpleFuelS3() override = default;
  SimpleFuelS3(const SimpleFuelS3& rhs) noexcept = delete;
  SimpleFuelS3(SimpleFuelS3&& rhs) noexcept = delete;
  SimpleFuelS3& operator=(const SimpleFuelS3& rhs) noexcept = delete;
  SimpleFuelS3& operator=(SimpleFuelS3&& rhs) noexcept = delete;
  /**
   * \brief FBP fuel type S-3
   * \param code Code to identify fuel with
   */
  explicit constexpr SimpleFuelS3(const FuelCodeSize& code) noexcept
    : SimpleFuelSlash(code, "S-3", LOG_0_75, &duff::FeatherMoss, &duff::PineSeney)
  { }
};
template <class FuelSpring, class FuelSummer>
class SimpleFuelVariable;
template <class FuelSpring, class FuelSummer>
[[nodiscard]] const SimpleFuelType& find_fuel_by_season(
  const int nd,
  const SimpleFuelVariable<FuelSpring, FuelSummer>& fuel
) noexcept
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  // if not green yet, then still in spring conditions
  return settings.force_greenup    ? fuel.summer()
       : settings.force_no_greenup ? fuel.spring()
       : calculate_is_green(nd)    ? fuel.summer()
                                   : fuel.spring();
}
template <class FuelSpring, class FuelSummer>
[[nodiscard]] MathSize compare_by_season(
  const SimpleFuelVariable<FuelSpring, FuelSummer>& fuel,
  const function<MathSize(const SimpleFuelType&)>& fct
)
{
  // HACK: no way to tell which is which, so let's assume they have to be the same??
  // HACK: use a function so that DEBUG section doesn't get out of sync
  const auto for_spring = fct(fuel.spring());
#ifdef DEBUG_FUEL_VARIABLE
  const auto for_summer = fct(fuel.summer());
  logging::check_fatal(for_spring != for_summer, "Expected spring and summer cfb to be identical");
#endif
  return for_spring;
}
/**
 * \brief A fuel type that changes based on the season.
 * \tparam FuelSpring Fuel type to use in the spring
 * \tparam FuelSummer Fuel type to use in the summer
 */
template <class FuelSpring, class FuelSummer>
class SimpleFuelVariable : public SimpleFuelType
{
public:
  // don't delete pointers since they're handled elsewhere
  ~SimpleFuelVariable() override = default;
  /**
   * \brief A slash fuel type
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel can have a crown fire
   * \param spring Fuel type to use in the spring
   * \param summer Fuel type to use in the summer
   */
  constexpr SimpleFuelVariable(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const FuelSpring* const spring,
    const FuelSummer* const summer
  )
    : SimpleFuelType(code, name, can_crown), spring_(spring), summer_(summer)
  { }
  SimpleFuelVariable(SimpleFuelVariable&& rhs) noexcept = delete;
  SimpleFuelVariable(const SimpleFuelVariable& rhs) = delete;
  SimpleFuelVariable& operator=(SimpleFuelVariable&& rhs) noexcept = delete;
  SimpleFuelVariable& operator=(const SimpleFuelVariable& rhs) = delete;
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
    return compare_by_season(*this, [bui](const SimpleFuelType& fuel) {
      return fuel.buiEffect(bui);
    });
  }
  /**
   * \brief Grass curing
   * \return Grass curing (or -1 if invalid for this fuel type)
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const FwiWeather& wx) const override
  {
    return compare_by_season(*this, [&](const SimpleFuelType& fuel) {
      return fuel.grass_curing(nd, wx);
    });
  }
  /**
   * \brief Crown base height (m) [ST-X-3 table 8]
   * \return Crown base height (m) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cbh() const override
  {
    return compare_by_season(*this, [](const SimpleFuelType& fuel) { return fuel.cbh(); });
  }
  /**
   * \brief Crown fuel load (kg/m^2) [ST-X-3 table 8]
   * \return Crown fuel load (kg/m^2) [ST-X-3 table 8]
   */
  [[nodiscard]] MathSize cfl() const override
  {
    return compare_by_season(*this, [](const SimpleFuelType& fuel) { return fuel.cfl(); });
  }
  /**
   * \brief Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   * \param cfb Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \return Crown Fuel Consumption (CFC) (kg/m^2) [ST-X-3 eq 66]
   */
  [[nodiscard]] MathSize crownConsumption(const MathSize cfb) const override
  {
    return compare_by_season(*this, [cfb](const SimpleFuelType& fuel) {
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
  [[nodiscard]] MathSize calculateRos(const int nd, const FwiWeather& wx, const MathSize isi)
    const override
  {
    return find_fuel_by_season(nd, *this).calculateRos(nd, wx, isi);
  }
  /**
   * \brief Calculate ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   * \param spread SpreadInfo to use
   * \param isi Initial Spread Index
   * \return ISI with slope influence and zero wind (ISF) [ST-X-3 eq 41]
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo& spread, const MathSize isi) const override
  {
    return find_fuel_by_season(spread.nd(), *this).calculateIsf(spread, isi);
  }
  /**
   * \brief Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   * \param spread SpreadInfo to use
   * \return Surface Fuel Consumption (SFC) (kg/m^2) [ST-X-3 eq 9-25]
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo& spread) const override
  {
    return find_fuel_by_season(spread.nd(), *this).surfaceFuelConsumption(spread);
  }
  /**
   * \brief Length to Breadth ratio [ST-X-3 eq 79]
   * \param ws Wind Speed (km/h)
   * \return Length to Breadth ratio [ST-X-3 eq 79]
   */
  [[nodiscard]] MathSize lengthToBreadth(const MathSize ws) const override
  {
    return compare_by_season(*this, [ws](const SimpleFuelType& fuel) {
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
  [[nodiscard]] MathSize finalRos(
    const SpreadInfo& spread,
    const MathSize isi,
    const MathSize cfb,
    const MathSize rss
  ) const override
  {
    return find_fuel_by_season(spread.nd(), *this).finalRos(spread, isi, cfb, rss);
  }
  /**
   * \brief Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   * \param spread SpreadInfo to use in calculation
   * \return Critical Surface Fire Intensity (CSI) [ST-X-3 eq 56]
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const override
  {
    return find_fuel_by_season(spread.nd(), *this).criticalSurfaceIntensity(spread);
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
    return spring().crownFractionBurned(rss, rso);
  }
  /**
   * \brief Calculate probability of burning [Anderson eq 1]
   * \param mc_fraction moisture content (% / 100)
   * \return Calculate probability of burning [Anderson eq 1]
   */
  [[nodiscard]] MathSize probabilityPeat(const MathSize mc_fraction) const noexcept override
  {
    return spring().probabilityPeat(mc_fraction);
  }
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] MathSize survivalProbability(const FwiWeather& wx) const noexcept override
  {
    return spring().survivalProbability(wx);
  }
  /**
   * \brief Fuel to use before green-up
   * \return Fuel to use before green-up
   */
  [[nodiscard]] constexpr const SimpleFuelType& spring() const { return *spring_; }
  /**
   * \brief Fuel to use after green-up
   * \return Fuel to use after green-up
   */
  [[nodiscard]] constexpr const SimpleFuelType& summer() const { return *summer_; }

private:
  /**
   * \brief Fuel to use before green-up
   */
  const FuelSpring* const spring_{nullptr};
  /**
   * \brief Fuel to use after green-up
   */
  const FuelSummer* const summer_{nullptr};
};
/**
 * \brief FBP fuel type D-1/D-2.
 */
class SimpleFuelD1D2 : public SimpleFuelVariable<SimpleFuelD1, SimpleFuelD2>
{
public:
  SimpleFuelD1D2() = delete;
  ~SimpleFuelD1D2() override = default;
  SimpleFuelD1D2(const SimpleFuelD1D2& rhs) noexcept = delete;
  SimpleFuelD1D2(SimpleFuelD1D2&& rhs) noexcept = delete;
  SimpleFuelD1D2& operator=(const SimpleFuelD1D2& rhs) noexcept = delete;
  SimpleFuelD1D2& operator=(SimpleFuelD1D2&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between D-1/D-2 depending on green-up
   * \param code Code to identify fuel with
   * \param d1 D-1 fuel to use before green-up
   * \param d2 D-2 fuel to use after green-up
   */
  constexpr SimpleFuelD1D2(
    const FuelCodeSize& code,
    const SimpleFuelD1* d1,
    const SimpleFuelD2* d2
  ) noexcept
    : SimpleFuelVariable(code, "D-1/D-2", false, d1, d2)
  { }
};
/**
 * \brief FBP fuel type M-1/M-2.
 * \tparam PercentConifer Percent conifer
 */
template <int PercentConifer>
class SimpleFuelM1M2
  : public SimpleFuelVariable<SimpleFuelM1<PercentConifer>, SimpleFuelM2<PercentConifer>>
{
public:
  SimpleFuelM1M2() = delete;
  ~SimpleFuelM1M2() override = default;
  SimpleFuelM1M2(const SimpleFuelM1M2& rhs) noexcept = delete;
  SimpleFuelM1M2(SimpleFuelM1M2&& rhs) noexcept = delete;
  SimpleFuelM1M2& operator=(const SimpleFuelM1M2& rhs) noexcept = delete;
  SimpleFuelM1M2& operator=(SimpleFuelM1M2&& rhs) noexcept = delete;
  // HACK: it's up to you to make sure these match
  /**
   * \brief A fuel that changes between M-1/M-2 depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param m1 M-1 fuel to use before green-up
   * \param m2 M-2 fuel to use after green-up
   */
  constexpr SimpleFuelM1M2(
    const FuelCodeSize& code,
    const char* name,
    const SimpleFuelM1<PercentConifer>* m1,
    const SimpleFuelM2<PercentConifer>* m2
  )
    : SimpleFuelVariable<SimpleFuelM1<PercentConifer>, SimpleFuelM2<PercentConifer>>(
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
class SimpleFuelM3M4
  : public SimpleFuelVariable<SimpleFuelM3<PercentDeadFir>, SimpleFuelM4<PercentDeadFir>>
{
public:
  SimpleFuelM3M4() = delete;
  ~SimpleFuelM3M4() override = default;
  SimpleFuelM3M4(const SimpleFuelM3M4& rhs) noexcept = delete;
  SimpleFuelM3M4(SimpleFuelM3M4&& rhs) noexcept = delete;
  SimpleFuelM3M4& operator=(const SimpleFuelM3M4& rhs) noexcept = delete;
  SimpleFuelM3M4& operator=(SimpleFuelM3M4&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between M-3/M-4 depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param m3 M-3 fuel to use before green-up
   * \param m4 M-4 fuel to use after green-up
   */
  constexpr SimpleFuelM3M4(
    const FuelCodeSize& code,
    const char* name,
    const SimpleFuelM3<PercentDeadFir>* m3,
    const SimpleFuelM4<PercentDeadFir>* m4
  )
    : SimpleFuelVariable<SimpleFuelM3<PercentDeadFir>, SimpleFuelM4<PercentDeadFir>>(
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
class SimpleFuelO1 : public SimpleFuelVariable<SimpleFuelO1A, SimpleFuelO1B>
{
public:
  SimpleFuelO1() = delete;
  ~SimpleFuelO1() override = default;
  SimpleFuelO1(const SimpleFuelO1& rhs) noexcept = delete;
  SimpleFuelO1(SimpleFuelO1&& rhs) noexcept = delete;
  SimpleFuelO1& operator=(const SimpleFuelO1& rhs) noexcept = delete;
  SimpleFuelO1& operator=(SimpleFuelO1&& rhs) noexcept = delete;
  /**
   * \brief A fuel that changes between O-1a/O-1b depending on green-up
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param o1a O1-a fuel to use before green-up
   * \param o1b O1-b fuel to use after green-up
   */
  constexpr SimpleFuelO1(
    const FuelCodeSize& code,
    const char* name,
    const SimpleFuelO1A* o1a,
    const SimpleFuelO1B* o1b
  )
    : SimpleFuelVariable<SimpleFuelO1A, SimpleFuelO1B>(code, name, true, o1a, o1b)
  { }
};
}
namespace fs::testing
{
using settings::Settings;
int test_fbp(const int argc, const char* const argv[]);
}
#endif
