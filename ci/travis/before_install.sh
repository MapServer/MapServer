#!/bin/sh
set -eu

sudo mv /etc/apt/sources.list.d/pgdg* /tmp
dpkg -l | grep postgresql
dpkg -l | grep postgis
sudo apt-get remove --purge postgresql* libpq-dev libpq5
sudo add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
sudo apt-get update
sudo apt-get install --allow-unauthenticated protobuf-c-compiler libprotobuf-c0-dev bison flex python-lxml libfribidi-dev cmake librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev libgdal-dev libproj-dev libxml2-dev python-dev libexempi-dev lcov lftp postgis libharfbuzz-dev gdal-bin ccache curl pyflakes postgresql-server-dev-10 postgresql-10-postgis-3 postgresql-10-postgis-3-scripts swig
sudo apt-get install --allow-unauthenticated libmono-system-drawing4.0-cil mono-mcs
#sudo apt-get install --allow-unauthenticated php-dev
sudo apt-get install --allow-unauthenticated libperl-dev
sudo pip install cpp-coveralls
sudo pip install -U -r msautotest/requirements.txt
export CC="ccache gcc"
export CXX="ccache g++"

sudo sed -i  's/md5/trust/' /etc/postgresql/10/main/pg_hba.conf
sudo sed -i  's/peer/trust/' /etc/postgresql/10/main/pg_hba.conf
sudo service postgresql restart 10

cd msautotest
pyflakes .
./create_postgis_test_data.sh
python -m SimpleHTTPServer &> /dev/null &
cd ..
touch maplexer.l
touch mapparser.y
