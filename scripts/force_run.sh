#!/bin/bash
DIR=`dirname $(realpath "$0")`
. /appl/data/config || . /appl/config
source /appl/.venv/bin/activate || echo No venv
cd /appl/firestarr
${DIR}/with_lock_update.sh python /appl/firestarr/src/py/firestarr/main.py $*
