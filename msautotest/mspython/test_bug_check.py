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
import pytest

import pmstestlib

mapscript_available = False
try:
    import mapscript
    mapscript_available = True
except ImportError:
    pass

pytestmark = pytest.mark.skipif(not mapscript_available, reason="mapscript not available")

def get_relpath_to_this(filename):
    return os.path.join(os.path.dirname(__file__), filename)

###############################################################################
# Attempt to verify that bug673 remains fixed.  This bug is trigger if
# a map is drawn such that the layer->project flag gets set to FALSE (no
# reproject needed), but then a subsequent reset of the layer projection
# and new draw map end up not having the reprojection done because the flag
# is still false.
#
# http://trac.osgeo.org/mapserver/ticket/673

def test_bug_673():

    map = mapscript.mapObj(get_relpath_to_this('../misc/ogr_direct.map'))

    map.setProjection('+proj=utm +zone=11 +datum=WGS84')

    layer = map.getLayer(0)

    # Draw map without reprojection.

    layer.setProjection('+proj=utm +zone=11 +datum=WGS84')
    map.draw()

    # Draw map with reprojection

    map.setProjection('+proj=latlong +datum=WGS84')
    map.setExtent(-117.25,43.02,-117.21,43.05)

    img2 = map.draw()
    try:
        os.mkdir(get_relpath_to_this('result'))
    except:
        pass
    img2.save( get_relpath_to_this('result/bug673.png') )

    # Verify we got the image we expected ... at least hopefully we didn't
    # get all white which would indicate the bug is back.

    pmstestlib.compare_and_report( 'bug673.png', this_path = os.path.dirname(__file__) )


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

    assert shape.project(polar_proj, longlat_proj) == 0

    part1 = shape.get(0)
    assert part1
    part2 = shape.get(1)
    assert part2

    point11 = part1.get(0)
    point12 = part1.get(1)
    point21 = part2.get(0)
    point22 = part2.get(1)
    assert point11.x == pytest.approx(-179.0, abs=1e-7)
    assert point12.x == pytest.approx(-180.0, abs=1e-7)
    assert point21.x == pytest.approx(180.0, abs=1e-7)
    assert point22.x == pytest.approx(179.0, abs=1e-7)

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

    assert shape.project(polar_proj, longlat_proj) == 0

    part1 = shape.get(0)
    assert part1
    part2 = shape.get(1)
    assert part2

    point11 = part1.get(0)
    point12 = part1.get(1)
    point21 = part2.get(0)
    point22 = part2.get(1)
    assert point11.x == pytest.approx(-19926188.85, abs=1e-2)
    assert point12.x == pytest.approx(-20037508.34, abs=1e-2)
    assert point21.x == pytest.approx(20037508.34, abs=1e-2)
    assert point22.x == pytest.approx(19926188.85, abs=1e-2)
