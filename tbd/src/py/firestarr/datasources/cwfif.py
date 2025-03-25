import datetime
import json
import os
import re
from functools import cache

import numpy as np
import pandas as pd
from common import (
    BOUNDS,
    DIR_DOWNLOAD,
    do_nothing,
    ensure_dir,
    ensures,
    logging,
    read_csv_safe,
    remove_timezone_utc,
)
from datasources.datatypes import SourceModel
from net import try_save_http

from gis import gdf_from_file, save_geojson, to_gdf

DIR_CWFIF = ensure_dir(os.path.join(DIR_DOWNLOAD, "cwfif"))
URL_CWFIF_WX = "https://app-cwfmf-api-cwfis-dev.wittyplant-59b495b3.canadacentral.azurecontainerapps.io/gribwx?"
# GEPS model is 0.5 degree resoltion, so two digits is too much
# CHECK: seems like just rounding to 0.5 wouldn't always give the same closest
#       value as actual distance calculation?
COORDINATE_PRECISION = 1


def make_cwfif_query(model, lat, lon, **kwargs):
    model = model.lower()
    if not model:
        raise RuntimeError("No model specified")
    lat, lon = fix_coords(lat, lon)
    # HACK: needs member specified
    if model == "geps":
        kwargs["member"] = "all"
    # lat=59&lon=-125&duration=999&model=all&format=csv&precision=2&latest=True
    url = URL_CWFIF_WX + "&".join(
        [
            f"model={model}",
            f"lat={lat}",
            f"lon={lon}",
            # f"timezone=UTC",
            # f"duration=999",
            # f"format=csv",
            # f"precision=2",
            # f"latest=True",
        ]
        + [f"{k}={v}" for k, v in kwargs.items()]
    )
    return url


def make_cwfif_parse(need_column, fct_parse=None, expected_value=None):
    def do_parse(_):
        # df = read_csv_safe(_, encoding="utf-8")
        df = read_csv_safe(_)
        # df.columns = [x.lower for x in df.columns]
        valid = need_column in df.columns
        if valid:
            if expected_value:
                valid = list(np.unique(df[need_column])) == [expected_value]
        if not valid:
            with open(_) as f:
                for line in f.readlines():
                    if "api limit" in line.lower():
                        logging.fatal(line)
                        raise RuntimeError(line)
            str_suffix = f" with value {expected_value}" if expected_value else ""
            raise RuntimeError(f"Expected column {need_column}{str_suffix}")
        return (fct_parse or do_nothing)(df)

    return do_parse


def get_model_dir_uncached(model):
    model = model.lower()
    # request middle of bounds since point shouldn't change model time
    lat = BOUNDS["latitude"]["mid"]
    lon = BOUNDS["longitude"]["mid"]
    url = make_cwfif_query(model, lat, lon, recent="True")
    save_as = os.path.join(ensure_dir(os.path.join(DIR_CWFIF, model)), f"cwfif_{model}_current.json")

    def do_parse(_):
        with open(_) as f:
            txt = f.readlines()
            # HACK: ignore case
            txt = "".join(txt).lower()
            j = json.loads(txt)
            t = j[model]
            model_time = datetime.datetime.strptime(t, "%Y-%m-%d %H:%MZ")
            return ensure_dir(os.path.join(DIR_CWFIF, model, model_time.strftime("%Y%m%d_%HZ")))

    return try_save_http(
        url,
        save_as,
        keep_existing=False,
        fct_pre_save=None,
        fct_post_save=do_parse,
    )


# HACK: allow setting so it doesn't use current all the time
_MODEL_DIR = None


def set_model_dir(dir_model):
    global _MODEL_DIR
    _MODEL_DIR = dir_model


@cache
def get_model_dir(model):
    global _MODEL_DIR
    return _MODEL_DIR or get_model_dir_uncached(model)


def get_rounding():
    return COORDINATE_PRECISION


def fix_coords(lat, lon):
    n = get_rounding()
    return round(lat, n), round(lon, n)


def fmt_rounded(x):
    n = get_rounding()
    return f"{x:0{n + 4}.{n}f}"


def make_filename(model, lat, lon, ext):
    return f"cwfif_{model}_{fmt_rounded(lat)}_{fmt_rounded(lon)}.{ext}"


@cache
def query_wx_ensembles_rounded(model, lat, lon):
    dir_model = get_model_dir(model)
    url = make_cwfif_query(
        model,
        lat,
        lon,
        timezone="UTC",
        duration=999,
        format="csv",
        precision=1,
        latest=True,
    )
    save_as = os.path.join(dir_model, make_filename(model, lat, lon, "csv"))

    def do_parse(_):
        df_initial = pd.read_csv(_)
        df_wx = df_initial
        models = [x for x in df_wx["model"].unique()]

        df = None
        for i, g in df_wx.groupby(["model"]):
            m = i[0]
            g["id"] = models.index(m)
            # replace model with just name since members are [model + number]
            g["model"] = re.sub("\d", "", m)
            df = pd.concat([df, g])
        df.columns = [x.lower() for x in df.columns]
        df["datetime"] = remove_timezone_utc(df["datetime"])
        num_days = len(np.unique(df["datetime"].dt.date))
        if 14 > num_days:
            raise RuntimeError(f"Expected at least 14 days of weather in GEPS model but got {num_days}")
        df = df.rename(columns={"precip": "prec"})
        # HACK: re-parse from directory for now
        df["issuedate"] = remove_timezone_utc(datetime.datetime.strptime(os.path.basename(dir_model), "%Y%m%d_%HZ"))
        df["lat"] = lat
        df["lon"] = lon
        index_final = ["model", "lat", "lon", "issuedate", "id"]
        df = df[index_final + ["datetime", "temp", "rh", "wd", "ws", "prec"]]
        # HACK: grib data has values outside ranges, so api does since it's raw data
        df.loc[df["rh"] > 100, "rh"] = 100
        df.loc[df["rh"] < 0, "rh"] = 0
        df.loc[df["ws"] < 0, "ws"] = 0
        df.loc[df["prec"] < 0, "prec"] = 0
        df = df.set_index(index_final)
        return df

    print(url)
    return try_save_http(
        url,
        save_as,
        keep_existing=True,
        fct_pre_save=None,
        fct_post_save=do_parse,
    )


@cache
def get_wx_ensembles(model, lat, lon):
    lat, lon = fix_coords(lat, lon)
    # only care about limiting queries - processing time doesn't matter
    return query_wx_ensembles_rounded(model, lat, lon)


class SourceCWFIFModel(SourceModel):
    def __init__(self, dir_out) -> None:
        super().__init__(bounds=None)
        self._dir_out = dir_out

    def _get_wx_model(self, lat, lon):
        file_out = os.path.join(self._dir_out, make_filename(self.model(), lat, lon, "geojson"))

        # retry once in case existing file doesn't parse
        @ensures(
            file_out,
            True,
            fct_process=gdf_from_file,
            retries=1,
        )
        def do_create(_):
            gdf = to_gdf(get_wx_ensembles(self.model(), lat, lon).reset_index())
            save_geojson(gdf, _)
            gdf.to_csv(_.replace(".geojson", ".csv"))
            return _

        return do_create(file_out)


class SourceGEPS(SourceCWFIFModel):
    def __init__(self, dir_out) -> None:
        super().__init__(dir_out)

    @classmethod
    def model(cls):
        return "geps"
