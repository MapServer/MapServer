##
# mapserver/mapserver
#
# This creates an Ubuntu derived base image that installs the MAPSERVER_VERSION of MapServer and the GDAL_VERSION of GDAL
# Git checkout compiled with needed drivers.
#

# Ubuntu
FROM ubuntu:vivid

MAINTAINER Michael Smith <Michael.smith.erdc@gmail.com>

#Setup user
ARG UID
ARG GID
RUN addgroup --gid $GID msgroup
RUN adduser --no-create-home --disabled-login msuser  --gecos "" --uid $UID --gid $GID

# Setup build env
RUN mkdir /build
RUN apt-key adv --recv-keys --keyserver hkp://keyserver.ubuntu.com:80 16126D3A3E5C1192    \
  && apt-get update && apt-get install -y --fix-missing --no-install-recommends software-properties-common \
  && add-apt-repository ppa:ubuntugis/ubuntugis-unstable -y    \
  && apt-get update && apt-get install -y --fix-missing --no-install-recommends build-essential ca-certificates curl wget git make cmake python-numpy python-dev \
      python-software-properties software-properties-common  libc6-dev openssh-client libpng12-dev libjpeg-dev libgif-dev liblzma-dev libgeos-dev \
      libproj-dev libxml2-dev libexpat-dev libxerces-c-dev libnetcdf-dev netcdf-bin libpoppler-dev libspatialite-dev swig  \
      libhdf5-serial-dev libpodofo-dev poppler-utils libfreexl-dev libwebp-dev libepsilon-dev libpcre3-dev gfortran \
      libpq-dev libhdf5-serial-dev libhdf5-dev libjsoncpp-dev clang  libhdf4-alt-dev libsqlite3-dev    \
      libltdl-dev libcurl4-openssl-dev ninja python-pip libpng-dev  libjasper-dev libdap-dev  ninja libfreetype6-dev \
      unzip libcairo2-dev libpq-dev libharfbuzz-dev libfribidi-dev flex bison libfcgi-dev libxml2 bzip2 apache2 apache2-threaded-dev  apache2-mpm-worker \
  && apt-get remove --purge -y $BUILD_PACKAGES  && rm -rf /var/lib/apt/lists/*

# Getting libKML
ENV LIBKML_DOWNLOAD=install-libkml-r864-64bit.tar.gz
RUN wget http://s3.amazonaws.com/etc-data.koordinates.com/gdal-travisci/${LIBKML_DOWNLOAD} -O /build/${LIBKML_DOWNLOAD}  \
  && tar -C /build -xzf /build/${LIBKML_DOWNLOAD} \
  && cp -r /build/install-libkml/include/* /usr/local/include  \
  && cp -r /build/install-libkml/lib/* /usr/local/lib \
  && rm -Rf /build/install-libkml

ARG GDAL_VERSION
RUN cd /build  \
    && git clone https://github.com/OSGeo/gdal.git  \
    && cd /build/gdal  \
    && git checkout ${GDAL_VERSION}  \
    && cd /build/gdal/gdal \
    && ./configure  \
        --with-png=internal \
        --with-jpeg=internal \
        --with-libz=internal \
        --with-libtiff=internal \
        --with-geotiff=internal \
        --with-gif=internal \
        --with-libjson-c=internal \
        --with-python \
        --with-poppler \
        --with-podofo \
        --with-spatialite \
        --with-liblzma \
        --with-webp \
        --with-pg \
        --with-hdf5 \
        --with-hdf4 \
        --with-webp \
        --with-jasper \
        --with-netcdf \
        --with-ogdi \
        --with-dods-root=/usr \
        --with-openjpeg=yes \
        --with-freexl=yes \
        --with-libkml=yes \
        --with-armadillo=yes \
        --with-liblzma=yes \
        --with-epsilon=/usr \
    && make  \
    && make install  \
    && ldconfig  \
    && rm -Rf /build/gdal

RUN mkdir /vdatum \
    && cd /vdatum \
    && wget http://download.osgeo.org/proj/vdatum/usa_geoid2012.zip && unzip -j -u usa_geoid2012.zip -d /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/usa_geoid2009.zip && unzip -j -u usa_geoid2009.zip -d /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/usa_geoid2003.zip && unzip -j -u usa_geoid2003.zip -d /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/usa_geoid1999.zip && unzip -j -u usa_geoid1999.zip -d /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/vertcon/vertconc.gtx && mv vertconc.gtx /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/vertcon/vertcone.gtx && mv vertcone.gtx /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/vertcon/vertconw.gtx && mv vertconw.gtx /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/egm96_15/egm96_15.gtx && mv egm96_15.gtx /usr/share/proj \
    && wget http://download.osgeo.org/proj/vdatum/egm08_25/egm08_25.gtx && mv egm08_25.gtx /usr/share/proj \
    && rm -rf /vdatum


RUN apt-get install apache2

ARG MAPSERVER_VERSION
RUN cd /build  \
    && git clone https://github.com/mapserver/mapserver.git mapserver  \
    && cd /build/mapserver  \
    && git checkout ${MAPSERVER_VERSION} \
    && mkdir /build/mapserver/build

RUN cd /build/mapserver/build \
    && cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DHARFBUZZ_INCLUDE_DIR=/usr/include/harfbuzz \
      -DWITH_CLIENT_WFS=ON \
      -DWITH_CLIENT_WMS=ON \
      -DWITH_CURL=ON \
      -DWITH_GDAL=ON \
      -DWITH_GIF=ON \
      -DWITH_ICONV=ON \
      -DWITH_KML=ON \
      -DWITH_LIBXML2=ON \
      -DWITH_OGR=ON \
      -DWITH_POINT_Z_M=ON \
      -DWITH_PROJ=ON \
      -DWITH_SOS=ON  \
      -DWITH_THREAD_SAFETY=ON \
      -DWITH_WCS=ON \
      -DWITH_WFS=ON \
      -DWITH_WMS=ON \
      -DWITH_FCGI=ON \
      -DWITH_FRIBIDI=ON \
      -DWITH_CAIRO=ON \
      -DWITH_POSTGRES=ON \
      -DWITH_HARFBUZZ=ON \
      -DWITH_POSTGIS=on \
      ..  \
    && make  \
    && make install \
    && ldconfig \
    && rm -Rf /build/mapserver

# Externally accessible data is by default put in /u02
WORKDIR /data

# Enable these Apache modules
RUN  a2enmod actions cgi alias

RUN chmod o+x /usr/local/bin/mapserv
RUN ln -s /usr/local/bin/mapserv /usr/lib/cgi-bin/mapserv
RUN chmod 755 /usr/lib/cgi-bin

EXPOSE  80

ENV HOST_IP `ifconfig | grep inet | grep Mask:255.255.255.0 | cut -d ' ' -f 12 | cut -d ':' -f 2`

CMD apache2ctl -D FOREGROUND
USER msuser