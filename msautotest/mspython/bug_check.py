#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id: wkt.py 5742 2006-09-05 20:40:04Z frank $
#
# Project:  MapServer
# Purpose:  Regression test for fixed bugs.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
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
# Attempt to verify that bug673 remains fixed.  This bug is trigger if
# a map is drawn such that the layer->project flag gets set to FALSE (no
# reproject needed), but then a subsequent reset of the layer projection
# and new draw map end up not having the reprojection done because the flag
# is still false.
#
# http://trac.osgeo.org/mapserver/ticket/673

def bug_673():

    
    if string.find(mapscript.msGetVersion(),'SUPPORTS=PROJ') == -1:
        return 'skip'

    map = mapscript.mapObj('../misc/ogr_direct.map')

    map.setProjection('+proj=utm +zone=11 +datum=WGS84')

    layer = map.getLayer(0)

    # Draw map without reprojection.

    layer.setProjection('+proj=utm +zone=11 +datum=WGS84')
    img1 = map.draw()

    # Draw map with reprojection

    map.setProjection('+proj=latlong +datum=WGS84')
    map.setExtent(-117.25,43.02,-117.21,43.05)

    img2 = map.draw()
    try:
        os.mkdir('result')
    except:
        pass
    img2.save( 'result/bug673.png' )

    # Verify we got the image we expected ... at least hopefully we didn't
    # get all white which would indicate the bug is back.

    return pmstestlib.compare_and_report( 'bug673.png' )

test_list = [
    bug_673,
    None ]

if __name__ == '__main__':

    pmstestlib.setup_run( 'bug_check' )

    pmstestlib.run_tests( test_list )

    pmstestlib.summarize()

    mapscript.msCleanup()

