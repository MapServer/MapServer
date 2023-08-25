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

import unittest
import mapscript
from .testing import MapPrimitivesTestCase


class RectObjTestCase(MapPrimitivesTestCase):

    def testRectObjConstructorNoArgs(self):
        """a rect can be initialized with no args"""
        r = mapscript.rectObj()
        self.assertAlmostEqual(r.minx, -1.0)
        self.assertAlmostEqual(r.miny, -1.0)
        self.assertAlmostEqual(r.maxx, -1.0)
        self.assertAlmostEqual(r.maxy, -1.0)

    def testRectObjConstructorArgs(self):
        """a rect can be initialized with arguments"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        self.assertAlmostEqual(r.minx, -1.0)
        self.assertAlmostEqual(r.miny, -2.0)
        self.assertAlmostEqual(r.maxx, 3.0)
        self.assertAlmostEqual(r.maxy, 4.0)

    def testRectObjConstructorBadXCoords(self):
        """an invalid extent raises proper error (x direction)"""
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj,
                          1.0, -2.0, -3.0, 4.0)

    def testRectObjConstructorBadYCoords(self):
        """an invalid extent raises proper error (y direction)"""
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj,
                          -1.0, 2.0, 3.0, -2.0)

    def testRectObjToPolygon(self):
        """a rect can be converted into a MS_POLYGON shape"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        s = r.toPolygon()
        assert s.numlines == 1, s.numlines
        line = self.getLineFromShape(s, 0)
        assert line.numpoints == 5, line.numpoints
        point = self.getPointFromLine(line, 0)
        self.assertAlmostEqual(point.x, -1.0)
        self.assertAlmostEqual(point.y, -2.0)

    def testExceptionMessage(self):
        """test formatted error message"""
        try:
            r = mapscript.rectObj(1.0, -2.0, -3.0, 4.0)
            print(r)
        except mapscript.MapServerError as msg:
            assert str(msg) == "rectObj(): Invalid rectangle. { 'minx': 1.000000 , " \
                               "'miny': -2.000000 , 'maxx': -3.000000 , 'maxy': 4.000000 }", msg

    def testRect__str__(self):
        """__str__ returns properly formatted string"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0001, 4.0)
        r_str = str(r)
        assert r_str == "{ 'minx': -1 , 'miny': -2 , 'maxx': 3.0001 , 'maxy': 4 }", r_str
        r2 = eval(r_str)
        self.assertAlmostEqual(r2['minx'], r.minx)
        self.assertAlmostEqual(r2['miny'], r.miny)
        self.assertAlmostEqual(r2['maxx'], r.maxx)
        self.assertAlmostEqual(r2['maxy'], r.maxy)

    def testRectToString(self):
        """return properly formatted string"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0001, 4.0)
        r_str = r.toString()
        assert r_str == "{ 'minx': -1 , 'miny': -2 , 'maxx': 3.0001 , 'maxy': 4 }", r_str

    def testRectContainsPoint(self):
        """point is contained (spatially) in rectangle"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        p = mapscript.pointObj(0.0, 0.0)
        assert p in r, (p.x, p.y, r)

    def testRectContainsPointNot(self):
        """point is not contained (spatially) in rectangle"""
        r = mapscript.rectObj(-1.0, -2.0, 3.0, 4.0)
        p = mapscript.pointObj(3.00001, 0.0)
        assert p not in r, (p.x, p.y, r)


class ImageRectObjTestCase(MapPrimitivesTestCase):

    def testRectObjConstructorArgs(self):
        """create a rect in image units"""
        r = mapscript.rectObj(-1.0, 2.0, 3.0, 0.0, mapscript.MS_TRUE)
        self.assertAlmostEqual(r.minx, -1.0)
        self.assertAlmostEqual(r.miny, 2.0)
        self.assertAlmostEqual(r.maxx, 3.0)
        self.assertAlmostEqual(r.maxy, 0.0)

    def testRectObjConstructorBadYCoords(self):
        """in image units miny should be greater than maxy"""
        self.assertRaises(mapscript.MapServerError, mapscript.rectObj,
                          -1.0, 0.0, 3.0, 2.0, mapscript.MS_TRUE)


if __name__ == '__main__':
    unittest.main()
