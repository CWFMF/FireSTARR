/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_CELL_H
#define FS_CELL_H
#include "../stdafx.h"
#include "../util/Util.h"
namespace fs
{
using SpreadKey = uint32_t;
/**
 * \brief A Position with a Slope, Aspect, and Fuel.
 */
class Cell
{
protected:
  /**
   * \brief Stored hash that contains x and y data
   */
  SpreadKey data_;

public:
  /**
   * \brief Full stored hash that may contain data from subclasses
   * \return Full stored hash that may contain data from subclasses
   */
  [[nodiscard]] constexpr SpreadKey fullHash() const { return data_; }
  /**
   * \brief Construct from hash value
   * \param hash Hash defining all attributes
   */
  explicit constexpr Cell(const SpreadKey hash) noexcept : data_{hash} { }
  /**
   * \brief Hash attributes into a SpreadKey value
   * \param slope Slope
   * \param aspect Aspect
   * \param fuel Fuel
   * \return Hash
   */
  [[nodiscard]] static constexpr SpreadKey hashCell(
    const SlopeSize slope,
    const AspectSize aspect,
    const FuelCodeSize& fuel
  ) noexcept
  {
    // HACK: so we can call and set it all invalid if anything is
    // if any are invalid then they all should be
    const auto do_hash_cell = [](const SlopeSize s, const AspectSize a, const FuelCodeSize f) {
      return static_cast<SpreadKey>(f) << FuelShift | static_cast<SpreadKey>(s) << SlopeShift
           | static_cast<SpreadKey>(a) << AspectShift;
    };
    if (INVALID_SLOPE == slope || INVALID_ASPECT == aspect || INVALID_FUEL_CODE == fuel)
    {
      return do_hash_cell(INVALID_SLOPE, INVALID_ASPECT, INVALID_FUEL_CODE);
    }
    // if slope is 0 make aspect north so less unique keys
    return do_hash_cell(slope, 0 == slope ? 0 : aspect, fuel);
  }
  constexpr Cell() noexcept
    : Cell(
        numeric_limits<SlopeSize>::min(),
        numeric_limits<AspectSize>::min(),
        numeric_limits<FuelCodeSize>::min()
      )
  { }
  /**
   * \brief Construct based on given attributes
   * \param hash Hash of x and y
   * \param slope Slope
   * \param aspect Aspect
   * \param fuel Fuel
   */
  constexpr Cell(const SlopeSize slope, const AspectSize aspect, const FuelCodeSize& fuel) noexcept
    : Cell(hashCell(slope, aspect, fuel))
  { }
  /**
   * \brief A key defining Slope, Aspect, and Fuel, used for determining Cells that spread the same
   * \param value SpreadKey to extract from
   * \return A key defining Slope, Aspect, and Fuel
   */
  [[nodiscard]] static constexpr SpreadKey key(const SpreadKey value) noexcept
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
    return static_cast<AspectSize>((value & AspectMask) >> AspectShift);
  }
  /**
   * \brief Fuel
   * \param value SpreadKey to extract from
   * \return Fuel
   */
  [[nodiscard]] static constexpr FuelCodeSize fuelCode(const SpreadKey value) noexcept
  {
    return static_cast<FuelCodeSize>((value & FuelMask) >> FuelShift);
  }
  /**
   * \brief Slope (degrees)
   * \param value SpreadKey to extract from
   * \return Slope (degrees)
   */
  [[nodiscard]] static constexpr SlopeSize slope(const SpreadKey value) noexcept
  {
    return static_cast<SlopeSize>((value & SlopeMask) >> SlopeShift);
  }
  /**
   * \brief A key defining Slope, Aspect, and Fuel, used for determining Cells that spread the same
   * \return A key defining Slope, Aspect, and Fuel
   */
  [[nodiscard]] constexpr SpreadKey key() const noexcept { return Cell::key(data_); }
  /**
   * \brief Aspect (degrees)
   * \return Aspect (degrees)
   */
  [[nodiscard]] constexpr AspectSize aspect() const noexcept { return Cell::aspect(data_); }
  /**
   * \brief Fuel
   * \return Fuel
   */
  [[nodiscard]] constexpr FuelCodeSize fuelCode() const noexcept { return Cell::fuelCode(data_); }
  /**
   * \brief Slope (degrees)
   * \return Slope (degrees)
   */
  [[nodiscard]] constexpr SlopeSize slope() const noexcept { return Cell::slope(data_); }

protected:
  /*
   * Field                    Natural Range     Used Range      Bits    Bit Range
   * Y                        0 - 4095          0 - 4095        12      0 - 4095
   * X                        0 - 4095          0 - 4095        12      0 - 4095
   * PADDING                                                    10
   * Fuel                     0 - 140           0 - 140         8       0 - 255
   * Aspect                   0 - 359           0 - 359         9       0 - 511
   * Slope                    0 - infinity      0 - 511         9       0 - 511
   * Extra                                                      8
   *
   * X and Y are restricted to 4096 since that's what gets clipped out of
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
   * horizontal rate of spread if the slope has been truncated and the distance
   * calculated will be wrong.
   */
  /**
   * \brief Shift for fuel bitmask
   */
  static constexpr uint32_t FuelShift = 0;
  // // Need to make sure that fuel, slope & aspect aren't in first 32 bits
  // static_assert(32 <= FuelShift);
  /**
   * \brief Number of bits in fuel bitmask
   */
  static constexpr uint32_t FuelBits = std::bit_width<uint32_t>(NUMBER_OF_FUELS);
  /**
   * \brief Bitmask for fuel information in SpreadKey before shift
   */
  static constexpr SpreadKey FuelBitMask = bit_mask<FuelBits, SpreadKey>();
  static_assert(FuelBitMask == 0xFF);
  static_assert(FuelBitMask >= NUMBER_OF_FUELS);
  /**
   * \brief Bitmask for fuel information in SpreadKey
   */
  static constexpr SpreadKey FuelMask = FuelBitMask << FuelShift;
  /**
   * \brief Shift for aspect bitmask
   */
  static constexpr uint32_t AspectShift = FuelBits + FuelShift;
  /**
   * \brief Number of bits in aspect bitmask
   */
  static constexpr uint32_t AspectBits = std::bit_width<uint32_t>(MAX_ASPECT);
  /**
   * \brief Bitmask for aspect in SpreadKey before shift
   */
  static constexpr SpreadKey AspectBitMask = bit_mask<AspectBits, SpreadKey>();
  static_assert(AspectBitMask == 0x1FF);
  static_assert(AspectBitMask >= INVALID_ASPECT);
  /**
   * \brief Bitmask for aspect in SpreadKey
   */
  static constexpr SpreadKey AspectMask = AspectBitMask << AspectShift;
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
   * \brief Bitmask for slope in SpreadKey before shift
   */
  static constexpr SpreadKey SlopeBitMask = bit_mask<SlopeBits, SpreadKey>();
  static_assert(SlopeBitMask == 0x1FF);
  static_assert(SlopeBitMask >= INVALID_SLOPE);
  /**
   * \brief Bitmask for slope in SpreadKey
   */
  static constexpr SpreadKey SlopeMask = SlopeBitMask << SlopeShift;
  static_assert(
    static_cast<size_t>(std::bit_width(std::numeric_limits<SpreadKey>::max()))
    >= SlopeBits + SlopeShift
  );
};
}
#endif
