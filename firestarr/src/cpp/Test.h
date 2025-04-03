/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
namespace tbd::wx
{
class FwiWeather;
};
namespace tbd::sim
{
#include "stdafx.h"

static const double TEST_GRID_SIZE = 100.0;
static const char TEST_PROJ4[] =
  "+proj=tmerc +lat_0=0.000000000 +lon_0=-90.000000000"
  " +k=0.999600 +x_0=500000.000 +y_0=0.000 +a=6378137.000 +b=6356752.314 +units=m";
static const double TEST_XLLCORNER = 324203.990666;
static const double TEST_YLLCORNER = 12646355.311160;
/**
 * \brief Runs test cases for constant weather and fuel based on given arguments.
 * \param dir_out root directory for outputs
 * \param fuel_name FBP fuel to use or empty string for default
 * \param wx FwiWeather to use for constant indices, where anything Invalid uses default
 * \param test_all whether to run all combinations of test outputs filtered by criteria (true), or to just run the default single value modified by whatever has been specified (false)
 * \return
 */
int test(
  const string& output_directory,
  const DurationSize num_hours,
  const tbd::wx::FwiWeather* wx,
  const string& fuel_name,
  const SlopeSize slope,
  const AspectSize aspect,
  const bool test_all);
}
