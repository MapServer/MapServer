#!/usr/bin/env python
###############################################################################
# Project:  MapServer
# Purpose:  Generate the list of EPSG SRS codes that need axis inversion
# Author:   Even Rouault <even dot rouault at spatialys dot com
#
###############################################################################
# Copyright (c) 2015, Even Rouault <even dot rouault at spatialys dot com
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################

# This requires the EPSG database to be loaded.
# See https://trac.osgeo.org/geotiff/browser/trunk/libgeotiff/csv/README

from osgeo import ogr
ds = ogr.Open('PG:dbname=epsg')

sql_lyr = ds.ExecuteSQL("""SELECT * 
FROM epsg_coordinatereferencesystem
WHERE coord_sys_code IN 
( 
    SELECT CAA.coord_sys_code 
    FROM epsg_coordinateaxis AS CAA 
    INNER JOIN epsg_coordinateaxis AS CAB ON CAA.coord_sys_code = CAB.coord_sys_code 
    WHERE 
       CAA.coord_axis_order=1 AND CAB.coord_axis_order=2
      AND 
      ( 
        ( CAA.coord_axis_orientation ILIKE 'north%' AND CAB.coord_axis_orientation ILIKE 'east%' ) 
       OR
        ( CAA.coord_axis_orientation ILIKE 'south%'  AND CAB.coord_axis_orientation ILIKE 'west%' ) 
      ) 
)  AND coord_ref_sys_code <= 32767
ORDER BY coord_ref_sys_code""")

print('epsg_code')
for f in sql_lyr:
    print(f['coord_ref_sys_code'])


# Could work but GDAL doesn't support 3D geographical SRS at the moment
if False:
    from osgeo import gdal, osr

    sr = osr.SpatialReference()
    print('epsg_code')
    for code in range(32767):
        gdal.PushErrorHandler()
        ret = sr.ImportFromEPSGA(code)
        gdal.PopErrorHandler()
        if ret == 0 and (sr.EPSGTreatsAsLatLong() or sr.EPSGTreatsAsNorthingEasting()):
            print(code)
