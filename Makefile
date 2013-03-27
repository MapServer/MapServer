AUTOTEST_OPTS=-strict
PHP_MAPSCRIPT=build/mapscript/php/php_mapscript.so

wxs-testcase:
	cd msautotest/wxs && export PATH=../..:$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

renderers-testcase:
	cd msautotest/renderers && export PATH=../..:$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

misc-testcase:
	cd msautotest/misc && export PATH=../..:$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

gdal-testcase:
	cd msautotest/gdal && export PATH=../..:$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

query-testcase:
	cd msautotest/query && export PATH=../..:$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

autotest-install:
	test -d "msautotest/wxs" ||  ( git submodule init && git submodule update )

php-testcase:
	test -f "$(PHP_MAPSCRIPT)" && (export PHP_MAPSCRIPT_SO="../../$(PHP_MAPSCRIPT)" && cd msautotest/php && ./run_test.sh)

print-test-results:
	@./print-test-results.sh

test:
	@$(MAKE) $(MFLAGS)	wxs-testcase renderers-testcase misc-testcase gdal-testcase query-testcase
	@$(MAKE) print-test-results
	@$(MAKE) php-testcase

