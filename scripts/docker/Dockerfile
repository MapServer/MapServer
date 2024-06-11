FROM ubuntu:xenial
MAINTAINER Michael Smith <Michael.smith.erdc@gmail.com>

#Setup user
ARG UID
ARG GID
ARG MRSID_DOWNLOAD_URL
RUN addgroup --gid $GID msgroup
RUN adduser --no-create-home --disabled-login msuser  --gecos "" --uid $UID --gid $GID

ENV LIBKML_DOWNLOAD=install-libkml-r864-64bit.tar.gz
ENV FILEGDBAPI_DOWNLOAD=FileGDB_API_1_2-64.tar.gz
ENV MRSID_DIR=MrSID_DSDK-9.5.4.4703-rhel6.x86-64.gcc482
ENV MRSID_DOWNLOAD=MrSID_DSDK-9.5.4.4703-rhel6.x86-64.gcc482.tar.gz

# Setup build env
RUN mkdir /build
RUN apt-key adv --recv-keys --keyserver hkp://keyserver.ubuntu.com:80 16126D3A3E5C1192    \
  && apt-get update && apt-get install -y --fix-missing --no-install-recommends software-properties-common \
  && add-apt-repository ppa:ubuntugis/ubuntugis-unstable -y    \
  && apt-get update && apt-get install -y --fix-missing --no-install-recommends gcc-4.9 g++-4.9  build-essential ca-certificates curl wget git make cmake python-dev \
      python-software-properties software-properties-common  libc6-dev openssh-client libpng12-dev libjpeg-dev libgif-dev liblzma-dev libgeos-dev \
      libproj-dev libxml2-dev libexpat-dev libxerces-c-dev libnetcdf-dev netcdf-bin libpoppler-dev libspatialite-dev swig  \
      libhdf5-serial-dev libpodofo-dev poppler-utils libfreexl-dev libwebp-dev libepsilon-dev libpcre3-dev gfortran libarpack2-dev \
      libpq-dev libflann-dev libhdf5-serial-dev libhdf5-dev libjsoncpp-dev clang-3.8  libhdf4-alt-dev libsqlite3-dev    \
      libltdl-dev libcurl4-openssl-dev ninja python-pip libpng-dev python-dev libprotobuf-c-dev protobuf-c-compiler\
      libboost-filesystem1.58-dev libboost-iostreams1.58-dev libboost-program-options1.58-dev  libboost-system1.58-dev libboost-thread1.58-dev libogdi3.2-dev time \
  && apt-get remove --purge -y $BUILD_PACKAGES  && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 20 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 20

RUN wget --no-verbose http://s3.amazonaws.com/etc-data.koordinates.com/gdal-travisci/${LIBKML_DOWNLOAD} -O /build/${LIBKML_DOWNLOAD} && \
 tar -C /build -xzf /build/${LIBKML_DOWNLOAD} && \
 cp -r /build/install-libkml/include/* /usr/local/include &&  \
 cp -r /build/install-libkml/lib/* /usr/local/lib \
 && rm -Rf /build/install-libkml

RUN curl -L ${$MRSID_DOWNLOAD_URL}/${MRSID_DOWNLOAD} -o /build/${MRSID_DOWNLOAD} && \
  tar -C /build -xzf /build/${MRSID_DOWNLOAD} && \
  cp -r /build/${MRSID_DIR}/Raster_DSDK/include/* /usr/local/include && \
  cp -r /build/${MRSID_DIR}/Raster_DSDK/lib/* /usr/local/lib \
  && rm -Rf /build/${MRSID_DIR}

RUN wget --no-verbose http://s3.amazonaws.com/etc-data.koordinates.com/gdal-travisci/${FILEGDBAPI_DOWNLOAD} -O /build/${FILEGDBAPI_DOWNLOAD} && \
 tar -C /build -xzf /build/${FILEGDBAPI_DOWNLOAD} &&  \
 cp -r /build/FileGDB_API/include/* /usr/local/include && \
 cp -r /build/FileGDB_API/lib/* /usr/local/lib \
 && rm -Rf /build/FileGDB_API

RUN cd /build && curl -LO https://github.com/uclouvain/openjpeg/archive/v2.3.0.tar.gz && \
  tar -zxf /build/v2.3.0.tar.gz && \
  cd /build/openjpeg-2.3.0 && \
  mkdir -v build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. && \
  make -j && make install && \
  rm -Rf /build/openjpeg*

RUN wget https://bootstrap.pypa.io/get-pip.py -O get-pip.py && python get-pip.py && pip install --upgrade pip wheel numpy

ARG GDAL_VERSION
RUN cd /build && \
    git clone https://github.com/OSGeo/gdal.git && \
    cd /build/gdal && \
    git checkout ${GDAL_VERSION} && \
    cd /build/gdal/gdal &&  \
    ./configure --prefix=/usr \
        --with-png=internal \
        --with-jpeg=internal \
        --with-libz=internal \
        --with-libtiff=internal \
        --with-geotiff=internal \
        --with-gif=internal \
        --with-libjson-c=internal \
        --with-poppler \
        --with-spatialite \
        --with-python \
        --with-liblzma \
        --with-openjpeg \
        --with-ogdi \
        --with-webp \
        --with-pg \
        --with-mrsid=/usr/local \
        --with-libkml \
        --with-filegdb \
        --with-hdf5=/usr/lib/x86_64-linux-gnu/hdf5/serial \
        --with-openjpeg && \
    make && \
    make install && \
    cd swig/python &&  python setup.py build && python setup.py install && \
    ldconfig && \
    rm -Rf /build/gdal

RUN apt-get update && apt-get install -y --fix-missing --no-install-recommends build-essential ca-certificates curl wget  \
    subversion git libaio1 make cmake python-numpy python-dev python-software-properties software-properties-common libv8-dev libc6-dev libfreetype6-dev \
    libcairo2-dev libpq-dev libharfbuzz-dev libfribidi-dev flex bison libfcgi-dev libxml2 libxml2-dev bzip2 apache2 apache2-utils apache2-dev \
    libaprutil1-dev libapr1-dev libpng12-dev  libjpeg-dev libcurl4-gnutls-dev libpcre3-dev libpixman-1-dev libgeos-dev libsqlite3-dev libdb-dev libtiff-dev sudo \
  && rm -rf /var/lib/apt/lists/partial/* /tmp/* /var/tmp/*
RUN echo "msuser ALL=NOPASSWD: ALL" >> /etc/sudoers

ARG MAPSERVER_VERSION
RUN cd /build && \
    git clone https://github.com/mapserver/mapserver.git mapserver && \
    cd /build/mapserver && \
    git checkout ${MAPSERVER_VERSION} \
    && mkdir /build/mapserver/build \
    && cd /build/mapserver/build \
    && cmake  \
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
      -DWITH_PROJ=ON \
      -DWITH_SOS=ON  \
      -DWITH_THREAD_SAFETY=ON \
      -DWITH_WCS=ON \
      -DWITH_WFS=ON \
      -DWITH_WMS=ON \
      -DWITH_FCGI=ON \
      -DWITH_FRIBIDI=ON \
      -DWITH_CAIRO=ON \
      -DWITH_HARFBUZZ=ON \
      -DWITH_POSTGIS=on \
      -DWITH_V8=OFF \
      ..  \
    && make  \
    && make install \
    && ldconfig \
    && rm -Rf /build/mapserver

#RUN mkdir /vdatum \
#    && cd /vdatum \
#    && wget http://download.osgeo.org/proj/vdatum/usa_geoid2012.zip && unzip -j -u usa_geoid2012.zip -d /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/usa_geoid2009.zip && unzip -j -u usa_geoid2009.zip -d /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/usa_geoid2003.zip && unzip -j -u usa_geoid2003.zip -d /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/usa_geoid1999.zip && unzip -j -u usa_geoid1999.zip -d /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/vertcon/vertconc.gtx && mv vertconc.gtx /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/vertcon/vertcone.gtx && mv vertcone.gtx /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/vertcon/vertconw.gtx && mv vertconw.gtx /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/egm96_15/egm96_15.gtx && mv egm96_15.gtx /usr/share/proj \
#    && wget http://download.osgeo.org/proj/vdatum/egm08_25/egm08_25.gtx && mv egm08_25.gtx /usr/share/proj \
#    && rm -rf /vdatum

# Update given built libraries dependencies
RUN ldconfig

# Enable these Apache modules
RUN  a2enmod actions cgi alias headers

RUN chmod o+x /usr/local/bin/mapserv
RUN ln -s /usr/local/bin/mapserv /usr/lib/cgi-bin/mapserv
RUN chmod 755 /usr/lib/cgi-bin

ENV HOST_IP `ifconfig | grep inet | grep Mask:255.255.255.0 | cut -d ' ' -f 12 | cut -d ':' -f 2`

CMD sudo service apache2 start && bash

USER msuser
