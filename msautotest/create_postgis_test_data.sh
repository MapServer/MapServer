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
