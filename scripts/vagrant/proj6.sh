#!/bin/bash

set -eu

NUMTHREADS=$(nproc)
export NUMTHREADS

cd /vagrant
curl -s http://download.osgeo.org/proj/proj-6.1.1.tar.gz > proj-6.1.1.tar.gz
tar xzf proj-6.1.1.tar.gz
rm -rf vagrant-proj
rm -rf /vagrant/install-vagrant-proj-6
mv proj-6.1.1 vagrant-proj
(cd vagrant-proj/data && curl -s http://download.osgeo.org/proj/proj-datumgrid-1.8.tar.gz > proj-datumgrid-1.8.tar.gz && tar xvzf proj-datumgrid-1.8.tar.gz)
(cd vagrant-proj; CFLAGS='-DPROJ_RENAME_SYMBOLS -O2' CXXFLAGS='-DPROJ_RENAME_SYMBOLS -O2' ./configure --disable-static --prefix=/vagrant/install-vagrant-proj-6 && make -j $NUMTHREADS && make -j $NUMTHREADS install)
cp -r /vagrant/install-vagrant-proj-6/include /vagrant/install-vagrant-proj-6/include_proj4_api_only
rm -f /vagrant/install-vagrant-proj-6/include_proj4_api_only/proj.h
