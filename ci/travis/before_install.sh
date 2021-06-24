#!/bin/sh
set -eu

# Remove pre-installed things in Travis image
if ls /etc/apt/sources.list.d/pgdg* 2>/dev/null >/dev/null; then sudo mv /etc/apt/sources.list.d/pgdg* /tmp; fi
dpkg -l | grep postgresql || /bin/true
dpkg -l | grep postgis || /bin/true
sudo apt-get remove --purge postgresql* libpq-dev libpq5 || /bin/true

sudo add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
sudo apt-get update
sudo apt-get install -y --allow-unauthenticated protobuf-c-compiler libprotobuf-c0-dev bison flex libfribidi-dev cmake librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev libgdal-dev libproj-dev libxml2-dev libexempi-dev lcov lftp postgis libharfbuzz-dev gdal-bin ccache curl postgresql-server-dev-10 postgresql-10-postgis-3 postgresql-10-postgis-3-scripts swig g++
# following are already installed on Travis CI
#sudo apt-get install --allow-unauthenticated php-dev python-dev python3-dev
sudo apt-get install -y --allow-unauthenticated libmono-system-drawing4.0-cil mono-mcs
sudo apt-get install -y --allow-unauthenticated libperl-dev
sudo apt-get install -y --allow-unauthenticated openjdk-8-jdk

pip install cpp-coveralls pyflakes lxml
sudo pip install -U -r msautotest/requirements.txt

export CC="ccache gcc"
export CXX="ccache g++"

sudo sed -i  's/md5/trust/' /etc/postgresql/10/main/pg_hba.conf
sudo sed -i  's/peer/trust/' /etc/postgresql/10/main/pg_hba.conf
sudo service postgresql restart 10

cd msautotest
python -m pyflakes .
./create_postgis_test_data.sh

# py3
python -m http.server &> /dev/null &

cd ..
touch maplexer.l
touch mapparser.y
