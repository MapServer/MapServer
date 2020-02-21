#!/bin/sh

sed -i 's#deb http://us.archive.ubuntu.com/ubuntu/#deb mirror://mirrors.ubuntu.com/mirrors.txt#' /etc/apt/sources.list

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y python-software-properties
add-apt-repository -y ppa:ubuntugis/ppa
add-apt-repository -y ppa:ubuntugis/ubuntugis-testing
add-apt-repository -y ppa:dluxen/cmake-backports
apt-get update
apt-get -y upgrade

# install packages we need
apt-get install -q -y git build-essential pkg-config cmake3 libgeos-dev rake \
    libpq-dev python-all-dev libproj-dev libxml2-dev postgis php5-dev \
    postgresql-server-dev-9.3 postgresql-9.3-postgis-2.2 vim bison flex swig \
    librsvg2-dev libpng12-dev libjpeg-dev libgif-dev \
    libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev \
    libgdal1-dev libfribidi-dev libexempi-dev \
    libprotobuf-dev libprotobuf-c0-dev protobuf-c-compiler libharfbuzz-dev gdal-bin \
    curl sqlite3

curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python get-pip.py
pip install -U -r /vagrant/msautotest/requirements.txt
