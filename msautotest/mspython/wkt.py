#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Test script for WKT translation.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2006, Frank Warmerdam <warmerdam@pobox.com>
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
# $Log$
# Revision 1.1  2006/09/05 20:40:04  frank
# New
#
#

import sys
import string

sys.path.append( '../pymod' )
import pmstestlib

import mapscript

###############################################################################
# Test simple geometries that should convert back to same thing - OGR only.

def wkt_test_1():

    if string.find(mapscript.msGetVersion(),'SUPPORTS=GEOS') != -1 \
       or string.find(mapscript.msGetVersion(),'INPUT=OGR') == -1:
        return 'skip'

    wkt_list = [ 'POINT (5 7)',
                 'LINESTRING (5 7,7 9,9 -1)',
                 'POLYGON ((500 500,3500 500,3500 2500,500 2500,500 500),(1000 1000,1000 1500,1500 1500,1500 1000,1000 1000))',
                 'MULTIPOINT (2000 2000,2000 1900)',
                 'MULTILINESTRING ((600 1500,1600 2500),(700 1500,1700 2500))']

    for orig_wkt in wkt_list:
        shp = mapscript.shapeObj.fromWKT( orig_wkt )
        new_wkt = shp.toWKT()

        if new_wkt != orig_wkt:
            pmstestlib.post_reason( 'WKT "%s" converted to "%s".' \
                                    % (orig_wkt, new_wkt) )
            return 'fail'
    
    return 'success'


###############################################################################
# Test MULTIPOLYGON which will translate back as a POLYGON since MapServer
# doesn't know how to distinguish - OGR Only.

def wkt_test_2():

    if string.find(mapscript.msGetVersion(),'SUPPORTS=GEOS') != -1 \
       or string.find(mapscript.msGetVersion(),'INPUT=OGR') == -1:
        return 'skip'

    orig_wkt = 'MULTIPOLYGON(((50 50, 350 50, 350 250, 50 250, 50 50)),((250 150, 550 150, 550 350, 250 350, 250 150)))'
    expected_wkt = 'POLYGON ((50 50,350 50,350 250,50 250,50 50),(250 150,550 150,550 350,250 350,250 150))'
    
    shp = mapscript.shapeObj.fromWKT( orig_wkt )
    new_wkt = shp.toWKT()

    if new_wkt != expected_wkt:
        pmstestlib.post_reason( 'WKT "%s" converted to "%s".' \
                                % (expected_wkt, new_wkt) )
        return 'fail'
    
    return 'success'


###############################################################################
# Test simple geometries that should convert back to same thing - GEOS only.

def wkt_test_3():

    if string.find(mapscript.msGetVersion(),'SUPPORTS=GEOS') == -1:
        return 'skip'

    wkt_list = [ 'POINT (5.0000000000000000 7.0000000000000000)',
                 'LINESTRING (5.0000000000000000 7.0000000000000000, 7.0000000000000000 9.0000000000000000, 9.0000000000000000 -1.0000000000000000)',
                 'POLYGON ((500.0000000000000000 500.0000000000000000, 3500.0000000000000000 500.0000000000000000, 3500.0000000000000000 2500.0000000000000000, 500.0000000000000000 2500.0000000000000000, 500.0000000000000000 500.0000000000000000), (1000.0000000000000000 1000.0000000000000000, 1000.0000000000000000 1500.0000000000000000, 1500.0000000000000000 1500.0000000000000000, 1500.0000000000000000 1000.0000000000000000, 1000.0000000000000000 1000.0000000000000000))',
                 'MULTIPOINT (2000.0000000000000000 2000.0000000000000000, 2000.0000000000000000 1900.0000000000000000)',
                 'MULTILINESTRING ((600.0000000000000000 1500.0000000000000000, 1600.0000000000000000 2500.0000000000000000), (700.0000000000000000 1500.0000000000000000, 1700.0000000000000000 2500.0000000000000000))']

    for orig_wkt in wkt_list:
        shp = mapscript.shapeObj.fromWKT( orig_wkt )
        new_wkt = shp.toWKT()

        if new_wkt != orig_wkt:
            pmstestlib.post_reason( 'WKT "%s" converted to "%s".' \
                                    % (orig_wkt, new_wkt) )
            return 'fail'
    
    return 'success'


###############################################################################
# Cleanup.

def wkt_test_cleanup():
    return 'success'

test_list = [
    wkt_test_1,
    wkt_test_2,
    wkt_test_3,
    wkt_test_cleanup ]

if __name__ == '__main__':

    pmstestlib.setup_run( 'wkt' )

    pmstestlib.run_tests( test_list )

    pmstestlib.summarize()

    mapscript.msCleanup(0)

