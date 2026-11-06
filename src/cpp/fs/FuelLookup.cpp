/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FuelLookup.h"
#include "FBP.h"
#include "FuelType.h"
#include "Log.h"
#include "Settings.h"
namespace fs::fuel
{
using fs::fuel::simplify_fuel_name;
static const map<const string_view, const string_view> DEFAULT_TYPES{
  {"Spruce-Lichen Woodland", "C-1"},
  {"Boreal Spruce", "C-2"},
  {"Mature Jack or Lodgepole Pine", "C-3"},
  {"Immature Jack or Lodgepole Pine", "C-4"},
  {"Red and White Pine", "C-5"},
  {"Conifer Plantation", "C-6"},
  {"Ponderosa Pine - Douglas-Fir", "C-7"},
  {"Leafless Aspen", "D-1"},
  {"Green Aspen (with BUI Thresholding)", "D-2"},
  {"Aspen", "D-1/D-2"},
  {"Jack or Lodgepole Pine Slash", "S-1"},
  {"White Spruce - Balsam Slash", "S-2"},
  {"Coastal Cedar - Hemlock - Douglas-Fir Slash", "S-3"},
  {"Matted Grass", "O-1a"},
  {"Standing Grass", "O-1b"},
  {"Grass", "O-1"},
  {"Boreal Mixedwood - Leafless", "M-1"},
  {"Boreal Mixedwood - Green", "M-2"},
  {"Boreal Mixedwood", "M-1/M-2"},
  {"Dead Balsam Fir Mixedwood - Leafless", "M-3"},
  {"Dead Balsam Fir Mixedwood - Green", "M-4"},
  {"Dead Balsam Fir Mixedwood", "M-3/M-4"},
  {"Not Available", "Non-fuel"},
  {"Non-fuel", "Non-fuel"},
  {"Water", "Non-fuel"},
  {"Urban", "Non-fuel"},
  {"Unknown", "Non-fuel"},
  {"Unclassified", "D-1/D-2"},
  {"Vegetated Non-Fuel", "M-1/M-2 (25 PC)"},
  {"Boreal Mixedwood - Leafless (00% Conifer)", "M-1 (00 PC)"},
  {"Boreal Mixedwood - Leafless (05% Conifer)", "M-1 (05 PC)"},
  {"Boreal Mixedwood - Leafless (10% Conifer)", "M-1 (10 PC)"},
  {"Boreal Mixedwood - Leafless (15% Conifer)", "M-1 (15 PC)"},
  {"Boreal Mixedwood - Leafless (20% Conifer)", "M-1 (20 PC)"},
  {"Boreal Mixedwood - Leafless (25% Conifer)", "M-1 (25 PC)"},
  {"Boreal Mixedwood - Leafless (30% Conifer)", "M-1 (30 PC)"},
  {"Boreal Mixedwood - Leafless (35% Conifer)", "M-1 (35 PC)"},
  {"Boreal Mixedwood - Leafless (40% Conifer)", "M-1 (40 PC)"},
  {"Boreal Mixedwood - Leafless (45% Conifer)", "M-1 (45 PC)"},
  {"Boreal Mixedwood - Leafless (50% Conifer)", "M-1 (50 PC)"},
  {"Boreal Mixedwood - Leafless (55% Conifer)", "M-1 (55 PC)"},
  {"Boreal Mixedwood - Leafless (60% Conifer)", "M-1 (60 PC)"},
  {"Boreal Mixedwood - Leafless (65% Conifer)", "M-1 (65 PC)"},
  {"Boreal Mixedwood - Leafless (70% Conifer)", "M-1 (70 PC)"},
  {"Boreal Mixedwood - Leafless (75% Conifer)", "M-1 (75 PC)"},
  {"Boreal Mixedwood - Leafless (80% Conifer)", "M-1 (80 PC)"},
  {"Boreal Mixedwood - Leafless (85% Conifer)", "M-1 (85 PC)"},
  {"Boreal Mixedwood - Leafless (90% Conifer)", "M-1 (90 PC)"},
  {"Boreal Mixedwood - Leafless (95% Conifer)", "M-1 (95 PC)"},
  {"Boreal Mixedwood - Green (00% Conifer)", "M-2 (00 PC)"},
  {"Boreal Mixedwood - Green (05% Conifer)", "M-2 (05 PC)"},
  {"Boreal Mixedwood - Green (10% Conifer)", "M-2 (10 PC)"},
  {"Boreal Mixedwood - Green (15% Conifer)", "M-2 (15 PC)"},
  {"Boreal Mixedwood - Green (20% Conifer)", "M-2 (20 PC)"},
  {"Boreal Mixedwood - Green (25% Conifer)", "M-2 (25 PC)"},
  {"Boreal Mixedwood - Green (30% Conifer)", "M-2 (30 PC)"},
  {"Boreal Mixedwood - Green (35% Conifer)", "M-2 (35 PC)"},
  {"Boreal Mixedwood - Green (40% Conifer)", "M-2 (40 PC)"},
  {"Boreal Mixedwood - Green (45% Conifer)", "M-2 (45 PC)"},
  {"Boreal Mixedwood - Green (50% Conifer)", "M-2 (50 PC)"},
  {"Boreal Mixedwood - Green (55% Conifer)", "M-2 (55 PC)"},
  {"Boreal Mixedwood - Green (60% Conifer)", "M-2 (60 PC)"},
  {"Boreal Mixedwood - Green (65% Conifer)", "M-2 (65 PC)"},
  {"Boreal Mixedwood - Green (70% Conifer)", "M-2 (70 PC)"},
  {"Boreal Mixedwood - Green (75% Conifer)", "M-2 (75 PC)"},
  {"Boreal Mixedwood - Green (80% Conifer)", "M-2 (80 PC)"},
  {"Boreal Mixedwood - Green (85% Conifer)", "M-2 (85 PC)"},
  {"Boreal Mixedwood - Green (90% Conifer)", "M-2 (90 PC)"},
  {"Boreal Mixedwood - Green (95% Conifer)", "M-2 (95 PC)"},
  {"Boreal Mixedwood (00% Conifer)", "M-1/M-2 (00 PC)"},
  {"Boreal Mixedwood (05% Conifer)", "M-1/M-2 (05 PC)"},
  {"Boreal Mixedwood (10% Conifer)", "M-1/M-2 (10 PC)"},
  {"Boreal Mixedwood (15% Conifer)", "M-1/M-2 (15 PC)"},
  {"Boreal Mixedwood (20% Conifer)", "M-1/M-2 (20 PC)"},
  {"Boreal Mixedwood (25% Conifer)", "M-1/M-2 (25 PC)"},
  {"Boreal Mixedwood (30% Conifer)", "M-1/M-2 (30 PC)"},
  {"Boreal Mixedwood (35% Conifer)", "M-1/M-2 (35 PC)"},
  {"Boreal Mixedwood (40% Conifer)", "M-1/M-2 (40 PC)"},
  {"Boreal Mixedwood (45% Conifer)", "M-1/M-2 (45 PC)"},
  {"Boreal Mixedwood (50% Conifer)", "M-1/M-2 (50 PC)"},
  {"Boreal Mixedwood (55% Conifer)", "M-1/M-2 (55 PC)"},
  {"Boreal Mixedwood (60% Conifer)", "M-1/M-2 (60 PC)"},
  {"Boreal Mixedwood (65% Conifer)", "M-1/M-2 (65 PC)"},
  {"Boreal Mixedwood (70% Conifer)", "M-1/M-2 (70 PC)"},
  {"Boreal Mixedwood (75% Conifer)", "M-1/M-2 (75 PC)"},
  {"Boreal Mixedwood (80% Conifer)", "M-1/M-2 (80 PC)"},
  {"Boreal Mixedwood (85% Conifer)", "M-1/M-2 (85 PC)"},
  {"Boreal Mixedwood (90% Conifer)", "M-1/M-2 (90 PC)"},
  {"Boreal Mixedwood (95% Conifer)", "M-1/M-2 (95 PC)"},
  {"Dead Balsam Fir Mixedwood - Leafless (00% Dead Fir)", "M-3 (00 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (05% Dead Fir)", "M-3 (05 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (10% Dead Fir)", "M-3 (10 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (15% Dead Fir)", "M-3 (15 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (20% Dead Fir)", "M-3 (20 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (25% Dead Fir)", "M-3 (25 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (30% Dead Fir)", "M-3 (30 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (35% Dead Fir)", "M-3 (35 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (40% Dead Fir)", "M-3 (40 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (45% Dead Fir)", "M-3 (45 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (50% Dead Fir)", "M-3 (50 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (55% Dead Fir)", "M-3 (55 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (60% Dead Fir)", "M-3 (60 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (65% Dead Fir)", "M-3 (65 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (70% Dead Fir)", "M-3 (70 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (75% Dead Fir)", "M-3 (75 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (80% Dead Fir)", "M-3 (80 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (85% Dead Fir)", "M-3 (85 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (90% Dead Fir)", "M-3 (90 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (95% Dead Fir)", "M-3 (95 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (00% Dead Fir)", "M-4 (00 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (05% Dead Fir)", "M-4 (05 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (10% Dead Fir)", "M-4 (10 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (15% Dead Fir)", "M-4 (15 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (20% Dead Fir)", "M-4 (20 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (25% Dead Fir)", "M-4 (25 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (30% Dead Fir)", "M-4 (30 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (35% Dead Fir)", "M-4 (35 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (40% Dead Fir)", "M-4 (40 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (45% Dead Fir)", "M-4 (45 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (50% Dead Fir)", "M-4 (50 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (55% Dead Fir)", "M-4 (55 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (60% Dead Fir)", "M-4 (60 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (65% Dead Fir)", "M-4 (65 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (70% Dead Fir)", "M-4 (70 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (75% Dead Fir)", "M-4 (75 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (80% Dead Fir)", "M-4 (80 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (85% Dead Fir)", "M-4 (85 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (90% Dead Fir)", "M-4 (90 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (95% Dead Fir)", "M-4 (95 PDF)"},
  {"Dead Balsam Fir Mixedwood (00% Dead Fir)", "M-3/M-4 (00 PDF)"},
  {"Dead Balsam Fir Mixedwood (05% Dead Fir)", "M-3/M-4 (05 PDF)"},
  {"Dead Balsam Fir Mixedwood (10% Dead Fir)", "M-3/M-4 (10 PDF)"},
  {"Dead Balsam Fir Mixedwood (15% Dead Fir)", "M-3/M-4 (15 PDF)"},
  {"Dead Balsam Fir Mixedwood (20% Dead Fir)", "M-3/M-4 (20 PDF)"},
  {"Dead Balsam Fir Mixedwood (25% Dead Fir)", "M-3/M-4 (25 PDF)"},
  {"Dead Balsam Fir Mixedwood (30% Dead Fir)", "M-3/M-4 (30 PDF)"},
  {"Dead Balsam Fir Mixedwood (35% Dead Fir)", "M-3/M-4 (35 PDF)"},
  {"Dead Balsam Fir Mixedwood (40% Dead Fir)", "M-3/M-4 (40 PDF)"},
  {"Dead Balsam Fir Mixedwood (45% Dead Fir)", "M-3/M-4 (45 PDF)"},
  {"Dead Balsam Fir Mixedwood (50% Dead Fir)", "M-3/M-4 (50 PDF)"},
  {"Dead Balsam Fir Mixedwood (55% Dead Fir)", "M-3/M-4 (55 PDF)"},
  {"Dead Balsam Fir Mixedwood (60% Dead Fir)", "M-3/M-4 (60 PDF)"},
  {"Dead Balsam Fir Mixedwood (65% Dead Fir)", "M-3/M-4 (65 PDF)"},
  {"Dead Balsam Fir Mixedwood (70% Dead Fir)", "M-3/M-4 (70 PDF)"},
  {"Dead Balsam Fir Mixedwood (75% Dead Fir)", "M-3/M-4 (75 PDF)"},
  {"Dead Balsam Fir Mixedwood (80% Dead Fir)", "M-3/M-4 (80 PDF)"},
  {"Dead Balsam Fir Mixedwood (85% Dead Fir)", "M-3/M-4 (85 PDF)"},
  {"Dead Balsam Fir Mixedwood (90% Dead Fir)", "M-3/M-4 (90 PDF)"},
  {"Dead Balsam Fir Mixedwood (95% Dead Fir)", "M-3/M-4 (95 PDF)"},
};
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
/**
 * \brief Implementation class for FuelLookup
 */
class FuelLookupImpl
{
public:
  // do it this way so the entire array is filled with a single invalid fuel
  /**
   * \brief Construct by reading from a file
   * \param filename File to read from. Uses .lut format from Prometheus
   */
  explicit FuelLookupImpl(const char* filename)
    : fuel_types_(new array<const FuelType*, numeric_limits<FuelSize>::max()>{&INVALID})
  {
    // HACK: resolve once and fail if not set already
    static const auto& settings = fs::settings::instance();
    for (auto i : FuelLookup::Fuels)
    {
      emplaceFuel(i);
    }
    // HACK: use offset from base fuel type
    const int pc = settings.default_percent_conifer;
    logging::check_fatal(
      0 >= pc || 100 <= pc || (pc % 5) != 0, "Invalid default percent conifer ({:d})", pc
    );
    const auto pc_offset = (static_cast<size_t>(pc) / 5) - 1;
    emplaceFuel("M-1", FuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M1_05)));
    emplaceFuel("M-2", FuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M2_05)));
    emplaceFuel("M-1/M-2", FuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M1_M2_05)));
    // 0 PC/PDF makes these effectively D1/D2 because of how the equations work
    emplaceFuel("M-1 (00 PC)", &D1_D2);
    emplaceFuel("M-2 (00 PC)", &D1_D2);
    emplaceFuel("M-1/M-2 (00 PC)", &D1_D2);
    emplaceFuel("M-3 (00 PDF)", &D1_D2);
    emplaceFuel("M-4 (00 PDF)", &D1_D2);
    emplaceFuel("M-3/M-4 (00 PDF)", &D1_D2);
    const int pdf = settings.default_percent_dead_fir;
    logging::check_fatal(
      0 > pdf || 100 < pdf || (pdf % 5) != 0, "Invalid default percent dead fir ({:d})", pdf
    );
    const auto pdf_offset = static_cast<size_t>(pdf) / 5 - 1;
    emplaceFuel("M-3", FuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M3_05)));
    emplaceFuel("M-4", FuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M4_05)));
    emplaceFuel("M-3/M-4", FuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M3_M4_05)));
    ifstream in;
    in.open(filename);
    bool read_ok = false;
    if (in.is_open())
    {
      string str;
      logging::info("Reading fuel lookup table from '{:s}'", filename);
      // read header line
      getline(in, str);
      while (getline(in, str))
      {
        istringstream iss(str);
        if (getline(iss, str, ','))
        {
          // grid_value
          const auto value = static_cast<FuelSize>(stoi(str));
          // export_value
          getline(iss, str, ',');
          // descriptive_name
          getline(iss, str, ',');
          const auto name = str;
          // fuel_type
          getline(iss, str, ',');
          const auto fuel = str;
          logging::debug("Fuel {:s} has code {:d}", fuel.c_str(), value);
          const auto by_name = fuel_by_name_.find(str);
          if (by_name != fuel_by_name_.end())
          {
            const auto fuel_obj = (*by_name).second;
            fuel_types_->at(value) = fuel_obj;
            // HACK: can't figure out how to compare to fuel from .find() result so keep using .at()
            // for now
            if (!DEFAULT_TYPES.contains(name) || DEFAULT_TYPES.at(name) != fuel
                || "Not Available" == name || "Non-fuel" == name || "Unclassified" == name
                || "Urban" == name || "Unknown" == name || "Vegetated Non-Fuel" == name)
            {
              logging::note(
                "Fuel ({:d}, '{:s}') is treated like '{:s}' with internal code {:d}",
                value,
                name.c_str(),
                fuel.c_str(),
                fuel_obj->code()
              );
            }
            else
            {
              logging::debug(
                "Fuel ({:d}, '{:s}') is treated like '{:s}' with internal code {:d}",
                value,
                name.c_str(),
                fuel.c_str(),
                fuel_obj->code()
              );
            }
            auto emplaced = fuel_grid_codes_.try_emplace(fuel_obj, value);
            if (!emplaced.second)
            {
              logging::debug(
                "Fuel ({:d}, '{:s}') is treated like '{:s}' with internal code {:d} and tried to replace value {:d} for {:d}",
                value,
                name.c_str(),
                fuel.c_str(),
                fuel_obj->code(),
                value,
                emplaced.first->second
              );
            }
            fuel_good_values_[value].push_back(str);
          }
          else
          {
            logging::warning("Unknown fuel type '{:s}' in fuel lookup table", str.c_str());
            fuel_bad_values_[value].push_back(str);
          }
          read_ok = true;
        }
      }
      in.close();
    }
    if (!read_ok)
    {
      logging::fatal("Unable to read file {:s}", filename);
    }
  }
  ~FuelLookupImpl() { delete fuel_types_; }
  /**
   * \brief Put fuel into lookup table based on name
   * \param fuel FuelType to put into lookup table based on its name
   */
  void emplaceFuel(const FuelType* fuel) { emplaceFuel(FuelType::safeName(fuel), fuel); }
  /**
   * \brief Put fuel into lookup table based on name
   * \param name Name to use for FuelType in lookup table
   * \param fuel FuelType to put into lookup table
   */
  void emplaceFuel(const string_view name, const FuelType* fuel)
  {
    fuel_by_name_.emplace(name, fuel);
    const auto simple_name = simplify_fuel_name(fuel->name());
    logging::verbose(
      "'{:s}' being registered as '{:s}' with simplified name '{:s}'",
      fuel->name(),
      string(name).c_str(),
      string(simple_name).c_str()
    );
    fuel_by_simplified_name_.emplace(simple_name, fuel);
  }
  /**
   * \brief Create a set of all FuelTypes used in this lookup table
   * \return Set of all FuelTypes used in this lookup table
   */
  set<const FuelType*> usedFuels() const
  {
    set<const FuelType*> result{};
    for (const auto& kv : used_by_name_)
    {
      if (kv.second)
      {
        result.insert(fuel_by_name_.at(kv.first));
      }
    }
    return result;
  }
  FuelLookupImpl(const FuelLookupImpl& rhs) = delete;
  FuelLookupImpl(FuelLookupImpl&& rhs) = delete;
  FuelLookupImpl& operator=(const FuelLookupImpl& rhs) = delete;
  FuelLookupImpl& operator=(FuelLookupImpl&& rhs) = delete;
  /**
   * \brief Look up a FuelType based on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return FuelType based on the given code
   */
  const FuelType* codeToFuel(const FuelSize value, const FuelSize nodata) const
  {
    // NOTE: this should be looking things up based on the .lut codes
    if (nodata == value)
    {
      return nullptr;
    }
    const auto result = fuel_types_->at(static_cast<size_t>(value));
    if (nullptr != result)
    {
      // use [] for insert or change
      used_by_name_[FuelType::safeName(result)] = true;
    }
    return result;
  }
  /**
   * \brief List all fuels and their codes
   */
  void listFuels() const
  {
    for (const auto& kv : fuel_good_values_)
    {
      for (const auto& name : kv.second)
      {
        logging::note("{:d} => {:s}", kv.first, name.c_str());
      }
    }
  }
  /**
   * \brief Look up the original grid code for a FuelType
   * \param value Value to use for lookup
   * \return Original grid code for the FuelType
   */
  FuelSize fuelToCode(const FuelType* const value) const
  {
    if (nullptr == value)
    {
      // HACK: for now assume 0 is always the invalid fuel type
      return INVALID_FUEL_CODE;
    }
    const auto seek = fuel_grid_codes_.find(value);
    if (seek != fuel_grid_codes_.end())
    {
      return seek->second;
    }
    logging::warning(
      "Invalid FuelType lookup: ({:s}, {:d}) was never used in grid with {:d} fuel codes defined",
      value->name(),
      value->code(),
      fuel_grid_codes_.size()
    );
    throw runtime_error("Converting fuel that wasn't in input grid to code");
    return 0;
  }
  /**
   * \brief Look up a FuelType based on the given name
   * \param name Name of the fuel to find
   * \return FuelType based on the given name
   */
  const FuelType* byName(const string_view name) const
  {
    const auto seek = fuel_by_name_.find(name);
    if (seek != fuel_by_name_.end())
    {
      return seek->second;
    }
    return nullptr;
  }
  /**
   * \brief Look up a FuelType based on the given simplified name
   * \param name Simplified name of the fuel to find
   * \return FuelType based on the given name
   */
  const FuelType* bySimplifiedName(const string_view name) const
  {
    const auto seek = fuel_by_simplified_name_.find(name);
    if (seek != fuel_by_simplified_name_.end())
    {
      return seek->second;
    }
    return nullptr;
  }

private:
  /**
   * \brief Array of all possible fuel types
   */
  array<const FuelType*, numeric_limits<FuelSize>::max()>* fuel_types_{nullptr};
  /**
   * \brief Map of FuelType to (first) original grid value
   */
  unordered_map<const FuelType*, FuelSize> fuel_grid_codes_{};
  /**
   * \brief Map of fuel name to FuelType
   */
  string_map<const FuelType*> fuel_by_name_{};
  /**
   * \brief Map of simplified fuel name to FuelType
   */
  string_map<const FuelType*> fuel_by_simplified_name_{};
  /**
   * \brief Map of fuel name to whether or not it is used in this simulation
   */
  mutable string_map<bool> used_by_name_{};
  /**
   * \brief Codes from input .lut that were for fuel types that are implemented
   */
  unordered_map<FuelSize, vector<string>> fuel_good_values_{};
  /**
   * \brief Codes from input .lut that were for fuel types that are not implemented
   */
  unordered_map<FuelSize, vector<string>> fuel_bad_values_{};
};
const FuelType* FuelLookup::codeToFuel(const FuelSize value, const FuelSize nodata) const
{
  return impl_->codeToFuel(value, nodata);
}
void FuelLookup::listFuels() const { impl_->listFuels(); }
FuelSize FuelLookup::fuelToCode(const FuelType* const value) const
{
  return impl_->fuelToCode(value);
}
const FuelType* FuelLookup::operator()(const FuelSize value, const FuelSize nodata) const
{
  return codeToFuel(value, nodata);
}
set<const FuelType*> FuelLookup::usedFuels() const { return impl_->usedFuels(); }
const FuelType* FuelLookup::byName(const string_view name) const { return impl_->byName(name); }
const FuelType* FuelLookup::bySimplifiedName(const string_view name) const
{
  return impl_->bySimplifiedName(name);
}
FuelLookup::FuelLookup(const char* filename) : impl_(make_shared<FuelLookupImpl>(filename)) { }
const array<const FuelType*, NUMBER_OF_FUELS> FuelLookup::Fuels{
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
}
