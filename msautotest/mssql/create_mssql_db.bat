set SQLPASSWORD=Password12!
set SERVER=(local)\SQL2019

sqlcmd -S "%SERVER%" -Q "USE [master]; CREATE DATABASE msautotest;"

ogr2ogr -s_srs epsg:26915 -t_srs epsg:26915 -f MSSQLSpatial "MSSQL:server=%SERVER%;database=msautotest;User Id=sa;Password=%SQLPASSWORD%;" "query/data/bdry_counpy2.shp" -nln "bdry_counpy2"
ogr2ogr -s_srs epsg:3857 -t_srs epsg:3857 -f MSSQLSpatial "MSSQL:server=%SERVER%;database=msautotest;User Id=sa;Password=%SQLPASSWORD%;" "renderers/data/cities.shp" -nln "cities"
