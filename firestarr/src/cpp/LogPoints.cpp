#include "LogPoints.h"

namespace tbd::sim
{
// FIX: clean this up but for now just hide the details from outside
class LogPoints
{
public:
  ~LogPoints()
  {
    if (NULL != log_points_)
    {
      fclose(log_points_);
      fclose(log_stages_);
    }
  }
  LogPoints(
    const string dir_out,
    bool do_log,
    size_t id,
    DurationSize start_time)
    : id_(id),
      start_time_(start_time),
      last_stage_(STAGE_INVALID),
      last_step_(std::numeric_limits<size_t>::max()),
      stage_id_()
  {
    if (do_log)
    {
      constexpr auto HEADER_STAGES = "step_id,scenario,stage,step,time\n";
#ifdef LOG_POINTS_CELL
      constexpr auto HEADER_POINTS = "step_id,column,row,x,y\n";
#else
      constexpr auto HEADER_POINTS = "step_id,x,y\n";
#endif
      char log_name[2048];
      sxprintf(log_name, "%s/scenario_%05ld_points.txt", dir_out.c_str(), id);
      log_points_ = fopen(log_name, "w");
      sxprintf(log_name, "%s/scenario_%05ld_stages.txt", dir_out.c_str(), id);
      log_stages_ = fopen(log_name, "w");
      fprintf(log_points_, HEADER_POINTS);
      fprintf(log_stages_, HEADER_STAGES);
    }
    else
    {
      static_assert(NULL == nullptr);
      log_points_ = nullptr;
      log_stages_ = nullptr;
    }
  }
  void log_point(size_t step,
                 const char stage,
                 const DurationSize time,
                 const XYSize x,
                 const XYSize y)
  {
    if (!isLogging())
    {
      return;
    }
    static const auto FMT_LOG_STAGE = "%s,%ld,%c,%ld,%f\n";
#ifdef LOG_POINTS_CELL
    static const auto FMT_LOG_POINT = "%s,%d,%d,%f,%f\n";
    const auto column = static_cast<Idx>(x);
    const auto row = static_cast<Idx>(y);
#else
    static const auto FMT_LOG_POINT = "%s,%f,%f\n";
#endif
#ifdef LOG_POINTS_RELATIVE
    constexpr auto MID = MAX_COLUMNS / 2;
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
      sxprintf(stage_id_, "%ld%c%ld", id_, stage, step);
      last_stage_ = stage;
      last_step_ = step;
#ifdef DEBUG_POINTS
      last_time_ = t;
#endif
      fprintf(log_stages_,
              FMT_LOG_STAGE,
              stage_id_,
              id_,
              stage,
              step,
              t);
#ifdef DEBUG_POINTS
    }
    else
    {
      logging::check_fatal(t != last_time_,
                           "Expected %s to have time %f but got %f",
                           stage_id_,
                           last_time_,
                           t);
#endif
    }
    fprintf(log_points_,
            FMT_LOG_POINT,
            stage_id_,
#ifdef LOG_POINTS_CELL
            column,
            row,
#endif
            static_cast<double>(p_x),
            static_cast<double>(p_y));
  }

  bool isLogging() const
  {
    return nullptr != log_points_;
  }
  void log_points(size_t step,
                  const char stage,
                  const DurationSize time,
                  const CellPoints& points)
  {
    // don't loop if not logging
    if (isLogging())
    {
      const auto u = points.unique();
#ifdef DEBUG_POINTS
      logging::check_fatal(
        u.empty(),
        "Logging empty points");
#endif
      for (const auto& p : u)
      {
        log_point(step, stage, time, p.first, p.second);
      }
    }
#ifdef DEBUG_POINTS
    else
    {
      const auto u = points.unique();
      logging::check_fatal(
        u.empty(),
        "Logging empty points");
    }
#endif
  };
private:
  size_t id_;
  DurationSize start_time_;
  char last_stage_;
  size_t last_step_;
#ifdef DEBUG_POINTS
  DurationSize last_time_;
#endif
  char stage_id_[1024];
  /**
   * \brief FILE to write logging information about points to
   */
  FILE* log_points_;
  FILE* log_stages_;
};

void no_log_point(size_t, const char, const DurationSize, const XYSize, const XYSize)
{
}

void no_log_points(size_t, const char, const DurationSize, const CellPoints&)
{
}

static unique_ptr<LogPoints> logger = nullptr;
std::function<void(size_t, const char, const DurationSize, const XYSize, const XYSize)> fct_log_point = &no_log_point;
std::function<void(size_t, const char, const DurationSize, const CellPoints&)> fct_log_points = &no_log_points;

void init_log_points(const string dir_out, bool do_log, size_t id, DurationSize start_time)
{
  using namespace std::placeholders;
  logger = make_unique<LogPoints>(dir_out, do_log, id, start_time);
  fct_log_point = std::bind(&LogPoints::log_point, &(*logger), _1, _2, _3, _4, _5);
  fct_log_points = std::bind(&LogPoints::log_points, &(*logger), _1, _2, _3, _4);
}

void log_point(size_t step, const char stage, const DurationSize time, const XYSize x, const XYSize y)
{
  fct_log_point(step, stage, time, x, y);
}

void log_points(size_t step, const char stage, const DurationSize time, const CellPoints& points)
{
  fct_log_points(step, stage, time, points);
}
}
