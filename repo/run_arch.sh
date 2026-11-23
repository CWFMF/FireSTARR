#!/bin/bash
set -e
. ./.env
# PLATFORMS="amd64 arm/v5 arm/v7 arm64/v8 i386 ppc64le riscv64 s390x"
PLATFORMS="arm64/v8"
# echo PLATFORMS = ${PLATFORMS}
for p in ${PLATFORMS}
do
  arch="${p//\//}"
  echo "Running ${arch}"
  # p=arm/v7
  platform="linux/${p}"
  target=firestarr
  h=$(git rev-parse --short --verify HEAD)
  v="v${VERSION}_${h}"
  id="${arch}_${v}"
  dir_out="data/test/arch/${id}"
  mkdir -p "${dir_out}"
  (docker build --progress=plain --platform=${platform} --target ${target} -t ${target}:${v} -f .docker/Dockerfile . \
  && docker run -v./test:/appl/firestarr/test -v./data:/appl/data -it --rm --platform=${platform} ${target}:${v} "/appl/${dir_out}" 2024-06-03 58.81228184403946 -122.9117103995713 01:00 --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --wx /appl/firestarr/test/input/10N_50651/firestarr_10N_50651_wx.csv --output_date_offsets [1] --tz -5 --raster-root /appl/firestarr/test/input/10N_50651/ --perim /appl/firestarr/test/input/10N_50651/10N_50651.tif \
  && echo "Finished running ${arch}") \
  || echo "Failed to run ${arch}"
done
