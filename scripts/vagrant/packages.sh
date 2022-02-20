#!/bin/sh

sed -i 's#deb http://us.archive.ubuntu.com/ubuntu/#deb mirror://mirrors.ubuntu.com/mirrors.txt#' /etc/apt/sources.list

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y software-properties-common
add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
apt-get update
apt-get -y upgrade

# Add repo to recent cmake from Kitware (see https://apt.kitware.com/)
sudo apt-get update
sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo apt-get update

# install packages we need
apt-get install -q -y git build-essential pkg-config cmake libgeos-dev rake \
    libpq-dev python3-dev libproj-dev libxml2-dev postgis php-dev \
    postgresql-server-dev-10 postgresql-10-postgis-3 postgresql-10-postgis-3-scripts vim bison flex swig \
    librsvg2-dev libpng-dev libjpeg-dev libgif-dev \
    libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev \
    libgdal-dev libfribidi-dev libexempi-dev \
    libprotobuf-dev libprotobuf-c0-dev protobuf-c-compiler libharfbuzz-dev gdal-bin \
    curl sqlite3 libperl-dev
    
# default to using python3
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 1
python -m pip install -U -r /vagrant/msautotest/requirements.txt

mkdir -p /vagrant/install-vagrant-proj/include_proj4_api_only
cp /usr/include/proj_api.h /vagrant/install-vagrant-proj/include_proj4_api_only
