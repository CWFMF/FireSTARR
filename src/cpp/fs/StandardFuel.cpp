/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "StandardFuel.h"
namespace fs
{
MathSize calculate_standard_foliar_moisture_fmc(const MathSize f) noexcept
{
  const auto fmc = round_value(f);
  return pow_int<4>(1.5 - 0.00275 * fmc) / (460.0 + 25.9 * fmc) / 0.778 * 1000.0;
}
MathSize calculate_standard_foliar_moisture_isi(const MathSize i) noexcept
{
  const auto isi = round_value(i);
  return 60.0 * (1.0 - exp(-0.0497 * isi));
}
MathSize calculate_standard_length_to_breadth(const MathSize w) noexcept
{
  const auto ws = round_value(w);
  return 1.0 + 8.729 * pow(1.0 - exp(-0.030 * ws), 2.155);
}
}
