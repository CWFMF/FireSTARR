/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once

// if in debug mode then set everything, otherwise uncomment turning things off if trying to debug specific things
#define DEBUG_DIRECTIONS
#define DEBUG_FUEL_VARIABLE
#define DEBUG_FWI_WEATHER
#define DEBUG_GRIDS
#define DEBUG_POINTS
#define DEBUG_PROBABILITY
#define DEBUG_SIMULATION
#define DEBUG_STATISTICS
// #define DEBUG_WEATHER
// // use this for trying to troubleshoot specific problems short-term
// #define DEBUG_TEMPORARY

#ifdef NDEBUG

#undef DEBUG_DIRECTIONS
#undef DEBUG_FUEL_VARIABLE
#undef DEBUG_FWI_WEATHER
#undef DEBUG_GRIDS
#undef DEBUG_POINTS
#undef DEBUG_PROBABILITY
#undef DEBUG_SIMULATION
#undef DEBUG_STATISTICS
#undef DEBUG_WEATHER

#endif

#if not(defined(NDEBUG)) || defined(DEBUG_DIRECTIONS) || defined(DEBUG_FUEL_VARIABLE) || defined(DEBUG_FWI_WEATHER) || defined(DEBUG_GRIDS) || defined(DEBUG_POINTS) || defined(DEBUG_PROBABILITY) || defined(DEBUG_SIMULATION) || defined(DEBUG_STATISTICS) || defined(DEBUG_WEATHER)
#define DEBUG_ANY
#endif

namespace fs::debug
{
void show_debug_settings();
}
