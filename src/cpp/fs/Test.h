/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TEST_H
#define FS_TEST_H
#include "Grid.h"
#include "Settings.h"
namespace fs
{
using settings::Settings;
static const TiffDouble TEST_GRID_SIZE = 100.0;
static const char TEST_PROJ4[] =
  "+proj=tmerc +lat_0=0.000000000 +lon_0=-90.000000000"
  " +k=0.999600 +x_0=500000.000 +y_0=0.000 +a=6378137.000 +b=6356752.314 +units=m";
static const TiffDouble TEST_XLLCORNER = 221803.990666;
static const TiffDouble TEST_YLLCORNER = 12543955.311160;
// Run test cases for constant weather and fuel based on given arguments.
int test(Settings& settings);
}
#endif
