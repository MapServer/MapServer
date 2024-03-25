#!/bin/bash

psql -c "drop database if exists msautotest" -U postgres
psql -c "create database msautotest" -U postgres
psql -c "create extension postgis" -d msautotest -U postgres
shp2pgsql -g the_geom query/data/bdry_counpy2.shp bdry_counpy2 | psql -U postgres -d msautotest
shp2pgsql -g the_geom -s 3978 wxs/data/popplace.shp popplace | psql -U postgres -d msautotest
shp2pgsql -g the_geom -s 3978 wxs/data/province.shp province | psql -U postgres -d msautotest
shp2pgsql -g the_geom -s 3978 wxs/data/road.shp road| psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern1.shp pattern1 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern2.shp pattern2 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern3.shp pattern3 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern4.shp pattern4 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern5.shp pattern5 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern6.shp pattern6 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern7.shp pattern7 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern8.shp pattern8 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern9.shp pattern9 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern10.shp pattern10 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern11.shp pattern11 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern12.shp pattern12 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern13.shp pattern13 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern14.shp pattern14 | psql -d msautotest -U postgres
shp2pgsql -s 4326 -g the_geom wxs/data/pattern15.shp pattern15 | psql -d msautotest -U postgres

psql -U postgres -d msautotest -c " 
alter TABLE pattern1 RENAME COLUMN time to time_str;
alter TABLE pattern1 ADD COLUMN time timestamp without time zone;
update pattern1 set time = to_timestamp(time_str,'YYYYMMDD');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern2 RENAME COLUMN time to time_str;
alter TABLE pattern2 ADD COLUMN time timestamp without time zone;
update pattern2 set time = to_timestamp(time_str,'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern3 RENAME COLUMN time to time_str;
alter TABLE pattern3 ADD COLUMN time timestamp without time zone;
update pattern3 set time = to_timestamp(time_str,'YYYY-MM-DD\"T\"HH24:MI:SS');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern4 RENAME COLUMN time to time_str;
alter TABLE pattern4 ADD COLUMN time timestamp without time zone;
update pattern4 set time = to_timestamp(time_str,'YYYY-MM-DD HH24:MI:SS');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern5 RENAME COLUMN time to time_str;
alter TABLE pattern5 ADD COLUMN time timestamp without time zone;
update pattern5 set time = to_timestamp(time_str,'YYYY-MM-DD\"T\"HH24:MI');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern6 RENAME COLUMN time to time_str;
alter TABLE pattern6 ADD COLUMN time timestamp without time zone;
update pattern6 set time = to_timestamp(time_str,'YYYY-MM-DD HH24:MI');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern7 RENAME COLUMN time to time_str;
alter TABLE pattern7 ADD COLUMN time timestamp without time zone;
update pattern7 set time = to_timestamp(time_str,'YYYY-MM-DD\"T\"HH24');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern8 RENAME COLUMN time to time_str;
alter TABLE pattern8 ADD COLUMN time timestamp without time zone;
update pattern8 set time = to_timestamp(time_str,'YYYY-MM-DD HH24');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern9 RENAME COLUMN time to time_str;
alter TABLE pattern9 ADD COLUMN time timestamp without time zone;
update pattern9 set time = to_timestamp(time_str,'YYYY-MM-DD');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern10 RENAME COLUMN time to time_str;
alter TABLE pattern10 ADD COLUMN time timestamp without time zone;
update pattern10 set time = to_timestamp(time_str,'YYYY-MM');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern11 RENAME COLUMN time to time_str;
alter TABLE pattern11 ADD COLUMN time timestamp without time zone;
update pattern11 set time = to_timestamp(time_str,'YYYY');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern12 RENAME COLUMN time to time_str;
alter TABLE pattern12 ADD COLUMN time time without time zone;
update pattern12 set time = to_timestamp(time_str,'\"T\"HH24:MI:SSZ')::time;
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern13 RENAME COLUMN time to time_str;
alter TABLE pattern13 ADD COLUMN time time without time zone;
update pattern13 set time = to_timestamp(time_str,'THH24:MI:SS')::time;
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern14 RENAME COLUMN time to time_str;
alter TABLE pattern14 ADD COLUMN time timestamp without time zone;
update pattern14 set time = to_timestamp(time_str,'YYYY-MM-DD HH24:MI:SSZ');
"
psql -U postgres -d msautotest -c " 
alter TABLE pattern15 RENAME COLUMN time to time_str;
alter TABLE pattern15 ADD COLUMN time timestamp without time zone;
update pattern15 set time = to_timestamp(time_str,'YYYY-MM-DDZ');
"
psql -U postgres -d msautotest -c " 
CREATE TABLE point3d (ID SERIAL PRIMARY KEY);
SELECT AddGeometryColumn('public', 'point3d', 'the_geom', 27700, 'POINT', 3);
INSERT INTO point3d (the_geom) VALUES (GeomFromEWKT('SRID=27700;POINT(1 2 3)'));
"
psql -U postgres -d msautotest -c " 
CREATE TABLE multipoint3d (ID SERIAL PRIMARY KEY);
SELECT AddGeometryColumn('public', 'multipoint3d', 'the_geom', 27700, 'MULTIPOINT', 3);
INSERT INTO multipoint3d (the_geom) VALUES (GeomFromEWKT('SRID=27700;MULTIPOINT(1 2 3,4 5 6)'));
"
psql -U postgres -d msautotest -c " 
CREATE TABLE linestring3d (ID SERIAL PRIMARY KEY);
SELECT AddGeometryColumn('public', 'linestring3d', 'the_geom', 27700, 'LINESTRING', 3);
INSERT INTO linestring3d (the_geom) VALUES (GeomFromEWKT('SRID=27700;LINESTRING(1 2 3,4 5 6)'));
"
psql -U postgres -d msautotest -c " 
CREATE TABLE multilinestring3d (ID SERIAL PRIMARY KEY);
SELECT AddGeometryColumn('public', 'multilinestring3d', 'the_geom', 27700, 'MULTILINESTRING', 3);
INSERT INTO multilinestring3d (the_geom) VALUES (GeomFromEWKT('SRID=27700;MULTILINESTRING((1 2 3,4 5 6),(7 8 9,10 11 12))'));
"
psql -U postgres -d msautotest -c " 
CREATE TABLE polygon3d (ID SERIAL PRIMARY KEY);
SELECT AddGeometryColumn('public', 'polygon3d', 'the_geom', 27700, 'POLYGON', 3);
INSERT INTO polygon3d (the_geom) VALUES (GeomFromEWKT('SRID=27700;POLYGON((0 0 1,10 0 2,10 10 3,0 10 4,0 0 1))'));
INSERT INTO polygon3d (the_geom) VALUES (GeomFromEWKT('SRID=27700;POLYGON((0 0 1,10 0 2,10 10 3,0 10 4,0 0 1),(1 1 2,1 9 3,9 9 4,9 1 5,1 1 2))'));
"
psql -U postgres -d msautotest -c " 
CREATE TABLE multipolygon3d (ID SERIAL PRIMARY KEY);
SELECT AddGeometryColumn('public', 'multipolygon3d', 'the_geom', 27700, 'MULTIPOLYGON', 3);
INSERT INTO multipolygon3d (the_geom) VALUES (GeomFromEWKT('SRID=27700;MULTIPOLYGON(((0 0 1,10 0 2,10 10 3,0 10 4,0 0 1),(1 1 2,1 9 3,9 9 4,9 1 5,1 1 2)),((10 10 0,10 20 1,20 20 2,20 10 3,10 10 0)))'));
"

psql -U postgres -d msautotest -c "
CREATE TABLE test_wfs_paging (ID SERIAL PRIMARY KEY);
SELECT AddGeometryColumn('public', 'test_wfs_paging', 'the_geom', 27700, 'LINESTRING', 2);
INSERT INTO test_wfs_paging (the_geom) VALUES (GeomFromEWKT('SRID=27700;LINESTRING (1 0,0 1)'));
INSERT INTO test_wfs_paging (the_geom) VALUES (GeomFromEWKT('SRID=27700;LINESTRING(0 0,10 10)'));
INSERT INTO test_wfs_paging (the_geom) VALUES (GeomFromEWKT('SRID=27700;LINESTRING(5 2,5 8)'));
"

psql -U postgres -d msautotest -c "
CREATE TABLE text_datatypes (id SERIAL PRIMARY KEY, mychar5 CHAR(5), myvarchar5 VARCHAR(5), mytext TEXT);
SELECT AddGeometryColumn('public', 'text_datatypes', 'the_geom', 27700, 'POINT', 2);
INSERT INTO text_datatypes (the_geom, mychar5, myvarchar5, mytext) VALUES (GeomFromEWKT('SRID=27700;POINT(1 2)'), 'abc  ', 'def  ', 'ghi ');
"

psql -U postgres -d msautotest -c "
CREATE TABLE autotypes (id SERIAL PRIMARY KEY, mychar CHAR(5), myvarchar VARCHAR(5), mytext TEXT, mybool BOOL, myint2 INT2, myint4 INT4, myint8 INT8, 
myfloat4 FLOAT4, myfloat8 FLOAT8, mynumeric NUMERIC(4,2), mydate DATE, mytime TIME, mytimez TIMETZ, mytimestamp TIMESTAMP, mytimestampz TIMESTAMPTZ);
SELECT AddGeometryColumn('public', 'autotypes', 'the_geom', 4326, 'POINT', 2);
INSERT INTO autotypes (the_geom, mychar, myvarchar, mytext, mybool, myint2, myint4, myint8, 
myfloat4, myfloat8, mynumeric, mydate, mytime, mytimez, mytimestamp, mytimestampz) 
VALUES (GeomFromEWKT('SRID=4326;POINT(1 2)'), 'abc', 'def', 'ghi', True, 10, 100, 1000, 
1.5, 2.5, 3.33, '2023-01-01', '00:00:00', '00:00:00', '2023-01-01 00:00:00', '2023-01-01 00:00:00');
"
