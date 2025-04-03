#!/bin/bash
DIR=`dirname $(realpath "$0")`
${DIR}/force_run.sh $* \
    && ${DIR}/lock_publish.sh
