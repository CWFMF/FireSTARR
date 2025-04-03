# #!/usr/bin/env python3
# # standard gdal_merge code, but change to use max value
# ###############################################################################
# # $Id$
# #
# # Project:  InSAR Peppers
# # Purpose:  Module to extract data from many rasters into one output.
# # Author:   Frank Warmerdam, warmerdam@pobox.com
# #
# ###############################################################################
# # Copyright (c) 2000, Atlantis Scientific Inc. (www.atlsci.com)
# # Copyright (c) 2009-2011, Even Rouault <even dot rouault at spatialys.com>
# #
# # This library is free software; you can redistribute it and/or
# # modify it under the terms of the GNU Library General Public
# # License as published by the Free Software Foundation; either
# # version 2 of the License, or (at your option) any later version.
# #
# # This library is distributed in the hope that it will be useful,
# # but WITHOUT ANY WARRANTY; without even the implied warranty of
# # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# # Library General Public License for more details.
# #
# # You should have received a copy of the GNU Library General Public
# # License along with this library; if not, write to the
# # Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# # Boston, MA 02111-1307, USA.
# ###############################################################################
# # changes 29Apr2011
# # If the input image is a multi-band one, use all the channels in
# # building the stack.
# # anssi.pekkarinen@fao.org

import math
import os

import numpy as np
from gis import with_gdal_exceptions_off
from osgeo import gdal
from osgeo_utils.auxiliary.util import GetOutputDriverFor
from redundancy import call_safe

# import logging
from tqdm import tqdm
from tqdm_util import keep_trying, pmap

__version__ = "$id$"[5:-1]


# import osgeo_utils
# import osgeo_utils.gdal_merge as gm


# # =============================================================================


# # =============================================================================


# # =============================================================================


# # =============================================================================


def names_to_fileinfos(names):
    """
    Translate a list of GDAL filenames, into file_info objects.

    names -- list of valid GDAL dataset names.

    Returns a list of file_info objects.  There may be less file_info objects
    than names if some of the names could not be opened as GDAL files.
    """

    files_invalid = []

    def init(name):
        try:
            nonlocal files_invalid
            fi = file_info_max()
            if fi.init_from_name(name) == 1:
                return fi
        except KeyboardInterrupt as ex:
            raise ex
        except Exception:
            pass
        files_invalid.append(name)
        return None

    file_infos = keep_trying(init, names, desc="Getting merge input file info")
    # HACK: looks like it just ignored things it couldn't load?
    file_infos = [x for x in file_infos if x is not None]

    return file_infos, files_invalid


# # *****************************************************************************


class file_info_max(object):
    """A class holding information about a GDAL file."""

    def __init__(self):
        self.band_type = None
        self.bands = None
        self.ct = None
        self.filename = None
        self.geotransform = None
        self.lrx = None
        self.lry = None
        self.projection = None
        self.ulx = None
        self.uly = None
        self.xsize = None
        self.ysize = None

    def init_from_name(self, filename):
        """
        Initialize file_info from filename

        filename -- Name of file to read.

        Returns 1 on success or 0 if the file can't be opened.
        """
        fh = gdal.Open(filename)
        if fh is None:
            return 0

        self.filename = filename
        self.bands = fh.RasterCount
        self.xsize = fh.RasterXSize
        self.ysize = fh.RasterYSize
        self.band_type = fh.GetRasterBand(1).DataType
        self.projection = fh.GetProjection()
        self.geotransform = fh.GetGeoTransform()
        self.ulx = self.geotransform[0]
        self.uly = self.geotransform[3]
        self.lrx = self.ulx + self.geotransform[1] * self.xsize
        self.lry = self.uly + self.geotransform[5] * self.ysize

        ct = fh.GetRasterBand(1).GetRasterColorTable()
        if ct is not None:
            self.ct = ct.Clone()
        else:
            self.ct = None

        return 1

    def copy_into(self, t_fh, s_band=1, t_band=1, nodata_arg=None):
        """
        Copy this files image into target file.

        This method will compute the overlap area of the file_info objects
        file, and the target gdal.Dataset object, and copy the image data
        for the common window area.  It is assumed that the files are in
        a compatible projection ... no checking or warping is done.  However,
        if the destination file is a different resolution, or different
        image pixel type, the appropriate resampling and conversions will
        be done (using normal GDAL promotion/demotion rules).

        t_fh -- gdal.Dataset object for the file into which some or all
        of this file may be copied.

        Returns 1 on success (or if nothing needs to be copied), and zero one
        failure.
        """
        t_geotransform = t_fh.GetGeoTransform()
        t_ulx = t_geotransform[0]
        t_uly = t_geotransform[3]
        t_lrx = t_geotransform[0] + t_fh.RasterXSize * t_geotransform[1]
        t_lry = t_geotransform[3] + t_fh.RasterYSize * t_geotransform[5]

        # figure out intersection region
        tgw_ulx = max(t_ulx, self.ulx)
        tgw_lrx = min(t_lrx, self.lrx)
        if t_geotransform[5] < 0:
            tgw_uly = min(t_uly, self.uly)
            tgw_lry = max(t_lry, self.lry)
        else:
            tgw_uly = max(t_uly, self.uly)
            tgw_lry = min(t_lry, self.lry)

        # do they even intersect?
        if tgw_ulx >= tgw_lrx:
            return 1
        if t_geotransform[5] < 0 and tgw_uly <= tgw_lry:
            return 1
        if t_geotransform[5] > 0 and tgw_uly >= tgw_lry:
            return 1

        # compute target window in pixel coordinates.
        tw_xoff = int((tgw_ulx - t_geotransform[0]) / t_geotransform[1] + 0.1)
        tw_yoff = int((tgw_uly - t_geotransform[3]) / t_geotransform[5] + 0.1)
        tw_xsize = int((tgw_lrx - t_geotransform[0]) / t_geotransform[1] + 0.5) - tw_xoff
        tw_ysize = int((tgw_lry - t_geotransform[3]) / t_geotransform[5] + 0.5) - tw_yoff

        if tw_xsize < 1 or tw_ysize < 1:
            return 1

        # Compute source window in pixel coordinates.
        sw_xoff = int((tgw_ulx - self.geotransform[0]) / self.geotransform[1] + 0.1)
        sw_yoff = int((tgw_uly - self.geotransform[3]) / self.geotransform[5] + 0.1)
        sw_xsize = int((tgw_lrx - self.geotransform[0]) / self.geotransform[1] + 0.5) - sw_xoff
        sw_ysize = int((tgw_lry - self.geotransform[3]) / self.geotransform[5] + 0.5) - sw_yoff

        if sw_xsize < 1 or sw_ysize < 1:
            return 1

        # Open the source file, and copy the selected region.
        if not os.path.isfile(self.filename):
            raise RuntimeError(f"No such file {self.filename}")
        s_fh = gdal.Open(self.filename)
        if s_fh is None:
            raise RuntimeError(f"Couldn't open file {self.filename}")

        return raster_copy_max(
            s_fh,
            sw_xoff,
            sw_yoff,
            sw_xsize,
            sw_ysize,
            s_band,
            t_fh,
            tw_xoff,
            tw_yoff,
            tw_xsize,
            tw_ysize,
            t_band,
            nodata_arg,
        )


# =============================================================================


def gdal_merge_max(
    file_out,
    names,
    creation_options,
    nodata=None,
    a_nodata=None,
    description=None,
    copy_pct=False,
    bTargetAlignedPixels=False,
    driver_name=None,
):
    if driver_name is None:
        driver_name = GetOutputDriverFor(file_out)

    driver = gdal.GetDriverByName(driver_name)
    if driver is None:
        raise RuntimeError("Format driver %s not found, pick a supported driver." % driver_name)

    DriverMD = driver.GetMetadata()
    if "DCAP_CREATE" not in DriverMD:
        raise RuntimeError(
            "Format driver %s does not support creation and piecewise writing.\n"
            "Please select a format that does, such as GTiff (the default)"
            "or HFA (Erdas Imagine)." % driver_name
        )

    # Collect information on all the source files.
    file_infos, files_invalid = names_to_fileinfos(names)

    # HACK: do this so we know it's always going to turn exceptions back on
    def do_merge():
        # do after getting file info so we don't redo that on error
        def create_merged():
            ulx = file_infos[0].ulx
            uly = file_infos[0].uly
            lrx = file_infos[0].lrx
            lry = file_infos[0].lry

            for fi in file_infos:
                ulx = min(ulx, fi.ulx)
                uly = max(uly, fi.uly)
                lrx = max(lrx, fi.lrx)
                lry = min(lry, fi.lry)

            psize_x = file_infos[0].geotransform[1]
            psize_y = file_infos[0].geotransform[5]

            band_type = file_infos[0].band_type

            t_fh = None
            try:
                if os.path.isfile(file_out):
                    t_fh = gdal.Open(file_out, gdal.GA_Update)
            except KeyboardInterrupt as ex:
                raise ex
            except Exception:
                pass

            # Create output file if it does not already exist.
            if t_fh is None:
                # logging.info("Creating new file %s", file_out)

                if bTargetAlignedPixels:
                    ulx = math.floor(ulx / psize_x) * psize_x
                    lrx = math.ceil(lrx / psize_x) * psize_x
                    lry = math.floor(lry / -psize_y) * -psize_y
                    uly = math.ceil(uly / -psize_y) * -psize_y

                geotransform = [ulx, psize_x, 0, uly, 0, psize_y]

                xsize = int((lrx - ulx) / geotransform[1] + 0.5)
                ysize = int((lry - uly) / geotransform[5] + 0.5)

                bands = file_infos[0].bands

                t_fh = driver.Create(file_out, xsize, ysize, bands, band_type, creation_options)
                if t_fh is None:
                    raise RuntimeError("Creation failed, terminating gdal_merge.")

                t_fh.SetGeoTransform(geotransform)
                t_fh.SetProjection(file_infos[0].projection)

                if copy_pct:
                    t_fh.GetRasterBand(1).SetRasterColorTable(file_infos[0].ct)
            else:
                bands = min(file_infos[0].bands, t_fh.RasterCount)

            # Do we need to set nodata value ?
            if a_nodata is not None:
                for i in range(t_fh.RasterCount):
                    t_fh.GetRasterBand(i + 1).SetNoDataValue(a_nodata)

            # Copy data from source files into output file.
            for fi in tqdm(file_infos, desc=f"Merging into {file_out}"):
                for band in range(1, bands + 1):
                    call_safe(fi.copy_into, t_fh, band, band, nodata)

            def set_description():
                t_fh.SetDescription(description)
                for i in range(t_fh.RasterCount):
                    t_fh.GetRasterBand(i + 1).SetDescription(description)

            if description:
                call_safe(set_description)

            # unfortunately probably need to redo everything if it fails here
            # Force file to be closed.
            t_fh = None

            return files_invalid

        return call_safe(create_merged)

    return with_gdal_exceptions_off(do_merge)


def raster_copy_max_with_nodata(
    s_fh,
    s_xoff,
    s_yoff,
    s_xsize,
    s_ysize,
    s_band_n,
    t_fh,
    t_xoff,
    t_yoff,
    t_xsize,
    t_ysize,
    t_band_n,
    nodata,
):
    s_band = s_fh.GetRasterBand(s_band_n)
    t_band = t_fh.GetRasterBand(t_band_n)

    data_src = s_band.ReadAsArray(s_xoff, s_yoff, s_xsize, s_ysize, t_xsize, t_ysize)
    data_dst = t_band.ReadAsArray(t_xoff, t_yoff, t_xsize, t_ysize)

    # HACK: write maximum value
    if not np.isnan(nodata):
        nodata_test = np.not_equal(data_src, nodata) + (2 * np.not_equal(data_dst, nodata))
    else:
        nodata_test = np.isfinite(data_src) + (2 * np.isfinite(data_dst))
    # 0: neither has data
    # 1: data_src has data
    # 2: data_dst has data
    # 3: both have data
    # np.maximum returns nans, so need to avoid that
    # 0: either
    # 1: data_src
    # 2: data_dst
    # 3: np.maximum(data_src, data_dst)
    # so just -1 and np.maximum(0, nodata_test) and then use:
    # (data_src, data_dst, np.maximum(data_src, data_dst))
    nodata_test = np.maximum(0, nodata_test - 1)
    to_write = np.choose(nodata_test, (data_src, data_dst, np.maximum(data_src, data_dst)))

    t_band.WriteArray(to_write, t_xoff, t_yoff)

    return 0


def raster_copy_max_with_mask(
    s_fh,
    s_xoff,
    s_yoff,
    s_xsize,
    s_ysize,
    s_band_n,
    t_fh,
    t_xoff,
    t_yoff,
    t_xsize,
    t_ysize,
    t_band_n,
    m_band,
):
    # logging.info("Running raster_copy_max_with_mask()")
    s_band = s_fh.GetRasterBand(s_band_n)
    t_band = t_fh.GetRasterBand(t_band_n)

    data_src = s_band.ReadAsArray(s_xoff, s_yoff, s_xsize, s_ysize, t_xsize, t_ysize)
    data_mask = m_band.ReadAsArray(s_xoff, s_yoff, s_xsize, s_ysize, t_xsize, t_ysize)
    data_dst = t_band.ReadAsArray(t_xoff, t_yoff, s_xsize, s_ysize, t_xsize, t_ysize)
    mask_test = np.equal(data_mask, 0)
    to_write = np.choose(mask_test, (np.fmax(data_src, data_dst), data_dst))
    t_band.WriteArray(to_write, t_xoff, t_yoff)

    return 0


def raster_copy_max(
    s_fh,
    s_xoff,
    s_yoff,
    s_xsize,
    s_ysize,
    s_band_n,
    t_fh,
    t_xoff,
    t_yoff,
    t_xsize,
    t_ysize,
    t_band_n,
    nodata=None,
):
    if nodata is not None:
        # logging.info("Calling raster_copy_with_nodata()")
        return raster_copy_max_with_nodata(
            s_fh,
            s_xoff,
            s_yoff,
            s_xsize,
            s_ysize,
            s_band_n,
            t_fh,
            t_xoff,
            t_yoff,
            t_xsize,
            t_ysize,
            t_band_n,
            nodata,
        )

    s_band = s_fh.GetRasterBand(s_band_n)
    m_band = None
    # Works only in binary mode and doesn't take into account
    # intermediate transparency values for compositing.
    if s_band.GetMaskFlags() != gdal.GMF_ALL_VALID:
        m_band = s_band.GetMaskBand()
    elif s_band.GetColorInterpretation() == gdal.GCI_AlphaBand:
        m_band = s_band
    if m_band is not None:
        # logging.info("Calling raster_copy_with_mask()")
        return raster_copy_max_with_mask(
            s_fh,
            s_xoff,
            s_yoff,
            s_xsize,
            s_ysize,
            s_band_n,
            t_fh,
            t_xoff,
            t_yoff,
            t_xsize,
            t_ysize,
            t_band_n,
            m_band,
        )
    # logging.info("Copying without mask or nodata")
    s_band = s_fh.GetRasterBand(s_band_n)
    t_band = t_fh.GetRasterBand(t_band_n)

    data = s_band.ReadRaster(s_xoff, s_yoff, s_xsize, s_ysize, t_xsize, t_ysize, t_band.DataType)
    data_cur = t_band.ReadRaster(t_xoff, t_yoff, t_xsize, t_ysize, data, t_xsize, t_ysize, t_band.DataType)
    data = np.maximum(data, data_cur)
    t_band.WriteRaster(t_xoff, t_yoff, t_xsize, t_ysize, data, t_xsize, t_ysize, t_band.DataType)
    return 0
