/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <vector>
#include "FuelLookup.h"

namespace fs
{
namespace sim
{
/**
 * \brief Difference minimum for MathSizes to be considered the same
 */
static const MathSize COMPARE_LIMIT = 1.0E-20f;
/**
 * \brief Reads and provides access to settings for the simulation.
 */
class Settings
{
public:
  /**
   * \brief Set root directory and read settings from file
   * \param dirname Directory to use for settings and relative paths
   */
  static void setRoot(const char* dirname) noexcept;
  /**
   * \brief Set raster root directory
   * \param dirname Directory to use for rasters
   */
  static void setRasterRoot(const char* dirname) noexcept;
  /**
   * \brief Root directory that raster inputs are stored in
   * \return Root directory that raster inputs are stored in
   */
  [[nodiscard]] static const char* rasterRoot() noexcept;
  /**
   * \brief Set fuel lookup table file
   * \param dirname Directory to use for rasters
   */
  static void setFuelLookupTable(const char* filename) noexcept;
  /**
   * \brief Fuel lookup table
   * \return Fuel lookup table
   */
  [[nodiscard]] static const fuel::FuelLookup& fuelLookup() noexcept;
  /**
   * \brief Whether or not to save probability grids
   * \return Whether or not to save probability grids
   */
  [[nodiscard]] static bool saveProbability() noexcept;
  /**
   * \brief Set whether or not to save probability grids
   * \param value Whether or not to save probability grids
   * \return None
   */
  static void setSaveProbability(bool value) noexcept;
  /**
   * \brief Minimum rate of spread before fire is considered to be spreading (m/min)
   * \return Minimum rate of spread before fire is considered to be spreading (m/min)
   */
  [[nodiscard]] static MathSize minimumRos() noexcept;
  static void setMinimumRos(MathSize value) noexcept;
  /**
   * \brief Maximum distance that the fire is allowed to spread in one step (# of cells)
   * \return Maximum distance that the fire is allowed to spread in one step (# of cells)
   */
  [[nodiscard]] static MathSize maximumSpreadDistance() noexcept;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the day
   * \return Minimum Fine Fuel Moisture Code required for spread during the day
   */
  [[nodiscard]] static MathSize minimumFfmc() noexcept;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the night
   * \return Minimum Fine Fuel Moisture Code required for spread during the night
   */
  [[nodiscard]] static MathSize minimumFfmcAtNight() noexcept;
  /**
   * \brief Offset from sunrise at which the day is considered to start (hours)
   * \return Offset from sunrise at which the day is considered to start (hours)
   */
  [[nodiscard]] static DurationSize offsetSunrise() noexcept;
  /**
   * \brief Offset from sunrise at which the day is considered to end (hours)
   * \return Offset from sunrise at which the day is considered to end (hours)
   */
  [[nodiscard]] static DurationSize offsetSunset() noexcept;
  /**
   * \brief Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
   * \return Percent of the stand that is composed of conifer (%)
   */
  [[nodiscard]] static int defaultPercentConifer() noexcept;
  /**
   * \brief Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
   * \return Percent of the stand that is composed of dead fir (NOT percent of the fir that is dead) (%)
   */
  [[nodiscard]] static int defaultPercentDeadFir() noexcept;
  /**
   * \brief Confidence required before simulation stops (% / 100)
   * \return Confidence required before simulation stops (% / 100)
   */
  [[nodiscard]] static ThresholdSize confidenceLevel() noexcept;
  /**
   * \brief Set confidence required before simulation stops (% / 100)
   * \return Set confidence required before simulation stops (% / 100)
   */
  static void setConfidenceLevel(const ThresholdSize value) noexcept;
  /**
   * \brief Maximum time simulation can run before it is ended and whatever results it has are used (s)
   * \return Maximum time simulation can run before it is ended and whatever results it has are used (s)
   */
  [[nodiscard]] static size_t maximumTimeSeconds() noexcept;
  /**
   * \brief Set maximum time simulation can run before it is ended and whatever results it has are used (s)
   * \return Set maximum time simulation can run before it is ended and whatever results it has are used (s)
   */
  static void setMaximumTimeSeconds(const size_t value) noexcept;
  /**
   * \brief Maximum number of simulations that can run before it is ended and whatever results it has are used
   * \return Maximum number of simulations that can run before it is ended and whatever results it has are used
   */
  [[nodiscard]] static size_t maximumCountSimulations() noexcept;
  /**
   * \brief Weight to give to Scenario part of thresholds
   * \return Weight to give to Scenario part of thresholds
   */
  [[nodiscard]] static ThresholdSize thresholdScenarioWeight() noexcept;
  /**
   * \brief Weight to give to daily part of thresholds
   * \return Weight to give to daily part of thresholds
   */
  [[nodiscard]] static ThresholdSize thresholdDailyWeight() noexcept;
  /**
   * \brief Weight to give to hourly part of thresholds
   * \return Weight to give to hourly part of thresholds
   */
  [[nodiscard]] static ThresholdSize thresholdHourlyWeight() noexcept;
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  [[nodiscard]] static vector<int> outputDateOffsets();
  /**
   * \brief Set days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return None
   */
  static void setOutputDateOffsets(const char* value);
  /**
   * \brief Whatever the maximum value in the date offsets is
   * \return Whatever the maximum value in the date offsets is
   */
  [[nodiscard]] static int maxDateOffset() noexcept;
  Settings() = delete;
};
}
}
