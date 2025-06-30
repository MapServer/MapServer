CMake Build Instructions
========================

Since version 6.4, MapServer is built with the CMake build tool instead of the
previous autotools chain. CMake is opensource and free of charge and is usually
included in distribution packages, or can be downloaded and compiled with no
third party dependencies.
CMake itself does not do the actual compiling of the MapServer source code, it
mainly creates platform specific build files that can then be used by standard
build utilities (make on unixes, visual studio on windows, xcode on osx, etc...)

Install CMake
-------------

MapServer now requires at least CMake version 3.0, although the CMake process was
first implemented in MapServer 6.4 with CMake 2.6.0.

Distro Packaged Version
.......................

Linux distributions usually include the cmake package, that can be installed
with your usual package manager: apt-get, yum, yast, etc...

Installing Your Own
...................

Head over to http://www.cmake.org/cmake/resources/software.html to download
a source tarball (for unixes) or a binary installer (for windows).
If you are building from source, the build process is detailed in the tarball
readme files, and consists only in

 $ tar xzf cmake-x.y.z.tar.gz
 $ cd cmake-x.y.z
 $ ./bootstrap
 $ make
 # make install


Creating the MapServer platform specific project with CMake
-----------------------------------------------------------

Although you can run and build from MapServer's source directory as created
by downloading a tarball or using a git clone, it is **highly** recommended
to run "out-of-source" builds, i.e. having all build files be compiled and
created in a different directory than the actual MapServer sources. This allows
to have different configurations running alongside each other (e.g. release and
debug builds, cross-compiling, enabled features, etc...).

Running CMake From the Command Line
...................................

mkdir build
cd build
cmake ..
## fix dependency issues
make

Running the GUI version of cmake
................................

CMake can be run in graphical mode, in which case the list of available options
are presented in a more user-friendly manner

mkdir build
cd build
ccmake ..
## follow instructions, fix dependency issues
make

Options and Dependencies
........................

Depending on what packages are available in the default locations of your system,
the previous "cmake .." step will most probably have failed with messages indicating
missing dependencies (by default, MapServer has *many* of those). The error message
that CMake prints out should give you a rather good idea of what steps you should take
next, depending on whether the failed dependency is a feature you require in your build
or not.

- Either disable the dependency by rerunning cmake with -DWITH_DEPENDENCY=0, e.g.

  $ cmake .. -DWITH_CAIRO=0

- Or, if the failed dependency relates to a feature you want built in, and that cmake has
  not been able to find it's installation location, there are 3 possible reasons:

 1 You have not installed the third party package, and/or the third party development
   headers. Use your standard package manager to install the failing package, along
   with it's development headers. The development packages on linux usually end with
   "-dev" or "-devel", e.g. libcairo2-devel , libpng-dev, etc...

   $ (sudo) apt-get install libcairo-dev
   $ cmake ..
   
 2 You have installed the third party package in a non standard location, which you
   must give to cmake so it can find the required headers and libraries

   $ cmake .. -DCMAKE_PREFIX_PATH=/opt/cairo-1.18.2

   Cmake expects these nonstandard prefixes to contain standard subdirectories, i.e.
   /opt/cairo-1.18.2/include/cairo.h and /opt/cairo-1.18.2/lib/libcairo.so.
   You can specify multiple prefixes on the cmake command line by separating them with
   the platform specific separator (e.g. ":" on unixes), e.g.

   $ cmake .. -DCMAKE_PREFIX_PATH=/opt/cairo-1.18.2:/opt/freeware

 3 If you're certain that the packages development headers are installed, and/or that
   you pointed to a valid installation prefix, but cmake is still failing, then there's
   an issue with MapServer's cmake setup, and you can bring this up on the
   mailing list or issue tracker.

Available Options
-----------------

Following is a list of option, taken from MapServer's CMakeLists.txt configuration file.
After the description of the option, the ON/OFF flag states if the option is enabled by
default (in which case the cmake step will fail if the dependency cannot be found). All
of these can be enabled or disabled by passing "-DWITH_XXX=0" or "-DWITH_XXX=1" to the
"cmake .." invocation in order to override a default selection.

This Readme file may be out of sync with the actual CMakeLists files shipped.
Refer to the CMakeLists.txt file for up-to-date options.

 - option(WITH_PROJ "Choose if reprojection support should be built in" ON)
 - option(WITH_KML "Enable native KML output support (requires libxml2 support)" OFF)
 - option(WITH_SOS "Enable SOS Server support (requires PROJ and libxml2 support)" OFF)
 - option(WITH_WMS "Enable WMS Server support (requires proj support)" ON)
 - option(WITH_FRIBIDI "Choose if FriBidi glyph reordering should be enabled (useful for right-to-left languages)(requires harfbuzz)" ON)
 - option(WITH_HARFBUZZ "Choose if Harfbuzz complex script glyph shaping support should be built in (requires fribidi)" ON)
 - option(WITH_ICONV "Choose if Iconv Internationalization support should be built in" ON)
 - option(WITH_CAIRO "Choose if CAIRO  rendering support should be built in (required for SVG and PDF output)" ON)
 - option(WITH_SVGCAIRO "Choose if SVG symbology support (via libsvgcairo) should be built in (requires cairo, libsvg, libsvg-cairo. Incompatible with librsvg)" OFF)
 - option(WITH_RSVG "Choose if SVG symbology support (via librsvg) should be built in (requires cairo, librsvg. Incompatible with libsvg-cairo)" OFF)
 - option(WITH_MYSQL "Choose if MYSQL joining support should be built in" OFF)
 - option(WITH_FCGI "Choose if FastCGI support should be built in" ON)
 - option(WITH_GEOS "Choose if GEOS geometry operations support should be built in" ON)
 - option(WITH_POSTGIS "Choose if Postgis input support should be built in" ON)
 - option(WITH_GDAL "Choose if GDAL input raster support should be built in" ON)
 - option(WITH_OGR "Choose if OGR/GDAL input vector support should be built in" ON)
 - option(WITH_CURL "Enable Curl HTTP support (required for wms/wfs client, remote SLDs and pixmap symbols)" OFF)
 - option(WITH_CLIENT_WMS "Enable Client WMS Layer support (requires CURL and GDAL support)" OFF)
 - option(WITH_CLIENT_WFS "Enable Client WMS Layer support (requires CURL and OGR support)" OFF)
 - option(WITH_WFS "Enable WFS Server support (requires PROJ and OGR support)" ON)
 - option(WITH_WCS "Enable WCS Server support (requires PROJ and GDAL support)" ON)
 - option(WITH_LIBXML2 "Choose if libxml2 support should be built in (used for sos, wcs 1.1,2.0 and wfs 1.1)" ON)
 - option(WITH_THREAD_SAFETY "Choose if a thread-safe version of libmapserver should be built (only recommended for some mapscripts)" OFF)
 - option(WITH_GIF "Enable GIF support (for PIXMAP loading)" ON)
 - option(WITH_PYTHON "Enable Python mapscript support" OFF)
 - option(WITH_PHP "Enable PHP mapscript support" OFF)
 - option(WITH_PERL "Enable Perl mapscript support" OFF)
 - option(WITH_RUBY "Enable Ruby mapscript support" OFF)
 - option(WITH_JAVA "Enable Java mapscript support" OFF)
 - option(WITH_CSHARP "Enable C# mapscript support" OFF)
 - option(WITH_ORACLESPATIAL "include oracle spatial database input support" OFF)
 - option(WITH_ORACLE_PLUGIN "include oracle spatial database input support as plugin" OFF)
 - option(WITH_MSSQL2008 "include mssql 2008 database input support as plugin" OFF)
 - option(WITH_SDE_PLUGIN "include ArcSDE support as a plugin (must specify SDE_INCLUDE_DIR and SDE_LIBRARY_DIR)." OFF)
 - option(WITH_SDE "include ArcSDE support. Add -DSDE_VERSION=91 to use 9.1 arcSDE version" OFF)
 - option(WITH_EXEMPI "include xmp output metadata support" OFF)
 - option(WITH_XMLMAPFILE "include native xml mapfile support (requires libxslt/libexslt)" OFF)
 - option(WITH_V8 "include javascript v8 scripting" OFF)


The following options are for advanced users, i.e. you should not enable them unless
you know what you are doing:

 - option(BUILD_STATIC "Also build a static version of mapserver" OFF)
 - option(LINK_STATIC_LIBMAPSERVER "Link to static version of libmapserver (also for mapscripts)" OFF)
 - option(WITH_APACHE_MODULE "include (experimental) support for apache module" OFF)
 - option(WITH_GENERIC_NINT "generic rounding" OFF)

The following are some common CMake options not specific to MapServer itself:

 - CMAKE_INSTALL_PREFIX : path where mapserver binaries and libraries should be installed. Defaults
   to /usr/local on unix.
 - CMAKE_PREFIX_PATH : platform-specific separator separated list of prefixes where dependencies will be looked for, e.g.
   "-DCMAKE_PREFIX_PATH=/opt/freeware:/opt/jdk-1.5.6"
 - CMAKE_BUILD_TYPE : Specify the build type. Usually one of 'Debug' or 'Release', e.g.
   "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_BUILD_TYPE=Debug"

You can find a more extensive list of cmake variables here: http://www.cmake.org/Wiki/CMake_Useful_Variables
