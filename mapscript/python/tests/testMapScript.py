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
        assert order == tuple(range(10)), order
    def testPromoteLayer1(self):
        self.mapobj1.getLayer(1).promote()
        order = self.mapobj1.getLayerOrder()
        assert order == (0, 2, 1) + tuple(range(3,10)), order
    def testDemoteLayer1(self):
        self.mapobj1.getLayer(1).demote()
        order = self.mapobj1.getLayerOrder()
        assert order == (1, 0) + tuple(range(2,10)), order
    def testSetLayerOrder(self):
        ord = (1, 0) + tuple(range(2,10))
        self.mapobj1.setLayerOrder(ord)
        order = self.mapobj1.getLayerOrder()
        assert order == ord, order

# Layer removal tests
class RemoveLayerTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testRemoveLayer1NumLayers(self):
        self.mapobj1.removeLayer(0)
        assert self.mapobj1.numlayers == 9
    def testRemoveLayer1LayerName(self):
        l2name = self.mapobj1.getLayer(1).name
        self.mapobj1.removeLayer(0)
        assert self.mapobj1.getLayer(0).name == l2name
    def testRemoveLayer2NumLayers(self):
        self.mapobj1.removeLayer(1)
        assert self.mapobj1.numlayers == 9
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
    def testGetNumSymbols(self):
        num = self.mapobj1.getNumSymbols()
        assert num == 4, num
    def testSymbolsetNumsymbols(self):
        num = self.mapobj1.symbolset.numsymbols
        assert num == 4, num
    def testSymbolsetSymbolNames(self):
        set = self.mapobj1.symbolset
        names = [None, 'circle', 'line', 'tie']
        for i in range(set.numsymbols):
            symbol = set.getSymbol(i)
            assert symbol.name == names[i], symbol.name

# fontset tests
class FontSetTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testGetFontSetFile(self):
        file = self.mapobj1.fontset.filename
        assert file == 'fonts.txt', file
    #def testGetFonts(self):
    #    items = [('LucidaSansRegular',
    #              '/Users/sean/Library/Fonts/LucidaSansRegular.ttf'), 
    #             ('LucidaSansDemiBold',
    #              '/Users/sean/Library/Fonts/LucidaSansDemiBold.ttf')]
    #    f = self.mapobj1.fontset.getFonts()
    #    assert f == items, f

class EmptyMapExceptionTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj('')
    def tearDown(self):
        self.mapobj1 = None
    def testDrawEmptyMap(self):
        self.assertRaises(MapServerError, self.mapobj1.draw)

class TestMapExceptionTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testDrawBadData(self):
        self.mapobj1.getLayerByName('ctybdpy2').data = 'foo'
        self.assertRaises(MapServerError, self.mapobj1.draw)
    def testZeroResultsQuery(self):
        p = pointObj()
        p.x, p.y = (-600000,1000000) # Way outside demo
        self.assertRaises(MapServerNotFoundError, \
                         self.mapobj1.queryByPoint, p, MS_SINGLE, 1.0)

class TestMapCloneTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testCloneMap(self):
        mapobj_clone = self.mapobj1.clone()
        assert mapobj_clone.thisown == 1
        assert mapobj_clone.name == self.mapobj1.name
        assert mapobj_clone.numlayers == self.mapobj1.numlayers
        del mapobj_clone

# If PIL is available, use it to test the saveToString() method

have_image = 0
try:
    from PIL import Image
    have_image = 1
except ImportError:
    pass
    
from StringIO import StringIO

class SaveToStringTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testSaveToString(self):
        msimg = self.mapobj1.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testSaveToString.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()
        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.format == 'PNG'
            assert pyimg.size == (600, 600)
            assert pyimg.mode == 'P'
        else:
            assert 1

class NoFontSetTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj('')
    def tearDown(self):
        self.mapobj1 = None
    def testNoGetFontSetFile(self):
        assert self.mapobj1.fontset.filename == None

class ExpressionTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testClearExpression(self):
        self.mapobj1.getLayer(0).setFilter('')
        fs = self.mapobj1.getLayer(0).getFilterString()
        assert fs == '"(null)"', fs
    def testSetStringExpression(self):
        self.mapobj1.getLayer(0).setFilter('foo')
        fs = self.mapobj1.getLayer(0).getFilterString()
        assert fs == '"foo"', fs
    def testSetQuotedStringExpression(self):
        self.mapobj1.getLayer(0).setFilter('"foo"')
        fs = self.mapobj1.getLayer(0).getFilterString()
        assert fs == '"foo"', fs
    def testSetRegularExpression(self):
        self.mapobj1.getLayer(0).setFilter('/foo/')
        fs = self.mapobj1.getLayer(0).getFilterString()
        assert fs == '/foo/', fs
    def testSetLogicalExpression(self):
        self.mapobj1.getLayer(0).setFilter('([foo] >= 2)')
        #self.mapobj1.getLayer(0).setFilter('foo')
        fs = self.mapobj1.getLayer(0).getFilterString()
        assert fs == '([foo] >= 2)', fs
       
class ZoomPointTestCase(unittest.TestCase):
    "testing new zoom* methods that we are adapting from the PHP MapScript"
    
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
        # Change the extent for purposes of zoom testing
        rect = rectObj()
        rect.minx, rect.miny, rect.maxx, rect.maxy = (-50.0, -50.0, 50.0, 50.0)
        self.mapobj1.extent = rect
        # Change height/width as well
        self.mapobj1.width, self.mapobj1.height = (100, 100)
    def tearDown(self):
        self.mapobj1 = None
    def testRecenter(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(1, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == -50.0, new_extent.minx
        assert new_extent.miny == -50.0, new_extent.miny
        assert new_extent.maxx == 50.0, new_extent.maxx
        assert new_extent.maxy == 50.0, new_extent.maxy
    def testZoomInPoint(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == -25.0, new_extent.minx
        assert new_extent.miny == -25.0, new_extent.miny
        assert new_extent.maxx == 25.0, new_extent.maxx
        assert new_extent.maxy == 25.0, new_extent.maxy
    def testZoomOutPoint(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == -100.0, new_extent.minx
        assert new_extent.miny == -100.0, new_extent.miny
        assert new_extent.maxx == 100.0, new_extent.maxx
        assert new_extent.maxy == 100.0, new_extent.maxy
    def testZoomOutPointConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        p = pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-4, p, w, h, extent, max)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == max.minx, new_extent.minx
        assert new_extent.miny == max.miny, new_extent.miny
        assert new_extent.maxx == max.maxx, new_extent.maxx
        assert new_extent.maxy == max.maxy, new_extent.maxy
    def testZoomBadSize(self):
        p = pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        w = 0
        h = -1
        self.assertRaises(MapServerError, 
            self.mapobj1.zoomPoint, -2, p, w, h, extent, None);
    def testZoomBadExtent(self):
        p = pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        extent.maxx = extent.maxx - 1000000
        w = 100
        h = 100
        self.assertRaises(MapServerError, 
            self.mapobj1.zoomPoint, -2, p, w, h, extent, None);

class ZoomRectangleTestCase(unittest.TestCase):
    "testing new zoom* methods that we are adapting from the PHP MapScript"
    
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
        # Change the extent for purposes of zoom testing
        rect = rectObj()
        rect.minx, rect.miny, rect.maxx, rect.maxy = (-50.0, -50.0, 50.0, 50.0)
        self.mapobj1.extent = rect
        # Change height/width as well
        self.mapobj1.width, self.mapobj1.height = (100, 100)
    def tearDown(self):
        self.mapobj1 = None
    def testZoomRectangle(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        r = rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (0, 25, 25, 0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomRectangle(r, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == -50.0, new_extent.minx
        assert new_extent.miny == 25.0, new_extent.miny
        assert new_extent.maxx == -25.0, new_extent.maxx
        assert new_extent.maxy == 50.0, new_extent.maxy
    def testZoomRectangleConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        r = rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (0, 200, 200, 0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomRectangle(r, w, h, extent, max)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == max.minx, new_extent.minx
        assert new_extent.miny == max.miny, new_extent.miny
        assert new_extent.maxx == max.maxx, new_extent.maxx
        assert new_extent.maxy == max.maxy, new_extent.maxy
    def testZoomRectangleBadly(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        r = rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (0, 0, 200, 200)
        extent = self.mapobj1.extent
        self.assertRaises(MapServerError, 
            self.mapobj1.zoomRectangle, r, w, h, extent, None)

class ZoomScaleTestCase(unittest.TestCase):
    "testing new zoom* methods that we are adapting from the PHP MapScript"
   
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
        # Change the extent for purposes of zoom testing
        rect = rectObj()
        rect.minx, rect.miny, rect.maxx, rect.maxy = (-50.0, -50.0, 50.0, 50.0)
        self.mapobj1.extent = rect
        # Change height/width as well
        self.mapobj1.width, self.mapobj1.height = (100, 100)
    def tearDown(self):
        self.mapobj1 = None
    def testRecenter(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = pointObj()
        p.x, p.y = (50, 50)
        scale = 2834.6472
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == -50.0, new_extent.minx
        assert new_extent.miny == -50.0, new_extent.miny
        assert new_extent.maxx == 50.0, new_extent.maxx
        assert new_extent.maxy == 50.0, new_extent.maxy
    def testZoomInScale(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = pointObj()
        p.x, p.y = (50, 50)
        scale = 1417.3236
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == -25.0, new_extent.minx
        assert new_extent.miny == -25.0, new_extent.miny
        assert new_extent.maxx == 25.0, new_extent.maxx
        assert new_extent.maxy == 25.0, new_extent.maxy
    def testZoomOutScale(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = pointObj()
        p.x, p.y = (50, 50)
        scale = 5669.2944
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == -100.0, new_extent.minx
        assert new_extent.miny == -100.0, new_extent.miny
        assert new_extent.maxx == 100.0, new_extent.maxx
        assert new_extent.maxy == 100.0, new_extent.maxy
    def testZoomOutPointConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        p = pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        scale = 10000
        self.mapobj1.zoomScale(scale, p, w, h, extent, max)
        new_extent = self.mapobj1.extent
        assert new_extent.minx == max.minx, new_extent.minx
        assert new_extent.miny == max.miny, new_extent.miny
        assert new_extent.maxx == max.maxx, new_extent.maxx
        assert new_extent.maxy == max.maxy, new_extent.maxy

if __name__ == '__main__':
    unittest.main()

