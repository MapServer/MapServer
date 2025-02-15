#!/bin/sh
set -eu  # Exit on error and treat unset variables as errors

# Ensure TOKEN is set
if [ -z "${TOKEN:-}" ]; then
    echo "Error: TOKEN is not set. Make sure to pass it as an environment variable."
    exit 1
fi

# Install libraries
add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
apt-get update -y

apt-get install --allow-unauthenticated \
    protobuf-c-compiler libprotobuf-c-dev bison flex libfribidi-dev cmake \
    librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev \
    libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev \
    libgdal-dev libproj-dev libxml2-dev libexempi-dev lcov lftp postgis \
    libharfbuzz-dev gdal-bin ccache curl \
    postgresql-server-dev-12 postgresql-12-postgis-3 postgresql-12-postgis-3-scripts \
    swig

# Build MapServer
mkdir build
touch src/maplexer.l
touch src/mapparser.y
flex --nounistd -Pmsyy -i -osrc/maplexer.c src/maplexer.l
yacc -d -osrc/mapparser.c src/mapparser.y
cd build
cmake -G "Unix Makefiles" \
    -DWITH_CLIENT_WMS=1 -DWITH_CLIENT_WFS=1 -DWITH_SOS=1 \
    -DWITH_PYTHON=0 -DWITH_JAVA=0 -DWITH_PHP=0 -DWITH_PERL=0 \
    -DWITH_FCGI=0 -DWITH_EXEMPI=1 -DWITH_KML=1 -DWITH_THREAD_SAFETY=1 \
    -DWITH_RSVG=1 -DWITH_CURL=1 -DWITH_FRIBIDI=1 -DWITH_HARFBUZZ=1 \
    ..

# Download Coverity Build Tool
wget -q "https://scan.coverity.com/download/cxx/linux64" \
     --post-data "token=$TOKEN&project=mapserver%2Fmapserver" \
     -O cov-analysis-linux64.tar.gz
mkdir -p cov-analysis-linux64
tar xzf cov-analysis-linux64.tar.gz --strip 1 -C cov-analysis-linux64

# Build with cov-build
export PATH=`pwd`/cov-analysis-linux64/bin:$PATH
cov-build --dir cov-int make

# Submit to Coverity Scan
curl --silent \
    --form project=mapserver%2Fmapserver \
    --form token="$TOKEN" \
    --form email=steve.lime@state.mn.us \
    --form file=@mapserver.tgz \
    --form version=main \
    --form description="$(git rev-parse --abbrev-ref HEAD) $(git rev-parse --short HEAD)" \
    https://scan.coverity.com/builds?project=mapserver%2Fmapserver
