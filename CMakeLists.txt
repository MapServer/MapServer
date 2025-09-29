cmake_minimum_required(VERSION 3.16...3.29)

project (MapServer)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
      message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:
   mkdir build; cd build; cmake ..
Before that, remove the files created by this failed run:
   rm -rf CMakeCache.txt CMakeFiles")
endif("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")

include(CheckLibraryExists)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)

set (MapServer_VERSION_MAJOR 8)
set (MapServer_VERSION_MINOR 5)
set (MapServer_VERSION_REVISION 0)
set (MapServer_VERSION_SUFFIX "")

# Set C++ version
# Make CMAKE_CXX_STANDARD available as cache option overridable by user
set(CMAKE_CXX_STANDARD 17
  CACHE STRING "C++ standard version to use (default is 17)")
message(STATUS "Requiring C++${CMAKE_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Requiring C++${CMAKE_CXX_STANDARD} - done")

# Set C version
# Make CMAKE_C_STANDARD available as cache option overridable by user
set(CMAKE_C_STANDARD 17
  CACHE STRING "C standard version to use (default is 17)")
message(STATUS "Requiring C${CMAKE_C_STANDARD}")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
message(STATUS "Requiring C${CMAKE_C_STANDARD} - done")

set(TARGET_VERSION_MAJOR ${MapServer_VERSION_MAJOR})
set(TARGET_VERSION_MINOR ${MapServer_VERSION_MINOR})

MATH(EXPR MapServer_IS_DEV_VERSION "${MapServer_VERSION_MINOR}%2")
if(MapServer_IS_DEV_VERSION)
  set (MapServer_VERSION_STRING "${MapServer_VERSION_MAJOR}.${MapServer_VERSION_MINOR}-dev")
else(MapServer_IS_DEV_VERSION)
  set (MapServer_VERSION_STRING "${MapServer_VERSION_MAJOR}.${MapServer_VERSION_MINOR}.${MapServer_VERSION_REVISION}")
  set (MapServer_VERSION_STRING "${MapServer_VERSION_STRING}${MapServer_VERSION_SUFFIX}")
endif(MapServer_IS_DEV_VERSION)
MATH(EXPR MapServer_VERSION_NUM "${MapServer_VERSION_MAJOR}*10000+${MapServer_VERSION_MINOR}*100+${MapServer_VERSION_REVISION}")

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
foreach(_version IN ITEMS 3.20)
    if(CMAKE_VERSION VERSION_LESS "${_version}")
        list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake/${_version}")
    endif()
endforeach()

if (APPLE)
  set(CMAKE_FIND_FRAMEWORK "LAST")
endif (APPLE)

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR share/mapserver/cmake)
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

macro (ms_link_libraries)
  if(BUILD_DYNAMIC)
    target_link_libraries(mapserver PRIVATE ${ARGV})
  endif(BUILD_DYNAMIC)
  if(BUILD_STATIC)
    target_link_libraries(mapserver_static PRIVATE ${ARGV})
  endif(BUILD_STATIC)
endmacro()

macro( report_optional_not_found component )
  message(SEND_ERROR "${component} library/component/dependency could not be found.
  HINTS:
  - disable ${component} support by adding -DWITH_${component}=0
  - add the ${component} install directory to the CMAKE_PREFIX_PATH variable (-DCMAKE_PREFIX_PATH=\"/path/to/${component}-install-dir;/path/to/other/dirs\"")
endmacro()
macro( report_mandatory_not_found component )
  message(SEND_ERROR "${component} library/component could not be found and is a mandatory dependency
  HINT:
  - add the ${component} install directory to the CMAKE_PREFIX_PATH variable (-DCMAKE_PREFIX_PATH=\"/path/to/${component}-install-dir;/path/to/other/dirs\"")
endmacro()
macro( report_dependency_error component dependency)
  message(SEND_ERROR "${component} support requires ${dependency} support, however ${dependency} support has been disabled.
  HINTS:
  - re-run with -DWITH_${dependency}=1 (or without -DWITH_${dependency}=0)
  - disable ${component} support by adding -DWITH_${component}=0"
  )
endmacro()

SET(CMAKE_REQUIRED_INCLUDES "math.h")
if(NOT(WIN32))
SET(CMAKE_REQUIRED_LIBRARIES "m")
endif(NOT(WIN32))

check_function_exists("strrstr"          HAVE_STRRSTR)
check_function_exists("strcasecmp"  HAVE_STRCASECMP)
check_function_exists("strcasestr"  HAVE_STRCASESTR)
check_function_exists("strlcat"  HAVE_STRLCAT)
check_function_exists("strlcpy"  HAVE_STRLCPY)
check_function_exists("strlen"  HAVE_STRLEN)
check_function_exists("strncasecmp"  HAVE_STRNCASECMP)

IF((HAVE_STRLCAT OR HAVE_STRLCPY) AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-D_DEFAULT_SOURCE)
ENDIF()

check_symbol_exists(vsnprintf stdio.h HAVE_VSNPRINTF)
IF(NOT HAVE_VSNPRINTF)
    check_function_exists("vsnprintf"  HAVE_VSNPRINTF)
ENDIF()

check_function_exists("lrintf" HAVE_LRINTF)
check_function_exists("lrint" HAVE_LRINT)

check_include_file(dlfcn.h HAVE_DLFCN_H)


check_c_source_compiles("
int main(int argc, char **argv) {
   long x=0,y=0;
   for(x=0;x<5;x++) {
     if(y>1) break;
     y=__sync_fetch_and_add(&x,1);
   }
}" HAVE_SYNC_FETCH_AND_ADD)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#options supported by the cmake builder
option(WITH_PROTOBUFC "Choose if protocol buffers support should be built in (required for vector tiles)" ON)
option(WITH_KML "Enable native KML output support (requires libxml2 support)" OFF)
option(WITH_SOS "Enable SOS Server support (requires PROJ and libxml2 support)" OFF)
option(WITH_WMS "Enable WMS Server support (requires PROJ support)" ON)
option(WITH_FRIBIDI "Choose if FriBidi glyph shaping support should be built in (useful for right-to-left languages) (requires HARFBUZZ)" ON)
option(WITH_HARFBUZZ "Choose if Harfbuzz complex text layout should be included (needed for e.g. arabic and hindi) (requires FRIBIDI)" ON)
option(WITH_ICONV "Choose if Iconv Internationalization support should be built in" ON)
option(WITH_CAIRO "Choose if CAIRO  rendering support should be built in (required for SVG and PDF output)" ON)
option(WITH_SVGCAIRO "Choose if SVG symbology support (via libsvgcairo) should be built in (requires cairo, libsvg, libsvg-cairo. Incompatible with librsvg)" OFF)
option(WITH_RSVG "Choose if SVG symbology support (via librsvg) should be built in (requires cairo, librsvg. Incompatible with libsvg-cairo)" OFF)
option(WITH_MYSQL "Choose if MYSQL joining support should be built in" OFF)
option(WITH_FCGI "Choose if FastCGI support should be built in" ON)
option(WITH_GEOS "Choose if GEOS geometry operations support should be built in" ON)
option(WITH_POSTGIS "Choose if Postgis input support should be built in" ON)
option(WITH_CLIENT_WMS "Enable Client WMS Layer support (requires CURL)" OFF)
option(WITH_CLIENT_WFS "Enable Client WMS Layer support (requires CURL)" OFF)
option(WITH_CURL "Enable Curl HTTP support (required for wms/wfs client, and remote SLD)" OFF)
option(WITH_WFS "Enable WFS Server support (requires PROJ and OGR support)" ON)
option(WITH_WCS "Enable WCS Server support (requires PROJ and GDAL support)" ON)
option(WITH_OGCAPI "Enable OGCAPI Server support (requires PROJ and OGR support)" ON)
option(WITH_LIBXML2 "Choose if libxml2 support should be built in (used for sos, wcs 1.1,2.0 and wfs 1.1)" ON)
option(WITH_THREAD_SAFETY "Choose if a thread-safe version of libmapserver should be built (only recommended for some mapscripts)" OFF)
option(WITH_GIF "Enable GIF support (for PIXMAP loading)" ON)
option(WITH_PYTHON "Enable Python mapscript support" OFF)
option(WITH_PHPNG "Enable PHPNG (SWIG) mapscript support" OFF)
option(WITH_PERL "Enable Perl mapscript support" OFF)
option(WITH_RUBY "Enable Ruby mapscript support" OFF)
option(WITH_JAVA "Enable Java mapscript support" OFF)
option(WITH_CSHARP "Enable C# mapscript support" OFF)
option(WITH_ORACLESPATIAL "include oracle spatial database input support" OFF)
option(WITH_ORACLE_PLUGIN "include oracle spatial database input support as plugin" OFF)
option(WITH_MSSQL2008 "include mssql 2008 database input support as plugin" OFF)
option(WITH_EXEMPI "include xmp output metadata support" OFF)
option(WITH_XMLMAPFILE "include native xml mapfile support (requires libxslt/libexslt)" OFF)
option(WITH_V8 "include javascript v8 scripting" OFF)
option(WITH_PIXMAN "use (experimental) support for pixman for layer compositing operations" OFF)

option(BUILD_STATIC "Also build a static version of mapserver" OFF)
option(LINK_STATIC_LIBMAPSERVER "Link to static version of libmapserver (also for mapscripts)" OFF)
option(WITH_APACHE_MODULE "include (experimental) support for apache module" OFF)
option(WITH_GENERIC_NINT "generic rounding" OFF)
option(WITH_PYMAPSCRIPT_ANNOTATIONS "Add annotations to Python mapscript output" OFF)
option(INSTALL_HTML_BOOTSTRAP "Whether to install HTML Bootstrap resources for OGCAPIs" ON)

option(FUZZER "Build fuzzers using libFuzzer (requires Clang, will disable executable - mapserv, etc. - generation)" OFF)
mark_as_advanced(FUZZER)
option(BUILD_FUZZER_REPRODUCER "Build fuzzer reproducer programs" ON)

if(FUZZER AND BUILD_FUZZER_REPRODUCER)
    message(FATAL_ERROR "FUZZER and BUILD_FUZZER_REPRODUCER options are mutually exclusive")
endif()

#TODO: USE_OGL? , USE_SDE, USE_CLUSTER_EXTERNAL USE_CLUSTER_PLUGIN, USE_MSSQL2008, USE_MSSQL2008_PLUGIN
# SIGNORE_MISSING_DATA, CGI_CL_DEBUG_ARGS, EXTRA DEBUG FLAGS?,
# PERLV_ld_DETECT?

# Add compiler flags for warnings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror=format-security")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=format-security")
endif()

if(FUZZER)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Need clang for libFuzzer support")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=fuzzer,address -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer,address -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION")
endif()

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(_LIBDIR_DEFAULT "lib")
  # Override this default 'lib' with 'lib64' iff:
  #  - we are on Linux system but NOT cross-compiling
  #  - we are NOT on debian
  #  - we are on a 64 bits system
  # reason is: amd64 ABI: http://www.x86-64.org/documentation/abi.pdf
  # Note that the future of multi-arch handling may be even
  # more complicated than that: http://wiki.debian.org/Multiarch
  if(CMAKE_SYSTEM_NAME MATCHES "Linux"
      AND NOT CMAKE_CROSSCOMPILING
      AND NOT EXISTS "/etc/debian_version")
    if(NOT DEFINED CMAKE_SIZEOF_VOID_P)
      message(AUTHOR_WARNING
        "Unable to determine default CMAKE_INSTALL_LIBDIR directory because no target architecture is known. "
        "Please enable at least one language before including GNUInstallDirs.")
    else()
      if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(_LIBDIR_DEFAULT "lib64")
      endif()
    endif()
  endif()
  set(CMAKE_INSTALL_LIBDIR "${_LIBDIR_DEFAULT}" CACHE PATH "object code libraries (${_LIBDIR_DEFAULT})")
endif()

if(NOT DEFINED CMAKE_INSTALL_RPATH)
  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
SET(CMAKE_MACOSX_RPATH ON)

if(LINK_STATIC_LIBMAPSERVER)
  set(BUILD_STATIC 1)
  set(BUILD_DYNAMIC 0)
  set(MAPSERVER_LIBMAPSERVER mapserver_static)
else(LINK_STATIC_LIBMAPSERVER)
  set(BUILD_DYNAMIC 1)
  set(MAPSERVER_LIBMAPSERVER mapserver)
endif(LINK_STATIC_LIBMAPSERVER)

set(agg_SOURCES
src/renderers/agg/src/agg_arc.cpp
src/renderers/agg/src/agg_vcgen_dash.cpp
src/renderers/agg/src/agg_vcgen_contour.cpp
src/renderers/agg/src/agg_curves.cpp
src/renderers/agg/src/agg_embedded_raster_fonts.cpp
src/renderers/agg/src/agg_trans_affine.cpp
src/renderers/agg/src/agg_vcgen_stroke.cpp
src/renderers/agg/src/agg_font_freetype.cpp
src/renderers/agg/src/agg_line_aa_basics.cpp
src/renderers/agg/src/clipper.cpp
)
include_directories(src/renderers/agg/include)

set(v8_SOURCES
  src/mapscript/v8/v8_object_wrap.hpp
  src/mapscript/v8/point.cpp
  src/mapscript/v8/line.cpp
  src/mapscript/v8/shape.cpp
  src/mapscript/v8/v8_mapscript.cpp
  src/mapv8.cpp
)
include_directories(src/mapscript/v8/)

# point to FlatGeobuf files
set(flatgeobuf_SOURCES
src/flatgeobuf/flatgeobuf_c.cpp
src/flatgeobuf/geometryreader.cpp
src/flatgeobuf/packedrtree.cpp
)
include_directories(src/flatgeobuf/include)

find_package(PCRE2)
set(WITH_PCRE2_DEFAULT OFF)
if(WIN32 AND (NOT DEFINED REGEX_DIR) AND PCRE2_FOUND)
  set(WITH_PCRE2_DEFAULT ON)
endif()
option(WITH_PCRE2 "Use PCRE2 instead of POSIX regex" ${PCRE2_FOUND})
if(WITH_PCRE2 AND NOT PCRE2_FOUND)
    message(FATAL_ERROR "WITH_PCRE2=ON set, but PCRE2 not found")
endif()

#add_definitions(-DHASH_DEBUG=1)
set(REGEX_SOURCES "")

if(WIN32 AND NOT WITH_PCRE2)
  if(NOT DEFINED REGEX_DIR)
      message(FATAL_ERROR "Variable REGEX_DIR should point to regex-0.12 source directory, or PCRE2 library should be available")
  endif()
  set(REGEX_SOURCES ${REGEX_DIR}/regex.c)
  include_directories(${REGEX_DIR})
  add_definitions(-DREGEX_MALLOC -DUSE_GENERIC_MS_NINT -DHAVE_STRING_H)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
  set(REGEX_MALLOC 1)
  set(USE_GENERIC_MS_NINT 1)
  set(HAVE_STRING_H 0)
  if(MSVC)
    # Suppress warnings for regex.c
    set_source_files_properties(${REGEX_SOURCES} PROPERTIES COMPILE_FLAGS /w)
  endif(MSVC)
  if(MINGW)
    # mingw provides stdlib.h
    add_definitions(-DSTDC_HEADERS=1)
  endif(MINGW)
endif()

set(mapserver_SOURCES src/fontcache.c
src/cgiutil.c src/mapgeos.c src/maporaclespatial.c src/mapsearch.c src/mapwms.cpp src/classobject.c
src/mapgml.c src/mapoutput.c src/mapwmslayer.c src/layerobject.c src/mapgraticule.c src/mapows.c src/mapogcapi.cpp
src/mapservutil.c src/mapxbase.c src/maphash.c src/mapowscommon.c src/mapshape.c src/mapxml.c src/mapbits.c
src/maphttp.c src/mapparser.c src/mapstring.cpp src/mapxmp.c src/mapcairo.c src/mapimageio.c
src/mappluginlayer.c src/mapsymbol.c src/mapchart.c src/mapimagemap.c src/mappool.c src/maptclutf.c
src/mapcluster.c src/mapio.c src/mappostgis.cpp src/maptemplate.c src/mapcontext.c src/mapjoin.c
src/mappostgresql.c src/mapthread.c src/mapcopy.c src/maplabel.c src/mapprimitive.cpp src/maptile.c
src/mapcpl.c src/maplayer.c src/mapproject.c src/maptime.c src/mapcrypto.c src/maplegend.c src/hittest.c
src/maptree.c src/mapdebug.c src/maplexer.c src/mapquantization.c src/mapunion.cpp
src/mapdraw.c src/maplibxml2.c src/mapquery.cpp src/maputil.c src/strptime.c src/mapdrawgdal.c
src/mapraster.c src/mapuvraster.cpp src/mapdummyrenderer.c src/mapobject.c src/maprasterquery.c
src/mapwcs.cpp src/maperror.c src/mapogcfilter.cpp src/mapregex.c src/mapwcs11.cpp src/mapfile.c
src/mapogcfiltercommon.cpp src/maprendering.c src/mapwcs20.cpp src/mapogcsld.cpp src/mapmetadata.c
src/mapresample.c src/mapwfs.cpp src/mapgdal.cpp src/mapogcsos.c src/mapscale.c src/mapwfs11.cpp src/mapwfs20.c
src/mapgeomtransform.c src/mapogroutput.cpp src/mapwfslayer.c src/mapagg.cpp src/mapkml.cpp
src/mapgeomutil.cpp src/mapkmlrenderer.cpp src/fontcache.c src/textlayout.c src/maputfgrid.cpp
src/mapogr.cpp src/mapcontour.c src/mapsmoothing.c src/mapv8.cpp ${REGEX_SOURCES} src/kerneldensity.c
src/idw.c src/interpolation.c src/mapflatgeobuf.c src/mapcompositingfilter.c src/mapmvt.c src/mapiconv.c
src/mapgraph.cpp src/mapserv-config.cpp src/maprasterlabel.cpp)

set(mapserver_HEADERS
src/cgiutil.h src/dejavu-sans-condensed.h src/dxfcolor.h src/fontcache.h src/hittest.h src/mapagg.h
src/mapaxisorder.h src/mapcopy.h src/mapentities.h src/maperror.h src/mapfile.h src/mapgml.h src/maphash.h
src/maphttp.h src/mapio.h src/mapkmlrenderer.h src/maplibxml2.h src/mapogcfilter.h src/mapogcsld.h
src/mapoglcontext.h src/mapoglrenderer.h src/mapowscommon.h src/mapows.h src/mapparser.h src/mapogcapi.h
src/mappostgis.h src/mapprimitive.h src/mapproject.h src/mapraster.h src/mapregex.h src/mapresample.h
src/mapserver-api.h src/mapserver.h src/apps/mapserv.h src/mapshape.h src/mapsymbol.h src/maptemplate.h
src/mapthread.h src/maptile.h src/maptime.h src/maptree.h src/maputfgrid.h src/mapwcs.h src/uthash.h src/mapiconv.h
src/mapflatgeobuf.h src/mapgraph.h src/mapserv-config.h)

if(WIN32)
	configure_file(
	  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc.in
	  ${CMAKE_CURRENT_BINARY_DIR}/src/version.rc
	  @ONLY)
	set(mapserver_SOURCES ${mapserver_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/src/version.rc)
endif(WIN32)

if(WITH_PROTOBUFC)
  find_package(ProtobufC)
  if(NOT PROTOBUFC_FOUND)
    report_optional_not_found(PROTOBUFC)
 else(NOT PROTOBUFC_FOUND)
   list(APPEND ALL_INCLUDE_DIRS ${PROTOBUFC_INCLUDE_DIR})

   FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/renderers/mvt)
    # Create custom command for protoc-c
    ADD_CUSTOM_COMMAND(
       COMMAND ${PROTOBUFC_COMPILER}
       ARGS --c_out=${PROJECT_BINARY_DIR}/renderers/mvt --proto_path=${PROJECT_SOURCE_DIR}/src/renderers/mvt ${PROJECT_SOURCE_DIR}/src/renderers/mvt/vector_tile.proto
       OUTPUT ${PROJECT_BINARY_DIR}/renderers/mvt/vector_tile.pb-c.c ${PROJECT_BINARY_DIR}/renderers/mvt/vector_tile.pb-c.h
    )

    SET_SOURCE_FILES_PROPERTIES(${PROJECT_BINARY_DIR}/renderers/mvt/vector_tile.pb-c.h ${PROJECT_BINARY_DIR}/renderers/mvt/vector_tile.pb-c.c GENERATED)
    set (vectortile_SOURCES ${PROJECT_BINARY_DIR}/renderers/mvt/vector_tile.pb-c.c)

    set (USE_PBF 1)
 endif(NOT PROTOBUFC_FOUND)
endif (WITH_PROTOBUFC)


if(BUILD_DYNAMIC)
  add_library(mapserver SHARED ${vectortile_SOURCES} ${mapserver_SOURCES} ${agg_SOURCES} ${v8_SOURCES} ${flatgeobuf_SOURCES})
  set_target_properties( mapserver  PROPERTIES
    VERSION ${MapServer_VERSION_STRING}
    SOVERSION 2
)
endif(BUILD_DYNAMIC)

if(BUILD_STATIC)
  add_library(mapserver_static STATIC ${vectortile_SOURCES} ${mapserver_SOURCES} ${agg_SOURCES} ${v8_SOURCES} ${flatgeobuf_SOURCES})
  set_target_properties( mapserver_static PROPERTIES
    VERSION ${MapServer_VERSION_STRING}
    SOVERSION 2
  )
endif(BUILD_STATIC)

#SOVERSION is not necessarily the same as the
#major version. The rule is that any breakage of the ABI must be
#indicated by incrementing the SOVERSION. So, adding e.g. functions is no
#problem, modifying argument lists or removing functions would required
#the SOVERSION to be incremented. Similar rules hold of course for
#non-opaque data-structures.

if( NOT FUZZER )
add_executable(mapserv src/apps/mapserv.c)
target_link_libraries(mapserv ${MAPSERVER_LIBMAPSERVER})
add_executable(map2img src/apps/map2img.c)
target_link_libraries(map2img ${MAPSERVER_LIBMAPSERVER})
add_executable(shptree src/apps/shptree.c)
target_link_libraries(shptree ${MAPSERVER_LIBMAPSERVER})
add_executable(coshp src/apps/coshp.c)
target_link_libraries(coshp ${MAPSERVER_LIBMAPSERVER})
add_executable(shptreevis src/apps/shptreevis.c)
target_link_libraries(shptreevis ${MAPSERVER_LIBMAPSERVER})
add_executable(sortshp src/apps/sortshp.c)
target_link_libraries(sortshp ${MAPSERVER_LIBMAPSERVER})
add_executable(legend src/apps/legend.c)
target_link_libraries(legend ${MAPSERVER_LIBMAPSERVER})
add_executable(scalebar src/apps/scalebar.c)
target_link_libraries(scalebar ${MAPSERVER_LIBMAPSERVER})
add_executable(msencrypt src/apps/msencrypt.c)
target_link_libraries(msencrypt ${MAPSERVER_LIBMAPSERVER})
add_executable(tile4ms src/apps/tile4ms.c)
target_link_libraries(tile4ms ${MAPSERVER_LIBMAPSERVER})
add_executable(shptreetst src/apps/shptreetst.c)
target_link_libraries(shptreetst ${MAPSERVER_LIBMAPSERVER})
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(USE_EXTENDED_DEBUG 1)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

find_package(PNG)
if(PNG_FOUND)
  include_directories(${PNG_INCLUDE_DIR})
  ms_link_libraries( ${PNG_LIBRARIES})
  list(APPEND ALL_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
  set(USE_PNG 1)
else(PNG_FOUND)
  report_mandatory_not_found(PNG)
endif(PNG_FOUND)

find_package(JPEG)
if(JPEG_FOUND)
  include_directories(${JPEG_INCLUDE_DIR})
  ms_link_libraries( ${JPEG_LIBRARY})
  list(APPEND ALL_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})
  set(USE_JPEG 1)
else(JPEG_FOUND)
  report_mandatory_not_found(JPEG)
endif(JPEG_FOUND)

find_package(Freetype)
if(NOT FREETYPE_FOUND)
  report_mandatory_not_found(FREETYPE)
endif(NOT FREETYPE_FOUND)
include_directories(${FREETYPE_INCLUDE_DIR})
ms_link_libraries( ${FREETYPE_LIBRARY})
list(APPEND ALL_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR})


find_package(Proj)
if(NOT PROJ_FOUND)
    report_mandatory_not_found(PROJ)
endif(NOT PROJ_FOUND)

include_directories(${PROJ_INCLUDE_DIR})
ms_link_libraries( ${PROJ_LIBRARY})
list(APPEND ALL_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})

if(USE_PBF)
    include_directories(${PROJECT_BINARY_DIR}/renderers/mvt)
    include_directories(${PROTOBUFC_INCLUDE_DIR})
    ms_link_libraries( ${PROTOBUFC_LIBRARY})
endif()

if(WITH_PIXMAN)
  find_package(PIXMAN)
  if(PIXMAN_FOUND)
    include_directories(${PIXMAN_INCLUDE_DIR})
    ms_link_libraries(${PIXMAN_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${PIXMAN_INCLUDE_DIR})
    set (USE_PIXMAN 1)
  else(PIXMAN_FOUND)
    report_optional_not_found(PIXMAN)
  endif(PIXMAN_FOUND)
endif (WITH_PIXMAN)

if(WITH_WMS)
  set(USE_WMS_SVR 1)
endif(WITH_WMS)

if(WITH_FRIBIDI)
  find_package(FriBiDi)
  if(NOT FRIBIDI_FOUND)
    report_optional_not_found(FRIBIDI)
 else(NOT FRIBIDI_FOUND)
    include_directories(${FRIBIDI_INCLUDE_DIR})
    ms_link_libraries( ${FRIBIDI_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${FRIBIDI_INCLUDE_DIR})
    set (USE_FRIBIDI 1)
    if(FRIBIDI_LEGACY)
      message(WARNING "Old Fribidi found, upgrade recommended")
    endif(FRIBIDI_LEGACY)
 endif(NOT FRIBIDI_FOUND)
endif (WITH_FRIBIDI)

if(WITH_HARFBUZZ)
  find_package(HarfBuzz)
  if(NOT HARFBUZZ_FOUND)
    report_optional_not_found(HARFBUZZ)
 else(NOT HARFBUZZ_FOUND)
    include_directories(${HARFBUZZ_INCLUDE_DIR})
    ms_link_libraries( ${HARFBUZZ_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${HARFBUZZ_INCLUDE_DIR})
    set (USE_HARFBUZZ 1)
 endif(NOT HARFBUZZ_FOUND)
endif (WITH_HARFBUZZ)

if( USE_HARFBUZZ AND NOT(USE_FRIBIDI) )
  report_dependency_error(HARFBUZZ FRIBIDI)
endif( USE_HARFBUZZ AND NOT(USE_FRIBIDI) )
if( USE_FRIBIDI AND NOT(USE_HARFBUZZ) )
  report_dependency_error(FRIBIDI HARFBUZZ)
endif( USE_FRIBIDI AND NOT(USE_HARFBUZZ) )


if(WITH_ICONV)
  find_package(ICONV)
  if(ICONV_FOUND)
    include_directories(${ICONV_INCLUDE_DIR})
    ms_link_libraries( ${ICONV_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${ICONV_INCLUDE_DIR})
    set (USE_ICONV 1)
  else(ICONV_FOUND)
    report_optional_not_found(ICONV)
  endif(ICONV_FOUND)
endif (WITH_ICONV)

if(WITH_GENERIC_NINT)
   set(USE_GENERIC_MS_NINT 1)
endif(WITH_GENERIC_NINT)


if(WITH_CAIRO)
  find_package(Cairo)
  if(CAIRO_FOUND)
    include_directories(${CAIRO_INCLUDE_DIRS})
    ms_link_libraries( ${CAIRO_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${CAIRO_INCLUDE_DIR})
    set (USE_CAIRO 1)
  else(CAIRO_FOUND)
    report_optional_not_found(CAIRO)
  endif(CAIRO_FOUND)
endif (WITH_CAIRO)

if(WITH_MYSQL)
  find_package(MySQL)
  if(MYSQL_FOUND)
    include_directories(${MYSQL_INCLUDE_DIR})
    ms_link_libraries( ${MYSQL_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${MYSQL_INCLUDE_DIR})
    set (USE_MYSQL 1)
    if (WIN32)
        find_package(Zstd REQUIRED CONFIG)
        include_directories(${ZSTD_INCLUDE_DIRS})
        ms_link_libraries(zstd::libzstd_shared)
    endif (WIN32)
  else(MYSQL_FOUND)
    report_optional_not_found(MYSQL)
  endif(MYSQL_FOUND)
endif (WITH_MYSQL)

if(WITH_ORACLE_PLUGIN AND WITH_ORACLESPATIAL)
  message(SEND_ERROR "WITH_ORACLESPATIAL and WITH_ORACLE_PLUGIN cannot be used simultaneously")
endif(WITH_ORACLE_PLUGIN AND WITH_ORACLESPATIAL)

if(WITH_ORACLESPATIAL OR WITH_ORACLE_PLUGIN)
  if(NOT DEFINED ENV{ORACLE_HOME})
    MESSAGE( SEND_ERROR "ORACLE_HOME environment variable not set, needed for detection")
  endif()
  find_package(ORACLE)
  if(ORACLE_FOUND)
    include_directories(${ORACLE_INCLUDE_DIR})
    list(APPEND ALL_INCLUDE_DIRS ${ORACLE_INCLUDE_DIR})
  else(ORACLE_FOUND)
    report_optional_not_found(ORACLESPATIAL)
    #FIXME: error message here could be misleading, only states ORACLESPATIAL whereas
    #the request could have been for ORACLE_PLUGIN
  endif(ORACLE_FOUND)
endif(WITH_ORACLESPATIAL OR WITH_ORACLE_PLUGIN)

if(ORACLE_FOUND AND WITH_ORACLESPATIAL)
    ms_link_libraries( ${ORACLE_LIBRARY})
    set (USE_ORACLESPATIAL 1)
endif(ORACLE_FOUND AND WITH_ORACLESPATIAL)

if(ORACLE_FOUND AND WITH_ORACLE_PLUGIN)
    add_library(msplugin_oracle MODULE src/maporaclespatial.c)
    target_link_libraries(msplugin_oracle ${ORACLE_LIBRARY} ${MAPSERVER_LIBMAPSERVER})
    set_target_properties(msplugin_oracle PROPERTIES COMPILE_FLAGS -DUSE_ORACLE_PLUGIN)
	set (USE_ORACLE_PLUGIN 1)
endif(ORACLE_FOUND AND WITH_ORACLE_PLUGIN)

if(WITH_MSSQL2008)
  find_package(ODBC)
  if(ODBC_FOUND)
	add_library(msplugin_mssql2008 MODULE src/mapmssql2008.c)
	target_link_libraries(msplugin_mssql2008 ${ODBC_LIBRARY} ${MAPSERVER_LIBMAPSERVER})
    set_target_properties(msplugin_mssql2008 PROPERTIES COMPILE_FLAGS "-DUSE_MSSQL2008_PLUGIN -DUSE_MSSQL2008")
    list(APPEND ALL_INCLUDE_DIRS ${ODBC_INCLUDE_DIR})
	set (USE_MSSQL2008 1)
  else(ODBC_FOUND)
    report_optional_not_found(ODBC)
  endif(ODBC_FOUND)
endif(WITH_MSSQL2008)

if(WITH_FCGI)
  find_package(FCGI)
  if(FCGI_FOUND)
    include_directories(${FCGI_INCLUDE_DIR})
    if( NOT FUZZER )
        target_link_libraries(mapserv ${FCGI_LIBRARY})
    endif()
    list(APPEND ALL_INCLUDE_DIRS ${FCGI_INCLUDE_DIR})
    set (USE_FASTCGI 1)
  else(FCGI_FOUND)
    report_optional_not_found(FCGI)
  endif(FCGI_FOUND)
endif (WITH_FCGI)

if(WITH_GEOS)
  find_package(GEOS)
  if(GEOS_FOUND)
    include_directories(${GEOS_INCLUDE_DIR})
    ms_link_libraries( ${GEOS_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${GEOS_INCLUDE_DIR})
    set (USE_GEOS 1)
  else(GEOS_FOUND)
    report_optional_not_found(GEOS)
  endif(GEOS_FOUND)
endif (WITH_GEOS)

if(WITH_POSTGIS)
  find_package(PostgreSQL)

  # map new module variables to uppercase variables
  set (POSTGRESQL_FOUND ${PostgreSQL_FOUND})
  set (POSTGRESQL_LIBRARY ${PostgreSQL_LIBRARIES})
  set (POSTGRESQL_INCLUDE_DIR ${PostgreSQL_INCLUDE_DIRS})

  if(POSTGRESQL_FOUND)
    #uppercase our variables
    if ( NOT DEFINED POSTGRESQL_LIBRARY )
      set( POSTGRESQL_LIBRARY ${PostgreSQL_LIBRARY})
    endif()
    if ( NOT DEFINED POSTGRESQL_INCLUDE_DIR )
      set( POSTGRESQL_INCLUDE_DIR ${PostgreSQL_INCLUDE_DIR})
    endif()
    include_directories(${POSTGRESQL_INCLUDE_DIR})
    ms_link_libraries( ${POSTGRESQL_LIBRARY})
    CHECK_LIBRARY_EXISTS(pq "PQserverVersion" POSTGRESQL_LIBRARY POSTGIS_HAS_SERVER_VERSION)
    list(APPEND ALL_INCLUDE_DIRS ${POSTGRESQL_INCLUDE_DIR})
    set (USE_POSTGIS 1)
  else(POSTGRESQL_FOUND)
    report_optional_not_found(POSTGIS)
  endif(POSTGRESQL_FOUND)
endif (WITH_POSTGIS)

find_package(GDAL)
if(NOT GDAL_FOUND)
  report_mandatory_not_found(GDAL)
endif(NOT GDAL_FOUND)

include_directories(${GDAL_INCLUDE_DIR})
ms_link_libraries( ${GDAL_LIBRARY})
list(APPEND ALL_INCLUDE_DIRS ${GDAL_INCLUDE_DIR})

if(WITH_SVGCAIRO)
  if(WITH_RSVG)
    message(SEND_ERROR "WITH_SVGCAIRO cannot be enabled with WITH_RSVG")
  endif(WITH_RSVG)
  find_package(SVGCairo)
  if(SVGCAIRO_FOUND)
    include_directories(${SVG_INCLUDE_DIR} ${SVGCAIRO_INCLUDE_DIR})
    ms_link_libraries( ${SVG_LIBRARY} ${SVGCAIRO_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${SVG_INCLUDE_DIR} ${SVGCAIRO_INCLUDE_DIR})
    set (USE_SVG_CAIRO 1)
  else(SVGCAIRO_FOUND)
    report_optional_not_found(SVGCAIRO)
  endif(SVGCAIRO_FOUND)
endif (WITH_SVGCAIRO)

if(WITH_RSVG)
  if(WITH_SVGCAIRO)
    message(SEND_ERROR "WITH_RSVG cannot be enabled with WITH_CAIROSVG")
  endif(WITH_SVGCAIRO)
  find_package(RSVG)
  if(RSVG_FOUND AND GOBJECT_FOUND)
    include_directories(${RSVG_INCLUDE_DIRS})
    ms_link_libraries( ${RSVG_LIBRARY} )
    list(APPEND ALL_INCLUDE_DIRS ${RSVG_INCLUDE_DIRS})
    include_directories(${GOBJECT_INCLUDE_DIRS})
    ms_link_libraries( ${GOBJECT_LIBRARY} )
    list(APPEND ALL_INCLUDE_DIRS ${GOBJECT_INCLUDE_DIRS})
    set (USE_RSVG 1)
  else(RSVG_FOUND AND GOBJECT_FOUND)
    report_optional_not_found(RSVG)
  endif(RSVG_FOUND AND GOBJECT_FOUND)
endif (WITH_RSVG)

if(WITH_CLIENT_WMS OR WITH_CLIENT_WFS)
   set(WITH_CURL ON)
endif(WITH_CLIENT_WMS OR WITH_CLIENT_WFS)

if(WITH_CURL)
   find_package(CURL)
   if(CURL_FOUND)
     include_directories(${CURL_INCLUDE_DIR})
     ms_link_libraries( ${CURL_LIBRARY})
     list(APPEND ALL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
     set(USE_CURL 1)
   else(CURL_FOUND)
     report_optional_not_found(CURL)
   endif(CURL_FOUND)
endif(WITH_CURL)

if(WITH_CLIENT_WMS OR WITH_CLIENT_WFS)
   if(NOT USE_CURL)
      if(WITH_CLIENT_WFS)
        report_dependency_error(CLIENT_WFS CURL)
      endif(WITH_CLIENT_WFS)
      if(WITH_CLIENT_WMS)
        report_dependency_error(CLIENT_WMS CURL)
      endif(WITH_CLIENT_WMS)
   endif(NOT USE_CURL)
endif(WITH_CLIENT_WMS OR WITH_CLIENT_WFS)

if(WITH_CLIENT_WMS)
  set(USE_WMS_LYR 1)
endif(WITH_CLIENT_WMS)

if(WITH_CLIENT_WFS)
  set(USE_WFS_LYR 1)
endif(WITH_CLIENT_WFS)

if(WITH_WFS)
  set(USE_WFS_SVR 1)
endif(WITH_WFS)

if(WITH_WCS)
  set(USE_WCS_SVR 1)
endif(WITH_WCS)

if(WITH_OGCAPI)
  set(USE_OGCAPI_SVR 1)
endif(WITH_OGCAPI)

if(WITH_LIBXML2)
  find_package(LibXml2)
  if(LIBXML2_FOUND)
    include_directories(${LIBXML2_INCLUDE_DIR})
    if(NOT DEFINED LIBXML2_LIBRARY)
      set(LIBXML2_LIBRARY ${LIBXML2_LIBRARIES})
    endif()
    ms_link_libraries( ${LIBXML2_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${LIBXML2_INCLUDE_DIR})
    set (USE_LIBXML2 1)
  else(LIBXML2_FOUND)
    report_optional_not_found(LIBXML2)
  endif(LIBXML2_FOUND)
endif (WITH_LIBXML2)

if( USE_WCS_SVR AND NOT USE_LIBXML2 )
   message(WARNING "WCS 1.1 and 2.0 require libxml2 support but it was not found. WCS 1.1 and 2.0 will not be supported by this build")
endif( USE_WCS_SVR AND NOT USE_LIBXML2 )
if( USE_WFS_SVR AND NOT USE_LIBXML2 )
   message(WARNING "WFS 1.1 and 2.0 require libxml2 support but it was not found. WFS 1.1 and 2.0 will not be supported by this build")
endif( USE_WFS_SVR AND NOT USE_LIBXML2 )

if(WITH_SOS)
  if(USE_LIBXML2)
    set(USE_SOS_SVR 1)
  else(USE_LIBXML2)
    report_dependency_error(SOS LIBXML2)
  endif(USE_LIBXML2)
endif(WITH_SOS)

if(WITH_KML)
  if(USE_LIBXML2)
    set(USE_KML 1)
  else(USE_LIBXML2)
    report_dependency_error(KML LIBXML2)
  endif(USE_LIBXML2)
endif(WITH_KML)

if(WITH_THREAD_SAFETY)
  set( CMAKE_THREAD_PREFER_PTHREAD 1 )
  find_package(Threads)
  if (THREADS_FOUND)
    ms_link_libraries( ${CMAKE_THREAD_LIBS_INIT})
    set(USE_THREAD 1)
  endif (THREADS_FOUND)
endif(WITH_THREAD_SAFETY)

if(WITH_XMLMAPFILE)
   if(NOT USE_LIBXML2)
    report_dependency_error(KML LIBXML2)
   endif(NOT USE_LIBXML2)

  #check for xslt and exslt
  find_package(LibXslt)
  if(LIBXSLT_FOUND)
    if(NOT DEFINED LIBXSLT_LIBRARY)
      set(LIBXSLT_LIBRARY ${LIBXSLT_LIBRARIES})
    endif()
    include_directories(${LIBXSLT_INCLUDE_DIR})
    ms_link_libraries( ${LIBXSLT_LIBRARY} ${LIBXSLT_EXSLT_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${LIBXSLT_INCLUDE_DIR})
    set(USE_XMLMAPFILE 1)
  else(LIBXSLT_FOUND)
    message(SEND_ERROR "Xml Mapfile support requires XSLT support which was not found.
    HINTS:
    - add the libxslt install directory to the CMAKE_PREFIX_PATH variable (-DCMAKE_PREFIX_PATH=\"/path/to/libxslt-install-dir;/path/to/other/dirs\"
    - disable Xml Mapfile support by adding -DWITH_XMLMAPFILE=0"
    )
  endif(LIBXSLT_FOUND)
endif(WITH_XMLMAPFILE)

if(WITH_GIF)
  find_package(GIF)
  if(GIF_FOUND)
    include_directories(${GIF_INCLUDE_DIR})
    ms_link_libraries( ${GIF_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${GIF_INCLUDE_DIR})
    set(USE_GIF 1)
  else(GIF_FOUND)
    report_optional_not_found(GIF)
  endif(GIF_FOUND)
endif(WITH_GIF)

if(WITH_EXEMPI)
  find_package(Exempi)
  if(EXEMPI_FOUND)
    include_directories(${LIBEXEMPI_INCLUDE_DIR})
    ms_link_libraries( ${LIBEXEMPI_LIBRARY})
    list(APPEND ALL_INCLUDE_DIRS ${LIBEXEMPI_INCLUDE_DIR})
    set(USE_EXEMPI 1)
  else(EXEMPI_FOUND)
    report_optional_not_found(EXEMPI)
  endif(EXEMPI_FOUND)
endif(WITH_EXEMPI)

if(WITH_PYTHON)
   add_subdirectory("src/mapscript/python")
   set(USE_PYTHON_MAPSCRIPT 1)
endif(WITH_PYTHON)

if(WITH_V8)
  FIND_PACKAGE(V8)
  IF(V8_FOUND EQUAL 1)
    SET(USE_V8_MAPSCRIPT 1)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${V8_INCLUDE})
    list(APPEND ALL_INCLUDE_DIRS ${V8_INCLUDE})
    MS_LINK_LIBRARIES( ${V8_LIBS})
  ELSE(V8_FOUND)
    MESSAGE(SEND_ERROR "V8 JavaScript support requested but not found.
      HINTS:
      - set V8_ROOT environment variable to the installation path of V8.
      - add the V8 install directory to the CMAKE_PREFIX_PATH variable (-DCMAKE_PREFIX_PATH=\"/path/to/${component}-install-dir;/path/to/other/dirs\") ")
  ENDIF()
ENDIF(WITH_V8)

if(WITH_PCRE2)
  SET(USE_PCRE2 1)
  ms_link_libraries(PCRE2::PCRE2-POSIX)
endif()

if(WITH_PHPNG)
   add_subdirectory("src/mapscript/phpng")
   set(USE_PHPNG_MAPSCRIPT 1)
endif(WITH_PHPNG)

if(WITH_APACHE_MODULE)
   add_subdirectory("src/apache")
   set(USE_APACHE_MODULE 1)
endif(WITH_APACHE_MODULE)

if(WITH_PERL)
   add_subdirectory("src/mapscript/perl")
   set(USE_PERL_MAPSCRIPT 1)
endif(WITH_PERL)

if(WITH_RUBY)
   add_subdirectory("src/mapscript/ruby")
   set(USE_RUBY_MAPSCRIPT 1)
endif(WITH_RUBY)

if(WITH_JAVA)
   add_subdirectory("src/mapscript/java")
   set(USE_JAVA_MAPSCRIPT 1)
endif(WITH_JAVA)

if(WITH_CSHARP)
   add_subdirectory("src/mapscript/csharp")
   set(USE_CSHARP_MAPSCRIPT 1)
endif(WITH_CSHARP)

if(FUZZER OR BUILD_FUZZER_REPRODUCER)
add_subdirectory("fuzzers")
endif()

if(UNIX)
ms_link_libraries( ${CMAKE_DL_LIBS} m )
endif(UNIX)

if (WIN32)
    ms_link_libraries( ${MS_EXTERNAL_LIBS} ws2_32.lib )
    if (MSVC)
        #4701 : Potentially uninitialized local variable 'name' used (https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4701)
        set_target_properties(mapserver PROPERTIES COMPILE_FLAGS "/EHsc /wd4267 /wd4244 /wd4018 /w14701")
    endif(MSVC)
endif (WIN32)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/mapserver-config.h.in"
  "${PROJECT_BINARY_DIR}/mapserver-config.h"
  )

configure_file (
  "${PROJECT_SOURCE_DIR}/src/mapserver-version.h.in"
  "${PROJECT_BINARY_DIR}/mapserver-version.h"
  )

if(BUILD_DYNAMIC)
  set_target_properties(mapserver PROPERTIES
    PUBLIC_HEADER "${mapserver_HEADERS};${PROJECT_BINARY_DIR}/mapserver-config.h;${PROJECT_BINARY_DIR}/mapserver-version.h"
  )

  if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
     target_compile_options(mapserver PRIVATE -Wall -Wextra -Werror=format-security)
  endif()
endif(BUILD_DYNAMIC)

if (NOT("${LIBMAPSERVER_EXTRA_FLAGS}" STREQUAL ""))
  set_source_files_properties(${mapserver_SOURCES} PROPERTIES COMPILE_FLAGS ${LIBMAPSERVER_EXTRA_FLAGS})
endif()

macro(status_optional_component component enabled libpath)
  if("${enabled}" EQUAL "1")
    message(STATUS "  * ${component}: ${libpath}")
  else()
    message(STATUS "  * ${component}: disabled")
  endif()
endmacro()
macro(status_optional_feature feature enabled)
  if("${enabled}" EQUAL "1")
    message(STATUS "  * ${feature}: ENABLED")
  else()
    message(STATUS "  * ${feature}: disabled")
  endif()
endmacro()

message(STATUS "* Summary of configured options for this build")
message(STATUS " * Mandatory components")
message(STATUS "  * GDAL: ${GDAL_LIBRARY}")
message(STATUS "  * PROJ: ${PROJ_LIBRARY}")
message(STATUS "  * png: ${PNG_LIBRARY}")
message(STATUS "  * jpeg: ${JPEG_LIBRARY}")
message(STATUS "  * freetype: ${FREETYPE_LIBRARY}")
message(STATUS " * Optional components")
status_optional_component("PCRE2" "${USE_PCRE2}" "${PCRE2-POSIX_LIBRARY}")
status_optional_component("GIF" "${USE_GIF}" "${GIF_LIBRARY}")
status_optional_component("MYSQL" "${USE_MYSQL}" "${MYSQL_LIBRARY}")
status_optional_component("FRIBIDI" "${USE_FRIBIDI}" "${FRIBIDI_LIBRARY}")
status_optional_component("HARFBUZZ" "${USE_HARFBUZZ}" "${HARFBUZZ_LIBRARY}")
status_optional_component("CAIRO" "${USE_CAIRO}" "${CAIRO_LIBRARY}")
status_optional_component("SVGCAIRO" "${USE_SVG_CAIRO}" "${SVGCAIRO_LIBRARY}")
status_optional_component("RSVG" "${USE_RSVG}" "${RSVG_LIBRARY}")
status_optional_component("CURL" "${USE_CURL}" "${CURL_LIBRARY}")
status_optional_component("PIXMAN" "${USE_PIXMAN}" "${PIXMAN_LIBRARY}")
status_optional_component("LIBXML2" "${USE_LIBXML2}" "${LIBXML2_LIBRARY}")
status_optional_component("POSTGIS" "${USE_POSTGIS}" "${POSTGRESQL_LIBRARY}")
status_optional_component("GEOS" "${USE_GEOS}" "${GEOS_LIBRARY}")
status_optional_component("FastCGI" "${USE_FASTCGI}" "${FCGI_LIBRARY}")
status_optional_component("PROTOBUFC" "${USE_PBF}" "${PROTOBUFC_LIBRARY}")
if(USE_ORACLESPATIAL OR USE_ORACLE_PLUGIN)
  if(USE_ORACLESPATIAL)
    status_optional_component("Oracle Spatial" "${USE_ORACLESPATIAL}" "${ORACLE_LIBRARY}")
  else(USE_ORACLESPATIAL)
    status_optional_component("Oracle Spatial (Built as plugin)" "${USE_ORACLE_PLUGIN}" "${ORACLE_LIBRARY}")
  endif(USE_ORACLESPATIAL)
else(USE_ORACLESPATIAL OR USE_ORACLE_PLUGIN)
  status_optional_component("Oracle Spatial" "" "${ORACLE_LIBRARY}")
endif(USE_ORACLESPATIAL OR USE_ORACLE_PLUGIN)
if(USE_MSSQL2008)
  status_optional_component("MSSQL 2008 (Built as plugin)" "${USE_MSSQL2008}" "${ODBC_LIBRARY}")
endif(USE_MSSQL2008)
status_optional_component("Exempi XMP" "${USE_EXEMPI}" "${LIBEXEMPI_LIBRARY}")
message(STATUS " * Optional features")
status_optional_feature("WMS SERVER" "${USE_WMS_SVR}")
status_optional_feature("WFS SERVER" "${USE_WFS_SVR}")
status_optional_feature("WCS SERVER" "${USE_WCS_SVR}")
status_optional_feature("SOS SERVER" "${USE_SOS_SVR}")
status_optional_feature("OGCAPI SERVER" "${USE_OGCAPI_SVR}")
status_optional_feature("Install HTML Bootstrap resources for OGCAPI" "${INSTALL_HTML_BOOTSTRAP}")
status_optional_feature("WMS CLIENT" "${USE_WMS_LYR}")
status_optional_feature("WFS CLIENT" "${USE_WFS_LYR}")
status_optional_feature("ICONV" "${USE_ICONV}")
status_optional_feature("Thread-safety support" "${USE_THREAD}")
status_optional_feature("KML output" "${USE_KML}")
status_optional_feature("XML Mapfile support" "${USE_XMLMAPFILE}")

message(STATUS " * Mapscripts")
status_optional_feature("Python" "${USE_PYTHON_MAPSCRIPT}")
status_optional_feature("PHPNG" "${USE_PHPNG_MAPSCRIPT}")
status_optional_feature("PERL" "${USE_PERL_MAPSCRIPT}")
status_optional_feature("RUBY" "${USE_RUBY_MAPSCRIPT}")
status_optional_feature("JAVA" "${USE_JAVA_MAPSCRIPT}")
status_optional_feature("C#" "${USE_CSHARP_MAPSCRIPT}")
status_optional_feature("V8 Javascript" "${USE_V8_MAPSCRIPT}")
status_optional_feature("Apache Module (Experimental)" "${USE_APACHE_MODULE}")
status_optional_feature("Python MapScript Annotations" "${WITH_PYMAPSCRIPT_ANNOTATIONS}")

message(STATUS "")
message(STATUS "PROJECT_BINARY_DIR is set to ${PROJECT_BINARY_DIR}")
message(STATUS "Will install files to ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Will install libraries to ${INSTALL_LIB_DIR}")


include_directories("${PROJECT_BINARY_DIR}")

if(WIN32)
set(REGEX_MALLOC 1)
set(USE_GENERIC_MS_NINT 1)

# FIXME: https://stackoverflow.com/a/65321305 this handles older MSVC
# SDKs that are used by Conda Forge windows builds
add_definitions(-D_XKEYCHECK_H=1)
endif(WIN32)


#INSTALL(FILES mapserver-api.h ${PROJECT_BINARY_DIR}/mapserver-version.h DESTINATION include)
if(USE_ORACLE_PLUGIN)
   INSTALL(TARGETS msplugin_oracle DESTINATION ${INSTALL_LIB_DIR})
endif(USE_ORACLE_PLUGIN)

if(USE_MSSQL2008)
   INSTALL(TARGETS msplugin_mssql2008 DESTINATION ${INSTALL_LIB_DIR})
endif(USE_MSSQL2008)

if(NOT FUZZER)
    INSTALL(TARGETS coshp sortshp shptree shptreevis msencrypt legend scalebar tile4ms shptreetst map2img mapserv
            RUNTIME DESTINATION ${INSTALL_BIN_DIR} COMPONENT bin
    )
endif()

if(BUILD_STATIC)
   INSTALL(TARGETS mapserver_static
           DESTINATION ${INSTALL_LIB_DIR} COMPONENT staticlib
   )
endif(BUILD_STATIC)
if(BUILD_DYNAMIC)
   INSTALL(TARGETS mapserver
           EXPORT mapserverTargets
           ARCHIVE DESTINATION ${INSTALL_LIB_DIR} COMPONENT shlib
           LIBRARY DESTINATION ${INSTALL_LIB_DIR} COMPONENT shlib
           PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDE_DIR}/mapserver COMPONENT dev
   )

   # Add all targets to the build-tree export set
   export(TARGETS mapserver
          FILE "${PROJECT_BINARY_DIR}/mapserverTargets.cmake"
   )

   # Export the package for use from the build-tree
   # (this registers the build-tree with a global CMake-registry)
   export(PACKAGE mapserver)

   list(APPEND ALL_INCLUDE_DIRS ${INSTALL_INCLUDE_DIR})
   list(APPEND ALL_INCLUDE_DIRS ${INSTALL_INCLUDE_DIR}/mapserver)

   list(REMOVE_DUPLICATES ALL_INCLUDE_DIRS)

   # Create the mapserver-config.cmake and mapserver-config-version files
   file(RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_CMAKE_DIR}" "${ALL_INCLUDE_DIRS}")

   # ... for the build tree
   set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
   configure_file(src/mapserver-config.cmake.in "${PROJECT_BINARY_DIR}/mapserver-config.cmake" @ONLY)

   # ... for the install tree
   set(CONF_INCLUDE_DIRS "\${MAPSERVER_CMAKE_DIR}/${REL_INCLUDE_DIR}")
   configure_file(src/mapserver-config.cmake.in "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/mapserver-config.cmake" @ONLY)

   # ... for both
   configure_file(src/mapserver-config-version.cmake.in "${PROJECT_BINARY_DIR}/mapserver-config-version.cmake" @ONLY)

   # Install the mapserver-config.cmake and mapserver-config-version.cmake
   install(FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/mapserver-config.cmake"
                 "${PROJECT_BINARY_DIR}/mapserver-config-version.cmake"
           DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
   )

   # Install the export set for use with the install-tree
   install(EXPORT mapserverTargets
           DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
   )
endif(BUILD_DYNAMIC)

if(UNIX)
  include(GNUInstallDirs)
  set(DEFAULT_DATA_SUBDIR ${CMAKE_INSTALL_DATAROOTDIR}/mapserver)
else()
  set(DEFAULT_DATA_SUBDIR share/mapserver)
endif()
set(DEFAULT_CONFIG_FILE ${CMAKE_INSTALL_FULL_SYSCONFDIR}/mapserver.conf)

# Locations are changeable by user to customize layout of MapServer installation
# (default values are platform-specific)
set(MAPSERVER_DATA_SUBDIR ${DEFAULT_DATA_SUBDIR} CACHE STRING
  "Subdirectory where data will be installed")

set(MAPSERVER_CONFIG_FILE "${DEFAULT_CONFIG_FILE}" CACHE STRING
  "Full file name for default configuration file (mapserver.conf)")

# Mark *DIR variables as advanced and dedicated to use by power-users only.
mark_as_advanced(
  MAPSERVER_DATA_SUBDIR
  MAPSERVER_CONFIG_FILE
)
add_definitions(-DMAPSERVER_BUILD)
add_definitions(-DMAPSERVER_CONFIG_FILE="${MAPSERVER_CONFIG_FILE}")

install(
  FILES ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/collection.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/collection-item.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/collection-items.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/collections.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/conformance.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/debug.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/footer.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/header.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/landing.html
        ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-plain/openapi.html
  DESTINATION ${MAPSERVER_DATA_SUBDIR}/ogcapi/templates/html-plain/
)

if(INSTALL_HTML_BOOTSTRAP)
  install(
    FILES ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/collection.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/collection-item.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/collection-items.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/collections.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/conformance.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/debug.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/footer.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/header.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/landing.html
          ${PROJECT_SOURCE_DIR}/share/ogcapi/templates/html-bootstrap/openapi.html
    DESTINATION ${MAPSERVER_DATA_SUBDIR}/ogcapi/templates/html-bootstrap/
  )
endif()

install(
  FILES ${PROJECT_SOURCE_DIR}/etc/mapserver-sample.conf
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/
)

option(BUILD_TESTING "Build unit test" ON)
if(BUILD_DYNAMIC AND BUILD_TESTING)
    enable_testing()
    add_executable(unit_test tests/unit/test.cpp)
    target_link_libraries(unit_test PRIVATE mapserver)
    add_test(NAME unit_test COMMAND unit_test)
endif()
