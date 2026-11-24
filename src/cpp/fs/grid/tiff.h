/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TIFF_H
#define FS_TIFF_H
#include "../stdafx.h"
#ifndef TIFFTAG_GDAL_NODATA
#define TIFFTAG_GDAL_NODATA 42113
#endif
using TIFF = struct tiff;
using GTIF = struct gtiff;
namespace fs
{
/**
 * Open file and register GeoTIFF tags so we can read and write properly
 * @param filename Name of file to open
 * @param mode Mode to open file with
 * @return Handle to open TIFF with fields registered
 */
class GeoTiff
{
public:
  ~GeoTiff();
  GeoTiff(const string_view filename, const char* const mode);
  const char* filename() const { return filename_.c_str(); }
  TIFF* tiff() { return tiff_; }
  GTIF* gtif() { return gtif_; }

private:
  string mode_{};
  string filename_{};
  TIFF* tiff_{nullptr};
  GTIF* gtif_{nullptr};
};
}
#endif
