/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "fs/ArgumentParser.h"
int main(const int argc, const char* const argv[])
{
  using namespace fs::settings;
  constexpr auto fct_main = fs::testing::test_duff;
  static const Usage USAGE_TEST{"Run tests and exit", ""};
  SettingsArgumentParser parser{USAGE_TEST, argc, argv, PositionalArgumentsRequired::NotRequired};
  parser.parse_args();
  exit(fct_main(argc, argv));
}
