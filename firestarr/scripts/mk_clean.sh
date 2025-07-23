#!/bin/bash
# didn't figure out how to do this with cmake yet but this works for now
DIR_BUILD=/appl/firestarr/build
VARIANT="${1}"
ASAN_ARGS=
if ( [ "Debug" == "${VARIANT}" ] || [ "Test" == "${VARIANT}" ] ); then
  ASAN_ARGS=-DASAN_ARGS:STRING="-fsanitize=address;-fsanitize=undefined"
fi
if [ "Test" == "${VARIANT}" ]; then
  VARIANT="Release"
fi
if ( [ -z "${VARIANT}" ] || ( [ "Release" != "${VARIANT}" ] && [ "Debug" != ${VARIANT} ] ) ); then
  # assume that argument is an arg to pass to cmake
  VARIANT="Release"
  echo "${@}"
  # don't shift because $1 wasn't the variant
else
  shift;
fi
echo "${@}"
echo Set VARIANT=${VARIANT}
echo Set ASAN_ARGS=${ASAN_ARGS}
rm -rf ${DIR_BUILD} \
  && /usr/bin/cmake --no-warn-unused-cli ${ASAN_ARGS} "${@}" -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S/appl/firestarr -B${DIR_BUILD} -G "Unix Makefiles" \
  && /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 --
