/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "Cell.h"
#include "FireSpread.h"

namespace tbd::sim
{
using topo::Cell;
using tbd::wx::Direction;
/**
 * \brief A specific Event scheduled in a specific Scenario.
 */
class Event
{
public:
  /**
   * \brief Cell representing no location
   */
  static constexpr Cell NoLocation{};
  // HACK: use type, so we can sort without having to give different times to them
  /**
   * \brief Type of Event
   */
  enum Type
  {
    SAVE,
    END_SIMULATION,
    NEW_FIRE,
    FIRE_SPREAD,
  };
  [[nodiscard]] static Event makeEvent(
    const DurationSize time,
    const Cell& cell,
    const Type type)
  {
    return {
      time,
      cell,
      0,
      type,
      0,
      0,
      Direction::Invalid,
      0};
  }
  /**
   * \brief Make simulation end event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event makeEnd(const DurationSize time)
  {
    return makeEvent(
      time,
      NoLocation,
      END_SIMULATION);
  }
  /**
   * \brief Make new fire event
   * \param time Time to schedule for
   * \param cell Cell to start new fire in
   * \return Event created
   */
  [[nodiscard]] static Event makeNewFire(
    const DurationSize time,
    const Cell& cell)
  {
    return makeEvent(
      time,
      cell,
      NEW_FIRE);
  }
  /**
   * \brief Make simulation save event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event makeSave(const DurationSize time)
  {
    return makeEvent(
      time,
      NoLocation,
      SAVE);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event makeFireSpread(const DurationSize time)
  {
    return makeEvent(
      time,
      NoLocation,
      FIRE_SPREAD);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \return Event created
   */
  [[nodiscard]] static Event makeFireSpread(
    const DurationSize time,
    const IntensitySize intensity,
    const ROSSize ros,
    const Direction raz)
  {
    return makeFireSpread(time, intensity, ros, raz, NoLocation);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \param cell Cell to spread in
   * \return Event created
   */
  [[nodiscard]] static Event makeFireSpread(
    const DurationSize time,
    const IntensitySize intensity,
    const ROSSize ros,
    const Direction raz,
    const Cell& cell)
  {
    return makeFireSpread(time, intensity, ros, raz, cell, 254);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \param cell Cell to spread in
   * \return Event created
   */
  [[nodiscard]] static Event makeFireSpread(
    const DurationSize time,
    const IntensitySize intensity,
    const ROSSize ros,
    const Direction raz,
    const Cell& cell,
    const CellIndex source)
  {
    return {time, cell, source, FIRE_SPREAD, intensity, ros, raz, 0};
  }
  ~Event() = default;
  /**
   * \brief Move constructor
   * \param rhs Event to move from
   */
  Event(Event&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Event to copy from
   */
  Event(const Event& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs Event to move from
   * \return This, after assignment
   */
  Event& operator=(Event&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs Event to copy from
   * \return This, after assignment
   */
  Event& operator=(const Event& rhs) = delete;
  /**
   * \brief Time of Event (decimal days)
   * \return Time of Event (decimal days)
   */
  [[nodiscard]] constexpr DurationSize time() const
  {
    return time_;
  }
  /**
   * \brief Type of Event
   * \return Type of Event
   */
  [[nodiscard]] constexpr Type type() const
  {
    return type_;
  }
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   * \return Duration that Event Cell has been burning (decimal days)
   */
  [[nodiscard]] constexpr DurationSize timeAtLocation() const
  {
    return time_at_location_;
  }
  /**
   * \brief Burn Intensity (kW/m)
   * \return Burn Intensity (kW/m)
   */
  [[nodiscard]] constexpr IntensitySize intensity() const
  {
    return intensity_;
  }
  /**
   * \brief Head fire spread direction
   * \return Head fire spread direction
   */
  [[nodiscard]] constexpr wx::Direction raz() const
  {
    return raz_;
  }
  /**
   * \brief Head fire rate of spread (m/min)
   * \return Head fire rate of spread (m/min)
   */
  [[nodiscard]] constexpr ROSSize ros() const
  {
    return ros_;
  }
  /**
   * \brief Cell Event takes place in
   * \return Cell Event takes place in
   */
  [[nodiscard]] constexpr const Cell& cell() const
  {
    return cell_;
  }
  /**
   * \brief CellIndex for relative Cell that spread into from
   * \return CellIndex for relative Cell that spread into from
   */
  [[nodiscard]] constexpr CellIndex source() const
  {
    return source_;
  }
private:
  /**
   * \brief Constructor
   * \param time Time to schedule for
   * \param cell CellIndex for relative Cell that spread into from
   * \param source Source that Event is coming from
   * \param type Type of Event
   * \param intensity Intensity to spread with (kW/m)
   * \param time_at_location Duration that Event Cell has been burning (decimal days)
   */
  constexpr Event(const DurationSize time,
                  const Cell& cell,
                  const CellIndex source,
                  const Type type,
                  const IntensitySize intensity,
                  const ROSSize ros,
                  const Direction raz,
                  const DurationSize time_at_location)
    : time_(time),
      time_at_location_(time_at_location),
      cell_(cell),
      type_(type),
      intensity_(intensity),
      ros_(ros),
      raz_(raz),
      source_(source)
  {
  }
  /**
   * \brief Time to schedule for
   */
  DurationSize time_;
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   */
  DurationSize time_at_location_;
  /**
   * \brief Cell to spread in
   */
  Cell cell_;
  /**
   * \brief Type of Event
   */
  Type type_;
  IntensitySize intensity_;
  ROSSize ros_;
  Direction raz_;
  // /**
  //  * \brief Spread information at time and place of event
  //  */
  // const SpreadInfo* spread_info_;
  /**
   * \brief CellIndex for relative Cell that spread into from
   */
  CellIndex source_;
};
}
