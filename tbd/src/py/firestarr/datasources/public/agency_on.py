"""Ontario's publicly available data"""
import datetime
import json
import os
import urllib.parse
from functools import cache

import geopandas as gpd
import numpy as np
import pandas as pd
import tqdm_util
from common import (
    DIR_DOWNLOAD,
    FMT_DATE_YMD,
    FMT_DATETIME,
    FMT_FILE_MINUTE,
    do_nothing,
    ensure_dir,
    ensures,
    is_empty,
    locks_for,
    remove_timezone_utc,
)
from datasources.datatypes import (
    COLUMN_TIME,
    SourceFwi,
    SourceHourly,
    check_columns,
    make_template_empty,
)
from datasources.spotwx import fix_coords, fmt_rounded
from gis import find_closest, save_geojson
from make_bounds import get_features_canada
from net import try_save_http

DIR_AGENCY_ON = ensure_dir(os.path.join(DIR_DOWNLOAD, "agency", "ON"))
SERVER_LIO = "https://ws.lioservices.lrc.gov.on.ca"
URL_SERVER = f"{SERVER_LIO}/arcgis1061a/rest/services/MNRF/Ontario_Fires_Map/MapServer"
LAYER_FIRE_POINT = 0
LAYER_HOURLY = 29
LAYER_DAILY = 30
DATE_FIELDS = {
    LAYER_HOURLY: "OBSERVATION_DATE",
    LAYER_DAILY: "DFOSS_WEATHER_DATE",
}
QUERY_ALL = "1=1"
FIELDS_ALL = ["*"]
BOUNDS_ON = get_features_canada().set_index(["ID"]).loc[["ON"]]


def fix_date(t):
    if not t:
        return None
    # HACK: this was failing on something but I can't remember what
    try:
        if np.isnan(t):
            return None
    except TypeError:
        pass
    # HACK: shp can't save datetime
    return fmt_date(
        (datetime.datetime(1970, 1, 1) + datetime.timedelta(milliseconds=t))
    )


def fix_dates(df):
    for x in df.columns:
        # HACK: assume anything with 'DATE' in the name is a time since epoch
        if "DATE" in x and df.dtypes[x] != "O":
            df[x] = tqdm_util.apply(df[x], fix_date, desc="Fixing dates")
    return df


def parse_by_extension(path):
    format = os.path.splitext(path)[-1][1:]
    if "geojson" == format:
        return fix_dates(gpd.read_file(path))
    with open(path) as f:
        if "pjson" == format:
            return json.load(f)
        return f.readlines()


def do_query(
    save_as, layer, fct_parse=None, query=QUERY_ALL, fields=FIELDS_ALL, other=None
):
    url = f"{URL_SERVER}/{layer}/query?" + "&".join(
        [
            f"where={urllib.parse.quote(query)}",
            f"outFields={','.join(fields)}",
        ]
        + (other or [])
        + [
            f"f={os.path.splitext(save_as)[-1][1:]}",
        ]
    )
    return try_save_http(
        url,
        save_as,
        keep_existing=False,
        fct_pre_save=None,
        fct_post_save=lambda _: (fct_parse or do_nothing)(parse_by_extension(_)),
    )


def fmt_date(d):
    return d.strftime(FMT_DATETIME)


def get_query_date(field, datetime_start=None, datetime_end=None):
    if datetime_start is None:
        datetime_start = datetime.date.today() - datetime.timedelta(days=1)
    query = f"{field}>=TIMESTAMP '{fmt_date(datetime_start)}'"
    if datetime_end is not None:
        query += f" AND {field}<=TIMESTAMP '{fmt_date(datetime_end)}'"
    return query


# don't worry about updating this during same run right now
@cache
def check_latest(layer):
    save_as = os.path.join(DIR_AGENCY_ON, f"on_wx_layer{layer}_latest.pjson")
    # look up closest station in layer and make a query
    stats = (
        '[{"statisticType":"max","onStatisticField":"'
        + DATE_FIELDS[layer]
        + '","outStatisticFieldName":"latest"}]'
    )
    other = [f"outStatistics={urllib.parse.quote(stats)}"]

    def do_parse(df):
        return pd.to_datetime(
            fix_date(df["features"][0]["attributes"]["LATEST"]), utc=True
        )

    return do_query(save_as, layer, fct_parse=do_parse, fields=["latest"], other=other)


@cache
def get_stns(layer, latest):
    save_as = os.path.join(
        DIR_AGENCY_ON,
        f"on_wx_layer{layer}_stns_{latest.strftime(FMT_FILE_MINUTE)}.geojson",
    )
    return do_query(
        save_as,
        layer,
        query=f"{DATE_FIELDS[layer]}=TIMESTAMP '{fmt_date(latest)}'",
        fields=["WEATHER_STATION_CODE", "LATITUDE", "LONGITUDE"],
    )


@cache
def try_download_fwi(date):
    layer = LAYER_DAILY
    latest = check_latest(layer)
    date_utc = pd.Timestamp(date).tz_localize("UTC")
    if date_utc > latest:
        return make_template_empty("fwi")
    datetime_start = date_utc
    datetime_end = date_utc + datetime.timedelta(days=1)
    # always use PM so it's actuals
    wx_type = "PM"
    query = " AND ".join(
        [
            f"{get_query_date(DATE_FIELDS[layer], datetime_start, datetime_end)}",
            f"DFOSS_WEATHER_TYPE='{wx_type}'",
        ]
    )
    save_as = os.path.join(
        DIR_AGENCY_ON, f"on_wx_layer{layer}_{date.strftime(FMT_DATE_YMD)}.geojson"
    )

    def do_parse(df):
        if is_empty(df):
            return make_template_empty("fwi")
        df.columns = [x.lower() for x in df.columns]
        df = df.rename(
            columns={
                "latitude": "lat",
                "longitude": "lon",
            }
        )
        df[COLUMN_TIME] = tqdm_util.apply(
            df["dfoss_weather_date"],
            lambda x: pd.to_datetime(x.replace("00:00:00", "12:00:00")),
            desc="Assigning datetimes",
        )
        return df

    return do_query(save_as, layer, query=query, fct_parse=do_parse)


@cache
def get_fwi(date):
    # once we have fwi actuals for a date they shouldn't change
    file_fwi_date = os.path.join(
        DIR_AGENCY_ON, f"fwi_{date.strftime(FMT_DATE_YMD)}.geojson"
    )
    # can't ensure that this is going to be created if no data exists
    with locks_for(file_fwi_date):
        if not os.path.exists(file_fwi_date):
            # want this in another function so it caches
            df = try_download_fwi(date)
            if is_empty(df):
                return df
            save_geojson(df, file_fwi_date)
        return gpd.read_file(file_fwi_date)


class SourceFwiON(SourceFwi):
    def __init__(self, dir_out) -> None:
        super().__init__(bounds=BOUNDS_ON)
        self._dir_out = dir_out

    def _get_fwi(self, lat, lon, date):
        return find_closest(get_fwi(date), lat, lon)


def make_file_name(layer, hr_begin, hr_end, dir_out=DIR_AGENCY_ON):
    return os.path.join(
        dir_out,
        f"on_wx_layer{layer}_"
        f"{hr_begin.strftime(FMT_FILE_MINUTE)}"
        f"_{hr_end.strftime(FMT_FILE_MINUTE)}.geojson",
    )


def file_for_date(layer, d, dir_out=DIR_AGENCY_ON):
    return os.path.join(
        dir_out,
        f"on_wx_layer{layer}_{d.strftime(FMT_DATE_YMD)}.geojson",
    )


@cache
def get_hourly_date(dir_out, layer, date):
    layer = LAYER_HOURLY
    latest = check_latest(layer)
    date_utc = pd.Timestamp(date).tz_localize("UTC")
    if date_utc > latest:
        return make_template_empty("hourly")

    # 5000 row limit on query results means 208 stations with 24 hours of data
    # can be returned - should be more than enough, so just get full day for all
    # stations
    file_wx_date = file_for_date(layer, date, dir_out)

    @ensures(
        file_wx_date,
        True,
        fct_process=gpd.read_file,
        msg_error=(f"Couldn't get hourly weather for {date}"),
    )
    def do_create(_):
        hr_begin = pd.to_datetime(date)
        hr_end = pd.to_datetime(date) + datetime.timedelta(hours=23)
        # ask for any ranges we're missing
        query = " AND ".join(
            [
                f"{get_query_date(DATE_FIELDS[layer], hr_begin, hr_end)}",
            ]
        )

        def do_parse(df):
            df.columns = [x.lower() for x in df.columns]
            df = df.rename(
                columns={
                    "observation_date": COLUMN_TIME,
                    "winddir": "wd",
                    "adjwindspeed": "ws",
                    "rainfall": "prec",
                    "latitude": "lat",
                    "longitude": "lon",
                }
            )
            df["id"] = 0
            df["model"] = "observed"
            # HACK: wind can be 'null' so set to 0 if it is
            df.loc[df["wd"].isna(), "wd"] = 0
            df[COLUMN_TIME] = remove_timezone_utc(df[COLUMN_TIME])
            return df

        df = do_query(
            file_for_date(layer, date, DIR_AGENCY_ON),
            layer,
            query=query,
            fct_parse=do_parse,
        )
        df = check_columns(df, "hourly")
        save_geojson(df, _)
        return _

    return do_create(file_wx_date)


@cache
def get_hourly(dir_out, layer, datetime_start, datetime_end):
    if datetime_end is None:
        datetime_end = check_latest(layer)
    # remove timezone so matching works later
    datetime_start = remove_timezone_utc(datetime_start)
    datetime_end = remove_timezone_utc(datetime_end)
    file_stn_wx = make_file_name(layer, datetime_start, datetime_end, dir_out)

    @ensures(
        file_stn_wx,
        True,
        fct_process=gpd.read_file,
        msg_error=(
            f"Couldn't get hourly weather from" f"{datetime_start} to {datetime_end}"
        ),
    )
    def do_create(_):
        df_wx = None
        for d in pd.date_range(
            datetime_start.date(),
            datetime_end.date(),
            freq="D",
            inclusive="both",
        ):
            df_wx = pd.concat([df_wx, get_hourly_date(dir_out, layer, d)])
        df_wx["datetime"] = remove_timezone_utc(df_wx["datetime"])
        df_wx = df_wx.sort_values([COLUMN_TIME])
        df_wx = df_wx.loc[
            (df_wx["datetime"] >= datetime_start) & (df_wx["datetime"] <= datetime_end)
        ]
        save_geojson(df_wx, _)
        return _

    return do_create(file_stn_wx)


class SourceHourlyON(SourceHourly):
    def __init__(self, dir_out) -> None:
        super().__init__(bounds=BOUNDS_ON)
        self._dir_out = dir_out

    def _get_wx_hourly(self, lat, lon, datetime_start, datetime_end=None):
        lat, lon = fix_coords(lat, lon)
        layer = LAYER_HOURLY
        file_wx = os.path.join(
            self._dir_out,
            f"on_wx_layer{layer}_{fmt_rounded(lat)}_{fmt_rounded(lon)}.geojson",
        )

        # don't try checking for updates within the same run
        @ensures(
            file_wx,
            True,
            fct_process=gpd.read_file,
            msg_error=(
                f"Couldn't get hourly weather for ({lat}, {lon}) from"
                f"{datetime_start} to {datetime_end}"
            ),
        )
        def do_create(_, datetime_start, datetime_end):
            df_hourly = get_hourly(self._dir_out, layer, datetime_start, datetime_end)
            # CHECK: might get station that's closest but doesn't exist for timespan
            df_wx = find_closest(df_hourly, lat, lon)
            save_geojson(df_wx, _)
            return _

        return do_create(file_wx, datetime_start, datetime_end)
