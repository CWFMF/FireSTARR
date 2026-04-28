mkdir -p data
mkdir -p data/sims
mkdir -p data/download
pushd data/download/
wget -c https://fgmfiles.spyd.com/datasets/FireSTARR_Dataset_2025_V1.1.zip
popd
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
docker compose run -it --entrypoint /bin/bash firestarr-app-dev -c 'cppscripts/build.sh'
docker compose run -it --entrypoint /bin/bash firestarr-app-dev -c 'python src/py/firestarr/make_bounds.geojson'
