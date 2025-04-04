/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

// Provides wrappers around functions that have different results when using
// different optimization flags, so they can be compiled with the same
// flags in release and debug mode to avoid changing the outputs.
#pragma once

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#endif

/**
 * \brief Type used for math operations
 */
using MathSize = double;

MathSize _cos(double angle) noexcept;
MathSize _sin(double angle) noexcept;
