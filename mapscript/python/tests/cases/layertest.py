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
        "test layer constructor with no argument"
        layer = mapscript.layerObj()
        t = type(layer)
        assert str(t) == "<class 'mapscript.layerObj'>", t
        assert layer.thisown == 1
    
    def testLayerConstructorMapArg(self):
        "test layer constructor with map argument"
        test_map = mapscript.mapObj()
        layer = mapscript.layerObj(test_map)
        t = type(layer)
        assert str(t) == "<class 'mapscript.layerObj'>", t
        assert layer.thisown == 1
        assert str(layer) == str(test_map.getLayer(0))
     

class LayerExtentTestCase(MapTestCase):
    
    def testPolygonGetExtent(self):
        layer = self.map.getLayerByName('POLYGON')
        e = mapscript.rectObj(-0.25, 51.227222, 0.25, 51.727222)
        self.assertRectsEqual(e, layer.getExtent())
    def testSetExtentBadly(self):
        layer = self.map.getLayerByName('POLYGON')
        self.assertRaises(mapscript.MapServerError, layer.setExtent,
                          1.0, -2.0, -3.0, 4.0)  

class LayerRasterProcessingTestCase(MapTestCase):
    
    def testSetProcessing(self):
        layer = self.map.getLayer(0)
        layer.setProcessing('directive0=foo')
        assert layer.numprocessing == 1, layer.numprocessing
        layer.setProcessing('directive1=bar')
        assert layer.numprocessing == 2, layer.numprocessing
        directives = [layer.getProcessing(i) \
                      for i in range(layer.numprocessing)]
        assert directives == ['directive0=foo', 'directive1=bar']

    def testClearProcessing(self):
        layer = self.map.getLayer(0)
        layer.setProcessing('directive0=foo')
        assert layer.numprocessing == 1, layer.numprocessing
        layer.setProcessing('directive1=bar')
        assert layer.numprocessing == 2, layer.numprocessing
        assert layer.clearProcessing() == mapscript.MS_SUCCESS

class RemoveClassTestCase(MapTestCase):

    def testRemoveClass1NumClasses(self):
        layer = self.map.getLayer(0)
        layer.removeClass(0)
        assert layer.numclasses == 1
    
    def testRemoveClass1ClassName(self):
        layer = self.map.getLayer(0)
        c2name = layer.getClass(1).name
        layer.removeClass(0)
        assert layer.getClass(0).name == c2name
    
    def testRemoveClass2NumClasses(self):
        layer = self.map.getLayer(0)
        layer.removeClass(1)
        assert layer.numclasses == 1
    
    def testRemoveClass2ClassName(self):
        layer = self.map.getLayer(0)
        c1name = layer.getClass(0).name
        layer.removeClass(1)
        assert layer.getClass(0).name == c1name

 
# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
