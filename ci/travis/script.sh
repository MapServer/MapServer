#!/bin/sh
set -eu

eval "$(pyenv init --path)"
eval "$(pyenv init -)"

export CC="ccache gcc"
export CXX="ccache g++"

#make sure to use recent CMake, and the pyenv Python instance
export PYTHONPREFIX="$(dirname $(realpath $(pyenv which python)))/.."
if [ -z ${TRAVIS+x} ]; then
    #not travis
    export PATH=${PYTHONPREFIX}/bin:${PATH}
else
    #travis
    export PATH=${TRAVIS_BUILD_DIR}/deps/cmake-install:${TRAVIS_BUILD_DIR}/deps/cmake-install/bin:${PYTHONPREFIX}/bin:${PATH}  
fi
cmake --version

# check we are using the correct versions
pyenv which pip
pyenv which python
#pyenv which python-config
# check for phpunit & xdebug
php msautotest/php/phpunit-10.phar --version
php -v

if [ "$WITH_ASAN" = "YES" ]; then
    # -DNDEBUG to avoid issues with cairo cleanup
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-g -fsanitize=address -DNDEBUG" CMAKE_CXX_FLAGS="-g -fsanitize=address -DNDEBUG" EXTRA_CMAKEFLAGS="-DCMAKE_BUILD_TYPE=None -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
    export AUTOTEST_OPTS="--strict --run_under_asan"
    # Only run tests that only involve mapserv/map2img binaries. mspython, etc would require LD_PREOLOAD'ing the asan shared object
    make -j4 asan_compatible_tests
else
    make cmakebuild MFLAGS="-j2" CMAKE_C_FLAGS="-O2" CMAKE_CXX_FLAGS="-O2" LIBMAPSERVER_EXTRA_FLAGS="-Wall -Werror -Wextra"
    make mspython-wheel
    make phpng-build    
    make -j4 test
fi
