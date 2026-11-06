/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Survival.h"
namespace fs::survival
{
ThresholdSize probability_peat(
  const MathSize bulk_density,
  const MathSize inorganic_percent,
  const MathSize mc_fraction
) noexcept
{
  // Anderson table 1
  const auto pb = bulk_density;
  // Anderson table 1
  const auto fi = inorganic_percent;
  const auto pi = fi * pb;
  // Inorganic ratio
  const auto ri = fi / (1 - fi);
  const auto const_part = -19.329 + 1.7170 * ri + 23.059 * pi;
  // Anderson eq 1
  return 1 / (1 + exp(17.047 * mc_fraction / (1 - fi) + const_part));
}
ThresholdSize survival_probability(
  const MathSize bulk_density,
  const MathSize inorganic_percent,
  const duff::Duff& duff_ffmc_type,
  const duff::Duff& duff_dmc_type,
  const MathSize dmc_ratio,
  const FwiWeather& wx
) noexcept
{
  // divide by 100 since we need moisture ratio
  //    IFERROR(((1 / (1 + EXP($G$43 + $I$43 *
  //            (Q$44 * $O$43 + $N$43)))) -
  //            (1 / (1 + EXP($G$43 + $I$43 * (2.5 * $O$43 + $N$43)))))
  //            / (1 / (1 + EXP($G$43 + $I$43 * $N$43))), 0)
  // HACK: use same constants for all fuels because they seem to work nicer than
  // using the ratios, but they change anyway because of the other fuel attributes
  static const auto WFfmc = 0.25;
  static const auto WDmc = 1.0;
  static const auto RatioHartford = 0.5;
  static const auto RatioFrandsen = 1.0 - RatioHartford;
  static const auto RatioAspen = 0.5;
  static const auto RatioFuel = 1.0 - RatioAspen;
  const auto ffmc_ratio = 1 - dmc_ratio;
  const auto mc_ffmc = wx.mcFfmc() * WFfmc + WDmc;
  static const auto McFfmcSaturated = 2.5 * WFfmc + WDmc;
  static const auto McDmc = WDmc;
  const auto prob_ffmc_peat = probability_peat(bulk_density, inorganic_percent, mc_ffmc);
  const auto prob_ffmc_peat_saturated =
    probability_peat(bulk_density, inorganic_percent, McFfmcSaturated);
  const auto prob_ffmc_peat_zero = probability_peat(bulk_density, inorganic_percent, McDmc);
  const auto prob_ffmc_peat_weighted =
    (prob_ffmc_peat - prob_ffmc_peat_saturated) / prob_ffmc_peat_zero;
  const auto prob_ffmc = duff_ffmc_type.probabilityOfSurvival(mc_ffmc * 100);
  const auto prob_ffmc_saturated = duff_ffmc_type.probabilityOfSurvival(McFfmcSaturated * 100);
  const auto prob_ffmc_zero = duff_ffmc_type.probabilityOfSurvival(McDmc);
  const auto prob_ffmc_weighted = (prob_ffmc - prob_ffmc_saturated) / prob_ffmc_zero;
  const auto term_otway = exp(-3.11 + 0.12 * wx.dmc.value);
  const auto prob_otway = term_otway / (1 + term_otway);
  const auto mc_pct = wx.mcDmcPct() * dmc_ratio + wx.mcFfmcPct() * ffmc_ratio;
  const auto prob_weight_ffmc = duff_ffmc_type.probabilityOfSurvival(mc_pct);
  const auto prob_weight_ffmc_peat =
    probability_peat(bulk_density, inorganic_percent, mc_pct / 100);
  const auto prob_weight_dmc = duff_dmc_type.probabilityOfSurvival(wx.mcDmcPct());
  const auto prob_weight_dmc_peat = probability_peat(bulk_density, inorganic_percent, wx.mcDmc());
  // chance of survival is 1 - chance of it not surviving in every fuel
  const auto tot_prob =
    1
    - (1 - prob_ffmc_peat_weighted) * (1 - prob_ffmc_weighted)
        * ((1 - prob_otway) * RatioAspen + ((1 - prob_weight_ffmc_peat) * RatioHartford + (1 - prob_weight_ffmc) * RatioFrandsen) * ((1 - prob_weight_dmc_peat) * RatioHartford + (1 - prob_weight_dmc) * RatioFrandsen) * RatioFuel);
  return tot_prob;
}
}
