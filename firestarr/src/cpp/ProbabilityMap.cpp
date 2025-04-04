/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "ProbabilityMap.h"
#include "FBP45.h"
#include "IntensityMap.h"
#include "Model.h"
#include "GridMap.h"
namespace tbd::sim
{
static constexpr size_t VALUE_UNPROCESSED = 2;
static constexpr size_t VALUE_PROCESSING = 3;
static constexpr size_t VALUE_PROCESSED = 4;

/**
 * \brief List of interim files that were saved
 */
static set<string> PATHS_INTERIM{};
static mutex PATHS_INTERIM_MUTEX{};

ProbabilityMap::ProbabilityMap(const string dir_out,
                               const DurationSize time,
                               const DurationSize start_time,
                               const int min_value,
                               const int low_max,
                               const int med_max,
                               const int max_value,
                               const data::GridBase& grid_info)
  : dir_out_(dir_out),
    all_(data::GridMap<size_t>(grid_info, 0)),
    high_(data::GridMap<size_t>(grid_info, 0)),
    med_(data::GridMap<size_t>(grid_info, 0)),
    low_(data::GridMap<size_t>(grid_info, 0)),
    time_(time),
    start_time_(start_time),
    min_value_(min_value),
    max_value_(max_value),
    low_max_(low_max),
    med_max_(med_max),
    perimeter_(nullptr)
{
}
ProbabilityMap* ProbabilityMap::copyEmpty() const
{
  return new ProbabilityMap(dir_out_,
                            time_,
                            start_time_,
                            min_value_,
                            low_max_,
                            med_max_,
                            max_value_,
                            all_);
}
void ProbabilityMap::setPerimeter(const topo::Perimeter* const perimeter)
{
  perimeter_ = perimeter;
}
void ProbabilityMap::addProbabilities(const ProbabilityMap& rhs)
{
#ifndef DEBUG_PROBABILITY
  logging::check_fatal(rhs.time_ != time_, "Wrong time");
  logging::check_fatal(rhs.start_time_ != start_time_, "Wrong start time");
  logging::check_fatal(rhs.min_value_ != min_value_, "Wrong min value");
  logging::check_fatal(rhs.max_value_ != max_value_, "Wrong max value");
  logging::check_fatal(rhs.low_max_ != low_max_, "Wrong low max value");
  logging::check_fatal(rhs.med_max_ != med_max_, "Wrong med max value");
#endif
  lock_guard<mutex> lock(mutex_);
  if (Settings::saveIntensity())
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
    static_cast<void>(util::insert_sorted(&sizes_, size));
  }
}
void ProbabilityMap::addProbability(const IntensityMap& for_time)
{
  lock_guard<mutex> lock(mutex_);
  std::for_each(
    for_time.cbegin(),
    for_time.cend(),
    [this](auto&& kv) {
      const auto k = kv.first;
      const auto v = kv.second;
      all_.data[k] += 1;
      if (Settings::saveIntensity())
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
          logging::fatal("Value %d doesn't fit into any range", v);
        }
      }
    });
  const auto size = for_time.fireSize();
  static_cast<void>(util::insert_sorted(&sizes_, size));
}
vector<MathSize> ProbabilityMap::getSizes() const
{
  return sizes_;
}
util::Statistics ProbabilityMap::getStatistics() const
{
  return util::Statistics{getSizes()};
}
size_t ProbabilityMap::numSizes() const noexcept
{
  return sizes_.size();
}
void ProbabilityMap::show() const
{
  // even if we only ran the actuals we'll still have multiple scenarios
  // with different randomThreshold values
  const auto day = static_cast<int>(time_ - floor(start_time_));
  const auto s = getStatistics();
  logging::note(
    "Fire size at end of day %d: %0.1f ha - %0.1f ha (mean %0.1f ha, median %0.1f ha)",
    day,
    s.min(),
    s.max(),
    s.mean(),
    s.median());
}
bool ProbabilityMap::record_if_interim(const char* filename) const
{
  lock_guard<mutex> lock(PATHS_INTERIM_MUTEX);
  logging::verbose("Checking if %s is interim", filename);
  if (NULL != strstr(filename, "interim_"))
  {
    logging::verbose("Recording %s as interim", filename);
    // is an interim file, so keep path for later deleting
    PATHS_INTERIM.emplace(string(filename));
    logging::check_fatal(!PATHS_INTERIM.contains(filename),
                         "Expected %s to be in interim files list",
                         filename);
    return true;
  }
  return false;
}
void ProbabilityMap::saveSizes(const string& base_name) const
{
  ofstream out;
  string filename = dir_out_ + base_name + ".csv";
  record_if_interim(filename.c_str());
  out.open(filename.c_str());
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
}
string make_string(const char* name, const tm& t, const int day)
{
  constexpr auto mask = "%s_%03d_%04d-%02d-%02d";
  char tmp[100];
  sxprintf(tmp,
           mask,
           name,
           day,
           t.tm_year + 1900,
           t.tm_mon + 1,
           t.tm_mday);
  return string(tmp);
};
void ProbabilityMap::deleteInterim()
{
  lock_guard<mutex> lock(PATHS_INTERIM_MUTEX);
  for (const auto& path : PATHS_INTERIM)
  {
    logging::debug("Removing interim file %s", path.c_str());
    if (util::file_exists(path.c_str()))
    {
      try
      {
#ifdef _WIN32
        _unlink(path.c_str());
#else
        unlink(path.c_str());
#endif
      }
      catch (const std::exception& err)
      {
        logging::error("Error trying to remove %s",
                       path.c_str());
        logging::error(err.what());
      }
    }
  }
}
void ProbabilityMap::saveAll(const tm& start_time,
                             const DurationSize time,
                             const bool is_interim) const
{
  lock_guard<mutex> lock(mutex_);
  auto t = start_time;
  auto ticks = mktime(&t);
  const auto day = static_cast<int>(round(time));
  ticks += (static_cast<size_t>(day) - t.tm_yday - 1) * DAY_SECONDS;
  t = *localtime(&ticks);
  auto fix_string = [&t, &day, &is_interim](string prefix) {
    auto text = (is_interim ? "interim_" : "") + prefix;
    return make_string(text.c_str(), t, day);
  };
  if (sim::Settings::runAsync())
  {
    vector<std::future<void>> results{};
    if (Settings::saveProbability())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveTotal,
                              this,
                              fix_string("probability"),
                              is_interim));
    }
    if (Settings::saveOccurrence())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveTotalCount,
                              this,
                              fix_string("occurrence")));
    }
    if (Settings::saveIntensity())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveLow,
                              this,
                              fix_string("intensity_L")));
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveModerate,
                              this,
                              fix_string("intensity_M")));
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveHigh,
                              this,
                              fix_string("intensity_H")));
    }
    results.push_back(async(launch::async,
                            &ProbabilityMap::saveSizes,
                            this,
                            fix_string("sizes")));
    for (auto& result : results)
    {
      result.wait();
    }
  }
  else
  {
    if (Settings::saveProbability())
    {
      saveTotal(fix_string("probability"), is_interim);
    }
    if (Settings::saveOccurrence())
    {
      saveTotalCount(fix_string("occurrence"));
    }
    if (Settings::saveIntensity())
    {
      saveLow(fix_string("intensity_L"));
      saveModerate(fix_string("intensity_M"));
      saveHigh(fix_string("intensity_H"));
    }
    saveSizes(fix_string("sizes"));
  }
}
void ProbabilityMap::saveTotal(const string& base_name, const bool is_interim) const
{
  // FIX: do this for other outputs too
  auto with_perim = all_;
  if (nullptr != perimeter_)
  {
    for (auto loc : perimeter_->burned())
    {
      // multiply initial perimeter cells so that probability shows processing status
      with_perim.data[loc] *= (is_interim ? VALUE_PROCESSING : VALUE_PROCESSED);
    }
  }
  saveToProbabilityFile<float>(with_perim, dir_out_, base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveTotalCount(const string& base_name) const
{
  saveToProbabilityFile<uint32_t>(all_, dir_out_, base_name, 1);
}
void ProbabilityMap::saveHigh(const string& base_name) const
{
  saveToProbabilityFile<float>(high_, dir_out_, base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveModerate(const string& base_name) const
{
  saveToProbabilityFile<float>(med_, dir_out_, base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveLow(const string& base_name) const
{
  saveToProbabilityFile<float>(low_, dir_out_, base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::reset()
{
  all_.clear();
  low_.clear();
  med_.clear();
  high_.clear();
  sizes_.clear();
}
}
