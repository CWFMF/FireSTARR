/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_DUFF_OLD_H
#define FS_DUFF_OLD_H
#include "../fs/stdafx.h"
// HACK: in test folder so both test_duff and test_fbp see it
namespace fs::duffold
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
class DuffTypeOld;
/**
 * \brief Base class for DuffTypeOld.
 */
class DuffOld
{
public:
  virtual ~DuffOld() = default;
  DuffOld(const DuffOld& rhs) noexcept = delete;
  DuffOld(DuffOld&& rhs) noexcept = delete;
  DuffOld& operator=(const DuffOld& rhs) noexcept = delete;
  DuffOld& operator=(DuffOld&& rhs) noexcept = delete;
  /**
   * \brief Equality operator
   * \param rhs DuffOld to compare to
   * \return Whether or not these are identical
   */
  [[nodiscard]] constexpr bool operator==(const DuffOld& rhs) const
  {
    // HACK: only equivalent if identical
    return this == &rhs;
  }
  /**
   * \brief Inequality operator
   * \param rhs DuffOld to compare to
   * \return Whether or not these are not identical
   */
  [[nodiscard]] constexpr bool operator!=(const DuffOld& rhs) const { return !operator==(rhs); }

protected:
  DuffOld() = default;
};
/**
 * \brief A specific type of DuffOld layer, and the associated smouldering coefficients.
 */
template <int Ash, int Rho, int B0, int B1, int B2, int B3>
class DuffTypeOld final : public DuffOld
{
public:
  DuffTypeOld() = default;
  ~DuffTypeOld() override = default;
  DuffTypeOld(const DuffTypeOld& rhs) noexcept = delete;
  DuffTypeOld(DuffTypeOld&& rhs) noexcept = delete;
  DuffTypeOld& operator=(const DuffTypeOld& rhs) noexcept = delete;
  DuffTypeOld& operator=(DuffTypeOld&& rhs) noexcept = delete;
  /**
   * \brief Probability of survival (% / 100) [eq Ig-1]
   * \param mc_pct Moisture content, percentage dry oven weight
   * \return Probability of survival (% / 100) [eq Ig-1]
   */
  /**
   * \brief Inorganic content, percentage oven dry weight
   * \return Inorganic content, percentage oven dry weight
   */
  [[nodiscard]] static constexpr MathSize ash() { return Ash / 10.0; }
  /**
   * \brief Organic bulk density (kg/m^3)
   * \return Organic bulk density (kg/m^3)
   */
  [[nodiscard]] static constexpr MathSize rho() { return Rho / 10.0; }
  /**
   * \brief B_0 [table 2]
   * \return B_0 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b0() { return B0 / 10000.0; }
  /**
   * \brief B_1 [table 2]
   * \return B_1 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b1() { return B1 / 10000.0; }
  /**
   * \brief B_2 [table 2]
   * \return B_2 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b2() { return B2 / 10000.0; }
  /**
   * \brief B_3 [table 2]
   * \return B_3 [table 2]
   */
  [[nodiscard]] static constexpr MathSize b3() { return B3 / 10000.0; }
  /**
   * \brief Ignition Probability (% / 100) [eq Ig-1]
   * \param mc_pct Moisture content, percentage dry oven weight
   * \return Ignition Probability (% / 100) [eq Ig-1]
   */
  [[nodiscard]] static ThresholdSize probabilityOfSurvival(const MathSize mc_pct) noexcept
  {
    const auto d = 1 + exp(-(b1() * mc_pct + ConstantPart));
    if (0 == d)
    {
      return 1.0;
    }
    return 1.0 / d;
  }

private:
  /**
   * \brief Constant part of ignition probability equation [eq Ig-1]
   */
  static constexpr auto ConstantPart = b0() + b2() * ash() + b3() * rho();
};
//  /**
//   * \brief Feather moss (upper) [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<172, 464, 139873, -3296, 4904, 568> FeatherMossUpper{};
//  /**
//   * \brief Feather moss (lower) [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<191, 389, 132628, -1167, 3308, -2604> FeatherMossLower{};
/**
 * \brief Sphagnum (upper) [Frandsen table 2/3]
 */
static const DuffTypeOld<124, 218, -88306, -608, 8095, 2735> SphagnumUpper{};
//  /**
//   * \brief Sphagnum (lower) [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<567, 1190, 3273347, -37655, -87849, 26684> SphagnumLower{};
/**
 * \brief Feather [Frandsen table 3]
 */
static const DuffTypeOld<181, 427, 90970, -1040, 1165, -646> FeatherMoss{};
/**
 * \brief Reindeer/feather [Frandsen table 2/3]
 */
static const DuffTypeOld<261, 563, 80359, -393, -591, -340> Reindeer{};
//  /**
//   * \brief Sedge meadow (upper) [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<233, 694, 398477, -1800, -3727, -1874> SedgeMeadowUpper{};
//  /**
//   * \brief Sedge meadow (lower) [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<449, 915, 290818, -2059, -2319, -420> SedgeMeadowLower{};
/**
 * \brief White spruce duff [Frandsen table 2/3]
 */
static const DuffTypeOld<359, 1220, 3325604, -12220, -21024, -12619> WhiteSpruce{};
/**
 * \brief Peat [Frandsen table 2/3]
 */
static const DuffTypeOld<94, 2220, -198198, -1169, 10414, 782> Peat{};
/**
 * \brief Peat muck [Frandsen table 2/3]
 */
static const DuffTypeOld<349, 2030, 372276, -1876, -2833, -951> PeatMuck{};
//  /**
//   * \brief Sedge meadow (Seney) [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<354, 1830, 71813, -1413, -1253, 390> SedgeMeadowSeney{};
/**
 * \brief Pine duff (Seney) [Frandsen table 2/3]
 */
static const DuffTypeOld<365, 1900, 451778, -3227, -3644, -362> PineSeney{};
/**
 * \brief Spruce/pine duff [Frandsen table 2/3]
 */
static const DuffTypeOld<307, 1160, 586921, -2737, -5413, -1246> SprucePine{};
//  /**
//   * \brief Grass/sedge marsh [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<352, 1200, 2362934, -8423, -25097, -4902> GrassSedgeMarsh{};
//  /**
//   * \brief Southern pine duff [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<680, 1120, 586921, -2737, -5413, -1246> SouthernPine{};
//  /**
//   * \brief Hardwood swamp (upper) [Frandsen table 2/3]
//   */
//  static const DuffTypeOld<182, 1380, 336907, -2946, -3002, -4040> HardwoodSwamp{};
// coefficients aren't defined in the table these came from
// static const DuffTypeOld Pocosin;
// static const DuffTypeOld SwampForest;
// static const DuffTypeOld Flatwoods;
}
#endif
