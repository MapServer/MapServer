AUTOTEST_OPTS?=--strict_mode
PHP_MAPSCRIPT=build/mapscript/php/php_mapscript.so
PYTHON_MAPSCRIPT_PATH=build/mapscript/python
JAVA_MAPSCRIPT_PATH=build/mapscript/java
CSHARP_MAPSCRIPT_PATH=build/mapscript/csharp
PERL_MAPSCRIPT_PATH=build/mapscript/perl
BUILDPATH=../../build
FLEX=flex
YACC=yacc
CMAKEFLAGS=-DCMAKE_C_FLAGS="--coverage ${CMAKE_C_FLAGS}" -DCMAKE_CXX_FLAGS="--coverage ${CMAKE_CXX_FLAGS}" \
			  -DCMAKE_SHARED_LINKER_FLAGS="-lgcov" -DWITH_CLIENT_WMS=1 \
			  -DWITH_CLIENT_WFS=1 -DWITH_KML=1 -DWITH_SOS=1 -DWITH_CSHARP=1 -DWITH_PHP=1 -DWITH_PERL=1 \
			  -DWITH_PYTHON=1 -DWITH_JAVA=1 -DWITH_THREAD_SAFETY=1 -DWITH_FRIBIDI=1 -DWITH_FCGI=0 -DWITH_EXEMPI=1 \
			  -DCMAKE_BUILD_TYPE=Release -DWITH_RSVG=1 -DWITH_CURL=1 -DWITH_HARFBUZZ=1 -DWITH_POINT_Z_M=1 -DWITH_MSSQL2008=ON ${EXTRA_CMAKEFLAGS}
all: cmakebuild

cmakebuild: lexer parser
	if test ! -s build/Makefile; then  mkdir -p build ; cd build ; cmake .. $(CMAKEFLAGS); fi
	cd build && $(MAKE) $(MFLAGS)

warning:
	$(error "This Makefile is used to run the \"test\" target")

wxs-testcase:
	cd msautotest/wxs && chmod 777 tmp && rm -f result/* && export PATH=$(BUILDPATH):$(PATH) && (./run_test.py $(AUTOTEST_OPTS) || /bin/true)

renderers-testcase:
	cd msautotest/renderers  && rm -f result/* && export PATH=$(BUILDPATH):$(PATH) && (./run_test.py $(AUTOTEST_OPTS) || /bin/true)

misc-testcase:
	cd msautotest/misc  && rm -f result/* && export PATH=$(BUILDPATH):$(PATH) && (./run_test.py $(AUTOTEST_OPTS) || /bin/true)

gdal-testcase:
	cd msautotest/gdal  && rm -f result/* && export PATH=$(BUILDPATH):$(PATH) && (./run_test.py $(AUTOTEST_OPTS) || /bin/true)

query-testcase:
	cd msautotest/query  && rm -f result/* && export PATH=$(BUILDPATH):$(PATH) && (./run_test.py $(AUTOTEST_OPTS) || /bin/true)

sld-testcase:
	cd msautotest/sld  && rm -f result/* && export PATH=$(BUILDPATH):$(PATH) && (./run_test.py $(AUTOTEST_OPTS) || /bin/true)

mspython-testcase:
	test -f "$(PYTHON_MAPSCRIPT_PATH)/_mapscript.so" && (export PYTHONPATH="../../$(PYTHON_MAPSCRIPT_PATH)" && cd msautotest/mspython && python run_all_tests.py)

mspython-wheel:
	cd build && cmake --build . --target pythonmapscript-wheel

php-testcase:
	test -f "$(PHP_MAPSCRIPT)" && (export PHP_MAPSCRIPT_SO="../../$(PHP_MAPSCRIPT)" && cd msautotest/php && ./run_test.sh)

java-testcase:
	test -d "$(JAVA_MAPSCRIPT_PATH)" && (export JAVA_MAPSCRIPT_SO="../../$(JAVA_MAPSCRIPT_PATH)" && cd mapscript/java && ./run_test.sh)

csharp-testcase:
	test -d "$(CSHARP_MAPSCRIPT_PATH)" && (export CSHARP_MAPSCRIPT_SO="../../$(CSHARP_MAPSCRIPT_PATH)" && cd mapscript/csharp && ./run_test.sh)

perl-testcase:
	cd "$(PERL_MAPSCRIPT_PATH)" \
	&& PERL5LIB=`pwd` \
	&& prove tests \
	&& perl examples/RFC24.pl ../../../tests/test.map \
	&& perl examples/shp_in_shp.pl --infile1 ../../../tests/line.shp --infile1_shpid 0 --infile2 ../../../tests/polygon.shp --infile2_shpid 0 \
	&& perl examples/dump.pl --file ../../../tests/line.shp \
	&& perl examples/thin.pl --input ../../../tests/polygon --output examples/junk --tolerance=5



test:  cmakebuild
	@$(MAKE) $(MFLAGS)	wxs-testcase renderers-testcase misc-testcase gdal-testcase query-testcase sld-testcase mspython-testcase
	@./print-test-results.sh
	@$(MAKE) $(MFLAGS)	php-testcase
	@$(MAKE) $(MFLAGS)	csharp-testcase
	@$(MAKE) $(MFLAGS)	perl-testcase

asan_compatible_tests:  cmakebuild
	@$(MAKE) $(MFLAGS)	wxs-testcase renderers-testcase misc-testcase gdal-testcase query-testcase sld-testcase
	@./print-test-results.sh

lexer: maplexer.c
parser: mapparser.c

maplexer.c: maplexer.l
	$(FLEX) --nounistd -Pmsyy -i -o$(CURDIR)/maplexer.c maplexer.l

mapparser.c: mapparser.y
	$(YACC) -d -o$(CURDIR)/mapparser.c mapparser.y
