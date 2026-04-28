RASTERS=FireSTARR_Dataset_2025_V1.1.zip
# get rasters
mkdir -p data
mkdir -p data/sims
mkdir -p data/download
pushd data
pushd download
wget -c https://fgmfiles.spyd.com/datasets/${RASTERS}
RASTERS=$(pwd)/${RASTERS}
popd
mkdir -p generated/grid/100m/default
pushd generated/grid/100m/default
# HACK: can't figure out how to use regex for extract files
d=$(7za l "${RASTERS}" | grep default | head -n1 | sed "s/.* \([^ ]*\/default.*\)/\1/")
7za e "${RASTERS}" "$d"
# unzip makes an empty directory
rmdir default
popd
popd
# build containers
pushd firestarr
docker compose build firestarr
docker compose build firestarr-dev
# HACK: make image for building c++ available for firestarr-app dockerfile
for image in vcpkg-installed firestarr-build
do
  docker build -f .docker/Dockerfile -t $image --target $image .
done
popd
docker compose build firestarr-app-dev
# build with this since it's mounting the ./firestarr directory at /appl/firestarr
cp bounds.geojson firestarr/
cp bounds.geojson data/
docker compose run -it --entrypoint /bin/bash firestarr-app-dev -c 'cppscripts/build.sh'
docker compose run -it --entrypoint /bin/bash firestarr-app-dev -c 'cd /appl/firestarr/; source ../.venv/bin/activate; python src/py/firestarr/make_bounds.py'
