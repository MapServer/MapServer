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
        assert order == (1, 0) + tuple(range(2,10)), order
    def testDemoteLayer0(self):
        self.mapobj1.getLayer(0).demote()
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
        file = self.mapobj1.getFontSetFile()
        assert file == 'fonts.txt', file
    def testGetFonts(self):
        fonts = [('LucidaSansRegular',
                  '/Users/sean/Library/Fonts/LucidaSansRegular.ttf'), 
                 ('LucidaSansDemiBold',
                  '/Users/sean/Library/Fonts/LucidaSansDemiBold.ttf')]
        f = self.mapobj1.getFonts()
        assert f == fonts, f

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

# If PIL is available, use it to test the saveToString() method

#have_image = 0
#try:
#    import Image
#    have_image = 1
#except ImportError:
#    pass
#    
#from StringIO import StringIO
#
#class SaveToStringTestCase(unittest.TestCase):
#    def setUp(self):
#        self.mapobj1 = mapObj(testMapfile)
#    def tearDown(self):
#        self.mapobj1 = None
#    def testSaveToString(self):
#        msimg = self.mapobj1.draw()
#        data = StringIO(msimg.saveToString())
#        if have_image:
#            pyimg = Image.open(data)
#            assert pyimg.format == 'PNG'
#            assert pyimg.size == (600, 600)
#            assert pyimg.mode == 'P'
#        else:
#            assert 1

class NoFontSetTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj('')
    def tearDown(self):
        self.mapobj1 = None
    def testNoGetFontSetFile(self):
        assert self.mapobj1.getFontSetFile() == None

class ExpressionTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
    def tearDown(self):
        self.mapobj1 = None
    def testClearExpression(self):
        self.mapobj1.getLayer(0).setFilter('')
        fs = self.mapobj1.getLayer(0).getFilterString()
        assert fs == '"(null)"', fs
    def testSetExpression(self):
        self.mapobj1.getLayer(0).setFilter('"foo"')
        fs = self.mapobj1.getLayer(0).getFilterString()
        assert fs == '"foo"', fs
        
if __name__ == '__main__':
    unittest.main()

