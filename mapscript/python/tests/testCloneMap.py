# $Id$

import os
import sys
import unittest
import cStringIO
import urllib
import distutils.util

from mapscripttest import MapscriptTestCase, MapPrimitivesTestCase
from mapscripttest import MapTestCase, MapZoomTestCase, ShapeObjTestCase
from mapscripttest import normalize_class_names, test_image, testMapfile

# Put local build directory on path
platformdir = '-'.join((distutils.util.get_platform(), 
                        '.'.join(map(str, sys.version_info[0:2]))))
sys.path.insert(0, os.path.join('build', 'lib.' + platformdir))

import mapscript
normalize_class_names(mapscript)

# ===========================================================================
# The test begins now!

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

    def testFontSetNumfonts(self):
        assert self.mapobj_clone.fontset.numfonts == 2
    
class SymbolSetTestCase(MapCloneTestCase):

    def testSymbolSetNumsymbols(self):
        num = self.mapobj_clone.symbolset.numsymbols
        assert num == 2, num
   
class DrawingTestCase(MapCloneTestCase):

    def testDrawClone(self):
        msimg = self.mapobj_clone.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testClone.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()

if __name__ == '__main__':
    unittest.main()

