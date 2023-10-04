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

import os

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


def get_relpath_to_this(filename):
    return os.path.join(os.path.dirname(__file__), filename)


###############################################################################
#


def test_xmp():

    if "SUPPORTS=XMP" not in mapscript.msGetVersion():
        pytest.skip()

    map = mapscript.mapObj(get_relpath_to_this("test_xmp.map"))
    img = map.draw()
    try:
        os.mkdir(get_relpath_to_this("result"))
    except Exception:
        pass
    resultfile = get_relpath_to_this("result/test_xmp.png")
    img.save(resultfile, map)

    f = open(resultfile, "rb")
    data = f.read().decode("utf-8", "ignore")
    f.close()

    def dump_md():
        try:
            from osgeo import gdal

            ds = gdal.Open(resultfile)
            print(ds.GetMetadata("xml:XMP"))
            ds = None
        except Exception:
            pass

    assert 'dc:Title="Super Map"' in data, dump_md()
    assert 'xmpRights:Marked="true"' in data, dump_md()
    assert (
        'cc:License="http://creativecommons.org/licenses/by-sa/2.0/"' in data
    ), dump_md()
    assert 'xmlns:lightroom="http://ns.adobe.com/lightroom/1.0/"' in data, dump_md()
    assert 'lightroom:PrivateRTKInfo="My Information Here"' in data, dump_md()
    assert 'exif:ExifVersion="dummy"' in data, dump_md()
    assert 'xmp:CreatorTool="MapServer"' in data, dump_md()
    assert 'photoshop:Source="MapServer"' in data, dump_md()
    assert 'crs:Converter="MapServer"' in data, dump_md()
    assert 'x:Author="John Doe"' in data, dump_md()
