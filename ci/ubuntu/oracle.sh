#!/bin/bash
# Build MapServer with Oracle Spatial support and run msautotest/msoracle.
# Expects the database from msautotest/msoracle/docker-compose.yaml at
# 127.0.0.1:1521 (see .github/workflows/oracle.yml).

set -e

apt-get update -y

export PYTHON_VERSION=system

LANG=en_US.UTF-8
export LANG
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    sudo locales tzdata software-properties-common python3-dev python3-venv python3-pip python3-setuptools git curl \
    apt-transport-https ca-certificates gnupg wget unzip
echo "$LANG UTF-8" > /etc/locale.gen
dpkg-reconfigure --frontend=noninteractive locales
update-locale LANG=$LANG

USER=root
export USER

# Install pyenv (as in .github/workflows/start.sh)
curl -L https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash
export PATH="/root/.pyenv/bin:$PATH"
eval "$(pyenv init -)"
eval "$(pyenv virtualenv-init -)"
ln -s /usr/bin/python3 /usr/bin/python
pyenv global $PYTHON_VERSION

cd "$WORK_DIR"

ci/ubuntu/setup.sh

# Oracle Instant Client (as in GDAL's Dockerfile.ci)
curl -L -O https://download.oracle.com/otn_software/linux/instantclient/199000/instantclient-basic-linux.x64-19.9.0.0.0dbru.zip
curl -L -O https://download.oracle.com/otn_software/linux/instantclient/199000/instantclient-sdk-linux.x64-19.9.0.0.0dbru.zip
unzip -q instantclient-basic-linux.x64-19.9.0.0.0dbru.zip -d /opt
unzip -q instantclient-sdk-linux.x64-19.9.0.0.0dbru.zip -d /opt
rm instantclient-*.zip
apt-get install -y libaio1t64
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
