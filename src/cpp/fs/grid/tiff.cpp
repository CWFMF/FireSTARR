/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "tiff.h"
#include <geo_normalize.h>
#include <tiffio.h>
#include <xtiffio.h>
#include "../log/Log.h"
namespace fs
{
TIFF* GeoTiffOpen(const char* const filename, const char* const mode)
{
  TIFF* tif = XTIFFOpen(filename, mode);
  // HACK: avoid warning about const char* cast
  char C_GDALNoDataValue[] = "GDALNoDataValue";
  char C_GeoPixelScale[] = "GeoPixelScale";
  char C_GeoTransformationMatrix[] = "GeoTransformationMatrix";
  char C_GeoTiePoints[] = "GeoTiePoints";
  char C_GeoKeyDirectory[] = "GeoKeyDirectory";
  char C_GeoDoubleParams[] = "GeoDoubleParams";
  char C_GeoASCIIParams[] = "GeoASCIIParams";
  static const TIFFFieldInfo xtiffFieldInfo[] = {
    {TIFFTAG_GDAL_NODATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false, C_GDALNoDataValue},
    {TIFFTAG_GEOPIXELSCALE, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, C_GeoPixelScale},
    {TIFFTAG_GEOTRANSMATRIX,
     -1,
     -1,
     TIFF_DOUBLE,
     FIELD_CUSTOM,
     true,
     true,
     C_GeoTransformationMatrix},
    {TIFFTAG_GEOTIEPOINTS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, C_GeoTiePoints},
    {TIFFTAG_GEOKEYDIRECTORY, -1, -1, TIFF_SHORT, FIELD_CUSTOM, true, true, C_GeoKeyDirectory},
    {TIFFTAG_GEODOUBLEPARAMS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, C_GeoDoubleParams},
    {TIFFTAG_GEOASCIIPARAMS, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, true, C_GeoASCIIParams}
  };
  TIFFMergeFieldInfo(tif, xtiffFieldInfo, sizeof(xtiffFieldInfo) / sizeof(xtiffFieldInfo[0]));
  return tif;
}
GeoTiff::~GeoTiff()
{
  if (gtif_)
  {
    GTIFFree(gtif_);
  }
  if (tiff_)
  {
    XTIFFClose(tiff_);
  }
}
GeoTiff::GeoTiff(const string_view filename, const char* const mode)
  : mode_(mode), filename_(filename), tiff_([&]() {
      logging::debug("{:s} file {:s}", ('r' == mode[0] ? "Reading" : "Writing"), filename_);
      // suppress warnings about geotiff tags that aren't found
      TIFFSetWarningHandler(nullptr);
      auto tiff = GeoTiffOpen(filename_.c_str(), mode);
      logging::check_fatal(!tiff, "Cannot open file {:s} as a TIF", filename_);
      return tiff;
    }()),
    gtif_([&]() {
      auto gtif = GTIFNew(tiff_);
      logging::check_fatal(!gtif, "Cannot open file {:s} as a GEOTIFF", filename_);
      return gtif;
    }())
{ }
}
