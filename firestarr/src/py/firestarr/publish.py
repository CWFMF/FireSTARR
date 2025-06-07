import datetime
import os
import shutil
import time

import numpy as np
from common import (
    CREATION_OPTIONS,
    DIR_OUTPUT,
    DIR_TMP,
    DIR_ZIP,
    FILE_LOCK_PUBLISH,
    FLAG_IGNORE_PERIM_OUTPUTS,
    FMT_DATE_YMD,
    FORMAT_OUTPUT,
    PUBLISH_AZURE_WAIT_TIME_SECONDS,
    ensure_dir,
    force_remove,
    is_newer_than,
    list_dirs,
    listdir_sorted,
    locks_for,
    logging,
    zip_folder,
)
from gdal_merge_max import gdal_merge_max
from gis import CRS_COMPARISON, find_invalid_tiffs, project_raster
from osgeo import gdal
from redundancy import call_safe, get_stack
from tqdm_util import keep_trying, tqdm


# distinguish erros with publishing from other problems
class PublishError(RuntimeError):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)


def publish_all(
    dir_output=None,
    changed_only=False,
    force=False,
    force_project=False,
    force_publish=False,
    merge_only=False,
):
    dir_output = find_latest_outputs(dir_output)
    # check_copy_interim(dir_output, include_interim)
    with locks_for(FILE_LOCK_PUBLISH):
        changed = merge_dirs(
            dir_output,
            changed_only=changed_only,
            force=force,
            force_project=force_project,
            create_zip=not merge_only,
        )
        if merge_only:
            logging.info("Stopping after merge for %s", dir_output)
            return
        if changed or force or force_publish:
            import publish_azure

            publish_azure.upload_dir(dir_output)
            logging.info("Uploaded to azure")
            logging.info("Publishing to geoserver from %s", dir_output)
            # HACK: might be my imagination, but maybe there's a delay so wait a bit
            time.sleep(PUBLISH_AZURE_WAIT_TIME_SECONDS)
            import publish_geoserver

            publish_geoserver.publish_folder(dir_output)
        else:
            logging.info("No changes for %s so not publishing", os.path.basename(dir_output))


def find_latest_outputs(dir_output=None):
    if dir_output is None:
        dir_default = DIR_OUTPUT
        dirs_with_initial = [
            x for x in list_dirs(dir_default) if os.path.isdir(os.path.join(dir_default, x, "initial"))
        ]
        if dirs_with_initial:
            dir_output = os.path.join(dir_default, dirs_with_initial[-1])
            logging.info("Defaulting to directory %s", dir_output)
            return dir_output
        else:
            raise PublishError(f'find_latest_outputs("{dir_output}") failed: No run found')
    return dir_output


def merge_dirs(
    dir_input=None,
    changed_only=False,
    force=False,
    force_project=False,
    creation_options=CREATION_OPTIONS,
    create_zip=True,
):
    any_change = False
    dir_input = find_latest_outputs(dir_input)
    # expecting dir_input to be a path ending in a runid of form '%Y%m%d%H%M'
    dir_base = os.path.join(dir_input, "initial")
    if not os.path.isdir(dir_base):
        raise PublishError(f"Directory {dir_base} missing")
    run_name = os.path.basename(dir_input)
    run_id = run_name[run_name.index("_") + 1 :]
    logging.info("Merging {}".format(dir_base))
    dir_parent = os.path.dirname(dir_base)
    # want to put probability and perims together
    files_by_for_what = {}
    for for_what in list_dirs(dir_base):
        dir_for_what = os.path.join(dir_base, for_what)
        files_by_for_what[for_what] = files_by_for_what.get(for_what, []) + [
            os.path.join(dir_for_what, x) for x in listdir_sorted(dir_for_what) if x.endswith(".tif")
        ]
    if not FLAG_IGNORE_PERIM_OUTPUTS:
        raise NotImplementedError("Need to deal with perimeters properly")
    else:
        if "perim" in files_by_for_what:
            del files_by_for_what["perim"]
    dirs_what = [os.path.basename(for_what) for for_what in files_by_for_what.keys()]
    for_dates = [datetime.datetime.strptime(_, FMT_DATE_YMD) for _ in dirs_what if "perim" != _]
    if not for_dates:
        raise PublishError("No dates to merge")
    date_origin = min(for_dates)
    reprojected = {}
    for for_what, files in tqdm(files_by_for_what.items(), desc=f"Merging {dir_parent}"):
        files = files_by_for_what[for_what]
        dir_in_for_what = os.path.basename(for_what)
        dir_crs = ensure_dir(os.path.join(dir_parent, "reprojected", dir_in_for_what))
        dir_tmp = ensure_dir(os.path.join(DIR_TMP, f"{run_name}/reprojected/{dir_in_for_what}"))

        def reproject(f):
            changed = False
            f_crs = os.path.join(dir_crs, os.path.basename(f))
            # don't project if file isn't newer, but keep track of file for merge
            if force_project or is_newer_than(f, f_crs):
                # FIX: this is super slow for perim tifs
                #       (because they're the full exz\\V tent of the UTM zone?)
                # do this to temp directory and then copy so it's faster (?)
                f_tmp = os.path.join(dir_tmp, os.path.basename(f))
                force_remove(f_tmp)
                b = project_raster(
                    f,
                    f_tmp,
                    resolution=100,
                    nodata=0,
                    crs=f"EPSG:{CRS_COMPARISON}",
                )
                if b is None:
                    return b
                force_remove(f_crs)
                call_safe(shutil.move, f_tmp, f_crs)
                changed = True
            return changed, f_crs

        results_crs = keep_trying(
            reproject,
            files,
            total=len(files),
            desc=f"Reprojecting for {dir_in_for_what}",
        )
        reprojected[dir_in_for_what] = [x for x in results_crs if x is not None]
    dir_combined = ensure_dir(f"{dir_parent}/combined")
    for dir_in_for_what, results_crs_all in tqdm(reprojected.items(), desc=f"Merging {dir_parent}"):
        # HACK: forget about tiling and just do what we need now
        if "perim" == dir_in_for_what:
            dir_for_what = "perim"
            date_cur = for_dates[0]
            description = "perimeter"
        else:
            date_cur = datetime.datetime.strptime(dir_in_for_what, FMT_DATE_YMD)
            offset = (date_cur - date_origin).days + 1
            dir_for_what = f"day_{offset:02d}"
            description = "probability"

        def merge_files(results_crs, dir_merge, verbose=False):
            files_crs = [x[1] for x in results_crs]
            files_crs_changed = [x[1] for x in results_crs if x[0]]
            changed = 0 < len(files_crs_changed)
            file_root = os.path.join(f"firestarr_{run_id}_{dir_for_what}_{date_cur.strftime('%Y%m%d')}")
            dir_tmp = ensure_dir(os.path.join(DIR_TMP, dir_merge.replace(dir_parent, "").strip("/")))

            file_tmp = os.path.join(dir_tmp, f"{file_root}_tmp.tif")
            file_base = os.path.join(ensure_dir(dir_merge), f"{file_root}.tif")
            # no point in doing this if nothing was added
            if force or changed or not os.path.isfile(file_base):
                force_remove(file_tmp)
                invalid_files = None
                # # NOTE: this doesn't work if previously had temp file and now it's final
                # if any([x for x in files_crs_changed if TMP_SUFFIX in x]):
                #     # if anything is a temporary output we can't only merge changed files
                #     changed_only = False
                # HACK: seems like currently making empty combined raster so delete
                #       first in case it's merging into existing and causing problems
                if changed_only and os.path.isfile(file_base):
                    # this seems like it would be fine but if interim fire outputs update then
                    # cell probability generally goes down, so merging with old final raster
                    # won't update those cells
                    files_merge = files_crs_changed + [file_base]
                else:
                    files_merge = files_crs
                if 0 == len(files_merge):
                    logging.error("No files to merge")
                    file_tmp = None
                else:
                    # HACK: don't get locks because it takes forever
                    # with locks_for(files_merge):
                    if 1 == len(files_merge):
                        f = files_merge[0]
                        if f == file_tmp:
                            logging.warning("Ignoring trying to merge file into iteslf: %s", f)
                        else:
                            logging.debug(
                                "Only have one file so just copying %s to %s",
                                f,
                                file_tmp,
                            )
                            shutil.copy(f, file_tmp)
                    else:
                        invalid_files = gdal_merge_max(
                            file_out=file_tmp,
                            names=files_merge,
                            creation_options=creation_options,
                            a_nodata=-1,
                            description=description,
                        )
                    mtime = np.max([os.path.getmtime(f) for f in files_merge])
                    # set modified time for merged raster to last modified input
                    os.utime(file_tmp, (mtime, mtime))

                    if invalid_files:
                        logging.error("Removing invalid files %s", invalid_files)
                        force_remove(invalid_files)

                    if not find_invalid_tiffs(file_tmp):
                        force_remove(file_base)
                        if "GTiff" == FORMAT_OUTPUT:
                            call_safe(shutil.move, file_tmp, file_base)
                        else:
                            # can't progressively update COG so need to copy final GTiff in full
                            logging.info("Converting file to %s: %s", FORMAT_OUTPUT, file_base)
                            gdal.Translate(
                                file_base,
                                file_tmp,
                                format=FORMAT_OUTPUT,
                                resampleAlg="near",
                                creationOptions=creation_options,
                            )
                            force_remove(file_tmp)
                        os.utime(file_base, (mtime, mtime))
                        changed = True
            else:
                if verbose:
                    logging.info("Output already exists for %s", file_base)
            return changed, file_base

        # doing this by zones is way slower even if this fails
        changed, file_base = merge_files(
            results_crs=results_crs_all,
            dir_merge=dir_combined,
            verbose=True,
        )

        # if not force:
        #     by_zone = {}
        #     for r in results_crs_all:
        #         f = r[1]
        #         file_base = os.path.basename(f)
        #         zone = file_base[: file_base.index("_")]
        #         by_zone[zone] = by_zone.get(zone, []) + [r]

        #     def merge_zone(for_what):
        #         zone, results_crs_zone = for_what
        #         dir_merge = f"{dir_parent}/zones/{zone}"
        #         changed, file_base = merge_files(results_crs_zone, dir_merge)
        #         return (changed, file_base)

        #     # # do this just to get something right now
        #     # zone_rasters_results = apply(by_zone.items(), merge_zone, desc="Merging zones")
        #     # # zone_rasters_results = keep_trying(merge_zone, by_zone.items(), desc="Merging zones")
        #     zone_rasters = {}
        #     for zone, results_crs_zone in tqdm(by_zone.items(), total=len(by_zone), desc="Merging zones"):
        #         dir_merge = f"{dir_parent}/zones/{zone}"
        #         changed, file_base = merge_files(results_crs_zone, dir_merge)
        #         zone_rasters[zone] = (changed, file_base)
        #         any_change = any_change or changed
        #     changed, file_base = merge_files(zone_rasters.values(), dir_combined, verbose=True)
        # else:
        #     all_files = [x[1] for x in results_crs_all]
        #     changed, file_base = merge_files(all_files, dir_combined, verbose=True)

        any_change = any_change or changed
    logging.info("Final results of merge are in %s", dir_combined)
    if create_zip:
        try:
            run_id = os.path.basename(dir_input)
            file_zip = os.path.join(DIR_ZIP, f"{run_name}.zip")
            if any_change or not os.path.isfile(file_zip):
                logging.info("Creating archive %s", file_zip)
                zip_folder(file_zip, dir_combined)
        except KeyboardInterrupt as ex:
            raise ex
        except Exception as ex:
            logging.error("Ignoring zip error")
            logging.error(get_stack(ex))

    return any_change
