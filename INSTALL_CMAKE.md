# CMake Build Instructions

Since version 6.4, MapServer must be built using the [CMake](https://cmake.org) 
build tool, instead of the previous autotools chain. CMake is Open Source, 
free of charge, and is usually included in distribution packages, or can be 
downloaded and compiled with no third party dependencies.
CMake itself does not do the actual compiling of the MapServer source code, it
mainly creates platform specific build files that can then be used by standard
build utilities (make on Unix, Visual Studio on Windows, Xcode on OSX, etc...)

## Install CMake

MapServer requires at least CMake version 3.16, although the CMake process was
first implemented in MapServer 6.4 with CMake 2.6.0.

### Distro Packaged Version

Linux distributions usually include the cmake package, that can be installed
with your usual package manager: apt-get, yum, yast, etc...

### Installing Your Own

Head over to https://cmake.org/download/ to download
a source tarball (for unixes) or a binary installer (for Windows).
If you are building CMake from source, the build process is detailed in the 
tarball readme files, such as

```bash
 $ tar xzf cmake-x.y.z.tar.gz
 $ cd cmake-x.y.z
 $ ./bootstrap
 $ make
 # make install
```

## Creating the MapServer platform specific project with CMake

Although you can run and build from MapServer's source directory as created
by downloading a tarball or using a git clone, it is **highly** recommended
to run "out-of-source" builds, i.e. having all build files be compiled and
created in a different directory than the actual MapServer sources. This allows
to have different configurations running alongside each other (e.g. release and
debug builds, cross-compiling, enabled features, etc...).

### Running CMake From the Command Line

```bash
mkdir build
cd build
cmake ..
## fix dependency issues
make
```

### Running the GUI version of cmake

CMake can be run in graphical mode, in which case the list of available options
are presented in a more user-friendly manner

```bash
mkdir build
cd build
ccmake ..
## follow instructions, fix dependency issues
make
```

### Options and Dependencies

Depending on what packages are available in the default locations of your system,
the previous "cmake .." step will most probably have failed with messages indicating
missing dependencies (by default, MapServer has *many* of those). The error message
that CMake prints out should give you a rather good idea of what steps you should take
next, depending on whether the failed dependency is a feature you require in your build
or not.

- Either disable the dependency by rerunning cmake with -DWITH_DEPENDENCY=0, e.g.
 
  ```bash
  $ cmake .. -DWITH_CAIRO=0
  ```

- Or, if the failed dependency relates to a feature you want built in, and that cmake has
  not been able to find it's installation location, there are 3 possible reasons:

 1 You have not installed the third party package, and/or the third party development
   headers. Use your standard package manager to install the failing package, along
   with it's development headers. The development packages on linux usually end with
   "-dev" or "-devel", e.g. libcairo2-devel , libpng-dev, etc...

   ```bash
   $ (sudo) apt-get install libcairo-dev
   $ cmake ..
   ```
   
 2 You have installed the third party package in a non standard location, which you
   must give to cmake so it can find the required headers and libraries

   ```bash
   $ cmake .. -DCMAKE_PREFIX_PATH=/opt/cairo-1.18.2
   ```

   Cmake expects these nonstandard prefixes to contain standard subdirectories, i.e.
   /opt/cairo-1.18.2/include/cairo.h and /opt/cairo-1.18.2/lib/libcairo.so.
   You can specify multiple prefixes on the cmake command line by separating them with
   the platform specific separator (e.g. ":" on unixes), e.g.

   ```bash
   $ cmake .. -DCMAKE_PREFIX_PATH=/opt/cairo-1.18.2:/opt/freeware
   ```

 3 If you're certain that the packages development headers are installed, and/or that
   you pointed to a valid installation prefix, but cmake is still failing, then there's
   an issue with MapServer's cmake setup, and you can bring this up on the
   mailing list or issue tracker.

## Available Options

Following is a list of option, taken from MapServer's [CMakeLists.txt](https://github.com/MapServer/MapServer/blob/main/CMakeLists.txt) 
configuration file. After the description of the option, the ON/OFF flag states 
if the option is enabled by default (in which case the cmake step will fail if 
the dependency cannot be found). All of these can be enabled or disabled by 
passing `"-DWITH_XXX=0"` or `"-DWITH_XXX=1"` to the `"cmake .."` invocation 
in order to override a default selection.

This Readme file may be out of sync with the actual CMakeLists files shipped.
Refer to the CMakeLists.txt file for up-to-date options.

 - `WITH_PROTOBUFC`: Choose if protocol buffers support should be built in (required for vector tiles) `(ON)`
 - `WITH_KML`: Enable native KML output support (requires libxml2 support) `(OFF)`
 - `WITH_SOS`: Enable SOS Server support (requires PROJ and libxml2 support) `(OFF)`
 - `WITH_WMS`: Enable WMS Server support (requires PROJ support) `(ON)`
 - `WITH_FRIBIDI`: Choose if FriBidi glyph shaping support should be built in (useful for right-to-left languages) (requires HARFBUZZ) `(ON)`
 - `WITH_HARFBUZZ`: Choose if Harfbuzz complex text layout should be included (needed for e.g. arabic and hindi) (requires FRIBIDI) `(ON)`
 - `WITH_ICONV`: Choose if Iconv Internationalization support should be built in `(ON)`
 - `WITH_CAIRO`: Choose if CAIRO  rendering support should be built in (required for SVG and PDF output) `(ON)`
 - `WITH_SVGCAIRO`: Choose if SVG symbology support (via libsvgcairo) should be built in (requires cairo, libsvg, libsvg-cairo. Incompatible with librsvg) `(OFF)`
 - `WITH_RSVG`: Choose if SVG symbology support (via librsvg) should be built in (requires cairo, librsvg. Incompatible with libsvg-cairo) `(OFF)`
 - `WITH_MYSQL`: Choose if MYSQL joining support should be built in `(OFF)`
 - `WITH_FCGI`: Choose if FastCGI support should be built in `(ON)`
 - `WITH_GEOS`: Choose if GEOS geometry operations support should be built in `(ON)`
 - `WITH_POSTGIS`: Choose if Postgis input support should be built in `(ON)`
 - `WITH_CLIENT_WMS`: Enable Client WMS Layer support (requires CURL) `(OFF)`
 - `WITH_CLIENT_WFS`: Enable Client WMS Layer support (requires CURL) `(OFF)`
 - `WITH_CURL`: Enable Curl HTTP support (required for wms/wfs client, and remote SLD) `(OFF)`
 - `WITH_WFS`: Enable WFS Server support (requires PROJ and OGR support) `(ON)`
 - `WITH_WCS`: Enable WCS Server support (requires PROJ and GDAL support) `(ON)`
 - `WITH_OGCAPI`: Enable OGCAPI Server support (requires PROJ and OGR support) `(ON)`
 - `WITH_LIBXML2`: Choose if libxml2 support should be built in (used for sos, wcs 1.1,2.0 and wfs 1.1) `(ON)`
 - `WITH_THREAD_SAFETY`: Choose if a thread-safe version of libmapserver should be built (only recommended for some mapscripts) `(OFF)`
 - `WITH_GIF`: Enable GIF support (for PIXMAP loading) `(ON)`
 - `WITH_PYTHON`: Enable Python mapscript support `(OFF)`
 - `WITH_PHPNG`: Enable PHPNG (SWIG) mapscript support `(OFF)`
 - `WITH_PERL`: Enable Perl mapscript support `(OFF)`
 - `WITH_RUBY`: Enable Ruby mapscript support `(OFF)`
 - `WITH_JAVA`: Enable Java mapscript support `(OFF)`
 - `WITH_CSHARP`: Enable C# mapscript support `(OFF)`
 - `WITH_ORACLESPATIAL`: include oracle spatial database input support `(OFF)`
 - `WITH_ORACLE_PLUGIN`: include oracle spatial database input support as plugin `(OFF)`
 - `WITH_MSSQL2008`: include mssql 2008 database input support as plugin `(OFF)`
 - `WITH_EXEMPI`: include xmp output metadata support `(OFF)`
 - `WITH_XMLMAPFILE`: include native xml mapfile support (requires libxslt/libexslt) `(OFF)`
 - `WITH_V8`: include javascript v8 scripting `(OFF)`
 - `WITH_PIXMAN`: use (experimental) support for pixman for layer compositing operations `(OFF)`
 - `INSTALL_HTML_BOOTSTRAP`: Whether to install HTML Bootstrap resources for OGCAPIs `(ON)`

The following options are for advanced users, i.e. you should not enable them unless
you know what you are doing:

 - `BUILD_STATIC`: Also build a static version of mapserver `(OFF)`
 - `LINK_STATIC_LIBMAPSERVER`: Link to static version of libmapserver (also for mapscripts) `(OFF)`
 - `WITH_APACHE_MODULE`: include (experimental) support for apache module `(OFF)`
 - `WITH_GENERIC_NINT`: generic rounding `(OFF)`
 - `WITH_PYMAPSCRIPT_ANNOTATIONS`: Add annotations to Python mapscript output `(OFF)`
 - `FUZZER`: Build fuzzers using libFuzzer (requires Clang, will disable executable - mapserv, etc. - generation) `(OFF)`
 - `BUILD_FUZZER_REPRODUCER`: Build fuzzer reproducer programs `(ON)`

The following are some common CMake options not specific to MapServer itself:

 - `CMAKE_INSTALL_PREFIX`: path where mapserver binaries and libraries should be installed. Defaults
   to /usr/local on unix.
 - `CMAKE_PREFIX_PATH`: platform-specific separator separated list of prefixes where dependencies will be looked for, e.g.
     ```bash"
     -DCMAKE_PREFIX_PATH=/opt/freeware:/opt/jdk-1.5.6"
     ```
 - `CMAKE_BUILD_TYPE`: Specify the build type. Usually one of 'Debug' or 'Release', e.g.
     ```bash
     "-DCMAKE_BUILD_TYPE=Release"
     or
     "-DCMAKE_BUILD_TYPE=Debug"
     ```

You can find a more extensive list of CMake variables here: https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
