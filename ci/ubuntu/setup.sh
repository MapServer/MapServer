#!/bin/sh
set -eu

# check if postgresql and postgis are installed and remove them
dpkg -l | grep postgresql || /bin/true
dpkg -l | grep postgis || /bin/true
sudo apt-get remove --purge postgresql* libpq-dev libpq5 cmake || /bin/true

# Fix missing Kitware key issue
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo tee /etc/apt/keyrings/kitware-archive-latest.asc > /dev/null
echo 'deb [signed-by=/etc/apt/keyrings/kitware-archive-latest.asc] https://apt.kitware.com/ubuntu noble main' | sudo tee /etc/apt/sources.list.d/kitware.list > /dev/null

# Add required repositories
sudo add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ noble main'

sudo apt-get update

sudo apt-get install -y --allow-unauthenticated build-essential protobuf-c-compiler libprotobuf-c-dev bison flex libfribidi-dev \
            librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev \
            libcairo2-dev libgdal-dev libproj-dev libxml2-dev libexempi-dev lcov lftp postgis libharfbuzz-dev gdal-bin proj-bin ccache curl \
            libpcre2-dev \
            postgresql-server-dev-16 postgresql-16-postgis-3 postgresql-16-postgis-3-scripts g++ ca-certificates \
            libmono-system-drawing4.0-cil mono-mcs \
            libperl-dev \
            openjdk-8-jdk \
            libonig5

# install recent CMake from Kitware
sudo apt-get install -y --allow-unauthenticated cmake

echo "cmake version"
cmake --version

# upgrade to recent SWIG
if [ ! -d "swig-git-master" ]; then
    echo "Cloning SWIG repository..."
    git clone https://github.com/swig/swig.git swig-git-master
else
    echo "swig-git-master already exists, skipping clone."
fi

cd swig-git-master

wget -nc https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.44/pcre2-10.44.tar.gz
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
pip install -r msautotest/requirements.txt --break-system-packages
