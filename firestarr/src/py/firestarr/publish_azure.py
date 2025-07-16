import datetime
import os
import urllib.parse

import numpy as np
import pandas as pd
from azure.storage.blob import BlobServiceClient, ExponentialRetry
from common import (
    CONFIG,
    DIR_OUTPUT,
    DIR_RUNS,
    DIR_ZIP,
    FLAG_IGNORE_PERIM_OUTPUTS,
    FMT_DATE_YMD,
    FMT_FILE_SECOND,
    is_empty,
    listdir_sorted,
    logging,
)
from sim_wrapper import TMP_SUFFIX

AZURE_URL = None
AZURE_TOKEN = None
AZURE_CONTAINER = None
AZURE_DIR_DATA = None


def get_token():
    # HACK: % in config file gets parsed as variable replacement, so unqoute for that
    token = CONFIG.get("AZURE_TOKEN", "")
    args = token.split("&")
    args_kv = {k: v for k, v in [(arg[: arg.index("=")], arg[(arg.index("=") + 1) :]) for arg in args]}
    args_kv["sig"] = urllib.parse.quote(args_kv["sig"])
    return "&".join(f"{k}={v}" for k, v in args_kv.items())


def read_config():
    global AZURE_URL
    global AZURE_TOKEN
    global AZURE_CONTAINER
    global AZURE_DIR_DATA
    try:
        AZURE_URL = CONFIG.get("AZURE_URL", "")
        AZURE_TOKEN = get_token()
        AZURE_CONTAINER = CONFIG.get("AZURE_CONTAINER", "")
        AZURE_DIR_DATA = CONFIG.get("AZURE_DIR_DATA", "")
    except ValueError as ex:
        logging.error(ex)
        logging.warning("Unable to read azure config")
    if not np.all([x is not None and 0 < len(x) for x in [AZURE_URL, AZURE_TOKEN, AZURE_CONTAINER, AZURE_DIR_DATA]]):
        return False
    # prefix after so AZURE_DIR_DATA being empty is detected if RESOURCE_PREFIX isn't
    AZURE_DIR_DATA = CONFIG.get("RESOURCE_PREFIX", "") + AZURE_DIR_DATA
    return True


def get_blob_service_client():
    retry = ExponentialRetry(initial_backoff=1, increment_base=3, retry_total=5)
    return BlobServiceClient(account_url=AZURE_URL, credential=AZURE_TOKEN, retry_policy=retry)


def get_container():
    logging.info("Getting container")
    blob_service_client = get_blob_service_client()
    container = blob_service_client.get_container_client(AZURE_CONTAINER)
    return container


def show_blobs(container):
    blob_list = [x for x in container.list_blobs()]
    # blob_list = container.list_blobs()
    for blob in blob_list:
        logging.debug(f"{container.container_name}: {blob.name}")


def find_latest():
    zips = [x for x in listdir_sorted(DIR_ZIP) if x.endswith(".zip")]
    return os.path.join(DIR_OUTPUT, os.path.splitext(zips[-1])[0])


def upload_static():
    global container
    if not read_config():
        logging.info("Azure not configured so not publishing static files")
        return False
    logging.info("Azure configured so publishing static files")
    dir_bounds = "/appl/data/generated/bounds"
    files_bounds = [x for x in listdir_sorted(dir_bounds) if x.startswith("bounds.")]
    if container is None:
        # wait until we know we need it
        container = get_container()
    logging.info("Listing blobs")
    dir_remote = "static"
    # delete old blobs
    blob_list = [x for x in container.list_blobs(name_starts_with=f"{dir_remote}/bounds.")]
    for blob in blob_list:
        logging.info("Deleting %s", blob.name)
        container.delete_blob(blob.name)
    # archive_current(container)
    for f in files_bounds:
        logging.debug("Pushing %s", f)
        path = os.path.join(dir_bounds, f)
        # HACK: just upload into archive too so we don't have to move later
        with open(path, "rb") as data:
            container.upload_blob(name=f"{dir_remote}/{f}", data=data, overwrite=True)


def upload_dir(dir_run=None):
    changed = False
    if not FLAG_IGNORE_PERIM_OUTPUTS:
        raise NotImplementedError("Need to deal with perimeters properly")
    if not read_config():
        logging.info("Azure not configured so not publishing %s", dir_run)
        return False
    if dir_run is None:
        dir_run = find_latest()
    logging.info("Azure configured so publishing %s", dir_run)
    run_name = os.path.basename(dir_run)
    run_id = run_name[run_name.index("_") + 1 :]
    source = run_name[: run_name.index("_")]
    as_datetime = pd.to_datetime(run_id)
    date = as_datetime.strftime(FMT_DATE_YMD)
    push_datetime = datetime.datetime.now(datetime.UTC)
    container = None
    dir_src = os.path.join(dir_run, "initial")
    dirs = listdir_sorted(dir_src)
    files_by_dir = {d: listdir_sorted(os.path.join(dir_src, d)) for d in dirs}
    origin = datetime.datetime.strptime(date, FMT_DATE_YMD).date()
    days = {d: (pd.to_datetime(d).date() - origin).days + 1 for d in dirs}
    run_length = max(days.values())
    metadata = {
        "model": "firestarr",
        "run_id": run_id,
        "run_length": f"{run_length}",
        "source": source,
        "origin_date": date,
    }
    if container is None:
        # wait until we know we need it
        container = get_container()
    dir_sim_data = os.path.join(DIR_RUNS, run_name, "data")
    dir_shp = f"{AZURE_DIR_DATA}_poly"
    file_root = "df_fires_prioritized"
    files_group = [x for x in listdir_sorted(dir_sim_data) if x.startswith(f"{file_root}.")]

    delete_after = []

    def remote_name(name):
        return name.replace(TMP_SUFFIX, "")

    def add_delete(match_start):
        nonlocal delete_after
        blob_list = [
            x for x in container.list_blobs(name_starts_with=match_start, include="metadata") if x.name.endswith(".tif")
        ]
        delete_after += blob_list

    def upload(path, name):
        nonlocal blobs
        changed = False
        mtime_src = str(os.path.getmtime(path))
        blob_dst = blobs.get(name, None)
        mtime_dst = None if blob_dst is None else blob_dst.metadata.get("file_modified_time", None)
        if mtime_src != mtime_dst:
            logging.debug("Pushing %s to %s" % (path, name))
            with open(path, "rb") as data:
                metadata["file_modified_time"] = mtime_src
                container.upload_blob(name=name, data=data, metadata=metadata, overwrite=True)
                changed = True
        if blob_dst is not None:
            del blobs[name]
        return changed

    # get old blobs for delete after
    logging.info("Finding %s blobs" % AZURE_DIR_DATA)
    # add_delete(f"{dir_shp}/{file_root}")
    dir_dst = os.path.basename(dir_run)
    add_delete(f"{AZURE_DIR_DATA}/{dir_dst}")

    blobs = {b.name: b for b in delete_after}
    for f in files_group:
        # NOTE: ignore if group changed
        upload(os.path.join(dir_sim_data, f), f"{dir_shp}/{f}")

    for d, files in files_by_dir.items():
        for_date = origin + datetime.timedelta(days=(days[d] - 1))
        metadata["for_date"] = for_date.strftime(FMT_DATE_YMD)
        for f in files:
            path = os.path.join(dir_src, d, f)
            p = remote_name(f"{dir_dst}/{d}/{f}")
            if upload(path, f"{AZURE_DIR_DATA}/{p}"):
                changed = True

    # delete old blobs that weren't overwritten
    for name, b in blobs.items():
        logging.debug("Removing %s", name)
        container.delete_blob(b)
    logging.info("Done pushing to azure")
    return changed


if "__main__" == __name__:
    upload_dir()
