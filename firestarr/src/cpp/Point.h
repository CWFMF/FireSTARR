/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
namespace fs::topo
{
/**
 * \brief A geographic location in lat/long coordinates.
 */
class Point
{
public:
  /**
   * \brief Constructor
   * \param latitude Latitude (decimal degrees)
   * \param longitude Longitude (decimal degrees)
   */
  constexpr Point(const MathSize latitude, const MathSize longitude) noexcept
    : latitude_(latitude), longitude_(longitude)
  {
  }
  ~Point() noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Point to copy from
   */
  Point(const Point& rhs) noexcept = default;
  /**
   * \brief Move constructor
   * \param rhs Point to move from
   */
  Point(Point&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs Point to copy from
   * \return This, after assignment
   */
  Point& operator=(const Point& rhs) noexcept = default;
  /**
   * \brief Move assignment
   * \param rhs Point to move from
   * \return This, after assignment
   */
  Point& operator=(Point&& rhs) noexcept = default;
  /**
   * \brief Latitude (decimal degrees)
   * \return Latitude (degrees)
   */
  [[nodiscard]] constexpr MathSize latitude() const noexcept
  {
    return latitude_;
  }
  /**
   * \brief Longitude (decimal degrees)
   * \return Longitude (decimal degrees)
   */
  [[nodiscard]] constexpr MathSize longitude() const noexcept
  {
    return longitude_;
  }
private:
  /**
   * \brief Latitude (decimal degrees)
   */
  MathSize latitude_;
  /**
   * \brief Longitude (decimal degrees)
   */
  MathSize longitude_;
};
}
