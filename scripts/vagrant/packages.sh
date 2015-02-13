#!/bin/sh

sed -i 's#deb http://us.archive.ubuntu.com/ubuntu/#deb mirror://mirrors.ubuntu.com/mirrors.txt#' /etc/apt/sources.list

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y python-software-properties
add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
apt-get update
apt-get -y upgrade

# install packages we need
apt-get install -q -y git build-essential pkg-config cmake libgeos-dev rake \
    libpq-dev python-all-dev libproj-dev libxml2-dev postgis php5-dev \
    postgresql-server-dev-9.1 postgresql-9.1-postgis-2.1 vim bison flex swig \
    librsvg2-dev libpng12-dev libjpeg-dev libgif-dev \
    libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev \
    libgdal1-dev libfribidi-dev libexempi-dev
