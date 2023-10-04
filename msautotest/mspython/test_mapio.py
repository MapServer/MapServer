#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Regression test for mapio
# Author:   Even Rouault
#
###############################################################################
#  Copyright (c) 2017, Even Rouault,<even.rouault at spatialys.com>
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


def test_msIO_getAndStripStdoutBufferMimeHeaders():

    if "SUPPORTS=WMS" not in mapscript.msGetVersion():
        pytest.skip()

    map = mapscript.mapObj(get_relpath_to_this("test_mapio.map"))
    request = mapscript.OWSRequest()
    mapscript.msIO_installStdoutToBuffer()
    request.loadParamsFromURL(
        "service=WMS&version=1.1.1&request=GetMap&layers=grey&srs=EPSG:4326&bbox=-180,-90,180,90&format=image/png&width=80&height=40&STYLES="
    )
    status = map.OWSDispatch(request)
    assert status == 0
    headers = mapscript.msIO_getAndStripStdoutBufferMimeHeaders()
    assert headers is not None
    assert "Content-Type" in headers
    assert headers["Content-Type"] == "image/png"
    assert "Cache-Control" in headers
    assert headers["Cache-Control"] == "max-age=86400"

    result = mapscript.msIO_getStdoutBufferBytes()
    assert result is not None
    assert result[1:4] == b"PNG"
