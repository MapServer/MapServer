#!/bin/sh
set -eu

# Remove pre-installed things in Travis image
if ls /etc/apt/sources.list.d/pgdg* 2>/dev/null >/dev/null; then sudo mv /etc/apt/sources.list.d/pgdg* /tmp; fi
dpkg -l | grep postgresql || /bin/true
dpkg -l | grep postgis || /bin/true
sudo apt-get remove --purge postgresql* libpq-dev libpq5 cmake || /bin/true

#install recent cmake if Travis
if [ -z ${TRAVIS+x} ]; then
    BUILD_ENV="nottravis"
else
    # install recent CMake
    DEPS_DIR="${TRAVIS_BUILD_DIR}/deps"
    mkdir ${DEPS_DIR} && cd ${DEPS_DIR}
    wget --no-check-certificate https://cmake.org/files/v3.23/cmake-3.23.1-linux-x86_64.tar.gz
    tar -xvf cmake-3.23.1-linux-x86_64.tar.gz > /dev/null
    mv cmake-3.23.1-linux-x86_64 cmake-install
    export PATH=${DEPS_DIR}/cmake-install:${DEPS_DIR}/cmake-install/bin:${PATH}
    cd ${TRAVIS_BUILD_DIR}
fi

# check CMake version installed
cmake --version

sudo add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
sudo apt-get update
sudo apt-get install -y --allow-unauthenticated protobuf-c-compiler libprotobuf-c-dev bison flex libfribidi-dev librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev libgdal-dev libproj-dev libxml2-dev libexempi-dev lcov lftp postgis libharfbuzz-dev gdal-bin proj-bin ccache curl postgresql-server-dev-12 postgresql-12-postgis-3 postgresql-12-postgis-3-scripts g++ ca-certificates
# following are already installed on Travis CI
#sudo apt-get install --allow-unauthenticated php-dev python-dev python3-dev
sudo apt-get install -y --allow-unauthenticated libmono-system-drawing4.0-cil mono-mcs
sudo apt-get install -y --allow-unauthenticated libperl-dev
sudo apt-get install -y --allow-unauthenticated openjdk-8-jdk

eval "$(pyenv init --path)"
eval "$(pyenv init -)"

# list installed and available Python/PHP versions
# pyenv versions
# echo $(pyenv root)
phpenv versions

# set the global Python version
pyenv global $PYTHON_VERSION

# check we are using the correct versions
pyenv which pip
pyenv which python

pip install --upgrade pip
pip install cryptography==3.4.6 # avoid requiring rust compiler for the cryptography dependency
pip install cpp-coveralls pyflakes lxml
pip install -r msautotest/requirements.txt

export CC="ccache gcc"
export CXX="ccache g++"

sudo sed -i  's/md5/trust/' /etc/postgresql/12/main/pg_hba.conf
sudo sed -i  's/peer/trust/' /etc/postgresql/12/main/pg_hba.conf
sudo service postgresql restart 12

cd msautotest
python -m pyflakes .
./create_postgis_test_data.sh

if [ $PYTHON_VERSION = "2.7" ]; then
    python -m SimpleHTTPServer &> /dev/null &
else
    # py3
    python -m http.server &> /dev/null &
fi

cd ..
touch maplexer.l
touch mapparser.y
