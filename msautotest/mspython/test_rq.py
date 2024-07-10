#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Test suite for MapServer Raster Query capability.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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

import math
import os
import sys

import pmstestlib
import pytest

mapscript_available = False
try:
    import mapscript

    mapscript_available = True
except ImportError:
    pass

pytestmark = pytest.mark.skipif(
    not mapscript_available, reason="mapscript not available"
)

###############################################################################
# Dump query result set ... used when debugging this test script.


def dumpResultSet(layer):
    layer.open()
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        print("(%d,%d)" % (result.shapeindex, result.tileindex))

        s = layer.getShape(result)

        for i in range(layer.numitems):
            print("%s: %s" % (layer.getItem(i), s.getValue(i)))

    layer.close()


def get_relpath_to_this(filename):
    return os.path.join(os.path.dirname(__file__), filename)


###############################################################################
# Execute region query.


def test_rq_2():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindex.map"))
    layer = map.getLayer(0)
    line = mapscript.lineObj()
    line.add(mapscript.pointObj(35, 25))
    line.add(mapscript.pointObj(45, 25))
    line.add(mapscript.pointObj(45, 35))
    line.add(mapscript.pointObj(35, 25))

    poly = mapscript.shapeObj(mapscript.MS_SHAPE_POLYGON)
    poly.add(line)

    layer.queryByShape(map, poly)

    # Scan results, checking count and the first shape information.

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 55

    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)
    s = layer.getShape(result)

    pmstestlib.check_items(
        layer,
        s,
        [
            ("value_0", "115"),
            ("red", "115"),
            ("green", "115"),
            ("blue", "115"),
            ("value_list", "115"),
            ("x", "39.5"),
            ("y", "29.5"),
        ],
    )

    #########################################################################
    # Check first shape geometry.
    assert s.type == mapscript.MS_SHAPE_POINT

    assert s.numlines == 1

    try:
        l = s.getLine(0)
    except Exception:
        l = s.get(0)
    assert l.numpoints == 1

    try:
        p = l.getPoint(0)
    except Exception:
        p = l.get(0)
    assert p.x == 39.5
    assert p.y == 29.5

    #########################################################################
    # Check last shape attributes.

    result = layer.getResult(54)
    s = layer.getShape(result)

    pmstestlib.check_items(layer, s, [("value_0", "132"), ("x", "44.5"), ("y", "25.5")])
    layer.close()  # discard resultset.


###############################################################################
# Execute multiple point query, and check result.


def test_rq_4():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindex.map"))
    layer = map.getLayer(0)

    pnt = mapscript.pointObj()
    pnt.x = 35.5
    pnt.y = 25.5

    layer.queryByPoint(map, pnt, mapscript.MS_MULTIPLE, 1.25)

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 9

    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)
    s = layer.getShape(result)

    pmstestlib.check_items(layer, s, [("value_0", "123"), ("x", "34.5"), ("y", "26.5")])

    #########################################################################
    # Check last shape attributes.

    result = layer.getResult(8)
    s = layer.getShape(result)

    pmstestlib.check_items(layer, s, [("value_0", "107"), ("x", "36.5"), ("y", "24.5")])
    layer.close()  # discard resultset.


###############################################################################
# Execute multiple point query, and check result.  Also operates on map,
# instead of layer.


def test_rq_6():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindex.map"))
    layer = map.getLayer(0)

    pnt = mapscript.pointObj()
    pnt.x = 35.2
    pnt.y = 25.3

    map.queryByPoint(pnt, mapscript.MS_SINGLE, 10.0)

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 1

    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)
    s = layer.getShape(result)

    pmstestlib.check_items(layer, s, [("value_0", "115"), ("x", "35.5"), ("y", "25.5")])
    layer.close()  # discard resultset.


###############################################################################
# Execute multiple point query, and check result.


def test_rq_8():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindex.map"))
    layer = map.getLayer(0)

    rect = mapscript.rectObj()
    rect.minx = 35
    rect.maxx = 45
    rect.miny = 25
    rect.maxy = 35

    layer.queryByRect(map, rect)

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 100
    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)
    s = layer.getShape(result)

    pmstestlib.check_items(
        layer,
        s,
        [
            ("value_0", "148"),
            ("red", "148"),
            ("green", "148"),
            ("blue", "148"),
            ("value_list", "148"),
            ("x", "35.5"),
            ("y", "34.5"),
        ],
    )

    #########################################################################
    # Check last shape attributes.

    result = layer.getResult(99)
    s = layer.getShape(result)

    pmstestlib.check_items(
        layer,
        s,
        [
            ("value_0", "132"),
            ("red", "132"),
            ("green", "132"),
            ("blue", "132"),
            ("value_list", "132"),
            ("x", "44.5"),
            ("y", "25.5"),
        ],
    )
    layer.close()  # discard resultset.


###############################################################################
# Execute a shape query without any tolerance and a line query region.


def test_rq_9():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindex.map"))
    layer = map.getLayer(0)

    line = mapscript.lineObj()
    line.add(mapscript.pointObj(35, 25))
    line.add(mapscript.pointObj(45, 25))
    line.add(mapscript.pointObj(45, 35))
    line.add(mapscript.pointObj(35, 25))

    poly = mapscript.shapeObj(mapscript.MS_SHAPE_LINE)
    poly.add(line)

    layer.queryByShape(map, poly)

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 47

    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)
    s = layer.getShape(result)

    pmstestlib.check_items(
        layer,
        s,
        [
            ("value_0", "115"),
            ("red", "115"),
            ("green", "115"),
            ("blue", "115"),
            ("value_list", "115"),
            ("x", "39.5"),
            ("y", "29.5"),
        ],
    )

    #########################################################################
    # Check first shape geometry.
    assert s.type == mapscript.MS_SHAPE_POINT
    assert s.numlines == 1

    try:
        l = s.getLine(0)
    except Exception:
        l = s.get(0)
    assert l.numpoints == 1

    try:
        p = l.getPoint(0)
    except Exception:
        p = l.get(0)
    assert p.x == 39.5
    assert p.y == 29.5

    #########################################################################
    # Check last shape attributes.

    result = layer.getResult(46)
    s = layer.getShape(result)

    pmstestlib.check_items(layer, s, [("value_0", "148"), ("x", "44.5"), ("y", "24.5")])
    layer.close()  # discard resultset.


###############################################################################
# Open a classified map and post a point query.


def test_rq_10():
    """
    This test requires the GIF driver to be available in GDAL, by setting GDAL_DRIVER_PATH
    """

    map = mapscript.mapObj(get_relpath_to_this("../gdal/classtest1.map"))
    layer = map.getLayer(0)

    pnt = mapscript.pointObj()
    pnt.x = 88.5
    pnt.y = 7.5

    layer.queryByPoint(map, pnt, mapscript.MS_SINGLE, 10.0)

    ###############################################################################
    # Scan results.  This query is for a transparent pixel within the "x" of
    # the cubewerx logo.  In the future the raster query may well stop returning
    # "offsite" pixels and we will need to update this test.

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 1

    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)
    s = layer.getShape(result)

    pmstestlib.check_items(
        layer,
        s,
        [
            ("value_0", "0"),
            ("red", "-255"),
            ("green", "-255"),
            ("blue", "-255"),
            ("class", "Text"),
            ("x", "88.5"),
            ("y", "7.5"),
        ],
    )
    layer.close()  # discard resultset.


###############################################################################
# Issue another point query, on colored text.


def test_rqtest_12():
    """
    This test requires the GIF driver to be available in GDAL, by setting GDAL_DRIVER_PATH
    """

    map = mapscript.mapObj(get_relpath_to_this("../gdal/classtest1.map"))
    layer = map.getLayer(0)

    pnt = mapscript.pointObj()
    pnt.x = 13.5
    pnt.y = 36.5

    layer.queryByPoint(map, pnt, mapscript.MS_SINGLE, 10.0)

    ###############################################################################
    # Scan results.  This query is for a pixel at a grid intersection.  This
    # pixel should be classified as grid and returned.
    #

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 1

    result = layer.getResult(0)
    s = layer.getShape(result)

    pmstestlib.check_items(
        layer,
        s,
        [
            ("value_0", "1"),
            ("red", "255"),
            ("green", "0"),
            ("blue", "0"),
            ("class", "Grid"),
            ("x", "13.5"),
            ("y", "36.5"),
        ],
    )
    layer.close()  # discard resultset.


###############################################################################
# Revert to tileindex.map and do a test where we force reprojection


def test_rq_14():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindex.map"))
    layer = map.getLayer(0)

    map.setProjection("+proj=utm +zone=30 +datum=WGS84")

    pnt = mapscript.pointObj()
    pnt.x = 889690
    pnt.y = 55369

    layer.queryByPoint(map, pnt, mapscript.MS_MULTIPLE, 200000.0)

    ###############################################################################
    # Check result count, and that the results are within the expected distance.
    # This also implicitly verifies the results were reprojected back to UTM
    # coordinates from the lat long system of the layer.

    pnt = mapscript.pointObj()
    pnt.x = 889690
    pnt.y = 55369

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

        s = layer.getShape(result)
        x = float(pmstestlib.get_item_value(layer, s, "x"))
        y = float(pmstestlib.get_item_value(layer, s, "y"))
        dist_sq = (x - pnt.x) * (x - pnt.x) + (y - pnt.y) * (y - pnt.y)
        dist = math.pow(dist_sq, 0.5)
        assert dist <= 200000.0

    assert count == 4

    layer.close()  # discard resultset.


###############################################################################
# Make a similar test with the tileindex file in mapinfo format (#2796)


def test_rq_16():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindex_mi.map"))
    layer = map.getLayer(0)

    map.setProjection("+proj=utm +zone=30 +datum=WGS84")

    pnt = mapscript.pointObj()
    pnt.x = 889690
    pnt.y = 55369

    layer.queryByPoint(map, pnt, mapscript.MS_MULTIPLE, 200000.0)

    ###############################################################################
    # Check result count, and that the results are within the expected distance.
    # This also implicitly verifies the results were reprojected back to UTM
    # coordinates from the lat long system of the layer.

    pnt = mapscript.pointObj()
    pnt.x = 889690
    pnt.y = 55369

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

        s = layer.getShape(result)
        x = float(pmstestlib.get_item_value(layer, s, "x"))
        y = float(pmstestlib.get_item_value(layer, s, "y"))
        dist_sq = (x - pnt.x) * (x - pnt.x) + (y - pnt.y) * (y - pnt.y)
        dist = math.pow(dist_sq, 0.5)
        assert dist <= 200000.0

    assert count == 4

    layer.close()  # discard resultset.


###############################################################################
# Test a layer with a tileindex with mixed SRS


@pytest.mark.skipif(sys.platform == "win32", reason="Fails on Windows")
def test_rq_18():

    map = mapscript.mapObj(get_relpath_to_this("../gdal/tileindexmixedsrs.map"))
    layer = map.getLayer(0)

    map.setProjection("+proj=latlong +datum=WGS84")

    pnt = mapscript.pointObj()
    pnt.x = -117.6
    pnt.y = 33.9

    layer.queryByPoint(map, pnt, mapscript.MS_SINGLE, 0.001)

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

        s = layer.getShape(result)
        x = float(pmstestlib.get_item_value(layer, s, "x"))
        y = float(pmstestlib.get_item_value(layer, s, "y"))
        dist_sq = (x - pnt.x) * (x - pnt.x) + (y - pnt.y) * (y - pnt.y)
        dist = math.pow(dist_sq, 0.5)
        assert dist <= 0.001

    assert count == 1

    layer.close()  # discard resultset.
