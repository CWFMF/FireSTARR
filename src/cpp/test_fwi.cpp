/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "fs/log/Log.h"
#include "fs/sim/ArgumentParser.h"
#include "fs/unstable.h"
#include "fs/util/Util.h"
#include "fs/wx/FwiReference.h"
#include "fs/wx/WeatherIndices.h"
#include "test_fwi/FwiOld.h"
namespace fs::testing
{
// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
using settings::Settings;
using namespace std;
using namespace fs::fwireference;
constexpr auto DEFAULT_LATITUDE = 46.0;
struct line_type
{
  int month;
  int day;
  MathSize temp;
  MathSize rhum;
  MathSize wind;
  MathSize prcp;
};
int test_fwi_file(
  const char* file_in,
  const char* file_out,
  const MathSize latitude = DEFAULT_LATITUDE
)
{
  string line;
  MathSize temp, rhum, wind, prcp;
  int month, day;
  /* Initialize FMC, DMC, and DC */
  Ffmc ffmc0{85.0};
  Dmc dmc0{6.0};
  Dc dc0{15.0};
  Ffmc ffmc0_{ffmc0};
  Dmc dmc0_{dmc0};
  Dc dc0_{dc0};
  // HACK: load file once and buffer
  static map<string, vector<line_type>> buffered_files{};
  auto buffer = buffered_files[file_in];
  if (buffer.empty())
  {
    /* Open input and output files */
    ifstream inputFile(file_in);
    if (!inputFile.is_open())
    {
      cout << "Unable to open input data file";
      return -1;
    }
    while (getline(inputFile, line))
    {
      istringstream ss(line);
      ss >> month >> day >> temp >> rhum >> wind >> prcp;
      buffer.push_back(line_type{month, day, temp, rhum, wind, prcp});
    }
    inputFile.close();
  }
  buffered_files[file_in] = buffer;
  assert(buffered_files[file_in].size() == buffer.size());
  ofstream outputFile;
  if (nullptr != file_out)
  {
    outputFile.open(file_out);
    if (!outputFile.is_open())
    {
      cout << "Unable to open output data file";
      return 1;
    }
  }
  logging::debug("Testing FWI generated from {:s} for latitude {:g}", file_in, latitude);
  const Latitude latitude_{latitude};
  /* Main loop for calculating indices */
  for (auto line : buffer)
  {
    month = line.month;
    day = line.day;
    temp = line.temp;
    rhum = line.rhum;
    wind = line.wind;
    prcp = line.prcp;
    const auto month_{Month::from_ordinal(month)};
    static constexpr MathSize EPSILON{std::numeric_limits<MathSize>::epsilon()};
    Temperature temp_{temp};
    RelativeHumidity rhum_{rhum};
    Speed wind_{wind};
    Precipitation prcp_{prcp};
    auto ffmc{fwireference::FFMCcalc(temp_, rhum_, wind_, prcp_, ffmc0)};
    auto ffmc_{fwiold::FFMCcalc(temp_, rhum_, wind_, prcp_, ffmc0_)};
    logging::check_tolerance(EPSILON, ffmc.value, ffmc_.value, "ffmc");
    auto dmc{fwireference::DMCcalc(temp_, rhum_, prcp_, dmc0_, month_, latitude_)};
    auto dmc_{fwiold::DMCcalc(temp_, rhum_, prcp_, dmc0_, month, latitude)};
    logging::check_tolerance(EPSILON, dmc.value, dmc_.value, "dmc");
    auto dc{fwireference::DCcalc(temp_, prcp_, dc0_, month_, latitude_)};
    auto dc_{fwiold::DCcalc(temp_, prcp_, dc0_, month, latitude)};
    logging::check_tolerance(EPSILON, dc.value, dc_.value, "dc");
    auto isi{fwireference::ISIcalc(wind_, ffmc)};
    auto isi_{fwiold::ISIcalc(wind_, ffmc_)};
    logging::check_tolerance(EPSILON, isi.value, isi_.value, "isi");
    auto bui{fwireference::BUIcalc(dmc, dc)};
    auto bui_{fwiold::BUIcalc(dmc_, dc_)};
    logging::check_tolerance(EPSILON, bui.value, bui_.value, "bui");
    auto fwi{fwireference::FWIcalc(isi, bui)};
    auto fwi_{fwiold::FWIcalc(isi_, bui_)};
    logging::check_tolerance(EPSILON, fwi.value, fwi_.value, "fwi");
    ffmc0 = ffmc;
    dmc0 = dmc;
    dc0 = dc;
    ffmc0_ = ffmc_;
    dmc0_ = dmc_;
    dc0_ = dc_;
    logging::verbose(
      "{:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f}",
      ffmc.value,
      dmc.value,
      dc.value,
      isi.value,
      bui.value,
      fwi.value
    );
    if (nullptr != file_out)
    {
      outputFile << std::format(
        "{:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f}",
        ffmc.value,
        dmc.value,
        dc.value,
        isi.value,
        bui.value,
        fwi.value
      ) << endl;
      // outputFile << ffmc << ' ' << dmc << ' ' << dc << ' ' << isi << ' ' << bui << ' ' << fwi <<
      // endl;
    }
  }
  if (nullptr != file_out)
  {
    outputFile.close();
  }
  return 0;
}
int compare_files(const string file0, const string file1)
{
  logging::info("Comparing {:s} to {:s}", file0, file1);
  ifstream input0(file0.c_str());
  if (!input0.is_open())
  {
    cout << "Unable to open input data file " << file0;
    return -1;
  }
  ifstream input1(file1.c_str());
  if (!input1.is_open())
  {
    cout << "Unable to open input data file " << file1;
    return 1;
  }
  string line0;
  string line1;
  while (getline(input0, line0))
  {
    if (!getline(input1, line1))
    {
      return 1;
    }
    if (auto cmp = line0.compare(line1); 0 != cmp)
    {
      logging::error("\t{:s}\n!=\t{:s}", line0, line1);
      return cmp;
    }
  }
  if (getline(input1, line1))
  {
    return -1;
  }
  return 0;
}
int test_fwi_files(const string file_expected, const string file_in, const string file_out)
{
  // FIX: other tests use test/input but want to switch to test/data
  if (auto ret = test_fwi_file(file_in.c_str(), file_out.c_str()); ret != 0)
  {
    logging::error("Generating {:s} from {:s} failed", file_out, file_in);
    return ret;
  }
  if (auto cmp = compare_files(file_expected, file_out); 0 != cmp)
  {
    logging::error(
      "Comparison between files [{:s}, {:s}] gives {:d}", file_expected, file_out, cmp
    );
    return cmp;
  }
  return 0;
}
int test_fwi(const int argc, const char* const argv[])
{
  // HACK: parser happens before this
  std::ignore = argc;
  std::ignore = argv;
  constexpr auto FILE_EXPECTED{"test/data/fwi/fwi_out.txt"};
  constexpr auto FILE_IN{"test/data/fwi/fwi_in.txt"};
  constexpr auto FILE_OUT{"test/output/fwi/fwi_out.txt"};
  constexpr auto LATITUDE_MIN{-90.0};
  constexpr auto LATITUDE_MAX{90.0};
  constexpr auto STEP{0.001};
  make_directory_recursive("test/output/fwi");
  if (const auto ret = test_fwi_files(FILE_EXPECTED, FILE_IN, FILE_OUT); 0 != ret)
  {
    return ret;
  }
  logging::info(
    "Testing across latitude range [{:+g}, {:+g}] with step {:g}", LATITUDE_MIN, LATITUDE_MAX, STEP
  );
  auto check_latitude = [&](const MathSize latitude) {
    if (auto ret = test_fwi_file(FILE_IN, nullptr, latitude); ret != 0)
    {
      logging::error("Generating from {:s} with latitude {:g} failed", FILE_IN, latitude);
      return ret;
    }
    return 0;
  };
  MathSize latitude = LATITUDE_MIN;
  while (LATITUDE_MAX > latitude)
  {
    if (auto ret = check_latitude(latitude); 0 != ret)
    {
      return ret;
    }
    latitude += STEP;
  }
  if (auto ret = check_latitude(LATITUDE_MAX); 0 != ret)
  {
    return ret;
  }
  logging::note("Testing FWI calculations succeeded");
  return 0;
}
}
int main(const int argc, const char* const argv[])
{
  using namespace fs::settings;
  constexpr auto fct_main = fs::testing::test_fwi;
  static const Usage USAGE_TEST{"Run tests and exit", ""};
  SettingsArgumentParser parser{USAGE_TEST, argc, argv, PositionalArgumentsRequired::NotRequired};
  parser.parse_args();
  exit(fct_main(argc, argv));
}
