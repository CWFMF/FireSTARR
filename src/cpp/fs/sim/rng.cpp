/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "../stdafx.h"
#include "rng.h"
#include "Settings.h"
namespace fs::rng
{
void make_threshold(
  vector<ThresholdSize>* thresholds,
  mt19937_64* mt,
  const Day start_day,
  const Day last_date,
  const ThresholdSize scenario_weight,
  const ThresholdSize daily_weight,
  const ThresholdSize hourly_weight,
  ThresholdSize (*convert)(_Float64 value)
)
{
  const auto total_weight = scenario_weight + daily_weight + hourly_weight;
  uniform_real_distribution<ThresholdSize> rand(0.0, 1.0);
  const auto general = rand(*mt);
  for (size_t i = start_day; i < MAX_DAYS; ++i)
  {
    const auto daily = rand(*mt);
    for (auto h = 0; h < DAY_HOURS; ++h)
    {
      // generate no matter what so if we extend the time period the results
      // for the first days don't change
      const auto hourly = rand(*mt);
      // only save if we're going to use it
      // HACK: +1 so if it's exactly at the end time there's something there
      if (i <= static_cast<size_t>(last_date + 1))
      {
        // subtract from 1.0 because we want weight to make things more likely not less
        // ensure we stay between 0 and 1
        thresholds->at((i - start_day) * DAY_HOURS + static_cast<size_t>(h)) = convert(max(
          0.0,
          min(
            1.0,
            1.0
              - (scenario_weight * general + daily_weight * daily + hourly_weight * hourly)
                  / total_weight
          )
        ));
      }
    }
  }
}
void make_threshold(
  vector<ThresholdSize>* thresholds,
  mt19937_64* mt,
  const Day start_day,
  const Day last_date,
  ThresholdSize (*convert)(_Float64 value)
)
{   // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  make_threshold(
    thresholds,
    mt,
    start_day,
    last_date,
    settings.threshold_scenario_weight,
    settings.threshold_daily_weight,
    settings.threshold_hourly_weight,
    convert
  );
}
std::seed_seq make_seed(
  const char* name,
  const StartPoint& start_point,
  const Day start_day,
  const size_t salt,
  const size_t base_salt
)
{
  // use independent seeds so that if we remove one threshold it doesn't affect the other
  // HACK: seed_seq takes a list of integers now, so multiply and convert to get more digits
  // NOTE: use abs() because negative numbers act differently on arm64 vs x64 vs windows
  // // NOTE: was matching to 15 digits (digits10 - 6) but use half precision so less likely
  // //       mismatches happen on different hardware/os combinations
  // constexpr auto precision = std::numeric_limits<size_t>::digits10 / 2;
  // NOTE: std::numeric_limits<size_t>::digits10 varies on different hardware
  //       (but is 8 on 32-bit so don't go beyond that in case we can ever get that working)
  constexpr auto precision = 8;
  static_assert(std::numeric_limits<size_t>::digits10 >= precision);
  // Convert through uint64_t, then truncate explicitly to the 32 bits seed_seq
  // keeps anyway. At precision 8 the product needs up to 11 digits, so on a
  // 32-bit target the old direct cast to size_t was out-of-range and therefore
  // undefined behaviour; wasm saturates, giving different random streams.
  const auto lat =
    static_cast<uint32_t>(static_cast<uint64_t>(abs(start_point.latitude()) * pow(10, precision)));
  const auto lon =
    static_cast<uint32_t>(static_cast<uint64_t>(abs(start_point.longitude()) * pow(10, precision)));
  logging::debug("lat/long {} converted to ({:d}, {:d})", start_point, lat, lon);
  const auto d = static_cast<size_t>(start_day);
  logging::info(
    "Seed inputs using precision of {:d} with base_salt {:d} for {:s}: {:d}, {:d}, {:d}, {:d}",
    precision,
    base_salt,
    name,
    salt,
    d,
    lat,
    lon
  );
  // size_t will wrap around so don't need to worry about overflow
  const size_t use_salt = base_salt + salt;
  // seed_seq keeps each value mod 2^32 anyway; truncate explicitly so 32- and
  // 64-bit agree.
  return std::seed_seq{static_cast<uint32_t>(use_salt), static_cast<uint32_t>(d), lat, lon};
}
}
