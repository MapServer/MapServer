#!/bin/bash

#fail script on first error
set -e

#do some cleaning up
git clean -d -f
cd msautotest
rm -f */result/*
git clean -d -f
cd ..
git submodule update

if [ -f CMakeLists.txt ]; then
   #cmake build
   if [ -d build ]; then
      rm -rf build
   fi
   mkdir -p build
   cd build
   cmake .. -DCMAKE_C_FLAGS="--coverage" -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_SHARED_LINKER_FLAGS="-lgcov" -DWITH_GD=1 -DWITH_CLIENT_WMS=1 -DWITH_CLIENT_WFS=1 -DWITH_KML=1 -DWITH_SOS=1 -DWITH_PHP=1 -DWITH_PYTHON=1 -DWITH_FRIBIDI=0 -DWITH_FCGI=0 -DWITH_EXEMPI=1 -DCMAKE_BUILD_TYPE=Release -DWITH_RSVG=1 -DWITH_CURL=1
   make -j4
   cd ..
   make -f Makefile.autotest -j4 test
else
   #autoconf build
   ./configure --with-gd --with-postgis --with-wmsclient --with-proj --with-wfsclient --with-kml --with-cairo --with-wcs --with-sos --with-geos --with-gdal --with-ogr --with-wfs
   make clean
   make -j4
   make -j4 test
fi
