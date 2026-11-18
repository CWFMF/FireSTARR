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
// use class to enforce rounding
class ROSOffset
{
  IntensitySize intensity_{};
  RoundedValue ros_{};
  Direction raz_{};
  Offset offset_{};

public:
  ROSOffset() noexcept = default;
  ROSOffset(
    const IntensitySize intensity,
    const RoundedValue ros,
    const Direction raz,
    const Offset offset
  ) noexcept
    // HACK: round ros again
    : intensity_{intensity}, ros_{as_value(ros)},
      raz_{Direction{Radians{RoundedValue{raz.asRadians()}}}},
      offset_{as_value(RoundedValue{offset.x}), as_value(RoundedValue{offset.y})}
  { }
  ROSOffset(const ROSOffset& rhs) noexcept = default;
  ROSOffset(ROSOffset&& rhs) noexcept = default;
  ROSOffset& operator=(const ROSOffset& rhs) noexcept = default;
  ROSOffset& operator=(ROSOffset&& rhs) noexcept = default;
  auto operator<=>(const ROSOffset& rhs) const noexcept = default;
  IntensitySize intensity() const noexcept { return intensity_; }
  const RoundedValue& ros() const noexcept { return ros_; }
  const Direction& raz() const noexcept { return raz_; }
  const Offset& offset() const noexcept { return offset_; }
};
using OffsetSet = vector<ROSOffset>;
class FuelType;
}
using fs::fuel::FuelType;
using fs::fuel::OffsetSet;
static constexpr MathConstantSize MAX_SPREAD_ANGLE = 5.0;
static constexpr MathConstantSize INVALID_ROS = -1.0;
static const RoundedIntensity INVALID_INTENSITY{-1.0};
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
  [[nodiscard]] static ThresholdSize calculateRosFromThreshold(const ThresholdSize threshold);
  /**
   * \brief Maximum intensity in any direction for spread (kW/m)
   * \return Maximum intensity in any direction for spread (kW/m)
   */
  [[nodiscard]] RoundedIntensity maxIntensity() const noexcept;
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
  [[nodiscard]] RoundedValue lengthToBreadth() const;
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
  [[nodiscard]] RoundedValue headRos() const;
  /**
   * \brief Head fire spread direction
   * \return Head fire spread direction
   */
  [[nodiscard]] Direction headDirection() const;
  /**
   * \brief Slope factor calculated from percent slope
   * \return Slope factor calculated from percent slope
   */
  [[nodiscard]] RoundedValue slopeFactor() const;
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
  RoundedValue crownFractionBurned() const;
  RoundedValue crownFuelConsumption() const;
  char fireDescription() const;
  RoundedValue surfaceFuelConsumption() const;
  RoundedValue totalFuelConsumption() const;

private:
  // HACK: have private constructor so is_spreading() can short-circuit the calculation,
  // but nothing else can get a partially constructed SpreadInfo object
  SpreadInfo(
    DurationSize time,
    RoundedValue min_ros,
    RoundedValue cell_size,
    const SlopeSize slope,
    const AspectSize aspect,
    const char* fuel_name,
    int nd,
    const FwiWeather weather
  );
  SpreadInfo(
    DurationSize time,
    RoundedValue min_ros,
    RoundedValue cell_size,
    const SpreadKey& key,
    int nd,
    const FwiWeather weather
  );

public:
  SpreadInfo(
    DurationSize time,
    RoundedValue min_ros,
    RoundedValue cell_size,
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
    RoundedValue min_ros,
    RoundedValue cell_size,
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
  static RoundedValue initial(
    SpreadInfo& spread,
    const FwiWeather& weather,
    RoundedValue& ffmc_effect,
    RoundedValue& wsv,
    RoundedValue& rso,
    const FuelType* const fuel,
    bool has_no_slope,
    RoundedValue heading_sin,
    RoundedValue heading_cos,
    RoundedValue bui_eff,
    RoundedValue min_ros,
    RoundedIntensity critical_surface_intensity
  );
  /**
   * \brief Offsets from origin point that represent spread under these conditions
   */
  OffsetSet offsets_{};
  /**
   * \brief Maximum intensity in any direction for spread (kW/m)
   */
  RoundedIntensity max_intensity_{INVALID_INTENSITY};
  /**
   * \brief Attributes for Cell spread is occurring in
   */
  SpreadKey key_{0};

public:
  /**
   * \brief FwiWeather determining spread
   */
  FwiWeather weather{};

private:
  /**
   * \brief Time that spread is occurring
   */
  DurationSize time_{-1};
  RoundedValue l_b_{-1};
  /**
   * \brief Head fire rate of spread (m/min)
   */
  RoundedValue head_ros_{INVALID_ROS};
  RoundedValue cfb_{-1};
  RoundedValue cfc_{-1};
  RoundedValue tfc_{-1};
  RoundedValue sfc_{-1};
  bool is_crown_{false};

public:
  /**
   * \brief Head fire spread direction
   */
  Direction raz_{Direction::Invalid()};

private:
  /**
   * \brief Difference between date and the date of minimum foliar moisture content (from ST-X-3)
   */
  int nd_{-1};
};
}
#endif
