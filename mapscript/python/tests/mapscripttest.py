import os, sys
import distutils.util
import unittest

# ===========================================================================
# Define local paths for testing

TESTS_PATH = '../../tests'

testMapfile = os.path.join(TESTS_PATH, 'test.map')
testNoFontSetMapfile = os.path.join(TESTS_PATH, 'test_nofontset.map')
test_image = os.path.join(TESTS_PATH, 'test.png')
xmarks_image = os.path.join(TESTS_PATH, 'xmarks.png')


# Add the distutils build path to our system path, allowing us to
# run unit tests before the module is installed
platformdir = '-'.join((distutils.util.get_platform(), 
                       '.'.join(map(str, sys.version_info[0:2]))))
sys.path.insert(0, os.path.join('build', 'lib.' + platformdir))

# import module
import mapscript

# normalize names
def normalize_class_names(module):
    if 'mapObj' not in dir(module):
        module.mapObj = module.Map
        module.layerObj = module.Layer
        module.classObj = module.Class
        module.styleObj = module.Style
        module.shapeObj = module.Shape
        module.lineObj = module.Line
        module.pointObj = module.Point
        module.rectObj = module.Rect
        module.outputFormatObj = module.OutputFormat
        module.symbolObj = module.Symbol
        module.symbolSetObj = module.SymbolSet
        module.colorObj = module.Color
        module.imageObj = module.Image
        module.shapefileObj = module.Shapefile
        module.projectionObj = module.Projection
        module.fontSetObj = module.FontSet

# ===========================================================================
# Import mapscript module and normalize any next generation class names
import mapscript
normalize_class_names(mapscript)

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

class MapTestCase(MapPrimitivesTestCase):
    """Base class for testing with a map fixture"""
    def setUp(self):
        self.mapobj1 = mapscript.mapObj(testMapfile)
        self.xmarks_image = xmarks_image
        self.test_image = test_image
    def tearDown(self):
        self.mapobj1 = None

class MapZoomTestCase(MapPrimitivesTestCase):
    "Testing new zoom* methods that we are adapting from the PHP MapScript"
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

