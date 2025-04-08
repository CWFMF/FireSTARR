import datetime
import os

import numpy as np
import pandas as pd
import pytz
from common import (
    FLAG_DEBUG,
    SECONDS_PER_HOUR,
    cffdrs,
    ensure_dir,
    ensures,
    in_run_folder,
    in_sim_folder,
    is_empty,
    logging,
    to_csv_safe,
    tqdm_util,
    tz_from_offset,
)
from datasources.datatypes import COLUMN_MODEL, COLUMN_TIME, COLUMNS_STREAM
from datasources.default import (
    SourceFwiBest,
    SourceHourlyBest,
    SourceModelAll,
    wx_interpolate,
)
from redundancy import NUM_RETRIES
from sim_wrapper import get_simulation_file
from timezonefinder import TimezoneFinder

from gis import CRS_COMPARISON, KM_TO_M, gdf_from_file, make_point, save_geojson

MAXIMUM_STATION_DISTANCE = 100 * KM_TO_M


def save_wx_input(df_wx, file_wx):
    df_wx = df_wx.loc[:]
    # rename to match expected columns for firestarr csv
    df_wx.columns = [x.upper() for x in df_wx.columns]
    # FIX: assign model, id combinations scenario numbers
    df_wx = df_wx.rename(
        columns={
            COLUMN_TIME.upper(): "Date",
            COLUMNS_STREAM[-1].upper(): "Scenario",
        }
    )[
        [
            "Scenario",
            "Date",
            "PREC",
            "TEMP",
            "RH",
            "WS",
            "WD",
            "FFMC",
            "DMC",
            "DC",
            "ISI",
            "BUI",
            "FWI",
        ]
    ]
    # remove timezone so it outputs in expected format
    df_wx["Date"] = df_wx["Date"].apply(lambda x: x.tz_localize(None))
    to_csv_safe(df_wx.round(2), file_wx, index=False, quoting=False)
    return file_wx


class Simulation(object):
    def __init__(self, dir_out, dir_sims, origin) -> None:
        self._dir_out = dir_out
        self._dir_sims = dir_sims
        self._origin = origin
        self._src_fwi = SourceFwiBest(self._dir_out)
        self._src_models = SourceModelAll(self._dir_out)
        self._src_hourly = SourceHourlyBest(self._dir_out)

    def prepare(self, df_fire):
        if len(df_fire) > 1:
            raise RuntimeError("Expected exactly one row")
        if "fire_name" not in df_fire.columns:
            raise RuntimeError("Expected fire_name to be in columns")
        row_fire = df_fire.iloc[0]
        fire_name = row_fire["fire_name"]
        dir_fire = os.path.join(self._dir_sims, fire_name)
        file_sim = get_simulation_file(dir_fire)
        ensure_dir(dir_fire)
        ensure_dir(os.path.dirname(dir_fire))

        # want to return directory name of created file
        @ensures(
            file_sim,
            True,
            fct_process=os.path.dirname,
            mkdirs=True,
            retries=NUM_RETRIES,
        )
        def do_create(_):
            logging.debug("Saving %s to %s", fire_name, dir_fire)
            max_days = row_fire["DURATION"]
            lat = row_fire["lat"]
            lon = row_fire["lon"]
            tf = TimezoneFinder()
            tz_original = pytz.timezone(tf.timezone_at(lng=lon, lat=lat))
            # figure out what timezone offset is for origin date
            # CHECK: what happens if we go over daylight savings time date?
            #        nothing I think? We're finding offset for LST here
            date_origin = pd.to_datetime(self._origin.today)
            utcoffset = date_origin - (
                (date_origin + tz_original.dst(date_origin))
                .tz_localize(tz_original)
                .tz_convert("UTC")
                .tz_localize(None)
            )
            utcoffset_hours = utcoffset.total_seconds() / SECONDS_PER_HOUR
            # do this instead of using utcoffset() so we know it's LST
            tz_lst = tz_from_offset(utcoffset)
            file_wx = in_sim_folder(_.replace(".geojson", "_wx.csv"))
            ensure_dir(os.path.dirname(file_wx))
            file_wx_streams = in_run_folder(_.replace(".geojson", "_wx_streams.geojson"))
            ensure_dir(os.path.dirname(file_wx_streams))
            # add so that loop can decrement before call
            date_try = self._origin.offset(1)
            # if no data yet then problem with data source so stop
            date_bad = self._origin.offset(-3)

            def utc_to_lst_no_timezone(d):
                return d.tz_localize("UTC").tz_convert(tz_lst).tz_localize(None)

            # HACK: get the last couple days and pick the closest station
            df_wx_actuals = []
            while date_try > date_bad:
                date_try = date_try - datetime.timedelta(days=1)
                df_wx_cur = self._src_fwi.get_fwi(lat, lon, date_try)
                if 0 < len(df_wx_cur):
                    df_wx_actuals.append(df_wx_cur)
            if not df_wx_actuals:
                raise RuntimeError(f"Problem getting fwi for {fire_name}")
            df_wx_actual = pd.concat(df_wx_actuals)
            # HACK: calculate distance (probably the second time)
            # HACK: figure out if too far away to use
            pt = make_point(lat, lon, CRS_COMPARISON)
            dists = df_wx_actual.to_crs(CRS_COMPARISON).distance(pt)
            dist_min = min(dists)
            df_wx_actual = df_wx_actual.loc[dists == dist_min]
            # NOTE: actuals should be in LST already
            if dist_min > MAXIMUM_STATION_DISTANCE:
                logging.warning(
                    "Station for (%f, %f) is %fkm from location",
                    lat,
                    lon,
                    round(dist_min / KM_TO_M, 1),
                )
            ffmc_old, dmc_old, dc_old, time_startup = df_wx_actual.sort_values([COLUMN_TIME], ascending=False).iloc[0][
                ["ffmc", "dmc", "dc", COLUMN_TIME]
            ]
            # HACK: get hourly for date not time, so we can know when latest is
            df_wx_hourly_date = self._src_hourly.get_wx_hourly(lat, lon, time_startup.date()).reset_index()
            # NOTE: hourly wx comes as UTC
            df_wx_hourly_date[COLUMN_TIME] = df_wx_hourly_date[COLUMN_TIME].apply(utc_to_lst_no_timezone)
            df_wx_models = self._src_models.get_wx_model(lat, lon)
            # NOTE: model wx comes as UTC
            df_wx_models[COLUMN_TIME] = df_wx_models[COLUMN_TIME].apply(utc_to_lst_no_timezone)
            # fill before selecting after hourly so that we always have the hour
            # right after the hourly
            df_wx_forecast = pd.concat([wx_interpolate(g) for i, g in df_wx_models.groupby(COLUMN_MODEL)])
            cur_time = None
            if not is_empty(df_wx_hourly_date):
                cur_time = max(df_wx_hourly_date[COLUMN_TIME])
                df_wx_forecast = df_wx_forecast.loc[df_wx_forecast[COLUMN_TIME] > cur_time]
            # splice every other member onto shorter members
            dates_by_model = df_wx_forecast.groupby("model")[COLUMN_TIME].max().sort_values(ascending=False)
            # deprecated
            # df_wx_forecast.loc[:, "id"] = df_wx_forecast["id"].apply(lambda x: f"{x:02d}")
            ids = df_wx_forecast["id"]
            del df_wx_forecast["id"]
            df_wx_forecast.loc[:, "id"] = ids.apply(lambda x: f"{x:02d}")
            df_spliced = None
            for (
                idx,
                model,
                date_end,
            ) in dates_by_model.reset_index().itertuples():
                df_model = df_wx_forecast.loc[df_wx_forecast["model"] == model]
                if df_spliced is not None:
                    df_append = df_spliced.loc[df_spliced[COLUMN_TIME] > date_end]
                    for i, g1 in df_model.groupby(COLUMNS_STREAM):
                        for j, g2 in df_append.groupby(COLUMNS_STREAM):
                            df_cur = pd.concat([g1, g2])
                            df_cur.loc[:, "model"] = f"{i[0]}x{j[0]}"
                            df_cur.loc[:, "id"] = f"{i[1]}x{j[1]}"
                            df_spliced = pd.concat([df_spliced, df_cur])
                else:
                    df_spliced = df_model
            df_streams = None
            # HACK: avoid comparing to empty df
            df_wx_hourly = df_wx_hourly_date
            if not is_empty(df_wx_hourly):
                # select just whatever's after startup indices time
                df_wx_hourly = df_wx_hourly_date.loc[df_wx_hourly_date["datetime"] >= time_startup]
            if is_empty(df_wx_hourly):
                # if no hourly weather then start at start of streams
                df_streams = df_spliced
            else:
                # don't assume we aren't using multiple hourly observed streams
                for i, g1 in df_wx_hourly.groupby(COLUMNS_STREAM):
                    for j, g2 in df_spliced.groupby(COLUMNS_STREAM):
                        df_cur = pd.concat([g1, g2])
                        df_cur.loc[:, "model"] = f"{i[0]}x{j[0]}"
                        df_cur.loc[:, "id"] = f"{i[1]:02d}x{j[1]}"
                        df_streams = pd.concat([df_streams, df_cur])
            df_streams = df_streams.set_index(COLUMNS_STREAM)
            df_streams["Scenario"] = None
            # avoid:
            # 'PerformanceWarning: indexing past lexsort depth may impact performance'
            df_streams = df_streams.sort_index()
            df_stream_ids = df_streams.index.drop_duplicates()
            for i, idx in enumerate(df_stream_ids):
                df_streams.loc[idx, "Scenario"] = i
            # HACK: need id field for fwi call
            df_wx = df_streams.reset_index().rename(
                columns={
                    "id": "combination",
                    "Scenario": "id",
                    "lat": "lat_nearest",
                    "lon": "lon_nearest",
                }
            )
            df_wx.loc[:, "lat"] = lat
            df_wx.loc[:, "lon"] = lon
            if FLAG_DEBUG:
                # make it easier to see problems if cffdrs isn't working
                save_geojson(df_wx, file_wx_streams)
                df_wx = gdf_from_file(file_wx_streams)
            df_wx_fire = df_wx.rename(columns={"lon": "long", COLUMN_TIME: "TIMESTAMP"})
            # remove timezone so it gets formatted properly
            df_wx_fire.columns = [s.upper() for s in df_wx_fire.columns]
            df_wx_fire[["YR", "MON", "DAY", "HR"]] = list(
                tqdm_util.apply(
                    df_wx_fire["TIMESTAMP"],
                    lambda x: (x.year, x.month, x.day, x.hour),
                    desc="Splitting date into columns",
                )
            )
            df_wx_fire = df_wx_fire[
                [
                    "ID",
                    "LAT",
                    "LONG",
                    "TIMESTAMP",
                    "YR",
                    "MON",
                    "DAY",
                    "HR",
                    "TEMP",
                    "RH",
                    "WD",
                    "WS",
                    "PREC",
                ]
            ].sort_values(["ID", "LAT", "LONG", "TIMESTAMP"])
            # NOTE: expects weather in localtime, but uses utcoffset to
            # figure out local sunrise/sunset
            # FIX: if values are not valid then station isn't started so use TMP to figure out when it should
            if not (0 <= ffmc_old):
                print(f"Invalid FFMC value for startup {ffmc_old}")
                ffmc_old = 0
            if not (0 <= dmc_old):
                print(f"Invalid DMC value for startup {dmc_old}")
                dmc_old = 0
            if not (0 <= dc_old):
                print(f"Invalid DC value for startup {dc_old}")
                dc_old = 0
            # HACK: calculate from hour after startup since startup values are on startup hour
            df_wx_since_startup = df_wx_fire.loc[df_wx_fire["TIMESTAMP"] > time_startup].sort_values(
                ["ID", "LAT", "LONG", "TIMESTAMP"]
            )
            df_wx_at_startup = df_wx_fire.loc[df_wx_fire["TIMESTAMP"] == time_startup].loc[:]
            if 0 < len(df_wx_at_startup):
                df_wx_at_startup.loc[:, "FFMC"] = ffmc_old
                df_wx_at_startup.loc[:, "DMC"] = dmc_old
                df_wx_at_startup.loc[:, "DC"] = dc_old
                df_wx_at_startup.loc[:, "ISI"] = df_wx_at_startup.apply(
                    lambda row: cffdrs.initial_spread_index(row["WS"], row["FFMC"]), axis=1
                )
                df_wx_at_startup.loc[:, "BUI"] = df_wx_at_startup.apply(
                    lambda row: cffdrs.buildup_index(row["DMC"], row["DC"]), axis=1
                )
                df_wx_at_startup.loc[:, "FWI"] = df_wx_at_startup.apply(
                    lambda row: cffdrs.fire_weather_index(row["ISI"], row["BUI"]), axis=1
                )
            df_fwi = cffdrs.hFWI(df_wx_since_startup, utcoffset_hours, ffmc_old, dmc_old, dc_old, silent=True)
            df_fwi.columns = [x.upper() for x in df_fwi.columns]
            df_fwi["TIMESTAMP"] = df_fwi.apply(
                lambda row: pd.to_datetime(
                    datetime.datetime(int(row["YR"]), int(row["MON"]), int(row["DAY"]), int(row["HR"]))
                ),
                axis=1,
            )
            # HACK: current hFWI() doesn't return WD
            if "WD" not in df_fwi.columns:
                # HACK: TIMESTAMP might have half hour in it because of timezone so use components
                # df_fwi = pd.merge(df_fwi, df_wx_fire[["ID", "TIMESTAMP", "WD"]])
                df_fwi = pd.merge(df_fwi, df_wx_fire[["ID", "YR", "MON", "DAY", "HR", "WD"]])
            # reinsert data for startup hour
            df_fwi = pd.concat([df_fwi, df_wx_at_startup])
            df_fwi = df_fwi.sort_values(["ID", "LAT", "LONG", "TIMESTAMP"])
            df_fwi = df_fwi.rename(
                columns={
                    "TIMESTAMP": COLUMN_TIME,
                }
            )
            # # HACK: only keep days with 24 hours since that's how it worked previously
            # df_hours = df_fwi[["YR", "MON", "DAY", "HR"]].drop_duplicates().drop(["HR"], axis=1)
            # df_days = df_hours.groupby(df_hours.columns.tolist(), as_index=False).size()
            # days_to_keep = df_days[24 == df_days["size"]].apply(
            #     lambda row: datetime.date(row["YR"], row["MON"], row["DAY"]), axis=1
            # )
            # HACK: exclude days that don't have noon since firestarr breaks when noon isn't there
            days_to_keep = (
                df_fwi.loc[12 == df_fwi["HR"]].apply(lambda row: row[COLUMN_TIME].date(), axis=1).drop_duplicates()
            )
            # CHECK: should be keeping weather starting at noon values so spread event
            # probability has something to work from
            df_fwi = df_fwi[np.isin(df_fwi["datetime"].apply(lambda d: d.date()), days_to_keep)]
            df_fwi.columns = [x.lower() for x in df_fwi.columns]
            df_fwi[COLUMN_TIME] = df_fwi[COLUMN_TIME].apply(lambda x: x.tz_localize(tz_lst))
            df_wx = df_fwi.loc[:]
            days_available = (df_wx[COLUMN_TIME].max() - df_wx[COLUMN_TIME].min()).days
            max_days = min(days_available, max_days)
            # HACK: start from current time - assumes all fire perimeters are up
            #       to date as of latest weather
            # FIX: if we're running at noon then it didn't grab 1100 but it needs
            #       it for this
            # HACK: don't start right at start because the hour before is missing
            # HACK: compare origin UTC so day offsets are all the same dates
            start_time = (
                min(df_wx[df_wx[COLUMN_TIME] >= pd.to_datetime(self._origin.today).tz_localize(tz_lst)][COLUMN_TIME])
            ) + datetime.timedelta(hours=1)
            # if no hourly data then cur_time isn't set
            if cur_time is not None:
                cur_time_with_tz = cur_time.tz_localize(tz_lst)
                start_time = max(cur_time_with_tz, start_time)
            start_time = start_time.tz_convert(tz_lst)
            # start time already in local time
            df_fire["date_startup"] = time_startup.isoformat()
            df_fire["ffmc_old"] = ffmc_old
            df_fire["dmc_old"] = dmc_old
            df_fire["dc_old"] = dc_old
            # HACK: FIX: need to actually figure this out
            df_fire["apcp_prev"] = 0
            df_fire["max_days"] = max_days
            df_fire["utcoffset_hours"] = utcoffset_hours
            df_fire["start_time"] = start_time.isoformat()
            df_fire["wx"] = save_wx_input(df_wx, file_wx)
            ensure_dir(os.path.dirname(_))
            save_geojson(df_fire, _)
            return _

        return do_create(file_sim)
