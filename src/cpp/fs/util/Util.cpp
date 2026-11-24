/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "../stdafx.h"
#include "Util.h"
#include <regex>
#include "../log/Log.h"
#include "TimeUtil.h"
namespace fs
{
FileList read_directory(const string_view name, const string_view match, const bool for_files)
{
  FileList files{};
  string full_match = ".*/" + string(match);
  logging::verbose("Matching '{:s}'", full_match);
  const std::regex re(full_match, std::regex_constants::icase);
  for (const auto& entry : std::filesystem::directory_iterator(name))
  {
    logging::verbose("Checking if file: {:s}", entry.path().string());
    if ((for_files && std::filesystem::is_regular_file(entry))
        || (!for_files && std::filesystem::is_directory(entry)))
    {
      logging::extensive("Checking regex match: {:s}", entry.path().string());
      if (std::regex_match(entry.path().string(), re))
      {
        files.emplace_back(entry
                             .path()
#ifdef _WIN32
                             .generic_string()
#endif
        );
      }
    }
  }
  return files;
}
FileList find_rasters(const string_view dir, const YearSize year)
{
  // HACK: read_directory() doesn't work if path doesn't end in '/'
  const string path = string(dir) + (dir.ends_with('/') ? "" : "/");
  const string for_year = path + to_string(year) + "/";
  const string for_default = path + "default/";
  // use first existing folder of dir/year, dir/default, or dir in that order
  const string raster_root = directory_exists(for_year.c_str())
                             ? for_year
                             : (directory_exists(for_default.c_str()) ? for_default : path);
  FileList files{};
  logging::info("Raster root is {:s}", raster_root);
  try
  {
    files.append_range(read_directory(raster_root, "fuel.*\\.tif"));
  }
  catch (const std::exception& ex)
  {
    logging::error("Unable to read directory {:s}", raster_root);
    logging::error("{:s}", ex.what());
  }
  return files;
}
bool directory_exists(const char* dir) noexcept
{
  struct stat dir_info{};
  return stat(dir, &dir_info) == 0 && dir_info.st_mode & S_IFDIR;
}
bool file_exists(const char* path) noexcept
{
  struct stat path_info{};
  // FIX: check that this works on symlinks
  return stat(path, &path_info) == 0 && path_info.st_mode & S_IFREG;
}
void make_directory(const char* dir) noexcept
{
  std::error_code ec{};
  if (std::filesystem::create_directory(dir, ec))
  {
    if (ec)
    {
      exit(logging::fatal("Error {:d} creating directory {:s}", ec.value(), dir));
    }
    struct stat dir_info{};
    if (stat(dir, &dir_info) != 0)
    {
      exit(logging::fatal("Cannot create directory {:s}", dir));
    }
    else if (dir_info.st_mode & S_IFDIR)
    {
      // everything is fine
    }
    else
    {
      exit(logging::fatal("{:s} is not a directory\n", dir));
    }
  }
}
void make_directory_recursive(string dir) noexcept
{
  const auto len = dir.length();
  if (dir[len - 1] == '/')
    dir[len - 1] = 0;
  for (auto p = &dir[0] + 1; *p; ++p)
    if (*p == '/')
    {
      *p = 0;
      make_directory(dir.c_str());
      *p = '/';
    }
  make_directory(dir.c_str());
}
UsageCount::~UsageCount() { logging::note("{:s} called {:d} times", for_what_, count_.load()); }
UsageCount::UsageCount(string for_what) noexcept : count_(0), for_what_(std::move(for_what)) { }
UsageCount& UsageCount::operator++() noexcept
{
  ++count_;
  return *this;
}
string get_canonical_path(const char* const dir_root, string path)
{
#ifdef _WIN32
  std::replace(path.begin(), path.end(), '/', '\\');
#else
  std::replace(path.begin(), path.end(), '\\', '/');
#endif
#ifdef _WIN32
  if (':' == path.at(1))
  {
    logging::note("Absolute path use on windows: {:s}", path.c_str());
  }
  else
#endif
    if (!path.starts_with('/'))
  {
    try
    {
      // not an absolute path so should be relative to root
      const pushd dir{dir_root};
      logging::note("dir_root = {:s}", dir_root);
      auto p_rel = std::filesystem::relative(path);
      logging::verbose("p_rel = {:s}", p_rel.generic_string());
      auto p_abs =
        p_rel.empty() ? dir.current_directory : std::filesystem::absolute(p_rel).generic_string();
      logging::verbose("p_abs = {:s}", p_abs);
      // auto p_can = std::filesystem::canonical(p_rel);
      // logging::note("p_can = {:s}", p_can.c_str());
      logging::debug(
        "Relative to {:s} path {:s} becomes {:s}", dir_root, p_rel.generic_string(), p_abs
        // ,
        // p_can.c_str()
      );
      path = p_abs;
      logging::info("Converted relative path to absolute path {:s}", path);
    }
    catch (std::exception&)
    {
      exit(logging::fatal("Unable to convert relative path to canonical for {:s}", path));
      path = "";
    }
  }
  return path;
}
string tolower(string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](const auto c) {
    return std::tolower(c);
  });
  return value;
}
string toupper(string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](const auto c) {
    return std::toupper(c);
  });
  return value;
}
}
