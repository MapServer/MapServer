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

# Import mapscript and define the test case classes
from mapscript import *

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
        
if __name__ == '__main__':
    unittest.main()

