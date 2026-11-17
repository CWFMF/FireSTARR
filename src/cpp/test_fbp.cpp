/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include <future>
#include <mutex>
#include "FireSpread.h"
#include "fs/ArgumentParser.h"
#include "fs/FBP.h"
#include "fs/FuelLookup.h"
#include "fs/FuelType.h"
#include "fs/Log.h"
#include "fs/RangeIterator.h"
#include "fs/stdafx.h"
#include "test_fbp/FBPOld.h"
#include "test_fbp/FuelOldLookup.h"
namespace fs::testing
{
// FIX: this was used to compare to the old template version, but doesn't work now
//      left for reference for now so idea could be used for more tests
using fs::fuel::FuelType;
using fs::fuel::ROSOffset;
using fs::fuelold::FuelOldLookup;
using namespace fs::fuelold;
using fs::fuel::FuelLookup;
using fs::fuel::FuelType;
using fs::fuel::FuelVariable;
using fs::fuel::StandardFuel;
using TestResult = std::future<int>;
class TestResults
{
public:
  constexpr TestResults() noexcept = default;
  TestResults(const TestResults& rhs) noexcept = delete;
  TestResults(TestResults&& rhs) noexcept { add(std::move(rhs)); }
  TestResults& operator=(const TestResults& rhs) noexcept = delete;
  TestResults& operator=(TestResults&& rhs) noexcept = delete;
  void add(std::future<int>&& f) noexcept
  {
    scoped_lock lock(mutex_);
    logging::check_fatal(finalized_, "TestResults were already finalized but trying to add more");
    results_.push_back(std::move(f));
  };
  void add(const auto& fct, auto... args) noexcept
  {
    // don't lock this until results are calculated
    add(std::async(launch::async, fct, args...));
  };
  void add(TestResults&& rhs) noexcept
  {
    scoped_lock lock(mutex_, rhs.mutex_);
    logging::check_fatal(finalized_, "TestResults were already finalized but trying to add more");
    if (rhs.finalized_)
    {
      // if finalized then results were cleared
      logging::check_fatal(
        !rhs.results_.empty(), "TestResults were finalized but still have results to check"
      );
      // keep first non-zero value
      if (0 == return_value_)
      {
        return_value_ = rhs.return_value_;
      }
    }
    else
    {
      std::move(rhs.results_.begin(), rhs.results_.end(), results_.end());
      rhs.finalized_ = true;
      rhs.results_.clear();
    }
  };
  std::future<int> value() noexcept
  {
    // don't lock here since calculate_value() needs the lock there
    return std::async(launch::async, &TestResults::calculate_value, this);
  }

private:
  int calculate_value() noexcept
  {
    scoped_lock lock(mutex_);
    if (!finalized_)
    {
      for (auto& result : results_)
      {
        result.wait();
        if (auto cmp = result.get(); 0 == return_value_ && 0 != cmp)
        {
          // HACK: keep first value to return but need to wait for every future
          return_value_ = cmp;
        }
      }
      logging::debug("Have {:d} results", results_.size());
      finalized_ = true;
      results_.clear();
    }
    return return_value_;
  }
  mutable mutex mutex_{};
  std::vector<TestResult> results_{};
  bool finalized_{false};
  int return_value_{0};
};
// check %, so 1 decimal is fine
static constexpr auto EPSILON = static_cast<MathSize>(1e-1);
auto check_equal(const auto& lhs, const auto& rhs, const char* name)
{
  logging::check_equal_verbose(logging::level::debug, lhs, rhs, name);
}
auto compare_fuel_valid(
  const string name,
  const FuelType* f_a,
  const FuelType* f_b,
  const char* msg = ""
)
{
  const FuelType& a = *f_a;
  const FuelType& b = *f_b;
  logging::info("Checking {:s}: {:s}", name, msg);
  //
  // FuelType
  //
  const auto a0 = a.isValid();
  const auto b0 = b.isValid();
  check_equal(a0, b0, "isValid");
  check_equal(a.name(), b.name(), "name");
  check_equal(a.code(), b.code(), "code");
  return 0;
}
// use vectors so FuelCompareOptions can assign any of these directly
static const auto BUI_RANGE_DEFAULTS = range(0.0, 300.0, 13.0);
static const auto DC_RANGE_DEFAULTS = range(0.0, 1000.0, 13.0);
static const vector<MathSize> DC_VALUES_GRASS{0, 10, 50, 100, 400, 499, 500, 501, 1000};
static const auto RANGE_MC_FRACTION = range(-1, 3, 0.011);
static const auto RANGE_WIND_SPEED = range(0, 200, 0.07);
static const auto RANGE_BUI_EFFECT = range(-1, 300, 0.7);
static const auto RANGE_CFB = range(0, 100, 0.03);
static const auto RANGE_ISI = range(0, 250, 0.3);
struct FuelCompareOptions
{
  // HACK: can't figure out how to refer to a range so just use vectors
  // single values for defaults
  const vector<int> nd_values{80};
  const vector<MathSize> bui_values{60};
  const vector<MathSize> dc_values{200};
};
static const FuelCompareOptions FUEL_COMPARE_DEFAULT{};
static const FuelCompareOptions FUEL_COMPARE_DECIDUOUS{.bui_values = BUI_RANGE_DEFAULTS};
auto compare_spread(
  const string name,
  const FuelType* a,
  const FuelType* b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  static const MathSize CELL_SIZE{100.0};
  static const vector<SlopeSize> slopes{0, 15, 30};
  static const vector<AspectSize> aspects{0, 15, 25, 35, 45, 55};
  // duration is in minutes
  auto show_offset = [=](const ROSOffset& o) {
    const auto intensity{o.intensity};
    const auto ros{o.ros};
    const auto direction{o.raz};
    // offsets are in fraction of a cell per minute
    const auto offset{o.offset};
    cout << std::format(
      " ({:d} kW/m; {:0.100g} m/min @{:03d} == ({:0.100g}, {:0.100g}))\n",
      intensity,
      ros,
      direction.asDegreesSize(),
      offset.x * CELL_SIZE,
      offset.y * CELL_SIZE
    );
  };
  static const auto FFMC_RANGE = range(0.0, 101.0, 1.0);
  static const auto DMC_SMALL_RANGE = range(0.0, 200.0, 47.0);
  static const auto DMC_RANGE = range(0.0, 200.0, 7.0);
  size_t count_comparisons{0};
  logging::debug(
    "compare_spread({:s}, {:s}, {:s})", name.c_str(), FuelType::safeName(a), FuelType::safeName(b)
  );
  // HACK: use less options for things with nd values (just grass?)
  const auto dmc_values = options.nd_values.size() > 1 ? DMC_SMALL_RANGE : DMC_RANGE;
  static auto it = [&]() {
    vector<std::tuple<int, MathSize, MathSize, MathSize, short, short>> results{};
    for (auto nd : options.nd_values)
    {
      logging::extensive("nd {:d}", nd);
      for (auto ffmc : FFMC_RANGE)
      {
        logging::extensive("ffmc {:0.100g}", ffmc);
        for (auto dmc : dmc_values)
        {
          logging::extensive("dmc {:0.100g}", dmc);
          // for (auto bui : options.bui_values)
          {
            for (auto dc : options.dc_values)
            // for (auto dc : range(0.0, std::ranges::max(options.dc_values), 17.0))
            {
              logging::extensive("dc {:0.100g}", dc);
              for (auto slope : slopes)
              {
                logging::extensive("slope {:d}", slope);
                for (auto aspect : aspects)
                {
                  results.emplace_back(nd, ffmc, dmc, dc, slope, aspect);
                }
              }
            }
          }
        }
      }
    }
    return results;
  }();
  return std::transform_reduce(
#if !defined(__APPLE__) || !defined(__clang__)
    // apple clang doesn't support this?
    std::execution::par_unseq,
#endif
    it.begin(),
    it.end(),
    0,
    [](const int a, const int b) {
      // keep first non-zero value
      return 0 == a ? b : a;
    },
    [&](const auto& v) {
      static const DurationSize TIME{INVALID_TIME};
      // HACK: 0.0 is causing offsets to be generated in grass
      static const MathSize MIN_ROS{1E-6};
      auto& [nd, ffmc, dmc, dc, slope, aspect] = v;
      const FwiWeather weather{Weather::Invalid(), Ffmc{ffmc}, Dmc{dmc}, Dc{dc}};
      ++count_comparisons;
      logging::extensive("aspect {:d}", aspect);
      // HACK: this constructor ignores fuel part of this
      const auto key = Cell::key(Cell::hashCell(slope, aspect, 0));
      const SpreadInfo spread_a{a, TIME, MIN_ROS, CELL_SIZE, key, nd, &weather, &weather};
      const SpreadInfo spread_b{b, TIME, MIN_ROS, CELL_SIZE, key, nd, &weather, &weather};
      const auto offsets_a = spread_a.offsets();
      const auto offsets_b = spread_b.offsets();
      const auto head_ros = spread_a.headRos();
      static constexpr MathSize ROS_MINIMAL{1.0};
      logging::verbose(
        "compare_spread() [{:d}] {:s}spreading for ffmc:{:0.100g}; dmc:{:0.100g}; dc:{:0.100g}; nd:{:d}; slope:{:d}; aspect: {:d}",
        count_comparisons,
        spread_a.isNotSpreading() ? "not "
        : head_ros < ROS_MINIMAL  ? "minimal "
                                  : "",
        ffmc,
        dmc,
        dc,
        nd,
        slope,
        aspect
      );
      const auto show_offsets =
        logging::should_log(logging::level::verbose) && head_ros >= ROS_MINIMAL;
      if (offsets_a.size() != offsets_b.size())
      {
        logging::error(
          "compare_spread() size failed for name: {:s}; ffmc:{:0.100g}; dmc:{:0.100g}; dc:{:0.100g}; nd:{:d}; slope:{:d}; aspect: {:d}",
          name.c_str(),
          ffmc,
          dmc,
          dc,
          nd,
          slope,
          aspect
        );
        if (offsets_a.size() < offsets_b.size())
        {
          logging::error("compare_spread() size == -1");
          return -1;
        }
        if (offsets_a.size() > offsets_b.size())
        {
          logging::error("compare_spread() size == 1");
          return 1;
        }
      }
      if (show_offsets)
      {
        cout << "Offsets are: [";
      }
      for (size_t i = 0; i < offsets_a.size(); ++i)
      {
        const auto pt_a{offsets_a.at(i)};
        const auto pt_b{offsets_b.at(i)};
        if (show_offsets)
        {
          show_offset(pt_a);
        }
        if (auto cmp_pt = pt_a <=> pt_b; 0 != cmp_pt)
        {
          if (!show_offsets)
          {
            show_offset(pt_a);
          }
          cout << " != ";
          show_offset(pt_b);
          cout << "\n";
          logging::error(
            "compare_spread() pt failed for name: {:s}; ffmc:{:0.100g}; dmc:{:0.100g}; dc:{:0.100g}; nd:{:d}; slope:{:d}; aspect: {:d}",
            name.c_str(),
            ffmc,
            dmc,
            dc,
            nd,
            slope,
            aspect
          );
          if (std::weak_ordering::less == cmp_pt)
          {
            logging::error("compare_spread() pt == -1");
            return -1;
          }
          if (std::weak_ordering::greater == cmp_pt)
          {
            logging::error("compare_spread() pt == 1");
            return 1;
          }
        }
      }
      if (show_offsets)
      {
        cout << "]\n";
      }
      logging::verbose("compare_spread() == 0 with {:d} comparisons", count_comparisons);
      return 0;
    }
  );
}
auto compare_fuel_basic(
  const string name,
  const FuelType* f_a,
  const FuelType* f_b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  if (nullptr == f_a)
  {
    logging::error("Invalid FuelTypeA");
    return -1;
  }
  if (nullptr == f_b)
  {
    logging::error("Invalid FuelTypeB");
    return 1;
  }
  const FuelType& a = *f_a;
  const FuelType& b = *f_b;
  const auto n_nd = options.nd_values.size();
  const auto n_bui = options.bui_values.size();
  const auto n_dc = options.dc_values.size();
  const auto n_isi = RANGE_ISI.size();
  static const auto n_mc_fraction = RANGE_MC_FRACTION.size();
  static const auto n_wind_speed = RANGE_WIND_SPEED.size();
  static const auto n_bui_effect = RANGE_BUI_EFFECT.size();
  static const auto n_cfb = RANGE_CFB.size();
  const auto n_loop = n_nd * n_bui * n_dc * n_isi;
  const auto n_total = n_loop + n_mc_fraction + n_wind_speed + n_bui_effect + n_cfb;
  const auto msg = std::format(
    "(({} nds X {} buis X {} dcs X {} isis) + {} mc_fractions + {} wind_speeds' + {} bui_effects + {} cfbs = {} combinations",
    n_nd,
    n_bui,
    n_dc,
    n_isi,
    n_mc_fraction,
    n_wind_speed,
    n_bui_effect,
    n_cfb,
    n_total
  );
  if (const auto cmp = compare_fuel_valid(name, f_a, f_b, msg.c_str()); 0 != cmp)
  {
    return cmp;
  }
  //
  // FuelType
  //
  // check_equal(a.isValid(), b.isValid(), "isValid");
  check_equal(FuelType::safeCode(&a), FuelType::safeCode(&b), "safeCode");
  check_equal(FuelType::safeName(&a), FuelType::safeName(&b), "safeName");
  // static constexpr MathSize criticalRos(const MathSize sfc, const MathSize csi)
  // static constexpr bool isCrown(const MathSize csi, const MathSize sfi)
  check_equal(a.cfl(), b.cfl(), "cfl");
  check_equal(a.canCrown(), b.canCrown(), "canCrown");
  // MathSize grass_curing(const int, const FwiWeather&) const
  check_equal(a.cbh(), b.cbh(), "cbh");
  // MathSize crownFractionBurned(MathSize rss, MathSize rso) const noexcept
  check_range(
    "probabilityPeat()",
    "mc_fraction",
    [&](const auto& v) { return a.probabilityPeat(v); },
    [&](const auto& v) { return b.probabilityPeat(v); },
    EPSILON,
    RANGE_MC_FRACTION
  );
  // ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept
  check_range(
    "buiEffect()",
    "bui",
    [&](const auto& v) { return a.buiEffect(v); },
    [&](const auto& v) { return b.buiEffect(v); },
    EPSILON,
    RANGE_BUI_EFFECT
  );
  check_range(
    "crownConsumption()",
    "cfb",
    [&](const auto& v) { return a.crownConsumption(v); },
    [&](const auto& v) { return b.crownConsumption(v); },
    EPSILON,
    RANGE_CFB
  );
  // MathSize calculateRos(int nd, const FwiWeather& wx, MathSize isi) const
  // need to check breakpoints
  // - BUI 80 (D2)
  // - DC 500 (O1)
  // FIX: use some weird increments to do less but not always have __0.0
  static auto it_nds = [&]() {
    vector<std::tuple<int, MathSize, MathSize>> results{};
    for (auto nd : options.nd_values)
    {
      for (auto bui : options.bui_values)
      {
        // logging::verbose("bui {:0.100g}", bui);
        // for (auto dc : range(0.0, 2000.0, 7.0))
        for (auto dc : options.dc_values)
        {
          results.emplace_back(nd, bui, dc);
        }
      }
    }
    return results;
  }();
  std::for_each(
#if !defined(__APPLE__) || !defined(__clang__)
    // apple clang doesn't support this?
    std::execution::par_unseq,
#endif
    it_nds.begin(),
    it_nds.end(),
    [&](const auto& v) {
      auto& [nd_binding, bui, dc] = v;
      // HACK: avoid 'reference to local binding 'nd' declared in enclosing lambda expression'
      auto nd = nd_binding;
      // logging::verbose("dc {:0.100g}", dc);
      const FwiWeather wx{
        Weather::Zero(), Ffmc::Zero(), Dmc::Zero(), Dc{dc}, Isi::Zero(), Bui{bui}, Fwi::Zero()
      };
      const string msg = logging::should_log(logging::level::verbose)
                         ? std::format("calculateRos(nd={}, bui={}, dc={})", nd, bui, dc)
                         : "calculateRos()";
      check_range(
        msg.c_str(),
        "isi",
        [=, &a](const auto& v) { return a.calculateRos(nd, wx, v); },
        [=, &b](const auto& v) { return b.calculateRos(nd, wx, v); },
        EPSILON,
        RANGE_ISI
      );
    }
  );
  // MathSize calculateIsf(const SpreadInfo& spread, MathSize isi)
  // MathSize surfaceFuelConsumption(const SpreadInfo& spread) const
  check_range(
    "lengthToBreadth()",
    "ws",
    [=, &a](const auto& v) { return a.lengthToBreadth(v); },
    [=, &b](const auto& v) { return b.lengthToBreadth(v); },
    EPSILON,
    RANGE_WIND_SPEED
  );
  // MathSize finalRos(const SpreadInfo& spread, MathSize isi, MathSize cfb, MathSize rss) const
  // MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const
  // check_equal(a.name(), b.name(), "name");
  // check_equal(a.code(), b.code(), "code");
  // return 0;
  return compare_spread(
    name, static_cast<const FuelType*>(&a), static_cast<const FuelType*>(&b), options
  );
}
template <class TypeB>
auto compare_fuel(
  const string name,
  const FuelType* f_a,
  const FuelType* f_b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  using TypeA = StandardFuel;
  // so we don't need dynamic_cast in call
  const TypeA& a = *dynamic_cast<const TypeA*>(f_a);
  const TypeB& b = *dynamic_cast<const TypeB*>(f_b);
  if (const auto cmp = compare_fuel_basic(name, f_a, f_b, options); 0 != cmp)
  {
    return cmp;
  }
  //
  // FuelBase
  //
  check_equal(a.bulkDensity(), b.bulkDensity(), "bulkDensity");
  check_equal(a.inorganicPercent(), b.inorganicPercent(), "inorganicPercent");
  check_equal(a.duffDepth(), b.duffDepth(), "duffDepth");
  testing::compare_duff("duffDmcType", *a.duffDmcType(), *b.duffDmcType(), logging::level::debug);
  testing::compare_duff(
    "duffFfmcType", *a.duffFfmcType(), *b.duffFfmcType(), logging::level::debug
  );
  check_equal(a.ffmcRatio(), b.ffmcRatio(), "ffmcRatio");
  check_equal(a.dmcRatio(), b.dmcRatio(), "dmcRatio");
  //
  // StandardFuel
  //
  check_range(
    "rosBasic()",
    "isi",
    [=, &a](const auto& v) { return a.rosBasic(v); },
    [=, &b](const auto& v) { return b.rosBasic(v); },
    EPSILON,
    RANGE_ISI
  );
  // MathSize limitIsf(const MathSize mu, const MathSize rsf) const noexcept
  check_equal(a.bui0(), b.bui0(), "bui0");
  check_equal(a.a(), b.a(), "a");
  check_equal(a.negB(), b.negB(), "negB");
  check_equal(a.c(), b.c(), "c");
  // static constexpr MathSize crownRateOfSpread(const MathSize isi, const MathSize fmc)
  // noexcept
  check_equal(a.logQ(), b.logQ(), "logQ");
  return 0;
}
template <class TypeB>
auto compare_fuel_variable(
  const string name,
  const FuelType* f_a,
  const FuelType* f_b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  using TypeA = FuelVariable;
  // so we don't need dynamic_cast in call
  const TypeA& a = *dynamic_cast<const TypeA*>(f_a);
  const TypeB& b = *dynamic_cast<const TypeB*>(f_b);
  assert(a.summer() != a.spring());
  // FIX: calling functions of FuelVariable should throw, but don't bother checking that
  if (const auto cmp = compare_fuel_basic(name, f_a->summer(), f_b->summer(), options); 0 != cmp)
  {
    return cmp;
  }
  return compare_fuel_basic(name, a.spring(), b.spring(), options);
}
// static constexpr FuelCompareOptions FUEL_COMPARE_DECIDUOUS{.dc_values = DC_DEFAULT_SINGLE};
vector<int> find_nd_values()
{
  using namespace fs::fuelold;
  // CHECK: FIX: how are nd values 400+?
  set<int> nd_ref_values{};
  set<int> nd_values{};
  static constexpr MathSize BOUNDS_CANADA_LAT_MIN = 41;
  static constexpr MathSize BOUNDS_CANADA_LAT_MAX = 84;
  static constexpr MathSize BOUNDS_CANADA_LON_MIN = -141;
  static constexpr MathSize BOUNDS_CANADA_LON_MAX = -52;
  // FIX: use some weird increments to do less but not always have __.0
  static constexpr MathSize DEGREE_INCREMENT = 0.7;
  // static constexpr MathSize ELEVATION_CANADA_MAX = 5959;
  static constexpr MathSize ELEVATION_EARTH_MIN = -418;
  static constexpr MathSize ELEVATION_EARTH_MAX = 8848;
  static constexpr MathSize ELEVATION_INCREMENT = 113;
  // - nd for different latitudes
  //   - elevation 0
  // const auto latitudes = range(-90.0, 90.0, DEGREE_INCREMENT);
  const auto latitudes = range(BOUNDS_CANADA_LAT_MIN, BOUNDS_CANADA_LAT_MAX, DEGREE_INCREMENT);
  // const auto longitudes = range(-180.0, 180.0, DEGREE_INCREMENT);
  const auto longitudes = range(BOUNDS_CANADA_LON_MIN, BOUNDS_CANADA_LON_MAX, DEGREE_INCREMENT);
  const auto elevations = range(ELEVATION_EARTH_MIN, ELEVATION_EARTH_MAX, ELEVATION_INCREMENT);
  logging::info(
    "Checking ({:d} latitudes X {:d} longitudes X {:d} elevations) = {:d} combinations",
    latitudes.size(),
    longitudes.size(),
    elevations.size(),
    latitudes.size() * longitudes.size() * elevations.size()
  );
  for (auto latitude : latitudes)
  {
    for (auto longitude : longitudes)
    {
      for (auto elevation : elevations)
      {
        const Point pt{latitude, longitude};
        const auto nd_ref = calculate_nd_ref_for_point(elevation, pt);
        nd_ref_values.emplace(nd_ref);
        logging::verbose(
          "now have {:d} values for nd: {}, {:0.100g} gives nd_ref {:d}",
          nd_ref_values.size(),
          pt,
          elevation,
          nd_ref
        );
      }
    }
  }
  for (int day : range_int(0, 366, 1))
  {
    for (auto nd_ref : nd_ref_values)
    {
      logging::verbose("jd {:d}", day);
      // from calculate_nd_for_point(const Day day, const int elevation, const Point& point)
      const auto nd = static_cast<int>(abs(day - nd_ref));
      nd_values.emplace(nd);
    }
  }
  auto min_nd = std::numeric_limits<int>::max();
  auto max_nd = std::numeric_limits<int>::min();
  for (auto nd : nd_values)
  {
    min_nd = min(min_nd, nd);
    max_nd = max(max_nd, nd);
  }
  const bool is_consecutive{static_cast<size_t>(max_nd - min_nd + 1) == nd_values.size()};
  logging::info(
    "Have {:d} nd values between {:d} and {:d} that are {:s}",
    nd_values.size(),
    min_nd,
    max_nd,
    is_consecutive ? "consecutive" : "non-consecutive"
  );
  // pick every Nth item
  vector<int> result{};
  constexpr auto N = 3;
  size_t i = 0;
  for (auto v : nd_values)
  {
    if (0 == (i % N))
    {
      result.emplace_back(v);
    }
    ++i;
  }
  return result;
}
auto compare_fuel_valid_by_index(const size_t i, const string name, const char* msg = "")
{
  return compare_fuel_valid(name, FuelLookup::Fuels[i], FuelOldLookup::Fuels[i], msg);
}
template <class TypeB>
auto compare_fuel_by_index(const size_t i, const string name)
{
  return compare_fuel<TypeB>(name, FuelLookup::Fuels[i], FuelOldLookup::Fuels[i]);
}
template <class TypeB>
auto compare_fuel_variable_by_index(const size_t i, const string name)
{
  return compare_fuel_variable<TypeB>(name, FuelLookup::Fuels[i], FuelOldLookup::Fuels[i]);
}
template <class TypeB>
auto compare_fuel_by_index_options(
  const size_t i,
  const string name,
  const FuelCompareOptions options
)
{
  // HACK: can't get reference properly when options has default value
  return compare_fuel<TypeB>(name, FuelLookup::Fuels[i], FuelOldLookup::Fuels[i], options);
}
template <class TypeB>
auto compare_fuel_variable_by_index_options(
  const size_t i,
  const string name,
  const FuelCompareOptions options
)
{
  // HACK: can't get reference properly when options has default value
  return compare_fuel_variable<TypeB>(name, FuelLookup::Fuels[i], FuelOldLookup::Fuels[i], options);
}
int test_fbp(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing FBP");
  const auto nd_all_values = find_nd_values();
  // HACK: initialize here so nd_all_values is set
  static const FuelCompareOptions FUEL_COMPARE_GRASS{
    .nd_values = nd_all_values, .dc_values = DC_VALUES_GRASS
  };
  // for (size_t i = 0; i < FuelOldLookup::Fuels.size(); ++i)
  // {
  //   auto& a = *fuel::Fuels[i];
  //   auto& b = *dynamic_cast<fs::FuelBase*>(FuelOldLookup::Fuels[i]);
  //   compare(a.name(), a, b);
  //   // compare("", *fuel::Fuels[i], *FuelOldLookup::Fuels[i]);
  // }
  TestResults results{};
  // HACK: keep i in here so we don't need to add it incrementing to all calls
  size_t i = 0;
  auto add_test = [&](const auto& fct, auto... args) {
    // HACK: increment i in here so we don't need to add it incrementing to all calls
    results.add(fct, i++, args...);
  };
  add_test(&compare_fuel_valid_by_index, "Non-fuel", "basic test only");
  add_test(&compare_fuel_valid_by_index, "Invalid", "basic test only");
  add_test(&compare_fuel_by_index<FuelOldC1>, "C1");
  add_test(&compare_fuel_by_index<FuelOldC2>, "C2");
  add_test(&compare_fuel_by_index<FuelOldC3>, "C3");
  add_test(&compare_fuel_by_index<FuelOldC4>, "C4");
  add_test(&compare_fuel_by_index<FuelOldC5>, "C5");
  add_test(&compare_fuel_by_index<FuelOldC6>, "C6");
  add_test(&compare_fuel_by_index<FuelOldC7>, "C7");
  add_test(&compare_fuel_by_index_options<FuelOldD1>, "D1", FUEL_COMPARE_DECIDUOUS);
  add_test(&compare_fuel_by_index_options<FuelOldD2>, "D2", FUEL_COMPARE_DECIDUOUS);
  add_test(&compare_fuel_by_index_options<FuelOldO1A>, "O1_A", FUEL_COMPARE_GRASS);
  add_test(&compare_fuel_by_index_options<FuelOldO1B>, "O1_B", FUEL_COMPARE_GRASS);
  add_test(&compare_fuel_by_index<FuelOldS1>, "S1");
  add_test(&compare_fuel_by_index<FuelOldS2>, "S2");
  add_test(&compare_fuel_by_index<FuelOldS3>, "S3");
  add_test(&compare_fuel_variable_by_index_options<FuelOldD1D2>, "D1_D2", FUEL_COMPARE_DECIDUOUS);
  add_test(&compare_fuel_by_index<FuelOldM1<5>>, "M1_05");
  add_test(&compare_fuel_by_index<FuelOldM1<10>>, "M1_10");
  add_test(&compare_fuel_by_index<FuelOldM1<15>>, "M1_15");
  add_test(&compare_fuel_by_index<FuelOldM1<20>>, "M1_20");
  add_test(&compare_fuel_by_index<FuelOldM1<25>>, "M1_25");
  add_test(&compare_fuel_by_index<FuelOldM1<30>>, "M1_30");
  add_test(&compare_fuel_by_index<FuelOldM1<35>>, "M1_35");
  add_test(&compare_fuel_by_index<FuelOldM1<40>>, "M1_40");
  add_test(&compare_fuel_by_index<FuelOldM1<45>>, "M1_45");
  add_test(&compare_fuel_by_index<FuelOldM1<50>>, "M1_50");
  add_test(&compare_fuel_by_index<FuelOldM1<55>>, "M1_55");
  add_test(&compare_fuel_by_index<FuelOldM1<60>>, "M1_60");
  add_test(&compare_fuel_by_index<FuelOldM1<65>>, "M1_65");
  add_test(&compare_fuel_by_index<FuelOldM1<70>>, "M1_70");
  add_test(&compare_fuel_by_index<FuelOldM1<75>>, "M1_75");
  add_test(&compare_fuel_by_index<FuelOldM1<80>>, "M1_80");
  add_test(&compare_fuel_by_index<FuelOldM1<85>>, "M1_85");
  add_test(&compare_fuel_by_index<FuelOldM1<90>>, "M1_90");
  add_test(&compare_fuel_by_index<FuelOldM1<95>>, "M1_95");
  add_test(&compare_fuel_by_index<FuelOldM2<5>>, "M2_05");
  add_test(&compare_fuel_by_index<FuelOldM2<10>>, "M2_10");
  add_test(&compare_fuel_by_index<FuelOldM2<15>>, "M2_15");
  add_test(&compare_fuel_by_index<FuelOldM2<20>>, "M2_20");
  add_test(&compare_fuel_by_index<FuelOldM2<25>>, "M2_25");
  add_test(&compare_fuel_by_index<FuelOldM2<30>>, "M2_30");
  add_test(&compare_fuel_by_index<FuelOldM2<35>>, "M2_35");
  add_test(&compare_fuel_by_index<FuelOldM2<40>>, "M2_40");
  add_test(&compare_fuel_by_index<FuelOldM2<45>>, "M2_45");
  add_test(&compare_fuel_by_index<FuelOldM2<50>>, "M2_50");
  add_test(&compare_fuel_by_index<FuelOldM2<55>>, "M2_55");
  add_test(&compare_fuel_by_index<FuelOldM2<60>>, "M2_60");
  add_test(&compare_fuel_by_index<FuelOldM2<65>>, "M2_65");
  add_test(&compare_fuel_by_index<FuelOldM2<70>>, "M2_70");
  add_test(&compare_fuel_by_index<FuelOldM2<75>>, "M2_75");
  add_test(&compare_fuel_by_index<FuelOldM2<80>>, "M2_80");
  add_test(&compare_fuel_by_index<FuelOldM2<85>>, "M2_85");
  add_test(&compare_fuel_by_index<FuelOldM2<90>>, "M2_90");
  add_test(&compare_fuel_by_index<FuelOldM2<95>>, "M2_95");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<5>>, "M1_M2_05");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<10>>, "M1_M2_10");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<15>>, "M1_M2_15");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<20>>, "M1_M2_20");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<25>>, "M1_M2_25");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<30>>, "M1_M2_30");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<35>>, "M1_M2_35");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<40>>, "M1_M2_40");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<45>>, "M1_M2_45");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<50>>, "M1_M2_50");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<55>>, "M1_M2_55");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<60>>, "M1_M2_60");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<65>>, "M1_M2_65");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<70>>, "M1_M2_70");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<75>>, "M1_M2_75");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<80>>, "M1_M2_80");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<85>>, "M1_M2_85");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<90>>, "M1_M2_90");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<95>>, "M1_M2_95");
  add_test(&compare_fuel_by_index<FuelOldM3<5>>, "M3_05");
  add_test(&compare_fuel_by_index<FuelOldM3<10>>, "M3_10");
  add_test(&compare_fuel_by_index<FuelOldM3<15>>, "M3_15");
  add_test(&compare_fuel_by_index<FuelOldM3<20>>, "M3_20");
  add_test(&compare_fuel_by_index<FuelOldM3<25>>, "M3_25");
  add_test(&compare_fuel_by_index<FuelOldM3<30>>, "M3_30");
  add_test(&compare_fuel_by_index<FuelOldM3<35>>, "M3_35");
  add_test(&compare_fuel_by_index<FuelOldM3<40>>, "M3_40");
  add_test(&compare_fuel_by_index<FuelOldM3<45>>, "M3_45");
  add_test(&compare_fuel_by_index<FuelOldM3<50>>, "M3_50");
  add_test(&compare_fuel_by_index<FuelOldM3<55>>, "M3_55");
  add_test(&compare_fuel_by_index<FuelOldM3<60>>, "M3_60");
  add_test(&compare_fuel_by_index<FuelOldM3<65>>, "M3_65");
  add_test(&compare_fuel_by_index<FuelOldM3<70>>, "M3_70");
  add_test(&compare_fuel_by_index<FuelOldM3<75>>, "M3_75");
  add_test(&compare_fuel_by_index<FuelOldM3<80>>, "M3_80");
  add_test(&compare_fuel_by_index<FuelOldM3<85>>, "M3_85");
  add_test(&compare_fuel_by_index<FuelOldM3<90>>, "M3_90");
  add_test(&compare_fuel_by_index<FuelOldM3<95>>, "M3_95");
  add_test(&compare_fuel_by_index<FuelOldM3<100>>, "M3_100");
  add_test(&compare_fuel_by_index<FuelOldM4<5>>, "M4_05");
  add_test(&compare_fuel_by_index<FuelOldM4<10>>, "M4_10");
  add_test(&compare_fuel_by_index<FuelOldM4<15>>, "M4_15");
  add_test(&compare_fuel_by_index<FuelOldM4<20>>, "M4_20");
  add_test(&compare_fuel_by_index<FuelOldM4<25>>, "M4_25");
  add_test(&compare_fuel_by_index<FuelOldM4<30>>, "M4_30");
  add_test(&compare_fuel_by_index<FuelOldM4<35>>, "M4_35");
  add_test(&compare_fuel_by_index<FuelOldM4<40>>, "M4_40");
  add_test(&compare_fuel_by_index<FuelOldM4<45>>, "M4_45");
  add_test(&compare_fuel_by_index<FuelOldM4<50>>, "M4_50");
  add_test(&compare_fuel_by_index<FuelOldM4<55>>, "M4_55");
  add_test(&compare_fuel_by_index<FuelOldM4<60>>, "M4_60");
  add_test(&compare_fuel_by_index<FuelOldM4<65>>, "M4_65");
  add_test(&compare_fuel_by_index<FuelOldM4<70>>, "M4_70");
  add_test(&compare_fuel_by_index<FuelOldM4<75>>, "M4_75");
  add_test(&compare_fuel_by_index<FuelOldM4<80>>, "M4_80");
  add_test(&compare_fuel_by_index<FuelOldM4<85>>, "M4_85");
  add_test(&compare_fuel_by_index<FuelOldM4<90>>, "M4_90");
  add_test(&compare_fuel_by_index<FuelOldM4<95>>, "M4_95");
  add_test(&compare_fuel_by_index<FuelOldM4<100>>, "M4_100");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<0>>, "M3_M4_00");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<5>>, "M3_M4_05");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<10>>, "M3_M4_10");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<15>>, "M3_M4_15");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<20>>, "M3_M4_20");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<25>>, "M3_M4_25");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<30>>, "M3_M4_30");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<35>>, "M3_M4_35");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<40>>, "M3_M4_40");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<45>>, "M3_M4_45");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<50>>, "M3_M4_50");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<55>>, "M3_M4_55");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<60>>, "M3_M4_60");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<65>>, "M3_M4_65");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<70>>, "M3_M4_70");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<75>>, "M3_M4_75");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<80>>, "M3_M4_80");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<85>>, "M3_M4_85");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<90>>, "M3_M4_90");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<95>>, "M3_M4_95");
  add_test(&compare_fuel_by_index<FuelOldM1<0>>, "M1_00");
  add_test(&compare_fuel_by_index<FuelOldM2<0>>, "M2_00");
  add_test(&compare_fuel_variable_by_index<FuelOldM1M2<0>>, "M1_M2_00");
  add_test(&compare_fuel_by_index<FuelOldM3<0>>, "M3_00");
  add_test(&compare_fuel_by_index<FuelOldM4<0>>, "M4_00");
  add_test(&compare_fuel_variable_by_index<FuelOldM3M4<100>>, "M3_M4_100");
  add_test(&compare_fuel_variable_by_index_options<FuelOldO1>, "O1", FUEL_COMPARE_GRASS);
  check_equal(NUMBER_OF_FUELS, i, "Number of fuels");
  return results.value().get();
}
}
int main(const int argc, const char* const argv[])
{
  using namespace fs::settings;
  constexpr auto fct_main = fs::testing::test_fbp;
  static const Usage USAGE_TEST{"Run tests and exit", ""};
  SettingsArgumentParser parser{USAGE_TEST, argc, argv, PositionalArgumentsRequired::NotRequired};
  parser.parse_args();
  exit(fct_main(argc, argv));
}
