/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FUEL_LOOKUP_H
#define FS_FUEL_LOOKUP_H
#include "../stdafx.h"
#include "FuelType.h"
namespace fs::fuel
{
class FuelLookupImpl;
/**
 * \brief Provides ability to look up a fuel type based on name or code.
 */
class FuelLookup
{
public:
  ~FuelLookup() = default;
  /**
   * \brief Construct by reading from a file
   * \param filename File to read from. Uses .lut format from Prometheus
   */
  explicit FuelLookup(const char* filename);
  FuelLookup(const FuelLookup& rhs) noexcept = default;
  FuelLookup(FuelLookup&& rhs) noexcept = default;
  FuelLookup& operator=(const FuelLookup& rhs) noexcept = default;
  FuelLookup& operator=(FuelLookup&& rhs) noexcept = default;
  /**
   * \brief Look up a FuelType based on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return FuelType based on the given code
   */
  [[nodiscard]] const FuelType* codeToFuel(FuelSize value, FuelSize nodata) const;
  /**
   * \brief List all fuels and their codes
   */
  void listFuels() const;
  /**
   * \brief Look up the original code for the given FuelType
   * \param value Value to use for lookup
   * \return code for the given FuelType
   */
  [[nodiscard]] FuelSize fuelToCode(const FuelType* fuel) const;
  /**
   * \brief Look up a FuelType ba1ed on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return FuelType based on the given code
   */
  [[nodiscard]] const FuelType* operator()(FuelSize value, FuelSize nodata) const;
  /**
   * \brief Retrieve set of FuelTypes that are used in the lookup table
   * \return set of FuelTypes that are used in the lookup table
   */
  [[nodiscard]] set<const FuelType*> usedFuels() const;
  /**
   * \brief Look up a FuelType based on the given name
   * \param name Name of the fuel to find
   * \return FuelType based on the given name
   */
  [[nodiscard]] const FuelType* byName(const string_view name) const;
  /**
   * \brief Look up a FuelType based on the given simplified name
   * \param name Simplified name of the fuel to find
   * \return FuelType based on the given name
   */
  [[nodiscard]] const FuelType* bySimplifiedName(const string_view name) const;
  /**
   * \brief Array of all FuelTypes available to be used in simulations
   */
  static const array<const FuelType*, NUMBER_OF_FUELS> Fuels;

private:
  /**
   * \brief Implementation class for FuelLookup
   */
  shared_ptr<FuelLookupImpl> impl_{nullptr};
};
/**
 * \brief Look up a FuelType based on the given code
 * \param code Value to use for lookup
 * \return FuelType based on the given code
 */
[[nodiscard]] const FuelType* fuel_by_code(const FuelCodeSize& code);
/**
 * \brief Get FuelType based on the given cell
 * \param cell Cell to retrieve FuelType for
 * \return FuelType based on the given cell
 */
[[nodiscard]] const FuelType* check_fuel(const Cell& cell);
const FuelType* check_fuel(const Cell& cell);
/**
 * \brief Whether or not there is no fuel in the Cell
 * \param cell Cell to check
 * \return Whether or not there is no fuel in the Cell
 */
[[nodiscard]] bool is_null_fuel(const FuelType* fuel);
/**
 * \brief Whether or not there is no fuel in the Cell
 * \param cell Cell to check
 * \return Whether or not there is no fuel in the Cell
 */
[[nodiscard]] bool is_null_fuel(const Cell& cell);
class LazyFuelLookup : public LazyPath
{
public:
  using LazyPath::LazyPath;
  const FuelLookup& lookup() const;
  LazyFuelLookup& operator=(const LazyFuelLookup& rhs) noexcept;
  LazyFuelLookup& operator=(LazyFuelLookup&& rhs) noexcept;
  LazyFuelLookup& operator=(const string& path) noexcept;

protected:
  mutable unique_ptr<FuelLookup> fuel_lookup_{nullptr};
};
}
#endif
