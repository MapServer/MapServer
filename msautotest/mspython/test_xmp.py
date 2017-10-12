#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Regression test for XMP
# Author:   Even Rouault
#
###############################################################################
#  Copyright (c) 2013, Even Rouault,<even dot rouault at mines-paris dot org>
# 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
###############################################################################

import sys
import string

sys.path.append( '../pymod' )
import pmstestlib

import mapscript
import os

###############################################################################
#

def test_xmp():

    if string.find(mapscript.msGetVersion(),'INPUT=GDAL') == -1:
        return 'skip'
    if string.find(mapscript.msGetVersion(),'SUPPORTS=XMP') == -1:
        return 'skip'

    map = mapscript.mapObj('test_xmp.map')
    layer = map.getLayer(0)
    img = map.draw()
    try:
        os.mkdir('result')
    except:
        pass
    img.save( 'result/test_xmp.png', map )

    f = open('result/test_xmp.png', 'rb')
    data = f.read()
    f.close()

    if data.find('dc:Title="Super Map"') == -1 or \
       data.find('xmpRights:Marked="true"') == -1 or \
       data.find('cc:License="http://creativecommons.org/licenses/by-sa/2.0/"') == -1 or \
       data.find('xmlns:lightroom="http://ns.adobe.com/lightroom/1.0/"') == -1 or \
       data.find('lightroom:PrivateRTKInfo="My Information Here"') == -1  or \
       data.find('exif:ExifVersion="dummy"') == -1  or \
       data.find('xmp:CreatorTool="MapServer"') == -1  or \
       data.find('photoshop:Source="MapServer"') == -1  or \
       data.find('crs:Converter="MapServer"') == -1 or \
       data.find('x:Author="John Doe"') == -1 :
        try:
            from osgeo import gdal
            ds = gdal.Open('result/test_xmp.png')
            print(ds.GetMetadata('xml:XMP'))
            ds = None
        except:
            pass
        return 'fail'

    return 'success'

test_list = [
    test_xmp,
    None ]

if __name__ == '__main__':

    pmstestlib.setup_run( 'test_xmp' )

    pmstestlib.run_tests( test_list )

    pmstestlib.summarize()

    mapscript.msCleanup(0)

