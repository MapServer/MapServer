#!/bin/sh
set -eu

#if [ "$BUILD_NAME" = "PHP_7.2_WITH_ASAN" ]; then
#    export CC="ccache clang"
#    export CXX="ccache clang++"
#else
    export CC="ccache gcc"
    export CXX="ccache g++"
#fi

if [ "$BUILD_NAME" = "PHP_7.2_WITH_ASAN" ]; then
    # Force use of PROJ 4 API
    sudo rm /usr/include/proj.h
    # -DNDEBUG to avoid issues with cairo cleanup
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-g -fsanitize=address -DNDEBUG -DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" CMAKE_CXX_FLAGS="-g -fsanitize=address -DNDEBUG -DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" EXTRA_CMAKEFLAGS="-DCMAKE_BUILD_TYPE=None -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
    export AUTOTEST_OPTS="--strict --run_under_asan"
    # Only run tests that only involve mapserv/shp2img binaries. mspython, etc would require LD_PREOLOAD'ing the asan shared object
    make -j4 asan_compatible_tests
elif [ "$BUILD_NAME" = "PHP_7.3_WITH_PROJ7" ]; then
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-O2" CMAKE_CXX_FLAGS="-O2" LIBMAPSERVER_EXTRA_FLAGS="-Wall -Werror -Wextra"
    make mspython-wheel
    make -j4 test
else
    # Force use of PROJ 4 API
    sudo rm /usr/include/proj.h
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" CMAKE_CXX_FLAGS="-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H" LIBMAPSERVER_EXTRA_FLAGS="-Wall -Werror -Wextra"
    make mspython-wheel
    make -j4 test
fi
