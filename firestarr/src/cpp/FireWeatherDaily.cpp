/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "FireWeatherDaily.h"
#include "FuelType.h"
namespace fs::wx
{
/*!
 * \page weather Hourly fire and weather indices
 *
 * Generally, indices are constant for all cells for the day in each simulation.
 * The exceptions to this are Wind and FFMC, and by extension, ISI and FWI.
 *
 * Hourly wind is calculated based on a simple statistical analysis, with wind
 * for each hour being a proportion of the daily wind.
 *
 * FFMC is calculated using the method described in
 * https://www.for.gov.bc.ca/hfd/pubs/Docs/Frr/FRR245.pdf
 */
// adjust based on statistical analysis of hourly wind
static array<MathSize, DAY_HOURS> BY_HOUR = {
  .570,
  .565,
  .563,
  .563,
  .564,
  .581,
  .642,
  .725,
  .808,
  .880,
  .936,
  .977,
  1,
  1.008,
  .999,
  .973,
  .915,
  .831,
  .724,
  .631,
  .593,
  .586,
  .584,
  .579};
inline MathSize wind_speed_adjustment(const int hour) noexcept
{
  return BY_HOUR.at(static_cast<size_t>(hour));
}
inline Ffmc ffmc_1200(const MathSize x,
                      const MathSize x_sq,
                      const MathSize x_cu,
                      const MathSize rt_x,
                      const MathSize exp_neg_x) noexcept
{
  if (x < 21)
  {
    constexpr auto a = 1.460075956;
    constexpr auto b = -0.00039079;
    constexpr auto c = 0.28156683;
    constexpr auto d = -0.00153983;
    constexpr auto e = -0.01282069;
    return ffmc_from_moisture(pow((a + c * x + e * x_sq) / (1 + b * x + d * x_sq), 2));
  }
  constexpr auto a = -60.0581786;
  constexpr auto b = -0.79226507;
  constexpr auto c = 1.04936e-05;
  constexpr auto d = 24.04228773;
  constexpr auto e = -4.7906e+09;
  return ffmc_from_moisture(a + b * x + c * x_cu + d * rt_x + e * exp_neg_x);
}
inline Ffmc ffmc_1300(const MathSize x,
                      const MathSize x_sq,
                      const MathSize x_cu,
                      const MathSize rt_x,
                      const MathSize ln_x) noexcept
{
  if (x < 22)
  {
    constexpr auto a = 1.255216373;
    constexpr auto b = 0.022921707;
    constexpr auto c = 0.35809518;
    constexpr auto d = -0.00333111;
    constexpr auto e = -0.01642423;
    constexpr auto f = 3.05664e-05;
    return ffmc_from_moisture(
      pow((a + c * x + e * x_sq) / (1 + b * x + d * x_sq + f * x_cu), 2));
  }
  constexpr auto a = 806.4657627;
  constexpr auto b = -1.49162346;
  constexpr auto c = 0.000887319;
  constexpr auto d = -11465.7458;
  constexpr auto e = 12093.7804;
  return ffmc_from_moisture(a + b * x + c * x_sq * ln_x + d / rt_x + e * ln_x / x);
}
inline Ffmc ffmc_1400(const MathSize x,
                      const MathSize x_sq,
                      const MathSize rt_x,
                      const MathSize ln_x,
                      const MathSize exp_x) noexcept
{
  if (x < 23)
  {
    constexpr auto a = 0.908217387;
    constexpr auto b = 0.989724752;
    constexpr auto c = 0.001041606;
    constexpr auto d = 4.634e-11;
    constexpr auto e = -0.00558197;
    return ffmc_from_moisture(a + b * x + c * x_sq * rt_x + d * exp_x + e * ln_x);
  }
  constexpr auto a = 6403.107753;
  constexpr auto b = 352.7042531;
  constexpr auto c = 873.3642944;
  constexpr auto d = -3766.49257;
  constexpr auto e = 3580.933366;
  return ffmc_from_moisture(a + b * x + c * rt_x * ln_x + d * x / ln_x + e / x_sq);
}
inline Ffmc ffmc_1500(const MathSize x,
                      const MathSize x_sq,
                      const MathSize x_cu,
                      const MathSize rt_x,
                      const MathSize ln_x) noexcept
{
  if (x < 23)
  {
    constexpr auto a = 0.248711327;
    constexpr auto b = 0.9000214139;
    constexpr auto c = 0.965899432;
    constexpr auto d = 0.007692506;
    constexpr auto e = -0.00030317;
    constexpr auto f = 1.12165e-05;
    return ffmc_from_moisture(
      sqrt(a + b * x + c * x_sq + d * x_cu + e * x_sq * x_sq + f * x_sq * x_cu));
  }
  constexpr auto a = 3201.553847;
  constexpr auto b = 176.852125;
  constexpr auto c = 436.6821439;
  constexpr auto d = -1883.24627;
  constexpr auto e = 1790.467302;
  return ffmc_from_moisture(a + b * x + c * rt_x * ln_x + d * x / ln_x + e / x_sq);
}
inline Ffmc ffmc_1700(const MathSize x,
                      const MathSize x_sq,
                      const MathSize rt_x,
                      const MathSize ln_x,
                      const MathSize exp_neg_x) noexcept
{
  if (x < 40)
  {
    constexpr auto a = 0.357837756;
    constexpr auto b = 1.043214753;
    constexpr auto c = -0.0013703;
    constexpr auto d = -8.5092e-05;
    constexpr auto e = 0.158059188;
    return ffmc_from_moisture(a + b * x + c * x_sq + d * x_sq * rt_x + e * exp_neg_x);
  }
  constexpr auto a = 2776.473019;
  constexpr auto b = 153.8288088;
  constexpr auto c = -0.0001011;
  constexpr auto d = 371.9483315;
  constexpr auto e = -1620.09304;
  return ffmc_from_moisture(a + b * x + c * x_sq * rt_x + d * rt_x * ln_x + e * x / ln_x);
}
inline Ffmc ffmc_1800(const MathSize x,
                      const MathSize x_sq,
                      const MathSize x_cu,
                      const MathSize rt_x,
                      const MathSize ln_x) noexcept
{
  if (x < 40)
  {
    constexpr auto a = 1.071980333;
    constexpr auto b = 1.36047785;
    constexpr auto c = 1.201854444;
    constexpr auto d = -0.00827306;
    return ffmc_from_moisture(sqrt(a + b * x + c * x_sq + d * x_cu));
  }
  constexpr auto a = 5552.947643;
  constexpr auto b = 306.6577058;
  constexpr auto c = -0.00020219;
  constexpr auto d = 743.89688;
  constexpr auto e = -3240.18702;
  return ffmc_from_moisture(a + b * x + c * x_sq * rt_x + d * rt_x * ln_x + e * x / ln_x);
}
inline Ffmc ffmc_1900(const MathSize x,
                      const MathSize x_sq,
                      const MathSize rt_x,
                      const MathSize exp_x,
                      const MathSize exp_neg_x) noexcept
{
  if (x < 42)
  {
    constexpr auto a = 1.948509314;
    constexpr auto b = 1.124895722;
    constexpr auto c = -0.00510068;
    constexpr auto d = 8.90555e-20;
    constexpr auto e = 0.262028658;
    return ffmc_from_moisture(a + b * x + c * x_sq + d * exp_x + e * exp_neg_x);
  }
  constexpr auto a = 28.7672909;
  constexpr auto b = -1.51195157;
  constexpr auto c = 0.421751405;
  constexpr auto d = -0.02633183;
  constexpr auto e = 0.000585907;
  return ffmc_from_moisture(a + b * x + c * x * rt_x + d * x_sq + e * x_sq * rt_x);
}
inline Ffmc ffmc_2000(const MathSize x,
                      const MathSize x_sq,
                      const MathSize x_cu,
                      const MathSize rt_x,
                      const MathSize ln_x,
                      const MathSize exp_neg_x) noexcept
{
  if (x < 49)
  {
    constexpr auto a = 3.367449306;
    constexpr auto b = 1.0839743;
    constexpr auto c = 0.007668483;
    constexpr auto d = -0.00361458;
    constexpr auto e = 0.000267591;
    return ffmc_from_moisture(a + b * x + c * x_sq + d * x_sq * rt_x + e * x_cu);
  }
  constexpr auto a = -111.658439;
  constexpr auto b = 1.238144219;
  constexpr auto c = -1.74e-06;
  constexpr auto d = 379.1717488;
  constexpr auto e = -5.512e+20;
  return ffmc_from_moisture(a + b * x + c * x_cu + d / ln_x + e * exp_neg_x);
}
inline Ffmc ffmc_0600_high(const MathSize x) noexcept
{
  // default: for unknown or RH > 87
  constexpr auto a = 14.89281073;
  constexpr auto b = 194.5261398;
  constexpr auto c = 2159.088828;
  constexpr auto d = 2.390534289;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0700_high(const MathSize x) noexcept
{
  // default: for unknown or RH > 77
  constexpr auto a = 12.52268635;
  constexpr auto b = 160.3933412;
  constexpr auto c = 1308.435221;
  constexpr auto d = 2.26945513;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0800_high(const MathSize x) noexcept
{
  // default: for unknown or RH > 67
  constexpr auto a = 10.21004191;
  constexpr auto b = 136.7485497;
  constexpr auto c = 848.3773713;
  constexpr auto d = 2.154869886;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0900_high(const MathSize x) noexcept
{
  // default: for unknown or RH > 62
  constexpr auto a = 9.099751897;
  constexpr auto b = 127.608943;
  constexpr auto c = 1192.457539;
  constexpr auto d = 2.288739471;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_1000_high(const MathSize x) noexcept
{
  // default: for unknown or RH > 57
  constexpr auto a = 7.891852885;
  constexpr auto b = 126.9570677;
  constexpr auto c = 2357.682971;
  constexpr auto d = 2.538559055;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_1100_high(const MathSize ln_x, const MathSize ln_x_sq) noexcept
{
  // default: for unknown or RH > 54.5
  constexpr auto a = 7.934004974;
  constexpr auto b = -0.2113458;
  constexpr auto c = -0.29835869;
  constexpr auto d = 0.015806934;
  constexpr auto e = 0.590134367;
  return ffmc_from_moisture((a + c * ln_x + e * ln_x_sq) / (1 + b * ln_x + d * ln_x_sq));
}
inline Ffmc ffmc_0600_med(const MathSize x) noexcept
{
  // default: 68 <= RH <= 87
  constexpr auto a = 11.80584752;
  constexpr auto b = 145.1618675;
  constexpr auto c = 1610.269345;
  constexpr auto d = 2.412647414;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0700_med(const MathSize x) noexcept
{
  // default: 58 <= RH <= 77
  constexpr auto a = 10.62087345;
  constexpr auto b = 120.3071748;
  constexpr auto c = 843.7712567;
  constexpr auto d = 2.143231971;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0800_med(const MathSize x) noexcept
{
  // default: 48 <= RH <= 67
  constexpr auto a = 9.179219105;
  constexpr auto b = 105.6311973;
  constexpr auto c = 547.1226761;
  constexpr auto d = 1.946001003;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0900_med(const MathSize x) noexcept
{
  // default: 43 <= RH <= 62
  constexpr auto a = 6.381382418;
  constexpr auto b = 88.54320781;
  constexpr auto c = 544.0978144;
  constexpr auto d = 2.000706808;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_1000_med(const MathSize x) noexcept
{
  // default: 38 <= RH <= 57
  constexpr auto a = 3.497497088;
  constexpr auto b = 71.24103374;
  constexpr auto c = 525.2068553;
  constexpr auto d = 2.010941812;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_1100_med(const MathSize x) noexcept
{
  // default: 35.5 <= RH <= 54.5
  constexpr auto a = 0.514536459;
  constexpr auto b = 53.63085254;
  constexpr auto c = 461.9583952;
  constexpr auto d = 2.149631748;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0600_low(const MathSize x) noexcept
{
  // default: RH < 68
  constexpr auto a = 6.966628145;
  constexpr auto b = 65.41928741;
  constexpr auto c = 192.8242799;
  constexpr auto d = 1.748892433;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0700_low(const MathSize x) noexcept
{
  // default: RH < 58
  constexpr auto a = 6.221403215;
  constexpr auto b = 61.83553856;
  constexpr auto c = 216.2009556;
  constexpr auto d = 1.812026562;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0800_low(const MathSize x) noexcept
{
  // default: RH < 48
  constexpr auto a = 5.454482668;
  constexpr auto b = 58.64610176;
  constexpr auto c = 253.0830911;
  constexpr auto d = 1.896023728;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_0900_low(const MathSize x) noexcept
{
  // default: RH < 43
  constexpr auto a = 3.966946509;
  constexpr auto b = 47.66100216;
  constexpr auto c = 206.2626505;
  constexpr auto d = 1.814962092;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_1000_low(const MathSize x) noexcept
{
  // default: RH < 38
  constexpr auto a = 2.509991705;
  constexpr auto b = 37.42399135;
  constexpr auto c = 161.7254088;
  constexpr auto d = 1.710574764;
  return ffmc_from_moisture(a + b * exp(-0.5 * pow(log(x / c) / d, 2)));
}
inline Ffmc ffmc_1100_low(const MathSize ln_x, const MathSize ln_x_sq) noexcept
{
  // default: for RH < 35.5
  constexpr auto a = 1.291826916;
  constexpr auto b = -0.38168658;
  constexpr auto c = 0.15814773;
  constexpr auto d = 0.051353647;
  constexpr auto e = 0.356051255;
  return ffmc_from_moisture((a + c * ln_x + e * ln_x_sq) / (1 + b * ln_x + d * ln_x_sq));
}
static const FwiWeather* make_wx(const Speed& speed,
                                 const FwiWeather& wx,
                                 const Ffmc& ffmc,
                                 const int hour)
{
  static set<FwiWeather> all_weather{};
  // HACK: assign rain to noon only
  const FwiWeather result(wx.temp(),
                          wx.rh(),
                          Wind(wx.wind().direction(), speed),
                          12 == hour ? wx.prec() : Precipitation::Zero,
                          ffmc,
                          wx.dmc(),
                          wx.dc());
  const auto& wx_inserted = all_weather.insert(result);
  // doesn't matter if was already there or just inserted
  return &(*(wx_inserted.first));
}
static const FwiWeather* make_wx(const FwiWeather& wx_wind,
                                 const FwiWeather& wx,
                                 const Ffmc& ffmc,
                                 const int hour)
{
  return make_wx(Speed(wx_wind.wind().speed().asValue() * wind_speed_adjustment(hour)),
                 wx,
                 ffmc,
                 hour);
}
static const FwiWeather* make_wx(const FwiWeather& wx, const Ffmc& ffmc, const int hour)
{
  return make_wx(wx, wx, ffmc, hour);
}
unique_ptr<vector<const FwiWeather*>> make_vector(map<Day, FwiWeather> data)
{
  const auto min_date = data.begin()->first;
  const auto max_date = data.rbegin()->first;
  auto r = make_unique<vector<const FwiWeather*>>((max_date - min_date + 2) * DAY_HOURS);
  // HACK: just approximate last day
  for (const auto& kv : data)
  {
    const auto day = kv.first;
    // use first day's weather for min date instead of all 0's
    const auto& wx = (day == min_date ? data.at(day + 1) : data.at(day));
    const auto x = wx.mcFfmcPct();
    const auto x_sq = x * x;
    const auto x_cu = x * x * x;
    const auto rt_x = sqrt(x);
    const auto ln_x = log(x);
    const auto exp_x = exp(x);
    const auto exp_neg_x = exp(-x);
    const auto add_wx =
      [&r, &day, &wx, &min_date](const int hour, const Ffmc& ffmc) {
        r->at(util::time_index(day, hour, min_date)) = make_wx(wx, ffmc, hour);
      };
    add_wx(12, ffmc_1200(x, x_sq, x_cu, rt_x, exp_neg_x));
    add_wx(13, ffmc_1300(x, x_sq, x_cu, rt_x, ln_x));
    add_wx(14, ffmc_1400(x, x_sq, rt_x, ln_x, exp_x));
    add_wx(15, ffmc_1500(x, x_sq, x_cu, rt_x, ln_x));
    add_wx(16, wx.ffmc());
    add_wx(17, ffmc_1700(x, x_sq, rt_x, ln_x, exp_neg_x));
    add_wx(18, ffmc_1800(x, x_sq, x_cu, rt_x, ln_x));
    add_wx(19, ffmc_1900(x, x_sq, rt_x, exp_x, exp_neg_x));
    add_wx(20, ffmc_2000(x, x_sq, x_cu, rt_x, ln_x, exp_neg_x));
  }
  // just use high curve for last day
  const auto& wx_last = data.at(max_date);
  const auto x_last = wx_last.mcFfmcPct();
  const auto add_last =
    [&r, &max_date, &min_date, &wx_last](const int hour, const Ffmc& ffmc) {
      r->at(util::time_index(max_date + 1, hour, min_date)) = make_wx(wx_last, ffmc, hour);
    };
  add_last(6, ffmc_0600_high(x_last));
  add_last(7, ffmc_0700_high(x_last));
  add_last(8, ffmc_0800_high(x_last));
  add_last(9, ffmc_0900_high(x_last));
  add_last(10, ffmc_1000_high(x_last));
  add_last(11, ffmc_1100_high(log(x_last), pow(log(x_last), 2)));
  // need to look at 1200 for tomorrow to figure out if this matches for today
  for (auto day = static_cast<Day>(max_date - 1); day >= min_date; --day)
  {
    // use first day's weather for min date instead of all 0's
    const auto& wx = (day == min_date ? data.at(day + 1) : data.at(day));
    // make sure we use tomorrow for the wind after midnight
    const auto& wx_wind = data.at(static_cast<size_t>(day + 1));
    const auto x = wx.mcFfmcPct();
    const auto ln_x = log(x);
    const auto ln_x_sq = ln_x * ln_x;
    const auto& at_1200 = r->at(util::time_index(day + 1, 12, min_date))->ffmc();
    // figure out which is the closest match and use that curve
    const auto at_1100_high = ffmc_1100_high(ln_x, ln_x_sq);
    const auto at_1100_med = ffmc_1100_med(x);
    const auto at_1100_low = ffmc_1100_low(ln_x, ln_x_sq);
    const auto for1200 = at_1200.asValue();
    const auto for1100_high = at_1100_high.asValue();
    const auto for1100_med = at_1100_med.asValue();
    const auto for1100_low = at_1100_low.asValue();
    const auto diff_high = abs(for1200 - for1100_high);
    const auto diff_med = abs(for1200 - for1100_med);
    const auto diff_low = abs(for1200 - for1100_low);
    const auto add_wx =
      [&r, &day, &wx_wind, &wx, &min_date](const int hour, const Ffmc& ffmc) {
        r->at(util::time_index(day + 1, hour, min_date)) = make_wx(wx_wind, wx, ffmc, hour);
      };
    // don't want to have 1100 be higher than 1200 but maybe that can happen
    if (for1200 >= for1100_low && diff_low <= diff_med && diff_low <= diff_high)
    {
      // note("low RH");
      add_wx(6, ffmc_0600_low(x));
      add_wx(7, ffmc_0700_low(x));
      add_wx(8, ffmc_0800_low(x));
      add_wx(9, ffmc_0900_low(x));
      add_wx(10, ffmc_1000_low(x));
      add_wx(11, at_1100_low);
    }
    else if (for1200 >= for1100_med && diff_med <= diff_high && diff_med <= diff_low)
    {
      // note("med RH");
      add_wx(6, ffmc_0600_med(x));
      add_wx(7, ffmc_0700_med(x));
      add_wx(8, ffmc_0800_med(x));
      add_wx(9, ffmc_0900_med(x));
      add_wx(10, ffmc_1000_med(x));
      add_wx(11, at_1100_med);
    }
    else
    {
      // note("high RH");
      add_wx(6, ffmc_0600_high(x));
      add_wx(7, ffmc_0700_high(x));
      add_wx(8, ffmc_0800_high(x));
      add_wx(9, ffmc_0900_high(x));
      add_wx(10, ffmc_1000_high(x));
      add_wx(11, at_1100_high);
    }
  }
  for (auto day = static_cast<Day>(max_date); day >= min_date; --day)
  {
    // use first day's weather for min date instead of all 0's
    const auto& wx = (day == min_date ? data.at(day + 1) : data.at(day));
    const auto ffmc_at_0600 = r->at(util::time_index(day + 1, 6, min_date))->ffmc().asValue();
    const auto ffmc_at_2000 = r->at(util::time_index(day, 20, min_date))->ffmc().asValue();
    // need linear interpolation between 2000 and 0600
    const auto ffmc_slope = (ffmc_at_0600 - ffmc_at_2000) / 10.0;
    const auto wind_at_0600 = r->at(util::time_index(day + 1, 6, min_date))->wind().speed().asValue();
    const auto wind_at_2000 = r->at(util::time_index(day, 20, min_date))->wind().speed().asValue();
    // need linear interpolation between 2000 and 0600
    const auto wind_slope = (wind_at_0600 - wind_at_2000) / 10.0;
    const auto add_wx =
      [&r, &day, &wx, &min_date, &wind_at_2000, &ffmc_at_2000, &wind_slope, &ffmc_slope](
        const Day day_offset,
        const int hour,
        const int offset) {
        const auto i = util::time_index(day + day_offset, hour, min_date);
        r->at(i) = make_wx(Speed(wind_at_2000 + wind_slope * offset),
                           wx,
                           Ffmc(ffmc_at_2000 + ffmc_slope * offset),
                           hour);
      };
    add_wx(0, 21, 1);
    add_wx(0, 22, 2);
    add_wx(0, 23, 3);
    add_wx(1, 0, 4);
    add_wx(1, 1, 5);
    add_wx(1, 2, 6);
    add_wx(1, 3, 7);
    add_wx(1, 4, 8);
    add_wx(1, 5, 9);
  }
  return r;
}
FireWeatherDaily::FireWeatherDaily(
  const set<const fuel::FuelType*>& used_fuels,
  const map<Day, FwiWeather>& data)
  : FireWeather(used_fuels,
                data.begin()->first,
                data.rbegin()->first,
                make_vector(data).release())
{
}
}
