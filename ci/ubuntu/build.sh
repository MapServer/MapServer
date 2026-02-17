#!/bin/sh
set -eu

export CC="ccache gcc"
export CXX="ccache g++"

# Turn CMake warnings as errors
EXTRA_CMAKEFLAGS="-Werror=dev"

rm -rf build

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

# msautotests setup
cd msautotest
./create_postgis_test_data.sh
# copy custom projection to the PROJ_DATA folder
sudo cp ./wxs/data/epsg2 /usr/share/proj/

# lint Python msautotests
python3 -m pyflakes .

# run the Python server for the tests
python3 -m http.server &> /dev/null &

cd .. # Back to root

PHP_VERSIONS="8.3 8.4 8.5"
ORIG_PHP_VERSION=$(php -r 'echo PHP_MAJOR_VERSION.".".PHP_MINOR_VERSION;')

for v in ${PHP_VERSIONS}; do
    echo "--- Testing PHP ${v} ---"
    sudo update-alternatives --set php /usr/bin/php${v}
    sudo update-alternatives --set php-config /usr/bin/php-config${v}
    
    
    # Force re-configuration for current PHP version
    # Remove Makefile to trigger CMake reconfiguration (Makefile has: if test ! -s build/Makefile)
    rm -f build/Makefile
    rm -f build/CMakeCache.txt
    
    # Explicitly pass PHP_CONFIG_EXECUTABLE to ensure CMake uses the correct PHP version
    PHP_CONFIG_PATH="/usr/bin/php-config${v}"
    make cmakebuild_mapscript_php EXTRA_CMAKEFLAGS="${EXTRA_CMAKEFLAGS} -DPHP_CONFIG_EXECUTABLE=${PHP_CONFIG_PATH}"
    
    # Run a quick check to see if extension loads
    export PHP_MAPSCRIPT_SO=$(pwd)/build/src/mapscript/phpng/php_mapscriptng.so
    php -d "extension=${PHP_MAPSCRIPT_SO}" -r "if(!class_exists('mapObj')) { echo 'MapScript not loaded!'; exit(1); } else { echo 'MapScript loaded successfully!'; }"
    
    # Run PHP tests
    cd msautotest/php
    ./run_test.sh 2>&1 | tee ../../php_test_results_${v}.log
    cd ../.. # Back to root
done

# Restore original PHP version
sudo update-alternatives --set php /usr/bin/php${ORIG_PHP_VERSION}
sudo update-alternatives --set php-config /usr/bin/php-config${ORIG_PHP_VERSION}


if [ "${WITH_ASAN:-}" = "true" ]; then
    export AUTOTEST_OPTS="--strict_mode --run_under_asan"
    # Only run tests that only involve mapserv/map2img binaries. mspython, etc would require LD_PREOLOAD'ing the asan shared object
    make -j4 asan_compatible_tests 2>&1 | tee python_test_results.log
else
    make -j4 test 2>&1 | tee python_test_results.log
fi

if [ "${WITH_COVERAGE:-}" = "true" ]; then
    lcov --directory . --capture --output-file mapserver.info 2>/dev/null
    # Capture coverage summary
    COVERAGE_OUTPUT=$(lcov --remove mapserver.info '/usr/*' --output-file mapserver.info)
    echo "$COVERAGE_OUTPUT"
    
    # Extract line coverage: "lines......: 12.3% (123 of 1000)"
    LINE_COV=$(echo "$COVERAGE_OUTPUT" | grep "lines......:" | head -1)
    # Extract function coverage: "functions..: 10.5% (10 of 95)"
    FUNC_COV=$(echo "$COVERAGE_OUTPUT" | grep "functions..:" | head -1)
fi

echo ""
echo "========================================"
echo "         TEST SUMMARY"
echo "========================================"
echo ""

# Summary for PHP tests
echo "PHP MapScript Tests:"
for v in ${PHP_VERSIONS}; do
    LOG_FILE="php_test_results_${v}.log"
    if [ -f "${LOG_FILE}" ]; then
        # Extract metrics: Tests: 165, Assertions: 213, Errors: 1, Skipped: 18
        # Use grep -E for extended regex, matching "Tests: ... Assertions: ..."
        # Handle optional trailing dot and variable whitespace
        METRICS=$(grep -E "Tests: [0-9]+,.?Assertions: [0-9]+,.?Errors: [0-9]+(,.?kipped: [0-9]+)?" "${LOG_FILE}" | tail -1)
        if [ -n "${METRICS}" ]; then
            echo "  - PHP ${v}: ${METRICS}"
        else
            echo "  - PHP ${v}: Completed (Metrics not found)"
            # Debug: print last few lines if metrics not found
            # tail -n 5 "${LOG_FILE}" | sed 's/^/    /'
        fi
        rm -f "${LOG_FILE}"
    else
        echo "  - PHP ${v}: Log file not found"
    fi
done
echo ""

# Summary for MapServer tests
echo "MapServer Tests:"
if [ -f "python_test_results.log" ]; then
    # Parse Pytest output: == 10 passed, 2 skipped in 0.5s ==
    # We need to sum up results from multiple test suites if run separately, but here make test runs them sequentially
    # Let's try to grab the last summary line which usually sums things up or list individual suite results
    
    # Since 'make test' runs multiple test suites (api, config, etc.), each has its own summary.
    # We can grep for all summary lines.
    
    echo "  Metrics per suite:"
    grep -E "== .* passed, .* skipped in .* ==" python_test_results.log | while read -r line; do
        # Clean up the line to just show the metrics part
        CLEAN_LINE=$(echo "$line" | sed 's/=//g' | xargs)
        echo "    - ${CLEAN_LINE}"
    done
    
    rm -f "python_test_results.log"
else
    if [ "${WITH_ASAN:-}" = "true" ]; then
        echo "  - ASAN-compatible tests completed (Log not found)"
    else
        echo "  - Full test suite completed (Log not found)"
    fi
fi

if [ "${WITH_COVERAGE:-}" = "true" ]; then
    echo ""
    echo "Code Coverage:"
    if [ -n "$LINE_COV" ]; then
         echo "  - ${LINE_COV}"
    else
         echo "  - Line coverage not available"
    fi
    if [ -n "$FUNC_COV" ]; then
         echo "  - ${FUNC_COV}"
    fi
fi
echo ""

# Check for any test failures in logs
FAILED_TESTS=0
if grep -r "FAILED" msautotest/*/result/ 2>/dev/null | grep -v ".vrt" | head -1 > /dev/null; then
    FAILED_TESTS=1
    echo "⚠️  WARNING: Some tests may have failed"
    echo "   Check msautotest/*/result/ directories for details"
else
    echo "✅ All tests appear to have passed"
fi
echo ""

echo "========================================"
echo "Build and test process completed!"
echo "========================================"

# Exit with error if tests failed
if [ $FAILED_TESTS -eq 1 ]; then
    exit 1
fi
