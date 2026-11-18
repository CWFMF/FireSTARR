// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
#ifndef FS_FWI_REFERENCE_H
#define FS_FWI_REFERENCE_H
#include "FireWeatherIndices.h"
#include "WeatherIndices.h"
namespace fs::fwireference
{
Ffmc FFMCcalc(
  Temperature temperature,
  RelativeHumidity relative_humidity,
  Speed wind_speed,
  Precipitation rain_24hr,
  Ffmc ffmc_previous
);
Dmc DMCcalc(
  Temperature temperature,
  RelativeHumidity relative_humidity,
  Precipitation rain_24hr,
  const Dmc dmc_previous,
  const Month month,
  const Latitude latitude = DEFAULT_LATITUDE
);
Dc DCcalc(
  Temperature temperature,
  Precipitation rain_24hr,
  const Dc dc_previous,
  const Month month,
  const Latitude latitude = DEFAULT_LATITUDE
);
Isi ISIcalc(Speed wind_speed, Ffmc ffmc);
Bui BUIcalc(Dmc dmc, Dc dc);
Fwi FWIcalc(Isi isi, Bui bui);
Dsr DSRcalc(const Fwi fwi) noexcept;
}
#endif
