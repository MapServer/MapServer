#!/bin/sh -ex
cd ${CSHARP_MAPSCRIPT_SO}
echo .: Testing shpdump.exe :.
LC_ALL=C mono shpdump.exe ../../../../tests/point.shp
echo .: Testing drawmap.exe :.
LC_ALL=C mono drawmap.exe ../../../../tests/test.map ./map.png
echo .: Testing shapeinfo.exe :.
LC_ALL=C mono shapeinfo.exe ../../../../tests/point.shp
echo .: Testing getbytes.exe :.
LC_ALL=C mono getbytes.exe ../../../../tests/test.map test_csharp2.png
echo .: Testing RFC24.exe :.
LC_ALL=C mono RFC24.exe ../../../../tests/test.map
