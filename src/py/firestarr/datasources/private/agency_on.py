import json
import os
import re
from functools import cache

import geopandas as gpd
import numpy as np
import pandas as pd
import tqdm_util
from bs4 import BeautifulSoup
from common import do_nothing, read_csv_safe, to_utc, unzip
from datasources.datatypes import (
    SourceFire,
    SourceModel,
    get_columns,
    pick_date_refresh,
)
from datasources.public.agency_on import BOUNDS_ON, SERVER_LIO, parse_by_extension
from gis import area_ha, find_closest, gdf_from_file, to_gdf
from net import try_save_http

NO = 0
YES = 1
ADD = 2

USE_CONVECTIVE = NO

FINAL_PERIM = 15
INTERIM_PERIM = 17

SERVER = f"{SERVER_LIO}/arcgis3/rest/services/FFIM/Forest_Fire_Info_Map/MapServer"
SERVER_EXTRANET = "https://www.affes.mnr.gov.on.ca"
SERVER_LIO_APPS = "https://www.lioapplications.lrc.gov.on.ca/Geocortex/Essentials"
SERVER_PERIM_ZIP = "https://ws.download.affes.mnr.gov.on.ca"

URL_WX_FORECAST = f"{SERVER_EXTRANET}/Extranet/CFS/METEOBLUE Forecast Report.csv"
URL_FIRES = f"{SERVER_PERIM_ZIP}/GIS_Perimeters/ActiveFire_Perimeters.zip"
URL_TOKEN = f"{SERVER_LIO_APPS}/essentials414/REST/sites/FFIM?f=json&deep=true"


@cache
def get_fires_kmz(dir_out):
    def do_parse(_):
        files = unzip(_, dir_out)
        file_kmz = [x for x in files if x.endswith(".kmz")][0]
        files_kmz = unzip(file_kmz, dir_out)
        file_kml = [x for x in files_kmz if x.endswith(".kml")][0]
        gpd.io.file.fiona.drvsupport.supported_drivers["KML"] = "rw"
        df_kmz = gdf_from_file(file_kml)

        def get_data(row):
            name = row["Name"]
            desc = row["Description"]
            geometry = row["geometry"]
            p = BeautifulSoup(desc)
            data = {"name": name, "geometry": geometry}
            for tr in p.find_all("tr"):
                tds = tr.find_all("td")
                if 2 == len(tds):
                    k, v = [x.text for x in tds]
                    data[k] = v
            return data

        data = tqdm_util.apply(df_kmz, get_data, desc="Reading kmz")
        names = df_kmz["Name"]
        df = pd.DataFrame({names[i]: data[i] for i in range(len(names))}).transpose()
        gdf = gpd.GeoDataFrame(df, crs="WGS84", geometry=df_kmz.geometry)
        gdf["fire_name"] = tqdm_util.apply(gdf["Fire_Numbe"], lambda x: f"{x[:3]}_FIRE_{x[3:]}", desc="Finding names")
        return gdf

    return try_save_http(
        URL_FIRES,
        os.path.join(dir_out, "on_fires.zip"),
        keep_existing=False,
        fct_post_save=do_parse,
    )


def get_token():
    def do_parse(_):
        with open(_) as f:
            j = json.load(f)
            s = json.dumps(j)
            i = s.index("tokenUrl")
            v = s[i:]
            v = v[: v.index('"')]
            token = v[v.index("token=") :].replace("token=", "")
            return token

    return try_save_http(
        URL_TOKEN,
        "/appl/data/tmp/on_fires_test.html",
        keep_existing=False,
        fct_pre_save=None,
        fct_post_save=do_parse,
    )


def get_perim_layer(layer, save_as, token=None):
    if token is None:
        token = get_token()
    url = f"{SERVER}/{layer}/query?token={token}&where=1%3D1&outFields=*&f=geojson"
    return try_save_http(
        url,
        save_as,
        keep_existing=False,
        fct_pre_save=None,
        fct_post_save=parse_by_extension,
    )


@cache
def get_wx_forecast(dir_out):
    def do_parse(_):
        df_wx = None
        # HACK: utf-8 fails if it's utf-16-le, but the other way works and give the
        # wrong language
        ENCODINGS = ["utf-8", "utf-16-le"]
        # HACK: utf16 was using \t as sep
        SEP = [",", "\t"]
        i = 0
        while df_wx is None and i < len(ENCODINGS):
            try:
                df_wx = read_csv_safe(_, sep=SEP[i], encoding=ENCODINGS[i])
            except KeyboardInterrupt as ex:
                raise ex
            except Exception:
                pass
            i += 1
        if not (df_wx is not None and "OFFICIAL_STATION_CODE" in df_wx.columns):
            raise RuntimeError(f"Error reading weather. Got result:\n{str(df_wx)}")
        df_wx["prec"] = df_wx["PRECIPITATION"]
        if USE_CONVECTIVE == ADD:
            df_wx["prec"] = df_wx["prec"] + df_wx["CONVECTIVE_PRECIPITATION"]
        elif USE_CONVECTIVE == YES:
            df_wx["prec"] = df_wx["CONVECTIVE_PRECIPITATION"]
        df_wx["model"] = "AFFES"
        df_wx["id"] = 0
        df_wx["CREATED_DATE"] = pd.to_datetime(df_wx["CREATED_DATE"])
        df_wx["FCST_TIMESTAMP"] = pd.to_datetime(df_wx["FCST_TIMESTAMP"], format="%b %d, %Y %I:%M:%S %p")
        df_latest = (
            df_wx[["LATITUDE", "LONGITUDE", "FCST_TIMESTAMP", "CREATED_DATE"]]
            .drop_duplicates()
            .sort_values(["CREATED_DATE"])
            .groupby(["LATITUDE", "LONGITUDE", "FCST_TIMESTAMP"])
            .first()
        ).reset_index()
        df_fcst = pd.merge(df_latest, df_wx, how="left")
        df_fcst = df_fcst.rename(
            columns={
                "OFFICIAL_STATION_CODE": "stn",
                "NETWORK": "org",
                "LATITUDE": "lat",
                "LONGITUDE": "lon",
                "FCST_TIMESTAMP": "datetime",
                "TEMPERATURE": "temp",
                "WINDSPEED": "ws",
                "WINDDIRECTION": "wd",
                "RELATIVEHUMIDITY": "rh",
                "CREATED_DATE": "issuedate",
            }
        )
        gdf = to_gdf(df_fcst)[get_columns("model")]
        # HACK: seems like there are multiple stations with the same lat/long
        return gdf.drop_duplicates()

    return try_save_http(
        URL_WX_FORECAST,
        os.path.join(dir_out, "on_wx_forecast.csv"),
        keep_existing=True,
        fct_pre_save=do_nothing,
        fct_post_save=do_parse,
    )


class SourceFireON(SourceFire):
    def __init__(self, dir_out) -> None:
        super().__init__(bounds=BOUNDS_ON)
        self._dir_out = dir_out
        self._token = None

    def _get_fires(self):
        # # FIX: use points for status?
        # file_pts = os.path.join(self._dir_out, "on_fire_points.geojson")
        # df_points = do_query(
        #     file_pts,
        #     LAYER_FIRE_POINT,
        #     fields=[
        #         "FIELD_FIRE_SIZE",
        #         "FIELD_AGENCY_FIRE_ID",
        #         "FIELD_STAGE_OF_CONTROL_STATUS",
        #     ],
        # )
        if self._token is None:
            self._token = get_token()
        df_final = get_perim_layer(
            FINAL_PERIM,
            os.path.join(self._dir_out, "on_fires_final.geojson"),
            self._token,
        )
        df_interim = get_perim_layer(
            INTERIM_PERIM,
            os.path.join(self._dir_out, "on_fires_interim.geojson"),
            self._token,
        )
        df = pd.concat([df_final, df_interim])
        df = df.rename(columns={c: c.lower() for c in df.columns})
        df["as_of"] = to_utc(df["date_mapped"])
        df["refresh"] = to_utc(df["refresh_datetime"])
        df["datetime"] = tqdm_util.apply(
            df,
            lambda x: pick_date_refresh(x["as_of"], x["refresh"]),
            desc="Finding refresh dates",
        )
        # HACK: ignore anything without a valid date
        df = df.loc[~np.isnan(df["datetime"])]

        def make_name(d, f):
            return f"{d.year}_ON_{f[:3]}_FIRE_{f[3:]}"

        df["fire_name"] = tqdm_util.apply(df, lambda x: make_name(x["datetime"], x["firenumb"]), desc="Naming fires")
        r = re.compile("^[0-9]{4}_ON_[A-Z]{3}_FIRE_[0-9]{3}$")
        df = df.loc[tqdm_util.apply(df["fire_name"], lambda x: bool(r.match(x)), desc="Filtering by name")]
        df = df.dissolve(by=["fire_name"])
        df["area"] = area_ha(df)
        df["status"] = None
        return df


class SourceModelON(SourceModel):
    def __init__(self, dir_out) -> None:
        super().__init__(bounds=BOUNDS_ON)
        self._dir_out = dir_out

    def _get_wx_model(self, lat, lon):
        return find_closest(get_wx_forecast(self._dir_out), lat, lon)
