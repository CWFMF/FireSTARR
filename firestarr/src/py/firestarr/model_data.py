import datetime
import os
import urllib
from functools import cache

import tqdm_util
from common import is_empty, logging, read_csv_safe
from gis import CRS_COMPARISON, to_gdf
from net import try_save_http

WFS_ROOT = "https://cwfis.cfs.nrcan.gc.ca/geoserver/public/wms?service=wfs&version=2.0.0"
DEFAULT_STATUS_IGNORE = ["OUT", "UC", "BH", "U"]
# DEFAULT_STATUS_KEEP = ["OC"]
URL_CWFIS_DOWNLOADS = "https://cwfis.cfs.nrcan.gc.ca/downloads"


def make_query_geoserver(
    table_name,
    features=None,
    filter=None,
    wfs_root=WFS_ROOT,
    output_format="application/json",
    crs=None,
):
    logging.debug(f"Getting table {table_name} in projection {str(CRS_COMPARISON)}")
    url = "&".join(
        [
            f"{wfs_root}&request=GetFeature",
            f"typename={table_name}",
            f"outputFormat={output_format}",
        ]
    )
    if features is not None:
        url += f"&propertyName={features}"
    if crs is not None:
        url += f"&SRSName={crs}"
    if filter is not None:
        url += f"&CQL_FILTER={urllib.parse.quote(filter)}"
    logging.debug(url)
    return url


@cache
def get_wx_cwfis(dir_out, date, indices="temp,rh,ws,wdir,precip,ffmc,dmc,dc,bui,isi,fwi,dsr"):
    # HACK: use 2022 because it has 2023 in it right now
    layer = "public:firewx_stns_2022"
    year = date.year
    month = date.month
    day = date.day
    save_as = os.path.join(dir_out, "cwfis_layer_{:04d}-{:02d}-{:02d}.csv".format(year, month, day))

    def do_parse(_):
        logging.debug("Reading {}".format(_))
        df = read_csv_safe(_)
        # HACK: doesn't make column if df is empty
        if is_empty(df):
            df["datetime"] = None
        else:
            df["datetime"] = tqdm_util.apply(
                df,
                lambda x: datetime.datetime.strptime(x["rep_date"], "%Y-%m-%dT%H:00:00"),
                desc="Converting times",
            )
        df = df[["lat", "lon"] + indices.split(",") + ["datetime"]]
        df = df.sort_values(["datetime", "lat", "lon"])
        gdf = to_gdf(df)
        return gdf

    return try_save_http(
        make_query_geoserver(
            layer,
            features=f"rep_date,prov,lat,lon,elev,the_geom,{indices}",
            filter=f"rep_date={year:04d}-{month:02d}-{day:02d}T12:00:00",
            output_format="csv",
        ),
        save_as,
        True,
        None,
        do_parse,
    )
