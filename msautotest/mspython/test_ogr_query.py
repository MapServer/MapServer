#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
#
# Project:  MapServer
# Purpose:  Test OGR queries, partly intended to confirm correct operation
#           after the one pass query overhaul.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2010, Frank Warmerdam <warmerdam@pobox.com>
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
#

import os

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
#
def check_EAS_ID_with_or_without_space(layer, s, expected_value):

    name = "EAS_ID"
    actual_value = pmstestlib.get_item_value(layer, s, name)
    if actual_value is None:
        print("missing expected attribute %s" % name)
        return False
    if actual_value == expected_value or actual_value == expected_value.strip():
        return True
    else:
        print(
            'attribute %s is "%s" instead of expected "%s"'
            % (name, actual_value, str(expected_value))
        )
        return False


###############################################################################
# Execute region query.


def test_ogr_query_2():

    map = mapscript.mapObj(get_relpath_to_this("ogr_query.map"))
    layer = map.getLayer(0)
    line = mapscript.lineObj()
    line.add(mapscript.pointObj(479000, 4763000))
    line.add(mapscript.pointObj(480000, 4763000))
    line.add(mapscript.pointObj(480000, 4764000))
    line.add(mapscript.pointObj(479000, 4764000))

    poly = mapscript.shapeObj(mapscript.MS_SHAPE_POLYGON)
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

        s = layer.getShape(result)
        count = count + 1

    assert count == 2

    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)

    s = layer.getShape(result)

    assert check_EAS_ID_with_or_without_space(layer, s, "        158")

    #########################################################################
    # Check first shape geometry.

    assert s.type == mapscript.MS_SHAPE_POLYGON
    assert s.numlines == 1

    try:
        l = s.getLine(0)
    except Exception:
        l = s.get(0)
    assert l.numpoints == 61

    try:
        p = l.getPoint(0)
    except Exception:
        p = l.get(5)

    assert abs(p.x - 480984.25) < 0.01 and abs(p.y - 4764875.0) < 0.01

    #########################################################################
    # Check last shape attributes.

    result = layer.getResult(1)

    s = layer.getShape(result)

    assert check_EAS_ID_with_or_without_space(layer, s, "        165")

    layer.close()
    layer.close()  # discard resultset.


###############################################################################
# Execute multiple point query, and check result.


def test_ogr_query_4():

    map = mapscript.mapObj(get_relpath_to_this("ogr_query.map"))
    layer = map.getLayer(0)
    rect = mapscript.rectObj()
    rect.minx = 479000
    rect.maxx = 480000
    rect.miny = 4763000
    rect.maxy = 4764000

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

    assert count == 2

    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)

    s = layer.getShape(result)

    assert check_EAS_ID_with_or_without_space(layer, s, "        158")

    #########################################################################
    # Check first shape geometry.

    assert s.type == mapscript.MS_SHAPE_POLYGON
    assert s.numlines == 1

    try:
        l = s.getLine(0)
    except Exception:
        l = s.get(0)
    assert l.numpoints == 61

    try:
        p = l.getPoint(0)
    except Exception:
        p = l.get(5)

    assert abs(p.x - 480984.25) < 0.01 and abs(p.y - 4764875.0) < 0.01

    #########################################################################
    # Check last shape attributes.

    result = layer.getResult(1)

    s = layer.getShape(result)

    assert check_EAS_ID_with_or_without_space(layer, s, "        165")

    layer.close()
    layer.close()  # discard resultset.


###############################################################################
# Confirm that we can still fetch features not in the result set directly
# by their feature id.
#
# NOTE: the ability to fetch features without going through the query API
# seems to be gone in 6.0!


def test_ogr_query_6():

    pytest.skip("no longer work")

    map = mapscript.mapObj(get_relpath_to_this("ogr_query.map"))
    layer = map.getLayer(0)
    layer.open()

    #########################################################################
    # Check first shape attributes.

    s = mapscript.shapeObj(mapscript.MS_SHAPE_POLYGON)
    layer.resultsGetShape(s, 9, 0)

    assert check_EAS_ID_with_or_without_space(layer, s, "        170")

    layer.close()
    layer.close()  # discard resultset.


###############################################################################
# Change the map extents and see if our query results have been altered.
# With the current implementation they will be, though this might be
# considered to be a defect.


def test_ogr_query_7():

    map = mapscript.mapObj(get_relpath_to_this("ogr_query.map"))
    layer = map.getLayer(0)
    rect = mapscript.rectObj()
    rect.minx = 479000
    rect.maxx = 480000
    rect.miny = 4763000
    rect.maxy = 4764000

    layer.queryByRect(map, rect)

    map.draw()

    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break

        count = count + 1

    assert count == 2
    #########################################################################
    # Check first shape attributes.

    result = layer.getResult(0)

    s = layer.getShape(result)

    assert check_EAS_ID_with_or_without_space(layer, s, "        168")

    layer.close()
    layer.close()  # discard resultset.
