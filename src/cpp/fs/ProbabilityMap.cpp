/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "ProbabilityMap.h"
#include "GridMap.h"
#include "IntensityMap.h"
#include "TimeUtil.h"
namespace fs
{
/**
 * \brief List of interim files that were saved
 */
static set<string> PATHS_INTERIM{};
static mutex PATHS_INTERIM_MUTEX{};
ProbabilityMap::~ProbabilityMap()
{
  // make sure we don't delete if still locked
  lock_guard<mutex> lock(mutex_);
}
ProbabilityMap::ProbabilityMap(
  const DurationSize time,
  const DurationSize start_time,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value,
  const GridBase& grid_info,
  const shared_ptr<Perimeter> perimeter
)
  : all_(GridMap<size_t>(grid_info, 0)), high_(GridMap<size_t>(grid_info, 0)),
    med_(GridMap<size_t>(grid_info, 0)), low_(GridMap<size_t>(grid_info, 0)), time(time),
    start_time(start_time), min_value_(min_value), max_value_(max_value), low_max_(low_max),
    med_max_(med_max), perimeter_(perimeter)
{ }
void ProbabilityMap::addProbabilities(const ProbabilityMap& rhs)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
#ifndef DEBUG_PROBABILITY
  logging::check_fatal(rhs.time != time, "Wrong time");
  logging::check_fatal(rhs.start_time != start_time, "Wrong start time");
  logging::check_fatal(rhs.min_value_ != min_value_, "Wrong min value");
  logging::check_fatal(rhs.max_value_ != max_value_, "Wrong max value");
  logging::check_fatal(rhs.low_max_ != low_max_, "Wrong low max value");
  logging::check_fatal(rhs.med_max_ != med_max_, "Wrong med max value");
#endif
  lock_guard<mutex> lock(mutex_);
  // need to lock both maps
  lock_guard<mutex> lock_rhs(rhs.mutex_);
  if (settings.save_intensity)
  {
    for (auto&& kv : rhs.low_.data)
    {
      low_.data[kv.first] += kv.second;
    }
    for (auto&& kv : rhs.med_.data)
    {
      med_.data[kv.first] += kv.second;
    }
    for (auto&& kv : rhs.high_.data)
    {
      high_.data[kv.first] += kv.second;
    }
  }
  for (auto&& kv : rhs.all_.data)
  {
    all_.data[kv.first] += kv.second;
  }
  for (auto size : rhs.sizes_)
  {
    static_cast<void>(insert_sorted(&sizes_, size));
  }
}
void ProbabilityMap::addProbability(const IntensityMap& for_time)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  lock_guard<mutex> lock(mutex_);
  std::for_each(for_time.cbegin(), for_time.cend(), [this](auto&& kv) {
    const auto k = kv.first;
    const auto v = kv.second;
    all_.data[k] += 1;
    if (settings.save_intensity)
    {
      if (v >= min_value_ && v <= low_max_)
      {
        low_.data[k] += 1;
      }
      else if (v > low_max_ && v <= med_max_)
      {
        med_.data[k] += 1;
      }
      else if (v > med_max_ && v <= max_value_)
      {
        high_.data[k] += 1;
      }
      else
      {
        exit(logging::fatal("Value {:d} doesn't fit into any range", v));
      }
    }
  });
  const auto size = for_time.fireSize();
  static_cast<void>(insert_sorted(&sizes_, size));
}
vector<MathSize> ProbabilityMap::getSizes() const { return sizes_; }
Statistics ProbabilityMap::getStatistics() const { return Statistics{getSizes()}; }
size_t ProbabilityMap::numSizes() const noexcept { return sizes_.size(); }
void ProbabilityMap::show() const
{
  lock_guard<mutex> lock(mutex_);
  // even if we only ran the actuals we'll still have multiple scenarios
  // with different randomThreshold values
  const auto day = static_cast<int>(time - floor(start_time));
  const auto s = getStatistics();
  logging::note(
    "Fire size at end of day {:d}: {:0.1f} ha - {:0.1f} ha (mean {:0.1f} ha, median {:0.1f} ha)",
    day,
    s.min(),
    s.max(),
    s.mean(),
    s.median()
  );
}
[[nodiscard]] FileList ProbabilityMap::record_if_interim(
  FileList&& files,
  const ProcessingStatus processing_status
) const
{
  if (processed == processing_status)
  {
    return files;
  }
  lock_guard<mutex> lock_interim(PATHS_INTERIM_MUTEX);
  for (const auto& filename : files)
  {
    logging::verbose("Recording {:s} as interim", filename);
    // is an interim file, so keep path for later deleting
    PATHS_INTERIM.emplace(filename);
    logging::check_fatal(
      !PATHS_INTERIM.contains(filename), "Expected {:s} to be in interim files list", filename
    );
  }
  return files;
}
[[nodiscard]] FileList ProbabilityMap::saveSizes(
  const string_view output_directory,
  const string_view base_name,
  const ProcessingStatus processing_status
) const
{
  const auto filename = string(output_directory) + string(base_name) + ".csv";
  ofstream out{filename};
  auto sizes = getSizes();
  if (!sizes.empty())
  {
    // don't want to modify original array so that we can still lookup in correct order
    sort(sizes.begin(), sizes.end());
  }
  for (const auto& s : sizes)
  {
    out << s << "\n";
  }
  out.close();
  return record_if_interim({filename}, processing_status);
}
string make_string(const char* name, const tm& t, const int day)
{
  return std::format(
    "{:s}_{:03d}_{:04d}-{:02d}-{:02d}",
    name,
    day,
    t.tm_year + TM_YEAR_OFFSET,
    t.tm_mon + TM_MONTH_OFFSET,
    t.tm_mday
  );
};
void ProbabilityMap::deleteInterim()
{
  lock_guard<mutex> lock_interim(PATHS_INTERIM_MUTEX);
  for (const auto& path : PATHS_INTERIM)
  {
    logging::debug("Removing interim file {:s}", path);
    if (file_exists(path.c_str()))
    {
      try
      {
        std::filesystem::remove(path.c_str());
      }
      catch (const std::exception& err)
      {
        logging::error("Error trying to remove {:s}:\n{:s}", path, err.what());
      }
    }
  }
}
[[nodiscard]] FileList ProbabilityMap::saveAll(
  const string_view output_directory,
  const tm& start_time,
  const DurationSize time,
  const ProcessingStatus processing_status
) const
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  lock_guard<mutex> lock(mutex_);
  FileList files{};
  const auto is_interim = processed != processing_status;
  auto t = start_time;
  auto ticks = fix_utc(&t);
  const auto day = static_cast<int>(round(time));
  ticks += (static_cast<size_t>(day) - t.tm_yday - 1) * DAY_SECONDS;
  if (!gmtime_r(&ticks, &t))
  {
    logging::fatal("Unable to convert time from {}", ticks);
  }
  auto fix_string = [=](const string prefix) {
    const auto text = (is_interim ? "interim_" : "") + prefix;
    return make_string(text.c_str(), t, day);
  };
  if (settings.run_async)
  {
    vector<std::future<FileList>> results{};
    if (settings.save_probability)
    {
      results.push_back(async(
        launch::async,
        &ProbabilityMap::saveTotal,
        this,
        output_directory,
        fix_string("probability"),
        processing_status
      ));
    }
    if (settings.save_occurrence)
    {
      results.push_back(async(
        launch::async,
        &ProbabilityMap::saveTotalCount,
        this,
        output_directory,
        fix_string("occurrence"),
        processing_status
      ));
    }
    if (settings.save_intensity)
    {
      results.push_back(async(
        launch::async,
        &ProbabilityMap::saveLow,
        this,
        output_directory,
        fix_string("intensity_L"),
        processing_status
      ));
      results.push_back(async(
        launch::async,
        &ProbabilityMap::saveModerate,
        this,
        output_directory,
        fix_string("intensity_M"),
        processing_status
      ));
      results.push_back(async(
        launch::async,
        &ProbabilityMap::saveHigh,
        this,
        output_directory,
        fix_string("intensity_H"),
        processing_status
      ));
    }
    results.push_back(async(
      launch::async,
      &ProbabilityMap::saveSizes,
      this,
      output_directory,
      fix_string("sizes"),
      processing_status
    ));
    for (auto& result : results)
    {
      result.wait();
      files.append_range(result.get());
    }
  }
  else
  {
    if (settings.save_probability)
    {
      files.append_range(saveTotal(output_directory, fix_string("probability"), processing_status));
    }
    if (settings.save_occurrence)
    {
      files.append_range(
        saveTotalCount(output_directory, fix_string("occurrence"), processing_status)
      );
    }
    if (settings.save_intensity)
    {
      files.append_range(saveLow(output_directory, fix_string("intensity_L"), processing_status));
      files.append_range(
        saveModerate(output_directory, fix_string("intensity_M"), processing_status)
      );
      files.append_range(saveHigh(output_directory, fix_string("intensity_H"), processing_status));
    }
    files.append_range(saveSizes(output_directory, fix_string("sizes"), processing_status));
  }
  return files;
}
[[nodiscard]] FileList ProbabilityMap::saveTotal(
  const string_view output_directory,
  const string_view base_name,
  const ProcessingStatus processing_status
) const
{
  // FIX: do this for other outputs too
  auto with_perim = all_;
  if (nullptr != perimeter_)
  {
    for (auto loc : perimeter_->burned)
    {
      // multiply initial perimeter cells so that probability shows processing status
      with_perim.data[loc] = max(static_cast<size_t>(1), with_perim.data[loc]) * processing_status;
    }
  }
  return saveToProbabilityFile<_Float32>(
    with_perim, output_directory, base_name, static_cast<_Float32>(numSizes()), processing_status
  );
}
[[nodiscard]] FileList ProbabilityMap::saveTotalCount(
  const string_view output_directory,
  const string_view base_name,
  const ProcessingStatus processing_status
) const
{
  return saveToProbabilityFile<uint32_t>(all_, output_directory, base_name, 1, processing_status);
}
FileList ProbabilityMap::saveHigh(
  const string_view output_directory,
  const string_view base_name,
  const ProcessingStatus processing_status
) const
{
  return saveToProbabilityFile<_Float32>(
    high_, output_directory, base_name, static_cast<_Float32>(numSizes()), processing_status
  );
}
FileList ProbabilityMap::saveModerate(
  const string_view output_directory,
  const string_view base_name,
  const ProcessingStatus processing_status
) const
{
  return saveToProbabilityFile<_Float32>(
    med_, output_directory, base_name, static_cast<_Float32>(numSizes()), processing_status
  );
}
FileList ProbabilityMap::saveLow(
  const string_view output_directory,
  const string_view base_name,
  const ProcessingStatus processing_status
) const
{
  return saveToProbabilityFile<_Float32>(
    low_, output_directory, base_name, static_cast<_Float32>(numSizes()), processing_status
  );
}
void ProbabilityMap::reset()
{
  lock_guard<mutex> lock(mutex_);
  all_.clear();
  low_.clear();
  med_.clear();
  high_.clear();
  sizes_.clear();
}
}
