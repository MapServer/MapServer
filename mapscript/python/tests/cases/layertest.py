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

# Base class
class MapLayerTestCase(MapTestCase):

    def setUp(self):
        MapTestCase.setUp(self)
        self.layer = self.map.getLayer(0)

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
   
    def setUp(self):
        MapTestCase.setUp(self)
        self.layer = self.map.getLayerByName('POLYGON')

    def testPolygonGetExtent(self):
        """LayerExtentTestCase.testPolygonGetExtent: retrieve the extent of a polygon layer"""
        e = mapscript.rectObj(-0.25, 51.227222, 0.25, 51.727222)
        self.assertRectsEqual(e, self.layer.getExtent())
        
    def testSetExtentBadly(self):
        """LayerExtentTestCase.testSetExtentBadly: test layer.setExtent() to provoke it to raise an error when given a bogus extent"""
        self.assertRaises(mapscript.MapServerError, self.layer.setExtent,
                          1.0, -2.0, -3.0, 4.0)
    
    def testGetPresetExtent(self):
        """LayerExtentTestCase.testGetPresetExtent: test layer.setExtent() and layer.getExtent() to return preset instead of calculating extents"""
        minx, miny, maxx, maxy = 1.0, 1.0, 3.0, 3.0
        self.layer.setExtent(minx, miny, maxx, maxy)
        rect = self.layer.getExtent()
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
        rect1 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        rect2 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        self.layer.extent = rect1
        assert repr(self.layer.extent) != repr(rect1)
        del rect1
        self.assertRectsEqual(self.layer.extent, rect2)
        self.assertRectsEqual(self.layer.getExtent(), rect2)
       
    def testDirectExtentAccess(self):
        """LayerExtentTestCase.testDirectExtentAccess: direct access to a layer's extent works properly"""
        pt_layer = self.map.getLayerByName('POINT')
        rect = pt_layer.extent
        assert str(pt_layer.extent) == str(rect), (pt_layer.extent, rect)
        e = mapscript.rectObj(-0.5, 51.0, 0.5, 52.0)
        self.assertRectsEqual(e, rect)
        
class LayerRasterProcessingTestCase(MapLayerTestCase):
    
    def testSetProcessing(self):
        """LayerRasterProcessingTestCase.testSetProcessing: setting a layer's processing directive works"""
        self.layer.setProcessing('directive0=foo')
        assert self.layer.numprocessing == 1, self.layer.numprocessing
        self.layer.setProcessing('directive1=bar')
        assert self.layer.numprocessing == 2, self.layer.numprocessing
        directives = [self.layer.getProcessing(i) \
                      for i in range(self.layer.numprocessing)]
        assert directives == ['directive0=foo', 'directive1=bar']

    def testClearProcessing(self):
        """LayerRasterProcessingTestCase.testClearProcessing: clearing a self.layer's processing directive works"""
        self.layer.setProcessing('directive0=foo')
        assert self.layer.numprocessing == 1, self.layer.numprocessing
        self.layer.setProcessing('directive1=bar')
        assert self.layer.numprocessing == 2, self.layer.numprocessing
        assert self.layer.clearProcessing() == mapscript.MS_SUCCESS

class RemoveClassTestCase(MapLayerTestCase):

    def testRemoveClass1NumClasses(self):
        """RemoveClassTestCase.testRemoveClass1NumClasses: removing the layer's first class by index leaves one class left"""
        self.layer.removeClass(0)
        assert self.layer.numclasses == 1
    
    def testRemoveClass1ClassName(self):
        """RemoveClassTestCase.testRemoveClass1ClassName: confirm removing the layer's first class reverts the name of the second class"""
        c2name = self.layer.getClass(1).name
        self.layer.removeClass(0)
        assert self.layer.getClass(0).name == c2name
    
    def testRemoveClass2NumClasses(self):
        """RemoveClassTestCase.testRemoveClass2NumClasses: removing the layer's second class by index leaves one class left"""
        self.layer.removeClass(1)
        assert self.layer.numclasses == 1
    
    def testRemoveClass2ClassName(self):
        """RemoveClassTestCase.testRemoveClass2ClassName: confirm removing the layer's second class reverts the name of the first class"""
        c1name = self.layer.getClass(0).name
        self.layer.removeClass(1)
        assert self.layer.getClass(0).name == c1name

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
        """removing the first layer by index from the mapfile leaves three"""
        self.map.removeLayer(0)
        assert self.map.numlayers == 3
    
    def testRemoveLayer1LayerName(self):
        """removing first layer reverts it to the second layer's name"""
        l2name = self.map.getLayer(1).name
        self.map.removeLayer(0)
        assert self.map.getLayer(0).name == l2name
    
    def testRemoveLayer2NumLayers(self):
        """removing second layer by index from mapfile leaves three layers"""
        self.map.removeLayer(1)
        assert self.map.numlayers == 3
    
    def testRemoveLayer2LayerName(self):
        """removing of the second layer reverts it to the first layer's name"""
        l1name = self.map.getLayer(0).name
        self.map.removeLayer(1)
        assert self.map.getLayer(0).name == l1name

class ExpressionTestCase(MapLayerTestCase):
    
    def testClearExpression(self):
        """layer expression can be properly cleared"""
        self.layer.setFilter('')
        fs = self.layer.getFilterString()
        assert fs == '"(null)"', fs
    
    def testSetStringExpression(self):
        """layer expression can be set to string"""
        self.layer.setFilter('foo')
        fs = self.layer.getFilterString()
        assert fs == '"foo"', fs
    
    def testSetQuotedStringExpression(self):
        """layer expression string can be quoted"""
        self.layer.setFilter('"foo"')
        fs = self.layer.getFilterString()
        assert fs == '"foo"', fs
    
    def testSetRegularExpression(self):
        """layer expression can be regular expression"""
        self.layer.setFilter('/foo/')
        fs = self.layer.getFilterString()
        assert fs == '/foo/', fs
    
    def testSetLogicalExpression(self):
        """layer expression can be logical expression"""
        self.layer.setFilter('([foo] >= 2)')
        fs = self.layer.getFilterString()
        assert fs == '([foo] >= 2)', fs
       

class LayerQueryTestCase(MapLayerTestCase):

    def setUp(self):
        MapLayerTestCase.setUp(self)
        self.layer = self.map.getLayerByName('POINT')
        self.layer.template = 'foo'

    def testRectQuery(self):
        qrect = mapscript.rectObj(-10.0, 45.0, 10.0, 55.0)
        self.layer.queryByRect(self.map, qrect)
        assert self.layer.getNumResults() == 1

    def testShapeQuery(self):
        qrect = mapscript.rectObj(-10.0, 45.0, 10.0, 55.0)
        qshape = qrect.toPolygon()
        self.layer.queryByShape(self.map, qshape)
        assert self.layer.getNumResults() == 1

    def testPointQuery(self):
        qpoint = mapscript.pointObj(0.0, 51.5)
        self.layer.queryByPoint(self.map, qpoint, mapscript.MS_MULTIPLE, 2.0)
        assert self.layer.getNumResults() == 1
   
    def testAttributeQuery(self):
        self.layer.queryByAttributes(self.map, "FNAME", '"A Point"',
                                     mapscript.MS_MULTIPLE)
        assert self.layer.getNumResults() == 1

    def testRectQueryNoResults(self):
        qrect = mapscript.rectObj(-101.0, 0.0, -100.0, 1.0)
        self.layer.queryByRect(self.map, qrect)
        assert self.layer.getNumResults() == 0

    def testShapeQueryNoResults(self):
        qrect = mapscript.rectObj(-101.0, 0.0, -100.0, 1.0)
        qshape = qrect.toPolygon()
        self.layer.queryByShape(self.map, qshape)
        assert self.layer.getNumResults() == 0

    def testPointQueryNoResults(self):
        qpoint = mapscript.pointObj(-100.0, 51.5)
        self.layer.queryByPoint(self.map, qpoint, mapscript.MS_MULTIPLE, 2.0)
        assert self.layer.getNumResults() == 0
    
    def testAttributeQueryNoResults(self):
        self.layer.queryByAttributes(self.map, "FNAME", '"Bogus Point"',
                                     mapscript.MS_MULTIPLE)
        assert self.layer.getNumResults() == 0


# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
