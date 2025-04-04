/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "Util.h"
#define LOOKUP_TABLES_OFF 1
#undef LOOKUP_TABLES_OFF
namespace tbd::util
{
/**
 * \brief A table initialized using the given function ranging over the number of digits and precision.
 * \tparam Fct Function to apply over the range of values
 * \tparam IndexDigits Number of digits to use for range of values
 * \tparam Precision Precision in decimal places to use for range of values
 */
template <MathSize (*Fct)(const MathSize), int IndexDigits = 3, int Precision = 1>
class LookupTable
{
#ifndef LOOKUP_TABLES_OFF
  /**
   * \brief Array with enough space for function called with specific number of digits and precision
   */
  using ValuesArray = array<MathSize, pow_int<IndexDigits>(10) * pow_int<Precision>(10)>;
  /**
   * \brief Array of values from calling function
   */
  const ValuesArray values_;
  /**
   * \brief Call function with range of values with given precision
   * \return Results of function with range of values with given precision
   */
  [[nodiscard]] constexpr ValuesArray makeValues()
  {
    // FIX: would prefer consteval but c++26 or external library is required for cmath functions
    ValuesArray values{};
    for (size_t i = 0; i < values.size(); ++i)
    {
      const auto value = i / static_cast<MathSize>(pow_int<Precision>(10));
      values[i] = Fct(value);
    }
    return values;
  }
#endif
public:
  constexpr explicit LookupTable() noexcept
#ifndef LOOKUP_TABLES_OFF
    : values_(makeValues())
#endif
  {
  }
  ~LookupTable() = default;
  LookupTable(LookupTable&& rhs) noexcept = delete;
  LookupTable(const LookupTable& rhs) noexcept = delete;
  LookupTable& operator=(LookupTable&& rhs) noexcept = delete;
  LookupTable& operator=(const LookupTable& rhs) noexcept = delete;
  /**
   * \brief Get result of function lookup table was initialized with for given value
   * \param value value to get lookup result for
   * \return result of lookup for function at value
   */
  [[nodiscard]] constexpr MathSize operator()(const MathSize value) const
  {
#ifndef LOOKUP_TABLES_OFF
    return values_.at(static_cast<size_t>(value * pow_int<Precision>(10)));
#else
    return Fct(value);
#endif
  }
};
}
