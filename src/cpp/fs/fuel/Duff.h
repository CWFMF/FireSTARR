/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_DUFF_H
#define FS_DUFF_H
#include "../stdafx.h"
namespace fs::duff
{
/*! \page survival Probability of fire survival
 *
 * Fire survival is determined for each point at each time step. Survival is
 * dependent on FFMC, DMC, and fuel in the cell. If the fire is determined to have
 * not survived in both the FFMC and DMC fuels then it is considered extinguished
 * and all points in that cell are removed from the simulation. Since the cell is
 * already marked as burned at this point, it will not burn again.
 *
 * Probability of survival is determined as per the papers referenced, with the
 * FBP fuel types having been assigned an FFMC duff type and a DMC duff type,
 * and the moisture for each type being calculated based off the current indices.
 *
 * \section References
 *
 * Lawson, B.D.; Frandsen, W.H.; Hawkes, B.C.; Dalrymple, G.N. 1997.
 * Probability of sustained smoldering ignitions for some boreal forest duff types.
 * https://cfs.nrcan.gc.ca/pubwarehouse/pdfs/11900.pdf
 *
 * Frandsen, W.H. 1997.
 * Ignition probability of organic soils.
 * https://www.nrcresearchpress.com/doi/pdf/10.1139/x97-106
 *
 * Anderson, Kerry 2002.
 * A model to predict lightning-caused fire occurrences.
 * International Journal of Wildland Fire 11, 163-172.
 * https://doi.org/10.1071/WF02001
 */
/**
 * \brief A specific type of Duff layer, and the associated smouldering coefficients.
 */
struct Duff
{
  /**
   * \brief Inorganic content, percentage oven dry weight
   */
  MathConstantSize ash{};
  /**
   * \brief Organic bulk density (kg/m^3)
   */
  MathConstantSize rho{};
  /**
   * \brief B_0 [table 2]
   */
  MathConstantSize b0{};
  /**
   * \brief B_1 [table 2]
   */
  MathConstantSize b1{};
  /**
   * \brief B_2 [table 2]
   */
  MathConstantSize b2{};
  /**
   * \brief B_3 [table 2]
   */
  MathConstantSize b3{};
  /**
   * \brief Probability of survival (% / 100) [eq Ig-1]
   * \param mc_pct Moisture content, percentage dry oven weight
   * \return Probability of survival (% / 100) [eq Ig-1]
   */
  [[nodiscard]] ThresholdSize probabilityOfSurvival(const MathConstantSize mc_pct) const noexcept
  {
    /**
     * \brief Constant part of ignition probability equation [eq Ig-1]
     */
    const auto ConstantPart = b0 + b2 * ash + b3 * rho;
    const auto d = 1 + exp(-(b1 * mc_pct + ConstantPart));
    if (0 == d)
    {
      return 1.0;
    }
    return 1.0 / d;
  }
  /**
   * \brief Equality operator
   * \param rhs Duff to compare to
   * \return Whether or not these are identical
   */
  [[nodiscard]] constexpr bool operator==(const Duff& rhs) const
  {
    // HACK: only equivalent if identical
    return this == &rhs;
  }
  /**
   * \brief Inequality operator
   * \param rhs Duff to compare to
   * \return Whether or not these are not identical
   */
  [[nodiscard]] constexpr bool operator!=(const Duff& rhs) const { return !operator==(rhs); }
};
// /**
//  * \brief Feather moss (upper) [Frandsen table 2/3]
//  */
// static constexpr Duff FeatherMossUpper{17.2, 46.4, 13.9873, -0.3296, 0.4904, 0.0568};
// /**
//  * \brief Feather moss (lower) [Frandsen table 2/3]
//  */
// static constexpr Duff FeatherMossLower{19.1, 38.9, 13.2628, -0.1167, 0.3308, -0.2604};
/**
 * \brief Sphagnum (upper) [Frandsen table 2/3]
 */
static constexpr Duff SphagnumUpper{12.4, 21.8, -8.8306, -0.0608, 0.8095, 0.2735};
// /**
//  * \brief Sphagnum (lower) [Frandsen table 2/3]
//  */
// static constexpr Duff SphagnumLower{56.7, 119.0, 327.3347, -3.7655, -8.7849, 2.6684};
/**
 * \brief Feather [Frandsen table 3]
 */
static constexpr Duff FeatherMoss{18.1, 42.7, 9.0970, -0.1040, 0.1165, -0.0646};
/**
 * \brief Reindeer/feather [Frandsen table 2/3]
 */
static constexpr Duff Reindeer{26.1, 56.3, 8.0359, -0.0393, -0.0591, -0.0340};
// /**
//  * \brief Sedge meadow (upper) [Frandsen table 2/3]
//  */
// static constexpr Duff SedgeMeadowUpper{23.3, 69.4, 39.8477, -0.1800, -0.3727, -0.1874};
// /**
//  * \brief Sedge meadow (lower) [Frandsen table 2/3]
//  */
// static constexpr Duff SedgeMeadowLower{44.9, 91.5, 29.0818, -0.2059, -0.2319, -0.0420};
/**
 * \brief White spruce duff [Frandsen table 2/3]
 */
static constexpr Duff WhiteSpruce{35.9, 122.0, 332.5604, -1.2220, -2.1024, -1.2619};
/**
 * \brief Peat [Frandsen table 2/3]
 */
static constexpr Duff Peat{9.4, 222.0, -19.8198, -0.1169, 1.0414, 0.0782};
/**
 * \brief Peat muck [Frandsen table 2/3]
 */
static constexpr Duff PeatMuck{34.9, 203.0, 37.2276, -0.1876, -0.2833, -0.0951};
// /**
//  * \brief Sedge meadow (Seney) [Frandsen table 2/3]
//  */
// static constexpr Duff SedgeMeadowSeney{35.4, 183.0, 71.813, -0.1413, -0.1253, 0.0390};
/**
 * \brief Pine duff (Seney) [Frandsen table 2/3]
 */
static constexpr Duff PineSeney{36.5, 190.0, 45.1778, -0.3227, -0.3644, -0.0362};
/**
 * \brief Spruce/pine duff [Frandsen table 2/3]
 */
static constexpr Duff SprucePine{30.7, 116.0, 58.6921, -0.2737, -0.5413, -0.1246};
// /**
//  * \brief Grass/sedge marsh [Frandsen table 2/3]
//  */
// static constexpr Duff GrassSedgeMarsh{35.2, 120.0, 236.2934, -0.8423, -2.5097, -0.4902};
// /**
//  * \brief Southern pine duff [Frandsen table 2/3]
//  */
// static constexpr Duff SouthernPine{68.0, 112.0, 58.6921, -0.2737, -0.5413, -0.1246};
// /**
//  * \brief Hardwood swamp (upper) [Frandsen table 2/3]
//  */
// static constexpr Duff HardwoodSwamp{18.2, 138.0, 33.6907, -0.2946, -0.3002, -0.4040};
// coefficients aren't defined in the table these came from
// static const Duff Pocosin;
// static const Duff SwampForest;
// static const Duff Flatwoods;
}
#endif
