# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Map "zooming"
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
        """LineObjTestCase.testCreateLine: the created line has the correct number of points"""
        assert self.line.numpoints == 2
    def testGetPointsFromLine(self):
        """LineObjTestCase.testGetPointsFromLine: the points in the line share are the same that were input"""
        for i in range(len(self.points)):
            got_point = self.getPointFromLine(self.line, i)
            self.assertPointsEqual(got_point, self.points[i])
    def testAddPointToLine(self):
        """LineObjTestCase.testAddPointToLine: adding a point to a line behaves correctly"""
        new_point = mapscript.pointObj(4.0, 5.0)
        self.addPointToLine(self.line, new_point)
        assert self.line.numpoints == 3
    def testAlterNumPoints(self):
        """LineObjTestCase.testAlterNumPoints: numpoints is immutable, this should raise error"""
        self.assertRaises(AttributeError, setattr, self.line, 'numpoints', 3)

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
        """ShapePointTestCase.testCreateShape: the number of lines is correct"""
        assert self.shape.numlines == 1
    def testShapeCopy(self):
        """ShapePointTestCase.testShapeCopy: test shape can be copied"""
        s = self.copyShape(self.shape)
        self.assertShapesEqual(self.shape, s)

class PointObjTestCase(MapscriptTestCase):
    def testPointObjConstructorNoArgs(self):
        """PointObjTestCase.testPointObjConstructorNoArgs: point can be created with no arguments"""
        p = mapscript.pointObj()
        self.assertAlmostEqual(p.x, 0.0)
        self.assertAlmostEqual(p.y, 0.0)
    def testPointObjConstructorArgs(self):
        """PointObjTestCase.testPointObjConstructorArgs: point can be created with arguments"""
        p = mapscript.pointObj(1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
    def testSetXY(self):
        """PointObjTestCase.testSetXY: point can have its x and y reset"""
        p = mapscript.pointObj()
        p.setXY(1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
        self.assertAlmostEqual(p.m, 0.0)
    def testSetXYM(self):
        """PointObjTestCase.testSetXYM: point can have its x and y reset (with m value)"""
        p = mapscript.pointObj()
        p.setXY(1.0, 1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
        self.assertAlmostEqual(p.m, 1.0)

class InlineFeatureTestCase(MapTestCase):
    """tests for issue http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=562"""
    def testAddPointFeature(self):
        """InlineFeatureTestCase.testAddPointFeature: adding a point to an inline feature works correctly"""
        inline_layer = self.map.getLayerByName('INLINE')
        p = mapscript.pointObj(0.2, 51.5)
        l = mapscript.lineObj()
        self.addPointToLine(l, p)
        shape = mapscript.shapeObj(inline_layer.type)
        self.addLineToShape(shape, l)
        inline_layer.addFeature(shape)
        msimg = self.map.draw()
        filename = 'testAddPointFeature.png'
        msimg.save(filename)
    def testGetShape(self):
        """InlineFeatureTestCase.testGetShape: returning the shape from an inline feature works"""
        inline_layer = self.map.getLayerByName('INLINE')
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
        """InlineFeatureTestCase.testGetNumFeatures: the number of features in the inline layer is correct"""
        inline_layer = self.map.getLayerByName('INLINE')
        assert inline_layer.getNumFeatures() == 1  

if __name__ == '__main__':
    unittest.main()