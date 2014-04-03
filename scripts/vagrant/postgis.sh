cd mapserver/msautotest

sudo sed -i  's/md5/trust/' /etc/postgresql/9.1/main/pg_hba.conf
sudo sed -i  's/peer/trust/' /etc/postgresql/9.1/main/pg_hba.conf

sudo service postgresql restart

./create_postgis_test_data.sh
