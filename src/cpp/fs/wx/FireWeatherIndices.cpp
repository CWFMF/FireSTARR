/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FireWeatherIndices.h"
#include "FwiReference.h"
#include "WeatherIndices.h"
// #define CHECK_CALCULATION 1
#ifndef DEBUG_FWI_WEATHER
#undef CHECK_CALCULATION
#endif
#define CHECK_EPSILON 0.1
#define USE_GIVEN
namespace fs
{
Ffmc::Ffmc(
  const Temperature temperature,
  const RelativeHumidity rh,
  const Speed wind,
  const Precipitation rain,
  const Ffmc ffmc_previous
) noexcept
  : Ffmc{fwireference::FFMCcalc(temperature, rh, wind, rain, ffmc_previous)}
{ }
Dmc::Dmc(
  const Temperature temperature,
  const RelativeHumidity rh,
  const Precipitation rain,
  const Dmc dmc_previous,
  const Month month,
  const Latitude latitude
) noexcept
  : Dmc{fwireference::DMCcalc(temperature, rh, rain, dmc_previous, month, latitude)}
{ }
Dmc::Dmc(
  const Temperature temperature,
  const RelativeHumidity rh,
  const Precipitation prec,
  const Dmc dmc_previous,
  const int month,
  const MathSize latitude
) noexcept
  : Dmc{temperature, rh, prec, dmc_previous, Month::from_ordinal(month), Latitude{latitude}}
{ }
Dc::Dc(
  const Temperature temperature,
  const Precipitation rain,
  const Dc dc_previous,
  const Month month,
  const Latitude latitude
) noexcept
  : Dc{fwireference::DCcalc(temperature, rain, dc_previous, month, latitude)}
{ }
Dc::Dc(
  const Temperature temperature,
  const Precipitation prec,
  const Dc dc_previous,
  const int month,
  const MathSize latitude
) noexcept
  : Dc{temperature, prec, dc_previous, Month::from_ordinal(month), Latitude{latitude}}
{ }
Isi::Isi(const Speed wind, const Ffmc ffmc) noexcept : Isi{fwireference::ISIcalc(wind, ffmc)} { }
Bui::Bui(const Dmc dmc, const Dc dc) noexcept : Bui{fwireference::BUIcalc(dmc, dc)} { }
Fwi::Fwi(const Isi isi, const Bui bui) noexcept : Fwi{fwireference::FWIcalc(isi, bui)} { }
Dsr::Dsr(const Fwi fwi) noexcept : Dsr{fwireference::DSRcalc(fwi)} { }
Isi check_isi(
  const MathSize
#if defined(CHECK_CALCULATION) | defined(USE_GIVEN)
    value
#endif
  ,
  const Speed&
#if defined(CHECK_CALCULATION) | !defined(USE_GIVEN)
    wind
#endif
  ,
  const Ffmc&
#if defined(CHECK_CALCULATION) | !defined(USE_GIVEN)
    ffmc
#endif
) noexcept
#ifdef USE_GIVEN
{
  const Isi isi{value};
#ifdef CHECK_CALCULATION
  const auto cmp = Isi(wind, ffmc).value;
#endif
#else
{
  const auto isi = calculate_isi(wind, ffmc);
#ifdef CHECK_CALCULATION
  const auto cmp = value;
#endif
#endif
#ifdef CHECK_CALCULATION
  logging::check_fatal(abs(isi.value - cmp) >= CHECK_EPSILON, [&]() {
    return std::format(
      "ISI is incorrect {:f}, {:f} => {:f} not {:f}", wind.value, ffmc.value, isi.value, cmp
    );
  });
#endif
  return isi;
}
Bui check_bui(
  MathSize
#if defined(CHECK_CALCULATION) | defined(USE_GIVEN)
    value
#endif
  ,
  const Dmc&
#if defined(CHECK_CALCULATION) | !defined(USE_GIVEN)
    dmc
#endif
  ,
  const Dc&
#if defined(CHECK_CALCULATION) | !defined(USE_GIVEN)
    dc
#endif
) noexcept
#ifdef USE_GIVEN
{
  const Bui bui{value};
#ifdef CHECK_CALCULATION
  const auto cmp = calculate_bui(dmc, dc).value;
#endif
#else
{
  const auto bui = calculate_bui(dmc, dc);
#ifdef CHECK_CALCULATION
  const auto cmp = value;
#endif
#endif
#ifdef CHECK_CALCULATION
  logging::check_fatal(abs(bui.value - cmp) >= CHECK_EPSILON, [&]() {
    return std::format(
      "BUI is incorrect {:f}, {:f} => {:f} not {:f}", dmc.value, dc.value, bui.value, cmp
    );
  });
#endif
  return bui;
}
Fwi check_fwi(
  MathSize
#if defined(CHECK_CALCULATION) | defined(USE_GIVEN)
    value
#endif
  ,
  const Isi&
#if defined(CHECK_CALCULATION) | !defined(USE_GIVEN)
    isi
#endif
  ,
  const Bui&
#if defined(CHECK_CALCULATION) | !defined(USE_GIVEN)
    bui
#endif
) noexcept
#ifdef USE_GIVEN
{
  const Fwi fwi{value};
#ifdef CHECK_CALCULATION
  const auto cmp = calculate_fwi(isi, bui).value;
#endif
#else
{
  const auto fwi = calculate_fwi(isi, bui);
#ifdef CHECK_CALCULATION
  const auto cmp = value;
#endif
#endif
#ifdef CHECK_CALCULATION
  logging::check_fatal(abs(fwi.value - cmp) >= CHECK_EPSILON, [&]() {
    return std::format(
      "FWI is incorrect {:f}, {:f} => {:f} not {:f}", isi.value, bui.value, fwi.value, cmp
    );
  });
#endif
  return fwi;
}
}
