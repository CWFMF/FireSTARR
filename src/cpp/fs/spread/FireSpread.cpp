/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FireSpread.h"
#include "../fuel/FuelLookup.h"
#include "../fuel/FuelType.h"
#include "../sim/Settings.h"
#include "../unstable.h"
#include "../util/TimeUtil.h"
#include "SpreadAlgorithm.h"
namespace fs
{
using namespace fs::fuel;
/**
 * \brief Maximum slope that affects ISI - everything after this is the same factor
 */
static constexpr auto MAX_SLOPE_FOR_FACTOR = 69;
SlopeTableArray make_slope_table() noexcept
{
  // HACK: slope can be infinite, but anything > max is the same as max
  // ST-X-3 Eq. 39 - Calculate Spread Factor
  // GLC-X-10 39a/b increase to 70% limit
  SlopeTableArray result{};
  for (size_t i = 0; i <= MAX_SLOPE_FOR_FACTOR; ++i)
  {
    result.at(i) = exp(3.533 * pow(static_cast<MathSize>(i) / 100.0, 1.2));
  }
  constexpr auto MAX_SLOPE = MAX_SLOPE_FOR_FACTOR + 1;
  // anything >=70 is just 10
  std::fill(&(result[MAX_SLOPE]), &(result[MAX_SLOPE_FOR_DISTANCE]), 10.0);
  // if we ask for result of invalid slope it should be invalid
  std::fill(&(result[MAX_SLOPE_FOR_DISTANCE + 1]), &(result[INVALID_SLOPE]), -1);
  static_assert(result.size() == INVALID_SLOPE + 1);
  return result;
}
const SlopeTableArray SpreadInfo::SlopeTable = make_slope_table();
int calculate_nd_ref_for_point(const int elevation, const Point& point) noexcept
{
  // NOTE: cffdrs R package stores longitude West as a positive, so this would be `- long`
  const auto latn = elevation <= 0 ? (46.0 + 23.4 * exp(-0.0360 * (150 + point.longitude())))
                                   : (43.0 + 33.7 * exp(-0.0351 * (150 + point.longitude())));
  // add 0.5 to round by truncating
  return static_cast<int>(trunc(
    0.5
    + (elevation <= 0 ? 151.0 * (point.latitude() / latn) : 142.1 * (point.latitude() / latn) + 0.0172 * elevation)
  ));
}
int calculate_nd_for_point(const Day day, const int elevation, const Point& point)
{
  return static_cast<int>(abs(day - calculate_nd_ref_for_point(elevation, point)));
}
FBPValue calculate_standard_back_isi_wsv(const LookupValue& v_) noexcept
{
  const auto v = round_value(v_);
  return 0.208 * exp(-0.05039 * v);
}
FBPValue calculate_standard_wsv(const LookupValue& v_) noexcept
{
  const auto v = round_value(v_);
  return v < 40.0 ? exp(0.05039 * v) : 12.0 * (1.0 - exp(-0.0818 * (v - 28)));
}
FBPValue SpreadInfo::initial(
  SpreadInfo& spread,
  const FwiWeather& weather,
  FBPValue& ffmc_effect,
  FBPValue& wsv,
  FBPValue& rso,
  const FuelType* const fuel,
  bool has_no_slope,
  FBPValue heading_sin,
  FBPValue heading_cos,
  FBPValue bui_eff,
  FBPValue min_ros,
  RoundedIntensity critical_surface_intensity
)
{
  ffmc_effect = spread.weather.ffmcEffect();
  // needs to be non-const so that we can update if slopeEffect changes direction
  auto raz = spread.weather.wind().heading();
  const auto isz = 0.208 * ffmc_effect;
  wsv = as_value(spread.weather.wind().speed);
  if (!has_no_slope)
  {
    const auto isf1 = fuel->calculateIsf(spread, isz);
    auto wse = 0.0 == isf1 ? 0 : log(isf1 / isz) / 0.05039;
    if (wse > 40)
    {
      wse =
        28.0 - log(1.0 - min(0.999 * 2.496 * ffmc_effect, isf1) / (2.496 * ffmc_effect)) / 0.0818;
    }
    const auto heading =
      Radians::from_degrees(static_cast<_Float64>(Cell::aspect(spread.key_))).to_heading();
    // FIX: ignore heading arguments for now since it was changing results
    std::ignore = heading_sin;
    std::ignore = heading_cos;
    const auto wsv_x = spread.weather.wind().wsvX() + wse * cos(heading);
    const auto wsv_y = spread.weather.wind().wsvY() + wse * sin(heading);
    // // we know that at->raz is already set to be the wind heading
    // const auto wsv_x = spread.weather->wind.wsvX() + wse * heading_sin;
    // const auto wsv_y = spread.weather->wind.wsvY() + wse * heading_cos;
    wsv = sqrt(wsv_x * wsv_x + wsv_y * wsv_y);
    raz = (0 == wsv) ? 0 : acos(wsv_y / wsv);
    if (wsv_x < 0)
    {
      raz = as_value(Radians::D_360()) - raz;
    }
  }
  spread.raz_ = Direction{Radians{raz}};
  const auto isi = isz * calculate_standard_wsv(as_value(wsv));
  // FIX: make this a member function so we don't need to preface head_ros_
  spread.head_ros_ = fuel->calculateRos(spread.nd(), weather, isi) * bui_eff;
  if (min_ros > spread.head_ros_)
  {
    spread.head_ros_ = INVALID_ROS;
  }
  else
  {
    spread.sfc_ = fuel->surfaceFuelConsumption(spread);
    auto sfc = spread.sfc_;
    auto csi = critical_surface_intensity;
    rso = FuelType::criticalRos(sfc, csi);
    const auto sfi = fire_intensity(spread.sfc_, spread.head_ros_);
    spread.is_crown_ = FuelType::isCrown(csi, sfi);
    if (spread.is_crown_)
    {
      spread.head_ros_ = fuel->finalRos(
        spread, isi, fuel->crownFractionBurned(spread.head_ros_, rso), spread.head_ros_
      );
    }
  }
  return spread.head_ros_;
}
static SpreadKey make_key(const SlopeSize slope, const AspectSize aspect, const char* fuel_name)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  static const auto& lookup = settings.fuel_lookup.lookup();
  const auto key =
    Cell::key(Cell::hashCell(slope, aspect, FuelType::safeCode(lookup.byName(fuel_name))));
  const auto a = Cell::aspect(key);
  const auto s = Cell::slope(key);
  const auto fuel = fuel_by_code(Cell::fuelCode(key));
  logging::check_equal(s, slope, "slope");
  logging::check_equal(
    a, (static_cast<SlopeSize>(0) == slope ? static_cast<AspectSize>(0) : aspect), "aspect"
  );
  logging::check_equal(fuel->name(), fuel_name, "fuel");
  return key;
}
[[nodiscard]] ThresholdSize SpreadInfo::calculateRosFromThreshold(const ThresholdSize threshold)
{
  // for some reason it returns -nan instead of nan if it's 1, so return this instead
  if (1.0 == threshold)
  {
    return std::numeric_limits<ThresholdSize>::infinity();
  }
  if (0.0 == threshold)
  {
    return 0.0;
  }
  /*! \page spread Probability of fire spread
   *
   * Probability of spread is converted into the ROS that would give it,
   * and then when checked the fire burns if ROS > ROS_threshold
   *
   * 25.0 / 4.0 * log(-(exp(41.0 / 25.0) * threshold) / (threshold - 1));
   * Should be the inverse of:
   * 1 / (1 + exp(1.64 - 0.16 * ros));
   *
   * chance of spread      ros (m/min)
   * ~16.2465%             0
   * ~18.5427%             1
   * ~23.8667%             3
   * ~30.1535%             5
   * ~49.0000%             10
   * ~68.1354%             15
   * ~82.6353%             20
   * 90%                   23.9827
   * 95%                   28.6527
   * 98%                   34.5739
   * 99%                   38.9695
   *
   * \section References
   *
   * Podur, Justin & Wotton, Mike. (2011). Defining fire spread event days for fire-growth
   * modelling. International Journal of Wildland Fire. 20. 497-507. 10.1071/WF09001.
   */
  return 25.0 / 4.0 * log(-(exp(41.0 / 25.0) * threshold) / (threshold - 1));
}
[[nodiscard]] RoundedIntensity SpreadInfo::maxIntensity() const noexcept { return max_intensity_; }
[[nodiscard]] const OffsetSet& SpreadInfo::offsets() const { return offsets_; }
[[nodiscard]] bool SpreadInfo::isNotSpreading() const { return isInvalid(); }
[[nodiscard]] int SpreadInfo::nd() const { return nd_; }
[[nodiscard]] DurationSize SpreadInfo::time() const { return time_; }
[[nodiscard]] FBPValue SpreadInfo::lengthToBreadth() const { return l_b_; }
[[nodiscard]] SlopeSize SpreadInfo::percentSlope() const { return Cell::slope(key_); }
[[nodiscard]] AspectSize SpreadInfo::slopeAzimuth() const { return Cell::aspect(key_); }
[[nodiscard]] FBPValue SpreadInfo::headRos() const { return head_ros_; }
[[nodiscard]] Direction SpreadInfo::headDirection() const { return raz_; }
[[nodiscard]] FBPValue SpreadInfo::slopeFactor() const
{
  // HACK: slope can be infinite, but anything > 60 is the same as 60
  // we already capped the percent slope when making the Cells
  return SlopeTable.at(percentSlope());
}
[[nodiscard]] _Float64 SpreadInfo::foliarMoisture(int nd)
{
  nd = abs(nd);
  // don't need to check  `&& nd < 50` in second part because of reordering
  return nd >= 50 ? 120.0
       : nd >= 30 ? 32.9 + 3.17 * nd - 0.0288 * nd * nd
                  : 85.0 + 0.0189 * nd * nd;
}
[[nodiscard]] _Float64 SpreadInfo::foliarMoisture() const { return foliarMoisture(nd_); }
[[nodiscard]] bool SpreadInfo::isInvalid() const { return INVALID_ROS == head_ros_; }
SpreadInfo::SpreadInfo(
  const YearSize year,
  const int month,
  const int day,
  const int hour,
  const int minute,
  const Point& start_point,
  const ElevationSize elevation,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const FwiWeather weather
)
  : SpreadInfo(
      to_utc(year, month, day, hour, minute),
      start_point,
      elevation,
      slope,
      aspect,
      fuel_name,
      weather
    )
{ }
SpreadInfo::SpreadInfo(
  const tm& start_date,
  const Point& start_point,
  const ElevationSize elevation,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const FwiWeather weather
)
  : SpreadInfo(
      to_time(start_date),
      0.0,
      100.0,
      slope,
      aspect,
      fuel_name,
      calculate_nd_for_point(static_cast<Day>(start_date.tm_yday), elevation, start_point),
      weather
    )
{ }
FBPValue SpreadInfo::crownFractionBurned() const { return cfb_; }
FBPValue SpreadInfo::crownFuelConsumption() const { return cfc_; }
char SpreadInfo::fireDescription() const { return cfb_ >= 0.9 ? 'C' : (cfb_ < 0.1 ? 'S' : 'I'); }
FBPValue SpreadInfo::surfaceFuelConsumption() const { return sfc_; }
FBPValue SpreadInfo::totalFuelConsumption() const { return tfc_; }
SpreadInfo::SpreadInfo(
  const DurationSize time,
  const FBPValue min_ros,
  const FBPValue cell_size,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const int nd,
  const FwiWeather weather
)
  : SpreadInfo(time, min_ros, cell_size, make_key(slope, aspect, fuel_name), nd, weather, weather)
{ }
SpreadInfo::SpreadInfo(
  const DurationSize time,
  const FBPValue min_ros,
  const FBPValue cell_size,
  const SpreadKey& key,
  const int nd,
  const FwiWeather weather
)
  : SpreadInfo(time, min_ros, cell_size, key, nd, weather, weather)
{ }
SpreadInfo::SpreadInfo(
  const DurationSize time,
  const FBPValue min_ros,
  const FBPValue cell_size,
  const SpreadKey& key,
  const int nd,
  const FwiWeather weather,
  const FwiWeather weather_daily
)
  : SpreadInfo{
      fuel_by_code(Cell::fuelCode(key)),
      time,
      min_ros,
      cell_size,
      key,
      nd,
      weather,
      weather_daily
    }
{ }
SpreadInfo::SpreadInfo(
  const FuelType* fuel_original,
  const DurationSize time,
  const FBPValue min_ros,
  const FBPValue cell_size,
  const SpreadKey& key,
  const int nd,
  const FwiWeather weather,
  const FwiWeather weather_daily
)
  : offsets_({}), max_intensity_(INVALID_INTENSITY), key_(key), weather(weather), time_(time),
    head_ros_(INVALID_ROS), cfb_(-1), cfc_(-1), tfc_(-1), sfc_(-1), is_crown_(false),
    raz_(fs::Direction::Invalid()), nd_(nd)
{
  // HACK: use weather_daily to figure out probability of spread but hourly for ROS
  const auto slope_azimuth = Cell::aspect(key_);
  if (is_null_fuel(fuel_original))
  {
    return;
  }
  // HACK: resolve to specific type here - lose original type but only care about this nd value
  const auto fuel = fuel_original->find_fuel_by_season(nd);
  const auto has_no_slope = 0 == percentSlope();
  auto [heading_sin, heading_cos] = [&]() noexcept -> std::pair<MathSize, MathSize> {
    if (has_no_slope)
    {
      return {0, 0};
    }
    const auto heading = Radians::from_aspect(slope_azimuth).to_heading();
    return {sin(heading), cos(heading)};
  }();
  // HACK: only use BUI from hourly weather for both calculations
  const auto _bui = as_value(weather.bui());
  const auto bui_eff = fuel->buiEffect(_bui);
  // FIX: gets calculated when not necessary sometimes
  const auto critical_surface_intensity = fuel->criticalSurfaceIntensity(*this);
  FBPValue ffmc_effect;
  FBPValue wsv;
  FBPValue rso;
  if (min_ros > SpreadInfo::initial(
        *this,
        weather_daily,
        ffmc_effect,
        wsv,
        rso,
        fuel,
        has_no_slope,
        heading_sin,
        heading_cos,
        bui_eff,
        min_ros,
        critical_surface_intensity
      )
      || sfc_ < COMPARE_LIMIT)
  {
    return;
  }
  // Now use hourly weather for actual spread calculations
  // don't check again if pointing at same weather
  if (weather != weather_daily)
  {
    if ((min_ros > SpreadInfo::initial(
           *this,
           weather,
           ffmc_effect,
           wsv,
           rso,
           fuel,
           has_no_slope,
           heading_sin,
           heading_cos,
           bui_eff,
           min_ros,
           critical_surface_intensity
         )
         || sfc_ < COMPARE_LIMIT))
    {
      // no spread with hourly weather
      // NOTE: only would happen if FFMC hourly is lower than FFMC daily?
      return;
    }
  }
  logging::verbose("initial ros is {:f}", as_value(head_ros_));
  const auto back_isi = ffmc_effect * calculate_standard_back_isi_wsv(as_value(wsv));
  auto back_ros = fuel->calculateRos(nd, weather, back_isi) * bui_eff;
  if (is_crown_)
  {
    back_ros = fuel->finalRos(*this, back_isi, fuel->crownFractionBurned(back_ros, rso), back_ros);
  }
  tfc_ = sfc_;
  // don't need to re-evaluate if crown with new head_ros_ because it would only go up if
  // is_crown_
  if (fuel->canCrown() && is_crown_)
  {
    // wouldn't be crowning if ros is 0 so that's why this is in an else
    cfb_ = fuel->crownFractionBurned(head_ros_, rso);
    cfc_ = fuel->crownConsumption(cfb_);
    tfc_ = FBPValue{sfc_ + cfc_};
  }
  // max intensity should always be at the head
  max_intensity_ = fire_intensity(tfc_, head_ros_);
  l_b_ = fuel->lengthToBreadth(wsv);
  const HorizontalAdjustment correction_factor =
    horizontal_adjustment(slope_azimuth, percentSlope());
  const auto spread_algorithm = WidestEllipseAlgorithm(MAX_SPREAD_ANGLE, cell_size, min_ros);
  offsets_ = spread_algorithm.calculate_offsets(
    correction_factor, tfc_, Radians{raz_.asRadians()}, head_ros_, back_ros, l_b_
  );
  // #endif
  // if no offsets then not spreading so invalidate head_ros_
  if (0 == offsets_.size())
  {
    head_ros_ = INVALID_ROS;
    max_intensity_ = INVALID_INTENSITY;
    cfb_ = -1;
    cfc_ = -1;
    tfc_ = -1;
    sfc_ = -1;
    is_crown_ = false;
    raz_ = fs::Direction::Invalid();
  }
}
}
