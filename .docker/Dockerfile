FROM debian:trixie-slim AS minimal-with-libs
ENV TMPDIR=/tmp
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  libgeotiff5
RUN mkdir -p /appl/firestarr


# saves more work to reuse python than it does to reuse gcc
FROM minimal-with-libs AS py-gdal
ARG USERNAME
USER root
# ensure that gdal can be imported at end
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  cron curl nano procps psmisc tar util-linux unzip wget zip p7zip-full rsync \
  time nano bc ministat sysstat screen \
  python3 gdal-bin python3-gdal python3-setuptools python3-pip python3-venv \
  && update-alternatives --install /usr/bin/python python /usr/bin/python3 10 \
  && python -c "from osgeo import gdal"
WORKDIR /appl/firestarr/
# if we make venv with with user then permissions should be correct
# HACK: make sure python folder doesn't break if version changes
# ensure that gdal can be imported at end
RUN python -m venv --system-site-packages /appl/.venv \
  && echo /appl/firestarr/src/py/firestarr > /appl/.venv/lib/`ls -1 /appl/.venv/lib/ | grep python`/site-packages/firestarr.pth \
  && . /appl/.venv/bin/activate \
  && python -c "from osgeo import gdal" \
  && echo source /appl/.venv/bin/activate >> /root/.bashrc


FROM py-gdal AS firestarr-app-base
WORKDIR /appl/firestarr/
# if we make venv with with user then permissions should be correct
COPY .docker/requirements.txt /appl/requirements.txt
RUN echo cd /appl/firestarr >> /root/.bashrc
# HACK: make sure osgeo/gdal is available before & after installing requirements
RUN python -c "from osgeo import gdal" \
  && . /appl/.venv/bin/activate \
  && python -c "from osgeo import gdal" \
  && python -m pip install -r /appl/requirements.txt \
  && python -c "from osgeo import gdal"


FROM firestarr-app-base AS firestarr-app-gcc
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  libtiff-dev libgeotiff-dev \
  gdb valgrind libdwarf-dev libelf-dev libdw-dev linux-perf clang-format \
  cmake gcc g++ make


FROM firestarr-app-gcc AS firestarr-app-dev
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
WORKDIR /appl/firestarr/
COPY bounds.geojson /appl/firestarr/
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  cmake gcc g++ make ninja-build pkg-config \
  curl zip unzip tar ca-certificates git
# vcpkg says: Environment variable VCPKG_FORCE_SYSTEM_BINARIES must be set on arm, s390x, ppc64le and riscv platforms.
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/appl/firestarr/vcpkg
WORKDIR /appl
# don't actually build vcpkg since we don't want it outside the mounted directory at
#   /appl/firestarr when using this for dev


FROM firestarr-app-dev AS firestarr-setup-gis
ENV TMPDIR=/tmp
WORKDIR /appl/gis/
RUN echo cd /appl/gis >> /root/.bashrc


FROM firestarr-app-base AS firestarr-prod-base-env
USER root
RUN apt-get clean \
  && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* /home/user/.cache
RUN find -type d -name __pycache__ / | xargs -tI {} rm -rf {}

FROM scratch AS firestarr-prod-base
COPY --from=firestarr-prod-base-env / /

# derive final image from base so updates to binary are small layers
FROM firestarr-prod-base AS firestarr-prod
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
WORKDIR /appl/firestarr
COPY ./config /appl/
COPY --from=firestarr-build /appl/firestarr/firestarr .
COPY ./bounds.geojson /appl/
COPY ./firestarr/fuel.lut /appl/firestarr/
COPY ./firestarr/settings.ini /appl/firestarr/
COPY ./scripts/ /appl/firestarr/scripts/
COPY ./src/py/firestarr/ /appl/firestarr/src/py/firestarr/
WORKDIR /appl/firestarr/src/py/cffdrs-ng
COPY ./src/py/cffdrs-ng/NG_FWI.py .
COPY ./src/py/cffdrs-ng/old_cffdrs.py .
COPY ./src/py/cffdrs-ng/util.py .

# doesn't delete intermediaries, but maybe smaller between versions?
FROM firestarr-prod AS firestarr-app
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
WORKDIR /appl/firestarr
ENTRYPOINT [ "/appl/firestarr/scripts/publish_after.sh" ]
