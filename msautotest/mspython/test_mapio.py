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

import sys

sys.path.append( '../pymod' )
import pmstestlib

import mapscript

###############################################################################
#

def test_msIO_getAndStripStdoutBufferMimeHeaders():

    if 'INPUT=GDAL' not in mapscript.msGetVersion():
        return 'skip'
    if 'SUPPORTS=WMS' not in mapscript.msGetVersion():
        return 'skip'

    map = mapscript.mapObj('test_mapio.map')
    request = mapscript.OWSRequest()
    mapscript.msIO_installStdoutToBuffer()
    request.loadParamsFromURL('service=WMS&version=1.1.1&request=GetMap&layers=grey&srs=EPSG:4326&bbox=-180,-90,180,90&format=image/png&width=80&height=40')
    status = map.OWSDispatch(request)
    if status != 0:
        pmstestlib.post_reason( 'wrong OWSDispatch status' )
        return 'fail'
    headers = mapscript.msIO_getAndStripStdoutBufferMimeHeaders()
    if headers is None:
        pmstestlib.post_reason( 'headers is None' )
        return 'fail'
    if 'Content-Type' not in headers or headers['Content-Type'] != 'image/png':
        pmstestlib.post_reason( 'wrong Content-Type' )
        print(headers)
        return 'fail'
    if 'Cache-Control' not in headers or headers['Cache-Control'] != 'max-age=86400':
        pmstestlib.post_reason( 'wrong Cache-Control' )
        print(headers)
        return 'fail'

    result = mapscript.msIO_getStdoutBufferBytes()
    if result is None or result[1:4] != b'PNG':
        pmstestlib.post_reason( 'wrong data' )
        return 'fail'

    return 'success'

test_list = [
    test_msIO_getAndStripStdoutBufferMimeHeaders,
    None ]

if __name__ == '__main__':

    pmstestlib.setup_run( 'test_mapio' )

    pmstestlib.run_tests( test_list )

    pmstestlib.summarize()

    mapscript.msCleanup()

