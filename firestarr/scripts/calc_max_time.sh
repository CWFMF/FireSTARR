#!/bin/bash
DIR=`dirname $(realpath "$0")`
${DIR}/calc_all_times.sh | sort -n | tail -n 1
