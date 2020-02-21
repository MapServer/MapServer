#!/bin/sh
set -eu

sudo mv /etc/apt/sources.list.d/pgdg* /tmp
dpkg -l | grep postgresql
dpkg -l | grep postgis
sudo apt-get remove postgresql*
sudo add-apt-repository -y ppa:ubuntugis/ppa
sudo add-apt-repository -y ppa:ubuntugis/ubuntugis-testing
sudo apt-get update
sudo apt-get install --allow-unauthenticated protobuf-c-compiler libprotobuf-c0-dev bison flex python-lxml libfribidi-dev cmake librsvg2-dev colordiff libpq-dev libpng12-dev libjpeg-dev libgif-dev libgeos-dev libgd2-xpm-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev libgdal1-dev libproj-dev libxml2-dev python-dev libexempi-dev lcov lftp postgis libharfbuzz-dev gdal-bin ccache curl pyflakes
sudo apt-get install --allow-unauthenticated libmono-system-drawing4.0-cil mono-mcs
sudo apt-get install --allow-unauthenticated php5-dev || sudo apt-get install --allow-unauthenticated php7-dev
sudo apt-get install --allow-unauthenticated libperl-dev
sudo pip install cpp-coveralls
sudo pip install -U -r msautotest/requirements.txt
# install swig 3.0.12 (defaults to 2.0.11 on trusty)
wget http://prdownloads.sourceforge.net/swig/swig-3.0.12.tar.gz
export CC="ccache gcc"
export CXX="ccache g++"
tar xf swig-3.0.12.tar.gz
cd swig-3.0.12 && ./configure --prefix=/usr && make -j2 && sudo make install
swig -version
cd ..
cd msautotest
pyflakes .
./create_postgis_test_data.sh
python -m SimpleHTTPServer &> /dev/null &
cd ..
touch maplexer.l
touch mapparser.y
