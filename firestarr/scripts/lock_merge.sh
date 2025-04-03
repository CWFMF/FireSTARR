#!/bin/bash
DIR=`dirname $(realpath "$0")`
export FORCE_RUN=${FORCE_RUN}
export IS_CRONJOB=${IS_CRONJOB}

# try without lock_publish for now
# ${DIR}/with_lock_publish.sh \
    ${DIR}/with_lock_update.sh \
    /appl/firestarr/scripts/merge_inputs.sh $*
