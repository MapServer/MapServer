#!/bin/sh

cd /vagrant/msautotest

sed -i  's/md5/trust/' /etc/postgresql/16/main/pg_hba.conf
sed -i  's/peer/trust/' /etc/postgresql/16/main/pg_hba.conf

service postgresql restart 16

./create_postgis_test_data.sh
