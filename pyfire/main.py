import os

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
import numpy as np
import rasterio
from rasterio import features
from rasterio.mask import mask
import rasterio as rio
from rasterio.plot import show

import subprocess

import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
# Implement the default Matplotlib key bindings.
from matplotlib.backend_bases import key_press_handler
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import shutil
import glob

DIR_OUT = 'Data/output.pyfire'
FILES = None
GEOMS = None
ax = None
root = tk.Tk()
frmMap = tk.Frame()
frmLocation = tk.Frame()
frmFWI = tk.Frame()
frmSettings = tk.Frame()
frmFile = tk.Frame()
fig = Figure(figsize=(5, 4), dpi=100)

canvas1 = FigureCanvasTkAgg(fig, master=frmMap)
canvas1.draw()

toolbar = NavigationToolbar2Tk(canvas1, frmMap)
toolbar.update()
toolbar.pack(side=tk.TOP, fill=tk.X, padx=8)

canvas1.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=1, padx=0, pady=0)

canvas1._tkcanvas.pack(side=tk.TOP, fill=tk.BOTH, expand=1, padx=0, pady=0)

frmMap.pack(fill=tk.BOTH, expand=1)


def add_entry(master, name, value, upper, lower=0, increment=1):
    var = tk.StringVar()
    var.set(str(value))
    label = tk.Label(master, text=name)
    entry = tk.Spinbox(master, from_=lower, to=upper, textvariable=var, increment=increment)
    label.pack(side="left")
    entry.pack(side="left")
    return label, entry, var

lblLat, inputLat, varLat = add_entry(frmLocation, "Latitude", 52.01, upper=90, lower=-90, increment=0.001)
lblLon, inputLon, varLon = add_entry(frmLocation, "Longitude", -89.024, upper=180, lower=-180, increment=0.001)
frmLocation.pack()

lblFFMC, inputFFMC, varFFMC = add_entry(frmFWI, "FFMC", 90, upper=101, increment=0.1)
lblDMC, inputDMC, varDMC = add_entry(frmFWI, "DMC", 40, upper=1000)
lblDC, inputDC, varDC = add_entry(frmFWI, "DC", 300, upper=10000)
lblAPCP, inputAPCP, varAPCP = add_entry(frmFWI, "APCP", 0, upper=1000, increment=0.1)

frmFWI.pack()

varFile = tk.StringVar()
lblFile = tk.Label(frmFile, text="File")
optFile = tk.OptionMenu(frmFile, varFile, None)
lblFile.pack(side=tk.LEFT)
optFile.pack(side=tk.LEFT)
frmFile.pack()

lblConfidence, inputConfidence, varConfidence = add_entry(frmSettings, "Confidence Level", 0.1, upper=1, lower=0.01, increment=0.01)
frmSettings.pack()

btnRun = tk.Button(text="Run")
btnRun.pack(side="bottom")


def handle_click(event):
    print("Running...")
    do_run()
    update_menu()


btnRun.bind("<Button-1>", handle_click)

def do_run():
    if os.path.exists(DIR_OUT):
        shutil.rmtree(DIR_OUT)
    ffmc = float(varFFMC.get())
    dmc = float(varDMC.get())
    dc = float(varDC.get())
    apcp_0800 = float(varAPCP.get())
    lat = float(varLat.get())
    lon = float(varLon.get())
    confidence = float(varConfidence.get())
    args = './{} 2017-08-27 {} {} 12:15 --wx test/wx.csv --ffmc {} --dmc {} --dc {} --apcp_0800 {} --confidence {}--no-intensity -v -v'.format(
        DIR_OUT, lat, lon, ffmc, dmc, dc, apcp_0800, confidence)
    cmd = [
        'wsl',
        'bash',
        '-c',
        'DOCKER_HOST="unix:///mnt/wsl/shared-docker/docker.sock" /usr/bin/docker-compose exec tbd cmake-build-release/tbd {}'.format(
            args)
    ]
    subprocess.run(cmd)

def update_menu():
    global FILES
    global GEOMS
    dir = r'../tbd/{}'.format(DIR_OUT)
    FILES = glob.glob("{}/*.tif".format(dir))
    print(FILES)
    menu = optFile["menu"]
    menu.delete(0, "end")
    for f in FILES:
        menu.add_command(label=f,
                         command=lambda value=f: varFile.set(value))
    if 0 < len(FILES):
        GEOMS = find_bounds()
        varFile.set(FILES[-1])
    else:
        GEOMS = None

def find_bounds():
    with rasterio.open(FILES[-1]) as src:
        b = src.bounds
        geoms = [{
            'type': 'Polygon',
            'coordinates': [[(b.left, b.top), (b.right, b.top), (b.right, b.bottom), (b.left, b.bottom)]]
        }]
        return geoms

def do_draw():
    global ax
    global fig
    global GEOMS

    fp = varFile.get()
    if fp is None:
        return

    fig.clf()
    if ax is not None:
        ax.cla()
    ax = fig.add_subplot(111)
    fig.subplots_adjust(bottom=0, right=1, top=1, left=0, wspace=0, hspace=0)

    # the first one is your raster on the right
    # and the second one your red raster
    with rasterio.open(fp) as src:
        proj4 = src.crs.to_proj4()
        print(proj4)
        central_meridian = None
        zone = None
        for kv in proj4.split():
            if kv.startswith('+lon_0='):
                central_meridian = float(kv[kv.find('=') + 1:])
                # meridian of -93 is zone 15
                zone = 15.0 + (central_meridian + 93.0) / 6.0
            elif kv.startswith('+zone='):
                zone = float(kv[kv.find('=') + 1:])
        file_dem = '../data/generated/grid/dem_{}.tif'.format("{}".format(zone).replace('.', '_'))
        with rasterio.open(file_dem) as src_dem:
            # crop the second raster using the
            # previously computed shapes
            out_img, out_transform = mask(
                dataset=src_dem,
                shapes=GEOMS,
                crop=True,
            )
            file_out = 'dem.tif'
            if os.path.exists(file_out):
                os.remove(file_out)
            # save the result
            # (don't forget to set the appropriate metadata)
            with rasterio.open(
                    file_out,
                    'w',
                    driver='GTiff',
                    crs=src_dem.crs,
                    height=out_img.shape[1],
                    width=out_img.shape[2],
                    count=src_dem.count,
                    dtype=out_img.dtype,
                    transform=out_transform,
                    nodata=src_dem.nodata
            ) as dst:
                dst.write(out_img)
            with rio.open(file_out) as src_plot:
                show(src_plot, ax=ax, cmap='gist_gray')
        # crop the second raster using the
        # previously computed shapes
        out_img, out_transform = mask(
            dataset=src,
            shapes=GEOMS,
            crop=True,
        )
        file_out = 'result.tif'
        if os.path.exists(file_out):
            os.remove(file_out)
        # save the result
        # (don't forget to set the appropriate metadata)
        with rasterio.open(
                file_out,
                'w',
                driver='GTiff',
                crs=src.crs,
                height=out_img.shape[1],
                width=out_img.shape[2],
                count=src.count,
                dtype=out_img.dtype,
                transform=out_transform,
                nodata=src.nodata
        ) as dst:
            dst.write(out_img)
        with rio.open(file_out) as src_plot:
            show(src_plot, ax=ax, cmap='Oranges', alpha=0.3)
            show(src_plot, ax=ax, cmap='gist_gray', contour=True)

    plt.close()
    ax.set(title="", xticks=[], yticks=[])
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_visible(False)
    ax.spines["bottom"].set_visible(False)
    canvas1.draw()

def on_pick_file(self, name='', index='', mode=''):
    do_draw()

varFile.trace("w", on_pick_file)
update_menu()
root.mainloop()