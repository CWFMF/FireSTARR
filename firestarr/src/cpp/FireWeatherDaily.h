/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <map>
#include <set>
#include <vector>
#include "FuelLookup.h"
#include "FWI.h"
#include "FireWeather.h"
namespace fs::wx
{
/**
 * \brief A stream of weather that gets used by a Scenario every Iteration.
 */
class FireWeatherDaily
  : public FireWeather
{
public:
  /**
   * \brief Destructor
   */
  virtual ~FireWeatherDaily() = default;
  /**
   * \brief Constructor
   * \param used_fuels set of FuelTypes that are used in the simulation
   * \param data map of Day to FwiWeather to use for weather stream
   */
  FireWeatherDaily(const set<const fuel::FuelType*>& used_fuels,
                   const map<Day, FwiWeather>& data);
  /**
   * \brief Move constructor
   * \param rhs FireWeatherDaily to move from
   */
  FireWeatherDaily(FireWeatherDaily&& rhs) = default;
  FireWeatherDaily(const FireWeatherDaily& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs FireWeatherDaily to move from
   * \return This, after assignment
   */
  FireWeatherDaily& operator=(FireWeatherDaily&& rhs) noexcept = default;
  FireWeatherDaily& operator=(const FireWeatherDaily& rhs) = delete;
};
}
