# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Layer
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
#     python tests/cases/layertest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from testing import mapscript
from testing import MapTestCase

# ===========================================================================
# Test begins now

class LayerConstructorTestCase(unittest.TestCase):

    def testLayerConstructorNoArg(self):
        """LayerConstructorTestCase.testLayerConstructorNoArg: test layer constructor with no argument"""
        layer = mapscript.layerObj()
        t = type(layer)
        assert str(t) == "<class 'mapscript.layerObj'>", t
        assert layer.thisown == 1
    
    def testLayerConstructorMapArg(self):
        """LayerConstructorTestCase.testLayerConstructorMapArg: test layer constructor with map argument"""
        test_map = mapscript.mapObj()
        layer = mapscript.layerObj(test_map)
        t = type(layer)
        assert str(t) == "<class 'mapscript.layerObj'>", t
        assert layer.thisown == 1
        assert str(layer) == str(test_map.getLayer(0))
     

class LayerExtentTestCase(MapTestCase):
    
    def testPolygonGetExtent(self):
        """LayerExtentTestCase.testPolygonGetExtent: retrieve the extent of a polygon layer"""
        layer = self.map.getLayerByName('POLYGON')
        e = mapscript.rectObj(-0.25, 51.227222, 0.25, 51.727222)
        self.assertRectsEqual(e, layer.getExtent())
        
    def testSetExtentBadly(self):
        """LayerExtentTestCase.testSetExtentBadly: test layer.setExtent() to provoke it to raise an error when given a bogus extent"""
        layer = self.map.getLayerByName('POLYGON')
        self.assertRaises(mapscript.MapServerError, layer.setExtent,
                          1.0, -2.0, -3.0, 4.0)
    
    def testGetPresetExtent(self):
        """LayerExtentTestCase.testGetPresetExtent: test layer.setExtent() and layer.getExtent() to return preset instead of calculating extents"""
        layer = self.map.getLayerByName('POLYGON')
        minx, miny, maxx, maxy = 1.0, 1.0, 3.0, 3.0
        layer.setExtent(minx, miny, maxx, maxy)
        rect = layer.getExtent()
        assert minx == rect.minx
        assert miny == rect.miny
        assert maxx == rect.maxx
        assert maxy == rect.maxy
        
    def testResetLayerExtent(self):
        """LayerExtentTestCase.testResetLayerExtent: test resetting a layer's extent"""
        layer = self.map.getLayerByName('POLYGON')
        layer.setExtent(-1.0, -1.0, -1.0, -1.0)
        self.testPolygonGetExtent()

    def testReBindingExtent(self):
        """LayerExtentTestCase.testReBindingExtent: rebind a layer's extent"""
        layer = self.map.getLayerByName('POLYGON')
        rect1 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        rect2 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        layer.extent = rect1
        assert str(layer.extent) != str(rect1)
        del rect1
        self.assertRectsEqual(layer.extent, rect2)
        self.assertRectsEqual(layer.getExtent(), rect2)
       
    def testDirectExtentAccess(self):
        """LayerExtentTestCase.testDirectExtentAccess: direct access to a layer's extent works properly"""
        layer = self.map.getLayerByName('POINT')
        rect = layer.extent
        assert str(layer.extent) == str(rect), (layer.extent, rect)
        e = mapscript.rectObj(-0.5, 51.0, 0.5, 52.0)
        self.assertRectsEqual(e, rect)
        
class LayerRasterProcessingTestCase(MapTestCase):
    
    def testSetProcessing(self):
        """LayerRasterProcessingTestCase.testSetProcessing: setting a layer's processing directive works"""
        layer = self.map.getLayer(0)
        layer.setProcessing('directive0=foo')
        assert layer.numprocessing == 1, layer.numprocessing
        layer.setProcessing('directive1=bar')
        assert layer.numprocessing == 2, layer.numprocessing
        directives = [layer.getProcessing(i) \
                      for i in range(layer.numprocessing)]
        assert directives == ['directive0=foo', 'directive1=bar']

    def testClearProcessing(self):
        """LayerRasterProcessingTestCase.testClearProcessing: clearing a layer's processing directive works"""
        layer = self.map.getLayer(0)
        layer.setProcessing('directive0=foo')
        assert layer.numprocessing == 1, layer.numprocessing
        layer.setProcessing('directive1=bar')
        assert layer.numprocessing == 2, layer.numprocessing
        assert layer.clearProcessing() == mapscript.MS_SUCCESS

class RemoveClassTestCase(MapTestCase):

    def testRemoveClass1NumClasses(self):
        """RemoveClassTestCase.testRemoveClass1NumClasses: removing the layer's first class by index leaves one class left"""
        layer = self.map.getLayer(0)
        layer.removeClass(0)
        assert layer.numclasses == 1
    
    def testRemoveClass1ClassName(self):
        """RemoveClassTestCase.testRemoveClass1ClassName: confirm removing the layer's first class reverts the name of the second class"""
        layer = self.map.getLayer(0)
        c2name = layer.getClass(1).name
        layer.removeClass(0)
        assert layer.getClass(0).name == c2name
    
    def testRemoveClass2NumClasses(self):
        """RemoveClassTestCase.testRemoveClass2NumClasses: removing the layer's second class by index leaves one class left"""
        layer = self.map.getLayer(0)
        layer.removeClass(1)
        assert layer.numclasses == 1
    
    def testRemoveClass2ClassName(self):
        """RemoveClassTestCase.testRemoveClass2ClassName: confirm removing the layer's second class reverts the name of the first class"""
        layer = self.map.getLayer(0)
        c1name = layer.getClass(0).name
        layer.removeClass(1)
        assert layer.getClass(0).name == c1name

class LayerTestCase(MapTestCase):
    def testLayerConstructorOwnership(self):
        """LayerTestCase.testLayerConstructorOwnership: newly constructed layer has proper ownership"""
        layer = mapscript.layerObj(self.map)
        assert layer.thisown == 1
    def testGetLayerOrder(self):
        """LayerTestCase.testGetLayerOrder: get layer drawing order"""
        order = self.map.getLayerOrder()
        assert order == tuple(range(4)), order
    def testSetLayerOrder(self):
        """LayerTestCase.testSetLayerOrder: set layer drawing order"""
        ord = (1, 0, 2, 3)
        self.map.setLayerOrder(ord)
        order = self.map.getLayerOrder()
        assert order == ord, order

# Layer removal tests
class RemoveLayerTestCase(MapTestCase):
    def testRemoveLayer1NumLayers(self):
        """RemoveLayerTestCase.testRemoveLayer1NumLayers: removing the first layer by index from the mapfile leaves three layers left"""
        self.map.removeLayer(0)
        assert self.map.numlayers == 3
    def testRemoveLayer1LayerName(self):
        """RemoveLayerTestCase.testRemoveLayer1LayerName: confirm removing of the first layer reverts it to the second layer's name"""
        l2name = self.map.getLayer(1).name
        self.map.removeLayer(0)
        assert self.map.getLayer(0).name == l2name
    def testRemoveLayer2NumLayers(self):
        """RemoveLayerTestCase.testRemoveLayer2NumLayers: removing the second layer by index from the mapfile leaves three layers left"""
        self.map.removeLayer(1)
        assert self.map.numlayers == 3
    def testRemoveLayer2LayerName(self):
        """RemoveLayerTestCaseconfirm.testRemoveLayer2LayerName: removing of the second layer reverts it to the first layer's name"""
        l1name = self.map.getLayer(0).name
        self.map.removeLayer(1)
        assert self.map.getLayer(0).name == l1name
        
# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
