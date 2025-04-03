FROM debian:bookworm-slim AS minimal-with-user
ARG USERNAME
ARG USER_ID
ENV TMPDIR=/tmp
RUN apt-get update --fix-missing \
  && apt-get install -y locales \
  && sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen \
  && locale-gen \
  && apt-get install -y sudo \
  && groupadd --gid ${USER_ID} ${USERNAME} \
  && useradd --uid ${USER_ID} --gid ${USER_ID} -m ${USERNAME} \
  && chsh --shell /bin/bash ${USERNAME} \
  && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} \
  && chmod 0440 /etc/sudoers.d/${USERNAME} \
  && mkdir -p /appl/firestarr \
  && chown -R ${USERNAME}:${USERNAME} /appl


FROM minimal-with-user AS minimal-with-libs
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  libgeotiff5


FROM minimal-with-libs AS gcc-cmake-with-libs
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  libtiff-dev libgeotiff-dev \
  cmake gcc g++ make


FROM gcc-cmake-with-libs AS firestarr-build
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
WORKDIR /appl/firestarr
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/src/cpp src/cpp/
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/CMakeLists.txt .
USER ${USERNAME}
RUN cmake -S . -B /appl/firestarr/build -D CMAKE_BUILD_TYPE=Release \
  && cmake --build /appl/firestarr/build --config Release --target all -j $(nproc)


FROM minimal-with-libs AS firestarr-base-env
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
USER root
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# make a layer that's the environment for firestarr with scratch to clear out old layers
FROM scratch AS firestarr-base
COPY --from=firestarr-base-env / /

# derive final image from base so updates to binary are small layers
FROM firestarr-base AS firestarr
ARG USERNAME
USER ${USERNAME}
WORKDIR /appl/firestarr
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/fuel.lut /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/settings.ini /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} --from=firestarr-build /appl/firestarr/firestarr /appl/firestarr/
USER ${USERNAME}
RUN echo cd /appl/firestarr >> /home/${USERNAME}/.bashrc


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
USER ${USERNAME}
WORKDIR /appl/firestarr/
# if we make venv with with user then permissions should be correct
# HACK: make sure python folder doesn't break if version changes
# ensure that gdal can be imported at end
RUN python -m venv --system-site-packages /appl/.venv \
  && echo /appl/firestarr/src/py/firestarr > /appl/.venv/lib/`ls -1 /appl/.venv/lib/ | grep python`/site-packages/firestarr.pth \
  && . /appl/.venv/bin/activate \
  && python -c "from osgeo import gdal" \
  && echo source /appl/.venv/bin/activate >> /home/${USERNAME}/.bashrc


FROM py-gdal AS firestarr-app-base
ARG USERNAME
WORKDIR /appl/firestarr/
# if we make venv with with user then permissions should be correct
COPY --chown=${USERNAME}:${USERNAME} .docker/requirements.txt /appl/requirements.txt
USER ${USERNAME}
RUN echo cd /appl/firestarr >> /home/${USERNAME}/.bashrc
# HACK: make sure osgeo/gdal is available before & after installing requirements
RUN python -c "from osgeo import gdal" \
  && . /appl/.venv/bin/activate \
  && python -c "from osgeo import gdal" \
  && python -m pip install -r /appl/requirements.txt \
  && python -c "from osgeo import gdal"


FROM firestarr-app-base AS firestarr-app-gcc
ARG USERNAME
USER root
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  libtiff-dev libgeotiff-dev \
  gdb valgrind libdwarf-dev libelf-dev libdw-dev linux-perf clang-format \
  cmake gcc g++ make
USER ${USERNAME}


FROM firestarr-app-gcc AS firestarr-dev
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
WORKDIR /appl/firestarr/


FROM firestarr-dev AS firestarr-dev-svc
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
# run as root so we can redirect to where docker logs will show progress
COPY .docker/crontab /etc/crontab
USER ${USERNAME}
SHELL ["/bin/bash"]
ENTRYPOINT ["sudo", "cron", "-f"]


FROM firestarr-dev AS firestarr-setup-gis
ARG USERNAME
USER ${USERNAME}
ENV TMPDIR=/tmp
WORKDIR /appl/gis/
RUN echo cd /appl/gis >> /home/${USERNAME}/.bashrc
SHELL ["/bin/bash"]
ENTRYPOINT ["tail", "-f", "/dev/null"]


FROM firestarr-app-base AS firestarr-prod-base-env
ARG USERNAME
USER root
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
USER ${USERNAME}

FROM scratch AS firestarr-prod-base
COPY --from=firestarr-prod-base-env / /

# derive final image from base so updates to binary are small layers
FROM firestarr-prod-base AS firestarr-prod
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
WORKDIR /appl/firestarr
COPY --chown=${USERNAME}:${USERNAME} ./config /appl/
COPY --chown=${USERNAME}:${USERNAME} --from=firestarr-build /appl/firestarr/firestarr .
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/bounds.geojson /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/fuel.lut /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/settings.ini /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/scripts /appl/firestarr/scripts/
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/src/py/firestarr /appl/firestarr/src/py/firestarr/
WORKDIR /appl/firestarr/src/py/cffdrs-ng
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/src/py/cffdrs-ng/NG_FWI.py .
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/src/py/cffdrs-ng/old_cffdrs.py .
COPY --chown=${USERNAME}:${USERNAME} ./firestarr/src/py/cffdrs-ng/util.py .
USER ${USERNAME}

# doesn't delete intermediaries, but maybe smaller between versions?
FROM firestarr-prod AS firestarr-app
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
USER ${USERNAME}
WORKDIR /appl/firestarr
ENTRYPOINT [ "/appl/firestarr/scripts/publish_after.sh" ]

FROM firestarr-app AS firestarr-app-svc
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
ENV TMPDIR=/tmp
# run as root so we can redirect to where docker logs will show progress
COPY .docker/crontab /etc/crontab
RUN sed -i 's/CRONJOB_RUN=.*/CRONJOB_RUN=1/g' /appl/config
WORKDIR /appl/firestarr
USER ${USERNAME}
ENTRYPOINT ["sudo", "cron", "-f"]
