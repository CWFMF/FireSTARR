#!/bin/bash
DIR=`dirname $(realpath "$0")`
find -type f -name "firestarr.log" | sort | xargs -I {} grep -li "Fire size at end of day 1" {} | xargs -I {} ${DIR}/calc_time.sh {}
