#!/bin/sh

set -eu

cd "$WORK_DIR"

apk add \
    g++ \
    make \
    cmake \
    gdal-dev \
    proj-dev \
    geos-dev \
    curl-dev \
    fcgi-dev \
    libxml2-dev \
    protobuf-c-dev \
    freetype-dev \
    fribidi-dev \
    harfbuzz-dev \
    exempi-dev \
    giflib-dev \
    libpq-dev \
    python3-dev \
    swig

mkdir build
cd build

cmake .. \
    -DCMAKE_CXX_FLAGS="-Werror" \
    -DCMAKE_C_FLAGS="-Werror" \
    -DWITH_CLIENT_WFS=1 -DWITH_CLIENT_WMS=1 -DWITH_KML=1 -DWITH_SOS=1 \
    -DWITH_PYTHON=1 -DWITH_THREAD_SAFETY=1 -DWITH_FRIBIDI=1 -DWITH_FCGI=1 -DWITH_EXEMPI=1 \
    -DCMAKE_BUILD_TYPE=Release -DWITH_CURL=1 -DWITH_HARFBUZZ=1

make -j$(nproc)
