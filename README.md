# FireSTARR

FireSTARR is a wildfire simulation and burn probability modelling system designed to support wildland fire response decision-making.

## Table of Contents

- [FireSTARR](#firestarr)
  - [Table of Contents](#table-of-contents)
  - [Documentation](#documentation)
    - [Additional Resources](#additional-resources)
  - [Usage](#usage)
    - [General Syntax](#general-syntax)

## Documentation

📖 **Full documentation and user guide:**

https://cwfmf.github.io/firestarr-cpp/

### Additional Resources

- User Guide: https://cwfmf.github.io/firestarr-cpp/
- Settings Reference: https://cwfmf.github.io/firestarr-cpp/settings.html
- Command Line Reference: https://cwfmf.github.io/firestarr-cpp/usage.html

---

## Usage

Specifies the arguments for use when using FireSTARR.

**Type:** `string`  
**Required:** Yes  
**Default:** None

### General Syntax

```text
firestarr.exe <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [OPTION]...

Usage: firestarr.exe <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [OPTION]...

Run simulations and save output in the specified directory

Usage: firestarr.exe surface <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [OPTION]...

Calculate probability surface and save output in the specified directory

.\firestarr.exe Output_Fire 2026-05-01 53 -100 13:00 \
    --ffmc 80 \
    --dmc 60 \
    --dc 300 \
    --tz 8

Input Options

  -h                        Show help
  -v                        Increase output level
  -q                        Decrease output level

  --ascii                   Save grids as .asc
  --no-tiff                 Do not save grids as .tif

  -i                        Save individual maps for simulations
  -s                        Run in synchronous mode

  --points                  Save simulation points to file
  --no-intensity            Do not output intensity grids
  --no-probability          Do not output probability grids
  --occurrence              Output occurrence grids
  --sim-area                Output simulation area grids

  --raster-root             Use specified directory as raster root
  --fuel-lut                Use specified fuel lookup table

  --tz                      UTC offset (hours)
  --curing                  Specify static grass curing
  --force-greenup           Force green up for all fires
  --force-no-greenup        Force no green up for all fires

  --log                     Output log file
  --wx                      Input weather file

  --deterministic           Run deterministically
                             (100% chance of spread & survival)

  --confidence              Use specified confidence level

  --perim                   Start from perimeter
  --size                    Start from size

  --ffmc                    Startup Fine Fuel Moisture Code
  --dmc                     Startup Duff Moisture Code
  --dc                      Startup Drought Code

  --apcp_prev               Startup precipitation between
                             1200 yesterday and start of
                             hourly weather

  --output_date_offsets     Override output date offsets