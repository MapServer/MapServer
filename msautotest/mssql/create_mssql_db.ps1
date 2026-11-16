# Create the msautotest database and load the MSSQL test data
param (
    [string]$Server = "localhost",
    [string]$Password = "Password12!"
)
$ErrorActionPreference = "Stop"

$env:MSSQLSPATIAL_USE_BCP = "FALSE"

sqlcmd -S $Server -U sa -P $Password -C -Q "USE [master]; CREATE DATABASE msautotest;"
if ($LASTEXITCODE -ne 0) { throw "CREATE DATABASE failed" }

$ogrConn = "MSSQL:DRIVER={ODBC Driver 18 for SQL Server};server=$Server;database=msautotest;UID=sa;PWD=$Password;TrustServerCertificate=yes;"
ogr2ogr -a_srs epsg:26915 -f MSSQLSpatial $ogrConn query/data/bdry_counpy2.shp -nln bdry_counpy2
if ($LASTEXITCODE -ne 0) { throw "Loading bdry_counpy2 failed" }
ogr2ogr -a_srs epsg:3857 -f MSSQLSpatial $ogrConn renderers/data/cities.shp -nln cities
if ($LASTEXITCODE -ne 0) { throw "Loading cities failed" }