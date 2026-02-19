#!/bin/sh
set -eu

export CC="ccache gcc"
export CXX="ccache g++"

# Turn CMake warnings as errors
EXTRA_CMAKEFLAGS="-Werror=dev"

if [ "${WITH_ASAN:-}" = "true" ]; then
    # -DNDEBUG to avoid issues with cairo cleanup
    make cmakebuild \
        MFLAGS="-j$(nproc)" \
        CMAKE_C_FLAGS="-g -fsanitize=address -DNDEBUG" \
        CMAKE_CXX_FLAGS="-g -fsanitize=address -DNDEBUG" \
        EXTRA_CMAKEFLAGS="${EXTRA_CMAKEFLAGS} -DCMAKE_BUILD_TYPE=None -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
else
    make cmakebuild \
        MFLAGS="-j$(nproc)" \
        CMAKE_C_FLAGS="-O2" \
        CMAKE_CXX_FLAGS="-O2" \
        LIBMAPSERVER_EXTRA_FLAGS="-Wall -Werror -Wextra" \
        EXTRA_CMAKEFLAGS="${EXTRA_CMAKEFLAGS} -DWITH_PCRE2=ON"
    make mspython-wheel
    make cmakebuild_mapscript_php
fi

# msautotests setup

# setup postgresql
sudo sed -i  's/md5/trust/' /etc/postgresql/16/main/pg_hba.conf
sudo sed -i  's/peer/trust/' /etc/postgresql/16/main/pg_hba.conf
sudo service postgresql restart 16

cd msautotest
./create_postgis_test_data.sh
# copy custom projection to the PROJ_DATA folder
sudo cp ./wxs/data/epsg2 /usr/share/proj/

# lint Python msautotests
python3 -m pyflakes .

# run the Python server for the tests
python3 -m http.server &> /dev/null &

cd .. # Back to root

# --- PHP multi-version testing ---
PHP_VERSIONS="8.3 8.4 8.5"
ORIG_PHP_VERSION=$(php -r 'echo PHP_MAJOR_VERSION.".".PHP_MINOR_VERSION;')

for v in ${PHP_VERSIONS}; do
    echo "--- Testing PHP ${v} ---"
    sudo update-alternatives --set php /usr/bin/php${v}
    sudo update-alternatives --set php-config /usr/bin/php-config${v}

    # Force CMake reconfiguration for the new PHP version
    rm -f build/Makefile build/CMakeCache.txt

    PHP_CONFIG_PATH="/usr/bin/php-config${v}"
    make cmakebuild_mapscript_php EXTRA_CMAKEFLAGS="${EXTRA_CMAKEFLAGS} -DPHP_CONFIG_EXECUTABLE=${PHP_CONFIG_PATH}"

    # Verify extension loads
    export PHP_MAPSCRIPT_SO=$(pwd)/build/src/mapscript/phpng/php_mapscriptng.so
    php -d "extension=${PHP_MAPSCRIPT_SO}" -r "if(!class_exists('mapObj')) { echo 'FAIL'; exit(1); } else { echo 'PHP ${v}: MapScript OK'; }"

    # Run PHP tests
    cd msautotest/php
    ./run_test.sh
    cd ../..
done

# Restore original PHP version
sudo update-alternatives --set php /usr/bin/php${ORIG_PHP_VERSION}
sudo update-alternatives --set php-config /usr/bin/php-config${ORIG_PHP_VERSION}

# Run MapServer tests
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
