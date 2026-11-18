#!/bin/bash
# Build MapServer with Oracle Spatial support and run msautotest/msoracle.
# Expects the database from msautotest/msoracle/docker-compose.yaml at
# 127.0.0.1:1521 (see .github/workflows/oracle.yml).

set -e

export DEBIAN_FRONTEND=noninteractive
LANG=en_US.UTF-8
export LANG

apt-get update -y
apt-get install -y --no-install-recommends \
    locales software-properties-common ca-certificates gnupg curl unzip \
    python3 python3-pip python-is-python3
echo "$LANG UTF-8" > /etc/locale.gen
dpkg-reconfigure --frontend=noninteractive locales
update-locale LANG=$LANG

# install only the libraries needed to build MapServer for these tests
add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
apt-get update -y
apt-get install -y --no-install-recommends \
    build-essential cmake bison flex \
    libgdal-dev libproj-dev libgeos-dev \
    libpng-dev libjpeg-dev libgif-dev libfreetype6-dev libfribidi-dev libharfbuzz-dev \
    libcairo2-dev libcurl4-gnutls-dev libxml2-dev libfcgi-dev libpcre2-dev \
    libprotobuf-c-dev protobuf-c-compiler libaio1t64

cd "$WORK_DIR"

# Python packages for msautotest
python -m pip install --break-system-packages -r msautotest/requirements.txt

# Oracle Instant Client (as in GDAL's Dockerfile.ci)
curl -L -O https://download.oracle.com/otn_software/linux/instantclient/199000/instantclient-basic-linux.x64-19.9.0.0.0dbru.zip
curl -L -O https://download.oracle.com/otn_software/linux/instantclient/199000/instantclient-sdk-linux.x64-19.9.0.0.0dbru.zip
unzip -q instantclient-basic-linux.x64-19.9.0.0.0dbru.zip -d /opt
unzip -q instantclient-sdk-linux.x64-19.9.0.0.0dbru.zip -d /opt
rm instantclient-*.zip
ln -sf /usr/lib/x86_64-linux-gnu/libaio.so.1t64 /usr/lib/x86_64-linux-gnu/libaio.so.1

export ORACLE_HOME=/opt/instantclient_19_9
export LD_LIBRARY_PATH="$ORACLE_HOME${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# minimal build to run the Oracle tests
cmake -S . -B build_oracle \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_ORACLESPATIAL=ON \
    -DWITH_PYTHON=OFF \
    -DWITH_PHPNG=OFF \
    -DWITH_CSHARP=OFF \
    -DWITH_PERL=OFF \
    -DWITH_JAVA=OFF
cmake --build build_oracle -j"$(nproc)"

export PATH="$WORK_DIR/build_oracle:$PATH"
mapserv -v
mapserv -v | grep -q "INPUT=ORACLESPATIAL" || (echo "MapServer was built without Oracle Spatial" && false)

cd msautotest/msoracle
python run_test.py
