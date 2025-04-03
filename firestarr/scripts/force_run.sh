#!/bin/bash
DIR=`dirname $(realpath "$0")`
export FORCE_RUN=1
export IS_CRONJOB=${IS_CRONJOB}
${DIR}/with_lock_update.sh ${DIR}/update.sh $*
