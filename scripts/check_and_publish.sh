#!/bin/bash
DIR=/appl/firestarr
pushd ${DIR}
CURDATE=`date -u --rfc-3339=seconds`
echo "${CURDATE}: Running check and publish"
. /appl/data/config || . /appl/config

if [ "" != "${IS_CRONJOB}" ];
then
    if [ -z "${CRONJOB_RUN}" ];
    then
        echo "${CURDATE}: Not running $0 since CRONJOB_RUN is not set"
        exit
    fi
fi


source /appl/.venv/bin/activate || echo No venv
# HACK: use python3 and not python so killall doesn't affect this
python3 ${DIR}/src/py/firestarr/check_and_publish.py $* || (echo FAILED)
echo `date -u --rfc-3339=seconds`: Done running check and publish
popd
