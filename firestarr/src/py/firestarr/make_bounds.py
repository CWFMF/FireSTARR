import os

import numpy as np
import pandas as pd
import shapely.geometry
from common import DIR_GENERATED, ensures
from gis import CRS_WGS84, gdf_from_file, gdf_to_file, load_geometry_file, vector_path

KM_TO_M = 1000
BY_NAME = {}
BUFFER_RESOLUTION = 32
SERVER_CENSUS = "https://www12.statcan.gc.ca/census-recensement"
URL_CANADA = f"{SERVER_CENSUS}/2011/geo/bound-limit/files-fichiers/2016/lpr_000b16a_e.zip"
URL_PARKS = "https://clss.nrcan-rncan.gc.ca/data-donnees/nplb_llpn/CLAB_CA_2023-09-08.zip"
centroids_canada = None
DIR_BOUNDS = os.path.join(DIR_GENERATED, "bounds")
FILE_BOUNDS = "bounds.geojson"
COLUMN_ENGLISH_NAME = "PRENAME"


def to_file_dir(df, name, dir_out, centroids_compare=None):
    print(name)
    global BY_NAME
    if name in BY_NAME and df.equals(BY_NAME[name]):
        return df
    step = len(BY_NAME)
    df_wgs84 = df.to_crs(CRS_WGS84)
    df_wgs84.to_file(os.path.join(dir_out, f"{step:02d}_{name}.geojson"))
    gdf_to_file(df_wgs84, dir_out, f"{step:02d}_{name}")
    if centroids_compare:
        try:
            print(dist(centroids_compare, centroids(dissolve(df).set_index(["EN"]))))
        except KeyboardInterrupt as ex:
            raise ex
        except Exception:
            print("problem comparing centroids")
    BY_NAME[name] = df
    return df


def to_envelope(df):
    df = df.iloc[:]
    df.geometry = df.envelope
    return df


def buffer(df, km, resolution=BUFFER_RESOLUTION):
    df = df.iloc[:]
    df.geometry = df.buffer(km * KM_TO_M, resolution=resolution)
    return df


def dissolve(df, by="ID"):
    return df.dissolve(by=by).reset_index().sort_values("EN")


def explode(df):
    return df.explode(index_parts=False)


def simplify(df, km):
    df = df.iloc[:]
    df.geometry = df.simplify(tolerance=km * KM_TO_M)
    return df


def convex_hull(df):
    df.geometry = df.convex_hull
    return df


def fill(df):
    df = df.iloc[:]

    def do_fill(b):
        polys = [shapely.geometry.Polygon(x) for x in b.interiors]
        for p in polys:
            b = b.union(p)
        return b

    df.loc[:, "geometry"] = df.geometry.apply(do_fill)
    return df


def centroids(df):
    c = df.centroid
    return {idx: c.loc[idx] for idx in df.index}


def dist(c1, c2):
    assert c1.keys() == c2.keys()
    return np.sum([v.distance(c2[k]) for k, v in c1.items()])


def get_features_canada(
    file_canada=URL_CANADA,
    dir_out=DIR_BOUNDS,
    file_bounds=FILE_BOUNDS,
    col_name=COLUMN_ENGLISH_NAME,
):
    file_out = vector_path(dir_out, "canada")

    @ensures(file_out, True, fct_process=gdf_from_file, mkdirs=True)
    def do_create(_):
        # col_name needs to be english name of area so it can join
        # on bounds column EN
        df_canada = load_geometry_file(file_canada).sort_values([col_name]).set_index([col_name])
        df = load_geometry_file(file_bounds).sort_values(["EN"]).to_crs(df_canada.crs).set_index(["EN"])
        df.loc[df_canada.index, "geometry"] = df_canada["geometry"]
        # don't need precise bounds so simplify
        df = simplify(df, 1)
        df.reset_index().to_file(_)
        return _

    return do_create(file_out)


def get_bounds_from_id(id, dir_out=DIR_BOUNDS, *args, **kwargs):
    id = id.upper()
    file_out = vector_path(dir_out, f"bounds_{id}")

    @ensures(file_out, True, fct_process=gdf_from_file, mkdirs=True)
    def do_create(_):
        gdf_ca = get_features_canada(dir_out=dir_out, *args, **kwargs)
        gdf = gdf_ca.set_index(["ID"]).loc[[id]]
        gdf = dissolve(fill(buffer(explode(gdf), 100)))
        gdf = simplify(gdf, 1)
        gdf.reset_index().to_file(_)
        return _

    return do_create(file_out)


def update_bounds(
    file_bounds=FILE_BOUNDS,
    dir_out=DIR_BOUNDS,
):
    df_canada = get_features_canada(file_bounds=file_bounds, dir_out=DIR_BOUNDS).set_index(["EN"])
    crs_orig = df_canada.crs

    df_parks = load_geometry_file(URL_PARKS)
    df_parks_all = df_parks.dissolve()[["geometry"]]
    df_parks_all[["EN", "ID", "FR", "PRIORITY", "DURATION"]] = [
        "Parks Canada",
        "PC",
        "Parcs Canada",
        2,
        14,
    ]
    df_parks_all = df_parks_all.set_index(["EN"]).to_crs(crs_orig)
    df_canada = pd.concat([df_canada, df_parks_all]).sort_index()
    centroids_canada = centroids(df_canada)

    def to_file(df, name):
        return to_file_dir(df, name, dir_out, centroids_canada)

    df_bounds = to_file(
        gdf_from_file(file_bounds).sort_values(["EN"]).to_crs(crs_orig),
        "bounds",
    )
    df = df_bounds.set_index(["EN"])
    if "Parks Canada" not in df.index:
        df = pd.concat([df, df_parks_all]).sort_index()
    df.loc[df_canada.index, "geometry"] = df_canada["geometry"]
    df = to_file(df.reset_index(), "bounds_exact")
    df = to_file(explode(df), "explode")
    df = to_file(buffer(df, 100), "buffer")
    df = to_file(dissolve(df), "buffer_simplify_dissolve")
    df = to_file(explode(df), "explode_dissolve")
    df = to_file(fill(df), "buffer_simplify_dissolve_fill")
    df = to_file(dissolve(df), "redissolve")
    df = to_file(simplify(df, 10), "simplify_10km")
    df = to_file(simplify(df, 100), "simplify_100km")
    assert list(df["EN"]) == list(df_bounds["EN"])
    bounds = (
        df.reset_index()[["ID", "EN", "FR", "PRIORITY", "DURATION", "geometry"]].set_index(["ID"]).to_crs(CRS_WGS84)
    )
    bounds.to_file(file_bounds)
    gdf_to_file(bounds, dir_out, "bounds")


if "__main__" == __name__:
    update_bounds()
