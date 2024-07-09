#!/bin/sh

if [ "$RUNNER_OS" == "Windows" ]; then
    SWIG=$CONDA_PREFIX/Library/bin/swig
else
    SWIG=$CONDA_PREFIX/bin/swig
fi


cmake -G Ninja                                       \
    -DSWIG_EXECUTABLE=$SWIG                          \
    -DCMAKE_INSTALL_PREFIX:PATH=$CONDA_PREFIX        \
    -DCMAKE_BUILD_TYPE=Release                       \
    -DWITH_APACHE_MODULE=0                           \
    -DWITH_CAIRO=1                                   \
    -DWITH_CLIENT_WFS=1                              \
    -DWITH_CLIENT_WMS=1                              \
    -DWITH_CSHARP=0                                  \
    -DWITH_CURL=1                                    \
    -DWITH_EXEMPI=0                                  \
    -DWITH_FASTCGI=0                                 \
    -DWITH_FCGI=0                                    \
    -DWITH_FREETYPE=1                                \
    -DWITH_FRIBIDI=1                                 \
    -DWITH_GDAL=1                                    \
    -DWITH_GEOS=1                                    \
    -DWITH_GIF=1                                     \
    -DWITH_HARFBUZZ=1                                \
    -DWITH_ICONV=1                                   \
    -DWITH_JAVA=0                                    \
    -DWITH_JPEG=1                                    \
    -DWITH_LIBXML2=1                                 \
    -DWITH_MYSQL=1                                   \
    -DWITH_ORACLESPATIAL=0                           \
    -DWITH_PERL=0                                    \
    -DWITH_PHP=0                                     \
    -DWITH_PIXMAN=1                                  \
    -DWITH_PNG=1                                     \
    -DWITH_POSTGIS=1                                 \
    -DWITH_POSTGRESQL=1                              \
    -DWITH_PROJ=1                                    \
    -DWITH_PROTOBUFC=0                               \
    -DWITH_PYTHON=1                                  \
    -DWITH_RSVG=0                                    \
    -DWITH_SOS=1                                     \
    -DWITH_SVGCAIRO=0                                \
    -DWITH_V8=0                                      \
    -DWITH_XMLMAPFILE=0                              \
    -DWITH_ZLIB=1                                    \
    ..
