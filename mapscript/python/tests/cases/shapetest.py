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
        """ShapePointTestCase.testCreateShape: the number of lines is correct"""
        assert self.shape.numlines == 1
    def testShapeCopy(self):
        """ShapePointTestCase.testShapeCopy: test shape can be copied"""
        s = self.copyShape(self.shape)
        self.assertShapesEqual(self.shape, s)

class InlineFeatureTestCase(MapTestCase):
    """tests for issue http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=562"""
    def testAddPointFeature(self):
        """InlineFeatureTestCase.testAddPointFeature: adding a point to an inline feature works correctly"""
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
