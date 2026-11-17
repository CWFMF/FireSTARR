/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LOG_H
#define FS_LOG_H
#include "stdafx.h"
#include <concepts>
#include <cstdlib>
#include <exception>
#include <format>
#ifdef NDEBUG
#define DEBUG_NOEXCEPT_OFF noexcept
#else
#define DEBUG_NOEXCEPT_OFF
#endif
namespace fs::logging
{
enum class level
{
  silent = 0,
  fatal = 1,
  error = 2,
  warning = 3,
  note = 4,
  info = 5,
  debug = 6,
  verbose = 7,
  extensive = 8,
  min = silent,
  max = extensive
};
void set_log_level(const logging::level log_level) noexcept;
void increase_log_level() noexcept;
void decrease_log_level() noexcept;
logging::level get_log_level() noexcept;
inline bool should_log(const logging::level log_level) noexcept
{
  return get_log_level() >= log_level;
}
inline bool should_not_log(const logging::level log_level) noexcept
{
  return get_log_level() < log_level;
}
int open_log_file(const char* filename) noexcept;
int close_log_file() noexcept;
// use [[nodiscard]] so we notice if output is used directly somewhere
// since we're not checking log level at this point the message can be resolved to a string
[[nodiscard]] string output_no_check(logging::level log_level, const string msg) DEBUG_NOEXCEPT_OFF;
template <typename... Args>
[[nodiscard]] inline string output_no_check(
  logging::level log_level,
  std::format_string<Args...> format,
  Args&&... args
) DEBUG_NOEXCEPT_OFF
{
  return output_no_check(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
[[nodiscard]] inline string output(
  logging::level log_level,
  std::format_string<Args...> format,
  Args&&... args
) DEBUG_NOEXCEPT_OFF
{
  if (should_not_log(log_level))
  {
    return "";
  }
  return output_no_check(log_level, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline void extensive(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::extensive, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline void verbose(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::verbose, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline void debug(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::debug, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline void info(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::info, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline void note(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::note, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline void warning(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::warning, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline void error(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::error, format, std::forward<Args>(args)...);
}
template <typename... Args>
inline int fatal(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  auto msg = output(logging::level::fatal, format, std::forward<Args>(args)...);
#ifndef NDEBUG
  // HACK: just throw the format for a start - just want to see stack traces when debugging
  throw std::runtime_error(msg);
#else
  exit(EXIT_FAILURE);
  return EXIT_FAILURE;
#endif
}
inline int fatal(const std::exception& ex) DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::fatal, "{:s}", ex.what());
#ifndef NDEBUG
  throw ex;
#else
  exit(EXIT_FAILURE);
#endif
}
template <typename... Args>
inline int fatal(const std::exception& ex, std::format_string<Args...> format, Args&&... args)
  DEBUG_NOEXCEPT_OFF
{
  std::ignore = output(logging::level::fatal, format, std::forward<Args>(args)...);
  return fatal(ex);
}
template <typename... Args>
inline constexpr void check_fatal(
  bool condition,
  std::format_string<Args...> format,
  Args&&... args
) DEBUG_NOEXCEPT_OFF
{
  if (!condition)
  {
    return;
  }
  fatal(format, std::forward<Args>(args)...);
}
template <typename T>
inline void check_equal(const T lhs, const T rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  check_fatal(lhs != rhs, "Expected {:s} to be {} but got {}", name, rhs, lhs);
}
inline void check_equal(const char* lhs, const char* rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  check_fatal(0 != strcmp(lhs, rhs), "Expected {:s} to be {:s} got {:s}", name, rhs, lhs);
}
inline void check_equal(const bool lhs, const bool rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  check_fatal(
    lhs != rhs,
    "Expected {:s} to be {:s} but got {:s}",
    name,
    lhs ? "true" : "false",
    rhs ? "true" : "false"
  );
}
template <typename T>
inline void compare_equal(const T lhs, const T rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  if (lhs != rhs)
  {
    logging::error("Expected {:s} to be {} but got {}", name, rhs, lhs);
  }
}
template <typename T>
concept HasValue = requires(const T& p) {
  { p.value } -> std::convertible_to<fs::MathSize>;
};
template <class T>
  requires HasValue<T>
inline void compare_equal(const T lhs, const T rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  compare_equal(lhs.value, rhs.value, name);
}
inline void compare_equal(const char* lhs, const char* rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  if (0 != strcmp(lhs, rhs))
  {
    logging::error("Expected {:s} to be {:s} but got {:s}", name, rhs, lhs);
  }
}
inline void compare_equal(const bool lhs, const bool rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  if (lhs != rhs)
  {
    logging::error(
      "Expected {:s} to be {:s} but got {:s}", name, lhs ? "true" : "false", rhs ? "true" : "false"
    );
  }
}
template <class V>
inline void check_equal_verbose(
  const logging::level log_level,
  const V& lhs,
  const V& rhs,
  const char* name
) DEBUG_NOEXCEPT_OFF
{
  check_equal(lhs, rhs, name);
  std::ignore = output(log_level, "{:s} matches", name);
}
inline void check_tolerance(
  const MathSize epsilon,
  const MathSize lhs,
  const MathSize rhs,
  const char* name
) DEBUG_NOEXCEPT_OFF
{
  const auto difference = abs(lhs - rhs);
  check_fatal(
    difference >= epsilon,
    "Difference too big for {:s}: ({:g} > {:g}) for {:g} vs {:g}",
    name,
    difference,
    epsilon,
    rhs,
    lhs
  );
}
}
#endif
