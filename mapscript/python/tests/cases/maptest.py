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
        "test map constructor with no argument"
        test_map = mapscript.mapObj()
        maptype = type(test_map)
        assert str(maptype) == "<class 'mapscript.mapObj'>", maptype
        assert test_map.thisown == 1
    
    def testMapConstructorEmptyStringArg(self):
        "test map constructor with old-style empty string argument"
        test_map = mapscript.mapObj('')
        maptype = type(test_map)
        assert str(maptype) == "<class 'mapscript.mapObj'>", maptype
        assert test_map.thisown == 1
        
    def testMapConstructorFilenameArg(self):
        "test map constructor with filename argument"
        test_map = mapscript.mapObj(TESTMAPFILE)
        maptype = type(test_map)
        assert str(maptype) == "<class 'mapscript.mapObj'>", maptype
        assert test_map.thisown == 1

        
class MapLayersTestCase(MapTestCase):

    def testMapInsertLayer(self):
        "test insertion of a new layer at default (last) index"
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
        "test insertion of a new layer at first index"
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
        "test affect of insertion of a new layer at index 1 on drawing order"
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
        "expect an exception when index is too large"
        layer = mapscript.layerObj()
        self.assertRaises(mapscript.MapServerChildError, self.map.insertLayer, 
                          layer, 1000)
         
    def testMapRemoveLayerAtTail(self):
        "test removal of highest index (tail) layer"
        n = self.map.numlayers
        layer = self.map.removeLayer(n-1)
        assert self.map.numlayers == n-1
        assert layer.name == 'INLINE'
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2), order

    def testMapRemoveLayerAtZero(self):
        "test removal of lowest index (0) layer"
        n = self.map.numlayers
        layer = self.map.removeLayer(0)
        assert self.map.numlayers == n-1
        assert layer.name == 'POLYGON'
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2), order
        
    def testMapRemoveLayerDrawingOrder(self):
        "test affect of layer removal on drawing order"
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
        
                
# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
