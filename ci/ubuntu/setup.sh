#!/bin/sh
set -eu

# check if postgresql and postgis are installed and remove them
dpkg -l | grep postgresql || /bin/true
dpkg -l | grep postgis || /bin/true
sudo apt-get remove --purge postgresql* libpq-dev libpq5 cmake || /bin/true

# This currently fails as of https://lists.osgeo.org/pipermail/ubuntu/2023-October/002046.html
# sudo add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 6B827C12C2D425E227EDCA75089EBE08314DF160
sudo add-apt-repository -y http://ppa.launchpad.net/ubuntugis/ubuntugis-unstable/ubuntu/
sudo apt-get update
sudo apt-get install -y --allow-unauthenticated build-essential protobuf-c-compiler libprotobuf-c-dev bison flex libfribidi-dev \
            librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev \
            libcairo2-dev libgdal-dev libproj-dev libxml2-dev libexempi-dev lcov lftp postgis libharfbuzz-dev gdal-bin proj-bin ccache curl \
            libpcre2-dev \
            postgresql-server-dev-12 postgresql-12-postgis-3 postgresql-12-postgis-3-scripts g++ ca-certificates

sudo apt-get install -y --allow-unauthenticated libmono-system-drawing4.0-cil mono-mcs
sudo apt-get install -y --allow-unauthenticated libperl-dev
sudo apt-get install -y --allow-unauthenticated openjdk-8-jdk
sudo apt-get install -y --allow-unauthenticated libonig5

#install recent cmake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get install -y --allow-unauthenticated cmake

echo "cmake version"
cmake --version

# upgrade to recent SWIG
git clone https://github.com/swig/swig.git swig-git-master
cd swig-git-master
wget https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.44/pcre2-10.44.tar.gz
./Tools/pcre-build.sh
./autogen.sh
./configure --prefix=/usr
make
sudo make install
sudo ldconfig

#check SWIG version
echo "SWIG version"
swig -version

cd ..
touch src/maplexer.l
touch src/mapparser.y

# report Python locations
which python
which pip

# install Python dependencies (required for msautotests)
pip install --upgrade pip
# pip install cryptography==3.4.6 # avoid requiring rust compiler for the cryptography dependency
pip install pyflakes
pip install -r msautotest/requirements.txt

