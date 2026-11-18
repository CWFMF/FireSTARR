/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FIRE_SPREAD_H
#define FS_FIRE_SPREAD_H
#include "../stdafx.h"
#include "../geo/Point.h"
#include "../grid/Cell.h"
#include "../types/Location.h"
#include "../wx/FwiWeather.h"
#include "../wx/WeatherIndices.h"
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
 * \brief Information regarding spread
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
  SpreadInfo(SpreadInfo&& rhs) noexcept = default;
  SpreadInfo(const SpreadInfo& rhs) noexcept = default;
  SpreadInfo& operator=(SpreadInfo&& rhs) noexcept = default;
  SpreadInfo& operator=(const SpreadInfo& rhs) noexcept = default;
  /**
   * \brief Determine rate of spread from probability of spread threshold
   * \param threshold Probability of spread threshold
   * \return Rate of spread at given threshold (m/min)
   */
  [[nodiscard]] static MathSize calculateRosFromThreshold(const ThresholdSize threshold);
  /**
   * \brief Maximum intensity in any direction for spread (kW/m)
   * \return Maximum intensity in any direction for spread (kW/m)
   */
  [[nodiscard]] MathSize maxIntensity() const noexcept;
  /**
   * \brief Offsets from origin point that represent spread under these conditions
   * \return Offsets from origin point that represent spread under these conditions
   */
  [[nodiscard]] const OffsetSet& offsets() const;
  /**
   * \brief Whether or not there is no spread
   * \return Whether or not there is no spread
   */
  [[nodiscard]] bool isNotSpreading() const;
  /**
   * \brief Difference between date and the date of minimum foliar moisture content
   * \return Difference between date and the date of minimum foliar moisture content
   */
  [[nodiscard]] int nd() const;
  /**
   * \brief Time used for spread
   * \return Time used for spread
   */
  [[nodiscard]] DurationSize time() const;
  /**
   * \brief Length to breadth ratio used for spread
   * \return Length to breadth ratio used for spread
   */
  [[nodiscard]] MathSize lengthToBreadth() const;
  /**
   * \brief Slope used for spread (%)
   * \return Slope used for spread (%)
   */
  [[nodiscard]] SlopeSize percentSlope() const;
  /**
   * \brief Aspect used for spread (degrees)
   * \return Aspect used for spread (degrees)
   */
  [[nodiscard]] AspectSize slopeAzimuth() const;
  /**
   * \brief Head fire rate of spread (m/min)
   * \return Head fire rate of spread (m/min)
   */
  [[nodiscard]] MathSize headRos() const;
  /**
   * \brief Head fire spread direction
   * \return Head fire spread direction
   */
  [[nodiscard]] Direction headDirection() const;
  /**
   * \brief Slope factor calculated from percent slope
   * \return Slope factor calculated from percent slope
   */
  [[nodiscard]] MathSize slopeFactor() const;
  /**
   * \brief Calculate foliar moisture
   * \return Calculated foliar moisture
   */
  [[nodiscard]] static _Float64 foliarMoisture(int nd);
  /**
   * \brief Calculate foliar moisture
   * \return Calculated foliar moisture
   */
  [[nodiscard]] _Float64 foliarMoisture() const;
  /**
   * \brief Whether or not there is no spread for given conditions
   * \return Whether or not there is no spread for given conditions
   */
  [[nodiscard]] bool isInvalid() const;
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
    const FwiWeather weather
  );
  SpreadInfo(
    const tm& start_date,
    const Point& start_point,
    const ElevationSize elevation,
    const SlopeSize slope,
    const AspectSize aspect,
    const char* fuel_name,
    const FwiWeather weather
  );
  MathSize crownFractionBurned() const;
  MathSize crownFuelConsumption() const;
  char fireDescription() const;
  MathSize surfaceFuelConsumption() const;
  MathSize totalFuelConsumption() const;

private:
  // HACK: have private constructor so is_spreading() can short-circuit the calculation,
  // but nothing else can get a partially constructed SpreadInfo object
  SpreadInfo(
    DurationSize time,
    MathSize min_ros,
    MathSize cell_size,
    const SlopeSize slope,
    const AspectSize aspect,
    const char* fuel_name,
    int nd,
    const FwiWeather weather
  );
  SpreadInfo(
    DurationSize time,
    MathSize min_ros,
    MathSize cell_size,
    const SpreadKey& key,
    int nd,
    const FwiWeather weather
  );

public:
  SpreadInfo(
    DurationSize time,
    MathSize min_ros,
    MathSize cell_size,
    const SpreadKey& key,
    int nd,
    const FwiWeather weather,
    const FwiWeather weather_daily
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
    const FwiWeather weather,
    const FwiWeather weather_daily
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
  FwiWeather weather{};

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
