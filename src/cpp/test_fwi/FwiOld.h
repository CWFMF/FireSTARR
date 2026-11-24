/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FWI_OLD_H
#define FS_FWI_OLD_H
#include "../fs/unstable.h"
#include "../fs/wx/FireWeatherIndices.h"
#include "../fs/wx/WeatherIndices.h"
namespace fs::fwiold
{
Ffmc FFMCcalc(
  const Temperature temperature,
  const RelativeHumidity relative_humidity,
  const Speed wind_speed,
  const Precipitation rain_24hr,
  const Ffmc ffmc_previous
) noexcept;
Dmc DMCcalc(
  const Temperature temperature,
  const RelativeHumidity relative_humidity,
  const Precipitation rain_24hr,
  const Dmc dmc_previous,
  const int month,
  const MathSize latitude = DEFAULT_LATITUDE.value
) noexcept;
Dc DCcalc(
  const Temperature temperature,
  const Precipitation rain_24hr,
  const Dc dc_previous,
  const int month,
  const MathSize latitude = DEFAULT_LATITUDE.value
) noexcept;
Isi ISIcalc(const Speed wind_speed, const Ffmc ffmc) noexcept;
Bui BUIcalc(const Dmc dmc, const Dc dc) noexcept;
Fwi FWIcalc(const Isi isi, const Bui bui) noexcept;
Dsr DSRcalc(const Fwi fwi) noexcept;
}
#endif
