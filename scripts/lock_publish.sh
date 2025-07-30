#!/bin/bash
DIR=`dirname $(realpath "$0")`
export FORCE_RUN=${FORCE_RUN}
export IS_CRONJOB=${IS_CRONJOB}

${DIR}/with_lock_publish.sh /appl/firestarr/scripts/check_and_publish.sh $*
