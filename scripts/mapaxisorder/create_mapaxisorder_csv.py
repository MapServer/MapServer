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

# To be run with GDAL 3.1

from osgeo import gdal, osr

def inverted_axis(code): 
    gdal.PushErrorHandler()
    ret = sr.ImportFromEPSGA(code)
    gdal.PopErrorHandler()

    if ret == 0 and sr.GetAxesCount() < 2:
        return False

    if sr.IsGeographic() and sr.EPSGTreatsAsLatLong(): 
        return True
    if sr.IsProjected() and sr.EPSGTreatsAsNorthingEasting():
        return True
    
    first_axis = sr.GetAxisOrientation(None, 0)
    second_axis = sr.GetAxisOrientation(None, 1)
    if first_axis == osr.OAO_North and second_axis == osr.OAO_East:
        return True
    if first_axis == osr.OAO_South and second_axis == osr.OAO_West:
        return True
    
    return False

sr = osr.SpatialReference()
print("epsg_code")
for code in range(32767):
    if inverted_axis(code):
        print(code)

