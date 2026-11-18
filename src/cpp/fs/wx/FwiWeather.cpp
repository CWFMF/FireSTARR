/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FwiWeather.h"
#include "Moisture.h"
namespace fs
{
[[nodiscard]] MathSize FwiWeatherImpl::ffmcEffect() const { return ffmc_effect(ffmc); }
[[nodiscard]] MathSize FwiWeatherImpl::mcDmc() const { return mcDmcPct() / 100.0; }
[[nodiscard]] MathSize FwiWeatherImpl::mcFfmc() const { return mcFfmcPct() / 100.0; }
[[nodiscard]] MathSize FwiWeatherImpl::mcFfmcPct() const { return ffmc_to_moisture(ffmc); }
[[nodiscard]] MathSize FwiWeatherImpl::mcDmcPct() const
{
  return exp((dmc.value - 244.72) / -43.43) + 20;
}
mutex FwiWeather::mutex_ = {};
}
