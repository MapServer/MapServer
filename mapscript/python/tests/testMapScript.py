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

# Paths
TESTS_PATH = '../../tests'

testMapfile = os.path.join(TESTS_PATH, 'test.map')
testNoFontSetMapfile = os.path.join(TESTS_PATH, 'test_nofontset.map')
test_image = os.path.join(TESTS_PATH, 'test.png')
xmarks_image = os.path.join(TESTS_PATH, 'xmarks.png')

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

# Base class for Primitives Tests -- no actual tests in this class

class MapscriptTestCase(unittest.TestCase):

    def assertAlmostEqual(self, first, second, places=7):
        """Copied from unittest for use with Python 2.1 or 2.2"""
        if round(second-first, places) != 0:
            raise AssertionError, \
                '%s != %s within %s places' % (`first`, `second`, `places`)
        
class MapPrimitivesTestCase(MapscriptTestCase):
    """Base class for testing primitives (points, lines, shapes)
    in stand-alone mode"""

    def addPointToLine(self, line, point):
        """Using either the standard or next_generation_api"""
        try:
            line.add(point)
        except AttributeError: # next_generation_api
            line.addPoint(point)
        except:
            raise

    def getPointFromLine(self, line, index):
        """Using either the standard or next_generation_api"""
        try:
            point = line.get(index)
            return point
        except AttributeError: # next_generation_api
            point = line.getPoint(index)
            return point
        except:
            raise

    def addLineToShape(self, shape, line):
        """Using either the standard or next_generation_api"""
        try:
            shape.add(line)
        except AttributeError: # next_generation_api
            shape.addLine(line)
        except:
            raise

    def getLineFromShape(self, shape, index):
        """Using either the standard or next_generation_api"""
        try:
            line = shape.get(index)
            return line
        except AttributeError: # next_generation_api
            line = shape.getLine(index)
            return line
        except:
            raise

    def assertPointsEqual(self, first, second):
        self.assertAlmostEqual(first.x, second.x)
        self.assertAlmostEqual(first.y, second.y)
     
    def assertLinesEqual(self, first, second):
        assert first.numpoints == second.numpoints
        for i in range(first.numpoints):
            point_first = self.getPointFromLine(first, i)
            point_second = self.getPointFromLine(second, i)
            self.assertPointsEqual(point_first, point_second)

    def assertShapesEqual(self, first, second):
        assert first.numlines == second.numlines
        for i in range(first.numlines):
            line_first = self.getLineFromShape(first, i)
            line_second = self.getLineFromShape(second, i)
            self.assertLinesEqual(line_first, line_second)

    def assertRectsEqual(self, first, second):
        self.assertAlmostEqual(first.minx, second.minx)
        self.assertAlmostEqual(first.miny, second.miny)
        self.assertAlmostEqual(first.maxx, second.maxx)
        self.assertAlmostEqual(first.maxy, second.maxy)

class MapTestCase(MapscriptTestCase):
    """Base class for testing with a map fixture"""
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None

# ---------------------------------------------------------------------------
# The tests begin here

class LayerTestCase(MapTestCase):

    def testLayerConstructor(self):
        layer = mapscript.layerObj(self.mapobj1)
        assert layer.thisown == 1

# Layer ordering tests
class LayerOrderTestCase(MapTestCase):
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
class RemoveLayerTestCase(MapTestCase):
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

class LayerExtentTestCase(MapPrimitivesTestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testPolygonExtent(self):
        layer = self.mapobj1.getLayerByName('POLYGON')
        e = mapscript.rectObj(-0.25, 51.227222, 0.25, 51.727222)
        self.assertRectsEqual(e, layer.getExtent())
  
class LayerRasterProcessingTestCase(MapTestCase):
    def testSetProcessing(self):
        layer = self.mapobj1.getLayer(0)
        layer.setProcessing('directive0=foo')
        assert layer.numprocessing == 1, layer.numprocessing
        layer.setProcessing('directive1=bar')
        assert layer.numprocessing == 2, layer.numprocessing
        directives = [layer.getProcessing(i) \
                      for i in range(layer.numprocessing)]
        assert directives == ['directive0=foo', 'directive1=bar']
    def testClearProcessing(self):
        layer = self.mapobj1.getLayer(0)
        layer.setProcessing('directive0=foo')
        assert layer.numprocessing == 1, layer.numprocessing
        layer.setProcessing('directive1=bar')
        assert layer.numprocessing == 2, layer.numprocessing
        assert layer.clearProcessing() == mapscript.MS_SUCCESS

# class removal tests
class RemoveClassTestCase(MapTestCase):
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

# symbol tests
class SymbolTestCase(MapPrimitivesTestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testConstructor(self):
        symbol = mapscript.symbolObj('test')
        assert symbol.name == 'test'
    def testAddSymbolToMapSymbolSet(self):
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        self.mapobj1.symbolset.appendSymbol(symbola) 
        self.mapobj1.symbolset.appendSymbol(symbolb) 
        num = self.mapobj1.symbolset.numsymbols
        assert num == 4, num
    def testRemoveSymbolFromMapSymbolSet(self):
        self.mapobj1.symbolset.removeSymbol(1)
        num = self.mapobj1.symbolset.numsymbols
        assert num == 1, num
    def testConstructor(self):
        symbol = mapscript.symbolObj('test')
        assert symbol.name == 'test'
    def testConstructorImage(self):
        symbol = mapscript.symbolObj('xmarks', xmarks_image)
        assert symbol.name == 'xmarks'
        assert symbol.type == mapscript.MS_SYMBOL_PIXMAP
    def testGetPoints(self):
        symbol = self.mapobj1.symbolset.getSymbol(1)
        assert symbol.name == 'line'
        line = symbol.getPoints()
        assert line.numpoints == 1, line.numpoints
        pt = self.getPointFromLine(line, 0)
        self.assertPointsEqual(pt, mapscript.pointObj(1.0, 1.0))
    def testSetPoints(self):
        symbol = self.mapobj1.symbolset.getSymbol(1)
        assert symbol.name == 'line'
        line = mapscript.lineObj()
        self.addPointToLine(line, mapscript.pointObj(2.0, 2.0))
        self.addPointToLine(line, mapscript.pointObj(3.0, 3.0))
        assert symbol.setPoints(line) == 2
        line = symbol.getPoints()
        assert line.numpoints == 2, line.numpoints
        pt = self.getPointFromLine(line, 1)
        self.assertPointsEqual(pt, mapscript.pointObj(3.0, 3.0))
    def testSetStyle(self):
        symbol = self.mapobj1.symbolset.getSymbol(1)
        assert symbol.setStyle(0, 1) == mapscript.MS_SUCCESS

# symbolset tests
class SymbolSetTestCase(MapTestCase):
    
    def testGetNumSymbols(self):
        num = self.mapobj1.getNumSymbols()
        assert num == 2, num
    
    def testSymbolSetNumsymbols(self):
        num = self.mapobj1.symbolset.numsymbols
        assert num == 2, num
    
    def testSymbolSetSymbolNames(self):
        set = self.mapobj1.symbolset
        names = [None, 'line', 'tie']
        for i in range(set.numsymbols):
            symbol = set.getSymbol(i)
            assert symbol.name == names[i], symbol.name
    
    def testSymbolIndex(self):
        i = self.mapobj1.symbolset.index('line')
        assert i == 1, i

    def testConstructorNoArgs(self):
        symbolset = mapscript.symbolSetObj()
        num = symbolset.numsymbols
        assert num == 1, num
    def testConstructorFile(self):
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        num = symbolset.numsymbols
        assert num == 2, num
    def testAddSymbolToNewSymbolSet(self):
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        symbolset.appendSymbol(symbola) 
        symbolset.appendSymbol(symbolb) 
        num = symbolset.numsymbols
        assert num == 4, num
        names = [None, 'line', 'testa', 'testb']
        for i in range(symbolset.numsymbols):
            symbol = symbolset.getSymbol(i)
            assert symbol.name == names[i], symbol.name
    def testRemoveSymbolFromNewSymbolSet(self):
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        symbolset.removeSymbol(1)
        num = symbolset.numsymbols
        assert num == 1, num
    def testSaveNewSymbolSet(self):
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        symbolset.appendSymbol(symbola) 
        symbolset.appendSymbol(symbolb)
        assert symbolset.save('new_symbols.txt') == mapscript.MS_SUCCESS
    def testDrawNewSymbol(self):
        symbol = mapscript.symbolObj('xmarks', xmarks_image)
        symbol_index = self.mapobj1.symbolset.appendSymbol(symbol)
        assert symbol_index == 2, symbol_index
        num = self.mapobj1.symbolset.numsymbols
        assert num == 3, num
        inline_layer = self.mapobj1.getLayerByName('INLINE')
        s = inline_layer.getClass(0).getStyle(0)
        s.symbol = symbol_index
        s.size = 24
        msimg = self.mapobj1.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testDrawNewSymbol.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()

# fontset tests
class FontSetTestCase(MapTestCase):
    def testGetFontSetFile(self):
        file = self.mapobj1.fontset.filename
        assert file == 'fonts.txt', file
    def testFontSetFonts(self):
        """testFontSetFonts may fail if fontset is improperly configured"""
        fonts = []
        font = None
        while 1:
            font = self.mapobj1.fontset.getNextFont(font)
            if not font:
                break
            fonts.append(font)
        assert fonts == ['LucidaSansRegular', 'LucidaSansDemiBold'], fonts

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

class ZoomPointTestCase(MapPrimitivesTestCase):
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
        p = mapscript.pointObj(50.0, 50.0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(1, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-50,-50,50,50))
    def testZoomInPoint(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj(50.0, 50.0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-25,-25,25,25))
    def testZoomOutPoint(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-100,-100,100,100))
    def testZoomOutPointConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-4, p, w, h, extent, max)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, max)
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

class ZoomRectangleTestCase(MapPrimitivesTestCase):
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
        self.assertRectsEqual(new_extent, mapscript.rectObj(-49,24,-24,49))
    def testZoomRectangleConstrained(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0,-100.0,100.0,100.0)
        r = mapscript.rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (0, 200, 200, 0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomRectangle(r, w, h, extent, max)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, max)
    def testZoomRectangleBadly(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        r = mapscript.rectObj()
        r.minx, r.miny, r.maxx, r.maxy = (0, 0, 200, 200)
        extent = self.mapobj1.extent
        self.assertRaises(mapscript.MapServerError, 
            self.mapobj1.zoomRectangle, r, w, h, extent, None)

class ZoomScaleTestCase(MapPrimitivesTestCase):
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
        self.assertRectsEqual(new_extent, mapscript.rectObj(-50,-50,50,50))
    def testZoomInScale(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 1417.3236
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-25,-25,25,25))
    def testZoomOutScale(self):
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 5669.2944
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-100,-100,100,100))
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
        self.assertRectsEqual(new_extent, max)

# Tests of getScale

class SetExtentTestCase(MapPrimitivesTestCase):
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testSetExtent(self):
        e = self.mapobj1.extent
        result = self.mapobj1.setExtent(e.minx, e.miny, e.maxx, e.maxy)
        self.assertAlmostEqual(self.mapobj1.scale, 14.173236)
        assert result == mapscript.MS_SUCCESS
    def testSetExtentBadly(self):
        self.assertRaises(mapscript.MapServerError, self.mapobj1.setExtent,
                          1.0, -2.0, -3.0, 4.0)

# mapscript.rectObj tests

class RectObjTestCase(MapPrimitivesTestCase):
    def testRectObjConstructorNoArgs(self):
        r = mapscript.rectObj()
        self.assertAlmostEqual(r.minx, 0.0)
        self.assertAlmostEqual(r.miny, 0.0)
        self.assertAlmostEqual(r.maxx, 0.0)
        self.assertAlmostEqual(r.maxy, 0.0)
    def testRectObjConstructorArgs(self):
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        self.assertAlmostEqual(r.minx, -1.0)
        self.assertAlmostEqual(r.miny, -2.0)
        self.assertAlmostEqual(r.maxx, 3.0)
        self.assertAlmostEqual(r.maxy, 4.0)
    def testRectObjConstructorBadly1(self):
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj, 1.0, -2.0, -3.0, 4.0)
    def testRectObjConstructorBadly2(self):
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj, -1.0, 2.0, 3.0, -2.0)
    def testRectObjToPolygon(self):
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        s = r.toPolygon()
        assert s.numlines == 1, s.numlines
        line = self.getLineFromShape(s, 0)
        assert line.numpoints == 5, line.numpoints
        point = self.getPointFromLine(line, 0)
        self.assertAlmostEqual(point.x, -1.0)
        self.assertAlmostEqual(point.y, -2.0)

# lineObj tests

class LineObjTestCase(MapPrimitivesTestCase):
    """Testing the lineObj class in stand-alone mode"""

    def setUp(self):
        """The test fixture is a line with two points"""
        self.points = (mapscript.pointObj(0.0, 1.0),
                       mapscript.pointObj(2.0, 3.0))
        self.line = mapscript.lineObj()
        self.addPointToLine(self.line, self.points[0])
        self.addPointToLine(self.line, self.points[1])

    def testCreateLine(self):
        assert self.line.numpoints == 2

    def testGetPointsFromLine(self):
        for i in range(len(self.points)):
            got_point = self.getPointFromLine(self.line, i)
            self.assertPointsEqual(got_point, self.points[i])
            
    def testAddPointToLine(self):
        new_point = mapscript.pointObj(4.0, 5.0)
        self.addPointToLine(self.line, new_point)
        assert self.line.numpoints == 3

    def testAlterNumPoints(self):
        """numpoints is immutable, this should raise error"""
        self.assertRaises(AttributeError, setattr, self.line, 'numpoints', 3)

# shapeObj tests

class ShapeObjTestCase(MapPrimitivesTestCase):
    """Base class for shapeObj tests"""
    
    def copyShape(self, shape):
        try:
            return shape.copy()
        except TypeError:
            s = mapscript.shapeObj(shape.type)
            shape.copy(s)
            return s
        except:
            raise

class ShapePointTestCase(ShapeObjTestCase):
    """Test point type shapeObj in stand-alone mode"""

    def setUp(self):
        """The test fixture is a shape of one point"""
        self.points = (mapscript.pointObj(0.0, 1.0),)
        self.lines = (mapscript.lineObj(),)
        self.addPointToLine(self.lines[0], self.points[0])
        self.shape = mapscript.shapeObj(mapscript.MS_SHAPE_POINT)
        self.addLineToShape(self.shape, self.lines[0])

    def testCreateShape(self):
        assert self.shape.numlines == 1
        
    def testShapeCopy(self):
        s = self.copyShape(self.shape)
        self.assertShapesEqual(self.shape, s)

# pointObj constructor tests

class PointObjTestCase(MapscriptTestCase):
    def testPointObjConstructorNoArgs(self):
        p = mapscript.pointObj()
        self.assertAlmostEqual(p.x, 0.0)
        self.assertAlmostEqual(p.y, 0.0)
    def testPointObjConstructorArgs(self):
        p = mapscript.pointObj(1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
    def testSetXY(self):
        p = mapscript.pointObj()
        p.setXY(1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
        self.assertAlmostEqual(p.m, 0.0)
    def testSetXYM(self):
        p = mapscript.pointObj()
        p.setXY(1.0, 1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
        self.assertAlmostEqual(p.m, 1.0)

# colorObj constructor tests

class ColorObjTestCase(unittest.TestCase):
    def testColorObjConstructorNoArgs(self):
        c = mapscript.colorObj()
        assert (c.red, c.green, c.blue, c.pen) == (0, 0, 0, -4)
    def testColorObjConstructorArgs(self):
        c = mapscript.colorObj(1, 2, 3)
        assert (c.red, c.green, c.blue, c.pen) == (1, 2, 3, -4)
    def testColorObjToHex(self):
        c = mapscript.colorObj(255, 255, 255)
        assert c.toHex() == '#ffffff'
    def testColorObjSetRGB(self):
        c = mapscript.colorObj()
        c.setRGB(255, 255, 255)
        assert (c.red, c.green, c.blue, c.pen) == (255, 255, 255, -4)
    def testColorObjSetHexLower(self):
        c = mapscript.colorObj()
        c.setHex('#ffffff')
        assert (c.red, c.green, c.blue, c.pen) == (255, 255, 255, -4)
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

    def testAppendNewStyleOldWay(self):
        p_layer = self.mapobj1.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        assert class0.numstyles == 2, class0.numstyles
        new_style = mapscript.styleObj(class0)
        assert new_style.thisown == 1, new_style.thisown
        new_style.color.setRGB(0, 0, 0)
        new_style.symbol = 1
        new_style.size = 3
        msimg = self.mapobj1.draw()
        data = msimg.saveToString()
        filename = 'testAppendNewStyleOldWay.png'
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

class InlineFeatureTestCase(MapPrimitivesTestCase):
    """tests for issue http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=562"""
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testAddPointFeature(self):
        inline_layer = self.mapobj1.getLayerByName('INLINE')
        p = mapscript.pointObj(0.2, 51.5)
        l = mapscript.lineObj()
        self.addPointToLine(l, p)
        shape = mapscript.shapeObj(inline_layer.type)
        self.addLineToShape(shape, l)
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
        l = self.getLineFromShape(s, 0)
        p = self.getPointFromLine(l, 0)
        self.assertAlmostEqual(p.x, -0.2)
        self.assertAlmostEqual(p.y, 51.5)
    def testGetNumFeatures(self):
        inline_layer = self.mapobj1.getLayerByName('INLINE')
        assert inline_layer.getNumFeatures() == 1

class NewOutputFormatTestCase(unittest.TestCase):
    """http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=511"""
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testOutputFormatConstructor(self):
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiff')
        assert new_format.refcount == 1, new_format.refcount
        assert new_format.name == 'gtiff'
        assert new_format.mimetype == 'image/tiff'
    def testAppendNewOutputFormat(self):
        num = self.mapobj1.numoutputformats
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiffx')
        assert new_format.refcount == 1, new_format.refcount
        self.mapobj1.appendOutputFormat(new_format)
        assert self.mapobj1.numoutputformats == num + 1
        assert new_format.refcount == 2, new_format.refcount
        self.mapobj1.setImageType('gtiffx')
        self.mapobj1.save('testAppendNewOutputFormat.map')
        imgobj = self.mapobj1.draw()
        filename = 'testAppendNewOutputFormat.tif'
        imgobj.save(filename)
    def testRemoveOutputFormat(self):
        """testRemoveOutputFormat may fail depending on GD options"""
        num = self.mapobj1.numoutputformats
        assert num == 6, num
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiffx')
        self.mapobj1.appendOutputFormat(new_format)
        assert self.mapobj1.numoutputformats == num + 1
        assert new_format.refcount == 2, new_format.refcount
        assert self.mapobj1.removeOutputFormat('gtiffx') == mapscript.MS_SUCCESS
        assert new_format.refcount == 1, new_format.refcount
        assert self.mapobj1.numoutputformats == num
        self.assertRaises(mapscript.MapServerError,
                          self.mapobj1.setImageType, 'gtiffx')
        self.mapobj1.setImageType('GTiff')
        assert self.mapobj1.outputformat.mimetype == 'image/tiff'

class MapMetaDataTestCase(MapTestCase):
    def testInvalidKeyAccess(self):
        self.assertRaises(mapscript.MapServerError, self.mapobj1.getMetaData, 'foo')
    def testFirstKeyAccess(self):
        key = self.mapobj1.getFirstMetaDataKey()
        assert key == 'key1'
        val = self.mapobj1.getMetaData(key)
        assert val == 'value1'
    def testLastKeyAccess(self):
        key = self.mapobj1.getFirstMetaDataKey()
        for i in range(3):
            key = self.mapobj1.getNextMetaDataKey(key)
            assert key is not None
        key = self.mapobj1.getNextMetaDataKey(key)
        assert key is None
    def testMapMetaData(self):
        keys = []
        key = self.mapobj1.getFirstMetaDataKey()
        if key is not None: keys.append(key)
        while 1:
            key = self.mapobj1.getNextMetaDataKey(key)
            if not key:
                break
            keys.append(key)
        assert keys == ['key1', 'key2', 'key3', 'key4'], keys
    def testLayerMetaData(self):
        keys = []
        key = self.mapobj1.getLayer(0).getFirstMetaDataKey()
        if key is not None: keys.append(key)
        while 1:
            key = self.mapobj1.getLayer(0).getNextMetaDataKey(key)
            if not key:
                break
            keys.append(key)
        assert keys == ['key1', 'key2', 'key3', 'key4'], keys
    def testClassMetaData(self):
        keys = []
        key = self.mapobj1.getLayer(0).getClass(0).getFirstMetaDataKey()
        if key is not None: keys.append(key)
        while 1:
            key = self.mapobj1.getLayer(0).getClass(0).getNextMetaDataKey(key)
            if not key:
                break
            keys.append(key)
        assert keys == ['key1', 'key2', 'key3', 'key4'], keys
    def testAccessNullLayerMetaData(self):
        layer = self.mapobj1.getLayer(1)
        self.assertRaises(mapscript.MapServerError, layer.getFirstMetaDataKey)
    def testAccessNullClassMetaData(self):
        layer = self.mapobj1.getLayer(1).getClass(1)
        self.assertRaises(mapscript.MapServerError, layer.getFirstMetaDataKey)

class DrawProgrammedStylesTestCase(MapTestCase):
    
    def testDrawPoints(self):
        points = [mapscript.pointObj(-0.2, 51.6),
                  mapscript.pointObj(0.0, 51.2),
                  mapscript.pointObj(0.2, 51.6)]
        colors = [mapscript.colorObj(255,0,0),
                  mapscript.colorObj(0,255,0),
                  mapscript.colorObj(0,0,255)]
        img = self.mapobj1.prepareImage()
        layer = self.mapobj1.getLayerByName('POINT')
        #layer.draw(self.mapobj1, img)
        class0 = layer.getClass(0)
        for i in range(len(points)):
            style0 = class0.getStyle(0)
            style0.color = colors[i]
            #style0.color.pen = -4
            assert style0.color.toHex() == colors[i].toHex()
            points[i].draw(self.mapobj1, layer, img, 0, "foo")
        img.save('test_draw_points.png')

if __name__ == '__main__':
    unittest.main()

