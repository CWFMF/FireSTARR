/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "../stdafx.h"
#include "IntensityMap.h"
#include "../sim/Model.h"
#include "../types/Location.h"
#include "../unstable.h"
#include "../util/Util.h"
#include "../wx/WeatherIndices.h"
#include "Perimeter.h"
namespace fs
{
template <class T>
std::optional<GridMap<T>> make_if_saving(const Model& model)
{
  // HACK: resolve once and fail if not set already
  static auto& settings = fs::settings::instance();
  const auto calc_fi_ros_raz = settings.save_individual || settings.save_intensity;
  if (calc_fi_ros_raz)
  {
    return model.environment().makeMap<T>(false);
  }
  return {};
}
IntensityMap::IntensityMap(const Model& model) noexcept
  : model_{model},
    // HACK: always assign this so we can use the iterator
    intensity_max_{model.environment().makeMap<IntensitySize>(false)},
    rate_of_spread_at_max_{make_if_saving<MathSize>(model)},
    direction_of_spread_at_max_{make_if_saving<DegreesSize>(model)},
    is_burned_{model.environment().unburnable()}
{ }
IntensityMap::IntensityMap(const IntensityMap& rhs)
  // : IntensityMap(rhs.model_, nullptr)
  : IntensityMap(rhs.model_)
{
  intensity_max_ = rhs.intensity_max_;
  rate_of_spread_at_max_ = rhs.rate_of_spread_at_max_;
  direction_of_spread_at_max_ = rhs.direction_of_spread_at_max_;
  is_burned_ = rhs.is_burned_;
}
void IntensityMap::applyPerimeter(const Perimeter& perimeter) noexcept
{
  std::for_each(
#if !defined(__APPLE__) || !defined(__clang__)
    // apple clang doesn't support this?
    std::execution::par_unseq,
#endif
    perimeter.burned.cbegin(),
    perimeter.burned.cend(),
    [this](const auto& location) { ignite(location); }
  );
}
bool IntensityMap::canBurn(const XYIdx& location) const { return !hasBurned(location); }
bool IntensityMap::hasBurned(const XYIdx& location) const
{
  scoped_lock lock{mutex_};
  return is_burned_.at(location);
}
bool IntensityMap::isSurrounded(const XYIdx& location) const
{
  // implement here so we can just lock once
  scoped_lock lock{mutex_};
  // FIX: same logic as makeEdge()
  const auto [x0, y0] = hash_to_xy_value(location);
  const auto min_y = static_cast<Idx>(max(y0 - 1, 0));
  const auto max_y = min(y0 + 1, this->height() - 1);
  const auto min_x = static_cast<Idx>(max(x0 - 1, 0));
  const auto max_x = min(x0 + 1, this->width() - 1);
  for (auto y1 = min_y; y1 <= max_y; ++y1)
  {
    for (auto x1 = min_x; x1 <= max_x; ++x1)
    {
      // actually check x, y too since we care if the cell itself is burned
      if (!is_burned_.at(XYIdx{x1, y1}))
      {
        return false;
      }
    }
  }
  return true;
}
void IntensityMap::ignite(const XYIdx& location) { burn(location, 1, 0, fs::Direction::Invalid()); }
void IntensityMap::burn(
  const XYIdx& location,
  IntensitySize intensity,
  MathSize ros,
  fs::Direction raz
)
{
  // HACK: resolve once and fail if not set already
  static auto& settings = fs::settings::instance();
  scoped_lock lock{mutex_};
  if (!is_burned_.at(location))
  {
    intensity_max_.set(location, intensity);
    if (rate_of_spread_at_max_.has_value())
    {
      rate_of_spread_at_max_->set(location, ros);
      direction_of_spread_at_max_->set(location, raz.asDegreesSize());
    }
    is_burned_.set(location);
  }
  // HACK: don't bother updating intensity if not saving
  else if (settings.save_intensity)
  {
    const auto intensity_old = intensity_max_.at(location);
    if (intensity_old < intensity)
    {
      intensity_max_.set(location, intensity);
    }
    if (rate_of_spread_at_max_.has_value())
    {
      // update ros and direction if higher ros
      const auto ros_old = rate_of_spread_at_max_->at(location);
      if (ros_old < ros)
      {
        rate_of_spread_at_max_->set(location, ros);
        direction_of_spread_at_max_->set(location, raz.asDegreesSize());
      }
    }
  }
}
FileList IntensityMap::save(const string_view dir, const string_view base_name) const
{
  // HACK: resolve once and fail if not set already
  static auto& settings = fs::settings::instance();
  // HACK: intensity_max_ will have values so we can iterate for probability grid, but we don't want
  // to save intensity grids
  if (!settings.save_intensity)
  {
    return FileList{};
  }
  scoped_lock lock{mutex_};
  const auto name_intensity = string(base_name) + "_intensity";
  const auto name_ros = string(base_name) + "_ros";
  const auto name_raz = string(base_name) + "_raz";
  // FIX: already done in IntensityObserver?
  auto files = intensity_max_.saveToFile(dir, name_intensity);
  // // HACK: writing a double to a tiff seems to not work?
  // double is way too much precision for outputs
  files.append_range(rate_of_spread_at_max_->saveToFile<_Float32>(dir, name_ros));
  files.append_range(direction_of_spread_at_max_->saveToFile(dir, name_raz));
  return files;
}
map<XYIdx, IntensitySize>::const_iterator IntensityMap::cend() const noexcept
{
  return intensity_max_.data.cend();
}
map<XYIdx, IntensitySize>::const_iterator IntensityMap::cbegin() const noexcept
{
  return intensity_max_.data.cbegin();
}
}
