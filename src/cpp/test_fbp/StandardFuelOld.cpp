/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "StandardFuelOld.h"
#include "../fs/unstable.h"
namespace fs::fuelold
{
MathSize calculate_standard_foliar_moisture_fmc(const LookupValue& fmc_) noexcept
{
  const auto fmc = round_value(fmc_);
  return pow_int<4>(1.5 - 0.00275 * fmc) / (460.0 + 25.9 * fmc) / 0.778 * 1000.0;
}
MathSize calculate_standard_foliar_moisture_isi(const LookupValue& isi_) noexcept
{
  const auto isi = round_value(isi_);
  return 60.0 * (1.0 - exp(-0.0497 * isi));
}
MathSize calculate_standard_length_to_breadth(const LookupValue& ws_) noexcept
{
  const auto ws = round_value(ws_);
  return 1.0 + 8.729 * pow(1.0 - exp(-0.030 * ws), 2.155);
}
MathSize calculate_standard_rsc(const RoundedValue& isi, const RoundedValue& fmc) noexcept
{
  // HACK: convert back to double to instantiate as LookupValue
  return calculate_standard_foliar_moisture_isi(as_value(isi))
       * calculate_standard_foliar_moisture_fmc(as_value(fmc));
}
}
