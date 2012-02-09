DROP TABLE eastern_point;
CREATE TABLE eastern_point (id int NOT NULL PRIMARY KEY);
SELECT AddGeometryColumn('eastern_point', 'the_geom', 2393, 'POINT', 2);
INSERT INTO eastern_point VALUES (1, 'SRID=2393;POINT(3449686 7674378)'::geometry);

DROP TABLE wgs84_point;
CREATE TABLE wgs84_point (id int NOT NULL PRIMARY KEY);
SELECT AddGeometryColumn('wgs84_point', 'the_geom', 4326, 'POINT', 2);
INSERT INTO wgs84_point VALUES (1, 'SRID=4326;POINT(25.734088 69.145507)'::geometry);

DROP TABLE utm_point;
CREATE TABLE utm_point (id int NOT NULL PRIMARY KEY);
SELECT AddGeometryColumn('utm_point', 'the_geom', 32631, 'POINT', 2);
INSERT INTO utm_point VALUES (1, 'SRID=32631;POINT(1385320.39262836 7837461.35472656)'::geometry);
