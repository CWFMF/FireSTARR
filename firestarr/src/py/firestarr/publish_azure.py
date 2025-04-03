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

AZURE_URL = None
AZURE_TOKEN = None
AZURE_CONTAINER = None


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
    try:
        AZURE_URL = CONFIG.get("AZURE_URL", "")
        AZURE_TOKEN = get_token()
        AZURE_CONTAINER = CONFIG.get("AZURE_CONTAINER", "")
    except ValueError as ex:
        logging.error(ex)
        logging.warning("Unable to read azure config")
    return np.all(bool(x) for x in [AZURE_URL, AZURE_TOKEN, AZURE_CONTAINER])


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
        print(f"{container.container_name}: {blob.name}")


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
        logging.info(f"Deleting {blob.name}")
        container.delete_blob(blob.name)
    # archive_current(container)
    for f in files_bounds:
        logging.info(f"Pushing {f}")
        path = os.path.join(dir_bounds, f)
        # HACK: just upload into archive too so we don't have to move later
        with open(path, "rb") as data:
            container.upload_blob(name=f"{dir_remote}/{f}", data=data, overwrite=True)


def upload_dir(dir_run=None):
    if not FLAG_IGNORE_PERIM_OUTPUTS:
        raise NotImplementedError("Need to deal with perimeters properly")
    if not read_config():
        logging.info(f"Azure not configured so not publishing {dir_run}")
        return False
    if dir_run is None:
        dir_run = find_latest()
    logging.info(f"Azure configured so publishing {dir_run}")
    run_name = os.path.basename(dir_run)
    run_id = run_name[run_name.index("_") + 1 :]
    source = run_name[: run_name.index("_")]
    as_datetime = pd.to_datetime(run_id)
    date = as_datetime.strftime(FMT_DATE_YMD)
    push_datetime = datetime.datetime.now(datetime.UTC)
    container = None
    dir_combined = os.path.join(dir_run, "combined")
    files = listdir_sorted(dir_combined)
    # HACK: ignore perim for now
    # NOTE: fix this if extension ever changes, but prevent .tif.aux.xml files
    files = [f for f in files if "perim" not in f and f.endswith(".tif")]

    # assert ('perim.tif' in files)
    def get_day(f):
        i = f.rindex("_")
        n = int(f[(f[:i].rindex("_") + 1) : i])
        return n

    days = {f: get_day(f) for f in files if f != "perim.tif"}
    run_length = max(days.values())
    metadata = {
        "model": "firestarr",
        "run_id": run_id,
        "run_length": f"{run_length}",
        "source": source,
        "origin_date": date,
    }
    origin = datetime.datetime.strptime(date, FMT_DATE_YMD).date()
    if container is None:
        # wait until we know we need it
        container = get_container()
    dir_sim_data = os.path.join(DIR_RUNS, run_name, "data")
    dir_shp = "current_shp"
    file_root = "df_fires_prioritized"
    files_group = [x for x in listdir_sorted(dir_sim_data) if x.startswith(f"{file_root}.")]

    delete_after = []

    def add_delete(match_start):
        nonlocal delete_after
        blob_list = [x for x in container.list_blobs(name_starts_with=match_start)]
        delete_after += blob_list

    def upload(path, name):
        nonlocal delete_after
        logging.info(f"Pushing {name}")
        with open(path, "rb") as data:
            container.upload_blob(name=name, data=data, metadata=metadata, overwrite=True)
        # remove from list of files to delete if overwritten
        delete_after = [x for x in delete_after if x.name != name]

    # get old blobs for delete after
    logging.info("Finding current blobs")
    add_delete(f"{dir_shp}/{file_root}")
    add_delete("current/firestarr")

    for f in files_group:
        upload(os.path.join(dir_sim_data, f), f"{dir_shp}/{f}")

    for f in files:
        if "perim.tif" == f:
            for_date = origin
        else:
            for_date = origin + datetime.timedelta(days=(days[f] - 1))
        metadata["for_date"] = for_date.strftime(FMT_DATE_YMD)
        path = os.path.join(dir_combined, f)
        # HACK: just upload into archive too so we don't have to move later
        upload(path, f"current/{f}")
        # FIX: copy from container link instead of uploading multiple times
        # upload into folder for this run, but don't keep multiple versions
        upload(path, f"archive/{run_id}/{f}")

    # delete old blobs that weren't overwritten
    for f in delete_after:
        logging.info(f"Removing {f.name}")
        container.delete_blob(f)


if "__main__" == __name__:
    upload_dir()
