/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "ConstantGrid.h"
#include <geo_normalize.h>
#include <tiffio.h>
#include <xtiffio.h>
#include "Log.h"
#include "tiff.h"
namespace fs
{
template <class T>
int value_at_int(void* const buf, const FullIdx offset)
{
  auto cur = *(static_cast<T*>(buf) + offset);
  return static_cast<int>(cur);
};
// ConstantGrids are all int values of some kind
/**
 * \brief Read a section of a TIFF into a ConstantGrid
 * \param filename File name to read from
 * \param point Point to center ConstantGrid on
 * \return ConstantGrid containing clipped data for TIFF
 */
[[nodiscard]] ConstantGrid<int, int> readTiffInt(const string_view filename, const Point& point)
{
  GeoTiff geotiff{filename, "r"};
  auto tif = geotiff.tiff();
#ifdef DEBUG_GRIDS
  auto min_value = std::numeric_limits<int>::max();
  auto max_value = std::numeric_limits<int>::min();
#endif
  logging::debug(
    "Raster type int has limits {:d}, {:d}",
    std::numeric_limits<int>::min(),
    std::numeric_limits<int>::max()
  );
  const GridBase grid_info = read_header(geotiff);
  uint32_t tile_width;
  uint32_t tile_length;
  TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
  TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_length);
  void* data;
  uint32_t count;
  TIFFGetField(tif, TIFFTAG_GDAL_NODATA, &count, &data);
  logging::check_fatal(0 == count, "NODATA value is not set in input");
  logging::debug("NODATA value is '{:s}'", static_cast<char*>(data));
  const auto nodata_orig = stoi(string(static_cast<char*>(data)));
  logging::debug("NODATA value is originally parsed as {:d}", nodata_orig);
  const auto nodata_input = static_cast<int>(stoi(string(static_cast<char*>(data))));
  logging::debug("NODATA value is parsed as {:d}", nodata_input);
  auto actual_height = grid_info.calculateHeight();
  auto actual_width = grid_info.calculateWidth();
  const auto coordinates = grid_info.findFullCoordinates(point, true);
  logging::verbose(
    "Coordinates before reading are ({:d}, {:d} => {:f}, {:f})",
    coordinates->x,
    coordinates->y,
    coordinates->x + coordinates->x_sub / 1000.0,
    coordinates->y + coordinates->y_sub / 1000.0
  );
  auto min_x = max(
    static_cast<FullIdx>(0),
    static_cast<FullIdx>(coordinates->x - static_cast<FullIdx>(MAX_WIDTH) / static_cast<FullIdx>(2))
  );
  if (min_x + MAX_WIDTH >= actual_width)
  {
    min_x = max(static_cast<FullIdx>(0), actual_width - MAX_WIDTH);
  }
  // make sure we're at the start of a tile
  const auto tile_x = tile_width * static_cast<FullIdx>(min_x / tile_width);
  const auto max_x = static_cast<FullIdx>(min(min_x + MAX_WIDTH - 1, actual_width));
#ifdef DEBUG_GRIDS
  logging::check_fatal(min_x < 0, "X can't be less than 0");
  logging::check_fatal(max_x - min_x > MAX_WIDTH, "Can't have width more than {:d}", MAX_WIDTH);
  logging::check_fatal(
    max_x > actual_width, "Can't have more than actual width {:d}", actual_width
  );
#endif
  auto min_y = max(
    static_cast<FullIdx>(0),
    static_cast<FullIdx>(
      coordinates->y - static_cast<FullIdx>(MAX_HEIGHT) / static_cast<FullIdx>(2)
    )
  );
  if (min_y + MAX_WIDTH >= actual_height)
  {
    min_y = max(static_cast<FullIdx>(0), actual_height - MAX_HEIGHT);
  }
  const auto tile_y = tile_width * static_cast<FullIdx>(min_y / tile_width);
  const auto max_y = static_cast<FullIdx>(min(min_y + MAX_HEIGHT - 1, actual_height));
#ifdef DEBUG_GRIDS
  logging::check_fatal(min_y < 0, "Y can't be less than 0 but is {:d}", min_y);
  logging::check_fatal(
    max_y - min_y > MAX_HEIGHT,
    "Can't have height more than {:d} but have {:d}",
    MAX_HEIGHT,
    max_y - min_y
  );
  logging::check_fatal(
    max_y > actual_height, "Can't have more than actual height {:d}", actual_height
  );
#endif
  vector<int> values(static_cast<size_t>(MAX_HEIGHT) * MAX_WIDTH, nodata_input);
  logging::verbose("{:s}: malloc start", geotiff.filename());
  int bps = std::numeric_limits<int>::digits + (1 * std::numeric_limits<int>::is_signed);
  uint16_t sample_format;
  logging::check_fatal(
    !TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sample_format),
    "Cannot determine TIFFTAG_SAMPLEFORMAT"
  );
  uint16_t bps_file;
  logging::check_fatal(
    !TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_file), "Cannot determine TIFFTAG_BITSPERSAMPLE"
  );
  logging::check_fatal(
    bps < bps_file,
    "Raster {:s} type is larger than expected type ({:d} bits instead of {:d})",
    geotiff.filename(),
    bps_file,
    bps
  );
#ifdef DEBUG_GRIDS
  int bps_int16_t =
    std::numeric_limits<int16_t>::digits + (1 * std::numeric_limits<int16_t>::is_signed);
  logging::debug("Size of pointer to int is {:d} vs {:d}", sizeof(int16_t*), sizeof(int*));
  logging::debug(
    "Raster {:s} calculated bps for type int is {:d}; tif says bps is {:d}; int16_t is {:d}",
    geotiff.filename(),
    bps,
    bps_file,
    bps_int16_t
  );
#endif
  const auto tile_size = TIFFTileSize(tif);
  logging::debug("Tile size for reading {:s} is {:d}", geotiff.filename(), tile_size);
  const auto buf = _TIFFmalloc(tile_size);
  logging::verbose("{:s}: read start", geotiff.filename());
  const tsample_t smp{};
  logging::debug(
    "Want to clip grid to ({:d}, {:d}) => ({:d}, {:d}) for a {:d}{:d} raster",
    min_x,
    min_y,
    max_x,
    max_y,
    actual_width,
    actual_height
  );
  // HACK: it really feels like there should be a better way to do this
  switch (sample_format)
  {
    case SAMPLEFORMAT_VOID:
      exit(logging::fatal("SAMPLEFORMAT_VOID unsupported"));
    case SAMPLEFORMAT_UINT:
      break;
    case SAMPLEFORMAT_INT:
      break;
    case SAMPLEFORMAT_IEEEFP:
      exit(logging::fatal("Expected integer raster but got SAMPLEFORMAT_IEEEFP"));
    case SAMPLEFORMAT_COMPLEXINT:
      exit(logging::fatal("Expected integer raster but got SAMPLEFORMAT_COMPLEXINT"));
    case SAMPLEFORMAT_COMPLEXIEEEFP:
      exit(logging::fatal("Expected integer raster but got SAMPLEFORMAT_COMPLEXIEEEFP"));
    default:
      exit(logging::fatal("Unknown TIFFTAG_SAMPLEFORMAT value {:d}", sample_format));
  }
  auto value_at = [&]() {
    if (SAMPLEFORMAT_UINT == sample_format)
    {
      // unsigned int
      if (8 == bps_file)
      {
        return &value_at_int<uint8_t>;
      }
      if (16 == bps_file)
      {
        return &value_at_int<uint16_t>;
      }
      if (32 == bps_file)
      {
        return &value_at_int<uint32_t>;
      }
    }
    else if (SAMPLEFORMAT_INT == sample_format)
    {
      // signed int
      if (8 == bps_file)
      {
        return &value_at_int<int8_t>;
      }
      if (16 == bps_file)
      {
        return &value_at_int<int16_t>;
      }
      if (32 == bps_file)
      {
        return &value_at_int<int32_t>;
      }
    }
    // return &value_at_int<int32_t>;
    exit(logging::fatal(
      "SAMPLEFORMAT {:d} has invalid TIFFTAG_BITSPERSAMPLE {:d}", sample_format, bps_file
    ));
  }();
  for (auto h = tile_y; h <= max_y; h += tile_length)
  {
    for (auto w = tile_x; w <= max_x; w += tile_width)
    {
      std::ignore =
        TIFFReadTile(tif, buf, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 0, smp);
      for (FullIdx y = 0; (y < static_cast<FullIdx>(tile_length)) && (y + h <= max_y); ++y)
      {
        // read in so that (0, 0) has a hash of 0
        const auto y0 = static_cast<HashSize>((h - min_y) + y);
        const auto actual_y = (max_y - min_y) - y0;
        if (actual_y >= 0 && actual_y < MAX_HEIGHT)
        {
          for (auto x = 0; (x < static_cast<FullIdx>(tile_width)) && (x + w <= max_x); ++x)
          {
            const auto offset = y * tile_width + x;
            const auto actual_x = ((w - min_x) + x);
            if (actual_x >= 0 && actual_x < MAX_HEIGHT)
            {
              const auto cur_hash = actual_y * MAX_WIDTH + actual_x;
              // auto cur = *(static_cast<int*>(buf) + offset);
              auto cur = value_at(buf, offset);
#ifdef DEBUG_GRIDS
              min_value = min(cur, min_value);
              max_value = max(cur, max_value);
#endif
              values.at(cur_hash) = cur;
            }
          }
        }
      }
    }
  }
  logging::verbose("{:s}: read end", geotiff.filename());
  _TIFFfree(buf);
  logging::verbose("{:s}: free end", geotiff.filename());
  const auto new_xll =
    grid_info.xllcorner() + (static_cast<MathSize>(min_x) * grid_info.cellSize());
  const auto new_yll =
    grid_info.yllcorner()
    + (static_cast<MathSize>(actual_height) - static_cast<MathSize>(max_y)) * grid_info.cellSize();
#ifdef DEBUG_GRIDS
  logging::check_fatal(new_yll < grid_info.yllcorner(), "New yllcorner is outside original grid");
#endif
  logging::verbose(
    "Translated lower left is ({:f}, {:f}) from ({:f}, {:f})",
    new_xll,
    new_yll,
    grid_info.xllcorner(),
    grid_info.yllcorner()
  );
  const auto height_calc = max_y - min_y + 1;
  const auto width_calc = max_x - min_x + 1;
  ConstantGrid<int, int> result{
    grid_info.cellSize(),
    static_cast<Idx>(width_calc),
    static_cast<Idx>(height_calc),
    nodata_input,
    nodata_input,
    new_xll,
    new_yll,
    new_xll + (static_cast<MathSize>(width_calc) + 1) * grid_info.cellSize(),
    new_yll + (static_cast<MathSize>(height_calc) + 1) * grid_info.cellSize(),
    grid_info.proj4(),
    std::move(values)
  };
  auto new_location = result.findCoordinates(point, false);
#ifdef DEBUG_GRIDS
  logging::check_fatal(!new_location.has_value(), "Invalid location after reading");
#endif
  logging::debug(
    "Coordinates are ({:d}, {:d} => {:f}, {:f})",
    new_location->x,
    new_location->y,
    new_location->x + new_location->x_sub / 1000.0,
    new_location->y + new_location->y_sub / 1000.0
  );
#ifdef DEBUG_GRIDS
  logging::note("Values for {:s} range from {:d} to {:d}", filename, min_value, max_value);
#endif
  return result;
}
}
