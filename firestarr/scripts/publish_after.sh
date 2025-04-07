#!/bin/bash
DIR=`dirname $(realpath "$0")`
${DIR}/archive_sims.sh \
    && ${DIR}/force_run.sh --queue $*
