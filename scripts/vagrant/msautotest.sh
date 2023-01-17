#!/bin/sh

cd /vagrant/msautotest

# copy custom projection to the PROJ_DATA folder
sudo cp ./wxs/data/epsg2 /usr/share/proj/
