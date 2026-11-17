/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "FBP.h"
#include "FireSpread.h"
#include "FWI.h"
#include "unstable.h"
#ifdef DEBUG_FUEL_VARIABLE
#include "Log.h"
#endif
namespace fs::fuel
{
using settings::Settings;
MathSize calculate_surface_fuel_consumption_mixed_or_c2(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 5.0 * (1.0 - exp(-0.0115 * bui));
}
MathSize calculate_surface_fuel_consumption_d1(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 1.5 * (1.0 - exp(-0.0183 * bui));
}
MathSize calculate_surface_fuel_consumption_jackpine(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 5.0 * pow(1.0 - exp(-0.0164 * bui), 2.24);
}
MathSize calculate_surface_fuel_consumption_pine(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 5.0 * pow(1.0 - exp(-0.0149 * bui), 2.48);
}
MathSize calculate_length_to_breadth_grass(const MathSize w) noexcept
{
  const auto ws = round_value(w);
  return ws < 1.0 ? 1.0 : (1.1 * pow(ws, 0.464));
}
MathSize calculate_base_multiplier_curing(const MathSize c) noexcept
{
  const auto curing = round_value(c);
  return (curing >= 58.8) ? (0.176 + 0.02 * (curing - 58.8)) : (0.005 * expm1(0.061 * curing));
}
MathSize FuelD1::isfD1(const SpreadInfo& spread, const MathSize ros_multiplier, const MathSize isi)
  const noexcept
{
  return limitIsf(
    ros_multiplier,
    spread.slopeFactor() * (ros_multiplier * a()) * pow(1.0 - exp(negB() * isi), c())
  );
}
/**
 * \brief Surface Fuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 * \param ffmc Fine Fuel Moisture Code
 * \return Surface Fuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c1(const MathSize f) noexcept
{
  const auto ffmc = round_value(f);
  return max(0.0, 0.75 + ((ffmc > 84) ? 0.75 : -0.75) * sqrt(1 - exp(-0.23 * abs(ffmc - 84))));
}
MathSize FuelC1::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c1(spread.weather->ffmc.value);
}
MathSize FuelC2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_mixed_or_c2(spread.weather->bui.value);
}
MathSize FuelC6::finalRos(
  const SpreadInfo& spread,
  const MathSize isi,
  const MathSize cfb,
  const MathSize rss
) const noexcept
{
  const auto rsc = crownRateOfSpread(isi, spread.foliarMoisture());
  // using max with 0 is the same as ensuring rsc > rss
  return rss + cfb * max(0.0, rsc - rss);
}
/**
 * \brief Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 * \param ffmc Fine Fuel Moisture Code
 * \return Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c7_ffmc(const MathSize f) noexcept
{
  const auto ffmc = round_value(f);
  return (ffmc > 70) ? 2.0 * (1.0 - exp(-0.104 * (ffmc - 70.0))) : 0.0;
}
/**
 * \brief Woody Fuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 * \return Woody Fuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 */
[[nodiscard]] MathSize calculate_surface_fuel_consumption_c7_bui(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return 1.5 * (1.0 - exp(-0.0201 * bui));
}
MathSize FuelC7::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_c7_ffmc(spread.weather->ffmc.value)
       + calculate_surface_fuel_consumption_c7_bui(spread.weather->bui.value);
}
[[nodiscard]] MathSize calculate_surface_fuel_consumption_d2(const MathSize b) noexcept
{
  const auto bui = round_value(b);
  return bui >= 80 ? 1.5 * (1.0 - exp(-0.0183 * bui)) : 0.0;
}
MathSize FuelD2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return calculate_surface_fuel_consumption_d2(spread.weather->bui.value);
}
MathSize FuelD2::calculateRos(const int, const FwiWeather& wx, const MathSize isi) const noexcept
{
  return (wx.bui.value >= 80) ? rosBasic(isi) : 0.0;
}
// FIX: ensure actual code use in compilation doesn't matter and don't need to be speicified
// manually in sequence
static_assert(0 == INVALID_FUEL_CODE);
static fs::fuel::InvalidFuel NULL_FUEL{INVALID_FUEL_CODE, "Non-fuel"};
static fs::fuel::InvalidFuel INVALID{1, "Invalid"};
static FuelC1 C1{2};
static FuelC2 C2{3};
static FuelC3 C3{4};
static FuelC4 C4{5};
static FuelC5 C5{6};
static FuelC6 C6{7};
static FuelC7 C7{8};
static FuelD1 D1{9};
static FuelD2 D2{10};
static FuelO1A O1_A{11};
static FuelO1B O1_B{12};
static FuelS1 S1{13};
static FuelS2 S2{14};
static FuelS3 S3{15};
static FuelD1D2 D1_D2{16, &D1, &D2};
static FuelM1 M1_05{17, "M-1 (05 PC)", 5};
static FuelM1 M1_10{18, "M-1 (10 PC)", 10};
static FuelM1 M1_15{19, "M-1 (15 PC)", 15};
static FuelM1 M1_20{20, "M-1 (20 PC)", 20};
static FuelM1 M1_25{21, "M-1 (25 PC)", 25};
static FuelM1 M1_30{22, "M-1 (30 PC)", 30};
static FuelM1 M1_35{23, "M-1 (35 PC)", 35};
static FuelM1 M1_40{24, "M-1 (40 PC)", 40};
static FuelM1 M1_45{25, "M-1 (45 PC)", 45};
static FuelM1 M1_50{26, "M-1 (50 PC)", 50};
static FuelM1 M1_55{27, "M-1 (55 PC)", 55};
static FuelM1 M1_60{28, "M-1 (60 PC)", 60};
static FuelM1 M1_65{29, "M-1 (65 PC)", 65};
static FuelM1 M1_70{30, "M-1 (70 PC)", 70};
static FuelM1 M1_75{31, "M-1 (75 PC)", 75};
static FuelM1 M1_80{32, "M-1 (80 PC)", 80};
static FuelM1 M1_85{33, "M-1 (85 PC)", 85};
static FuelM1 M1_90{34, "M-1 (90 PC)", 90};
static FuelM1 M1_95{35, "M-1 (95 PC)", 95};
static FuelM2 M2_05{36, "M-2 (05 PC)", 5};
static FuelM2 M2_10{37, "M-2 (10 PC)", 10};
static FuelM2 M2_15{38, "M-2 (15 PC)", 15};
static FuelM2 M2_20{39, "M-2 (20 PC)", 20};
static FuelM2 M2_25{40, "M-2 (25 PC)", 25};
static FuelM2 M2_30{41, "M-2 (30 PC)", 30};
static FuelM2 M2_35{42, "M-2 (35 PC)", 35};
static FuelM2 M2_40{43, "M-2 (40 PC)", 40};
static FuelM2 M2_45{44, "M-2 (45 PC)", 45};
static FuelM2 M2_50{45, "M-2 (50 PC)", 50};
static FuelM2 M2_55{46, "M-2 (55 PC)", 55};
static FuelM2 M2_60{47, "M-2 (60 PC)", 60};
static FuelM2 M2_65{48, "M-2 (65 PC)", 65};
static FuelM2 M2_70{49, "M-2 (70 PC)", 70};
static FuelM2 M2_75{50, "M-2 (75 PC)", 75};
static FuelM2 M2_80{51, "M-2 (80 PC)", 80};
static FuelM2 M2_85{52, "M-2 (85 PC)", 85};
static FuelM2 M2_90{53, "M-2 (90 PC)", 90};
static FuelM2 M2_95{54, "M-2 (95 PC)", 95};
static FuelM1M2 M1_M2_05{55, "M-1/M-2 (05 PC)", &M1_05, &M2_05, 5};
static FuelM1M2 M1_M2_10{56, "M-1/M-2 (10 PC)", &M1_10, &M2_10, 10};
static FuelM1M2 M1_M2_15{57, "M-1/M-2 (15 PC)", &M1_15, &M2_15, 15};
static FuelM1M2 M1_M2_20{58, "M-1/M-2 (20 PC)", &M1_20, &M2_20, 20};
static FuelM1M2 M1_M2_25{59, "M-1/M-2 (25 PC)", &M1_25, &M2_25, 25};
static FuelM1M2 M1_M2_30{60, "M-1/M-2 (30 PC)", &M1_30, &M2_30, 30};
static FuelM1M2 M1_M2_35{61, "M-1/M-2 (35 PC)", &M1_35, &M2_35, 35};
static FuelM1M2 M1_M2_40{62, "M-1/M-2 (40 PC)", &M1_40, &M2_40, 40};
static FuelM1M2 M1_M2_45{63, "M-1/M-2 (45 PC)", &M1_45, &M2_45, 45};
static FuelM1M2 M1_M2_50{64, "M-1/M-2 (50 PC)", &M1_50, &M2_50, 50};
static FuelM1M2 M1_M2_55{65, "M-1/M-2 (55 PC)", &M1_55, &M2_55, 55};
static FuelM1M2 M1_M2_60{66, "M-1/M-2 (60 PC)", &M1_60, &M2_60, 60};
static FuelM1M2 M1_M2_65{67, "M-1/M-2 (65 PC)", &M1_65, &M2_65, 65};
static FuelM1M2 M1_M2_70{68, "M-1/M-2 (70 PC)", &M1_70, &M2_70, 70};
static FuelM1M2 M1_M2_75{69, "M-1/M-2 (75 PC)", &M1_75, &M2_75, 75};
static FuelM1M2 M1_M2_80{70, "M-1/M-2 (80 PC)", &M1_80, &M2_80, 80};
static FuelM1M2 M1_M2_85{71, "M-1/M-2 (85 PC)", &M1_85, &M2_85, 85};
static FuelM1M2 M1_M2_90{72, "M-1/M-2 (90 PC)", &M1_90, &M2_90, 90};
static FuelM1M2 M1_M2_95{73, "M-1/M-2 (95 PC)", &M1_95, &M2_95, 95};
static FuelM3 M3_05{74, "M-3 (05 PDF)", 5};
static FuelM3 M3_10{75, "M-3 (10 PDF)", 10};
static FuelM3 M3_15{76, "M-3 (15 PDF)", 15};
static FuelM3 M3_20{77, "M-3 (20 PDF)", 20};
static FuelM3 M3_25{78, "M-3 (25 PDF)", 25};
static FuelM3 M3_30{79, "M-3 (30 PDF)", 30};
static FuelM3 M3_35{80, "M-3 (35 PDF)", 35};
static FuelM3 M3_40{81, "M-3 (40 PDF)", 40};
static FuelM3 M3_45{82, "M-3 (45 PDF)", 45};
static FuelM3 M3_50{83, "M-3 (50 PDF)", 50};
static FuelM3 M3_55{84, "M-3 (55 PDF)", 55};
static FuelM3 M3_60{85, "M-3 (60 PDF)", 60};
static FuelM3 M3_65{86, "M-3 (65 PDF)", 65};
static FuelM3 M3_70{87, "M-3 (70 PDF)", 70};
static FuelM3 M3_75{88, "M-3 (75 PDF)", 75};
static FuelM3 M3_80{89, "M-3 (80 PDF)", 80};
static FuelM3 M3_85{90, "M-3 (85 PDF)", 85};
static FuelM3 M3_90{91, "M-3 (90 PDF)", 90};
static FuelM3 M3_95{92, "M-3 (95 PDF)", 95};
static FuelM3 M3_100{93, "M-3 (100 PDF)", 100};
static FuelM4 M4_05{94, "M-4 (05 PDF)", 5};
static FuelM4 M4_10{95, "M-4 (10 PDF)", 10};
static FuelM4 M4_15{96, "M-4 (15 PDF)", 15};
static FuelM4 M4_20{97, "M-4 (20 PDF)", 20};
static FuelM4 M4_25{98, "M-4 (25 PDF)", 25};
static FuelM4 M4_30{99, "M-4 (30 PDF)", 30};
static FuelM4 M4_35{100, "M-4 (35 PDF)", 35};
static FuelM4 M4_40{101, "M-4 (40 PDF)", 40};
static FuelM4 M4_45{102, "M-4 (45 PDF)", 45};
static FuelM4 M4_50{103, "M-4 (50 PDF)", 50};
static FuelM4 M4_55{104, "M-4 (55 PDF)", 55};
static FuelM4 M4_60{105, "M-4 (60 PDF)", 60};
static FuelM4 M4_65{106, "M-4 (65 PDF)", 65};
static FuelM4 M4_70{107, "M-4 (70 PDF)", 70};
static FuelM4 M4_75{108, "M-4 (75 PDF)", 75};
static FuelM4 M4_80{109, "M-4 (80 PDF)", 80};
static FuelM4 M4_85{110, "M-4 (85 PDF)", 85};
static FuelM4 M4_90{111, "M-4 (90 PDF)", 90};
static FuelM4 M4_95{112, "M-4 (95 PDF)", 95};
static FuelM4 M4_100{113, "M-4 (100 PDF)", 100};
static FuelM3M4 M3_M4_05{114, "M-3/M-4 (05 PDF)", &M3_05, &M4_05, 5};
static FuelM3M4 M3_M4_10{115, "M-3/M-4 (10 PDF)", &M3_10, &M4_10, 10};
static FuelM3M4 M3_M4_15{116, "M-3/M-4 (15 PDF)", &M3_15, &M4_15, 15};
static FuelM3M4 M3_M4_20{117, "M-3/M-4 (20 PDF)", &M3_20, &M4_20, 20};
static FuelM3M4 M3_M4_25{118, "M-3/M-4 (25 PDF)", &M3_25, &M4_25, 25};
static FuelM3M4 M3_M4_30{119, "M-3/M-4 (30 PDF)", &M3_30, &M4_30, 30};
static FuelM3M4 M3_M4_35{120, "M-3/M-4 (35 PDF)", &M3_35, &M4_35, 35};
static FuelM3M4 M3_M4_40{121, "M-3/M-4 (40 PDF)", &M3_40, &M4_40, 40};
static FuelM3M4 M3_M4_45{122, "M-3/M-4 (45 PDF)", &M3_45, &M4_45, 45};
static FuelM3M4 M3_M4_50{123, "M-3/M-4 (50 PDF)", &M3_50, &M4_50, 50};
static FuelM3M4 M3_M4_55{124, "M-3/M-4 (55 PDF)", &M3_55, &M4_55, 55};
static FuelM3M4 M3_M4_60{125, "M-3/M-4 (60 PDF)", &M3_60, &M4_60, 60};
static FuelM3M4 M3_M4_65{126, "M-3/M-4 (65 PDF)", &M3_65, &M4_65, 65};
static FuelM3M4 M3_M4_70{127, "M-3/M-4 (70 PDF)", &M3_70, &M4_70, 70};
static FuelM3M4 M3_M4_75{128, "M-3/M-4 (75 PDF)", &M3_75, &M4_75, 75};
static FuelM3M4 M3_M4_80{129, "M-3/M-4 (80 PDF)", &M3_80, &M4_80, 80};
static FuelM3M4 M3_M4_85{130, "M-3/M-4 (85 PDF)", &M3_85, &M4_85, 85};
static FuelM3M4 M3_M4_90{131, "M-3/M-4 (90 PDF)", &M3_90, &M4_90, 90};
static FuelM3M4 M3_M4_95{132, "M-3/M-4 (95 PDF)", &M3_95, &M4_95, 95};
static FuelM3M4 M3_M4_100{133, "M-3/M-4 (100 PDF)", &M3_100, &M4_100, 100};
static FuelM1 M1_00{134, "M-1 (00 PC)", 0};
static FuelM2 M2_00{135, "M-2 (00 PC)", 0};
static FuelM1M2 M1_M2_00{136, "M-1/M-2 (00 PC)", &M1_00, &M2_00, 0};
static FuelM3 M3_00{137, "M-3 (00 PDF)", 0};
static FuelM4 M4_00{138, "M-4 (00 PDF)", 0};
static FuelM3M4 M3_M4_00{139, "M-3/M-4 (00 PDF)", &M3_00, &M4_00, 0};
static FuelVariable O1{140, "O-1", &O1_A, &O1_B};
const array<const FuelType*, NUMBER_OF_FUELS> Fuels{
  &NULL_FUEL, &INVALID,  &C1,       &C2,        &C3,       &C4,       &C5,       &C6,
  &C7,        &D1,       &D2,       &O1_A,      &O1_B,     &S1,       &S2,       &S3,
  &D1_D2,     &M1_05,    &M1_10,    &M1_15,     &M1_20,    &M1_25,    &M1_30,    &M1_35,
  &M1_40,     &M1_45,    &M1_50,    &M1_55,     &M1_60,    &M1_65,    &M1_70,    &M1_75,
  &M1_80,     &M1_85,    &M1_90,    &M1_95,     &M2_05,    &M2_10,    &M2_15,    &M2_20,
  &M2_25,     &M2_30,    &M2_35,    &M2_40,     &M2_45,    &M2_50,    &M2_55,    &M2_60,
  &M2_65,     &M2_70,    &M2_75,    &M2_80,     &M2_85,    &M2_90,    &M2_95,    &M1_M2_05,
  &M1_M2_10,  &M1_M2_15, &M1_M2_20, &M1_M2_25,  &M1_M2_30, &M1_M2_35, &M1_M2_40, &M1_M2_45,
  &M1_M2_50,  &M1_M2_55, &M1_M2_60, &M1_M2_65,  &M1_M2_70, &M1_M2_75, &M1_M2_80, &M1_M2_85,
  &M1_M2_90,  &M1_M2_95, &M3_05,    &M3_10,     &M3_15,    &M3_20,    &M3_25,    &M3_30,
  &M3_35,     &M3_40,    &M3_45,    &M3_50,     &M3_55,    &M3_60,    &M3_65,    &M3_70,
  &M3_75,     &M3_80,    &M3_85,    &M3_90,     &M3_95,    &M3_100,   &M4_05,    &M4_10,
  &M4_15,     &M4_20,    &M4_25,    &M4_30,     &M4_35,    &M4_40,    &M4_45,    &M4_50,
  &M4_55,     &M4_60,    &M4_65,    &M4_70,     &M4_75,    &M4_80,    &M4_85,    &M4_90,
  &M4_95,     &M4_100,   &M3_M4_00, &M3_M4_05,  &M3_M4_10, &M3_M4_15, &M3_M4_20, &M3_M4_25,
  &M3_M4_30,  &M3_M4_35, &M3_M4_40, &M3_M4_45,  &M3_M4_50, &M3_M4_55, &M3_M4_60, &M3_M4_65,
  &M3_M4_70,  &M3_M4_75, &M3_M4_80, &M3_M4_85,  &M3_M4_90, &M3_M4_95, &M1_00,    &M2_00,
  &M1_M2_00,  &M3_00,    &M4_00,    &M3_M4_100, &O1,
};
MathSize compare_by_season(const FuelVariable& fuel, const function<MathSize(const FuelType&)>& fct)
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
}
