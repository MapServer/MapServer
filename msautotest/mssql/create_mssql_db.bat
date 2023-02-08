set SQLPASSWORD=Password12!
set SERVER=(local)\SQL2019
set MSSQLSPATIAL_USE_BCP=FALSE

sqlcmd -S "%SERVER%" -Q "USE [master]; CREATE DATABASE msautotest;"

ogr2ogr -a_srs epsg:26915 -f MSSQLSpatial "MSSQL:server=%SERVER%;database=msautotest;UID=sa;PWD=%SQLPASSWORD%;" "query/data/bdry_counpy2.shp" -nln "bdry_counpy2"
ogr2ogr -a_srs epsg:3857 -f MSSQLSpatial "MSSQL:server=%SERVER%;database=msautotest;UID=sa;PWD=%SQLPASSWORD%;" "renderers/data/cities.shp" -nln "cities"
