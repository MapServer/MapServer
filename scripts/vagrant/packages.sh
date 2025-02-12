#!/bin/sh

sed -i 's#deb http://us.archive.ubuntu.com/ubuntu/#deb mirror://mirrors.ubuntu.com/mirrors.txt#' /etc/apt/sources.list

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y software-properties-common
add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
apt-get update
apt-get -y upgrade

# install packages we need
apt-get install -q -y git build-essential pkg-config cmake libgeos-dev rake \
    libpq-dev python3-dev python3-pip python3-venv libproj-dev libxml2-dev postgis php-dev \
    postgresql-server-dev-14 postgresql-14-postgis-3 postgresql-14-postgis-3-scripts vim bison flex swig \
    librsvg2-dev libpng-dev libjpeg-dev libgif-dev \
    libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev \
    libgdal-dev libfribidi-dev libexempi-dev \
    libprotobuf-dev libprotobuf-c-dev protobuf-c-compiler libharfbuzz-dev gdal-bin \
    curl sqlite3 libperl-dev python-is-python3

python -m pip install -U -r /vagrant/msautotest/requirements.txt
