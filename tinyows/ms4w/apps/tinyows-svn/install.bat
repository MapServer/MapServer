@echo off
set PG_SHARE="C:\Program Files\PostgreSQL\8.3\share\contrib"
set /p PG_SHARE=[Enter path to Postgresql share directory(default=C:\Program Files\PostgreSQL\8.3\share\contrib)]:
echo PG_SHARE is set to: %PG_SHARE%
echo on

set DB=tinyows_demo

echo "Create Spatial Database: %DB%"
dropdb.exe -U postgres %DB%
createdb.exe -U postgres %DB% 
createlang.exe -U postgres plpgsql %DB%
psql.exe -U postgres -d %DB% -f %PG_SHARE%\lwpostgis.sql
psql.exe -U postgres -d %DB% -f %PG_SHARE%\spatial_ref_sys.sql

echo "Import layer data: world" 
shp2pgsql.exe -s 4326 -I demo\world.shp world > world.sql
psql.exe -U postgres -d %DB% -f world.sql
echo "Import layer data: france_dept" 
shp2pgsql.exe -s 27582 -I -W latin1 demo\france_dept.shp france_dept > france_dept.sql
psql.exe -U postgres -d %DB% -f france_dept.sql
