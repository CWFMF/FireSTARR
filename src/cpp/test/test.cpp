/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "test.h"
#include "../fs/fuel/Duff.h"
#include "../fs/log/Log.h"
namespace fs::testing
{
// FIX: this was used to compare to the old template version, but doesn't work now
//      left for reference for now so idea could be used for more tests
int test_duff(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing Duff");
  compare_duff("SphagnumUpper", duff::SphagnumUpper, duff::SphagnumUpper);
  compare_duff("FeatherMoss", duff::FeatherMoss, duff::FeatherMoss);
  compare_duff("Reindeer", duff::Reindeer, duff::Reindeer);
  compare_duff("WhiteSpruce", duff::WhiteSpruce, duff::WhiteSpruce);
  compare_duff("Peat", duff::Peat, duff::Peat);
  compare_duff("PeatMuck", duff::PeatMuck, duff::PeatMuck);
  compare_duff("PineSeney", duff::PineSeney, duff::PineSeney);
  compare_duff("SprucePine", duff::SprucePine, duff::SprucePine);
  return 0;
}
}
