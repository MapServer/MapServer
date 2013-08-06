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
   make -j4 test
else
   #autoconf build
   ./configure --with-gd --with-postgis --with-wmsclient --with-proj --with-wfsclient --with-kml --with-cairo --with-wcs --with-sos --with-geos --with-gdal --with-ogr --with-wfs
   make clean
   make -j4
   make -j4 test
fi
