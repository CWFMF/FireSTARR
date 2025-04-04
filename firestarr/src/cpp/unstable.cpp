/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "unstable.h"
#include <cmath>

MathSize _cos(const double angle) noexcept
{
  return static_cast<MathSize>(cos(angle));
}
MathSize _sin(const double angle) noexcept
{
  return static_cast<MathSize>(sin(angle));
}
