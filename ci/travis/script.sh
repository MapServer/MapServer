#!/bin/sh
set -eu

if [ "$BUILD_NAME" = "PHP_7.2_WITH_ASAN" ]; then
    export CC="ccache clang"
    export CXX="ccache clang++"
    # -DNDEBUG to avoid issues with cairo cleanup
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-g -fsanitize=address -DNDEBUG" CMAKE_CXX_FLAGS="-g -fsanitize=address -DNDEBUG" EXTRA_CMAKEFLAGS="-DCMAKE_BUILD_TYPE=None -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
    export AUTOTEST_OPTS="-q -strict -run_under_asan"
    # Only run tests that only involve mapserv/shp2img binaries. mspython, etc would require LD_PREOLOAD'ing the asan shared object
    make -j4 asan_compatible_tests
else
    export CC="ccache gcc"
    export CXX="ccache g++"
    make cmakebuild MFLAGS="-j2"
    make mspython-wheel
    make -j4 test
fi
