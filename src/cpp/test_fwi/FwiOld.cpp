/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FwiOld.h"
#include "../fs/log/Log.h"
#include "../fs/util/Util.h"
#include "../fs/wx/Moisture.h"
#include "../fs/wx/WeatherIndices.h"
// adapted from http://www.columbia.edu/~rf2426/index_files/FWI.vba
//******************************************************************************************
//
// Description: VBA module containing functions to calculate the components of
//              the Canadian Fire Weather Index system, as described in
//
//      Van Wagner, C.E. 1987. Development and structure of the Canadian Forest Fire
//      Weather Index System. Canadian Forest Service, Ottawa, Ont. For. Tech. Rep. 35.
//      37 p.
//
//      Equation numbers from VW87 are listed throughout, to the right of the equations
//      in the code.
//
//      A more recent technical description can be found in:
//      http://www.cawcr.gov.au/publications/technicalreports/CTR_010.pdf
//
//      This module is essentially a direct C to VBA translation of Kerry Anderson's
//      fwi84.c code. The latitude adjustments were developed by Marty Alexander, and used
//      over Indonesia in the paper:
//
//      Field, R.D., Y. Wang, O. Roswintiarti, and Guswanto. A drought-based predictor of
//      recent haze events in western Indonesia. Atmospheric Environment, 38, 1869-1878,
//      2004.
//
//      A technical description of the latitude adjustments can be found in Appendix 3 of:
//      http://cfs.nrcan.gc.ca/pubwarehouse/pdfs/29152.pdf
//
//      Robert Field, robert.field@utoronto.ca
//******************************************************************************************
namespace fs::fwiold
{
constexpr auto LATITUDE_INNER = 10.0;
constexpr auto LATITUDE_MIDDLE = 30.0;
// The following two functions refer to the MEA day length adjustment 'note'.
//
//******************************************************************************************
// Function Name: DayLengthFactor
// Description: Calculates latitude/date dependent day length factor for Drought Code
// Parameters:
//      Latitude is latitude in decimal degrees of calculation location
//      Month is integer (1..12) value of month of year for calculation
//
//******************************************************************************************
static MathSize day_length_factor(const MathSize latitude, const int month) noexcept
{
  static constexpr MathSize LfN[] = {
    -1.6, -1.6, -1.6, 0.9, 3.8, 5.8, 6.4, 5.0, 2.4, 0.4, -1.6, -1.6
  };
  static constexpr MathSize LfS[] = {
    6.4, 5.0, 2.4, 0.4, -1.6, -1.6, -1.6, -1.6, -1.6, 0.9, 3.8, 5.8
  };
  //'    '/* Use Northern hemisphere numbers */
  //'   '/* something goes wrong with >= */
  if (latitude >= LATITUDE_INNER)
  {
    return LfN[month];
  }
  //'    '/* Use Equatorial numbers */
  if (LATITUDE_INNER > abs(latitude))
  {
    // HACK: why was this not 1.4?
    // return 1.39;
    return 1.4;
  }
  //'    '/* Use Southern hemisphere numbers */
  if (latitude < LATITUDE_INNER)
  {
    return LfS[month];
  }
  exit(logging::fatal("Unable to calculate DayLengthFactor"));
}
using MonthArray = array<MathSize, 12>;
static constexpr MonthArray
  DAY_LENGTH46_N{6.5, 7.5, 9.0, 12.8, 13.9, 13.9, 12.4, 10.9, 9.4, 8.0, 7.0, 6.0};
static constexpr MonthArray
  DAY_LENGTH20_N{7.9, 8.4, 8.9, 9.5, 9.9, 10.2, 10.1, 9.7, 9.1, 8.6, 8.1, 7.8};
static constexpr MonthArray
  DAY_LENGTH20_S{10.1, 9.6, 9.1, 8.5, 8.1, 7.8, 7.9, 8.3, 8.9, 9.4, 9.9, 10.2};
static constexpr MonthArray
  DAY_LENGTH40_S{11.5, 10.5, 9.2, 7.9, 6.8, 6.2, 6.5, 7.4, 8.7, 10.0, 11.2, 11.8};
//******************************************************************************************
// Function Name: DayLength
// Description: Calculates latitude/date dependent day length for DMC calculation
// Parameters:
//      Latitude is latitude in decimal degrees of calculation location
//      Month is integer (1..12) value of month of year for calculation
//
//******************************************************************************************
static MathSize day_length(const MathSize latitude, const int month) noexcept
{
  //'''/* Day Length Arrays for four different latitude ranges '*/
  //''/*
  //'    Use four ranges which respectively span:
  //'        - 90N to 33 N
  //'        - 33 N to 0
  //'        - 0 to -30
  //'        - -30 to -90
  ///
  // HACK: why was this 15 previously?
  // if ((latitude <= 15.0) && (latitude > -15.0))
  if (LATITUDE_INNER > abs(latitude))
  {
    return 9.0;
  }
  if (latitude >= LATITUDE_MIDDLE)
  {
    return DAY_LENGTH46_N.at(static_cast<size_t>(month) - 1);
  }
  if (latitude >= LATITUDE_INNER)
  {
    return DAY_LENGTH20_N.at(static_cast<size_t>(month) - 1);
  }
  if (latitude <= -LATITUDE_MIDDLE)
  {
    return DAY_LENGTH40_S.at(static_cast<size_t>(month) - 1);
  }
  if ((latitude > -LATITUDE_MIDDLE))
  {
    return DAY_LENGTH20_S.at(static_cast<size_t>(month) - 1);
  }
  exit(logging::fatal("Unable to calculate DayLength"));
}
MathSize find_m(
  const Temperature temperature,
  const RelativeHumidity rh,
  const Speed wind,
  const MathSize mo
) noexcept
{
  //'''/* 4  '*/
  const auto ed = 0.942 * pow(rh.value, 0.679) + 11.0 * exp((rh.value - 100.0) / 10.0)
                + 0.18 * (21.1 - temperature.value) * (1.0 - exp(-0.115 * rh.value));
  if (mo > ed)
  {
    //'''/* 6a '*/
    const auto ko = 0.424 * (1.0 - pow(rh.value / 100.0, 1.7))
                  + 0.0694 * sqrt(wind.value) * (1.0 - pow_int<8>(rh.value / 100.0));
    //'''/* 6b '*/
    const auto kd = ko * 0.581 * exp(0.0365 * temperature.value);
    //'''/* 8  '*/
    return ed + (mo - ed) * pow(10.0, -kd);
  }
  //'''/* 5  '*/
  const auto ew = 0.618 * pow(rh.value, 0.753) + 10.0 * exp((rh.value - 100.0) / 10.0)
                + 0.18 * (21.1 - temperature.value) * (1.0 - exp(-0.115 * rh.value));
  if (mo < ew)
  {
    //'''/* 7a '*/
    const auto kl = 0.424 * (1.0 - pow((100.0 - rh.value) / 100.0, 1.7))
                  + 0.0694 * sqrt(wind.value) * (1 - pow_int<8>((100.0 - rh.value) / 100.0));
    //'''/* 7b '*/
    const auto kw = kl * 0.581 * exp(0.0365 * temperature.value);
    //'''/* 9  '*/
    return ew - (ew - mo) * pow(10.0, -kw);
  }
  return mo;
}
//******************************************************************************************
// Function Name: FFMC
// Description: Calculates today's Fine Fuel Moisture Code
// Parameters:
//    temperature is the 12:00 LST temperature in degrees celsius
//    rh is the 12:00 LST relative humidity in %
//    wind is the 12:00 LST wind speed in kph
//    rain is the 24-hour accumulated rainfall in mm, calculated at 12:00 LST
//    ffmc_previous is the previous day's FFMC
//******************************************************************************************
Ffmc FFMCcalc(
  const Temperature temperature,
  const RelativeHumidity rh,
  const Speed wind,
  const Precipitation rain,
  const Ffmc ffmc_previous
) noexcept
{
  //'''/* 1  '*/
  auto mo = ffmc_to_moisture(ffmc_previous);
  if (rain.value > 0.5)
  {
    //'''/* 2  '*/
    const auto rf = rain.value - 0.5;
    //'''/* 3a '*/
    auto mr = mo + 42.5 * rf * (exp(-100.0 / (251.0 - mo))) * (1 - exp(-6.93 / rf));
    if (mo > 150.0)
    {
      //'''/* 3b '*/
      mr += 0.0015 * pow_int<2>(mo - 150.0) * sqrt(rf);
    }
    if (mr > 250.0)
    {
      mr = 250.0;
    }
    mo = mr;
  }
  const auto m = find_m(temperature, rh, wind, mo);
  //'''/* 10 '*/
  return Ffmc{moisture_to_ffmc(m).value};
}
//******************************************************************************************
// Function Name: DMC
// Description: Calculates today's Duff Moisture Code
// Parameters:
//    temperature is the 12:00 LST temperature in degrees celsius
//    rh is the 12:00 LST relative humidity in %
//    rain is the 24-hour accumulated rainfall in mm, calculated at 12:00 LST
//    dmc_previous is the previous day's DMC
//    latitude is the latitude in decimal degrees of the location for calculation
//    month is the month of Year (1..12) for the current day's calculations.
//******************************************************************************************
Dmc DMCcalc(
  const Temperature temperature,
  const RelativeHumidity rh,
  const Precipitation rain,
  const Dmc dmc_previous,
  const int month,
  const MathSize latitude
) noexcept
{
  auto previous = dmc_previous.value;
  if (rain.value > 1.5)
  {
    //'''/* 11  '*/
    const auto re = 0.92 * rain.value - 1.27;
    //'''/* 12  '*/
    //    const auto mo = 20.0 + exp(5.6348 - previous / 43.43)
    // Alteration to Eq. 12 to calculate more accurately
    const auto mo = 20 + 280 / exp(0.023 * previous);
    const auto b = (previous <= 33.0) ?   //'''/* 13a '*/
                     100.0 / (0.5 + 0.3 * previous)
                                      : ((previous <= 65.0) ?   //'''/* 13b '*/
                                           14.0 - 1.3 * (log(previous))
                                                            :   //'''/* 13c '*/
                                           6.2 * log(previous) - 17.2);
    //'''/* 14  '*/
    const auto mr = mo + 1000.0 * re / (48.77 + b * re);
    //'''/* 15  '*/
    //    const auto pr = 244.72 - 43.43 * log(mr - 20.0)
    // Alteration to Eq. 15 to calculate more accurately
    const auto pr = 43.43 * (5.6348 - log(mr - 20));
    previous = max(pr, 0.0);
  }
  const auto k = (temperature.value > -1.1) ? 1.894 * (temperature.value + 1.1) * (100.0 - rh.value)
                                                * day_length(latitude, month) * 0.0001
                                            : 0.0;
  //'''/* 17  '*/
  return Dmc{previous + k};
}
//******************************************************************************************
// Function Name: DC
// Description: Calculates today's Drought Code
// Parameters:
//    temperature is the 12:00 LST temperature in degrees celsius
//    rain is the 24-hour accumulated rainfall in mm, calculated at 12:00 LST
//    dc_previous is the previous day's DC
//    latitude is the latitude in decimal degrees of the location for calculation
//    month is the month of Year (1..12) for the current day's calculations.
//******************************************************************************************
Dc DCcalc(
  const Temperature temperature,
  const Precipitation rain,
  const Dc dc_previous,
  const int month,
  const MathSize latitude
) noexcept
{
  auto previous = dc_previous.value;
  if (rain.value > 2.8)
  {
    //'/* 18  */
    const auto rd = 0.83 * (rain.value) - 1.27;
    //'/* 19  */
    const auto qo = 800.0 * exp(-previous / 400.0);
    //'/* 20  */
    const auto qr = qo + 3.937 * rd;
    //'/* 21  */
    const auto dr = 400.0 * log(800.0 / qr);
    previous = (dr > 0.0) ? dr : 0.0;
  }
  const auto lf = day_length_factor(latitude, month - 1);
  //'/* 22  */
  const auto v = max(0.0, (temperature.value > -2.8) ? 0.36 * (temperature.value + 2.8) + lf : lf);
  //'/* 23  */
  const auto d = previous + 0.5 * v;
  // HACK: don't allow negative values
  return Dc{max(0.0, d)};
}
MathSize ffmc_effect(const Ffmc ffmc) noexcept
{
  //'''/* 1   '*/
  const auto mc = ffmc_to_moisture(ffmc);
  //'''/* 25  '*/
  return 91.9 * exp(-0.1386 * mc) * (1 + pow(mc, 5.31) / 49300000.0);
}
//******************************************************************************************
// Function Name: ISI
// Description: Calculates today's Initial Spread Index
// Parameters:
//    wind is the 12:00 LST wind speed in kph
//    ffmc is the current day's FFMC
//******************************************************************************************
Isi ISIcalc(const Speed wind, const Ffmc ffmc) noexcept
{
  //'''/* 24  '*/
  const auto f_wind = exp(0.05039 * wind.value);
  const auto f_f = fwiold::ffmc_effect(ffmc);
  //'''/* 26  '*/
  return Isi{0.208 * f_wind * f_f};
}
//******************************************************************************************
// Function Name: BUI
// Description: Calculates today's Buildup Index
// Parameters:
//    DMC is the current day's Duff Moisture Code
//    DC is the current day's Drought Code
//******************************************************************************************
Bui BUIcalc(const Dmc dmc, const Dc dc) noexcept
{
  if (dmc.value <= 0.4 * dc.value)
  {
    // HACK: this isn't normally part of it, but it's division by 0 without this
    if (0 == dc.value)
    {
      return Bui{0.0};
    }
    //'''/* 27a '*/
    return Bui{max(0.0, 0.8 * dmc.value * dc.value / (dmc.value + 0.4 * dc.value))};
  }
  //'''/* 27b '*/
  return Bui{max(
    0.0,
    dmc.value
      - (1.0 - 0.8 * dc.value / (dmc.value + 0.4 * dc.value))
          * (0.92 + pow(0.0114 * dmc.value, 1.7))
  )};
}
//******************************************************************************************
// Function Name: FWI
// Description: Calculates today's Fire Weather Index
// Parameters:
//    ISI is current day's ISI
//    BUI is the current day's BUI
//******************************************************************************************
Fwi FWIcalc(const Isi isi, const Bui bui) noexcept
{
  const auto f_d = (bui.value <= 80.0) ?   //'''/* 28a '*/
                     0.626 * pow(bui.value, 0.809) + 2.0
                                       :   //'''/* 28b '*/
                     1000.0 / (25.0 + 108.64 * exp(-0.023 * bui.value));
  //'''/* 29  '*/
  const auto b = 0.1 * isi.value * f_d;
  if (b > 1.0)
  {
    //'''/* 30a '*/
    return Fwi{exp(2.72 * pow(0.434 * log(b), 0.647))};
  }
  //'''/* 30b '*/
  return Fwi{b};
}
//******************************************************************************************
// Function Name: DSR
// Description: Calculates today's Daily Severity Rating
// Parameters:
//    FWI is current day's FWI
//******************************************************************************************
Dsr DSRcalc(const Fwi fwi) noexcept
{
  //'''/* 41 '*/
  return Dsr{0.0272 * pow(fwi.value, 1.77)};
}
}
