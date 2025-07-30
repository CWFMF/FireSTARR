#/bin/bash
DIR=`dirname $(realpath "$0")`
. /appl/data/config || . /appl/config
echo "Publishing to geoserver ${GEOSERVER_SERVER}"

if [ -z "${GEOSERVER_COVERAGE}" ] \
    || [ -z "${GEOSERVER_LAYER}" ] \
    || [ -z "${GEOSERVER_CREDENTIALS}" ] \
    || [ -z "${GEOSERVER_SERVER}" ] \
    || [ -z "${GEOSERVER_WORKSPACE_NAME}" ] \
    || [ -z "${GEOSERVER_DIR_ROOT}" ] \
    || [ -z "${AZURE_DIR_DATA}" ] \
    ; then
    echo Missing required configuration so not publishing
else
    RUN_ID="$1"
    if [ -z "${RUN_ID}" ]; then
        RUN_ID=`ls -1 /appl/data/sims/ | tail -1`
    fi
    # RESOURCE_PREFIX is empty or 'test_'
    COVERAGE="${RESOURCE_PREFIX}${GEOSERVER_COVERAGE}"
    LAYER="${RESOURCE_PREFIX}${GEOSERVER_LAYER}"
    DIR_DATA="${RESOURCE_PREFIX}${AZURE_DIR_DATA}"

    GEOSERVER_DIR_DATA="${GEOSERVER_DIR_ROOT}/${DIR_DATA}/${RUN_ID}"
    if [ -z "${TMPDIR}" ]; then
        TMPDIR=/tmp
    fi
    GEOSERVER_EXTENSION=imagemosaic
    GEOSERVER_WORKSPACE=${GEOSERVER_SERVER}/workspaces/${GEOSERVER_WORKSPACE_NAME}
    GEOSERVER_STORE=${GEOSERVER_WORKSPACE}/coveragestores/${COVERAGE}
    TMP_LAYER=${TMPDIR}/${LAYER}.xml
    TAG=abstract
    echo "Publishing to ${GEOSERVER_STORE}"

    # HACK: get rid of granules for interim fires in case they've finished and the files no longer exist
    curl -v -v -sS -u "${GEOSERVER_CREDENTIALS}" -XDELETE "${GEOSERVER_STORE}/coverages/${LAYER}/index/granules.xml?filter=location%20like%27%__tmp__%%27"

    # update to match azure mount
    curl -v -u "${GEOSERVER_CREDENTIALS}" -XPOST -H "Content-type: text/plain" --write-out %{http_code} -d "${GEOSERVER_DIR_DATA}" "${GEOSERVER_STORE}/external.${GEOSERVER_EXTENSION}"

    # get rid of old granules
    curl -v -v -sS -u "${GEOSERVER_CREDENTIALS}" -XDELETE "${GEOSERVER_STORE}/coverages/${LAYER}/index/granules.xml?filter=location%20not%20like%27${GEOSERVER_DIR_DATA}%%27"

    # extract timestamp from RUN_ID
    RUN_TIME=`echo ${RUN_ID} | sed "s/.*_\([0-9]*\).*/\1/g"`
    ABSTRACT="FireSTARR run from ${RUN_TIME}"
    # replace tag
    curl -v -v -sS -u "${GEOSERVER_CREDENTIALS}" -XGET "${GEOSERVER_STORE}/coverages/${LAYER}" > ${TMP_LAYER}
    TAG_UPDATED="<${TAG}>${ABSTRACT}<\/${TAG}>"
    # if no tag then insert it after title
    (grep "<${TAG}>" ${TMP_LAYER} > /dev/null && sed -i "s/<${TAG}>[^<]*<\/${TAG}>/${TAG_UPDATED}/g" ${TMP_LAYER}) || (sed -i "s/\( *\)\(<title>.*\)/\1\2\n\1${TAG_UPDATED}/g" ${TMP_LAYER})
    # upload with updated tag
    curl -v -u "${GEOSERVER_CREDENTIALS}" -XPUT -H "Content-type: text/xml" -d @${TMP_LAYER} "${GEOSERVER_STORE}/coverages/${LAYER}"?calculate=nativebbox,latlonbbox,dimensions
    # not sure why this isn't picking up .tif band description
    sed -i "s/GRAY_INDEX/probability/g" ${TMP_LAYER}
    # HACK: calculate sets band name to GRAY_INDEX so set again without calculate
    curl -v -u "${GEOSERVER_CREDENTIALS}" -XPUT -H "Content-type: text/xml" -d @${TMP_LAYER} "${GEOSERVER_STORE}/coverages/${LAYER}"
fi
