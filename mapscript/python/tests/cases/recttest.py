# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of rectObj
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
#     python tests/cases/recttest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from testing import mapscript
from testing import MapPrimitivesTestCase

# ===========================================================================
# Test begins now

class RectObjTestCase(MapPrimitivesTestCase):
    def testRectObjConstructorNoArgs(self):
        """RectObjTestCase.testRectObjConstructorNoArgs: a rect can be initialized with no args"""
        r = mapscript.rectObj()
        self.assertAlmostEqual(r.minx, -1.0)
        self.assertAlmostEqual(r.miny, -1.0)
        self.assertAlmostEqual(r.maxx, -1.0)
        self.assertAlmostEqual(r.maxy, -1.0)
    def testRectObjConstructorArgs(self):
        """RectObjTestCase.testRectObjConstructorArgs: a rect can be initialized with arguments"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        self.assertAlmostEqual(r.minx, -1.0)
        self.assertAlmostEqual(r.miny, -2.0)
        self.assertAlmostEqual(r.maxx, 3.0)
        self.assertAlmostEqual(r.maxy, 4.0)
    def testRectObjConstructorBadly1(self):
        """RectObjTestCase.testRectObjConstructorBadly1: an invalid extent raises proper error (x direction)"""
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj, 1.0, -2.0, -3.0, 4.0)
    def testRectObjConstructorBadly2(self):
        """RectObjTestCase.testRectObjConstructorBadly2: an invalid extent raises proper error (y direction)"""
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj, -1.0, 2.0, 3.0, -2.0)
    def testRectObjToPolygon(self):
        """RectObjTestCase.testRectObjToPolygon: a rect can be converted into a MS_POLYGON shape"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        s = r.toPolygon()
        assert s.numlines == 1, s.numlines
        line = self.getLineFromShape(s, 0)
        assert line.numpoints == 5, line.numpoints
        point = self.getPointFromLine(line, 0)
        self.assertAlmostEqual(point.x, -1.0)
        self.assertAlmostEqual(point.y, -2.0)


# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
