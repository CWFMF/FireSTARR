/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "LookupTable.h"
namespace tbd::fuel
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
template <int Ash, int Rho, int B0, int B1, int B2, int B3>
class DuffType;
/**
 * \brief Base class for DuffType.
 */
class Duff
{
public:
  virtual ~Duff() = default;
  Duff(const Duff& rhs) noexcept = delete;
  Duff(Duff&& rhs) noexcept = delete;
  Duff& operator=(const Duff& rhs) noexcept = delete;
  Duff& operator=(Duff&& rhs) noexcept = delete;
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
  [[nodiscard]] constexpr bool operator!=(const Duff& rhs) const
  {
    return !operator==(rhs);
  }
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple formulae
   * \param mc_pct Moisture content (%)
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] virtual ThresholdSize probabilityOfSurvival(MathSize mc_pct) const noexcept = 0;
  //  /**
  //   * \brief Feather moss (upper) [Frandsen table 2/3]
  //   */
  //  static const DuffType<172, 464, 139873, -3296, 4904, 568> FeatherMossUpper;
  //  /**
  //   * \brief Feather moss (lower) [Frandsen table 2/3]
  //   */
  //  static const DuffType<191, 389, 132628, -1167, 3308, -2604> FeatherMossLower;
  /**
   * \brief Sphagnum (upper) [Frandsen table 2/3]
   */
  static const DuffType<124, 218, -88306, -608, 8095, 2735> SphagnumUpper;
  //  /**
  //   * \brief Sphagnum (lower) [Frandsen table 2/3]
  //   */
  //  static const DuffType<567, 1190, 3273347, -37655, -87849, 26684> SphagnumLower;
  /**
   * \brief Feather [Frandsen table 3]
   */
  static const DuffType<181, 427, 90970, -1040, 1165, -646> FeatherMoss;
  /**
   * \brief Reindeer/feather [Frandsen table 2/3]
   */
  static const DuffType<261, 563, 80359, -393, -591, -340> Reindeer;
  //  /**
  //   * \brief Sedge meadow (upper) [Frandsen table 2/3]
  //   */
  //  static const DuffType<233, 694, 398477, -1800, -3727, -1874> SedgeMeadowUpper;
  //  /**
  //   * \brief Sedge meadow (lower) [Frandsen table 2/3]
  //   */
  //  static const DuffType<449, 915, 290818, -2059, -2319, -420> SedgeMeadowLower;
  /**
   * \brief White spruce duff [Frandsen table 2/3]
   */
  static const DuffType<359, 1220, 3325604, -12220, -21024, -12619> WhiteSpruce;
  /**
   * \brief Peat [Frandsen table 2/3]
   */
  static const DuffType<94, 2220, -198198, -1169, 10414, 782> Peat;
  /**
   * \brief Peat muck [Frandsen table 2/3]
   */
  static const DuffType<349, 2030, 372276, -1876, -2833, -951> PeatMuck;
  //  /**
  //   * \brief Sedge meadow (Seney) [Frandsen table 2/3]
  //   */
  //  static const DuffType<354, 1830, 71813, -1413, -1253, 390> SedgeMeadowSeney;
  /**
   * \brief Pine duff (Seney) [Frandsen table 2/3]
   */
  static const DuffType<365, 1900, 451778, -3227, -3644, -362> PineSeney;
  /**
   * \brief Spruce/pine duff [Frandsen table 2/3]
   */
  static const DuffType<307, 1160, 586921, -2737, -5413, -1246> SprucePine;
  //  /**
  //   * \brief Grass/sedge marsh [Frandsen table 2/3]
  //   */
  //  static const DuffType<352, 1200, 2362934, -8423, -25097, -4902> GrassSedgeMarsh;
  //  /**
  //   * \brief Southern pine duff [Frandsen table 2/3]
  //   */
  //  static const DuffType<680, 1120, 586921, -2737, -5413, -1246> SouthernPine;
  //  /**
  //   * \brief Hardwood swamp (upper) [Frandsen table 2/3]
  //   */
  //  static const DuffType<182, 1380, 336907, -2946, -3002, -4040> HardwoodSwamp;
  // coefficients aren't defined in the table these came from
  // static const DuffType Pocosin;
  // static const DuffType SwampForest;
  // static const DuffType Flatwoods;
protected:
  Duff() = default;
};
/**
 * \brief A specific type of Duff layer, and the associated smouldering coefficients.
 */
template <int Ash, int Rho, int B0, int B1, int B2, int B3>
class DuffType final
  : public Duff
{
public:
  DuffType() = default;
  ~DuffType() override = default;
  DuffType(const DuffType& rhs) noexcept = delete;
  DuffType(DuffType&& rhs) noexcept = delete;
  DuffType& operator=(const DuffType& rhs) noexcept = delete;
  DuffType& operator=(DuffType&& rhs) noexcept = delete;
  /**
   * \brief Probability of survival (% / 100) [eq Ig-1]
   * \param mc_pct Moisture content, percentage dry oven weight
   * \return Probability of survival (% / 100) [eq Ig-1]
   */
  [[nodiscard]] ThresholdSize probabilityOfSurvival(const MathSize mc_pct) const noexcept override
  {
    return probability_of_survival_(mc_pct);
  }
  /**
   * \brief Inorganic content, percentage oven dry weight
   * \return Inorganic content, percentage oven dry weight
   */
  [[nodiscard]] static constexpr MathSize ash()
  {
    return Ash / 10.0;
  }
  /**
   * \brief Organic bulk density (kg/m^3)
   * \return Organic bulk density (kg/m^3)
   */
  [[nodiscard]] static constexpr MathSize rho()
  {
    return Rho / 10.0;
  }
  /**
   * \brief B_0 [table 2]
   * \return B_0 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b0()
  {
    return B0 / 10000.0;
  }
  /**
   * \brief B_1 [table 2]
   * \return B_1 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b1()
  {
    return B1 / 10000.0;
  }
  /**
   * \brief B_2 [table 2]
   * \return B_2 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b2()
  {
    return B2 / 10000.0;
  }
  /**
   * \brief B_3 [table 2]
   * \return B_3 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b3()
  {
    return B3 / 10000.0;
  }
private:
  /**
   * \brief Constant part of ignition probability equation [eq Ig-1]
   */
  static constexpr auto ConstantPart = b0() + b2() * ash() + b3() * rho();
  /**
   * \brief Ignition Probability (% / 100) [eq Ig-1]
   * \param mc_pct Moisture content, percentage dry oven weight
   * \return Ignition Probability (% / 100) [eq Ig-1]
   */
  [[nodiscard]] static ThresholdSize
    duffFunction(const MathSize mc_pct) noexcept
  {
    const auto d = 1 + exp(-(b1() * mc_pct + ConstantPart));
    if (0 == d)
    {
      return 1.0;
    }
    return 1.0 / d;
  }
  /**
   * \brief Ignition Probability (% / 100) [eq Ig-1]
   * \param mc_pct Moisture content, percentage dry oven weight
   * \return Ignition Probability (% / 100) [eq Ig-1]
   */
  util::LookupTable<&duffFunction> probability_of_survival_{};
};
}
