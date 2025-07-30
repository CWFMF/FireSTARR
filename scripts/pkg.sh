#!/bin/bash
if [ -z "${VERSION}" ]; then
    echo VERSION not set so not packaging
else
    DIR=dist/${VERSION}
    cd /appl/firestarr
    mkdir -p ${DIR}
    cp firestarr ${DIR}/
    cp fuel.lut ${DIR}/
    cp settings.ini ${DIR}/
    pushd ${DIR}
    zip ../${VERSION}.zip *
fi
