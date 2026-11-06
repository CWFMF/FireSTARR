/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "LogPoints.h"
#include "Log.h"
#include "unstable.h"
#define LOG_POINTS_RELATIVE
// #undef LOG_POINTS_RELATIVE
#define LOG_POINTS_CELL
#undef LOG_POINTS_CELL
namespace fs
{
// FIX: clean this up but for now just hide the details from outside
LogPoints::~LogPoints()
{
  log_points_.close();
  log_stages_.close();
}
LogPoints::LogPoints(
  const string_view output_directory,
  bool do_log,
  size_t id,
  DurationSize start_time
) noexcept
  : id_(id), start_time_(start_time), last_stage_(STAGE_INVALID),
    last_step_(std::numeric_limits<size_t>::max()), stage_id_()
{
  if (do_log)
  {
    constexpr auto HEADER_STAGES = "step_id,scenario,stage,step,time\n";
#ifdef LOG_POINTS_CELL
    constexpr auto HEADER_POINTS = "step_id,column,row,x,y\n";
#else
    constexpr auto HEADER_POINTS = "step_id,x,y\n";
#endif
    const auto file_points = std::format("{:s}/scenario_{:05d}_points.txt", output_directory, id);
    log_points_.open(file_points);
    logging::check_fatal(!log_points_.is_open(), "Unable to write to {:s}", file_points);
    const auto file_stages = std::format("{:s}/scenario_{:05d}_stages.txt", output_directory, id);
    log_stages_.open(file_stages);
    logging::check_fatal(!log_stages_.is_open(), "Unable to write to {:s}", file_stages);
    log_points_ << HEADER_POINTS;
    log_stages_ << HEADER_STAGES;
  }
}
void LogPoints::log(
  size_t step,
  const char stage,
  const DurationSize time,
  const XYSize x,
  const XYSize y
) const noexcept
{
  if (!isLogging())
  {
    return;
  }
  log_unchecked(step, stage, time, x, y);
}
void LogPoints::log_unchecked(
  size_t step,
  const char stage,
  const DurationSize time,
  const XYSize x,
  const XYSize y
) const noexcept
{
#ifdef LOG_POINTS_RELATIVE
  constexpr auto MID = MAX_WIDTH / 2;
  const auto p_x = x - MID;
  const auto p_y = y - MID;
  const auto t = time - start_time_;
#else
  const auto p_x = x;
  const auto p_y = y;
  const auto t = time;
#endif
  // time should always be the same for each step, regardless of stage
  if (last_step_ != step || last_stage_ != stage)
  {
    stage_id_ = std::format("{:d}{:c}{:d}", id_, stage, step);
    last_stage_ = stage;
    last_step_ = step;
#ifdef DEBUG_POINTS
    last_time_ = t;
#endif
    log_stages_ << std::format("{:s},{:d},{:c},{:d},{:f}\n", stage_id_, id_, stage, step, t);
#ifdef DEBUG_POINTS
  }
  else
  {
    logging::check_fatal(
      t != last_time_, "Expected {:s} to have time {:f} but got {:f}", stage_id_, last_time_, t
    );
#endif
  }
  log_points_ << std::format(
#ifdef LOG_POINTS_CELL
    "{:s},{:d},{:d},{:f},{:f}\n",
#else
    "{:s},{:f},{:f}\n",
#endif
    stage_id_,
#ifdef LOG_POINTS_CELL
    static_cast<Idx>(x),
    static_cast<Idx>(y),
#endif
    static_cast<_Float64>(p_x),
    static_cast<_Float64>(p_y)
  );
}
bool LogPoints::isLogging() const noexcept { return log_points_.is_open(); }
}
