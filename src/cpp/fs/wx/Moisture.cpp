/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Moisture.h"
namespace fs
{
MathSize ffmc_effect(const Ffmc ffmc) noexcept
{
  /*Eq. 1*/
  const auto mc = ffmc_to_moisture(ffmc);
  /*Eq. 25*/
  return 91.9 * exp(-0.1386 * mc) * (1 + pow(mc, 5.31) / 49300000.0);
}
constexpr auto FFMC_MOISTURE_CONSTANT = 250.0 * 59.5 / 101.0;
MathSize ffmc_to_moisture(const MathSize ffmc) noexcept
{
  return FFMC_MOISTURE_CONSTANT * (101.0 - ffmc) / (59.5 + ffmc);
}
MathSize ffmc_to_moisture(const Ffmc& ffmc) noexcept { return ffmc_to_moisture(ffmc.value); }
Ffmc moisture_to_ffmc(const MathSize m) noexcept
{
  return Ffmc{(59.5 * (250.0 - m) / (FFMC_MOISTURE_CONSTANT + m))};
}
Ffmc ffmc_from_moisture(const MathSize m) noexcept { return Ffmc(moisture_to_ffmc(m)); }
}
