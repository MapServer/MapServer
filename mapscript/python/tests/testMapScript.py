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

# Layer ordering tests
class LayerOrderTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testGetLayerOrder(self):
        order = self.mapobj1.getLayerOrder()
        assert order == tuple(range(4)), order
    def testPromoteLayer1(self):
        self.mapobj1.getLayer(1).promote()
        order = self.mapobj1.getLayerOrder()
        assert order == (0, 2, 1, 3), order
    def testDemoteLayer1(self):
        self.mapobj1.getLayer(1).demote()
        order = self.mapobj1.getLayerOrder()
        assert order == (1, 0, 2, 3), order
    def testSetLayerOrder(self):
        ord = (1, 0, 2, 3)
        self.mapobj1.setLayerOrder(ord)
        order = self.mapobj1.getLayerOrder()
        assert order == ord, order

# Layer removal tests
class RemoveLayerTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testRemoveLayer1NumLayers(self):
        self.mapobj1.removeLayer(0)
        assert self.mapobj1.numlayers == 3
    def testRemoveLayer1LayerName(self):
        l2name = self.mapobj1.getLayer(1).name
        self.mapobj1.removeLayer(0)
        assert self.mapobj1.getLayer(0).name == l2name
    def testRemoveLayer2NumLayers(self):
        self.mapobj1.removeLayer(1)
        assert self.mapobj1.numlayers == 3
    def testRemoveLayer2LayerName(self):
        l1name = self.mapobj1.getLayer(0).name
        self.mapobj1.removeLayer(1)
        assert self.mapobj1.getLayer(0).name == l1name

def rectObjToTuple(rect):
    return (rect.minx, rect.miny, rect.maxx, rect.maxy)

class LayerExtentTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testPolygonExtent(self):
        layer = self.mapobj1.getLayerByName('POLYGON')
        e = layer.getExtent()
        assert int(100*e.minx) == -25
        assert int(100*e.miny) == 5122
        assert int(100*e.maxx) == 25
        assert int(100*e.maxy) == 5172
        
# class removal tests
class RemoveClassTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
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
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testGetNumSymbols(self):
        num = self.mapobj1.getNumSymbols()
        assert num == 2, num
    def testSymbolsetNumsymbols(self):
        num = self.mapobj1.symbolset.numsymbols
        assert num == 2, num
    def testSymbolsetSymbolNames(self):
        set = self.mapobj1.symbolset
        names = [None, 'line', 'tie']
        for i in range(set.numsymbols):
            symbol = set.getSymbol(i)
            assert symbol.name == names[i], symbol.name

# fontset tests
class FontSetTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testGetFontSetFile(self):
        file = self.mapobj1.fontset.filename
        assert file == 'fonts.txt', file

class EmptyMapExceptionTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj('')
    def tearDown(self):
        self.mapobj1 = None
    def testDrawEmptyMap(self):
        self.assertRaises(mapscript.MapServerError, self.mapobj1.draw)

class TestMapExceptionTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testDrawBadData(self):
        self.mapobj1.getLayerByName('POLYGON').data = 'foo'
        self.assertRaises(mapscript.MapServerError, self.mapobj1.draw)
    def testZeroResultsQuery(self):
        p = mapscript.pointObj()
        p.x, p.y = (-600000,1000000) # Way outside demo
        self.assertRaises(mapscript.MapServerNotFoundError, \
                         self.mapobj1.queryByPoint, p, mapscript.MS_SINGLE, 1.0)

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
        self.mapobj1 = mapscript.mapObj(testMapfile)
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
            assert pyimg.size == (200, 200)
            assert pyimg.mode == 'P'
        else:
            assert 1

class NoFontSetTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj('')
    def tearDown(self):
        self.mapobj1 = None
    def testNoGetFontSetFile(self):
        assert self.mapobj1.fontset.filename == None

class ExpressionTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
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
        self.mapobj1 = mapscript.mapObj(testMapfile)
        # Change the extent for purposes of zoom testing
        rect = mapscript.rectObj()
        rect.minx, rect.miny, rect.maxx, rect.maxy = (-50.0, -50.0, 50.0, 50.0)
        self.mapobj1.extent = rect
        # Change height/width as well
        self.mapobj1.width, self.mapobj1.height = (100, 100)
    def tearDown(self):
        self.mapobj1 = None
    def testRecenter(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(1, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == -50, new_extent.minx
        assert int(new_extent.miny) == -50, new_extent.miny
        assert int(new_extent.maxx) == 50, new_extent.maxx
        assert int(new_extent.maxy) == 50, new_extent.maxy
    def testZoomInPoint(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == -25, new_extent.minx
        assert int(new_extent.miny) == -25, new_extent.miny
        assert int(new_extent.maxx) == 25, new_extent.maxx
        assert int(new_extent.maxy) == 25, new_extent.maxy
    def testZoomOutPoint(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == -100, new_extent.minx
        assert int(new_extent.miny) == -100, new_extent.miny
        assert int(new_extent.maxx) == 100, new_extent.maxx
        assert int(new_extent.maxy) == 100, new_extent.maxy
    def testZoomOutPointConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-4, p, w, h, extent, max)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == int(max.minx), new_extent.minx
        assert int(new_extent.miny) == int(max.miny), new_extent.miny
        assert int(new_extent.maxx) == int(max.maxx), new_extent.maxx
        assert int(new_extent.maxy) == int(max.maxy), new_extent.maxy
    def testZoomBadSize(self):
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        w = 0
        h = -1
        self.assertRaises(mapscript.MapServerError, 
            self.mapobj1.zoomPoint, -2, p, w, h, extent, None);
    def testZoomBadExtent(self):
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        extent.maxx = extent.maxx - 1000000
        w = 100
        h = 100
        self.assertRaises(mapscript.MapServerError, 
            self.mapobj1.zoomPoint, -2, p, w, h, extent, None);

class ZoomRectangleTestCase(unittest.TestCase):
    "testing new zoom* methods that we are adapting from the PHP MapScript"
    
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
        # Change the extent for purposes of zoom testing
        rect = mapscript.rectObj()
        rect.minx, rect.miny, rect.maxx, rect.maxy = (-50.0, -50.0, 50.0, 50.0)
        self.mapobj1.extent = rect
        # Change height/width as well
        self.mapobj1.width, self.mapobj1.height = (100, 100)
    def tearDown(self):
        self.mapobj1 = None
    def testZoomRectangle(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        r = mapscript.rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (1, 26, 26, 1)
        extent = self.mapobj1.extent
        self.mapobj1.zoomRectangle(r, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == -49, new_extent.minx
        assert int(new_extent.miny) == 24, new_extent.miny
        assert int(new_extent.maxx) == -24, new_extent.maxx
        assert int(new_extent.maxy) == 49, new_extent.maxy
    def testZoomRectangleConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        r = mapscript.rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (0, 200, 200, 0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomRectangle(r, w, h, extent, max)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == int(max.minx), new_extent.minx
        assert int(new_extent.miny) == int(max.miny), new_extent.miny
        assert int(new_extent.maxx) == int(max.maxx), new_extent.maxx
        assert int(new_extent.maxy) == int(max.maxy), new_extent.maxy
    def testZoomRectangleBadly(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        r = mapscript.rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (0, 0, 200, 200)
        extent = self.mapobj1.extent
        self.assertRaises(mapscript.MapServerError, 
            self.mapobj1.zoomRectangle, r, w, h, extent, None)

class ZoomScaleTestCase(unittest.TestCase):
    "testing new zoom* methods that we are adapting from the PHP MapScript"
   
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
        # Change the extent for purposes of zoom testing
        rect = mapscript.rectObj()
        rect.minx, rect.miny, rect.maxx, rect.maxy = (-50.0, -50.0, 50.0, 50.0)
        self.mapobj1.extent = rect
        # Change height/width as well
        self.mapobj1.width, self.mapobj1.height = (100, 100)
    def tearDown(self):
        self.mapobj1 = None
    def testRecenter(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 2834.6472
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == -50, new_extent.minx
        assert int(new_extent.miny) == -50, new_extent.miny
        assert int(new_extent.maxx) == 50, new_extent.maxx
        assert int(new_extent.maxy) == 50, new_extent.maxy
    def testZoomInScale(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 1417.3236
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == -25, new_extent.minx
        assert int(new_extent.miny) == -25, new_extent.miny
        assert int(new_extent.maxx) == 25, new_extent.maxx
        assert int(new_extent.maxy) == 25, new_extent.maxy
    def testZoomOutScale(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 5669.2944
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == -100, new_extent.minx
        assert int(new_extent.miny) == -100, new_extent.miny
        assert int(new_extent.maxx) == 100, new_extent.maxx
        assert int(new_extent.maxy) == 100, new_extent.maxy
    def testZoomOutPointConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        scale = 10000
        self.mapobj1.zoomScale(scale, p, w, h, extent, max)
        new_extent = self.mapobj1.extent
        assert int(new_extent.minx) == int(max.minx), new_extent.minx
        assert int(new_extent.miny) == int(max.miny), new_extent.miny
        assert int(new_extent.maxx) == int(max.maxx), new_extent.maxx
        assert int(new_extent.maxy) == int(max.maxy), new_extent.maxy

# Tests of getScale

class SetExtentTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testSetExtent(self):
        e = self.mapobj1.extent
        result = self.mapobj1.setExtent(e.minx, e.miny, e.maxx, e.maxy)
        assert int(1000*self.mapobj1.scale) == 14173, self.mapobj1.scale
        assert result == mapscript.MS_SUCCESS
    def testSetExtentBadly(self):
        self.assertRaises(mapscript.MapServerError, self.mapobj1.setExtent,
                          1.0, -2.0, -3.0, 4.0)

# mapscript.rectObj tests

class RectObjTestCase(unittest.TestCase):
    def testRectObjConstructorNoArgs(self):
        r = mapscript.rectObj()
        assert int(r.minx) == 0
        assert int(r.miny) == 0
        assert int(r.maxx) == 0
        assert int(r.maxy) == 0
    def testRectObjConstructorArgs(self):
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        assert int(r.minx) == -1
        assert int(r.miny) == -2
        assert int(r.maxx) == 3
        assert int(r.maxy) == 4
    def testRectObjConstructorBadly1(self):
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj, 1.0, -2.0, -3.0, 4.0)
    def testRectObjConstructorBadly2(self):
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj, -1.0, 2.0, 3.0, -2.0)
    def testRectObjToPolygon(self):
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        s = r.toPolygon()
        assert s.numlines == 1, s.numlines
        assert s.get(0).numpoints == 5
        assert int(s.get(0).get(0).x) == -1
        assert int(s.get(0).get(0).y) == -2

# pointObj constructor tests

class PointObjTestCase(unittest.TestCase):
    def testPointObjConstructorNoArgs(self):
        p = mapscript.pointObj()
        assert p.x == 0.0
        assert p.y == 0.0
    def testPointObjConstructorArgs(self):
        p = mapscript.pointObj(1.0, 1.0)
        assert int(p.x) == 1
        assert int(p.y) == 1

# colorObj constructor tests

class ColorObjTestCase(unittest.TestCase):
    def testColorObjConstructorNoArgs(self):
        c = mapscript.colorObj()
        assert (c.red, c.green, c.blue) == (0, 0, 0)
    def testColorObjConstructorArgs(self):
        c = mapscript.colorObj(1, 2, 3)
        assert (c.red, c.green, c.blue) == (1, 2, 3)
    def testColorObjToHex(self):
        c = mapscript.colorObj(255, 255, 255)
        assert c.toHex() == '#ffffff'
    def testColorObjSetRGB(self):
        c = mapscript.colorObj()
        c.setRGB(255, 255, 255)
        assert (c.red, c.green, c.blue) == (255, 255, 255)
    def testColorObjSetHexLower(self):
        c = mapscript.colorObj()
        c.setHex('#ffffff')
        assert (c.red, c.green, c.blue) == (255, 255, 255)
    def testColorObjSetHexUpper(self):
        c = mapscript.colorObj()
        c.setHex('#FFFFFF')
        assert (c.red, c.green, c.blue) == (255, 255, 255)
    def testColorObjSetHexBadly(self):
        c = mapscript.colorObj()
        self.assertRaises(mapscript.MapServerError, c.setHex, '#fffffg')

import cStringIO
import urllib

class ImageObjTestCase(unittest.TestCase):
    def testConstructor(self):
        imgobj = mapscript.imageObj(10, 10)
        assert imgobj.thisown == 1
        assert imgobj.height == 10
        assert imgobj.width == 10
    def testConstructorWithDriver(self):
        driver = 'GD/PNG'
        imgobj = mapscript.imageObj(10, 10, driver)
        assert imgobj.thisown == 1
        assert imgobj.format.driver == driver
        assert imgobj.height == 10
        assert imgobj.width == 10
    def testConstructorFilename(self):
        imgobj = mapscript.imageObj(0, 0, None, test_image)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
    def testConstructorFilenameDriver(self):
        imgobj = mapscript.imageObj(0, 0, 'GD/PNG', test_image)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
    def testConstructorStream(self):
        f = open(test_image, 'rb')
        imgobj = mapscript.imageObj(0, 0, 'GD/PNG', f)
        f.close()
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
    def testConstructorStringIO(self):
        f = open(test_image, 'rb')
        data = f.read()
        f.close()
        s = cStringIO.StringIO(data)
        imgobj = mapscript.imageObj(0, 0, 'GD/PNG', s)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
    def testConstructorUrlStream(self):
        url = urllib.urlopen('http://mapserver.gis.umn.edu/bugs/ant.jpg')
        imgobj = mapscript.imageObj(0, 0, 'GD/JPEG', url)
        assert imgobj.thisown == 1
        assert imgobj.height == 220
        assert imgobj.width == 329

class NewStylesTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testStyleConstructor(self):
        new_style = mapscript.styleObj()
        assert new_style.color.red == -1
        assert new_style.color.green == -1
        assert new_style.color.blue == -1
    def testStyleColorSettable(self):
        new_style = mapscript.styleObj()
        new_style.color.setRGB(1,2,3)
        assert new_style.color.red == 1
        assert new_style.color.green == 2
        assert new_style.color.blue == 3
    def testAppendNewStyle(self):
        p_layer = self.mapobj1.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        assert class0.numstyles == 2, class0.numstyles
        new_style = mapscript.styleObj()
        new_style.color.setRGB(0, 0, 0)
        new_style.symbol = 1
        new_style.size = 3
        index = class0.insertStyle(new_style)
        assert index == 2, index
        assert class0.numstyles == 3, class0.numstyles
        msimg = self.mapobj1.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testAppendNewStyle.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()
    def testInsertNewStyleAtIndex0(self):
        l_layer = self.mapobj1.getLayerByName('LINE')
        class0 = l_layer.getClass(0)
        new_style = mapscript.styleObj()
        new_style.color.setRGB(255, 255, 0)
        new_style.symbol = 1
        new_style.size = 7
        index = class0.insertStyle(new_style, 0)
        assert index == 0, index
        assert class0.numstyles == 2, class0.numstyles
        msimg = self.mapobj1.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testInsertNewStyleAtIndex0.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()
    def testRemovePointStyle(self):
        p_layer = self.mapobj1.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        rem_style = class0.removeStyle(1)
        assert class0.numstyles == 1, class0.numstyles
        msimg = self.mapobj1.draw()
        filename = 'testRemovePointStyle.png'
        msimg.save(filename)
    def testModifyMultipleStyle(self):
        p_layer = self.mapobj1.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        style1 = class0.getStyle(1)
        style1.color.setRGB(255, 255, 0)
        msimg = self.mapobj1.draw()
        filename = 'testModifyMutiplePointStyle.png'
        msimg.save(filename)
    def testInsertTooManyStyles(self):
        p_layer = self.mapobj1.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        new_style = mapscript.styleObj()
        # Add three styles successfully
        index = class0.insertStyle(new_style)
        index = class0.insertStyle(new_style)
        index = class0.insertStyle(new_style)
        # We've reached the maximum, next attempt should raise exception
        self.assertRaises(mapscript.MapServerChildError, class0.insertStyle, new_style)
    def testInsertStylePastEnd(self):
        p_layer = self.mapobj1.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        new_style = mapscript.styleObj()
        self.assertRaises(mapscript.MapServerChildError, class0.insertStyle, new_style, 6)

class InlineFeatureTestCase(unittest.TestCase):
    """tests for issue http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=562"""
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testAddPointFeature(self):
        inline_layer = self.mapobj1.getLayerByName('INLINE')
        p = mapscript.pointObj(0.2, 51.5)
        l = mapscript.lineObj()
        l.add(p)
        shape = mapscript.shapeObj(inline_layer.type)
        shape.add(l)
        inline_layer.addFeature(shape)
        msimg = self.mapobj1.draw()
        filename = 'testAddPointFeature.png'
        msimg.save(filename)
    def testGetShape(self):
        inline_layer = self.mapobj1.getLayerByName('INLINE')
        s = mapscript.shapeObj(inline_layer.type)
        inline_layer.open()
        try:
            inline_layer.getShape(s, 0, 0)
        except TypeError: # next generation API
            s = inline_layer.getShape(0)
        l = s.get(0)
        p = l.get(0)
        assert int(p.x * 10) == -2
        assert int(p.y * 10) == 515
    def testGetNumFeatures(self):
        inline_layer = self.mapobj1.getLayerByName('INLINE')
        assert inline_layer.getNumFeatures() == 1

if __name__ == '__main__':
    unittest.main()

