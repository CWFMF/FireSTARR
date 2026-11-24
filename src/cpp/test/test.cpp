/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "test.h"
#include "../fs/fuel/Duff.h"
#include "../fs/log/Log.h"
#include "DuffOld.h"
namespace fs::testing
{
// FIX: this was used to compare to the old template version, but doesn't work now
//      left for reference for now so idea could be used for more tests
int test_duff(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing Duff");
  compare_duff("SphagnumUpper", duff::SphagnumUpper, duffold::SphagnumUpper);
  compare_duff("FeatherMoss", duff::FeatherMoss, duffold::FeatherMoss);
  compare_duff("Reindeer", duff::Reindeer, duffold::Reindeer);
  compare_duff("WhiteSpruce", duff::WhiteSpruce, duffold::WhiteSpruce);
  compare_duff("Peat", duff::Peat, duffold::Peat);
  compare_duff("PeatMuck", duff::PeatMuck, duffold::PeatMuck);
  compare_duff("PineSeney", duff::PineSeney, duffold::PineSeney);
  compare_duff("SprucePine", duff::SprucePine, duffold::SprucePine);
  return 0;
}
}
