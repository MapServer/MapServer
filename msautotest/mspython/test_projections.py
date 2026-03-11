#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# Project:  MapServer
# Purpose:  Test suite for loading projections
# Author:   Seth Girvin
#
###############################################################################
#  Copyright (c) 2026, Seth Girvin,<sethg at geographika.co.uk>
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


def make_layer():
    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.name = "test_layer"
    layer.status = mapscript.MS_ON
    layer.type = mapscript.MS_LAYER_POLYGON
    return layer


PROJECTION_CASES = [
    # (input, expected_output, marks)
    ("+proj=utm +zone=11 +datum=WGS84", "+proj=utm +zone=11 +datum=WGS84"),
    ("+proj=latlong +datum=WGS84", "+proj=latlong +datum=WGS84"),
    ("+proj=ortho +datum=WGS84", "+proj=ortho +datum=WGS84"),
    ("init=epsg:4326", "+init=epsg:4326"),
    ("epsg:4326", "+init=epsg:4326"),
    ("EPSG:4326", "+init=epsg:4326"),
    ("EPSG:2157", "+init=epsg:2157"),
    ("urn:ogc:def:crs:OGC:1.3:CRS84", "+init=epsg:4326"),
    ("ESRI:53009", "+ESRI:53009"),
    ("AUTO:42001,9001,2.35,48.85", "+AUTO:42001,9001,2.35,48.85"),
    ("AUTO2:42005,1,0,90", "+AUTO2:42005,1,0,90"),
    # ensure the epsg2 file with the custom projection is in the PROJ_DATA folder
    ("init=epsg2:42304", "+init=epsg2:42304"),
    ("http://www.opengis.net/def/crs/EPSG/0/4326", "+init=epsg:4326 +epsgaxis=ne"),
    ("http://www.opengis.net/def/crs/EPSG/0/32615", "+init=epsg:32615"),
    ("http://www.opengis.net/gml/srs/epsg.xml#4326", "+init=epsg:4326"),
    ("urn:ogc:def:crs:ESRI::53009", "+ESRI:53009"),
    ("urn:ogc:def:crs:EPSG::4326", "+init=epsg:4326 +epsgaxis=ne"),
    ("urn:ogc:def:crs:OGC:1.3:CRS84", "+init=epsg:4326"),
    ("urn:x-ogc:def:crs:EPSG::3857", "+init=epsg:3857"),
    ("urn:EPSG:geographicCRS:4326", "+init=epsg:4326 +epsgaxis=ne"),
    ("urn:ogc:def:crs:EPSG::3857", "+init=epsg:3857"),
    ("IAU_2015:30100", "+IAU_2015:30100"),
    ("IGNF:ATIGBONNE.BOURD", "+IGNF:ATIGBONNE.BOURD"),
    pytest.param(
        "urn:x-ogc:def:crs:OGC:1.3:CRS84",
        None,
        marks=pytest.mark.xfail(
            reason="OGC authority not supported in urn:x-ogc: handler"
        ),
    ),
    pytest.param(
        "urn:x-ogc:def:crs:OGC::CRS84",
        None,
        marks=pytest.mark.xfail(
            reason="OGC authority not supported in urn:x-ogc: handler"
        ),
    ),
    pytest.param(
        "urn:ogc:def:crs:OGC::imageCRS",
        None,
        marks=pytest.mark.xfail(
            reason="imageCRS is pixel space, not supported by PROJ"
        ),
    ),
    # IAU_2015 is used by PROJ rather than IAU:2015
    pytest.param(
        "IAU:2015:30100",
        "+IAU:2015:30100",
        marks=pytest.mark.xfail(reason="IAU:2015 is handled as IAU_2015"),
    ),
]


@pytest.mark.parametrize("input_str,expected", PROJECTION_CASES)
def test_projection_string(input_str, expected):
    layer = make_layer()
    layer.setProjection(input_str)
    assert layer.getProjection() == expected
