// Copyright (C) 2020  Queen's Printer for Ontario
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Last Updated 2020-04-07 <Evens, Jordan (MNRF)>

#include "stdafx.h"
#include "FuelLookup.h"
#include "FBP45.h"
#include "Log.h"
#include "Settings.h"
namespace firestarr
{
namespace fuel
{
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
static_assert(0 == INVALID_FUEL_CODE);
static InvalidFuel NULLFUEL{INVALID_FUEL_CODE, "Non-static Fuel"};
static InvalidFuel INVALID{1, "Invalid"};
static fbp::FuelC1 C1{2};
static fbp::FuelC2 C2{3};
static fbp::FuelC3 C3{4};
static fbp::FuelC4 C4{5};
static fbp::FuelC5 C5{6};
static fbp::FuelC6 C6{7};
static fbp::FuelC7 C7{8};
static fbp::FuelD1 D1{9};
static fbp::FuelD2 D2{10};
static fbp::FuelO1A O1_A{11};
static fbp::FuelO1B O1_B{12};
static fbp::FuelS1 S1{13};
static fbp::FuelS2 S2{14};
static fbp::FuelS3 S3{15};
static fbp::FuelD1D2 D1_D2{16, &D1, &D2};
static fbp::FuelM1<5> M1_05{17, "M-1 (05 PC)"};
static fbp::FuelM1<10> M1_10{18, "M-1 (10 PC)"};
static fbp::FuelM1<15> M1_15{19, "M-1 (15 PC)"};
static fbp::FuelM1<20> M1_20{20, "M-1 (20 PC)"};
static fbp::FuelM1<25> M1_25{21, "M-1 (25 PC)"};
static fbp::FuelM1<30> M1_30{22, "M-1 (30 PC)"};
static fbp::FuelM1<35> M1_35{23, "M-1 (35 PC)"};
static fbp::FuelM1<40> M1_40{24, "M-1 (40 PC)"};
static fbp::FuelM1<45> M1_45{25, "M-1 (45 PC)"};
static fbp::FuelM1<50> M1_50{26, "M-1 (50 PC)"};
static fbp::FuelM1<55> M1_55{27, "M-1 (55 PC)"};
static fbp::FuelM1<60> M1_60{28, "M-1 (60 PC)"};
static fbp::FuelM1<65> M1_65{29, "M-1 (65 PC)"};
static fbp::FuelM1<70> M1_70{30, "M-1 (70 PC)"};
static fbp::FuelM1<75> M1_75{31, "M-1 (75 PC)"};
static fbp::FuelM1<80> M1_80{32, "M-1 (80 PC)"};
static fbp::FuelM1<85> M1_85{33, "M-1 (85 PC)"};
static fbp::FuelM1<90> M1_90{34, "M-1 (90 PC)"};
static fbp::FuelM1<95> M1_95{35, "M-1 (95 PC)"};
static fbp::FuelM2<5> M2_05{17, "M-2 (05 PC)"};
static fbp::FuelM2<10> M2_10{18, "M-2 (10 PC)"};
static fbp::FuelM2<15> M2_15{19, "M-2 (15 PC)"};
static fbp::FuelM2<20> M2_20{20, "M-2 (20 PC)"};
static fbp::FuelM2<25> M2_25{21, "M-2 (25 PC)"};
static fbp::FuelM2<30> M2_30{22, "M-2 (30 PC)"};
static fbp::FuelM2<35> M2_35{23, "M-2 (35 PC)"};
static fbp::FuelM2<40> M2_40{24, "M-2 (40 PC)"};
static fbp::FuelM2<45> M2_45{25, "M-2 (45 PC)"};
static fbp::FuelM2<50> M2_50{26, "M-2 (50 PC)"};
static fbp::FuelM2<55> M2_55{27, "M-2 (55 PC)"};
static fbp::FuelM2<60> M2_60{28, "M-2 (60 PC)"};
static fbp::FuelM2<65> M2_65{29, "M-2 (65 PC)"};
static fbp::FuelM2<70> M2_70{30, "M-2 (70 PC)"};
static fbp::FuelM2<75> M2_75{31, "M-2 (75 PC)"};
static fbp::FuelM2<80> M2_80{32, "M-2 (80 PC)"};
static fbp::FuelM2<85> M2_85{33, "M-2 (85 PC)"};
static fbp::FuelM2<90> M2_90{34, "M-2 (90 PC)"};
static fbp::FuelM2<95> M2_95{35, "M-2 (95 PC)"};
static fbp::FuelM1M2<5> M1_M2_05{17, "M-1/M-2 (05 PC)", &M1_05, &M2_05};
static fbp::FuelM1M2<10> M1_M2_10{18, "M-1/M-2 (10 PC)", &M1_10, &M2_10};
static fbp::FuelM1M2<15> M1_M2_15{19, "M-1/M-2 (15 PC)", &M1_15, &M2_15};
static fbp::FuelM1M2<20> M1_M2_20{20, "M-1/M-2 (20 PC)", &M1_20, &M2_20};
static fbp::FuelM1M2<25> M1_M2_25{21, "M-1/M-2 (25 PC)", &M1_25, &M2_25};
static fbp::FuelM1M2<30> M1_M2_30{22, "M-1/M-2 (30 PC)", &M1_30, &M2_30};
static fbp::FuelM1M2<35> M1_M2_35{23, "M-1/M-2 (35 PC)", &M1_35, &M2_35};
static fbp::FuelM1M2<40> M1_M2_40{24, "M-1/M-2 (40 PC)", &M1_40, &M2_40};
static fbp::FuelM1M2<45> M1_M2_45{25, "M-1/M-2 (45 PC)", &M1_45, &M2_45};
static fbp::FuelM1M2<50> M1_M2_50{26, "M-1/M-2 (50 PC)", &M1_50, &M2_50};
static fbp::FuelM1M2<55> M1_M2_55{27, "M-1/M-2 (55 PC)", &M1_55, &M2_55};
static fbp::FuelM1M2<60> M1_M2_60{28, "M-1/M-2 (60 PC)", &M1_60, &M2_60};
static fbp::FuelM1M2<65> M1_M2_65{29, "M-1/M-2 (65 PC)", &M1_65, &M2_65};
static fbp::FuelM1M2<70> M1_M2_70{30, "M-1/M-2 (70 PC)", &M1_70, &M2_70};
static fbp::FuelM1M2<75> M1_M2_75{31, "M-1/M-2 (75 PC)", &M1_75, &M2_75};
static fbp::FuelM1M2<80> M1_M2_80{32, "M-1/M-2 (80 PC)", &M1_80, &M2_80};
static fbp::FuelM1M2<85> M1_M2_85{33, "M-1/M-2 (85 PC)", &M1_85, &M2_85};
static fbp::FuelM1M2<90> M1_M2_90{34, "M-1/M-2 (90 PC)", &M1_90, &M2_90};
static fbp::FuelM1M2<95> M1_M2_95{35, "M-1/M-2 (95 PC)", &M1_95, &M2_95};
static fbp::FuelM3<5> M3_05{36, "M-3 (05 PDF)"};
static fbp::FuelM3<10> M3_10{37, "M-3 (10 PDF)"};
static fbp::FuelM3<15> M3_15{38, "M-3 (15 PDF)"};
static fbp::FuelM3<20> M3_20{39, "M-3 (20 PDF)"};
static fbp::FuelM3<25> M3_25{40, "M-3 (25 PDF)"};
static fbp::FuelM3<30> M3_30{41, "M-3 (30 PDF)"};
static fbp::FuelM3<35> M3_35{42, "M-3 (35 PDF)"};
static fbp::FuelM3<40> M3_40{43, "M-3 (40 PDF)"};
static fbp::FuelM3<45> M3_45{44, "M-3 (45 PDF)"};
static fbp::FuelM3<50> M3_50{45, "M-3 (50 PDF)"};
static fbp::FuelM3<55> M3_55{46, "M-3 (55 PDF)"};
static fbp::FuelM3<60> M3_60{47, "M-3 (60 PDF)"};
static fbp::FuelM3<65> M3_65{48, "M-3 (65 PDF)"};
static fbp::FuelM3<70> M3_70{49, "M-3 (70 PDF)"};
static fbp::FuelM3<75> M3_75{50, "M-3 (75 PDF)"};
static fbp::FuelM3<80> M3_80{51, "M-3 (80 PDF)"};
static fbp::FuelM3<85> M3_85{52, "M-3 (85 PDF)"};
static fbp::FuelM3<90> M3_90{53, "M-3 (90 PDF)"};
static fbp::FuelM3<95> M3_95{54, "M-3 (95 PDF)"};
static fbp::FuelM3<100> M3_100{55, "M-3 (100 PDF)"};
static fbp::FuelM4<5> M4_05{36, "M-4 (05 PDF)"};
static fbp::FuelM4<10> M4_10{37, "M-4 (10 PDF)"};
static fbp::FuelM4<15> M4_15{38, "M-4 (15 PDF)"};
static fbp::FuelM4<20> M4_20{39, "M-4 (20 PDF)"};
static fbp::FuelM4<25> M4_25{40, "M-4 (25 PDF)"};
static fbp::FuelM4<30> M4_30{41, "M-4 (30 PDF)"};
static fbp::FuelM4<35> M4_35{42, "M-4 (35 PDF)"};
static fbp::FuelM4<40> M4_40{43, "M-4 (40 PDF)"};
static fbp::FuelM4<45> M4_45{44, "M-4 (45 PDF)"};
static fbp::FuelM4<50> M4_50{45, "M-4 (50 PDF)"};
static fbp::FuelM4<55> M4_55{46, "M-4 (55 PDF)"};
static fbp::FuelM4<60> M4_60{47, "M-4 (60 PDF)"};
static fbp::FuelM4<65> M4_65{48, "M-4 (65 PDF)"};
static fbp::FuelM4<70> M4_70{49, "M-4 (70 PDF)"};
static fbp::FuelM4<75> M4_75{50, "M-4 (75 PDF)"};
static fbp::FuelM4<80> M4_80{51, "M-4 (80 PDF)"};
static fbp::FuelM4<85> M4_85{52, "M-4 (85 PDF)"};
static fbp::FuelM4<90> M4_90{53, "M-4 (90 PDF)"};
static fbp::FuelM4<95> M4_95{54, "M-4 (95 PDF)"};
static fbp::FuelM4<100> M4_100{55, "M-4 (100 PDF)"};
static fbp::FuelM3M4<5> M3_M4_05{36, "M-3/M-4 (05 PDF)", &M3_05, &M4_05};
static fbp::FuelM3M4<10> M3_M4_10{37, "M-3/M-4 (10 PDF)", &M3_10, &M4_10};
static fbp::FuelM3M4<15> M3_M4_15{38, "M-3/M-4 (15 PDF)", &M3_15, &M4_15};
static fbp::FuelM3M4<20> M3_M4_20{39, "M-3/M-4 (20 PDF)", &M3_20, &M4_20};
static fbp::FuelM3M4<25> M3_M4_25{40, "M-3/M-4 (25 PDF)", &M3_25, &M4_25};
static fbp::FuelM3M4<30> M3_M4_30{41, "M-3/M-4 (30 PDF)", &M3_30, &M4_30};
static fbp::FuelM3M4<35> M3_M4_35{42, "M-3/M-4 (35 PDF)", &M3_35, &M4_35};
static fbp::FuelM3M4<40> M3_M4_40{43, "M-3/M-4 (40 PDF)", &M3_40, &M4_40};
static fbp::FuelM3M4<45> M3_M4_45{44, "M-3/M-4 (45 PDF)", &M3_45, &M4_45};
static fbp::FuelM3M4<50> M3_M4_50{45, "M-3/M-4 (50 PDF)", &M3_50, &M4_50};
static fbp::FuelM3M4<55> M3_M4_55{46, "M-3/M-4 (55 PDF)", &M3_55, &M4_55};
static fbp::FuelM3M4<60> M3_M4_60{47, "M-3/M-4 (60 PDF)", &M3_60, &M4_60};
static fbp::FuelM3M4<65> M3_M4_65{48, "M-3/M-4 (65 PDF)", &M3_65, &M4_65};
static fbp::FuelM3M4<70> M3_M4_70{49, "M-3/M-4 (70 PDF)", &M3_70, &M4_70};
static fbp::FuelM3M4<75> M3_M4_75{50, "M-3/M-4 (75 PDF)", &M3_75, &M4_75};
static fbp::FuelM3M4<80> M3_M4_80{51, "M-3/M-4 (80 PDF)", &M3_80, &M4_80};
static fbp::FuelM3M4<85> M3_M4_85{52, "M-3/M-4 (85 PDF)", &M3_85, &M4_85};
static fbp::FuelM3M4<90> M3_M4_90{53, "M-3/M-4 (90 PDF)", &M3_90, &M4_90};
static fbp::FuelM3M4<95> M3_M4_95{54, "M-3/M-4 (95 PDF)", &M3_95, &M4_95};
static fbp::FuelM3M4<100> M3_M4_100{55, "M-3/M-4 (100 PDF)", &M3_100, &M4_100};
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
    for (auto i : FuelLookup::Fuels)
    {
      emplaceFuel(i);
    }
    // HACK: use offset from base fuel type
    const auto pc = sim::Settings::defaultPercentConifer();
    logging::check_fatal(0 >= pc || 100 <= pc || (pc % 5) != 0,
                         "Invalid default percent conifer (%d)",
                         pc);
    const auto pc_offset = (static_cast<size_t>(pc) / 5) - 1;
    emplaceFuel("M-1", FuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M1_M2_05)));
    emplaceFuel("M-2", FuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M1_M2_05)));
    emplaceFuel("M-1/M-2",
                FuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M1_M2_05)));
    const auto pdf = sim::Settings::defaultPercentDeadFir();
    logging::check_fatal(0 > pdf || 100 < pdf || (pdf % 5) != 0,
                         "Invalid default percent dead fir (%d)",
                         pdf);
    const auto pdf_offset = static_cast<size_t>(pdf) / 5 - 1;
    emplaceFuel("M-3", FuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M3_M4_05)));
    emplaceFuel("M-4", FuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M3_M4_05)));
    emplaceFuel("M-3/M-4",
                FuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M3_M4_05)));
    // 0% dead fir makes these effectively D1 because of how the equations work
    emplaceFuel("M-3 (00 PDF)", &D1);
    emplaceFuel("M-4 (00 PDF)", &D1);
    emplaceFuel("M-3/M-4 (00 PDF)", &D1);
    emplaceFuel("M-1 (05 PC)", &M1_M2_05);
    emplaceFuel("M-2 (05 PC)", &M1_M2_05);
    emplaceFuel("M-1 (10 PC)", &M1_M2_10);
    emplaceFuel("M-2 (10 PC)", &M1_M2_10);
    emplaceFuel("M-1 (15 PC)", &M1_M2_15);
    emplaceFuel("M-2 (15 PC)", &M1_M2_15);
    emplaceFuel("M-1 (20 PC)", &M1_M2_20);
    emplaceFuel("M-2 (20 PC)", &M1_M2_20);
    emplaceFuel("M-1 (25 PC)", &M1_M2_25);
    emplaceFuel("M-2 (25 PC)", &M1_M2_25);
    emplaceFuel("M-1 (30 PC)", &M1_M2_30);
    emplaceFuel("M-2 (30 PC)", &M1_M2_30);
    emplaceFuel("M-1 (35 PC)", &M1_M2_35);
    emplaceFuel("M-2 (35 PC)", &M1_M2_35);
    emplaceFuel("M-1 (40 PC)", &M1_M2_40);
    emplaceFuel("M-2 (40 PC)", &M1_M2_40);
    emplaceFuel("M-1 (45 PC)", &M1_M2_45);
    emplaceFuel("M-2 (45 PC)", &M1_M2_45);
    emplaceFuel("M-1 (50 PC)", &M1_M2_50);
    emplaceFuel("M-2 (50 PC)", &M1_M2_50);
    emplaceFuel("M-1 (55 PC)", &M1_M2_55);
    emplaceFuel("M-2 (55 PC)", &M1_M2_55);
    emplaceFuel("M-1 (60 PC)", &M1_M2_60);
    emplaceFuel("M-2 (60 PC)", &M1_M2_60);
    emplaceFuel("M-1 (65 PC)", &M1_M2_65);
    emplaceFuel("M-2 (65 PC)", &M1_M2_65);
    emplaceFuel("M-1 (70 PC)", &M1_M2_70);
    emplaceFuel("M-2 (70 PC)", &M1_M2_70);
    emplaceFuel("M-1 (75 PC)", &M1_M2_75);
    emplaceFuel("M-2 (75 PC)", &M1_M2_75);
    emplaceFuel("M-1 (80 PC)", &M1_M2_80);
    emplaceFuel("M-2 (80 PC)", &M1_M2_80);
    emplaceFuel("M-1 (85 PC)", &M1_M2_85);
    emplaceFuel("M-2 (85 PC)", &M1_M2_85);
    emplaceFuel("M-1 (90 PC)", &M1_M2_90);
    emplaceFuel("M-2 (90 PC)", &M1_M2_90);
    emplaceFuel("M-1 (95 PC)", &M1_M2_95);
    emplaceFuel("M-2 (95 PC)", &M1_M2_95);
    emplaceFuel("M-3 (05 PDF)", &M3_M4_05);
    emplaceFuel("M-4 (05 PDF)", &M3_M4_05);
    emplaceFuel("M-3 (10 PDF)", &M3_M4_10);
    emplaceFuel("M-4 (10 PDF)", &M3_M4_10);
    emplaceFuel("M-3 (15 PDF)", &M3_M4_15);
    emplaceFuel("M-4 (15 PDF)", &M3_M4_15);
    emplaceFuel("M-3 (20 PDF)", &M3_M4_20);
    emplaceFuel("M-4 (20 PDF)", &M3_M4_20);
    emplaceFuel("M-3 (25 PDF)", &M3_M4_25);
    emplaceFuel("M-4 (25 PDF)", &M3_M4_25);
    emplaceFuel("M-3 (30 PDF)", &M3_M4_30);
    emplaceFuel("M-4 (30 PDF)", &M3_M4_30);
    emplaceFuel("M-3 (35 PDF)", &M3_M4_35);
    emplaceFuel("M-4 (35 PDF)", &M3_M4_35);
    emplaceFuel("M-3 (40 PDF)", &M3_M4_40);
    emplaceFuel("M-4 (40 PDF)", &M3_M4_40);
    emplaceFuel("M-3 (45 PDF)", &M3_M4_45);
    emplaceFuel("M-4 (45 PDF)", &M3_M4_45);
    emplaceFuel("M-3 (50 PDF)", &M3_M4_50);
    emplaceFuel("M-4 (50 PDF)", &M3_M4_50);
    emplaceFuel("M-3 (55 PDF)", &M3_M4_55);
    emplaceFuel("M-4 (55 PDF)", &M3_M4_55);
    emplaceFuel("M-3 (60 PDF)", &M3_M4_60);
    emplaceFuel("M-4 (60 PDF)", &M3_M4_60);
    emplaceFuel("M-3 (65 PDF)", &M3_M4_65);
    emplaceFuel("M-4 (65 PDF)", &M3_M4_65);
    emplaceFuel("M-3 (70 PDF)", &M3_M4_70);
    emplaceFuel("M-4 (70 PDF)", &M3_M4_70);
    emplaceFuel("M-3 (75 PDF)", &M3_M4_75);
    emplaceFuel("M-4 (75 PDF)", &M3_M4_75);
    emplaceFuel("M-3 (80 PDF)", &M3_M4_80);
    emplaceFuel("M-4 (80 PDF)", &M3_M4_80);
    emplaceFuel("M-3 (85 PDF)", &M3_M4_85);
    emplaceFuel("M-4 (85 PDF)", &M3_M4_85);
    emplaceFuel("M-3 (90 PDF)", &M3_M4_90);
    emplaceFuel("M-4 (90 PDF)", &M3_M4_90);
    emplaceFuel("M-3 (95 PDF)", &M3_M4_95);
    emplaceFuel("M-4 (95 PDF)", &M3_M4_95);
    emplaceFuel("M-3 (100 PDF)", &M3_M4_100);
    emplaceFuel("M-4 (100 PDF)", &M3_M4_100);
    ifstream in;
    in.open(filename);
    bool read_ok = false;
    if (in.is_open())
    {
      string str;
      logging::info("Reading fuel lookup table from '%s'", filename);
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
          logging::debug("Fuel %s has code %d", fuel.c_str(), value);
          const auto by_name = fuel_by_name_.find(str);
          if (by_name != fuel_by_name_.end())
          {
            fuel_types_->at(value) = (*by_name).second;
            if (DEFAULT_TYPES.at(name) != fuel
                || "Not Available" == name
                || "Non-fuel" == name
                || "Unclassified" == name
                || "Urban" == name
                || "Unknown" == name
                || "Vegetated Non-Fuel" == name)
            {
              logging::note("Fuel '%s' is treated like '%s'", name.c_str(), fuel.c_str());
            }
          }
          else
          {
            logging::warning("Unknown fuel type '%s' in fuel lookup table", str.c_str());
            fuel_bad_values_.emplace(value, str);
          }
          read_ok = true;
        }
      }
      in.close();
    }
    if (!read_ok)
    {
      logging::fatal("Unable to read file %s", filename);
    }
  }
  ~FuelLookupImpl()
  {
    delete fuel_types_;
  }
  /**
   * \brief Put fuel into lookup table based on name
   * \param fuel FuelType to put into lookup table based on its name
   */
  void emplaceFuel(const FuelType* fuel)
  {
    emplaceFuel(FuelType::safeName(fuel), fuel);
  }
  /**
   * \brief Put fuel into lookup table based on name
   * \param name Name to use for FuelType in lookup table
   * \param fuel FuelType to put into lookup table
   */
  void emplaceFuel(const string& name, const FuelType* fuel)
  {
    fuel_by_name_.emplace(name, fuel);
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
  const FuelType* intToFuel(const int value, const int nodata) const
  {
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
   * \brief Look up a FuelType based on the given name
   * \param name Name of the fuel to find
   * \return FuelType based on the given name
   */
  const FuelType* byName(const string& name) const
  {
    const auto seek = fuel_by_name_.find(name);
    if (seek != fuel_by_name_.end())
    {
      return seek->second;
    }
    return nullptr;
  }
private:
  /**
   * \brief Array of all possible fuel types
   */
  array<const FuelType*, numeric_limits<FuelSize>::max()>* fuel_types_;
  /**
   * \brief Map of fuel name to FuelType
   */
  unordered_map<string, const FuelType*> fuel_by_name_{};
  /**
   * \brief Map of fuel name to whether or not it is used in this simulation
   */
  mutable unordered_map<string, bool> used_by_name_{};
  /**
   * \brief Codes from input .lut that were for fuel types that are not implemented
   */
  unordered_map<FuelSize, string> fuel_bad_values_{};
};
const FuelType* FuelLookup::intToFuel(const int value, const int nodata) const
{
  return impl_->intToFuel(value, nodata);
}
const FuelType* FuelLookup::operator()(const int value, const int nodata) const
{
  return intToFuel(value, nodata);
}
set<const FuelType*> FuelLookup::usedFuels() const
{
  return impl_->usedFuels();
}
const FuelType* FuelLookup::byName(const string& name) const
{
  return impl_->byName(name);
}
FuelLookup::FuelLookup(const char* filename)
  : impl_(make_shared<FuelLookupImpl>(filename))
{
}
const array<const FuelType*, NUMBER_OF_FUELS> FuelLookup::Fuels{
  &NULLFUEL,
  &INVALID,
  &C1,
  &C2,
  &C3,
  &C4,
  &C5,
  &C6,
  &C7,
  &D1,
  &D2,
  &O1_A,
  &O1_B,
  &S1,
  &S2,
  &S3,
  &D1_D2,
  &M1_M2_05,
  &M1_M2_10,
  &M1_M2_15,
  &M1_M2_20,
  &M1_M2_25,
  &M1_M2_30,
  &M1_M2_35,
  &M1_M2_40,
  &M1_M2_45,
  &M1_M2_50,
  &M1_M2_55,
  &M1_M2_60,
  &M1_M2_65,
  &M1_M2_70,
  &M1_M2_75,
  &M1_M2_80,
  &M1_M2_85,
  &M1_M2_90,
  &M1_M2_95,
  &M3_M4_05,
  &M3_M4_10,
  &M3_M4_15,
  &M3_M4_20,
  &M3_M4_25,
  &M3_M4_30,
  &M3_M4_35,
  &M3_M4_40,
  &M3_M4_45,
  &M3_M4_50,
  &M3_M4_55,
  &M3_M4_60,
  &M3_M4_65,
  &M3_M4_70,
  &M3_M4_75,
  &M3_M4_80,
  &M3_M4_85,
  &M3_M4_90,
  &M3_M4_95,
  &M3_M4_100,
};
}
}
