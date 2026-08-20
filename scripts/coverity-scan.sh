#!/bin/bash
set -eu  # Exit on error and treat unset variables as errors

# Ensure TOKEN is set
if [ -z "${TOKEN:-}" ]; then
    echo "Error: TOKEN is not set. Make sure to pass it as an environment variable."
    exit 1
fi

# first install necessary packages
apt-get update -y
apt-get install software-properties-common -y

# Install libraries
apt-get update -y
apt-get install -y software-properties-common
add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
apt-get update -y

apt-get install -y --allow-unauthenticated \
    protobuf-c-compiler libprotobuf-c-dev bison flex libfribidi-dev cmake \
    librsvg2-dev colordiff libpq-dev libpng-dev libjpeg-dev libgif-dev \
    libgeos-dev libfreetype6-dev libfcgi-dev libcurl4-gnutls-dev libcairo2-dev \
    libgdal-dev libproj-dev libxml2-dev libexempi-dev lcov lftp postgis \
    libharfbuzz-dev gdal-bin ccache curl \
    postgresql-server-dev-16 postgresql-16-postgis-3 postgresql-16-postgis-3-scripts \
    swig wget git jq

# Build MapServer
cd "${WORK_DIR}"
git config --global --add safe.directory "${WORK_DIR}"

mkdir build
touch src/maplexer.l
touch src/mapparser.y
flex --nounistd -Pmsyy -i -osrc/maplexer.c src/maplexer.l
yacc -d -osrc/mapparser.c src/mapparser.y
cd build
cmake -G "Unix Makefiles" \
    -DWITH_CLIENT_WMS=1 -DWITH_CLIENT_WFS=1 -DWITH_SOS=1 \
    -DWITH_PYTHON=0 -DWITH_JAVA=0 -DWITH_PERL=0 \
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
tar czf cov-int.tgz cov-int

## Below from https://scan.coverity.com/projects/mapserver-mapserver/builds/new

# Initialize a build. Fetch a cloud upload url.
curl -X POST \
  -d version=main \
  -d description="$(git rev-parse --abbrev-ref HEAD) $(git rev-parse --short HEAD)" \
  -d token="$TOKEN" \
  -d email=steve.lime@state.mn.us \
  -d file_name="cov-int.tgz" \
  https://scan.coverity.com/projects/1409/builds/init \
  | tee response

# Store response data to use in later stages.
upload_url=$(jq -r '.url' response)
build_id=$(jq -r '.build_id' response)

# Upload the tarball to the Cloud.
curl -X PUT \
  --header 'Content-Type: application/json' \
  --upload-file cov-int.tgz \
  $upload_url

#  Trigger the build on Scan.
curl -X PUT \
  -d token="$TOKEN" \
  https://scan.coverity.com/projects/1409/builds/$build_id/enqueue
