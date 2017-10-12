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

import sys
import math

sys.path.append( '../pymod' )
import pmstestlib

import mapscript

###############################################################################
# Dump query result set ... used when debugging this test script.

def dumpResultSet( layer ):
    layer.open()
    for i in range(1000):
        result = layer.getResult( i )
        if result is None:
            break
        
        print '(%d,%d)' % (result.shapeindex, result.tileindex)
        
        s = layer.getShape( result )
        for i in range(layer.numitems):
            print '%s: %s' % (layer.getItem(i), s.getValue(i))
            
    layer.close()

###############################################################################
# Open map and get working layer.

def ogr_query_1():
    
    pmstestlib.map = mapscript.mapObj('ogr_query.map')
    pmstestlib.layer = pmstestlib.map.getLayer(0)

    return 'success'

###############################################################################
# Execute region query.

def ogr_query_2():

    line = mapscript.lineObj()
    line.add( mapscript.pointObj( 479000, 4763000 ) )
    line.add( mapscript.pointObj( 480000, 4763000 ) )
    line.add( mapscript.pointObj( 480000, 4764000 ) )
    line.add( mapscript.pointObj( 479000, 4764000 ) )

    poly = mapscript.shapeObj( mapscript.MS_SHAPE_POLYGON )
    poly.add( line )

    pmstestlib.layer.queryByShape( pmstestlib.map, poly )

    return 'success'

###############################################################################
#
def check_EAS_ID_with_or_without_space(layer, s, expected_value):

    name = 'EAS_ID'
    actual_value = pmstestlib.get_item_value( layer, s, name )
    if actual_value is None:
        post_reason( 'missing expected attribute %s' % name )
        return False
    if actual_value == expected_value or actual_value == expected_value.strip():
        return True
    else:
        post_reason( 'attribute %s is "%s" instead of expected "%s"' % \
                        (name, actual_value, str(expected_value)) )
        return False

###############################################################################
# Scan results, checking count and the first shape information.

def ogr_query_3():
    layer = pmstestlib.layer
    
    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult( i )
        if result is None:
            break

        s = layer.getShape( result )
        count = count + 1

    if count != 2:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 55) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    
    s = layer.getShape( result )
    
    if not check_EAS_ID_with_or_without_space( layer, s,'        158' ):
        return 'fail'

    #########################################################################
    # Check first shape geometry.

    if s.type != mapscript.MS_SHAPE_POLYGON:
        pmstestlib.post_reason( 'query result is not a polygon.' )
        return 'fail'

    if s.numlines != 1:
        pmstestlib.post_reason( 'query has other than 1 lines.' )
        return 'fail'

    try:
        l = s.getLine( 0 )
    except:
        l = s.get( 0 )
    if l.numpoints != 61:
        pmstestlib.post_reason( 'query has %d points, instead of expected number.' % l.numpoints )
        return 'fail'

    try:
        p = l.getPoint(0)
    except:
        p = l.get(5)

    if abs(p.x-480984.25) > 0.01 or abs(p.y-4764875.0) > 0.01:
        print p.x, p.y
        pmstestlib.post_reason( 'got wrong location.' )
        return 'fail'
    
    #########################################################################
    # Check last shape attributes.

    result = layer.getResult( 1 )
    
    s = layer.getShape( result )

    if not check_EAS_ID_with_or_without_space( layer, s,'        165' ):
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Execute multiple point query, and check result.

def ogr_query_4():

    rect = mapscript.rectObj()
    rect.minx = 479000
    rect.maxx = 480000
    rect.miny = 4763000
    rect.maxy = 4764000
    
    pmstestlib.layer.queryByRect( pmstestlib.map, rect )

    return 'success'

###############################################################################
# Scan results, checking count and the first shape information.

def ogr_query_5():
    layer = pmstestlib.layer
    
    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult( i )
        if result is None:
            break
    
        count = count + 1

    if count != 2:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 55) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    
    s = layer.getShape( result )
    
    if not check_EAS_ID_with_or_without_space( layer, s,'        158' ):
        return 'fail'

    #########################################################################
    # Check first shape geometry.

    if s.type != mapscript.MS_SHAPE_POLYGON:
        pmstestlib.post_reason( 'query result is not a polygon.' )
        return 'fail'

    if s.numlines != 1:
        pmstestlib.post_reason( 'query has other than 1 lines.' )
        return 'fail'

    try:
        l = s.getLine( 0 )
    except:
        l = s.get( 0 )
    if l.numpoints != 61:
        pmstestlib.post_reason( 'query has %d points, instead of expected number.' % l.numpoints )
        return 'fail'

    try:
        p = l.getPoint(0)
    except:
        p = l.get(5)

    if abs(p.x-480984.25) > 0.01 or abs(p.y-4764875.0) > 0.01:
        print p.x, p.y
        pmstestlib.post_reason( 'got wrong location.' )
        return 'fail'
    
    #########################################################################
    # Check last shape attributes.

    result = layer.getResult( 1 )
    
    s = layer.getShape( result )

    if not check_EAS_ID_with_or_without_space( layer, s,'        165' ):
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'

###############################################################################
# Confirm that we can still fetch features not in the result set directly
# by their feature id.
#
# NOTE: the ability to fetch features without going through the query API
# seems to be gone in 6.0!  

def ogr_query_6():

    return 'skip'

    layer = pmstestlib.layer
    
    layer.open()
    
    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    
    s = mapscript.shapeObj( mapscript.MS_SHAPE_POLYGON )
    layer.resultsGetShape( s, 9, 0 )
    
    if not check_EAS_ID_with_or_without_space( layer, s,'        170' ):
        return 'fail'

    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Change the map extents and see if our query results have been altered.
# With the current implementation they will be, though this might be
# considered to be a defect.

def ogr_query_7():
    map = pmstestlib.map

    map.draw()
    layer = pmstestlib.layer
    
    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult( i )
        if result is None:
            break
    
        count = count + 1

    if count != 2:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 55) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    
    s = layer.getShape( result )
    
    if not check_EAS_ID_with_or_without_space( layer, s,'        168' ):
        return 'fail'

    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Cleanup.

def ogr_query_cleanup():
    pmstestlib.layer = None
    pmstestlib.map = None
    return 'success'

test_list = [
    ogr_query_1,
    ogr_query_2,
    ogr_query_3,
    ogr_query_4,
    ogr_query_5,
    ogr_query_6,
    ogr_query_7,
    ogr_query_cleanup ]

if __name__ == '__main__':

    pmstestlib.setup_run( 'ogr_query' )

    pmstestlib.run_tests( test_list )

    pmstestlib.summarize()

    mapscript.msCleanup(0)

