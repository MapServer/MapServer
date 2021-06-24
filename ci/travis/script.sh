#!/bin/sh
set -eu

#if [ "$BUILD_NAME" = "PHP_7.3_WITH_ASAN" ]; then
#    export CC="ccache clang"
#    export CXX="ccache clang++"
#else
    export CC="ccache gcc"
    export CXX="ccache g++"
#fi

curl http://download.osgeo.org/proj/proj-6.1.1.tar.gz > proj-6.1.1.tar.gz
tar xzf proj-6.1.1.tar.gz
mv proj-6.1.1 proj
(cd proj/data && curl http://download.osgeo.org/proj/proj-datumgrid-1.8.tar.gz > proj-datumgrid-1.8.tar.gz && tar xvzf proj-datumgrid-1.8.tar.gz)
(cd proj; CFLAGS='-O2 -DPROJ_RENAME_SYMBOLS' CXXFLAGS='-O2 -DPROJ_RENAME_SYMBOLS' ./configure --disable-static --prefix=/usr/local && CCACHE_CPP2=yes make -j2 && sudo make -j3 install)
sudo rm -f /usr/include/proj_api.h

if [ "$BUILD_NAME" = "PHP_7.3_WITH_ASAN" ]; then
    # Force use of PROJ 4 API
    sudo rm /usr/local/include/proj.h
    # -DNDEBUG to avoid issues with cairo cleanup
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-g -fsanitize=address -DNDEBUG -DPROJ_RENAME_SYMBOLS -DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" CMAKE_CXX_FLAGS="-g -fsanitize=address -DNDEBUG -DPROJ_RENAME_SYMBOLS -DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" EXTRA_CMAKEFLAGS="-DCMAKE_BUILD_TYPE=None -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address -DPROJ_INCLUDE_DIR=/usr/local/include -DPROJ_LIBRARY=/usr/local/lib/libproj.so.15"
    export AUTOTEST_OPTS="--strict --run_under_asan"
    # Only run tests that only involve mapserv/shp2img binaries. mspython, etc would require LD_PREOLOAD'ing the asan shared object
    make -j4 asan_compatible_tests
elif [ "$BUILD_NAME" = "PHP_7.4_WITH_PROJ6" ]; then
    # Avoid any use of PROJ 4 API
    sudo rm -f /usr/include/proj_api.h
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-O2 -DPROJ_RENAME_SYMBOLS" CMAKE_CXX_FLAGS="-O2 -DPROJ_RENAME_SYMBOLS" EXTRA_CMAKEFLAGS="-DPROJ_INCLUDE_DIR=/usr/local/include -DPROJ_LIBRARY=/usr/local/lib/libproj.so.15"
    make mspython-wheel
    make -j4 test
else
    # Force use of PROJ 4 API
    sudo rm /usr/local/include/proj.h
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-DPROJ_RENAME_SYMBOLS -DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" CMAKE_CXX_FLAGS="-DPROJ_RENAME_SYMBOLS -DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" EXTRA_CMAKEFLAGS="-DPROJ_INCLUDE_DIR=/usr/local/include -DPROJ_LIBRARY=/usr/local/lib/libproj.so.15"
    make mspython-wheel
    make -j4 test
fi
