#!/bin/bash -eu
# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

BUILD_SH_FROM_REPO="$SRC/MapServer/fuzzers/build.sh"
if test -f "$BUILD_SH_FROM_REPO"; then
    if test "$0" != "$BUILD_SH_FROM_REPO"; then
        echo "Running $BUILD_SH_FROM_REPO"
        exec "$BUILD_SH_FROM_REPO"
        exit $?
    fi
fi

apt-get install -y libsqlite3-dev liblzma-dev sqlite3
apt-get remove -y libproj15 libproj-dev libxml2-dev libxml2

# Build libxml2 from source (packaged one depends on libicu which is C++)
wget https://gitlab.gnome.org/GNOME/libxml2/-/archive/v2.10.2/libxml2-v2.10.2.tar.gz
tar xvzf libxml2-v2.10.2.tar.gz
cd libxml2-v2.10.2
cmake . -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC"
make -j$(nproc) -s
make install
cd ..

# Build PROJ dependency (we can't use the packaged one because of incompatibilities
# between libstdc++ from clang and the libc++ of clang used by ossfuzz
git clone --depth 1 https://github.com/OSGeo/PROJ proj
cd proj
cmake . -DBUILD_SHARED_LIBS:BOOL=OFF \
      -DENABLE_TIFF:BOOL=OFF \
      -DENABLE_CURL:BOOL=OFF \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DBUILD_APPS:BOOL=OFF \
      -DBUILD_TESTING:BOOL=OFF
make -j$(nproc) -s
make install
cd ..

#Build gdal dependency
pushd $SRC/gdal
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF \
-DGDAL_BUILD_OPTIONAL_DRIVERS:BOOL=OFF -DOGR_BUILD_OPTIONAL_DRIVERS:BOOL=OFF \
-DBUILD_APPS=OFF \
-DCMAKE_EXE_LINKER_FLAGS="-pthread" ../
make -j$(nproc) GDAL
make install
popd


#Build MapServer
cd $SRC/MapServer
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC=ON \
-DWITH_PROTOBUFC=0 -DWITH_FRIBIDI=0 -DWITH_HARFBUZZ=0 -DWITH_CAIRO=0 -DWITH_FCGI=0 \
-DWITH_GEOS=0 -DWITH_POSTGIS=0 -DWITH_GIF=0 ../
#While using undefined sanitizer, Project cannot compile binary but can compile library.
make -j$(nproc) mapserver_static

#Fuzzers
for fuzzer in mapfuzzer shapefuzzer configfuzzer; do
    $CC $CFLAGS -Wall -Wextra -I. -I.. -I/usr/include/gdal/. -DPROJ_VERSION_MAJOR=6 -c ../fuzzers/${fuzzer}.c

    $CXX $CXXFLAGS $LIB_FUZZING_ENGINE ${fuzzer}.o -o ${fuzzer} \
        -L. -lmapserver_static -lgdal \
        -Wl,-Bstatic -lpng -ljpeg -lfreetype -lproj -lxml2 -lz -lsqlite3 -llzma \
        -Wl,-Bdynamic  -lpthread -ldl -lc++

    cp ${fuzzer} $OUT/
done

cd ..

zip -r $OUT/mapfuzzer_seed_corpus.zip tests/*.map
zip -r $OUT/shapefuzzer_seed_corpus.zip tests/*.shp tests/*.shx tests/*.dbf
zip -r $OUT/configfuzzer_seed_corpus.zip tests/*.conf etc/*.conf msautotest/config/*.conf
