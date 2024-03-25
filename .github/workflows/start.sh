#!/bin/sh

set -e

apt-get update -y

export PYTHON_VERSION=system

LANG=en_US.UTF-8
export LANG
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    sudo locales tzdata software-properties-common python3-dev python3-pip python3-setuptools git curl \
    apt-transport-https ca-certificates gnupg software-properties-common wget
#install PHP 8.1
DEBIAN_FRONTEND=noninteractive add-apt-repository ppa:ondrej/php -y
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    sudo php8.1-dev php8.1-xml php8.1-mbstring && \
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
# ensure python points to python3
ln -s /usr/bin/python3 /usr/bin/python

export CRYPTOGRAPHY_DONT_BUILD_RUST=1 # to avoid issue when building Cryptography python module

# set the global Python version
pyenv global $PYTHON_VERSION

cd "$WORK_DIR"

ci/setup.sh
ci/build.sh

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

 SetEnv MAPSERVER_CONFIG_FILE ${WORK_DIR}/msautotest/etc/mapserv.conf
 FcgidInitialEnv MAPSERVER_CONFIG_FILE ${WORK_DIR}/msautotest/etc/mapserv.conf

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

echo "Check that we return an error if given no arguments"
curl -s "http://localhost/cgi-bin/mapserv.fcgi" > /tmp/res.xml
cat /tmp/res.xml | grep -q 'QUERY_STRING is set, but empty' || (cat /tmp/res.xml && /bin/false)

echo "Check again to make sure further errors are reported (#6543)"
curl -s "http://localhost/cgi-bin/mapserv.fcgi" > /tmp/res.xml
cat /tmp/res.xml | grep -q 'QUERY_STRING is set, but empty' || (cat /tmp/res.xml && /bin/false)

cd msautotest/wxs

export PATH=/tmp/install-mapserver/bin:$PATH

# Demonstrate that mapserv will error out if cannot find config file
mapserv 2>&1  | grep "msLoadConfig(): Unable to access file" >/dev/null && echo yes
mapserv QUERY_STRING="MAP=wfs_simple.map&REQUEST=GetCapabilities" 2>&1  | grep "msLoadConfig(): Unable to access file" >/dev/null && echo yes
mapserv QUERY_STRING="map=ows_context.map&CONTEXT=ows_context.xml&SERVICE=WMS&VERSION=1.1.1&REQUEST=GetCapabilities"  2>&1  | grep "msLoadConfig(): Unable to access file" >/dev/null && echo "Check that we can't load a OWS context file if MS_CONTEXT_PATTERN is not defined: yes"

echo "Check that MS_MAP_NO_PATH works"
cat <<EOF >/tmp/mapserver.conf
CONFIG
  ENV
    "MS_MAP_NO_PATH" "1"
  END
  MAPS
    "MYMAPFILE" "wfs_simple.map"
  END
END
EOF
# Also demonstrate that mapserv can find config file in ${CMAKE_INSTALL_FULL_SYSCONFDIR}/etc/mapserver.conf by default
ln -s /tmp/mapserver.conf /tmp/install-mapserver/etc
mapserv QUERY_STRING="MAP=MYMAPFILE&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
rm /tmp/install-mapserver/etc/mapserver.conf
cat /tmp/res.txt | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that -conf switch parameter works in a non-CGI context"
mapserv QUERY_STRING="MAP=MYMAPFILE&SERVICE=WFS&REQUEST=GetCapabilities" -conf /tmp/mapserver.conf > /tmp/res.txt
cat /tmp/res.txt | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAP_NO_PATH works (rejecting a value not defined in the MAPS section)"
MAPSERVER_CONFIG_FILE=/tmp/mapserver.conf mapserv QUERY_STRING="MAP=FOO&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep "Web application error" >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAP_PATTERN works (accepting valid MAP)"
cat <<EOF >/tmp/mapserver.conf
CONFIG
  ENV
    "MS_MAP_PATTERN" "^wfs_simple\.map$"
  END
END
EOF
MAPSERVER_CONFIG_FILE=/tmp/mapserver.conf mapserv QUERY_STRING="MAP=wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAP_PATTERN works (rejecting invalid MAP)"
cat <<EOF >/tmp/mapserver.conf
CONFIG
  ENV
    "MS_MAP_PATTERN" "mypatternmapserv"
  END
END
EOF
MAPSERVER_CONFIG_FILE=/tmp/mapserver.conf mapserv QUERY_STRING="MAP=wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep "Web application error" >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that MS_MAPFILE works alone"
cat <<EOF >/tmp/mapserver.conf
CONFIG
  ENV
    "MS_MAPFILE" "wfs_simple.map"
  END
END
EOF
MAPSERVER_CONFIG_FILE=/tmp/mapserver.conf mapserv QUERY_STRING="SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep wfs:WFS_Capabilities >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Check that a MAP query parameter isn't accepted when MS_MAPFILE and MS_MAP_NO_PATH are specified"
cat <<EOF >/tmp/mapserver.conf
CONFIG
  ENV
    "MS_MAPFILE" "wfs_simple.map"
    "MS_MAP_NO_PATH" "1"
  END
END
EOF
MAPSERVER_CONFIG_FILE=/tmp/mapserver.conf mapserv QUERY_STRING="MAP=wfs_simple.map&SERVICE=WFS&REQUEST=GetCapabilities" > /tmp/res.txt
cat /tmp/res.txt | grep "Web application error" >/dev/null || (cat /tmp/res.txt && /bin/false)

echo "Done !"
