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
test_image = '../../tests/test.png'
fontsetfile = "/home/sean/projects/ms_42/mapserver/tests/fonts.txt"
symbolsetfile = "/home/sean/projects/ms_42/mapserver/tests/symbols.txt"

# Import all from mapscript
import mapscript

# If mapscript is using the next generation names
if 'mapObj' not in dir(mapscript):
    mapscript.mapObj = mapscript.Map
    mapscript.layerObj = mapscript.Layer
    mapscript.classObj = mapscript.Class
    mapscript.styleObj = mapscript.Style
    mapscript.shapeObj = mapscript.Shape
    mapscript.lineObj = mapscript.Line
    mapscript.pointObj = mapscript.Point
    mapscript.rectObj = mapscript.Rect
    mapscript.outputFormatObj = mapscript.OutputFormat
    mapscript.symbolObj = mapscript.Symbol
    mapscript.symbolSetObj = mapscript.SymbolSet
    mapscript.colorObj = mapscript.Color
    mapscript.imageObj = mapscript.Image
    mapscript.shapefileObj = mapscript.Shapefile
    mapscript.projectionObj = mapscript.Projection
    mapscript.fontSetObj = mapscript.FontSet

class MapscriptTestCase(unittest.TestCase):

    def assertAlmostEqual(self, first, second, places=7):
        """Copied from unittest for use with Python 2.1 or 2.2"""
        if round(second-first, places) != 0:
            raise AssertionError, \
                '%s != %s within %s places' % (`first`, `second`, `places`)
        
class MapCloneTestCase(MapscriptTestCase):
    """Base class for testing with a map fixture"""

    def setUp(self):
        self.mapobj_orig = mapscript.mapObj(testMapfile)
        self.mapobj_clone = self.mapobj_orig.clone()
    def tearDown(self):
        self.mapobj_orig = None
        self.mapobj_clone = None

# ---------------------------------------------------------------------------
# The tests begin here

class MapCloningTestCase(MapscriptTestCase):

    def testCloneAndDeleteOriginalFirst(self):
        mapobj_orig = mapscript.mapObj(testMapfile)
        mapobj_clone = mapobj_orig.clone()
        del mapobj_orig
        del mapobj_clone
        
    def testCloneAndDeleteCloneFirst(self):
        mapobj_orig = mapscript.mapObj(testMapfile)
        mapobj_clone = mapobj_orig.clone()
        del mapobj_clone
        del mapobj_orig

class ClonedMapAttributesTestCase(MapCloneTestCase):

    def testClonedName(self):
        assert self.mapobj_clone.name == self.mapobj_orig.name

    def testClonedLayers(self):
        assert self.mapobj_clone.numlayers == self.mapobj_orig.numlayers
        assert self.mapobj_clone.getLayer(0).data == self.mapobj_orig.getLayer(0).data

class FontSetTestCase(MapCloneTestCase):

    def testSetFontSet(self):
        #self.mapobj_clone.setFontSet(fontsetfile)
        assert self.mapobj_clone.fontset.numfonts == 2
    
class SymbolSetTestCase(MapCloneTestCase):

    def testSetSymbolSet(self):
        #self.mapobj_clone.setSymbolSet(symbolsetfile)
        num = self.mapobj_clone.symbolset.numsymbols
        assert num == 2, num
   
class DrawingTestCase(MapCloneTestCase):

    def testDrawClone(self):
        #self.mapobj_clone.setFontSet(fontsetfile)
        #self.mapobj_clone.setSymbolSet(symbolsetfile)
        msimg = self.mapobj_clone.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testClone.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()

if __name__ == '__main__':
    unittest.main()

