AUTOTEST_OPTS=-strict -q
PHP_MAPSCRIPT=build/mapscript/php/php_mapscript.so
PYTHON_MAPSCRIPT_PATH=build/mapscript/python
JAVA_MAPSCRIPT_PATH=build/mapscript/java
BUILDPATH=../../build
FLEX=flex
YACC=yacc
CMAKEFLAGS=-DCMAKE_C_FLAGS="--coverage" -DCMAKE_CXX_FLAGS="--coverage" \
			  -DCMAKE_SHARED_LINKER_FLAGS="-lgcov" -DWITH_CLIENT_WMS=1 \
			  -DWITH_CLIENT_WFS=1 -DWITH_KML=1 -DWITH_SOS=1 -DWITH_PHP=1 \
			  -DWITH_PYTHON=1 -DWITH_JAVA=1 -DWITH_THREAD_SAFETY=1 -DWITH_FRIBIDI=1 -DWITH_FCGI=0 -DWITH_EXEMPI=1 \
			  -DCMAKE_BUILD_TYPE=Release -DWITH_RSVG=1 -DWITH_CURL=1 -DWITH_HARFBUZZ=1 -DWITH_POINT_Z_M=1
all: cmakebuild

cmakebuild: lexer parser
	if test ! -s build/Makefile; then  mkdir -p build ; cd build ; cmake .. $(CMAKEFLAGS); fi
	cd build && $(MAKE) $(MFLAGS)

warning:
	$(error "This Makefile is used to run the \"test\" target")

wxs-testcase:
	cd msautotest/wxs && export PATH=$(BUILDPATH):$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

renderers-testcase:
	cd msautotest/renderers && export PATH=$(BUILDPATH):$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

misc-testcase:
	cd msautotest/misc && export PATH=$(BUILDPATH):$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

gdal-testcase:
	cd msautotest/gdal && export PATH=$(BUILDPATH):$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

query-testcase:
	cd msautotest/query && export PATH=$(BUILDPATH):$(PATH) && ./run_test.py $(AUTOTEST_OPTS)

autotest-install:
	test -d "msautotest/wxs" ||  ( git submodule init && git submodule update )

mspython-testcase:
	test -f "$(PYTHON_MAPSCRIPT_PATH)/_mapscript.so" && (export PYTHONPATH="../../$(PYTHON_MAPSCRIPT_PATH)" && cd msautotest/mspython && python run_all_tests.py)

php-testcase:
	test -f "$(PHP_MAPSCRIPT)" && (export PHP_MAPSCRIPT_SO="../../$(PHP_MAPSCRIPT)" && cd msautotest/php && ./run_test.sh)

java-testcase:
	test -d "$(JAVA_MAPSCRIPT_PATH)" && (export JAVA_MAPSCRIPT_SO="../../$(JAVA_MAPSCRIPT_PATH)" && cd mapscript/java && ./run_test.sh)

test: autotest-install cmakebuild
	@$(MAKE) $(MFLAGS)	wxs-testcase renderers-testcase misc-testcase gdal-testcase query-testcase mspython-testcase
	@./print-test-results.sh
	@$(MAKE) $(MFLAGS)	php-testcase
	@$(MAKE) $(MFLAGS)	java-testcase


lexer: maplexer.c
parser: mapparser.c

maplexer.c: maplexer.l
	$(FLEX) --nounistd -Pmsyy -i -o$(CURDIR)/maplexer.c $(CURDIR)/maplexer.l

mapparser.c: mapparser.y
	$(YACC) -d -o$(CURDIR)/mapparser.c $(CURDIR)/mapparser.y
