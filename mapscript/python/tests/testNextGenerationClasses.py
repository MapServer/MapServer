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
testMapfile = '../../tests/test.map'
testNoFontSetMapfile = '../../tests/test_nofontset.map'
test_image = '../../tests/test.png'

import mapscript

if 'mapObj' not in dir(mapscript):
    mapscript.mapObj = mapscript.Map

# mapscript Layer tests
class LayerTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testGetShape(self):
        polygon_layer = self.mapobj1.getLayerByName('POLYGON')
        polygon_layer.open()
        s = polygon_layer.getShape(0)
        l = s.get(0)
        p = l.get(0)
        assert int(p.x * 10) == -2, p.x
        assert int(p.y * 10) == 512, p.y
    
if __name__ == '__main__':
    unittest.main()

