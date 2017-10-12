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

def rqtest_1():
    
    pmstestlib.map = mapscript.mapObj('../gdal/tileindex.map')
    pmstestlib.layer = pmstestlib.map.getLayer(0)

    return 'success'

###############################################################################
# Execute region query.

def rqtest_2():

    line = mapscript.lineObj()
    line.add( mapscript.pointObj( 35, 25 ) )
    line.add( mapscript.pointObj( 45, 25 ) )
    line.add( mapscript.pointObj( 45, 35 ) )
    line.add( mapscript.pointObj( 35, 25 ) )

    poly = mapscript.shapeObj( mapscript.MS_SHAPE_POLYGON )
    poly.add( line )

    pmstestlib.layer.queryByShape( pmstestlib.map, poly )

    return 'success'

###############################################################################
# Scan results, checking count and the first shape information.

def rqtest_3():
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

    if count != 55:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 55) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    s = layer.getShape( result )
    
    if pmstestlib.check_items( layer, s,
                               [('value_0','115'),
                                ('red','115'),
                                ('green','115'),
                                ('blue','115'),
                                ('value_list','115'),
                                ('x','39.5'),
                                ('y','29.5')] ) == 0:
        return 'fail'

    #########################################################################
    # Check first shape geometry.
    if s.type != mapscript.MS_SHAPE_POINT:
        pmstestlib.post_reason( 'raster query result is not a point.' )
        return 'fail'

    if s.numlines != 1:
        pmstestlib.post_reason( 'raster query has other than 1 lines.' )
        return 'fail'

    try:
        l = s.getLine( 0 )
    except:
        l = s.get( 0 )
    if l.numpoints != 1:
        pmstestlib.post_reason( 'raster query has other than 1 points.' )
        return 'fail'

    try:
        p = l.getPoint(0)
    except:
        p = l.get(0)
    if p.x != 39.5 or p.y != 29.5:
        pmstestlib.post_reason( 'got wrong point location.' )
        return 'fail'
    
    #########################################################################
    # Check last shape attributes.

    result = layer.getResult( 54 )
    s = layer.getShape( result )

    if pmstestlib.check_items( layer, s,
                               [('value_0','132'),
                                ('x','44.5'),
                                ('y','25.5')] ) == 0:
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Execute multiple point query, and check result.

def rqtest_4():

    pnt = mapscript.pointObj()
    pnt.x = 35.5
    pnt.y = 25.5
    
    pmstestlib.layer.queryByPoint( pmstestlib.map, pnt, mapscript.MS_MULTIPLE,
                                   1.25 )

    return 'success'

###############################################################################
# Scan results, checking count and the first shape information.

def rqtest_5():
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

    if count != 9:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 9) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    s = layer.getShape( result )
    
    if pmstestlib.check_items( layer, s,
                               [('value_0','123'),
                                ('x','34.5'),
                                ('y','26.5')] ) == 0:
        return 'fail'
    
    #########################################################################
    # Check last shape attributes.

    result = layer.getResult( 8 )
    s = layer.getShape( result )

    if pmstestlib.check_items( layer, s,
                               [('value_0','107'),
                                ('x','36.5'),
                                ('y','24.5')] ) == 0:
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Execute multiple point query, and check result.  Also operates on map,
# instead of layer.

def rqtest_6():

    pnt = mapscript.pointObj()
    pnt.x = 35.2
    pnt.y = 25.3
    
    pmstestlib.map.queryByPoint( pnt, mapscript.MS_SINGLE, 10.0 )

    return 'success'

###############################################################################
# Scan results, checking count and the first shape information.

def rqtest_7():
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

    if count != 1:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 1) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    s = layer.getShape( result )
    
    if pmstestlib.check_items( layer, s,
                               [('value_0','115'),
                                ('x','35.5'),
                                ('y','25.5')] ) == 0:
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Execute multiple point query, and check result.

def rqtest_8():

    rect = mapscript.rectObj()
    rect.minx = 35
    rect.maxx = 45
    rect.miny = 25
    rect.maxy = 35
    
    pmstestlib.layer.queryByRect( pmstestlib.map, rect )

    return 'success'

###############################################################################
# Scan results, checking count and the first shape information.

def rqtest_9():
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

    if count != 100:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 100) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    s = layer.getShape( result )
    
    if pmstestlib.check_items( layer, s,
                               [('value_0','148'),
                                ('red','148'),
                                ('green','148'),
                                ('blue','148'),
                                ('value_list','148'),
                                ('x','35.5'),
                                ('y','34.5')] ) == 0:
        return 'fail'
    
    #########################################################################
    # Check last shape attributes.

    result = layer.getResult( 99 )
    s = layer.getShape( result )

    if pmstestlib.check_items( layer, s,
                               [('value_0','132'),
                                ('red','132'),
                                ('green','132'),
                                ('blue','132'),
                                ('value_list','132'),
                                ('x','44.5'),
                                ('y','25.5')] ) == 0:
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Execute a shape query without any tolerance and a line query region.

def rqtest_9_1():

    line = mapscript.lineObj()
    line.add( mapscript.pointObj( 35, 25 ) )
    line.add( mapscript.pointObj( 45, 25 ) )
    line.add( mapscript.pointObj( 45, 35 ) )
    line.add( mapscript.pointObj( 35, 25 ) )

    poly = mapscript.shapeObj( mapscript.MS_SHAPE_LINE )
    poly.add( line )

    pmstestlib.layer.queryByShape( pmstestlib.map, poly )

    return 'success'

###############################################################################
# Scan results, checking count and the first shape information.

def rqtest_9_2():
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

    if count != 47:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 47) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    s = layer.getShape( result )
    
    if pmstestlib.check_items( layer, s,
                               [('value_0','115'),
                                ('red','115'),
                                ('green','115'),
                                ('blue','115'),
                                ('value_list','115'),
                                ('x','39.5'),
                                ('y','29.5')] ) == 0:
        return 'fail'

    #########################################################################
    # Check first shape geometry.
    if s.type != mapscript.MS_SHAPE_POINT:
        pmstestlib.post_reason( 'raster query result is not a point.' )
        return 'fail'

    if s.numlines != 1:
        pmstestlib.post_reason( 'raster query has other than 1 lines.' )
        return 'fail'

    try:
        l = s.getLine( 0 )
    except:
        l = s.get( 0 )
    if l.numpoints != 1:
        pmstestlib.post_reason( 'raster query has other than 1 points.' )
        return 'fail'

    try:
        p = l.getPoint(0)
    except:
        p = l.get(0)
    if p.x != 39.5 or p.y != 29.5:
        pmstestlib.post_reason( 'got wrong point location.' )
        return 'fail'
    
    #########################################################################
    # Check last shape attributes.

    result = layer.getResult( 46 )
    s = layer.getShape( result )

    if pmstestlib.check_items( layer, s,
                               [('value_0','148'),
                                ('x','44.5'),
                                ('y','24.5')] ) == 0:
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Close old map, and open a classified map and post a point query.

def rqtest_10():

    pmstestlib.layer = None
    pmstestlib.map = None

    pmstestlib.map = mapscript.mapObj('../gdal/classtest1.map')
    pmstestlib.layer = pmstestlib.map.getLayer(0)
    
    pnt = mapscript.pointObj()
    pnt.x = 88.5
    pnt.y = 7.5
    
    pmstestlib.layer.queryByPoint( pmstestlib.map, pnt, mapscript.MS_SINGLE,
                                   10.0 )

    return 'success'

###############################################################################
# Scan results.  This query is for a transparent pixel within the "x" of
# the cubewerx logo.  In the future the raster query may well stop returning
# "offsite" pixels and we will need to update this test.

def rqtest_11():
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

    if count != 1:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 1) )
        return 'fail'

    #########################################################################
    # Check first shape attributes.
    
    result = layer.getResult( 0 )
    s = layer.getShape( result )
    
    if pmstestlib.check_items( layer, s,
                               [('value_0','0'),
                                ('red','-255'),
                                ('green','-255'),
                                ('blue','-255'),
                                ('class','Text'),
                                ('x','88.5'),
                                ('y','7.5')] ) == 0:
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Issue another point query, on colored text. 

def rqtest_12():

    pnt = mapscript.pointObj()
    pnt.x = 13.5
    pnt.y = 36.5
    
    pmstestlib.layer.queryByPoint( pmstestlib.map, pnt, mapscript.MS_SINGLE,
                                   10.0 )

    return 'success'

###############################################################################
# Scan results.  This query is for a pixel at a grid intersection.  This
# pixel should be classified as grid and returned.
#

def rqtest_13():
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

    if count != 1:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 1) )
        return 'fail'

    result = layer.getResult( 0 )
    s = layer.getShape( result )
    
    if pmstestlib.check_items( layer, s,
                               [('value_0','1'),
                                ('red','255'),
                                ('green','0'),
                                ('blue','0'),
                                ('class','Grid'),
                                ('x','13.5'),
                                ('y','36.5')] ) == 0:
        return 'fail'
    
    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Revert to tileindex.map and do a test where we force reprojection

def rqtest_14():

    pmstestlib.map = mapscript.mapObj('../gdal/tileindex.map')
    pmstestlib.layer = pmstestlib.map.getLayer(0)

    pmstestlib.map.setProjection("+proj=utm +zone=30 +datum=WGS84")

    pnt = mapscript.pointObj()
    pnt.x =  889690
    pnt.y =   55369
    
    pmstestlib.layer.queryByPoint( pmstestlib.map, pnt, mapscript.MS_MULTIPLE,
                                   200000.0 )

    return 'success'

###############################################################################
# Check result count, and that the results are within the expected distance.
# This also implicitly verifies the results were reprojected back to UTM
# coordinates from the lat long system of the layer.

def rqtest_15():
    
    layer = pmstestlib.layer
    
    pnt = mapscript.pointObj()
    pnt.x =  889690
    pnt.y =   55369
    
    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult( i )
        if result is None:
            break

        count = count + 1

        s = layer.getShape( result )
        x = float(pmstestlib.get_item_value( layer, s, 'x' ))
        y = float(pmstestlib.get_item_value( layer, s, 'y' ))
        dist_sq = (x-pnt.x) * (x-pnt.x) + (y-pnt.y) * (y-pnt.y)
        dist = math.pow(dist_sq,0.5)
        if dist > 200000.0:
            pmstestlib.post_reason(
                'Got point %f from target, but tolerance was 200000.0.' % dist )
            return 'fail'
                
    
    if count != 4:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 10) )
        return 'fail'

    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Make a similar test with the tileindex file in mapinfo format (#2796)

def rqtest_16():

    pmstestlib.map = mapscript.mapObj('../gdal/tileindex_mi.map')
    pmstestlib.layer = pmstestlib.map.getLayer(0)

    pmstestlib.map.setProjection("+proj=utm +zone=30 +datum=WGS84")

    pnt = mapscript.pointObj()
    pnt.x =  889690
    pnt.y =   55369
    
    pmstestlib.layer.queryByPoint( pmstestlib.map, pnt, mapscript.MS_MULTIPLE,
                                   200000.0 )

    return 'success'

###############################################################################
# Check result count, and that the results are within the expected distance.
# This also implicitly verifies the results were reprojected back to UTM
# coordinates from the lat long system of the layer.

def rqtest_17():
    
    layer = pmstestlib.layer
    
    pnt = mapscript.pointObj()
    pnt.x =  889690
    pnt.y =   55369
    
    #########################################################################
    # Check result count.
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult( i )
        if result is None:
            break

        count = count + 1

        s = layer.getShape( result )
        x = float(pmstestlib.get_item_value( layer, s, 'x' ))
        y = float(pmstestlib.get_item_value( layer, s, 'y' ))
        dist_sq = (x-pnt.x) * (x-pnt.x) + (y-pnt.y) * (y-pnt.y)
        dist = math.pow(dist_sq,0.5)
        if dist > 200000.0:
            pmstestlib.post_reason(
                'Got point %f from target, but tolerance was 200000.0.' % dist )
            return 'fail'
                
    
    if count != 4:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 4) )
        return 'fail'

    layer.close() 
    layer.close() # discard resultset.

    return 'success'
    
###############################################################################
# Test a layer with a tileindex with mixed SRS

def rqtest_18():

    pmstestlib.map = mapscript.mapObj('../gdal/tileindexmixedsrs.map')
    pmstestlib.layer = pmstestlib.map.getLayer(0)

    pmstestlib.map.setProjection("+proj=latlong +datum=WGS84")

    pnt = mapscript.pointObj()
    pnt.x =  -117.6
    pnt.y =   33.9

    pmstestlib.layer.queryByPoint( pmstestlib.map, pnt, mapscript.MS_SINGLE,
                                   0.001 )

    #########################################################################
    # Check result count.
    layer = pmstestlib.layer
    layer.open()
    count = 0
    for i in range(1000):
        result = layer.getResult( i )
        if result is None:
            break

        count = count + 1

        s = layer.getShape( result )
        x = float(pmstestlib.get_item_value( layer, s, 'x' ))
        y = float(pmstestlib.get_item_value( layer, s, 'y' ))
        dist_sq = (x-pnt.x) * (x-pnt.x) + (y-pnt.y) * (y-pnt.y)
        dist = math.pow(dist_sq,0.5)
        if dist > 0.001:
            pmstestlib.post_reason(
                'Got point %f from target, but tolerance was 0.001.' % dist )
            return 'fail'

    if count != 1:
        pmstestlib.post_reason( 'got %d results instead of expected %d.' \
                             % (count, 1) )
        return 'fail'

    layer.close() 
    layer.close() # discard resultset.

    return 'success'

###############################################################################
# Cleanup.

def rqtest_cleanup():
    pmstestlib.layer = None
    pmstestlib.map = None
    return 'success'

test_list = [
    rqtest_1,
    rqtest_2,
    rqtest_3,
    rqtest_4,
    rqtest_5,
    rqtest_6,
    rqtest_7,
    rqtest_8,
    rqtest_9,
    rqtest_9_1,
    rqtest_9_2,
    rqtest_10,
    rqtest_11,
    rqtest_12,
    rqtest_13,
    rqtest_14,
    rqtest_15,
    rqtest_16,
    rqtest_17,
    rqtest_18,
    rqtest_cleanup ]

if __name__ == '__main__':

    pmstestlib.setup_run( 'rqtest' )

    pmstestlib.run_tests( test_list )

    pmstestlib.summarize()

    mapscript.msCleanup(0)

