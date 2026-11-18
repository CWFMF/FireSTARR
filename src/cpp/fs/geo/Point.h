/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_POINT_H
#define FS_POINT_H
#include "../stdafx.h"
namespace fs
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
  { }
  ~Point() noexcept = default;
  Point(const Point& rhs) noexcept = default;
  Point(Point&& rhs) noexcept = default;
  Point& operator=(const Point& rhs) noexcept = default;
  Point& operator=(Point&& rhs) noexcept = default;
  std::partial_ordering operator<=>(const Point& rhs) const noexcept = default;
  /**
   * \brief Latitude (decimal degrees)
   * \return Latitude (degrees)
   */
  [[nodiscard]] constexpr MathSize latitude() const noexcept { return latitude_; }
  /**
   * \brief Longitude (decimal degrees)
   * \return Longitude (decimal degrees)
   */
  [[nodiscard]] constexpr MathSize longitude() const noexcept { return longitude_; }

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
template <typename T>
concept LatLong = requires(const T& p) {
  { p.latitude() } noexcept -> std::same_as<fs::MathSize>;
  { p.longitude() } noexcept -> std::same_as<fs::MathSize>;
};
template <class T>
  requires LatLong<T>
struct std::formatter<T> : std::formatter<string_view>
{
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
  // apple clang didn't work with std::format_context
  template <class FormatContext>
  auto format(const T& p, FormatContext& ctx) const
  {
    std::string tmp{};
    std::format_to(
      std::back_inserter(tmp), "({:f}\u00b0, {:f}\u00b0)", p.latitude(), p.longitude()
    );
    return std::formatter<string_view>::format(tmp, ctx);
  }
};
#endif
