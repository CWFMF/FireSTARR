/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <limits>
#include "Location.h"
#include "FuelType.h"
#include "Util.h"
namespace fs::topo
{
using SpreadKey = uint32_t;
using fuel::INVALID_FUEL_CODE;
/**
 * \brief A Position with a Slope, Aspect, and Fuel.
 */
class Cell
  : public Position<Topo>
{
public:
  /**
   * \brief Default constructor
   */
  constexpr Cell() noexcept
    : Cell(-1,
           -1,
           numeric_limits<SlopeSize>::min(),
           numeric_limits<AspectSize>::min(),
           numeric_limits<FuelCodeSize>::min())
  {
  }
  /**
   * \brief Full stored hash that may contain data from subclasses
   * \return Full stored hash that may contain data from subclasses
   */
  [[nodiscard]] constexpr Topo fullHash() const
  {
    return topo_data_;
  }
  /**
   * \brief Hash attributes into a Topo value
   * \param slope Slope
   * \param aspect Aspect
   * \param fuel Fuel
   * \return Hash
   */
  [[nodiscard]] static constexpr Topo hashCell(const SlopeSize slope,
                                               const AspectSize aspect,
                                               const FuelCodeSize& fuel) noexcept
  {
    // HACK: so we can call and set it all invalid if anything is
    // if any are invalid then they all should be
    const auto do_hash_cell = [](const SlopeSize s, const AspectSize a, const FuelCodeSize f) {
      return static_cast<Topo>(f) << FuelShift
           | static_cast<Topo>(s) << SlopeShift
           | static_cast<Topo>(a) << AspectShift;
    };
    if (
      INVALID_SLOPE == slope
      || INVALID_ASPECT == aspect
      || INVALID_FUEL_CODE == fuel)
    {
      return do_hash_cell(INVALID_SLOPE, INVALID_ASPECT, INVALID_FUEL_CODE);
    }
    // if slope is 0 make aspect north so less unique keys
    return do_hash_cell(slope, 0 == slope ? 0 : aspect, fuel);
  }
  /**
   * \brief Construct from hash value
   * \param hash Hash defining all attributes
   */
  explicit constexpr Cell(const Topo hash) noexcept
    : Position<Topo>(hash)
  {
  }
  /**
   * \brief Construct based on given attributes
   * \param hash Hash of row and column
   * \param slope Slope
   * \param aspect Aspect
   * \param fuel Fuel
   */
  constexpr Cell(const HashSize hash,
                 const SlopeSize slope,
                 const AspectSize aspect,
                 const FuelCodeSize& fuel) noexcept
    : Position<Topo>(
        static_cast<Topo>(hash & HashMask)
        | hashCell(slope, aspect, fuel))
  {
  }
  /**
   * \brief Constructor
   * \param row Row
   * \param column Column
   * \param slope Slope
   * \param aspect Aspect
   * \param fuel Fuel
   */
  constexpr Cell(const Idx row,
                 const Idx column,
                 const SlopeSize slope,
                 const AspectSize aspect,
                 const FuelCodeSize& fuel) noexcept
    : Position<Topo>(
        static_cast<Topo>(doHash(row, column))
        | hashCell(slope, aspect, fuel))
  {
  }
  /**
   * \brief A key defining Slope, Aspect, and Fuel, used for determining Cells that spread the same
   * \param value Topo to extract from
   * \return A key defining Slope, Aspect, and Fuel
   */
  [[nodiscard]] static constexpr SpreadKey key(const Topo value) noexcept
  {
    // can just shift since these are the only bits left after
    return static_cast<SpreadKey>(value >> FuelShift);
  }
  /**
   * \brief Aspect (degrees)
   * \param value SpreadKey to extract from
   * \return Aspect (degrees)
   */
  [[nodiscard]] static constexpr AspectSize aspect(const SpreadKey value) noexcept
  {
    return static_cast<AspectSize>((value & (AspectMask >> FuelShift)) >> (AspectShift - FuelShift));
  }
  /**
   * \brief Fuel
   * \param value SpreadKey to extract from
   * \return Fuel
   */

  [[nodiscard]] static constexpr FuelCodeSize fuelCode(const SpreadKey value) noexcept
  {
    return static_cast<FuelCodeSize>((value & (FuelMask >> FuelShift)) >> (FuelShift - FuelShift));
  }
  /**
   * \brief Slope (degrees)
   * \param value SpreadKey to extract from
   * \return Slope (degrees)
   */
  [[nodiscard]] static constexpr SlopeSize slope(const SpreadKey value) noexcept
  {
    return static_cast<SlopeSize>((value & (SlopeMask >> FuelShift)) >> (SlopeShift - FuelShift));
  }
  /**
   * \brief Aspect (degrees)
   * \param value Topo to extract from
   * \return Aspect (degrees)
   */
  [[nodiscard]] static constexpr AspectSize aspect(const Topo value) noexcept
  {
    return static_cast<AspectSize>((value & AspectMask) >> AspectShift);
  }
  /**
   * \brief Fuel
   * \param value Topo to extract from
   * \return Fuel
   */

  [[nodiscard]] static constexpr FuelCodeSize fuelCode(const Topo value) noexcept
  {
    return static_cast<FuelCodeSize>((value & FuelMask) >> FuelShift);
  }
  /**
   * \brief Slope (degrees)
   * \param value Topo to extract from
   * \return Slope (degrees)
   */
  [[nodiscard]] static constexpr SlopeSize slope(const Topo value) noexcept
  {
    return static_cast<SlopeSize>((value & SlopeMask) >> SlopeShift);
  }
  /**
   * \brief Topo that contains Cell data
   * \param value Topo to extract from
   * \return Topo that contains Cell data
   */
  [[nodiscard]] static constexpr Topo topoHash(const Topo value) noexcept
  {
    return static_cast<Topo>(value) & CellMask;
  }
  /**
   * \brief A key defining Slope, Aspect, and Fuel, used for determining Cells that spread the same
   * \return A key defining Slope, Aspect, and Fuel
   */
  [[nodiscard]] constexpr SpreadKey key() const noexcept
  {
    return Cell::key(topo_data_);
  }
  /**
   * \brief Aspect (degrees)
   * \return Aspect (degrees)
   */
  [[nodiscard]] constexpr AspectSize aspect() const noexcept
  {
    return Cell::aspect(topo_data_);
  }
  /**
   * \brief Fuel
   * \return Fuel
   */
  [[nodiscard]] constexpr FuelCodeSize fuelCode() const noexcept
  {
    return Cell::fuelCode(topo_data_);
  }
  /**
   * \brief Slope (degrees)
   * \return Slope (degrees)
   */
  [[nodiscard]] constexpr SlopeSize slope() const noexcept
  {
    return Cell::slope(topo_data_);
  }
  /**
   * \brief Topo that contains Cell data
   * \return Topo that contains Cell data
   */
  [[nodiscard]] constexpr Topo topoHash() const noexcept
  {
    return Cell::topoHash(topo_data_);
  }
protected:
  /*
   * Field                    Natural Range     Used Range      Bits    Bit Range
   * Row                      0 - 4095          0 - 4095        12      0 - 4095
   * Column                   0 - 4095          0 - 4095        12      0 - 4095
   * PADDING                                                    10
   * Fuel                     0 - 140           0 - 140         8       0 - 255
   * Aspect                   0 - 359           0 - 359         9       0 - 511
   * Slope                    0 - infinity      0 - 511         9       0 - 511
   * Extra                                                      8
   *
   * Rows and Columns are restricted to 4096 since that's what gets clipped out of
   * the GIS outputs.
   *
   * Fuel is tied to how many variations of percent conifer/dead fir we want to use, and
   * if we want to allow M1/M2/M3/M4 on their own, or just use the ones that are
   * automatically tied to the green-up.
   *
   * Aspect is calculated to be in degrees, so 0 - 359.
   *
   * Slope is truncated to 0 - 70 for slope effect calculations speed for slopes,
   * but keep higher range because there's an issue with this when it tries to calculate the
   * horizontal rate of spread since if the slope has been truncated and the distance
   * calculated will be wrong.
   */
  /**
   * \brief Shift for fuel bitmask
   */
  static constexpr uint32_t FuelShift = 32;
  // Need to make sure that fuel, slope & aspect aren't in first 32 bits
  static_assert(32 <= FuelShift);
  /**
   * \brief Number of bits in fuel bitmask
   */
  static constexpr uint32_t FuelBits = std::bit_width<uint32_t>(NUMBER_OF_FUELS);
  /**
   * \brief Bitmask for fuel information in Topo before shift
   */
  static constexpr Topo FuelBitMask = util::bit_mask<FuelBits, Topo>();
  static_assert(FuelBitMask == 0xFF);
  static_assert(FuelBitMask >= NUMBER_OF_FUELS);
  /**
   * \brief Bitmask for fuel information in Topo
   */
  static constexpr Topo FuelMask = FuelBitMask << FuelShift;
  /**
   * \brief Shift for aspect bitmask
   */
  static constexpr uint32_t AspectShift = FuelBits + FuelShift;
  /**
   * \brief Number of bits in aspect bitmask
   */
  static constexpr uint32_t AspectBits = std::bit_width<uint32_t>(MAX_ASPECT);
  /**
   * \brief Bitmask for aspect in Topo before shift
   */
  static constexpr Topo AspectBitMask = util::bit_mask<AspectBits, Topo>();
  static_assert(AspectBitMask == 0x1FF);
  static_assert(AspectBitMask >= INVALID_ASPECT);
  /**
   * \brief Bitmask for aspect in Topo
   */
  static constexpr Topo AspectMask = AspectBitMask << AspectShift;
  /**
   * \brief Shift for slope bitmask
   */
  static constexpr uint32_t SlopeShift = AspectBits + AspectShift;
  /**
   * \brief Number of bits in slope bitmask
   */
  static constexpr uint32_t SlopeBits = std::bit_width<uint32_t>(MAX_SLOPE_FOR_DISTANCE);
  static_assert(SlopeBits == 9);
  /**
   * \brief Bitmask for slope in Topo before shift
   */
  static constexpr Topo SlopeBitMask = util::bit_mask<SlopeBits, Topo>();
  static_assert(SlopeBitMask == 0x1FF);
  static_assert(SlopeBitMask >= INVALID_SLOPE);
  /**
   * \brief Bitmask for slope in Topo
   */
  static constexpr Topo SlopeMask = SlopeBitMask << SlopeShift;
  /**
   * \brief Bitmask for Cell information in Topo
   */
  static constexpr Topo CellMask = HashMask | FuelMask | AspectMask | SlopeMask;
  static_assert(static_cast<size_t>(std::bit_width(std::numeric_limits<Topo>::max())) >= SlopeBits + SlopeShift);
};
/**
 * \brief Less than operator
 * \param lhs First Cell
 * \param rhs Second Cell
 * \return Whether or not first is less than second
 */
constexpr bool operator<(const Cell& lhs, const Cell& rhs)
{
  return lhs.topoHash() < rhs.topoHash();
}
}
namespace std
{
/**
 * \brief Provides hashing of Cell objects
 */
template <>
struct hash<fs::topo::Cell>
{
  /**
   * \brief Provides hash for Cell objects
   * \param k Cell to get hash for
   * \return Hash value
   */
  std::size_t operator()(const fs::topo::Cell& k) const noexcept
  {
    return k.fullHash();
  }
};
}
