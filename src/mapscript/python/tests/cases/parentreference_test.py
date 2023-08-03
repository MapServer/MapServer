# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Map
# Author:   Umberto Nicoletti, unicoletti@prometeo.it
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

import gc
import unittest
import mapscript
from .testing import TESTMAPFILE


class ParentReferenceTestCase(unittest.TestCase):

    def initMap(self):
        self.map = mapscript.mapObj(TESTMAPFILE)

    def testGetLayerObj(self):
        self.initMap()
        layer = self.map.getLayer(1)
        self.map = None
        assert str(layer.p_map).find('mapscript.mapObj') != -1
        gc.collect()
        assert layer.map is not None, layer.map

    def testGetLayerObjByName(self):
        self.initMap()
        layer = self.map.getLayerByName('POLYGON')
        self.map = None
        assert str(layer.p_map).find('mapscript.mapObj') != -1
        gc.collect()
        assert layer.map is not None, layer.map

    def testLayerObj(self):
        self.initMap()
        layer = mapscript.layerObj(self.map)
        self.map = None
        assert str(layer.p_map).find('mapscript.mapObj') != -1
        gc.collect()
        assert layer.map is not None, layer.map

    def testInsertLayerObj(self):
        self.initMap()
        layer = mapscript.layerObj()
        self.map.insertLayer(layer)
        self.map = None
        assert str(layer.p_map).find('mapscript.mapObj') != -1
        gc.collect()
        assert layer.map is not None, layer.map

    def testGetClassObj(self):
        self.initMap()
        layer = self.map.getLayer(1)
        clazz = layer.getClass(0)
        self.map = None
        layer = None
        assert str(clazz.p_layer).find('mapscript.layerObj') != -1
        gc.collect()
        assert clazz.layer is not None, clazz.layer

    def testClassObj(self):
        self.initMap()
        layer = mapscript.layerObj(self.map)
        clazz = mapscript.classObj(layer)
        self.map = None
        layer = None
        assert str(clazz.p_layer).find('mapscript.layerObj') != -1
        gc.collect()
        assert clazz.layer is not None, clazz.layer

    def testInsertClassObj(self):
        self.initMap()
        layer = mapscript.layerObj()
        clazz = mapscript.classObj()
        layer.insertClass(clazz)
        self.map = None
        layer = None
        assert str(clazz.p_layer).find('mapscript.layerObj') != -1
        gc.collect()
        assert clazz.layer is not None, clazz.layer

    def testRemoveClassObj(self):
        self.initMap()
        layer = self.map.getLayer(1)
        clazz = layer.removeClass(0)
        if hasattr(clazz, 'p_layer'):
            assert clazz.p_layer is None, clazz.p_layer
        assert clazz.layer is None, clazz.layer
        self.initMap()
        position = self.map.getLayer(0).insertClass(clazz)
        assert position == 0, position
        assert clazz.p_layer is not None, clazz.p_layer
        assert clazz.layer is not None, clazz.layer


if __name__ == '__main__':
    unittest.main()
