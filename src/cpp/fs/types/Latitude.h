/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LATITUDE_H
#define FS_LATITUDE_H
#include <compare>
#include "../unstable.h"
namespace fs
{
struct Latitude
{
  MathSize value{};
  std::partial_ordering operator<=>(const Latitude& rhs) const = default;
  Latitude operator-(const Latitude& rhs) const { return {value - rhs.value}; }
  Latitude operator-() const { return {-value}; }
};
static inline Latitude abs(const Latitude& rhs) { return {std::abs(rhs.value)}; }
}
#endif
