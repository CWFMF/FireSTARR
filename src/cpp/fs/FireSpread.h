/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FIRESPREAD_H
#define FS_FIRESPREAD_H
#include "stdafx.h"
#include "Cell.h"
#include "FWI.h"
#include "Location.h"
#include "Point.h"
#include "Weather.h"
namespace fs
{
namespace fuel
{
struct ROSOffset
{
  IntensitySize intensity;
  ROSSize ros;
  Direction raz;
  Offset offset;
  auto operator<=>(const ROSOffset& rhs) const noexcept = default;
};
using OffsetSet = vector<ROSOffset>;
class FuelType;
}
using fs::fuel::FuelType;
using fs::fuel::OffsetSet;
static constexpr MathSize MAX_SPREAD_ANGLE = 5.0;
static constexpr MathSize INVALID_ROS = -1.0;
static constexpr MathSize INVALID_INTENSITY = -1.0;
class Scenario;
/**
 * \brief Possible results of an attempt to spread.
 */
enum class SpreadResult
{
  SPREAD_TOO_SLOW,
  SPREAD_IMPOSSIBLE,
  SPREAD_SCHEDULED
};
int calculate_nd_ref_for_point(const int elevation, const Point& point) noexcept;
int calculate_nd_for_point(const Day day, const int elevation, const Point& point);
/**
 * \brief Information regarding spread within a Cell for a specific Scenario and time.
 */
class SpreadInfo
{
public:
  /**
   * \brief Lookup table for Slope Factor calculated from Percent Slope
   */
  static const SlopeTableArray SlopeTable;
  SpreadInfo() = default;
  ~SpreadInfo() = default;
  SpreadInfo(
    const Scenario& scenario,
    DurationSize time,
    const SpreadKey& key,
    int nd,
    const ptr<const FwiWeather> weather
  );
  constexpr SpreadInfo(SpreadInfo&& rhs) noexcept = default;
  SpreadInfo(const SpreadInfo& rhs) noexcept = default;
  constexpr SpreadInfo& operator=(SpreadInfo&& rhs) noexcept = default;
  SpreadInfo& operator=(const SpreadInfo& rhs) noexcept = default;
  /**
   * \brief Determine rate of spread from probability of spread threshold
   * \param threshold Probability of spread threshold
   * \return Rate of spread at given threshold (m/min)
   */
  [[nodiscard]] static constexpr MathSize calculateRosFromThreshold(const ThresholdSize threshold)
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
  /**
   * \brief Maximum intensity in any direction for spread (kW/m)
   * \return Maximum intensity in any direction for spread (kW/m)
   */
  [[nodiscard]] MathSize maxIntensity() const noexcept { return max_intensity_; }
  /**
   * \brief Offsets from origin point that represent spread under these conditions
   * \return Offsets from origin point that represent spread under these conditions
   */
  [[nodiscard]] const OffsetSet& offsets() const { return offsets_; }
  /**
   * \brief Whether or not there is no spread
   * \return Whether or not there is no spread
   */
  [[nodiscard]] constexpr bool isNotSpreading() const { return isInvalid(); }
  /**
   * \brief Difference between date and the date of minimum foliar moisture content
   * \return Difference between date and the date of minimum foliar moisture content
   */
  [[nodiscard]] constexpr int nd() const { return nd_; }
  /**
   * \brief Time used for spread
   * \return Time used for spread
   */
  [[nodiscard]] constexpr DurationSize time() const { return time_; }
  /**
   * \brief Length to breadth ratio used for spread
   * \return Length to breadth ratio used for spread
   */
  [[nodiscard]] constexpr MathSize lengthToBreadth() const { return l_b_; }
  /**
   * \brief Slope used for spread (%)
   * \return Slope used for spread (%)
   */
  [[nodiscard]] constexpr SlopeSize percentSlope() const { return Cell::slope(key_); }
  /**
   * \brief Aspect used for spread (degrees)
   * \return Aspect used for spread (degrees)
   */
  [[nodiscard]] constexpr AspectSize slopeAzimuth() const { return Cell::aspect(key_); }
  /**
   * \brief Head fire rate of spread (m/min)
   * \return Head fire rate of spread (m/min)
   */
  [[nodiscard]] constexpr MathSize headRos() const { return head_ros_; }
  /**
   * \brief Head fire spread direction
   * \return Head fire spread direction
   */
  [[nodiscard]] constexpr Direction headDirection() const { return raz_; }
  /**
   * \brief Slope factor calculated from percent slope
   * \return Slope factor calculated from percent slope
   */
  [[nodiscard]] constexpr MathSize slopeFactor() const
  {
    // HACK: slope can be infinite, but anything > 60 is the same as 60
    // we already capped the percent slope when making the Cells
    return SlopeTable.at(percentSlope());
  }
  /**
   * \brief Calculate foliar moisture
   * \return Calculated foliar moisture
   */
  [[nodiscard]] static _Float64 foliarMoisture(int nd)
  {
    nd = abs(nd);
    // don't need to check  `&& nd < 50` in second part because of reordering
    return nd >= 50 ? 120.0
         : nd >= 30 ? 32.9 + 3.17 * nd - 0.0288 * nd * nd
                    : 85.0 + 0.0189 * nd * nd;
  }
  /**
   * \brief Calculate foliar moisture
   * \return Calculated foliar moisture
   */
  [[nodiscard]] _Float64 foliarMoisture() const { return foliarMoisture(nd_); }
  /**
   * \brief Whether or not there is no spread for given conditions
   * \return Whether or not there is no spread for given conditions
   */
  [[nodiscard]] constexpr bool isInvalid() const { return INVALID_ROS == head_ros_; }
  SpreadInfo(
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
    const ptr<const FwiWeather> weather
  );
  SpreadInfo(
    const tm& start_date,
    const Point& start_point,
    const ElevationSize elevation,
    const SlopeSize slope,
    const AspectSize aspect,
    const char* fuel_name,
    const ptr<const FwiWeather> weather
  );
  MathSize crownFractionBurned() const { return cfb_; }
  MathSize crownFuelConsumption() const { return cfc_; }
  char fireDescription() const { return cfb_ >= 0.9 ? 'C' : (cfb_ < 0.1 ? 'S' : 'I'); }
  MathSize surfaceFuelConsumption() const { return sfc_; }
  MathSize totalFuelConsumption() const { return tfc_; }

private:
  // HACK: have private constructor so is_spreading() can short-circuit the calculation,
  // but nothing else can get a partially constructed SpreadInfo object
  /**
   * \brief Calculate fire spread for time and place
   * \param scenario Scenario this is spreading in
   * \param time Time spread is occurring
   * \param key Attributes for Cell spread is occurring in
   * \param nd Difference between date and the date of minimum foliar moisture content
   *DurationSize timether FwiWeather to use for calculations
   * \param weather_daily FwiWeather to use for spread event probability
   */
  SpreadInfo(
    const Scenario& scenario,
    DurationSize time,
    const SpreadKey& key,
    int nd,
    const ptr<const FwiWeather> weather,
    const ptr<const FwiWeather> weather_daily
  );
  /**
   * Actual fire spread calculation without needing to worry about settings or scenarios
   */
  SpreadInfo(
    DurationSize time,
    MathSize min_ros,
    MathSize cell_size,
    const SlopeSize slope,
    const AspectSize aspect,
    const char* fuel_name,
    int nd,
    const ptr<const FwiWeather> weather
  );
  SpreadInfo(
    DurationSize time,
    MathSize min_ros,
    MathSize cell_size,
    const SpreadKey& key,
    int nd,
    const ptr<const FwiWeather> weather
  );
  SpreadInfo(
    DurationSize time,
    MathSize min_ros,
    MathSize cell_size,
    const SpreadKey& key,
    int nd,
    const ptr<const FwiWeather> weather,
    const ptr<const FwiWeather> weather_daily
  );

public:
  // HACK: for testing fuels
  SpreadInfo(
    const FuelType* fuel_original,
    DurationSize time,
    MathSize min_ros,
    MathSize cell_size,
    const SpreadKey& key,
    int nd,
    const ptr<const FwiWeather> weather,
    const ptr<const FwiWeather> weather_daily
  );

private:
  /**
   * Do initial spread calculations
   * \return Initial head ros calculation (-1 for none)
   */
  static MathSize initial(
    SpreadInfo& spread,
    const FwiWeather& weather,
    MathSize& ffmc_effect,
    MathSize& wsv,
    MathSize& rso,
    const FuelType* const fuel,
    bool has_no_slope,
    MathSize heading_sin,
    MathSize heading_cos,
    MathSize bui_eff,
    MathSize min_ros,
    MathSize critical_surface_intensity
  );
  /**
   * \brief Offsets from origin point that represent spread under these conditions
   */
  OffsetSet offsets_{};
  /**
   * \brief Maximum intensity in any direction for spread (kW/m)
   */
  MathSize max_intensity_ = INVALID_INTENSITY;
  /**
   * \brief Attributes for Cell spread is occurring in
   */
  SpreadKey key_ = 0;

public:
  /**
   * \brief FwiWeather determining spread
   */
  ptr<const FwiWeather> weather{nullptr};

private:
  /**
   * \brief Time that spread is occurring
   */
  DurationSize time_ = -1;
  MathSize l_b_ = -1;
  /**
   * \brief Head fire rate of spread (m/min)
   */
  MathSize head_ros_ = INVALID_ROS;
  MathSize cfb_ = -1;
  MathSize cfc_ = -1;
  MathSize tfc_ = -1;
  MathSize sfc_ = -1;
  bool is_crown_ = false;

public:
  /**
   * \brief Head fire spread direction
   */
  Direction raz_{Direction::Invalid()};

private:
  /**
   * \brief Difference between date and the date of minimum foliar moisture content (from ST-X-3)
   */
  int nd_ = -1;
};
}
#endif
