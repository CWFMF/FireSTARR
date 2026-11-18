/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_UTIL_H
#define FS_UTIL_H
#include "../stdafx.h"
#include <filesystem>
#include "../types/Radians.h"
namespace fs
{
class FileList : public vector<string>
{
public:
  using vector::vector;
// HACK: this is in C++23 but needs gcc 15
#ifndef __cpp_lib_containers_ranges
  void append_range(const FileList& rhs) noexcept { insert(end(), rhs.cbegin(), rhs.cend()); }
#endif
};
/**
 * \brief Return the passed value as given type
 * \tparam T Type to return
 * \param value Value to return
 * \return Value casted to type T
 */
template <class T>
[[nodiscard]] T no_convert(int value, int) noexcept
{
  return static_cast<T>(value);
}
/**
 * \brief Read from a stream until delimiter is found
 * \tparam Elem Elem for stream
 * \tparam Traits Traits for stream
 * \tparam Alloc Allocator for stream
 * \param stream Stream to read from
 * \param str gstring to read into
 * \param delimiter Delimiter to stop at
 * \return
 */
template <class Elem, class Traits, class Alloc>
std::basic_istream<Elem, Traits>& getline(
  std::basic_istream<Elem, Traits>* stream,
  std::basic_string<Elem, Traits, Alloc>* str,
  const Elem delimiter
)
{
  // make template so we can use pointers directly
  return getline(*stream, *str, delimiter);
}
/**
 * \brief Check if a directory exists
 * \param dir Directory to check existence of
 * \return Whether or not the directory exists
 */
[[nodiscard]] bool directory_exists(const char* dir) noexcept;
/**
 * \brief Check if a file exists
 * \param path File to check existence of
 * \return Whether or not the file exists
 */
[[nodiscard]] bool file_exists(const char* path) noexcept;
/**
 * \brief Get a list of items in the given directory matching the given regex
 * \param name Directory to search
 * \param match regular expression to match in names
 * \param for_files Match files and not directories
 * \return FileList of rasters in the directory
 */
[[nodiscard]] FileList read_directory(
  const string_view name,
  const string_view match = "*",
  const bool for_files = true
);
/**
 * \brief Get a list of rasters in the given directory for the specified year
 * \param dir Root directory to look for rasters in
 * \param year Year to use rasters for if available, else default
 * \return FileList of rasters in the directory
 */
[[nodiscard]] FileList find_rasters(const string_view dir, YearSize year);
/**
 * \brief Make the given directory if it does not exist
 * \param dir Directory to create
 */
void make_directory(const char* dir) noexcept;
/**
 * \brief Make the given directory and any parent directories that do not exist
 * \param dir Directory to create
 */
void make_directory_recursive(string dir) noexcept;
/**
 * \brief Square a number
 * \param x number to square
 * \return x squared
 */
template <class T>
[[nodiscard]] inline constexpr MathSize sq(const T& x)
{
  return static_cast<MathSize>(x) * static_cast<MathSize>(x);
}
/**
 * \brief Makes a bit mask of all 1's the specified size
 * \tparam N Number of bits
 * \tparam T Type to return
 * \return Bit mask of all 1's of specified size
 */
template <unsigned int N, class T>
[[nodiscard]] constexpr T bit_mask()
{
  return static_cast<T>(pow_int<N, int64_t>(2) - 1);
}
/**
 * \brief Round value to specified precision
 * \tparam N Precision to round to
 * \param value Value to round
 * \return Value after rounding
 */
template <unsigned int N>
[[nodiscard]] MathSize round_to_precision(const MathSize value) noexcept
{
  // HACK: this can't actually make the value be the precision we want due to
  // floating point storage, but we can round it to what it would be if it were
  // that precision
  static const auto b = pow_int<N, int64_t>(10);
  return round(value * b) / b;
}
/**
 * \brief Provides the ability to determine how many times something is used during a simulation.
 */
class UsageCount
{
  /**
   * \brief How many times this has been used
   */
  atomic<size_t> count_;
  /**
   * \brief What this is tracking usage of
   */
  string for_what_;

public:
  ~UsageCount();
  /**
   * \brief Constructor
   * \param for_what What this is tracking usage of
   */
  explicit UsageCount(string for_what) noexcept;
  UsageCount(UsageCount&& rhs) noexcept = delete;
  UsageCount(const UsageCount& rhs) noexcept = delete;
  UsageCount& operator=(UsageCount&& rhs) noexcept = delete;
  UsageCount& operator=(const UsageCount& rhs) noexcept = delete;
  /**
   * \brief Increment operator
   * \return Value after increment
   */
  UsageCount& operator++() noexcept;
};
// https://stackoverflow.com/questions/15843525/how-do-you-insert-the-value-in-a-sorted-vector
/**
 * \brief Insert value into vector in sorted order even if it is already present
 * \tparam T Type of value to insert
 * \param vec vector to insert into
 * \param item Item to insert
 * \return iterator result of insert
 */
template <typename T>
[[nodiscard]] typename std::vector<T>::iterator insert_sorted(std::vector<T>* vec, T const& item)
{
  return vec->insert(std::upper_bound(vec->begin(), vec->end(), item), item);
}
/**
 * \brief Insert value into vector in sorted order if it does not already exist
 * \tparam T Type of value to insert
 * \param vec vector to insert into
 * \param item Item to insert
 */
template <typename T>
void insert_unique(std::vector<T>* vec, T const& item)
{
  const auto i = std::lower_bound(vec->begin(), vec->end(), item);
  if (i == vec->end() || *i != item)
  {
    vec->insert(i, item);
  }
}
/**
 * \brief Do binary search over function for T that results in value
 * \tparam T Type of input values
 * \tparam V Result type of fct
 * \param lower Lower bound
 * \param upper Upper bound
 * \param value Value to look for
 * \param fct Function taking T and returning V
 * \return T value that results in closest to value
 * \return
 */
template <typename T, typename V>
T binary_find(const T lower, const T upper, const V value, const std::function<V(T)>& fct)
{
  const auto mid = lower + (upper - lower) / 2;
  if (lower == upper)
  {
    return lower;
  }
  if (fct(mid) < value)
  {
    return binary_find(lower, mid, value, fct);
  }
  return binary_find(mid + 1, upper, value, fct);
}
/**
 * \brief Check lower and upper limits before doing binary search over function for T that results
 * in value
 * \tparam T Type of input values
 * \tparam V Result type of fct
 * \param lower Lower bound to check
 * \param upper Upper bound to check
 * \param value Value to look for
 * \param fct Function taking T and returning V
 * \return T value that results in closest to value
 */
template <typename T, typename V>
T binary_find_checked(const T lower, const T upper, const V value, const std::function<V(T)>& fct)
{
  if (fct(lower) < value)
  {
    return lower;
  }
  if (fct(upper) >= value)
  {
    return upper;
  }
  return binary_find(lower, upper, value, fct);
}
struct string_hash
{
  using is_transparent = void;
  [[nodiscard]] size_t operator()(const char* txt) const
  {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(std::string_view txt) const
  {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(const std::string& txt) const
  {
    return std::hash<std::string>{}(txt);
  }
};
template <class V>
using string_map = unordered_map<string, V, string_hash, std::equal_to<>>;
/**
 * Convert circle angle to the angle that would be on an ellipse with
 * given length-to-breadth ratio
 * @param length_to_breadth length-to-breadth ratio
 * @param theta direction to convert to ellipse direction (radians)
 */
[[nodiscard]] inline Radians ellipse_angle(const MathSize length_to_breadth, const Radians& theta)
{
  return Radians{atan2(sin(theta) / length_to_breadth, cos(theta))}.fix();
}
// make a set of shared pointers that compares the underlying objects
template <class T>
struct sp_less
{
  bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const
  {
    if (!lhs || !rhs)
    {
      return !lhs && rhs;
    }
    return *lhs < *rhs;
  }
};
template <class T>
class sp_set : public std::set<shared_ptr<T>, sp_less<T>>
{
  using base = std::set<shared_ptr<T>, sp_less<T>>;

public:
  template <class... Args>
  auto sp_emplace(Args&&... args)
  {
    // make it so emplace will work on object type and not shared_ptr
    return base::emplace(new T(std::forward<Args>(args)...));
  }
};
inline string find_value(
  const string_view key,
  const string_view within,
  const string fallback = ""
)
{
  const auto c = within.find(key);
  if (c != string::npos)
  {
    const string str = &within.at(c + string(key).length());
    return str.substr(0, str.find(' '));
  }
  return fallback;
}
inline int str_to_int(const string str) { return stoi(str); }
inline MathSize str_to_value(const string str) { return static_cast<MathSize>(stod(str)); }
template <class T>
inline bool find_value(
  const string_view key,
  const string_view within,
  T* result,
  T (*convert)(const string_view str)
)
{
  const auto str = find_value(key, within);
  if (!str.empty())
  {
    *result = convert(str);
    // logging::extensive("{:s} '{:s}'\n", key, str);
    return true;
  }
  return false;
}
string get_canonical_path(const char* const dir_root, string path);
class LazyPath
{
public:
  LazyPath() = default;
  LazyPath(const string dir_root, const string path)
    : dir_root_{[&]() {
        // have to convert to absolute path or this makes no sense if current path changes
        return std::filesystem::absolute(dir_root).generic_string();
      }()},
      path_{path}
  { }
  LazyPath(const string_view dir_root, const string_view path)
    : LazyPath(string(dir_root), string(path))
  { }
  bool empty() const { return path_.empty(); }
  string intended_path() const noexcept { return std::format("{}/{}", dir_root_, path_); }
  const char* canonical() const
  {
    if (nullptr == canonical_path_)
    {
      canonical_path_ = make_unique<string>(get_canonical_path(dir_root_.c_str(), path_));
    }
    return canonical_path_->c_str();
  }
  LazyPath& operator=(const LazyPath& rhs) noexcept
  {
    dir_root_ = rhs.dir_root_;
    path_ = rhs.path_;
    canonical_path_ = nullptr;
    return *this;
  }
  LazyPath& operator=(LazyPath&& rhs) noexcept
  {
    dir_root_ = std::move(rhs.dir_root_);
    path_ = std::move(rhs.path_);
    canonical_path_ = std::move(rhs.canonical_path_);
    rhs.canonical_path_ = nullptr;
    return *this;
  }
  LazyPath& operator=(const string& path) noexcept
  {
    path_ = path;
    canonical_path_ = nullptr;
    return *this;
  }

protected:
  string dir_root_;
  string path_;
  mutable unique_ptr<string> canonical_path_{nullptr};
};
// convert string to lower case
string tolower(string value);
// convert string to upper case
string toupper(string value);
template <class V>
std::ostream& operator<<(std::ostream& os, const std::optional<V>& v)
{
  if (v.has_value())
  {
    os << v.value();
  }
  return os;
}
// go into directory and return to previous location when object is deleted
class pushd
{
public:
  const string previous_directory;
  const string current_directory;
  pushd(string path) noexcept
    : previous_directory{std::filesystem::current_path().generic_string()},
      current_directory{[&]() {
        // HACK: empty is current, but still do rest of parsing so results match that process
        if (path.empty())
        {
          path = ".";
        }
        auto p_rel = std::filesystem::relative(path);
        auto p_abs = std::filesystem::absolute(p_rel);
        auto p = p_abs.lexically_normal();
        std::filesystem::current_path(p);
        return p.generic_string();
      }()}
  { }
  ~pushd() noexcept { std::filesystem::current_path(previous_directory); }
  pushd& operator=(const pushd& rhs) noexcept = delete;
  pushd& operator=(pushd&& rhs) noexcept = delete;
  pushd(const pushd& rhs) noexcept = delete;
  pushd(pushd&& rhs) noexcept = delete;
};
}
#endif
