#!/bin/bash
DIR=`dirname $(realpath "$0")`
# use file for completed runs and running processes instead of files for everything
${DIR}/calc_done_times.sh | cat - <(${DIR}/calc_running_times.sh)
