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


import unittest

import mapscript

from .testing import MapPrimitivesTestCase


class LineObjTestCase(MapPrimitivesTestCase):
    """Testing the lineObj class in stand-alone mode"""

    def setUp(self):
        """The test fixture is a line with two points"""
        self.points = (mapscript.pointObj(0.0, 1.0), mapscript.pointObj(2.0, 3.0))
        self.line = mapscript.lineObj()
        self.addPointToLine(self.line, self.points[0])
        self.addPointToLine(self.line, self.points[1])

    def testCreateLine(self):
        """created line has the correct number of points"""
        assert self.line.numpoints == 2

    def testGetPointsFromLine(self):
        """points in the line share are the same that were input"""
        for i in range(len(self.points)):
            got_point = self.getPointFromLine(self.line, i)
            self.assertPointsEqual(got_point, self.points[i])

    def testAddPointToLine(self):
        """adding a point to a line behaves correctly"""
        new_point = mapscript.pointObj(4.0, 5.0)
        self.addPointToLine(self.line, new_point)
        assert self.line.numpoints == 3

    def xtestAlterNumPoints(self):
        """
        numpoints is immutable, this should raise error
        """
        # The following works locally but fails on Appveyor (error is not raised)
        # with self.assertRaises(AttributeError):
        #    self.line.numpoints = 3

        # the following approach also fails on appveyor
        error_thrown = False
        try:
            self.line.numpoints = 3
        except AttributeError:
            error_thrown = True

        assert error_thrown is True

    def testLineGeoInterface(self):
        """return line using the  __geo_interface__ protocol"""
        assert self.line.__geo_interface__ == {
            "type": "LineString",
            "coordinates": [(0.0, 1.0, 0.0), (2.0, 3.0, 0.0)],
        }


if __name__ == "__main__":
    unittest.main()
