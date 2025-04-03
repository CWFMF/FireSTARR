#!/bin/bash
set -e

DIR=`dirname $(realpath "$0")`
pushd ${DIR}

pushd /appl/firestarr
DIR_BUILD="${DIR}/build"
VARIANT=Debug
echo Set VARIANT=${VARIANT}
# rm -rf ${DIR_BUILD} \
#   &&
  /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S/appl/firestarr -B${DIR_BUILD} -G "Unix Makefiles" \
  && /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 --
popd

DIR_OUT="./output"
rm -rf ${DIR_OUT}

./firestarr "${DIR_OUT}" \
    2023-08-03 \
    60.387 \
    -116.272 \
    01:00 \
    -i \
    --ffmc 86.0 \
    --dmc 118.4 \
    --dc 826.1 \
    --apcp_prev 0 \
    -v \
    --deterministic \
    --output_date_offsets "[1]" \
    --wx "wx.csv"


DIR_ORIG="${DIR_OUT}.orig"
if [ -d ${DIR_ORIG} ]; then
diff -rq ${DIR_OUT} ${DIR_ORIG}
else
echo "Storing outputs as original outputs since those don't exist yet"
cp -r ${DIR_OUT}/ ${DIR_ORIG}
fi

popd
