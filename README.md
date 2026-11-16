<?xml version="1.0" encoding="UTF-8"?>
<?asciidoc-toc?>
<?asciidoc-numbered?>
<article xmlns="http://docbook.org/ns/docbook" xmlns:xl="http://www.w3.org/1999/xlink" version="5.0" xml:lang="en">
`<info>`{=html}
<title>
Project Documentation
</title>
`<date>`{=html}2026-11-16`</date>`{=html} `</info>`{=html}
<section xml:id="_overview">
<title>
Overview
</title>
<simpara>
FireSTARR is designed to support wildland fire response decision-making,
and is the fire growth algorithm originally in the FireGUARD suite of
programs.
</simpara>
<simpara>
It focuses on the generation of burn probabilities from replicated
simulation of fire growth, smouldering, and natural extinction under the
weather and stochastic fire behaviour scenarios.
</simpara>
</section>
<section xml:id="usage">
<title>
Usage
</title>
<simpara>
Specifies the arguments for use when using FireSTARR.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
string
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Required
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Yes
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
None
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
<simpara>
Example:
</simpara>
<screen>
firestarr.exe \<output_dir\> \<yyyy-mm-dd\> \<lat\> \<lon\> \<HH:MM\>
\[OPTION\]...
</screen>
<section xml:id="_basic_help_printout">
<title>
Basic Help Printout
</title>
<simpara>
Arguments are:
</simpara>
<simpara>
`<literal>`{=html}E:`\firestarr`{=tex}`\firestarr`{=tex}.exe`</literal>`{=html}
</simpara>
<simpara>
`<literal>`{=html}Usage: firestarr.exe \<output_dir\> \<yyyy-mm-dd\>
\<lat\> \<lon\> \<HH:MM\> \[OPTION\]...​`</literal>`{=html}
</simpara>
<programlisting language="Run simulations and save output in the specified directory" linenumbering="unnumbered">

Usage: firestarr.exe surface \<output_dir\> \<yyyy-mm-dd\> \<lat\>
\<lon\> \<HH:MM\> \[OPTION\]...

Calculate probability surface and save output in the specified directory

Usage: firestarr.exe test \<output_dir\> \[OPTION\]...

Run test cases and save output in the specified directory

Input Options -h Show help -v Increase output level -q Decrease output
level --ascii Save grids as .asc --no-tiff Do not save grids as .tif -i
Save individual maps for simulations -s Run in synchronous mode --points
Save simulation points to file --no-intensity Do not output intensity
grids --no-probability Do not output probability grids --occurrence
Output occurrence grids --sim-area Output simulation area grids
--raster-root Use specified directory as raster root --fuel-lut Use
specified fuel lookup table --tz UTC offset (hours) --curing Specify
static grass curing --force-greenup Force green up for all fires
--force-no-greenup Force no green up for all fires --log Output log file
--wx Input weather file --deterministic Run deterministically (100%
chance of spread & survival) --confidence Use specified confidence level
--perim Start from perimeter --size Start from size --ffmc Startup Fine
Fuel Moisture Code --dmc Startup Duff Moisture Code --dc Startup Drought
Code --apcp_prev Startup precipitation between 1200 yesterday and start
of hourly weather --output_date_offsets Override output date offsets
</programlisting>
</section>
<section xml:id="_example_use_of_the_tool">
<title>
Example use of the tool
</title>
<simpara>
Note: FireSTARR will look to the location it is invoked from for it's
information to run. Any datasets the model requires need to be defined
with appropriate flags.
</simpara>
<simpara>
`<literal>`{=html}.`\firestarr`{=tex}.exe Output_Fire 2026-05-01 53 -100
13:00 --ffmc 80 --dmc 60 --dc 300 --tz 8`</literal>`{=html}
</simpara>
<simpara>
FFMC, DMC, DC and Timezone are mandatory fields to run FireSTARR
</simpara>
</section>
</section>
<section xml:id="_settings">
<title>
Settings
</title>
<simpara xml:id="settings.ini">
The settings.ini file contains a number of definitions that allow
FireSTARR to find the information it requires more rapidly as well as a
number of constants for use within your simulation.
</simpara>
</section>
<section xml:id="_critical_components">
<title>
Critical components
</title>
<simpara>
The fuel look up table (how FireStarr knows what the numbers in your
fuel grid represent), the root to your raster data, and the output date
offsets (what your forecast horrizons are).
</simpara>
<simpara>
`<literal>`{=html}RASTER_ROOT`</literal>`{=html} = ./generated/grid/100m
Root directory to read rasters from. FireSTARR will read from this
location without explicit definition
</simpara>
<simpara>
`<literal>`{=html}MINIMUM_ROS`</literal>`{=html} = 0.0Minimum rate of
spread before fire is considered to actually be spreading. If this is
above 0 it would yield results where the fire is not moving so long as
the rate of spread is below the value defined here.
</simpara>
<simpara>
`<literal>`{=html}MAX_SPREAD_DISTANCE`</literal>`{=html} = 0.4 Maximum
distance that head can spread per step (\* cell size).This setting will
ensure very fast fires are polling the underlying data so it's always
spreading the fuels represented by the underlying grid. If you go beyond
1 assessments of the fuel grid would not
</simpara>
<simpara>
`<literal>`{=html}OUTPUT_DATE_OFFSETS`</literal>`{=html} =
\[1,2,3,7,14\] Days to output probability contours for. In this case,
days 1, 2, 3 7 and 14 days.
</simpara>
<simpara>
`<literal>`{=html}FUEL_LOOKUP_TABLE`</literal>`{=html} = ./fuel.lut
Lookup table (LUT) for fuels (prometheus format). The Lookup table is
also the location where you would provide meaningful overrides for
non-fuels. The LUT that is shipped with FireSTARR has non-fuels (except
water) being turned into D-2 or some low conifer M series fuel. This
ensure some spread through non-fuel.
</simpara>
<simpara>
`<literal>`{=html}MINIMUM_FFMC`</literal>`{=html} = 65.0 Minimum ffmc
for fire to spread. This is essentially a global burning condition,
without an FFMC over 65, fire will not spread.
</simpara>
<simpara>
`<literal>`{=html}MINIMUM_FFMC_AT_NIGHT`</literal>`{=html} = 85.0
Minimum ffmc for fire to spread at night. This is essentially a global
burning condition, without an FFMC over 85, fire will not spread.
</simpara>
<simpara>
`<literal>`{=html}OFFSET_SUNRISE`</literal>`{=html} = 0.0 Time after
sunrise to start burning (hours). Another global burning condition to
limit fire spread.
</simpara>
<simpara>
`<literal>`{=html}OFFSET_SUNSET`</literal>`{=html} = 0.0 Time before
sunset to stop burning (hours). Another global burning condition to
limit fire spread.
</simpara>
<simpara>
`<literal>`{=html}THRESHOLD_SCENARIO_WEIGHT`</literal>`{=html} = 1.0
Weight given to the scenario thresholds
</simpara>
<simpara>
`<literal>`{=html}THRESHOLD_DAILY_WEIGHT`</literal>`{=html} = 3.0 Weight
given to the daily thresholds
</simpara>
<simpara>
`<literal>`{=html}THRESHOLD_HOURLY_WEIGHT`</literal>`{=html} = 2.0
Weight given to the hourly thresholds
</simpara>
<simpara>
The spread potential thresholds are set using 3 separate random numbers,
scenario, day and hour. These numbers are all drawn independently and
aggregated into a total weight after being multiple against their
weights as established with the THRESHOLD\_\*\*\*\_WEIGHT.
</simpara>
<simpara>
Spread Example 1
</simpara>
<informalfigure>
<mediaobject>
`<imageobject>`{=html}
`<imagedata fileref="../images/Spread_1.png"/>`{=html}
`</imageobject>`{=html} `<textobject>`{=html}`<phrase>`{=html}Spread
Example 1`</phrase>`{=html}`</textobject>`{=html}
</mediaobject>
</informalfigure>
<simpara>
Spread Example 2
</simpara>
<informalfigure>
<mediaobject>
`<imageobject>`{=html}
`<imagedata fileref="../images/Spread_2.png"/>`{=html}
`</imageobject>`{=html} `<textobject>`{=html}`<phrase>`{=html}Spread
Example 2`</phrase>`{=html}`</textobject>`{=html}
</mediaobject>
</informalfigure>
<simpara>
What this looks like in practice is as follows.
</simpara>
<simpara>
Wherever the weighted simulation + daily + total threshold is above the
value required for spread then the fire will spread. With the same
scenario for weather, there can still be different outcomes. For
example, if the scenario produced the sine-wave for the rate of spread,
you can see that for the 2nd scenario's 1st day, the rate of spread is
above the threshold most of the time, and thus it would spread, whereas
the 1st scenario would spread briefly before noon, but otherwise not
spread during that day.
</simpara>
<simpara>
`<literal>`{=html}DEFAULT_PERCENT_CONIFER`</literal>`{=html} = 50
Default M-1/M-2 percent conifer if none specified
</simpara>
<simpara>
`<literal>`{=html}DEFAULT_PERCENT_DEAD_FIR`</literal>`{=html} = 30
Default M-3/M-4 percent dead fir if none specified
</simpara>
<simpara>
`<literal>`{=html}MAXIMUM_TIME`</literal>`{=html} = 0 Maximum amount of
time to take for simulation (seconds) (0 is unlimited)
</simpara>
<simpara>
`<literal>`{=html}INTERIM_OUTPUT_INTERVAL`</literal>`{=html} = 240
Amount of time between generating interim outputs (seconds)
</simpara>
<simpara>
`<literal>`{=html}MAXIMUM_SIMULATIONS`</literal>`{=html} = 10000 Maximum
number of simulations to do (0 is exactly 1 simulation per scenario)
</simpara>
<simpara>
`<literal>`{=html}CONFIDENCE_LEVEL`</literal>`{=html} = 0.1 Maximum
percent change in statistics between runs before results are consider
stable \[0 - 1\]
</simpara>
<simpara>
`<literal>`{=html}INTENSITY_MAX_LOW`</literal>`{=html} = 2000 Intensity
considered to be top of the range (kW/m)
</simpara>
<simpara>
`<literal>`{=html}INTENSITY_MAX_MODERATE`</literal>`{=html} = 4000
Intensity considered to be top of the range (kW/m)
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Setting
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
RASTER_ROOT
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
./generated/grid/100m
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
MINIMUM_ROS
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
0.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
MAX_SPREAD_DISTANCE
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
0.4
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
OUTPUT_DATE_OFFSETS
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
\[1,2,3,7,14\]
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
FUEL_LOOKUP_TABLE
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
./fuel.lut
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
MINIMUM_FFMC
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
65.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
MINIMUM_FFMC_AT_NIGHT
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
85.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
OFFSET_SUNRISE
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
0.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
OFFSET_SUNSET
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
0.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
THRESHOLD_SCENARIO_WEIGHT
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
1.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
THRESHOLD_DAILY_WEIGHT
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
3.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
THRESHOLD_HOURLY_WEIGHT
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
2.0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
DEFAULT_PERCENT_CONIFER
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
50
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
DEFAULT_PERCENT_DEAD_FIR
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
30
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
MAXIMUM_TIME
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
0
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
INTERIM_OUTPUT_INTERVAL
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
240
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
MAXIMUM_SIMULATIONS
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
10000
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
CONFIDENCE_LEVEL
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
0.1
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
INTENSITY_MAX_LOW
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
2000
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
INTENSITY_MAX_MODERATE
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
4000
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="_command_line_arguments">
<title>
Command Line Arguments
</title>
<section xml:id="help">
<title>
-h
</title>
<simpara>
Show help information and usage instructions.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Off
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
<simpara>
`</simpara> </section> <section xml:id="verbosity-increase"> <title>-v</title> <simpara>Increase output verbosity level. Can typically be repeated for more detail. eg. <literal>-vvv</literal></simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>Normal verbosity</simpara></entry> </row> </tbody> </tgroup> </informaltable> </section> <section xml:id="verbosity-decrease"> <title>-q</title> <simpara>Decrease output verbosity level.</simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>Normal verbosity</simpara></entry> </row> </tbody> </tgroup> </informaltable> </section> <section xml:id="arg-ascii"> <title>--ascii</title> <simpara>Save output grids in ASCII (<literal>.asc</literal>) format.</simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>Disabled</simpara></entry> </row> </tbody> </tgroup> </informaltable> </section> <section xml:id="arg-no-tiff"> <title>--no-tiff</title> <simpara>Disable saving output grids in TIFF (<literal>.tif</literal>) format.</simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>TIFF enabled</simpara></entry> </row> </tbody> </tgroup> </informaltable> <simpara>`
</simpara>
</section>
<section xml:id="arg-i">
<title>
-i
</title>
<simpara>
Save individual simulation maps.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Disabled
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-s">
<title>
-s
</title>
<simpara>
Run simulation in synchronous mode.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Asynchronous
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-points">
<title>
--points
</title>
<simpara>
Save simulation point outputs to file.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Disabled
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-no-intensity">
<title>
--no-intensity
</title>
<simpara>
Disable output of fire intensity grids.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Enabled
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-no-probability">
<title>
--no-probability
</title>
<simpara>
Disable output of probability grids.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Enabled
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-occurrence">
<title>
--occurrence
</title>
<simpara>
Enable output of fire occurrence grids.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Disabled
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-sim-area">
<title>
--sim-area
</title>
<simpara>
Output simulation area grids.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Disabled
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-raster-root">
<title>
--raster-root
</title>
<simpara>
Root directory to read rasters from. FireSTARR will read from the
default location without explicit definition.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Path
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Required
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Yes (context dependent)
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
./generated/grid/100m
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-fuel-lut">
<title>
--fuel-lut
</title>
<simpara>
Specify fuel lookup table file.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
File path
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-tz">
<title>
--tz
</title>
<simpara>
Specify UTC offset (hours).
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Integer
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Units
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Hours
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-curing">
<title>
--curing
</title>
<simpara>
Specify static grass curing value.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Numeric
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-force-greenup">
<title>
--force-greenup
</title>
<simpara>
Force green-up conditions for all fires.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-force-no-greenup">
<title>
--force-no-greenup
</title>
<simpara>
Force no green-up conditions for all fires.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
<simpara>
Unresolved directive in index.adoc - include::arguments/log.adoc\[\]
</simpara>
</section>
<section xml:id="arg-wx">
<title>
--wx
</title>
<simpara>
Specify input weather file. This requires the location to and the name
of the file with it's extension.
</simpara>
<simpara>
Whole address:
</simpara>
<simpara>
`<literal>`{=html}E:`\Data`{=tex}`\Burning `{=tex}Information`\Weather`{=tex}.csv`</literal>`{=html}
</simpara>
<simpara>
Relative address from current working directory:
</simpara>
<simpara>
`<literal>`{=html}.`\Data`{=tex}`\Weather`{=tex}.csv`</literal>`{=html}
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
File path
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Required
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Yes
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Manadtory Fields
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
`<literal>`{=html}Date, Time, Temperature, Relative Humidity, Wind
Speed, Wind Direction, Precipitation`</literal>`{=html}
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
<simpara>
\`\`
</simpara>
</section>
<section xml:id="arg-deterministic">
<title>
--deterministic
</title>
<simpara>
Run simulation deterministically (100% probability of spread and
survival).
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Flag
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Default
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Stochastic
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-confidence">
<title>
--confidence
</title>
<simpara>
Specify confidence level used in outputs.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Numeric
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-perim">
<title>
--perim
</title>
<simpara>
Initialize simulation from a fire perimeter.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Input mode
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-size">
<title>
--size
</title>
<simpara>
Initialize simulation using initial fire size.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Numeric
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-ffmc">
<title>
--ffmc
</title>
<simpara>
Set starting Fine Fuel Moisture Code (FFMC).
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Numeric
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-dmc">
<title>
--dmc
</title>
<simpara>
Set starting Duff Moisture Code (DMC).
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Numeric
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-dc">
<title>
--dc
</title>
<simpara>
Set starting Drought Code (DC).
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Numeric
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-apcp-prev">
<title>
--apcp_prev
</title>
<simpara>
Set precipitation from 1200 previous day to start of hourly weather.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Numeric
</simpara>
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
<simpara>
Units
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
mm
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
<section xml:id="arg-output-date-offsets">
<title>
--output_date_offsets
</title>
<simpara>
Override default output date offsets.
</simpara>
<informaltable frame="all" rowsep="1" colsep="1">
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
<tbody>
`<row>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Type
</simpara>
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
<simpara>
Configuration
</simpara>
`</entry>`{=html} `</row>`{=html}
</tbody>
`</tgroup>`{=html}
</informaltable>
</section>
</section>
<section xml:id="license">
<title>
License & Publications
</title>
</section>
<section xml:id="_additional_license_condition">
<title>
Additional License Condition
</title>
<simpara>
All covered works (e.g., copies of this work, derived works) must
include a copy of the file (or an updated version of it) that details
credits for work up to the time of the original open source release.
That file is available [here](./ORIGIN.md).
</simpara>
</section>
<section xml:id="_publications">
<title>
Publications
</title>
<simpara>
FireGUARD is published at the following locations:
</simpara>
<itemizedlist>
`<listitem>`{=html}
<simpara>
\[Weather forecast
model\](`<link xl:href="https://doi.org/10.3390/fire3020016">`{=html}https://doi.org/10.3390/fire3020016`</link>`{=html})
</simpara>
`</listitem>`{=html} `<listitem>`{=html}
<simpara>
Burn probability model (In progress)
</simpara>
`</listitem>`{=html} `<listitem>`{=html}
<simpara>
\[Impact and likelihood-weighted impact
model\](`<link xl:href="https://doi.org/10.1071/WF18189">`{=html}https://doi.org/10.1071/WF18189`</link>`{=html})
</simpara>
`</listitem>`{=html}
</itemizedlist>
</section>
</article>
