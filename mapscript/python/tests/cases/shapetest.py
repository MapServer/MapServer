# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Shape
# Author:   Sean Gillies, sgillies@frii.com
#
# ===========================================================================
# Copyright (c) 2004, Sean Gillies
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# ===========================================================================
#
# Execute this module as a script from mapserver/mapscript/python
#
#     python tests/cases/shapetest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from testing import mapscript, MapTestCase
from testing import MapPrimitivesTestCase, ShapeObjTestCase
from testing import MapscriptTestCase

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
        """the number of lines is correct"""
        assert self.shape.numlines == 1
    
    def testShapeClone(self):
        """test shape can be copied"""
        s = self.shape.clone()
        self.assertShapesEqual(self.shape, s)

class InlineFeatureTestCase(MapTestCase):
    """tests for issue http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=562"""
    
    def testAddPointFeature(self):
        """adding a point to an inline feature works correctly"""
        inline_layer = self.map.getLayerByName('INLINE')
        assert inline_layer.connectiontype == mapscript.MS_INLINE
        p = mapscript.pointObj(0.2, 51.5)
        l = mapscript.lineObj()
        self.addPointToLine(l, p)
        shape = mapscript.shapeObj(inline_layer.type)
        shape.classindex = 0
        self.addLineToShape(shape, l)
        inline_layer.addFeature(shape)
        msimg = self.map.draw()
        filename = 'testAddPointFeature.png'
        msimg.save(filename)
    
    def testGetShape(self):
        """returning the shape from an inline feature works"""
        inline_layer = self.map.getLayerByName('INLINE')
        inline_layer.open()
        s = inline_layer.getFeature(0)
        l = self.getLineFromShape(s, 0)
        p = self.getPointFromLine(l, 0)
        self.assertAlmostEqual(p.x, -0.2)
        self.assertAlmostEqual(p.y, 51.5)
    
    def testGetNumFeatures(self):
        """the number of features in the inline layer is correct"""
        inline_layer = self.map.getLayerByName('INLINE')
        assert inline_layer.getNumFeatures() == 1  


class ShapeValuesTestCase(unittest.TestCase):

    def testNullValue(self):
        so = mapscript.shapeObj(mapscript.MS_SHAPE_POINT)
        assert so.numvalues == 0
        assert so.getValue(0) == None
    
    def testSetValue(self):
        so = mapscript.shapeObj(mapscript.MS_SHAPE_POINT)
        so.initValues(4)
        so.setValue(0, 'Foo');
        assert so.numvalues == 4
        assert so.getValue(0) == 'Foo'
        assert so.getValue(1) == ''
        
# New class for testing the WKT stuff of RFC-2       
class ShapeWKTTestCase(unittest.TestCase):
    
    # define a pair of coords, and WKT as class data
    point_xy = (-105.5000000000000000, 40.0000000000000000)
    point_wkt = 'POINT (-105.5000000000000000 40.0000000000000000)'
    
    def testSetPointWKT(self):
        # Create new instance and set/init from WKT
        so = mapscript.shapeObj.fromWKT(self.point_wkt)
        
        # expect one line with one point
        self.assert_(so.numlines == 1, so.numlines)
        self.assert_(so.get(0).numpoints == 1, so.get(0).numpoints)
        
        # expect shape's x and y values to be correct
        po = so.get(0).get(0)
        self.assertAlmostEqual(po.x, self.point_xy[0])
        self.assertAlmostEqual(po.y, self.point_xy[1])

    def testGetPointWKT(self):
        # Create new instance from class data
        po = mapscript.pointObj(self.point_xy[0], self.point_xy[1])
        lo = mapscript.lineObj()
        lo.add(po)
        so = mapscript.shapeObj(mapscript.MS_SHAPE_POINT)
        so.add(lo)

        # test output WKT
        wkt = so.toWKT()
        self.assert_(wkt == self.point_wkt, wkt)
        

# ============================================================================
if __name__ == '__main__':
    unittest.main()
