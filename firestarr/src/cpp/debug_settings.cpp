/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "debug_settings.h"
#include <string.h>
#include <stdio.h>
#include <string>

namespace fs::debug
{
constexpr auto HR = "**********************************************************************";
constexpr auto EDGE = 3;
constexpr auto WIDTH = static_cast<int>(sizeof(HR)) - (2 * (EDGE + 1));

void printf_centered(const char* str)
{
  const auto n = static_cast<int>(strlen(str));
  const auto pad_left = ((WIDTH - n) / 2) + n;
  const auto pad_right = WIDTH - pad_left;
  printf("%.*s %*s%*s %.*s\n", EDGE, HR, pad_left, str, pad_right, "", EDGE, HR);
};

void show_debug_settings()
{
#ifdef DEBUG_ANY
  printf("%s\n", HR);
  printf_centered("DEBUG OPTIONS");
  printf("%s\n", HR);
#endif

#ifndef NDEBUG
  printf_centered("DEBUG");
#endif
#ifdef DEBUG_DIRECTIONS
  printf_centered("DEBUG_DIRECTIONS");
#endif
#ifdef DEBUG_FUEL_VARIABLE
  printf_centered("DEBUG_FUEL_VARIABLE");
#endif
#ifdef DEBUG_FWI_WEATHER
  printf_centered("DEBUG_FWI_WEATHER");
#endif
#ifdef DEBUG_GRIDS
  printf_centered("DEBUG_GRIDS");
#endif
#ifdef DEBUG_POINTS
  printf_centered("DEBUG_POINTS");
#endif
#ifdef DEBUG_PROBABILITY
  printf_centered("DEBUG_PROBABILITY");
#endif
#ifdef DEBUG_SIMULATION
  printf_centered("DEBUG_SIMULATION");
#endif
#ifdef DEBUG_STATISTICS
  printf_centered("DEBUG_STATISTICS");
#endif
#ifdef DEBUG_WEATHER
  printf_centered("DEBUG_WEATHER");
#endif
#ifdef DEBUG_ANY
  printf("%s\n", HR);
#endif
}
}
