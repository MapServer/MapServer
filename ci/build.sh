#!/bin/sh
set -eu

export CC="ccache gcc"
export CXX="ccache g++"

if [ "${MAPSCRIPT_PYTHON_ONLY:-}" = "true" ]; then
    # only build MapServer with the Python MapScript and not PHP, Perl etc.
    make cmakebuild_mapscript_python MFLAGS="-j$(nproc)" CMAKE_C_FLAGS="-O2" CMAKE_CXX_FLAGS="-O2" LIBMAPSERVER_EXTRA_FLAGS="-Wall -Werror -Wextra"
    # build the wheel and run the Python MapScript test suite
    make mspython-wheel
    exit
fi

# Turn CMake warnings as errors
EXTRA_CMAKEFLAGS="-Werror=dev"

if [ "${WITH_ASAN:-}" = "true" ]; then
    # -DNDEBUG to avoid issues with cairo cleanup
    make cmakebuild MFLAGS="-j$(nproc)" CMAKE_C_FLAGS="-g -fsanitize=address -DNDEBUG" CMAKE_CXX_FLAGS="-g -fsanitize=address -DNDEBUG" \
    EXTRA_CMAKEFLAGS="${EXTRA_CMAKEFLAGS} -DCMAKE_BUILD_TYPE=None -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
else
    make cmakebuild MFLAGS="-j$(nproc)" CMAKE_C_FLAGS="-O2" CMAKE_CXX_FLAGS="-O2" LIBMAPSERVER_EXTRA_FLAGS="-Wall -Werror -Wextra"
    make mspython-wheel
    make phpng-build
fi

# msautotests setup

# setup postgresql
sudo sed -i  's/md5/trust/' /etc/postgresql/12/main/pg_hba.conf
sudo sed -i  's/peer/trust/' /etc/postgresql/12/main/pg_hba.conf
sudo service postgresql restart 12

cd msautotest
./create_postgis_test_data.sh
# copy custom projection to the PROJ_DATA folder
sudo cp ./wxs/data/epsg2 /usr/share/proj/

# lint Python msautotests
python -m pyflakes .

# run the Python server for the tests
python -m http.server &> /dev/null &

# get PHPUnit
echo "PHP version"
php -v
cd php && curl -LO https://phar.phpunit.de/phpunit-11.phar
echo "PHPUnit version"
php phpunit-11.phar --version
echo "PHP includes"
php-config --includes

cd ../../

if [ "${WITH_ASAN:-}" = "true" ]; then
    export AUTOTEST_OPTS="--strict_mode --run_under_asan"
    # Only run tests that only involve mapserv/map2img binaries. mspython, etc would require LD_PREOLOAD'ing the asan shared object
    make -j4 asan_compatible_tests
else
    make -j4 test
fi

if [ "${WITH_COVERAGE:-}" = "true" ]; then
    lcov --directory . --capture --output-file mapserver.info 2>/dev/null
    lcov --remove mapserver.info '/usr/*' --output-file mapserver.info
fi
