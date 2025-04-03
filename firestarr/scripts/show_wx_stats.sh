#!/bin/bash
DIR=`dirname $(realpath "$0")`
${DIR}/calc_wx_stats.sh FFMC
${DIR}/calc_wx_stats.sh DMC
${DIR}/calc_wx_stats.sh DC
