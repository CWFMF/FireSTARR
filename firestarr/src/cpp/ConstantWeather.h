/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "FireWeather.h"

namespace fs::sim
{
static vector<const wx::FwiWeather*>* make_constant_weather(const wx::Dc& dc,
                                                            const wx::Dmc& dmc,
                                                            const wx::Ffmc& ffmc,
                                                            const wx::Wind& wind)
{
  static constexpr wx::Temperature TEMP(20.0);
  static constexpr wx::RelativeHumidity RH(30.0);
  static constexpr wx::Precipitation PREC(0.0);
  const auto bui = wx::Bui(dmc, dc);
  auto wx = new vector<const wx::FwiWeather*>{static_cast<size_t>(YEAR_HOURS)};
  std::generate(wx->begin(),
                wx->end(),
                [&wind, &ffmc, &dmc, &dc, &bui]() {
                  return make_unique<wx::FwiWeather>(
                           TEMP,
                           RH,
                           wind,
                           PREC,
                           ffmc,
                           dmc,
                           dc,
                           wx::Isi(wind.speed(), ffmc),
                           bui,
                           wx::Fwi(wx::Isi(wind.speed(), ffmc), bui))
                    .release();
                });
  return wx;
}
/**
 * \brief A FireWeather stream with the same value for every date and time.
 */
class ConstantWeather final
  : public wx::FireWeather
{
public:
  ~ConstantWeather() override = default;
  ConstantWeather(const ConstantWeather& rhs) = delete;
  ConstantWeather(ConstantWeather&& rhs) = delete;
  ConstantWeather& operator=(const ConstantWeather& rhs) = delete;
  ConstantWeather& operator=(ConstantWeather&& rhs) = delete;
  /**
   * \brief A Constant weather stream with only one possible fuel
   * \param fuel Fuel to use
   * \param start_date Start date for stream
   * \param dc Drought Code
   * \param bui Build-up Index
   * \param dmc Duff Moisture Code
   * \param ffmc Fine Fuel Moisture Code
   * \param wind Wind
   */
  ConstantWeather(const fuel::FuelType* fuel,
                  const Day start_date,
                  const wx::Dc& dc,
                  const wx::Dmc& dmc,
                  const wx::Ffmc& ffmc,
                  const wx::Wind& wind)
    : ConstantWeather(set<const fuel::FuelType*>{fuel},
                      start_date,
                      dc,
                      dmc,
                      ffmc,
                      wind)
  {
  }
  ConstantWeather(const set<const fuel::FuelType*>& used_fuels,
                  const Day start_date,
                  const wx::Dc& dc,
                  const wx::Dmc& dmc,
                  const wx::Ffmc& ffmc,
                  const wx::Wind& wind)
    : FireWeather(used_fuels,
                  start_date,
                  MAX_DAYS - 1,
                  make_constant_weather(dc, dmc, ffmc, wind))
  {
  }
};
}
