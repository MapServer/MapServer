#!/bin/sh

cd /vagrant/msautotest

sed -i  's/md5/trust/' /etc/postgresql/12/main/pg_hba.conf
sed -i  's/peer/trust/' /etc/postgresql/12/main/pg_hba.conf

service postgresql restart 12

./create_postgis_test_data.sh
