#!/bin/bash

NUMTHREADS=$(nproc)
export NUMTHREADS

cd /vagrant

cd msautotest
python -m SimpleHTTPServer &> /dev/null &
cd ..

mkdir build_vagrant
touch maplexer.l
touch mapparser.y
flex --nounistd -Pmsyy -i -omaplexer.c maplexer.l
yacc -d -omapparser.c mapparser.y
cd build_vagrant
cmake   -G "Unix Makefiles" -DWITH_CLIENT_WMS=1 \
        -DWITH_CLIENT_WFS=1 -DWITH_KML=1 -DWITH_SOS=1 -DWITH_PHP=1 \
        -DWITH_PYTHON=1 -DWITH_JAVA=0 -DWITH_THREAD_SAFETY=1 \
        -DWITH_FCGI=0 -DWITH_EXEMPI=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_RSVG=1 -DWITH_CURL=1 -DWITH_FRIBIDI=1 -DWITH_HARFBUZZ=1 \
        -DPROJ_INCLUDE_DIR=/vagrant/install-vagrant-proj-6/include_proj4_api_only -DPROJ_LIBRARY=/vagrant/install-vagrant-proj-6/lib/libproj.so.15 \
        -DCMAKE_C_FLAGS="-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H -DPROJ_RENAME_SYMBOLS" -DCMAKE_CXX_FLAGS="-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H -DPROJ_RENAME_SYMBOLS" \
        ..

make -j $NUMTHREADS
sudo make install
cd ..

mkdir build_vagrant_proj6
cd build_vagrant_proj6
cmake   -G "Unix Makefiles" -DWITH_CLIENT_WMS=1 \
        -DWITH_CLIENT_WFS=1 -DWITH_KML=1 -DWITH_SOS=1 -DWITH_PHP=1 \
        -DWITH_PYTHON=1 -DWITH_JAVA=0 -DWITH_THREAD_SAFETY=1 \
        -DWITH_FCGI=0 -DWITH_EXEMPI=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_RSVG=1 -DWITH_CURL=1 -DWITH_FRIBIDI=1 -DWITH_HARFBUZZ=1 \
        -DPROJ_INCLUDE_DIR=/vagrant/install-vagrant-proj-6/include -DPROJ_LIBRARY=/vagrant/install-vagrant-proj-6/lib/libproj.so.15 \
        -DCMAKE_C_FLAGS="-DPROJ_RENAME_SYMBOLS" -DCMAKE_CXX_FLAGS="-DPROJ_RENAME_SYMBOLS" \
        ..

make -j $NUMTHREADS
cd ..
