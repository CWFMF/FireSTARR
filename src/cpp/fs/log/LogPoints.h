/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LOG_POINTS_H
#define FS_LOG_POINTS_H
#include "../stdafx.h"
namespace fs
{
constexpr auto STAGE_CONDENSE = 'C';
constexpr auto STAGE_NEW = 'N';
constexpr auto STAGE_SPREAD = 'S';
constexpr auto STAGE_INVALID = 'X';
class LogPoints
{
public:
  ~LogPoints();
  LogPoints() noexcept = default;
  LogPoints(
    const string_view output_directory,
    bool do_log,
    size_t id,
    DurationSize start_time
  ) noexcept;
  bool isLogging() const noexcept;
  void log(size_t step, const char stage, const DurationSize time, const XYSize x, const XYSize y)
    const noexcept;
  template <std::ranges::forward_range R>
  void log(size_t step, const char stage, const DurationSize time, const R& points) const noexcept
  {
    assert(!points.empty() && "Logging empty points");
    // don't loop if not logging
    if (isLogging())
    {
      for (const auto& p : points)
      {
        log_unchecked(step, stage, time, p.x.value, p.y.value);
      }
    }
  }
  void log(size_t step, const char stage, const DurationSize time, const auto& points)
    const noexcept
  {
    // don't loop if not logging
    if (isLogging())
    {
      log(step, stage, time, points.unique());
    }
  };

protected:
  void log_unchecked(
    size_t step,
    const char stage,
    const DurationSize time,
    const XYSize x,
    const XYSize y
  ) const noexcept;

private:
  size_t id_{};
  DurationSize start_time_{};
  mutable char last_stage_{};
  mutable size_t last_step_{};
#ifdef DEBUG_POINTS
  mutable DurationSize last_time_{};
#endif
  mutable string stage_id_{};
  mutable ofstream log_points_{};
  mutable ofstream log_stages_{};
};
};
#endif
