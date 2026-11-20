# syntax=docker/dockerfile:1.7
#
# Commit image for EasyRPG Player (C++17, CMake). Builds on the org base; adds the apt -dev
# libraries Player needs and liblcf (the RPG Maker data-format library Player requires at the
# same development version; Debian does not package it), built once from a pinned commit into
# /opt/toolchains/deps/liblcf-<commit>/ (SPEC §6.2 item 19).
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends \
        cmake ninja-build pkg-config git ca-certificates \
        libpng-dev zlib1g-dev libfmt-dev libpixman-1-dev libexpat1-dev libicu-dev \
        libinih-dev nlohmann-json3-dev liblhasa-dev libsdl2-dev \
 && rm -rf /var/lib/apt/lists/*

# liblcf, pinned by commit (master at the time of the port; Player master needs liblcf master).
ARG LIBLCF_COMMIT=6854310c3432e553fd4ae672ce861899c80c3bd0
RUN d=/opt/toolchains/deps/liblcf-${LIBLCF_COMMIT} && mkdir -p "$d/src" /tmp/dep-build \
 && git -C "$d/src" init -q && git -C "$d/src" remote add origin https://github.com/EasyRPG/liblcf.git \
 && git -C "$d/src" fetch -q --depth 1 origin ${LIBLCF_COMMIT} \
 && git -C "$d/src" checkout -q FETCH_HEAD && rm -rf "$d/src/.git" \
 && cmake -S "$d/src" -B /tmp/dep-build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_C_FLAGS="-g -gdwarf-3" -DCMAKE_CXX_FLAGS="-g -gdwarf-3" \
      -DBUILD_SHARED_LIBS=OFF -DLIBLCF_ENABLE_TOOLS=OFF -DLIBLCF_ENABLE_TESTS=OFF \
      -DLIBLCF_UPDATE_MIMEDB=OFF -DLIBLCF_ENABLE_INSTALL=ON -DCMAKE_INSTALL_PREFIX="$d" \
 && cmake --build /tmp/dep-build -j"$(nproc)" && cmake --install /tmp/dep-build \
 && ln -sfn "$d" /opt/toolchains/deps/liblcf \
 && rm -rf /tmp/dep-build && chmod -R a+rX "$d"

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
