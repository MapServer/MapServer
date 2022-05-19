#!/bin/sh
set -eu

eval "$(pyenv init --path)"
eval "$(pyenv init -)"

#if [ "$BUILD_NAME" = "PHP_7.2_WITH_ASAN" ]; then
#    export CC="ccache clang"
#    export CXX="ccache clang++"
#else
    export CC="ccache gcc"
    export CXX="ccache g++"
#fi

#upgrade to recent SWIG
git clone https://github.com/swig/swig.git swig-git-master
cd swig-git-master
wget https://github.com/PhilipHazel/pcre2/releases/download/pcre2-10.39/pcre2-10.39.tar.gz
./Tools/pcre-build.sh
./autogen.sh
./configure --prefix=/usr
make
sudo make install
sudo ldconfig
cd ../
#check SWIG version
swig -version

#make sure to use recent CMake
export PATH=${TRAVIS_BUILD_DIR}/deps/cmake-install:${TRAVIS_BUILD_DIR}/deps/cmake-install/bin:$PATH
cmake --version

# check we are using the correct versions
pyenv which pip
pyenv which python
pyenv which python-config

if [ "$BUILD_NAME" = "PHP_7.4_WITH_ASAN" ]; then
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
