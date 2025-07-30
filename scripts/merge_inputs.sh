#!/bin/bash
DIR=`dirname $(realpath "$0")`
export FORCE_RUN=1
# export IS_CRONJOB=${IS_CRONJOB}
# HACK: clear for now
export IS_CRONJOB=
# use update.sh so we can lock entire script on `/usr/bin/flock -n /appl/data/update.lock`

DIR_SIMS="/appl/data/sims"
DIR_RUNS="/appl/data/runs"
SUBDIR_COMMON="current"
set -e

echo "Checking that previous run completed and published successfully"
# NOTE: don't use --resume because if it's old weather it needs to run from scratch
(${DIR}/update.sh --no-publish --no-merge --no-retry --resume && ${DIR}/check_and_publish.sh)
RESULT=$?
if [ 0 -ne "${RESULT}" ]; then
 echo "Previous run didn't finish properly"
 exit ${RESULT}
fi

LAST_RUN=`ls -1 ${DIR_SIMS} | sort | grep -v "${SUBDIR_COMMON}" | tail -n 1`
dir_last_sims="${DIR_SIMS}/${LAST_RUN}"
dir_last_runs="${DIR_RUNS}/${LAST_RUN}"

# since previous run finished fine we can do this
echo "Preparing new run to merge into"
(${DIR}/update.sh --prepare-only --no-resume --no-retry)
# delay this so we can delete if failed
RESULT=$?

# do this instead of || so we can copy the whole line above when debugging
# same as above since we just added another directory to the end of the list
CUR_RUN=`ls -1 ${DIR_SIMS} | sort | grep -v "${SUBDIR_COMMON}" | tail -n 1`
dir_cur_sims="${DIR_SIMS}/${CUR_RUN}"
dir_cur_runs="${DIR_RUNS}/${CUR_RUN}"

if [ 0 -ne "${RESULT}" ]; then
 if [ "${CUR_RUN}" != "${LAST_RUN}" ]; then
  echo "Deleting failed run ${CUR_RUN}"
  rm -rf "${dir_cur_sims}" "${dir_cur_runs}"
 fi
 echo "Couldn't create new run to merge into"
 exit ${RESULT}
fi

echo "Merging ${LAST_RUN} into ${CUR_RUN}"
N=`ls -1 "${dir_cur_sims}" | grep -v "model" | wc -l`
# use separate script so we can use xargs to do this
merge_results=`diff -q ${dir_last_sims} ${dir_cur_sims} \
    | grep -v "model" \
    | grep "Common subdirectories" \
    | sed "s/.*\/\([^ ]*\) and.*/\1/g" \
    | xargs -P $(nproc) -I {} ${DIR}/merge_folders.sh "${dir_last_sims}" "${dir_cur_sims}" "{}"`
RESULT=$?
if [ 0 -ne "${RESULT}" ]; then
 echo "Merging failed:"
 echo -n "${merge_results}"
 exit ${RESULT}
fi
# HACK: 0 needs to be there to determine number of copied if nothing copied since blank in that case
copied=`cat <(echo -n "${merge_results}") <(echo 0) \
    | paste -sd+ - \
    | bc`
RESULT=$?
if [ 0 -ne "${RESULT}" ]; then
 echo "Failed to determine number of copied directories based on output:"
 echo -n "${merge_results}"
 exit ${RESULT}
fi
echo "Copied outputs from ${copied} / ${N} directories"

if [ "${copied}" -eq "0" ]; then
    # FIX: kind of dumb to copy everything if it's the same and then delete but simpler for now
    echo "All inputs are the same so no reason to run anything or keep this version"
    echo "Deleting ${CUR_RUN}"
    rm -rf "${dir_cur_sims}" "${dir_cur_runs}"
    echo "${LAST_RUN} was already up-to-date so deleted ${CUR_RUN}"
else
    echo "Running merged run in ${CUR_RUN}"
    # update symlinks for current folders
    dir_common_sims="${DIR_SIMS}/${SUBDIR_COMMON}"
    dir_common_runs="${DIR_RUNS}/${SUBDIR_COMMON}"
    ln -sfn "`realpath ${dir_cur_sims} --relative-to=${DIR_SIMS}`" "${dir_common_sims}"
    ln -sfn "`realpath ${dir_cur_runs} --relative-to=${DIR_RUNS}`" "${dir_common_runs}"
    # if we call pointing at current directory it should make tasks there instead of using actual folder name
    (${DIR}/update.sh --no-publish --no-merge --no-retry "${dir_common_runs}")
    RESULT=$?
    if [ 0 -ne "${RESULT}" ]; then
        echo "Merged run didn't finish properly"
        exit ${RESULT}
    fi
    echo "Validating merged run in ${CUR_RUN}"
    (${DIR}/update.sh --no-publish --no-merge --no-retry "${dir_common_runs}")
    RESULT=$?
    if [ 0 -ne "${RESULT}" ]; then
        echo "Validating merged run"
        exit ${RESULT}
    fi
    echo "Publishing merged run in ${CUR_RUN}"
    (${DIR}/check_and_publish.sh)
    RESULT=$?
    if [ 0 -ne "${RESULT}" ]; then
        echo "Couldn't publish after finished"
        exit ${RESULT}
    fi
fi
