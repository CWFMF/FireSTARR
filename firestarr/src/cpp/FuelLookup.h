/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <memory>
#include <set>
#include <string>
#include "FuelType.h"
#include "Cell.h"
#include "Util.h"
namespace fs::fuel
{
class FuelLookupImpl;

string simplify_fuel_name(const string& fuel);

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
  /**
   * \brief Copy constructor
   * \param rhs FuelLookup to copy from
   */
  FuelLookup(const FuelLookup& rhs) noexcept = default;
  /**
   * \brief Move constructor
   * \param rhs FuelLookup to move from
   */
  FuelLookup(FuelLookup&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs FuelLookup to copy from
   * \return This, after assignment
   */
  FuelLookup& operator=(const FuelLookup& rhs) noexcept = default;
  /**
   * \brief Move assignment
   * \param rhs FuelLookup to move from
   * \return This, after assignment
   */
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
  [[nodiscard]] const FuelType* byName(const string& name) const;
  /**
   * \brief Look up a FuelType based on the given simplified name
   * \param name Simplified name of the fuel to find
   * \return FuelType based on the given name
   */
  [[nodiscard]] const FuelType* bySimplifiedName(const string& name) const;
  /**
   * \brief Array of all FuelTypes available to be used in simulations
   */
  static const array<const FuelType*, NUMBER_OF_FUELS> Fuels;
private:
  /**
   * \brief Implementation class for FuelLookup
   */
  shared_ptr<FuelLookupImpl> impl_;
};
/**
 * \brief Look up a FuelType based on the given code
 * \param code Value to use for lookup
 * \return FuelType based on the given code
 */
[[nodiscard]] constexpr const FuelType* fuel_by_code(const FuelCodeSize& code)
{
  return FuelLookup::Fuels.at(code);
}
/**
 * \brief Get FuelType based on the given cell
 * \param cell Cell to retrieve FuelType for
 * \return FuelType based on the given cell
 */
[[nodiscard]] constexpr const FuelType* check_fuel(const topo::Cell& cell)
{
  return fuel_by_code(cell.fuelCode());
}
/**
 * \brief Whether or not there is no fuel in the Cell
 * \param cell Cell to check
 * \return Whether or not there is no fuel in the Cell
 */
[[nodiscard]] constexpr bool is_null_fuel(const FuelType* fuel)
{
  return INVALID_FUEL_CODE == FuelType::safeCode(fuel);
}
/**
 * \brief Whether or not there is no fuel in the Cell
 * \param cell Cell to check
 * \return Whether or not there is no fuel in the Cell
 */
[[nodiscard]] constexpr bool is_null_fuel(const topo::Cell& cell)
{
  return is_null_fuel(fuel_by_code(cell.fuelCode()));
}
}
