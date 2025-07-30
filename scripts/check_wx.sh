#!/bin/bash
. /appl/data/config || . /appl/config

if [ -z "${BOUNDS_LATITUDE_MAX}" ] \
    || [ -z "${BOUNDS_LATITUDE_MIN}" ] \
    || [ -z "${BOUNDS_LONGITUDE_MAX}" ] \
    || [ -z "${BOUNDS_LONGITUDE_MIN}" ] \
    ; then
    echo "Bounds must be set"
else
    LATITUDE=$(($((${BOUNDS_LATITUDE_MAX} + ${BOUNDS_LATITUDE_MIN})) / 2))
    LONGITUDE=$(($((${BOUNDS_LONGITUDE_MAX} + ${BOUNDS_LONGITUDE_MIN})) / 2))
    MODEL=geps
    # specifying member doesn't seem to hurt for other models
    URL_TEST="https://app-cwfmf-api-cwfis-dev.wittyplant-59b495b3.canadacentral.azurecontainerapps.io/gribwx?lat=${LATITUDE}&lon=${LONGITUDE}&model=${MODEL}&timezone=UTC&duration=1&format=csv&precision=1&latest=True&member=1"
    DIR=/appl/data
    CURDATE=`date -u --rfc-3339=seconds`

    source /appl/.venv/bin/activate || echo No venv
    curl -sk "${URL_TEST}"
fi
