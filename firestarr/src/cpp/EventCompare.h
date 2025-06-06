/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "Event.h"
namespace fs::sim
{
/**
 * \brief Defines how Events are compared for sorting.
 */
struct EventCompare
{
  /**
   * \brief Defines ordering on Events
   * \param x First Event
   * \param y Second Event
   * \return Whether first Event is less than second Event
   */
  [[nodiscard]] constexpr bool operator()(const Event& x, const Event& y) const
  {
    if (x.time() == y.time())
    {
      if (x.type() == y.type())
      {
        return x.cell().hash() < y.cell().hash();
      }
      return x.type() < y.type();
    }
    return x.time() < y.time();
  }
};
}
