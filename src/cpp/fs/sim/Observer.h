/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_OBSERVER_H
#define FS_OBSERVER_H
#include "../stdafx.h"
#include "../grid/GridMap.h"
#include "Event.h"
#include "Scenario.h"
namespace fs
{
struct Event;
class Scenario;
/**
 * \brief Interface for observers that get notified when cells burn so they can track things.
 */
class IObserver
{
public:
  virtual ~IObserver() = default;
  IObserver(const IObserver& rhs) = delete;
  IObserver(IObserver&& rhs) = delete;
  IObserver& operator=(const IObserver& rhs) = delete;
  IObserver& operator=(IObserver&& rhs) = delete;
  /**
   * \brief Handle given event
   * \param event Event to handle
   * \return None
   */
  virtual void handleEvent(const Event& event) = 0;
  /**
   * \brief Save observations
   * \param dir Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  virtual FileList save(const string_view dir, const string_view base_name) const = 0;
  /**
   * \brief Clear all observations
   * \return None
   */
  virtual void reset() = 0;
  /**
   * \brief Make name to save file as
   * \param base_name Base file name
   * \param suffix Suffix to append
   * \return File name to use based on inputs
   */
  static string makeName(const string_view base_name, const string_view suffix);

protected:
  IObserver() = default;
};
/**
 * \brief An IObserver that tracks notification data using a GridMap.
 * \tparam T Type of map that is being tracked
 */
template <typename T>
class MapObserver : public IObserver
{
public:
  ~MapObserver() override = default;
  MapObserver(const MapObserver& rhs) = delete;
  MapObserver(MapObserver&& rhs) = delete;
  MapObserver& operator=(const MapObserver& rhs) = delete;
  MapObserver& operator=(MapObserver&& rhs) = delete;
  /**
   * \brief Keeps observations in a map
   * \param scenario Scenario to observe
   * \param nodata Value to use for no data
   * \param suffix Suffix to use on saved file
   */
  MapObserver(const Scenario& scenario, T nodata, string suffix)
    : map_(scenario.model().environment().makeMap<T>(nodata)), scenario_(scenario),
      suffix_(std::move(suffix))
  { }
  /**
   * \brief Function that returns the value we care about regarding the Event
   * \param event
   * \return
   */
  [[nodiscard]] virtual T getValue(const Event& event) const = 0;
  /**
   * \brief Handle given event
   * \param event Event to handle
   */
  void handleEvent(const Event& event) override { map_.set(event.xy, getValue(event)); }
  /**
   * \brief Save observations
   * \param dir Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList save(const string_view dir, const string_view base_name) const override
  {
    return map_.saveToFile(dir, makeName(base_name, suffix_));
  }
  /**
   * \brief Clear all observations
   */
  void reset() noexcept override { map_.clear(); }

protected:
  /**
   * \brief Map of observations
   */
  GridMap<T> map_{};

protected:
  /**
   * \brief Scenario being observed
   */
  const Scenario& scenario_;

private:
  /**
   * \brief Suffix to append to file during save
   */
  string suffix_;
};
/**
 * \brief Tracks when fire initially arrives in a Cell.
 */
class ArrivalObserver final : public MapObserver<DurationSize>
{
public:
  ~ArrivalObserver() override = default;
  ArrivalObserver(const ArrivalObserver& rhs) = delete;
  ArrivalObserver(ArrivalObserver&& rhs) = delete;
  ArrivalObserver& operator=(const ArrivalObserver& rhs) = delete;
  ArrivalObserver& operator=(ArrivalObserver&& rhs) = delete;
  /**
   * \brief Constructor
   * \param scenario Scenario to track
   */
  explicit ArrivalObserver(const Scenario& scenario);
  [[nodiscard]] DurationSize getValue(const Event& event) const noexcept override;
};
/**
 * \brief Tracks source Cell that fire arrived in Cell from.
 */
class SourceObserver final : public MapObserver<CellIndex>
{
public:
  ~SourceObserver() override = default;
  SourceObserver(const SourceObserver& rhs) = delete;
  SourceObserver(SourceObserver&& rhs) = delete;
  SourceObserver& operator=(const SourceObserver& rhs) = delete;
  SourceObserver& operator=(SourceObserver&& rhs) = delete;
  /**
   * \brief Constructor
   * \param scenario Scenario to track
   */
  explicit SourceObserver(const Scenario& scenario);
  [[nodiscard]] CellIndex getValue(const Event& event) const noexcept override;
};
/**
 * \brief Tracks the intensity that Cells burn at.
 */
class IntensityObserver final : public MapObserver<IntensitySize>
{
public:
  ~IntensityObserver() override = default;
  IntensityObserver(const IntensityObserver& rhs) = delete;
  IntensityObserver(IntensityObserver&& rhs) = delete;
  IntensityObserver& operator=(const IntensityObserver& rhs) = delete;
  IntensityObserver& operator=(IntensityObserver&& rhs) = delete;
  /**
   * \brief Constructor
   * \param scenario Scenario to observe
   * \param suffix Suffix to append to output file
   */
  explicit IntensityObserver(const Scenario& scenario) noexcept;
  [[nodiscard]] IntensitySize getValue(const Event& event) const noexcept override;
  /**
   * \brief Save observations
   * \param dir Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList save(const string_view dir, const string_view base_name) const override;
};
}
#endif
