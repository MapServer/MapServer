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

import os
import sys

sys.path.append( '../pymod' )
import pmstestlib

import mapscript

###############################################################################
# Attempt to verify that bug673 remains fixed.  This bug is trigger if
# a map is drawn such that the layer->project flag gets set to FALSE (no
# reproject needed), but then a subsequent reset of the layer projection
# and new draw map end up not having the reprojection done because the flag
# is still false.
#
# http://trac.osgeo.org/mapserver/ticket/673

def bug_673():

    if 'SUPPORTS=PROJ' not in mapscript.msGetVersion():
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

###############################################################################
# Test https://github.com/mapserver/mapserver/issues/4943

def test_pattern():

    si = mapscript.styleObj()
    if len(si.pattern) != 0:
        pmstestlib.post_reason('fail 1')
        return 'fail'
    if si.patternlength != 0:
        pmstestlib.post_reason('fail 2')
        return 'fail'

    si.pattern = [2.0,3,4]
    if si.pattern != (2.0, 3.0, 4.0):
        pmstestlib.post_reason('fail 3')
        return 'fail'
    if si.patternlength != 3:
        pmstestlib.post_reason('fail 4')
        return 'fail'

    try:
        si.pattern = [1.0]
        pmstestlib.post_reason('fail 5')
        return 'fail'
    except mapscript.MapServerError:
        pass

    try:
        si.pattern = [i for i in range(11)]
        pmstestlib.post_reason('fail 6')
        return 'fail'
    except mapscript.MapServerError:
        pass

    try:
        si.patternlength = 0
        pmstestlib.post_reason('fail 7')
        return 'fail'
    except mapscript.MapServerError:
        pass

    return 'success'

###############################################################################
# Test reprojection of lines from Polar Stereographic and crossing the antimerdian

def test_reprojection_lines_from_polar_stereographic_to_geographic():

    shape = mapscript.shapeObj( mapscript.MS_SHAPE_LINE )
    line = mapscript.lineObj()
    line.add( mapscript.pointObj( -5554682.77568025, -96957.3485051449 ) ) # -179 40
    line.add( mapscript.pointObj( -5554682.77568025, 96957.3485051449 ) ) # 179 40
    shape.add( line )

    polar_proj = mapscript.projectionObj("+proj=stere +lat_0=90 +lat_ts=60 +lon_0=270 +datum=WGS84")
    longlat_proj = mapscript.projectionObj("+proj=longlat +datum=WGS84")

    if shape.project(polar_proj, longlat_proj) != 0:
        pmstestlib.post_reason('shape.project() failed')
        return 'fail'

    part1 = shape.get(0)
    part2 = shape.get(1)
    if not part1 or not part2:
        pmstestlib.post_reason('should get two parts')
        return 'fail'

    point11 = part1.get(0)
    point12 = part1.get(1)
    point21 = part2.get(0)
    point22 = part2.get(1)
    if abs(point11.x - -179.0) > 1e-7:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    if abs(point12.x - -180.0) > 1e-7:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    if abs(point21.x - 180.0) > 1e-7:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    if abs(point22.x - 179.0) > 1e-7:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    return 'success'

###############################################################################
# Test reprojection of lines from Polar Stereographic and crossing the antimerdian

def test_reprojection_lines_from_polar_stereographic_to_webmercator():

    shape = mapscript.shapeObj( mapscript.MS_SHAPE_LINE )
    line = mapscript.lineObj()
    line.add( mapscript.pointObj( -5554682.77568025, -96957.3485051449 ) ) # -179 40
    line.add( mapscript.pointObj( -5554682.77568025, 96957.3485051449 ) ) # 179 40
    shape.add( line )

    polar_proj = mapscript.projectionObj("+proj=stere +lat_0=90 +lat_ts=60 +lon_0=270 +datum=WGS84")
    longlat_proj = mapscript.projectionObj("init=epsg:3857")

    if shape.project(polar_proj, longlat_proj) != 0:
        pmstestlib.post_reason('shape.project() failed')
        return 'fail'

    part1 = shape.get(0)
    part2 = shape.get(1)
    if not part1 or not part2:
        pmstestlib.post_reason('should get two parts')
        return 'fail'

    point11 = part1.get(0)
    point12 = part1.get(1)
    point21 = part2.get(0)
    point22 = part2.get(1)
    if abs(point11.x - -19926188.85) > 1e-2:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    if abs(point12.x - -20037508.34) > 1e-2:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    if abs(point21.x - 20037508.34) > 1e-2:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    if abs(point22.x - 19926188.85) > 1e-2:
        print(point11.x, point12.x, point21.x, point22.x)
        pmstestlib.post_reason('did not get expected coordinates')
        return 'fail'
    return 'success'

test_list = [
    bug_673,
    test_pattern,
    test_reprojection_lines_from_polar_stereographic_to_geographic,
    test_reprojection_lines_from_polar_stereographic_to_webmercator,
    None ]

if __name__ == '__main__':

    pmstestlib.setup_run( 'bug_check' )

    pmstestlib.run_tests( test_list )

    pmstestlib.summarize()

    mapscript.msCleanup()

