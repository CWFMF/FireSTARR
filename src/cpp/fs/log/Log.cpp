/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Log.h"
#include <tiffio.h>
#ifdef NDEBUG
#ifdef _WIN32
#include "../util/TimeUtil.h"
#endif
#endif
#ifndef DEBUG_NOEXCEPT_OFF
#error DEBUG_NOEXCEPT_OFF not defined
#endif
namespace fs::logging
{
// Current logging level
logging::level logging_level_ = logging::level::debug;
// do this in .cpp so that we don't get unused warnings including the .h
static const char* LOG_LABELS[] = {
  "SILENT:    ",
  "FATAL:     ",
  "ERROR:     ",
  "WARNING:   ",
  "NOTE:      ",
  "INFO:      ",
  "DEBUG:     ",
  "VERBOSE:   ",
  "EXTENSIVE: "
};
void set_log_level(const logging::level log_level) noexcept { logging_level_ = log_level; }
void increase_log_level() noexcept
{
  // HACK: make sure we never go above max
  logging_level_ =
    logging::level{min(static_cast<int>(logging::level::max), static_cast<int>(get_log_level()) + 1)
    };
}
void decrease_log_level() noexcept
{
  // HACK: make sure we never go below min
  logging_level_ =
    logging::level{max(static_cast<int>(logging::level::min), static_cast<int>(get_log_level()) - 1)
    };
}
logging::level get_log_level() noexcept { return logging_level_; }
// HACK: use function to ensure initialization
stringstream* pre_file_log() noexcept
{
  static stringstream pre_file_log_{};
  return &pre_file_log_;
}
ofstream* out_file() noexcept
{
  // closes automatically when ofstream is deconstructed
  static ofstream out_file_{};
  return &out_file_;
}
mutex mutex_{};
ostream* output_{pre_file_log()};
// HACK: rely on error handler being set when log is opened
void handle_tiff_error(const char* module, const char* const fmt, va_list args)
{
  const string tmp;
  stringstream iss(tmp);
  // HACK: just use a static size instead of trying to determine
  char buffer[1024]{0};
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
  // NOLINTNEXTLINE
  vsnprintf(buffer, std::size(buffer), fmt, args);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
  if (module && 0 < strlen(module))
  {
    iss << module << ": ";
  }
  iss << buffer;
  std::ignore = output(logging::level::error, "{}", iss.str());
}
int open_log_file(const char* filename) noexcept
{
  scoped_lock lock{mutex_};
  // turn off buffering so lines write to file immediately
  auto outfile = out_file();
  outfile->rdbuf()->pubsetbuf(nullptr, 0);
  outfile->open(filename);
  if (!outfile->is_open())
  {
    return false;
  }
  auto prefile = pre_file_log();
  const auto log_so_far = prefile->str();
  if (!log_so_far.empty())
  {
    *outfile << log_so_far;
    prefile->clear();
  }
  output_ = outfile;
  // HACK: redirect TIFF errors here since log is always opened
  TIFFSetErrorHandler(&handle_tiff_error);
  return true;
}
int close_log_file() noexcept
{
  scoped_lock lock{mutex_};
  auto outfile = out_file();
  output_ = pre_file_log();
  if (outfile->is_open())
  {
    outfile->close();
    return outfile->fail();
  }
  return 0;
}
inline string format_log_message(const char* prefix, const string msg)
{
  // do this separately from output() so we can redo it for fatal errors
  // NOTE: create string first so that entire line writes
  // (otherwise threads might mix lines)
  stringstream iss{};
  // try to make output consistent if in debug mode
#ifdef NDEBUG
  tm out{};
  const time_t now = time(nullptr);
  // NOTE: this is specifically local time for the log output
  if (!localtime_r(&now, &out))
  {
    logging::fatal("Error formatting time {}", now);
  }
  iss << put_time(&out, "[%F %T] ");
#endif
  iss << prefix << msg;
  return iss.str();
}
string output_no_check(const logging::level log_level, const string msg) DEBUG_NOEXCEPT_OFF
{
  try
  {
    auto msg_fmt = format_log_message(LOG_LABELS[static_cast<int>(log_level)], msg);
    scoped_lock lock{mutex_};
    cout << msg_fmt << "\n";
    // output is out_file_ or pre_file_log_ if not open yet
    *output_ << msg_fmt << "\n";
    return msg_fmt;
  }
  catch (const std::exception& ex)
  {
    exit(fatal(ex));
  }
}
}
