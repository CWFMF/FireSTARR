#!/bin/bash
old="$1"
new="$2"
d="$3"

if [ -z "${old}" ]; then
    echo "Missing previous simulation directory"
    exit -1
fi
if [ -z "${new}" ]; then
    echo "Missing current simulation directory"
    exit -1
fi
if [ -z "${d}" ]; then
    # use old and new directly
    d=`basename ${old}`
    d_new=`basename ${new}`
    if [ "${d}" != "${d_new}" ]; then
        echo "Excepted the same basename but got ${d} and ${d_new}"
        exit -1
    fi
else
    # append directory to base directories
    old="${old}/${d}"
    new="${new}/${d}"
fi
# FIX: if this is for parallelizing then need to do a single folder and return 1 or 0?

dir_cur_sims=`dirname ${new}`
dir_last_sims=`dirname ${old}`
is_different=
# echo "Checking ${d}"
# if one of these sim.sh files doesn't exist then shouldn't copy
if [ -f "${old}/sim.sh" ] && [ -f "${new}/sim.sh" ]; then
    if [ "$(realpath ${old})" != "$(realpath ${new})" ]; then
        # check that sim conditions, input tif and weather are the same
        # only care about simulation call line being the same so we can tweak script otherwise and still see as the same
        # not sure how to chain these without continue
        (diff <(grep firestarr ${old}/sim.sh) <(grep firestarr ${new}/sim.sh) > /dev/null) \
            && ((diff -rqs ${old} ${new} | grep -v ".lock" | grep identical | grep ${d}/${d}.tif) > /dev/null) \
            && ((diff -rqs ${old} ${new} | grep -v ".lock" | grep identical | grep ${d}/firestarr_${d}_wx.csv) > /dev/null)
        # if last return succeeded then no differences
        if [ "0" -eq "$?" ]; then
            # echo "Results for ${d} are from same inputs so reusing"
            # instead of copying just use symlinks
            # use relative symlink so path should be good in azure still
            rm -rf "${new}" && ln -s `realpath "${old}" --relative-to="${dir_cur_sims}"` "${new}"
        else
            # check if files already exist in new directory (somehow - maybe ran already in interim?)
            ls -1 ${new}/*probability*.tif > /dev/null 2>&1
            if [ "0" -ne "$?" ]; then
                # something has changed but we don't want to throw out the old results before we have new ones
                # echo "Moving old results into new folder"
                # just worry about rasters since using these as interim results
                for r in `ls -1 ${old}/*probability*.tif`; do
                    r_new=`basename "${r}" | sed "s/^.*probability_/interim_probability_/g"`
                    cp "${r}" "${new}/${r_new}"
                done
            # else
            #     echo "Rasters already exist in new directory so not copying"
            fi
            is_different=1
        fi
    # else
    #     # NOTE: this should never get reached but leaving it for clarity about what's going on
    #     echo "Results for ${d} are already reused"
    fi
else
    is_different=1
fi
# have a final message so calling scripts can check result from output
if [ ! -z "${is_different}" ] ; then
    # echo "Folders are different"
    echo ${is_different}
# else
#     echo "Folders are not different"
fi
