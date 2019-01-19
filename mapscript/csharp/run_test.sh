#!/bin/sh -ex
cd ${CSHARP_MAPSCRIPT_SO}
echo .: Testing shpdump.exe :.
mono shpdump.exe ../../../tests/point.shp
echo .: Testing drawmap.exe :.
mono drawmap.exe ../../../tests/test.map ./map.png
echo .: Testing shapeinfo.exe :.
mono shapeinfo.exe ../../../tests/point.shp
echo .: Testing getbytes.exe :.
mono getbytes.exe ../../../tests/test.map test_csharp2.png
echo .: Testing RFC24.exe :.
mono RFC24.exe ../../../tests/test.map
