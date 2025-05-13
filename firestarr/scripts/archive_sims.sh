#!/bin/bash
# NOTE: this merges sims and runs directories for this run into root of archive
DIR_FROM_SIMS="/appl/data/sims"
DIR_FROM_RUNS="/appl/data/runs"
DIR_BKUP="/appl/data/sims.bkup"
DIR_TMP="${TMPDIR}/bkup"
SUBDIR_COMMON="current"

# override KEEP_UNARCHIVED if set in config
. /appl/data/config || . /appl/config

# ensure 7za exists
7za > /dev/null || (echo "7za not found" && exit -1)

function do_archive()
{
  run="$1"
  # # update without existing file is the same as 'a'
  # zip_type="u"
  # '-sdel' doesn't work with 'u'
  zip_type="a"
  # always -sdel since archiving from tmp folder
  options="-mtm -mtc -mx=9 -stl -sdel"
  file_out="${DIR_BKUP}/${run}.7z"
  dir_sims="${DIR_FROM_SIMS}/${run}"
  dir_runs="${DIR_FROM_RUNS}/${run}"
  echo "Archiving ${run} as ${file_out}"
  # if the folders still exist then don't try to figure out what files are newer
  if [ -f "${file_out}" ]; then
    echo "Updating ${file_out}"
    echo "Checking existing archive ${file_out}"
    # if 7z can't open the archive then we need to get rid of it
    7za t "${file_out}" || (echo "Removing invalid file ${file_out}" && rm "${file_out}")
  fi
  # HACK: merge directories before zipping
  echo "Archiving and deleting ${run} as ${file_out}"
  # do sims and runs separately because it sees them as dupes if not
  # HACK: ensure both directories exist if only one did
  mkdir -p "${dir_sims}"
  mkdir -p "${dir_runs}"
  7za ${zip_type} ${options} "${file_out}" "${dir_sims}/*" \
      && rmdir "${dir_sims}" \
      && 7za ${zip_type} ${options} "${file_out}" "${dir_runs}/*" \
      && rmdir "${dir_runs}"
  RESULT=$?
  if [ 0 -ne "${RESULT}" ]; then
    echo "Failed to archive ${run}"
  fi
}

pushd ${DIR_FROM_RUNS}
# get rid of bkup folder in case old junk is in there
# rm -rf ${DIR_BKUP} && \
mkdir -p ${DIR_BKUP}
rmdir * > /dev/null 2>&1
set -e
match_last=`ls -1 | grep -v "${SUBDIR_COMMON}" | tail -n1 | sed "s/.*\([0-9]\{8\}\)[0-9]\{4\}/\1/"`
echo "Archiving everything except ${match_last}"
# also filter out anything for today since might be symlinking to it
for run in `ls -1  | grep -v "${SUBDIR_COMMON}" | grep -v "${match_last}" | head -n-${KEEP_UNARCHIVED}`
do
  echo "${run}"
  do_archive "${run}"
done
popd
