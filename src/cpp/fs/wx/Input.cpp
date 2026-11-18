/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Input.h"
#include "../log/Log.h"
#include "../util/Util.h"
#include "FireWeatherIndices.h"
namespace fs
{
inline MathSize stod(const string* const str) { return stod(*str); }
FwiWeather read_fwi_weather(istringstream* iss, string* str)
{
  // PREC
  getline(iss, str, ',');
  logging::extensive("PREC is {:s}", *str);
  const Precipitation prec(stod(str));
  // TEMP
  getline(iss, str, ',');
  logging::extensive("TEMP is {:s}", *str);
  const Temperature temp(stod(str));
  // RH
  getline(iss, str, ',');
  logging::extensive("RH is {:s}", *str);
  const RelativeHumidity rh(stod(str));
  // WS
  getline(iss, str, ',');
  logging::extensive("WS is {:s}", *str);
  const Speed ws(stod(str));
  // WD
  getline(iss, str, ',');
  logging::extensive("WD is {:s}", *str);
  const Direction wd{Degrees{stod(str)}};
  const Wind wind{ws, wd};
  // FIX: pretend we're checking these but the flag is unset for now
  getline(iss, str, ',');
  logging::extensive("FFMC is {:s}", *str);
  const Ffmc ffmc(stod(str));
  getline(iss, str, ',');
  logging::extensive("DMC is {:s}", *str);
  const Dmc dmc(stod(str));
  getline(iss, str, ',');
  logging::extensive("DC is {:s}", *str);
  const Dc dc(stod(str));
  getline(iss, str, ',');
  logging::extensive("ISI is {:s}", *str);
  const Isi isi{check_isi(stod(str), ws, ffmc)};
  getline(iss, str, ',');
  logging::extensive("BUI is {:s}", *str);
  const Bui bui{check_bui(stod(str), dmc, dc)};
  getline(iss, str, ',');
  logging::extensive("FWI is {:s}", *str);
  const Fwi fwi{check_fwi(stod(str), isi, bui)};
  return {temp, rh, wind, prec, ffmc, dmc, dc, isi, bui, fwi};
}
FwiWeather read_weather(istringstream* iss, string* str)
{
  // PREC
  getline(iss, str, ',');
  logging::extensive("PREC is {:s}", *str);
  const Precipitation prec(stod(str));
  // TEMP
  getline(iss, str, ',');
  logging::extensive("TEMP is {:s}", *str);
  const Temperature temp(stod(str));
  // RH
  getline(iss, str, ',');
  logging::extensive("RH is {:s}", *str);
  const RelativeHumidity rh(stod(str));
  // WS
  getline(iss, str, ',');
  logging::extensive("WS is {:s}", *str);
  const Speed ws(stod(str));
  // WD
  getline(iss, str, ',');
  logging::extensive("WD is {:s}", *str);
  const Direction wd{Degrees{stod(str)}};
  const Wind wind{ws, wd};
  return {
    {.temperature = temp, .rh = rh, .wind = wind, .prec = prec},
    Ffmc::Zero(),
    Dmc::Zero(),
    Dc::Zero(),
    Isi::Zero(),
    Bui::Zero(),
    Fwi::Zero()
  };
}
}
