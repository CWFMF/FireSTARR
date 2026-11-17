/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Grid.h"
#include <cstdint>
#include <geo_normalize.h>
#include <tiffio.h>
#include <xtiffio.h>
#include "Log.h"
#include "projection.h"
#include "tiff.h"
#include "unstable.h"
using fs::Idx;
namespace fs
{
using fs::find_value;
using fs::from_lat_long;
using fs::to_lat_long;
using fs::try_fix_meridian;
template <class R>
string saveToTiffFile(
  const GridBase& grid,
  const Idx width,
  const Idx height,
  const tuple<Idx, Idx, Idx, Idx> bounds,
  const string_view dir,
  const string_view base_name,
  const uint16_t bits_per_sample,
  const uint16_t sample_format,
  std::function<R(const XYIdx&)> value_at,
  const int nodata_as_int
)
{
  // https://gdal.org/en/stable/drivers/raster/gtiff.html
  // tile width/height must be divisible by 16
  auto tileWidth = static_cast<uint32_t>(min(static_cast<int>(ceil(width / 16.0) * 16), 256));
  auto tileHeight = static_cast<uint32_t>(min(static_cast<int>(ceil(height / 16.0) * 16), 256));
  auto [min_x, min_y, max_x, max_y] = bounds;
  // auto min_x = std::get<0>(bounds);
  // auto min_y = std::get<1>(bounds);
  // auto max_x = std::get<2>(bounds);
  // auto max_y = std::get<3>(bounds);
  logging::check_fatal(min_x > max_x, "Invalid bounds for width with {:d} => {:d}", min_x, max_x);
  logging::check_fatal(min_y > max_y, "Invalid bounds for height with {:d} => {:d}", min_y, max_y);
#ifdef DEBUG_GRIDS
  logging::debug("Bounds are ({:d}, {:d}), ({:d}, {:d}) initially", min_x, min_y, max_x, max_y);
#endif
  Idx c_min = 0;
  while (c_min + static_cast<Idx>(tileWidth) <= min_x)
  {
    c_min += static_cast<Idx>(tileWidth);
  }
  Idx c_max = c_min + static_cast<Idx>(tileWidth);
  while (c_max < max_x)
  {
    c_max += static_cast<Idx>(tileWidth);
  }
  min_x = c_min;
  max_x = c_max;
  Idx r_min = 0;
  while (r_min + static_cast<Idx>(tileHeight) <= min_y)
  {
    r_min += static_cast<Idx>(tileHeight);
  }
  Idx r_max = r_min + static_cast<Idx>(tileHeight);
  while (r_max < max_y)
  {
    r_max += static_cast<Idx>(tileHeight);
  }
  min_y = r_min;
  max_y = r_max;
  logging::check_fatal(min_x >= max_x, "Invalid bounds for width with {:d} => {:d}", min_x, max_x);
  logging::check_fatal(min_y >= max_y, "Invalid bounds for height with {:d} => {:d}", min_y, max_y);
#ifdef DEBUG_GRIDS
  logging::debug(
    "Bounds are ({:d}, {:d}), ({:d}, {:d}) after correction", min_x, min_y, max_x, max_y
  );
#endif
  logging::extensive("({:d}, {:d}) => ({:d}, {:d})", min_x, min_y, max_x, max_y);
  logging::check_fatal(
    static_cast<uint32_t>(max_x - min_x) % tileWidth != 0, "Invalid start and end x"
  );
  logging::check_fatal(
    static_cast<uint32_t>(max_y - min_y) % tileHeight != 0, "Invalid start and end y"
  );
  logging::extensive("Lower left corner is ({:d}, {:d})", min_x, min_y);
  logging::extensive("Upper right corner is ({:d}, {:d})", max_x, max_y);
  const MathSize xll = grid.xllcorner() + min_x * grid.cellSize();
  // offset is different for y since it's flipped
  const MathSize yll = grid.yllcorner() + (min_y)*grid.cellSize();
  logging::extensive("Lower left corner is ({:f}, {:f})", xll, yll);
  const auto height_calc = static_cast<size_t>(max_y - min_y);
  const auto width_calc = static_cast<size_t>(max_x - min_x);
  // ensure this is always divisible by tile size
  logging::check_fatal(
    0 != (height_calc % tileHeight),
    "Height {:d} not divisible by tile height {:d}",
    height_calc,
    tileHeight
  );
  logging::check_fatal(
    0 != (width_calc % tileWidth),
    "Width {:d} not divisible by tile width {:d}",
    width_calc,
    tileWidth
  );
  const auto filename = create_file_name(dir, base_name, "tif");
  GeoTiff geotiff{filename, "w"};
  auto tif = geotiff.tiff();
  auto gtif = geotiff.gtif();
  logging::check_fatal(!gtif, "Cannot open file {:s} as a GEOTIFF", filename);
  const TiffDouble xul = xll;
  const TiffDouble yul = grid.yllcorner() + (grid.cellSize() * max_y);
  TiffDouble tiePoints[6] = {0.0, 0.0, 0.0, xul, yul, 0.0};
  TiffDouble pixelScale[3] = {grid.cellSize(), grid.cellSize(), 0.0};
  // make sure to use floating point if values are
  TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, sample_format);
  // FIX: was using double, and that usually doesn't make sense, but sometime it might?
  // use buffer big enought to fit any (R  + '.000\0') + 1
  constexpr auto n = std::numeric_limits<R>::digits10;
  static_assert(n > 0);
  const auto nodata_str = std::format("{:d}.000", nodata_as_int);
  constexpr auto bps = sizeof(R) * 8;
  logging::check_fatal(
    bps != bits_per_sample,
    "Cannot have mismatched bps and type ({:d} != {:d})",
    bps,
    bits_per_sample
  );
  TIFFSetField(tif, TIFFTAG_GDAL_NODATA, nodata_str.c_str());
  logging::extensive("{:s} takes {:d} bits", base_name, bits_per_sample);
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width_calc);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height_calc);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField(tif, TIFFTAG_TILEWIDTH, tileWidth);
  TIFFSetField(tif, TIFFTAG_TILELENGTH, tileHeight);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
  GTIFSetFromProj4(gtif, grid.proj4().c_str());
  TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, tiePoints);
  TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixelScale);
  size_t tileSize = tileWidth * tileHeight;
  const auto buf_size = static_cast<tmsize_t>(tileSize * sizeof(R));
  logging::extensive("{:s} has buffer size {:d}", base_name, buf_size);
  // HACK: using R means size changes on different cpu types
  auto buf = static_cast<R*>(_TIFFmalloc(buf_size));
  for (uint32_t x0 = 0; x0 < width_calc; x0 += tileWidth)
  {
    for (uint32_t y0 = 0; y0 < height_calc; y0 += tileHeight)
    {
      std::fill_n(&buf[0], tileWidth * tileHeight, static_cast<R>(nodata_as_int));
      // NOTE: shouldn't need to check if writing outside of tile because we made bounds on tile
      // edges above need to put data from grid into buffer, but flipped vertically
      for (uint32_t x1 = 0; x1 < tileWidth; ++x1)
      {
        for (uint32_t y1 = 0; y1 < tileHeight; ++y1)
        {
          // FIX: did this to for sure not change behaviour, but sort this out
          const auto y2 =
            static_cast<Idx>(static_cast<uint32_t>(static_cast<Idx>(max_y)) - (y0 + y1 + 1));
          const auto x2 =
            static_cast<Idx>(static_cast<uint32_t>(static_cast<Idx>(min_x)) + x0 + x1);
          const XYIdx idx{x2, y2};
          // might be out of bounds if not divisible by number of tiles
          if (!(height <= y2 || 0 > y2 || width <= x2 || 0 > x2))
          {
            // HACK: was getting invalid rasters if assigning directly into buf
            const R value = value_at(idx);
            buf[x1 + y1 * tileWidth] = value;
          }
        }
      }
      const auto write_result = TIFFWriteTile(tif, buf, x0, y0, 0, 0);
      logging::check_fatal(write_result < 0, "Cannot write tile to {:s}", filename);
    }
  }
  GTIFWriteKeys(gtif);
  _TIFFfree(buf);
  return filename;
}
string GridBase::saveToTiffFileInt(
  const Idx width,
  const Idx height,
  const tuple<Idx, Idx, Idx, Idx> bounds,
  const string_view dir,
  const string_view base_name,
  const uint16_t bits_per_sample,
  // const uint16_t sample_format,
  const bool is_unsigned,
  std::function<int(const XYIdx&)> value_at,
  const int nodata_as_int
) const
{
  // logging::check_fatal(
  //   SAMPLEFORMAT_INT != sample_format && SAMPLEFORMAT_UINT != sample_format,
  //   "Expected int for SAMPLEFORMAT but got value {:d}",
  //   bits_per_sample
  // );
  logging::check_fatal(
    8 != bits_per_sample && 16 != bits_per_sample && 32 != bits_per_sample,
    "Expected integer to have 8, 16, or 32 bits but got {:d}",
    bits_per_sample
  );
  const auto sample_format = is_unsigned ? SAMPLEFORMAT_UINT : SAMPLEFORMAT_INT;
  if (is_unsigned)
  {
    // unsigned int
    if (8 == bits_per_sample)
    {
      return saveToTiffFile<uint8_t>(
        *this,
        width,
        height,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        static_cast<uint16_t>(sample_format),
        [&](const XYIdx& idx) { return static_cast<uint8_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (16 == bits_per_sample)
    {
      return saveToTiffFile<uint16_t>(
        *this,
        width,
        height,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        static_cast<uint16_t>(sample_format),
        [&](const XYIdx& idx) { return static_cast<uint16_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (32 == bits_per_sample)
    {
      return saveToTiffFile<uint32_t>(
        *this,
        width,
        height,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        static_cast<uint16_t>(sample_format),
        [&](const XYIdx& idx) { return static_cast<uint32_t>(value_at(idx)); },
        nodata_as_int
      );
    }
  }
  else
  {
    // signed int
    if (8 == bits_per_sample)
    {
      return saveToTiffFile<int8_t>(
        *this,
        width,
        height,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        static_cast<uint16_t>(sample_format),
        [&](const XYIdx& idx) { return static_cast<int8_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (16 == bits_per_sample)
    {
      return saveToTiffFile<int16_t>(
        *this,
        width,
        height,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        static_cast<uint16_t>(sample_format),
        [&](const XYIdx& idx) { return static_cast<int16_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (32 == bits_per_sample)
    {
      return saveToTiffFile<int32_t>(
        *this,
        width,
        height,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        static_cast<uint16_t>(sample_format),
        [&](const XYIdx& idx) { return static_cast<int32_t>(value_at(idx)); },
        nodata_as_int
      );
    }
  }
  exit(logging::fatal(
    "Invalid combination of BITSPERSAMPLE ({:d}) and SAMPLEFORMAT ({:d})",
    bits_per_sample,
    sample_format
  ));
}
string GridBase::saveToTiffFileFloat(
  const Idx width,
  const Idx height,
  const tuple<Idx, Idx, Idx, Idx> bounds,
  const string_view dir,
  const string_view base_name,
  // const uint16_t bits_per_sample,
  // const uint16_t sample_format,
  std::function<TiffDouble(const XYIdx&)> value_at,
  const int nodata_as_int
) const
{
  // HACK: use whatever hardware does for TiffFloat for now
  constexpr auto bits_per_sample = 8 * sizeof(TiffFloat);
  constexpr auto sample_format = SAMPLEFORMAT_IEEEFP;
  return saveToTiffFile<TiffFloat>(
    *this,
    width,
    height,
    bounds,
    dir,
    base_name,
    bits_per_sample,
    sample_format,
    [&](const XYIdx& idx) { return static_cast<TiffFloat>(value_at(idx)); },
    nodata_as_int
  );
}
GridBase::GridBase(
  const MathSize cell_size,
  const MathSize xllcorner,
  const MathSize yllcorner,
  const MathSize xurcorner,
  const MathSize yurcorner,
  const string_view proj4
) noexcept
  : proj4_(try_fix_meridian(proj4)), cell_size_(cell_size), xllcorner_(xllcorner),
    yllcorner_(yllcorner), xurcorner_(xurcorner), yurcorner_(yurcorner)
{ }
void GridBase::createPrj(const string_view dir, const string_view base_name) const
{
  const auto filename = string(dir) + string(base_name) + ".prj";
  ofstream out{filename};
  logging::check_fatal(!out.is_open(), "Unable to write to {:s}", filename);
  logging::extensive("{:s}", proj4_);
  // HACK: use what we know is true for the grids that were generated and
  // hope it's correct otherwise
  const auto proj = find_value("+proj=", proj4_);
  // HACK: expect zone to already be converted to meridian in proj4 definition
  const auto lon_0 = find_value("+lon_0=", proj4_);
  const auto lat_0 = find_value("+lat_0=", proj4_);
  const auto k = find_value("+k=", proj4_);
  const auto x_0 = find_value("+x_0=", proj4_);
  const auto y_0 = find_value("+y_0=", proj4_);
  if (proj.empty() || k.empty() || lat_0.empty() || lon_0.empty() || x_0.empty() || y_0.empty())
  {
    exit(logging::fatal("Cannot convert proj4 '{:s}' into .prj file", proj4_.c_str()));
  }
  out << "Projection    TRANSVERSE\n";
  out << "Datum         AI_CSRS\n";
  out << "Spheroid      GRS80\n";
  // NOTE: uses American spelling
  out << "Units         METERS\n";
  out << "Zunits        NO\n";
  out << "Xshift        0.0\n";
  out << "Yshift        0.0\n";
  out << "Parameters    \n";
  // NOTE: it seems like comments in .prj files is no longer supported?
  // these are all just strings since they're out of the proj4 string
  //   - but convert to number so precision matches old output
  // HACK: this is the order they were previously, so not just outputting in order they occur
  out << std::format("{:g} /* scale factor at central meridian\n", stod(k));
  out << std::format("{:g}  0  0.0 /* longitude of central meridian\n", stod(lon_0));
  out << std::format("{:4g}  0  0.0 /* latitude of origin\n", stod(lat_0));
  // NOTE: uses American spelling
  out << std::format("{:6.1f} /* false easting (meters)\n", stod(x_0));
  out << std::format("{:0.1f} /* false northing (meters)\n", stod(y_0));
  out.close();
}
std::optional<Coordinates> GridBase::findCoordinates(const Point& point, const bool flipped) const
{
  auto full = findFullCoordinates(point, flipped);
  if (!full.has_value())
  {
    return {};
  }
  return Coordinates{
    static_cast<Idx>(full->x), static_cast<Idx>(full->y), full->x_sub, full->y_sub
  };
}
bool GridBase::validate(const Point& point) const
{
  // TODO: refactor this to avoid duplicated code
  bool invalid{false};
  const auto xy = from_lat_long(this->proj4_, point);
  logging::debug("Coordinates {} converted to ({:f}, {:f})", point, xy.x.value, xy.y.value);
  // find positions and lat/long translations for checking bounds
  const XPos x_left{xllcorner_};
  const XPos x_mid{(xllcorner_ + xurcorner_) / 2.0};
  const XPos x_right{xurcorner_};
  const YPos y_mid{(yllcorner_ + yurcorner_) / 2.0};
  const YPos y_bottom{yllcorner_};
  const YPos y_top{yurcorner_};
  const XYPos xy_center{x_mid, y_mid};
  const XYPos xy_s{x_mid, y_bottom};
  const XYPos xy_sw{x_left, y_bottom};
  const XYPos xy_se{x_right, y_bottom};
  // figure out corner points for checks
  const auto center = to_lat_long(this->proj4_, xy_center);
  const auto pt_s = to_lat_long(this->proj4_, xy_s);
  const auto pt_sw = to_lat_long(this->proj4_, xy_sw);
  const auto pt_se = to_lat_long(this->proj4_, xy_se);
  // how much it can be off in grid center
  constexpr MathSize MAX_DEVIATION{5};
  constexpr MathSize DEVIATION_FACTOR_BOTTOM{1};
  constexpr MathSize DEVIATION_FACTOR_ORIGIN{1.1};
  constexpr MathSize DEVIATION_FACTOR_EDGE{2};
  // different levels of deviation are acceptable at different points relative to the grid
  if (!check_deviation("center", proj4_, center, MAX_DEVIATION))
  {
    invalid = true;
  }
  constexpr auto MAX_DEVIATION_BOTTOM{DEVIATION_FACTOR_BOTTOM * MAX_DEVIATION};
  if (!check_deviation("lower center", proj4_, pt_s, MAX_DEVIATION_BOTTOM))
  {
    invalid = true;
  }
  constexpr auto MAX_DEVIATION_ORIGIN{DEVIATION_FACTOR_ORIGIN * MAX_DEVIATION};
  if (!check_deviation("origin", proj4_, point, MAX_DEVIATION_ORIGIN))
  {
    invalid = true;
  }
  constexpr auto MAX_DEVIATION_EDGES{DEVIATION_FACTOR_EDGE * MAX_DEVIATION};
  if (!check_deviation("lower left", proj4_, pt_sw, MAX_DEVIATION_EDGES))
  {
    invalid = true;
  }
  if (!check_deviation("lower right", proj4_, pt_se, MAX_DEVIATION_EDGES))
  {
    invalid = true;
  }
  constexpr auto M_PER_KM = 1000.0;
  const auto cell_km = cell_size_ / M_PER_KM;
  // use calculated longitude to figure out grid locations for "corners"
  const XYPos xy_w{x_left, y_mid};
  const XYPos xy_e{x_right, y_mid};
  const XYPos xy_n{x_mid, y_top};
  const auto pt_w = to_lat_long(this->proj4_, xy_w);
  const auto pt_e = to_lat_long(this->proj4_, xy_e);
  const auto pt_n = to_lat_long(this->proj4_, xy_n);
  const Point corner_nw{pt_n.latitude(), pt_w.longitude()};
  const Point corner_ne{pt_n.latitude(), pt_e.longitude()};
  const Point corner_sw{pt_s.latitude(), pt_w.longitude()};
  const Point corner_se{pt_s.latitude(), pt_e.longitude()};
  // convert corners to grid positions and see how much the distance between them varies
  const auto corner_xy_nw = from_lat_long(proj4_, corner_nw);
  const auto corner_xy_ne = from_lat_long(proj4_, corner_ne);
  const auto corner_xy_sw = from_lat_long(proj4_, corner_sw);
  const auto corner_xy_se = from_lat_long(proj4_, corner_se);
  // FIX: UTM grids are ~5% but they get clipped to MAX_HEIGHT and should compare clipped bounds
  constexpr MathSize MAX_DISTORTION_PERCENT{10};
  // do North-South distance along edges
  const auto dist_center{(y_top - y_bottom).value * cell_km};
  const auto dist_left{(corner_xy_nw.y - corner_xy_sw.y).value * cell_km};
  const auto dist_right{(corner_xy_ne.y - corner_xy_se.y).value * cell_km};
  const auto pct_left = (dist_left / dist_center) * 100;
  const auto pct_center = (dist_center / dist_center) * 100;
  assert(100.0 == pct_center);
  const auto pct_right = (dist_right / dist_center) * 100;
  const auto diff_left = pct_left - pct_center;
  const auto diff_right = pct_right - pct_center;
  logging::debug(
    "North-South distances are:\n\tleft:   {:>12.1f}km {:10.3f}%\n\tcenter: {:>12.1f}km {:10.3f}%\n\tright:  {:>12.1f}km {:10.3f}%",
    dist_left,
    pct_left,
    dist_center,
    pct_center,
    dist_right,
    pct_right
  );
  if (MAX_DISTORTION_PERCENT < abs(diff_left))
  {
    logging::error(
      "North-South distance West of grid ({:.1f}km) is {:+0.1f}% of the size of the distance across the midpoint ({:.1f}km), which is greater than the maximum of {:0.3f}%",
      dist_left,
      diff_left,
      dist_center,
      MAX_DISTORTION_PERCENT
    );
    invalid = true;
  }
  if (MAX_DISTORTION_PERCENT < abs(diff_right))
  {
    logging::error(
      "North-South distance East of grid ({:.1f}km) is {:+0.1f}% of the size of the distance across the midpoint ({:.1f}km), which is greater than the maximum of {:0.3f}%",
      dist_right,
      diff_right,
      dist_center,
      MAX_DISTORTION_PERCENT
    );
    invalid = true;
  }
  return !invalid;
}
std::optional<FullCoordinates> GridBase::findFullCoordinates(const Point& point, const bool flipped)
  const
{
  const auto xy = from_lat_long(this->proj4_, point);
  logging::debug("Coordinates {} converted to ({:f}, {:f})", point, xy.x.value, xy.y.value);
  // find positions and lat/long translations for checking bounds
  const XPos x_left{xllcorner_};
  const XPos x_mid{(xllcorner_ + xurcorner_) / 2.0};
  const XPos x_right{xurcorner_};
  const YPos y_mid{(yllcorner_ + yurcorner_) / 2.0};
  const YPos y_bottom{yllcorner_};
  const YPos y_top{yurcorner_};
  logging::verbose("Lower left is ({:f}, {:f})", this->xllcorner_, this->yllcorner_);
  // convert coordinates into cell position
  const auto actual_x = (xy.x.value - this->xllcorner_) / this->cell_size_;
  // these are already flipped across the y-axis on reading, so it's the same as for x now
  auto actual_y = (!flipped) ? (xy.y.value - this->yllcorner_) / this->cell_size_
                             : (yurcorner_ - xy.y.value) / cell_size_;
  const auto x1 = static_cast<FullIdx>(actual_x);
  const auto y1 = static_cast<FullIdx>(round(actual_y - 0.5));
  if (0 > x1 || x1 >= calculateWidth() || 0 > y1 || y1 >= calculateHeight())
  {
    logging::verbose(
      "Returning nullptr from findFullCoordinates() for ({:f}, {:f}) => ({:d}, {:d})",
      actual_x,
      actual_y,
      x1,
      y1
    );
    return {};
  }
  const auto sub_x = static_cast<SubSize>((actual_x - static_cast<MathSize>(x1)) * 1000);
  const auto sub_y = static_cast<SubSize>((actual_y - static_cast<MathSize>(y1)) * 1000);
  return FullCoordinates{static_cast<FullIdx>(x1), static_cast<FullIdx>(y1), sub_x, sub_y};
}
void write_ascii_header(
  ofstream& out,
  const MathSize width,
  const MathSize height,
  const MathSize xll,
  const MathSize yll,
  const MathSize cell_size,
  const MathSize no_data
)
{
  out << "ncols         " << width << "\n";
  out << "nrows         " << height << "\n";
  out << "xllcorner     " << fixed << setprecision(6) << xll << "\n";
  out << "yllcorner     " << fixed << setprecision(6) << yll << "\n";
  out << "cellsize      " << cell_size << "\n";
  out << "NODATA_value  " << no_data << "\n";
}
[[nodiscard]] GridBase read_header(GeoTiff& geotiff)
{
  auto tif = geotiff.tiff();
  auto gtif = geotiff.gtif();
  GTIFDefn definition;
  if (GTIFGetDefn(geotiff.gtif(), &definition))
  {
    uint32_t width;
    uint32_t height;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TiffDouble x = 0.0;
    TiffDouble y = height;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    const auto yllcorner = y;
    const auto xllcorner = x;
    logging::debug("Lower left for header is ({:f}, {:f})", xllcorner, yllcorner);
    TiffDouble adf_coefficient[6] = {0};
    x = 0.5;
    y = 0.5;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    adf_coefficient[4] = x;
    adf_coefficient[5] = y;
    x = 1.5;
    y = 0.5;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    const auto cell_width = x - adf_coefficient[4];
    x = 0.5;
    y = 1.5;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    const auto cell_height = y - adf_coefficient[5];
    logging::check_fatal(cell_width != -cell_height, "Can only use grids with square pixels");
    logging::debug("Cell size is {:f}", cell_width);
    // HACK: GTIFGetProj4Defn uses malloc
    std::unique_ptr<char, decltype(std::free)*> proj4_char{
      GTIFGetProj4Defn(&definition), std::free
    };
    auto proj4 = string(proj4_char.get());
    const auto xurcorner = xllcorner + cell_width * width;
    const auto yurcorner = yllcorner + cell_width * height;
    return {cell_width, xllcorner, yllcorner, xurcorner, yurcorner, proj4};
  }
  throw runtime_error("Cannot read TIFF header");
}
[[nodiscard]] GridBase read_header(const string_view filename)
{
  GeoTiff geotiff{filename, "r"};
  return read_header(geotiff);
}
string create_file_name(
  const string_view dir,
  const string_view base_name,
  const string_view extension
)
{
  return string(dir) + string(base_name) + "." + string(extension);
}
}
