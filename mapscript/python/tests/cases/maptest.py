# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Map
# Author:   Sean Gillies, sgillies@frii.com
#
# ===========================================================================
# Copyright (c) 2004, Sean Gillies
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# ===========================================================================
#
# Execute this module as a script from mapserver/mapscript/python
#
#     python tests/cases/maptest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from testing import mapscript, MapTestCase, TESTMAPFILE

# ===========================================================================
# Test begins now

class MapConstructorTestCase(unittest.TestCase):

    def testMapConstructorNoArg(self):
        """MapConstructorTestCase.testMapConstructorNoArg: test map constructor with no argument"""
        test_map = mapscript.mapObj()
        maptype = type(test_map)
        assert str(maptype) == "<class 'mapscript.mapObj'>", maptype
        assert test_map.thisown == 1
    
    def testMapConstructorEmptyStringArg(self):
        """MapConstructorTestCase.testMapConstructorEmptyStringArg: test map constructor with old-style empty string argument"""
        test_map = mapscript.mapObj('')
        maptype = type(test_map)
        assert str(maptype) == "<class 'mapscript.mapObj'>", maptype
        assert test_map.thisown == 1
        
    def testMapConstructorFilenameArg(self):
        """MapConstructorTestCasetest.testMapConstructorEmptyStringArg: map constructor with filename argument"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        maptype = type(test_map)
        assert str(maptype) == "<class 'mapscript.mapObj'>", maptype
        assert test_map.thisown == 1

class MapExtentTestCase(MapTestCase):
    def testSetExtent(self):
        """MapExtentTestCase.testSetExtent: test the setting of a mapObj's extent"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        e = test_map.extent
        result = test_map.setExtent(e.minx, e.miny, e.maxx, e.maxy)
        self.assertAlmostEqual(test_map.scale, 14.173236)
        assert result == mapscript.MS_SUCCESS, result
        
    def testSetExtentBadly(self):
        """MapExtentTestCase.testSetExtentBadly: test that mapscript raises an error for an invalid mapObj extent"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        self.assertRaises(mapscript.MapServerError, test_map.setExtent,
                          1.0, -2.0, -3.0, 4.0)
        
    def testReBindingExtent(self):
        """MapExtentTestCase.testReBindingExtent: test the rebinding of a map's extent"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        rect1 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        rect2 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        test_map.extent = rect1
        assert str(test_map.extent) != str(rect1)
        del rect1
        self.assertRectsEqual(test_map.extent, rect2)
        
class MapLayersTestCase(MapTestCase):

    def testMapInsertLayer(self):
        """MapLayersTestCase.testMapInsertLayer: test insertion of a new layer at default (last) index"""
        n = self.map.numlayers
        layer = mapscript.layerObj()
        layer.name = 'new'
        index = self.map.insertLayer(layer)
        assert index == n, index
        assert self.map.numlayers == n + 1
        names = [self.map.getLayer(i).name for i in range(self.map.numlayers)]
        assert names == ['POLYGON', 'LINE', 'POINT', 'INLINE', 'new']
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2, 3, 4), order
        
    def testMapInsertLayerAtZero(self):
        """MapLayersTestCase.testMapInsertLayerAtZero: test insertion of a new layer at first index"""
        n = self.map.numlayers
        layer = mapscript.layerObj()
        layer.name = 'new'
        index = self.map.insertLayer(layer, 0)
        assert index == 0, index
        assert self.map.numlayers == n + 1
        names = [self.map.getLayer(i).name for i in range(self.map.numlayers)]
        assert names == ['new', 'POLYGON', 'LINE', 'POINT', 'INLINE'], names 
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2, 3, 4), order

    def testMapInsertLayerDrawingOrder(self):
        """MapLayersTestCase.testMapInsertLayerDrawingOrder: test affect of insertion of a new layer at index 1 on drawing order"""
        n = self.map.numlayers
        # reverse layer drawing order
        o_start = (3, 2, 1, 0)
        self.map.setLayerOrder(o_start)
        # insert Layer
        layer = mapscript.layerObj()
        layer.name = 'new'
        index = self.map.insertLayer(layer, 1)
        assert index == 1, index 
        # We expect our new layer to be at index 1 in drawing order as well
        order = self.map.getLayerOrder()
        assert order == (4, 1, 3, 2, 0), order

    def testMapInsertLayerBadIndex(self):
        """MapLayersTestCase.testMapInsertLayerBadIndex: expect an exception when index is too large"""
        layer = mapscript.layerObj()
        self.assertRaises(mapscript.MapServerChildError, self.map.insertLayer, 
                          layer, 1000)
         
    def testMapRemoveLayerAtTail(self):
        """MapLayersTestCase.testMapRemoveLayerAtTail: test removal of highest index (tail) layer"""
        n = self.map.numlayers
        layer = self.map.removeLayer(n-1)
        assert self.map.numlayers == n-1
        assert layer.name == 'INLINE'
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2), order

    def testMapRemoveLayerAtZero(self):
        """MapLayersTestCase.testMapInsertLayerAtZero: test removal of lowest index (0) layer"""
        n = self.map.numlayers
        layer = self.map.removeLayer(0)
        assert self.map.numlayers == n-1
        assert layer.name == 'POLYGON'
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2), order
        
    def testMapRemoveLayerDrawingOrder(self):
        """MapLayersTestCase.testMapInsertLayerDrawingOrder: test affect of layer removal on drawing order"""
        n = self.map.numlayers
        # reverse layer drawing order
        o_start = (3, 2, 1, 0)
        self.map.setLayerOrder(o_start)
        layer = self.map.removeLayer(1)
        assert self.map.numlayers == n-1
        assert layer.name == 'LINE'
        order = self.map.getLayerOrder()
        assert order == (2, 1, 0), order
        names = [self.map.getLayer(i).name for i in range(self.map.numlayers)]
        assert names == ['POLYGON', 'POINT', 'INLINE'], names  
        
class MapExceptionTestCase(MapTestCase):
    def testDrawBadData(self):
        """MapExceptionTestCase.testDrawBadData: a bad data descriptor in a layer returns proper error"""
        self.map.getLayerByName('POLYGON').data = 'foo'
        self.assertRaises(mapscript.MapServerError, self.map.draw)
    def testZeroResultsQuery(self):
        """MapExceptionTestCase.testZeroResultsQuery: zero query results returns proper error"""
        p = mapscript.pointObj()
        p.x, p.y = (-600000,1000000) # Way outside demo
        self.assertRaises(mapscript.MapServerNotFoundError, \
                         self.map.queryByPoint, p, mapscript.MS_SINGLE, 1.0)

class EmptyMapExceptionTestCase(unittest.TestCase):
    def setUp(self):
        self.map = mapscript.mapObj('')
    def tearDown(self):
        self.map = None
    def testDrawEmptyMap(self):
        """EmptyMapExceptionTestCase.testDrawEmptyMap: drawing an empty map returns proper error"""
        self.assertRaises(mapscript.MapServerError, self.map.draw)

class MapMetaDataTestCase(MapTestCase):
    def testInvalidKeyAccess(self):
        """MapMetaDataTestCase.testInvalidKeyAccess: an invalid map metadata key returns proper error"""
        self.assertRaises(mapscript.MapServerError, self.map.getMetaData, 'foo')
    def testFirstKeyAccess(self):
        """MapMetaDataTestCase.testFirstKeyAccess: first metadata key is correct value"""
        key = self.map.getFirstMetaDataKey()
        assert key == 'key1'
        val = self.map.getMetaData(key)
        assert val == 'value1'
    def testLastKeyAccess(self):
        """MapMetaDataTestCase.testLastKeyAccess: last metadata key is correct value"""
        key = self.map.getFirstMetaDataKey()
        for i in range(3):
            key = self.map.getNextMetaDataKey(key)
            assert key is not None
        key = self.map.getNextMetaDataKey(key)
        assert key is None
    def testMapMetaData(self):
        """MapMetaDataTestCase.testMapMetaData: map metadata keys are correct values"""
        keys = []
        key = self.map.getFirstMetaDataKey()
        if key is not None: keys.append(key)
        while 1:
            key = self.map.getNextMetaDataKey(key)
            if not key:
                break
            keys.append(key)
        assert keys == ['key1', 'key2', 'key3', 'key4'], keys
    def testLayerMetaData(self):
        """MapMetaDataTestCase.testLayerMetaData: layer metadata keys are correct values"""
        keys = []
        key = self.map.getLayer(0).getFirstMetaDataKey()
        if key is not None: keys.append(key)
        while 1:
            key = self.map.getLayer(0).getNextMetaDataKey(key)
            if not key:
                break
            keys.append(key)
        assert keys == ['key1', 'key2', 'key3', 'key4'], keys
    def testClassMetaData(self):
        """MapMetaDataTestCase.testClassMetaData: class metadata keys are correct values"""
        keys = []
        key = self.map.getLayer(0).getClass(0).getFirstMetaDataKey()
        if key is not None: keys.append(key)
        while 1:
            key = self.map.getLayer(0).getClass(0).getNextMetaDataKey(key)
            if not key:
                break
            keys.append(key)
        assert keys == ['key1', 'key2', 'key3', 'key4'], keys
        
# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
