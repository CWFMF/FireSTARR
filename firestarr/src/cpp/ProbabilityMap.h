/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <string>
#include <vector>
#include "GridMap.h"
#include "Statistics.h"
#include "Perimeter.h"
namespace fs
{
namespace sim
{
/**
 * \brief The code to burn into probability maps where the perimeter is to represent the status of the sims
 */
enum ProcessingStatus : size_t
{
  unprocessed = 2,
  processing = 3,
  processed = 4,
};
class Model;
class IntensityMap;
/**
 * \brief Map of the percentage of simulations in which a Cell burned in each intensity category.
 */
class ProbabilityMap
{
public:
  ProbabilityMap() = delete;
  ~ProbabilityMap();
  ProbabilityMap(const ProbabilityMap& rhs) noexcept = delete;
  ProbabilityMap(ProbabilityMap&& rhs) noexcept = delete;
  ProbabilityMap& operator=(const ProbabilityMap& rhs) noexcept = delete;
  ProbabilityMap& operator=(ProbabilityMap&& rhs) noexcept = delete;
  /**
   * \brief Constructor
   * \param dir_out Directory to save outputs to
   * \param time Time in simulation this ProbabilityMap represents
   * \param start_time Start time of simulation
   * \param min_value Lower bound of 'low' intensity range
   * \param low_max Upper bound of 'low' intensity range
   * \param med_max Upper bound of 'moderate' intensity range
   * \param max_value Upper bound of 'high' intensity range
   * \param grid_info GridBase to use for extent of this
   */
  ProbabilityMap(const string dir_out,
                 DurationSize time,
                 DurationSize start_time,
                 const data::GridBase& grid_info);
  /**
   * \brief Assign perimeter to use for marking cells as initial perimeter
   * \param perimeter Ignition grid to store for marking in outputs
   */
  void setPerimeter(const topo::Perimeter* const perimeter);
  /**
   * \brief Combine results from another ProbabilityMap into this one
   * \param rhs ProbabilityMap to combine from
   */
  void addProbabilities(const ProbabilityMap& rhs);
  /**
   * \brief Add in an IntensityMap to the appropriate probability grid based on each cell burn intensity
   * \param for_time IntensityMap to add results from
   */
  void addProbability(const IntensityMap& for_time);
  /**
   * \brief Output Statistics to log
   */
  void show() const;
  /**
   * \brief Save total, low, moderate, and high maps, and output information to log
   * \param start_time Start time of simulation
   * \param time Time for these maps
   * \param processing_status Stage in processing for simulations
   */
  void saveAll(const tm& start_time,
               DurationSize time,
               const ProcessingStatus processing_status) const;
  /**
   * \brief Clear maps and return to initial state
   */
  void reset();
  /**
   * Delete interim output files
   */
  static void deleteInterim();
private:
  /**
   * \brief Create a copy of this that is empty
   * \return New empty Probability with same range bounds and times
   */
  ProbabilityMap* copyEmpty() const;
  /**
   * \brief List of sizes of IntensityMaps that have been added
   * \return List of sizes of IntensityMaps that have been added
   */
  [[nodiscard]] vector<MathSize> getSizes() const;
  /**
   * \brief Generate Statistics on sizes of IntensityMaps that have been added
   * \return Generate Statistics on sizes of IntensityMaps that have been added
   */
  [[nodiscard]] util::Statistics getStatistics() const;
  /**
   * \brief Number of sizes that have been added
   * \return Number of sizes that have been added
   */
  [[nodiscard]] size_t numSizes() const noexcept;
  /**
   * \brief Save list of sizes
   * \param base_name Base name of file to save into
   */
  void saveSizes(const string& base_name) const;
  /**
   * \brief Save map representing all intensities
   * \param base_name Base file name to save to
   * \param processing_status Stage in processing for simulations
   */
  void saveTotal(const string& base_name, const ProcessingStatus processing_status) const;
  /**
   * \brief Save map representing all intensities occurrence
   * \param base_name Base file name to save to
   */
  void saveTotalCount(const string& base_name) const;
  /**
   * \brief Make note of any interim files for later deletion
   */
  bool record_if_interim(const char* filename) const;
  /**
   * \brief Save probability file and record filename if interim
   * \return Path for file that was written
   */
  template <class R>
  string saveToProbabilityFile(const data::GridMap<size_t>& grid,
                               const string& dir,
                               const string& base_name,
                               const R divisor) const
  {
    const string filename = grid.saveToProbabilityFile(dir, base_name, divisor);
    record_if_interim(filename.c_str());
    return filename;
  };
  /**
   * \brief Directory to write outputs to
   */
  const string dir_out_;
  /**
   * \brief Map representing all intensities
   */
  data::GridMap<size_t> all_;
  /**
   * \brief List of sizes for perimeters that have been added
   */
  vector<MathSize> sizes_{};
  /**
   * \brief Time in simulation this ProbabilityMap represents
   */
  const DurationSize time_;
  /**
   * \brief Start time of simulation
   */
  const DurationSize start_time_;
  /**
   * \brief Mutex for parallel access
   */
  mutable mutex mutex_;
  /**
   * \brief Initial ignition grid to apply to outputs
   */
  const topo::Perimeter* perimeter_;
};
}
}
