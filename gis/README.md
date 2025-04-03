This group of tools processes eFRI and other data to create fuel, slope, aspect and
elevation rasters.

# Introduction

# Setup

## Step 1: Run the setup

This should properly set up a python environment

## Step 2: Run grids/collect.py

This should get some base data that is required for generating the grids

## Step 3: Run earthenv.py

This should make the DEM for the area defined in the settings file

## Step 4: Run canvec/canvec.py

This should collect the canvec data that gets used for making more accurate water in the grids.
To minimize amount of downloaded data, run `python canvec.py extract Hydro` to just get water data.

## Step 5: Run grids/make_grids.py

This should generate the rasters that are required for FireSTARR to work

## Step 6: Place the grids in ../100m/default

This will make the grids the default grids used for the FireSTARR simulations. Otherwise, place
them in ../100m/{YEAR} to have them be used for simulations occurring during a certain year.
