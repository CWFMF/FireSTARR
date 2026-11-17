/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SURVIVAL_H
#define FS_SURVIVAL_H
#include "stdafx.h"
#include "Duff.h"
#include "FWI.h"
namespace fs::survival
{
// amount of duff to apply ffmc moisture to (cm) (1.2 cm is from Kerry's paper)
static constexpr MathSize DUFF_FFMC_DEPTH = 1.2;
/**
 * \brief Calculate probability of burning [Anderson eq 1]
 * \param mc_fraction moisture content (% / 100)
 * \return Calculate probability of burning [Anderson eq 1]
 */
[[nodiscard]] ThresholdSize probability_peat(
  const MathSize bulk_density,
  const MathSize inorganic_percent,
  const MathSize mc_fraction
) noexcept;
/**
 * \brief Survival probability calculated using probability of ony survival based on multiple
 * formulae
 * \param wx FwiWeather to calculate survival probability for
 * \return Chance of survival (% / 100)
 */
[[nodiscard]] ThresholdSize survival_probability(
  const MathSize bulk_density,
  const MathSize inorganic_percent,
  const duff::Duff& duff_ffmc_type,
  const duff::Duff& duff_dmc_type,
  const MathSize dmc_ratio,
  const FwiWeather& wx
) noexcept;
}
#endif
