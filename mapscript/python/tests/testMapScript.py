# $Id$
#
# Unit tests concerning getLayerOrder(), setLayerOrder(),
# raiseLayer(), lowerLayer() extensions.

import os, sys
import distutils.util
import unittest

# Construct the distutils build path, allowing us to run unit tests 
# before the module is installed
platformdir = '-'.join((distutils.util.get_platform(), 
                       '.'.join(map(str, sys.version_info[0:2]))))
sys.path.insert(0, 'build/lib.' + platformdir)

# Our testing mapfile
testMapfile = 'tests/test.map'

# Import all from mapscript
from mapscript import *

# Layer ordering tests
class LayerOrderTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testGetLayerOrder(self):
        order = self.mapobj1.getLayerOrder()
        assert order == (0, 1), order
    def testPromoteLayer1(self):
        self.mapobj1.getLayer(1).promote()
        order = self.mapobj1.getLayerOrder()
        assert order == (1, 0), order
    def testDemoteLayer0(self):
        self.mapobj1.getLayer(0).demote()
        order = self.mapobj1.getLayerOrder()
        assert order == (1, 0), order
    def testSetLayerOrder(self):
        self.mapobj1.setLayerOrder((1, 0))
        order = self.mapobj1.getLayerOrder()
        assert order == (1, 0), order

# Layer removal tests
class RemoveLayerTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testRemoveLayer1NumLayers(self):
        self.mapobj1.removeLayer(0)
        assert self.mapobj1.numlayers == 1
    def testRemoveLayer1LayerName(self):
        l2name = self.mapobj1.getLayer(1).name
        self.mapobj1.removeLayer(0)
        assert self.mapobj1.getLayer(0).name == l2name
    def testRemoveLayer2NumLayers(self):
        self.mapobj1.removeLayer(1)
        assert self.mapobj1.numlayers == 1
    def testRemoveLayer2LayerName(self):
        l1name = self.mapobj1.getLayer(0).name
        self.mapobj1.removeLayer(1)
        assert self.mapobj1.getLayer(0).name == l1name

# class removal tests
class RemoveClassTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testRemoveClass1NumClasses(self):
        self.mapobj1.getLayer(0).removeClass(0)
        assert self.mapobj1.getLayer(0).numclasses == 1
    def testRemoveClass1ClassName(self):
        c2name = self.mapobj1.getLayer(0).getClass(1).name
        self.mapobj1.getLayer(0).removeClass(0)
        assert self.mapobj1.getLayer(0).getClass(0).name == c2name
    def testRemoveClass2NumClasses(self):
        self.mapobj1.getLayer(0).removeClass(1)
        assert self.mapobj1.getLayer(0).numclasses == 1
    def testRemoveClass2ClassName(self):
        c1name = self.mapobj1.getLayer(0).getClass(0).name
        self.mapobj1.getLayer(0).removeClass(1)
        assert self.mapobj1.getLayer(0).getClass(0).name == c1name

# symbolset tests
class SymbolSetTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testDefaultNumSymbols(self):
        assert mapobj.symbolset.numsymbols == 1

if __name__ == '__main__':
    unittest.main()

