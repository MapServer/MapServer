#!/bin/sh
set -eu

sudo mv /etc/apt/sources.list.d/pgdg* /tmp
dpkg -l | grep postgresql
dpkg -l | grep postgis
sudo apt-get remove --purge postgresql* libpq-dev libpq5
sudo add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
sudo apt-get update
sudo apt-get install --allow-unauthenticated protobuf-c-compiler libprotobuf-c0-dev bison flex libfribidi-dev cmake librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev libgdal-dev libproj-dev libxml2-dev libexempi-dev lcov lftp postgis libharfbuzz-dev gdal-bin ccache curl postgresql-server-dev-10 postgresql-10-postgis-3 postgresql-10-postgis-3-scripts swig
sudo apt-get install --allow-unauthenticated python3-dev python3-pip python3-lxml python3-setuptools python3-pyflakes
sudo apt-get install --allow-unauthenticated libmono-system-drawing4.0-cil mono-mcs
#sudo apt-get install --allow-unauthenticated php-dev
sudo apt-get install --allow-unauthenticated libperl-dev

sudo python3 -m pip install cpp-coveralls
sudo python3 -m pip install pyflakes

export CC="ccache gcc"
export CXX="ccache g++"

sudo python3 -m pip install -U -r msautotest/requirements.txt

sudo sed -i  's/md5/trust/' /etc/postgresql/10/main/pg_hba.conf
sudo sed -i  's/peer/trust/' /etc/postgresql/10/main/pg_hba.conf
sudo service postgresql restart 10

cd msautotest
python3 -m pyflakes .
./create_postgis_test_data.sh
python -m SimpleHTTPServer &> /dev/null &
cd ..
touch maplexer.l
touch mapparser.y
