import os
import re
import numpy as np
import shutil
import logging
import rasterio
from rasterio.enums import Resampling
import rasterio.features
from tqdm import tqdm
import pandas as pd
import geopandas as gpd
import shapely
import shapely.geometry
import sys
sys.path.append("../util")
import server

DIR_ROOT = r"/appl/data/output"
DIR_TMP_ROOT = os.path.join(DIR_ROOT, "service")
FILE_LOG = os.path.join(DIR_TMP_ROOT, "log.txt")
CREATION_OPTIONS = ["COMPRESS=LZW", "TILED=YES"]

logging.basicConfig(
    filename=FILE_LOG,
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)

DIR_OUT = r"/appl/publish"
PREFIX_DAY = "firestarr_day_"
FORMAT_DAY = PREFIX_DAY + "{:02d}"
REGEX_TIF = re.compile("^{}[0-9]*.tif$".format(PREFIX_DAY))
FACTORS = [2, 4, 8, 16]


def symbolize(file_in, file_out):
    # FIX: figure out if symbolizing right in the map service makes sense or not
    # # write to .ovr instead of into raster
    # with rasterio.Env(TIFF_USE_OVR=True):
    #     # HACK: trying to get .ovr to compress
    with rasterio.Env(
        # # FIX: couldn't get it to compress .ovr, so just write to .tif
        # TIFF_USE_OVR=True,
        GDAL_PAM_ENABLED=True,
        ESRI_XML_PAM=True
        ):
        file_out_int = file_out.replace('.tif', '_int.tif')
        with rasterio.open(file_in, 'r') as src:
            profile = src.profile
            profile["profile"] = "GeoTIFF"
            profile_int = {k: v for k, v in profile.items()}
            profile_int['dtype'] = 'uint8'
            profile_int['nodata'] = 0
            # HACK: get length of generator so we can show progress
            n = 0
            for ji, window_ in src.block_windows(1):
                n += 1
            assert len(set(src.block_shapes)) == 1
            with rasterio.open(file_out, 'w', **profile) as dst:
                with rasterio.open(file_out_int, 'w', **profile_int) as dst_int:
                    for ji, window in tqdm(src.block_windows(1), total=n, desc=f"Processing {os.path.basename(file_in)}"):
                        # NOTE: should only be 1 band, but use all of them if more
                        d = src.read(window=window)
                        # we can read source once and use data twice
                        dst.write(d, window=window)
                        dst_int.write((10 * d).astype(int), window=window)
                logging.info("Building overviews")
            #     # NOTE: definitely do not want to blend everything out by using average
                dst.build_overviews(FACTORS, Resampling.nearest)
                dst.update_tags(ns='rio_overview', resampling='nearest')
    with rasterio.open(file_out_int, 'r') as src_int:
        crs = src_int.crs
        df = pd.DataFrame(rasterio.features.dataset_features(src_int, 1))
        df['geometry'] = df['geometry'].apply(shapely.geometry.shape)
        gdf = gpd.GeoDataFrame(df, geometry=df['geometry'], crs=crs)
        file_prob_shp = file_out.replace(".tif", ".shp").replace("-", "_")
        gdf['GRIDCODE'] = gdf['properties'].apply(lambda x: int(x['val']))
        gdf[['GRIDCODE', 'geometry']].to_file(file_prob_shp)
    os.remove(file_out_int)


def publish_folder(dir_runid):
    run_id = os.path.basename(dir_runid)
    dir_base = os.path.join(dir_runid, "combined")
    # find last date in directory
    # redundant to use loop now that output structure is different, but still works
    dir_date = [x for x in os.listdir(dir_base) if os.path.isdir(os.path.join(dir_base, x))][-1]
    dir_in = os.path.join(dir_base, dir_date, "rasters")
    logging.info("Using files in %s", dir_in)
    files_tif = [f for f in os.listdir(dir_in) if REGEX_TIF.match(f)]
    dir_tmp = os.path.join(DIR_TMP_ROOT, dir_date, run_id)
    #############################
    # dir_tmp += '_TEST'
    #############################
    logging.info("Staging in temporary directory %s", dir_tmp)
    if not os.path.exists(dir_tmp):
        os.makedirs(dir_tmp)
    for file in tqdm(files_tif, desc="Symbolizing files"):
        logging.info(f"Processing file")
        file_out = os.path.join(dir_tmp, file)
        file_in = os.path.join(dir_in, file)
        # shutil.copy(file_in, file_prob_tif)
        symbolize(file_in, file_out)
    files_tif_service = [f for f in os.listdir(DIR_OUT) if REGEX_TIF.match(f)]
    if ((len(files_tif_service) < len(files_tif))
            or (files_tif[:len(files_tif_service)] != files_tif_service)):
        logging.fatal(f"Files to be published do not match files that service is using\n{files_tif} != {files_tif_service}")
        raise RuntimeError("Files to be published do not match files that service is using")
    if len(files_tif_service) != len(files_tif):
        logging.warning("Copying files to publish directory, but service will need to be republished with new length %d",
                        len(files_tif_service))
    # HACK: copying seems to take a while, so try to do this without stopping before copy
    # logging.info("Stopping services")
    # server.stopServices()
    # HACK: using CopyRaster and CopyFeature fail, but this seems okay
    for file in tqdm(os.listdir(dir_tmp), desc=f"Copying to output directory {DIR_OUT}"):
        shutil.copy(os.path.join(dir_tmp, file), os.path.join(DIR_OUT, file))
    logging.info("Restarting services")
    server.restartServices()
    logging.info("Done")


def publish_latest(dir_input="current_m3"):
    logging.info(f"Publishing latest files for {dir_input}")
    # dir_input = "current_home_bfdata_affes_latest"
    dir_main = os.path.join(DIR_ROOT, dir_input)
    run_id = os.listdir(dir_main)[-1]
    ##########################
    # run_id = '202306131555'
    #######################
    dir_runid = os.path.join(dir_main, run_id)
    publish_folder(dir_runid)


if "__main__" == __name__:
    publish_latest()