# ignore 32-bit - 386, arm/v5, arm/v7
# amd64, arm/v5, arm64/v8, ppc64le, s390x
ARG BASE_DISTROLESS=gcr.io/distroless/static-debian13
# armv/v5, 386, riscv64, i686
ARG BASE_SLIM=debian:trixie-slim

FROM --platform=linux/amd64 ${BASE_DISTROLESS} AS distroless-amd64

# seems like we can just delete everything and then copy distroless into any platform?
FROM ${BASE_SLIM} AS base-minimal
WORKDIR /appl/firestarr
RUN mkdir -p /appl/firestarr/grid
# HACK: mkdir doesn't exist when copied into scratch but COPY should work
WORKDIR /
RUN rm -rf $(ls -1 / | grep -vE "appl|dev|etc|proc|sys")
COPY --from=distroless-amd64 / /

# make a layer with scratch to clear out old layers
FROM ${BASE_SLIM} AS base-cleaned
WORKDIR /appl/firestarr
RUN mkdir -p /appl/firestarr/grid
RUN apt-get clean \
  && rm -rf \
    /tmp/* \
    /usr/share/doc/* \
    /var/cache/* \
    /var/lib/apt/lists/* \
    /var/lib/dpkg/* \
    /var/tmp/*

# basic image to start from
FROM debian:trixie-slim AS gcc-cmake
ARG TARGETARCH
ARG TARGETVARIANT
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  cmake gcc g++ make ninja-build pkg-config \
  curl zip unzip tar ca-certificates git
WORKDIR /appl/firestarr
# HACK: show what gcc wants to use for debugging purposes
RUN (export \
  && ( \
      g++ -march=native -Q --help=target \
      || g++ -mcpu=native -Q --help=target \
      || echo "Can't use arch or cpu" \
    ) \
  ) &> /tmp/march.log
RUN (export && (g++ -mtune=native -Q --help=target || echo "Can't use -mtune=native")) &> /tmp/mtune.log

# clone/bootstrap for vcpkg doesn't depend on what we're going to install
FROM gcc-cmake AS vcpkg-bootstrap
# vcpkg says: Environment variable VCPKG_FORCE_SYSTEM_BINARIES must be set on arm, s390x, ppc64le and riscv platforms.
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/appl/firestarr/vcpkg
ARG TARGETARCH
WORKDIR /appl/firestarr
# copy so we know correct version is bootstrapped when files change
COPY ./vcpkg-configuration.json .
# --branch doesn't work with sha?
# HACK: uname doesn't work on 386 so override with ARCH=x86
RUN git clone -v https://github.com/microsoft/vcpkg.git \
  && cd vcpkg \
  && git checkout $(sed -n '/baseline/{s/.*\"\([^"]*\)",$/\1/g;p}' ../vcpkg-configuration.json) \
  && cd .. \
  && ( \
    ( \
      [ "386" = "$TARGETARCH" ] \
        && echo "Using ARCH=x86 instead of detecting" \
        && sed -i 's/ARCH=.*/ARCH=x86/' vcpkg/scripts/bootstrap.sh \
    ) \
    || echo "Detecting ARCH" \
  ) \
  && vcpkg/bootstrap-vcpkg.sh -disableMetrics

# install packages so they stay if nothing changes with vcpkg*.json
FROM vcpkg-bootstrap AS vcpkg-installed
WORKDIR /appl/firestarr
RUN --mount=type=bind,source=./vcpkg.json,target=./vcpkg.json \
  vcpkg/vcpkg install

# build and test
FROM vcpkg-installed AS firestarr-build
WORKDIR /appl/firestarr
# NOTE: .env needs to be rw here since cmake needs to write to it in github workflow
# mount instead of copy to minimize layers
RUN --mount=type=bind,source=./vcpkg.json,target=./vcpkg.json \
    --mount=type=bind,rw=true,source=./.env,target=./.env \
    --mount=type=bind,source=./cmake,target=./cmake \
    --mount=type=bind,source=./CMakeLists.txt,target=./CMakeLists.txt \
    --mount=type=bind,source=./CMakePresets.json,target=./CMakePresets.json \
    --mount=type=bind,source=./.clang-format,target=./.clang-format \
    --mount=type=bind,source=./.clang-tidy,target=./.clang-tidy \
    --mount=type=bind,source=./.git,target=./.git \
    --mount=type=bind,source=./src/cpp,target=./src/cpp \
    --mount=type=bind,source=./test/data,target=./test/data \
    --mount=type=bind,source=./test/input,target=./test/input \
    --mount=type=bind,rw=true,source=./test/output,target=./test/output \
    --mount=type=bind,source=./fuel.lut,target=./fuel.lut \
    --mount=type=bind,source=./settings.ini,target=./settings.ini \
    --mount=type=bind,source=./README.md,target=./README.md \
    --mount=type=bind,source=./ORIGIN.md,target=./ORIGIN.md \
    --mount=type=bind,source=./LICENSE,target=./LICENSE \
    cmake --preset Release \
    && cmake --build --parallel --preset Release \
    && ctest --parallel --preset Release \
    && ./firestarr -h

# make a layer with scratch to clear out old layers
FROM scratch AS firestarr-minimal
COPY --from=base-minimal / /
WORKDIR /appl/firestarr
COPY --from=firestarr-build /appl/firestarr/firestarr .
COPY --from=firestarr-build /appl/firestarr/proj.db ./proj.db
COPY ./fuel.lut .
COPY ./settings.ini .
ENTRYPOINT [ "/appl/firestarr/firestarr" ]

# derive final image from base so updates to binary are small layers
FROM scratch AS firestarr
COPY --from=base-cleaned / /
WORKDIR /appl/firestarr
COPY --from=firestarr-minimal /appl/firestarr/firestarr .
COPY --from=firestarr-minimal /appl/firestarr/proj.db ./proj.db
COPY ./fuel.lut .
COPY ./settings.ini .
ENTRYPOINT [ "/appl/firestarr/firestarr" ]
