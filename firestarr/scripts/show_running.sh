#!/bin/bash
# show which simulations are running in what folders
NUM_LINES=10
PROCESSES=`ps aux | grep -v grep | grep firestarr/firestarr | awk '{ print $2; }'`
echo ${PROCESSES}
for p in ${PROCESSES}; do
    folder=`pwdx ${p} | awk '{ print $2 }'`
    echo -e "Running in folder ${folder}"
    ps aux | grep -v grep | grep firestarr/firestarr | grep ${p}
    if [ "$1" == "-v" ]; then
        echo -e ""
        cat ${folder}/firestarr.log | tail -n ${NUM_LINES}
    fi
    echo -e ""
done
