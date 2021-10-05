#!/bin/sh

set -e

apt-get update -y

export BUILD_NAME=PHP_7.3_WITH_PROJ7
#export PYTHON_VERSION=3.6
export PYTHON_VERSION=system

LANG=en_US.UTF-8
export LANG
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    sudo locales tzdata software-properties-common python3-dev python3-pip python3-setuptools git curl \
    apt-transport-https ca-certificates gnupg software-properties-common wget \
    php-dev phpunit && \
    echo "$LANG UTF-8" > /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    update-locale LANG=$LANG

USER=root
export USER

# Install pyenv
curl -L https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash
export PATH="/root/.pyenv/bin:$PATH"
eval "$(pyenv init -)"
eval "$(pyenv virtualenv-init -)"
ln -s /usr/bin/python3 /usr/bin/python
ln -s /usr/bin/pip3 /usr/bin/pip

pip install --upgrade pip

# Install recent cmake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'

cd "$WORK_DIR"

ci/travis/before_install.sh
ci/travis/script.sh

# Validate openapi document
pip install jsonschema
wget https://raw.githubusercontent.com/OAI/OpenAPI-Specification/main/schemas/v3.0/schema.json -O openapi_schema.json
echo "Run jsonschema -i msautotest/api/expected/ogcapi_api.json openapi_schema.json"
jsonschema -i msautotest/api/expected/ogcapi_api.json openapi_schema.json

#####################################
# Test MapServer as CGI and FastCGI #
#####################################

make cmakebuild_nocoverage MFLAGS="-j${nproc}"

ln -s mapserv /tmp/install-mapserver/bin/mapserv.cgi
ln -s mapserv /tmp/install-mapserver/bin/mapserv.fcgi

DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends apache2 libapache2-mod-fcgid
a2enmod fcgid cgid
a2disconf serve-cgi-bin
a2dissite 000-default

echo "ServerName www.example.com" > /etc/apache2/conf-available/servername.conf
a2enconf servername

cat <<EOF >/etc/apache2/sites-available/001-mapserver.conf
<VirtualHost *:80>
 ErrorLog \${APACHE_LOG_DIR}/mapserv-error.log
 CustomLog \${APACHE_LOG_DIR}/mapserv-access.log combined

 ScriptAlias /cgi-bin/ "/tmp/install-mapserver/bin/"
 <Directory "/tmp/install-mapserver/bin">
        AddHandler fcgid-script .fcgi
        AddHandler cgi-script .cgi
        AllowOverride All
        Options +ExecCGI -MultiViews  +FollowSymLinks
        Order Allow,Deny
        Allow from all
        Require all granted
 </Directory>
</VirtualHost>
EOF

a2ensite 001-mapserver

service apache2 restart

# Prepare mapfile and related resources
cat msautotest/wxs/wfs_simple.map | sed "s/\.\/data\/epsg2/\/tmp\/data\/epsg2/" | sed "s/SHAPEPATH \.\/data/SHAPEPATH \/tmp\/data/" | sed "s/ etc/ \/tmp\/etc/" > /tmp/wfs_simple.map
cp -r $WORK_DIR/msautotest/wxs/data /tmp
cp -r $WORK_DIR/msautotest/wxs/etc /tmp

echo "Running CGI query"
curl -s "http://localhost/cgi-bin/mapserv.cgi?MAP=/tmp/wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.xml
cat /tmp/res.xml | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.xml && /bin/false)

echo "Running FastCGI query"
curl -s "http://localhost/cgi-bin/mapserv.fcgi?MAP=/tmp/wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.xml
cat /tmp/res.xml | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.xml && /bin/false)

echo "Running FastCGI query again"
curl -s "http://localhost/cgi-bin/mapserv.fcgi?MAP=/tmp/wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.xml
cat /tmp/res.xml | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.xml && /bin/false)

cd msautotest/wxs

export PATH=/tmp/install-mapserver/bin:$PATH

echo "Check that MS_MAP_NO_PATH works"
MS_MAP_NO_PATH=1 MYMAPFILE=wfs_simple.map mapserv QUERY_STRING="MAP=MYMAPFILE&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAP_NO_PATH cannot be abused with client-controlled env variables"
MS_MAP_NO_PATH=1 CONTENT_TYPE=wfs_simple.map mapserv QUERY_STRING="MAP=CONTENT_TYPE&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep "Web application error" >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAP_PATTERN works (accepting valid MAP)"
MS_MAP_PATTERN="^wfs_simple\.map$" mapserv QUERY_STRING="MAP=wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAP_PATTERN works (rejecting invalid MAP)"
MS_MAP_PATTERN=mypatternmapserv mapserv QUERY_STRING="MAP=wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep "Web application error" >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAPFILE works alone"
MS_MAPFILE=wfs_simple.map mapserv QUERY_STRING="SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that a MAP query parameter isn't accepted when MS_MAPFILE and MS_MAP_NO_PATH are specified"
MS_MAP_NO_PATH=1 MS_MAPFILE=wfs_simple.map mapserv QUERY_STRING="MAP=wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep "Web application error" >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Done !"
