# $Id$
#
# Unit tests concerning the removeLayer() and removeClass() extension
# methods.

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

# Import mapscript and define the test case classes
from mapscript import *

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

if __name__ == '__main__':
    unittest.main()

