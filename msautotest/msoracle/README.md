# Usage

A docker composition is provided to make the bootstrap of an Oracle Spatial
database easier. This docker image has been stolen from the QGis project.

The composition can be launched using the following command:

```
$ docker-compose up -d
```

Once launched, we still have to load the sample data (also inspired from the
QGis testsuite), using locally:

```
$ docker cp sample-data.sql msoracle_oracle_1:/tmp/
$ docker exec -it msoracle_oracle_1 bash -c \
  "export TNS_ADMIN=/opt/oracle/oradata/dbconfig/XE ; /opt/oracle/product/18c/dbhomeXE/bin/sqlplus -L SYSTEM/adminpass@XE < /tmp/sample-data.sql"
```

