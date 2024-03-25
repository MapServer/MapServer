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

from .testing import MapZoomTestCase


class ZoomPointTestCase(MapZoomTestCase):
    def testRecenter(self):
        """ZoomPointTestCase.testRecenter: recentering the map with a point returns the same extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj(50.0, 50.0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(1, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-50, -50, 50, 50))

    def testZoomInPoint(self):
        """ZoomPointTestCase.testZoomInPoint: zooming in by a power of 2 returns the proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj(50.0, 50.0)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-25, -25, 25, 25))

    def testZoomOutPoint(self):
        """ZoomPointTestCase.testZoomOutPoint: zooming out by a power of 2 returns the proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-2, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-100, -100, 100, 100))

    def testZoomOutPointConstrained(self):
        """ZoomPointTestCase.testZoomOutPointConstrained: zooming out to a constrained extent returns proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0, -100.0, 100.0, 100.0)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        self.mapobj1.zoomPoint(-4, p, w, h, extent, max)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, max)

    def testZoomBadSize(self):
        """ZoomPointTestCase.testZoomBadSize: zooming to a bad size raises proper error"""
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        w = 0
        h = -1
        self.assertRaises(
            mapscript.MapServerError, self.mapobj1.zoomPoint, -2, p, w, h, extent, None
        )

    def testZoomBadExtent(self):
        """ZoomPointTestCase.testZoomBadExtent: zooming to a bad extent raises proper error"""
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        extent.maxx = extent.maxx - 1000000
        w = 100
        h = 100
        self.assertRaises(
            mapscript.MapServerError, self.mapobj1.zoomPoint, -2, p, w, h, extent, None
        )


class ZoomRectangleTestCase(MapZoomTestCase):
    def testZoomRectangle(self):
        """ZoomRectangleTestCase.testZoomRectangle: zooming to an extent returns proper map extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        r = mapscript.rectObj(1, 26, 26, 1, 1)
        extent = self.mapobj1.extent
        self.mapobj1.zoomRectangle(r, w, h, extent, None)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, mapscript.rectObj(-49, 24, -24, 49))

    def testZoomRectangleConstrained(self):
        """ZoomRectangleTestCase.testZoomRectangleConstrained: zooming to a constrained extent returns proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj(-100.0, -100.0, 100.0, 100.0)
        r = mapscript.rectObj(0, 200, 200, 0, 1)
        extent = self.mapobj1.extent
        self.mapobj1.zoomRectangle(r, w, h, extent, max)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, max)

    def testZoomRectangleBadly(self):
        """zooming into an invalid extent raises proper error"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        r = mapscript.rectObj(0, 0, 200, 200)
        extent = self.mapobj1.extent
        self.assertRaises(
            mapscript.MapServerError, self.mapobj1.zoomRectangle, r, w, h, extent, None
        )


class ZoomScaleTestCase(MapZoomTestCase):
    def testRecenter(self):
        """ZoomScaleTestCase.testRecenter: recentering map returns proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)  # 100 by 100
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 2834.6472
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        assert self.mapobj1.scaledenom == scale
        new_extent = self.mapobj1.extent
        # self.assertRectsEqual(new_extent, mapscript.rectObj(-50, -50, 50, 50))  # old values
        self.assertRectsEqual(new_extent, mapscript.rectObj(-49.5, -49.5, 49.5, 49.5))

    def testZoomInScale(self):
        """ZoomScaleTestCase.testZoomInScale: zooming in to a specified scale returns proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 1417.3236
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        # self.assertRectsEqual(new_extent, mapscript.rectObj(-25, -25, 25, 25))  # old values
        self.assertRectsEqual(
            new_extent, mapscript.rectObj(-24.75, -24.75, 24.75, 24.75)
        )

    def testZoomOutScale(self):
        """ZoomScaleTestCase.testZoomOutScale: zooming out to a specified scale returns proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        scale = 5669.2944
        extent = self.mapobj1.extent
        self.mapobj1.zoomScale(scale, p, w, h, extent, None)
        new_extent = self.mapobj1.extent
        # self.assertRectsEqual(new_extent, mapscript.rectObj(-100, -100, 100, 100))  # old values
        self.assertRectsEqual(new_extent, mapscript.rectObj(-99, -99, 99, 99))

    def testZoomOutPointConstrained(self):
        """ZoomScaleTestCase.testZoomOutPointConstrained: zooming out to a constrained extent returns proper extent"""
        w, h = (self.mapobj1.width, self.mapobj1.height)
        max = mapscript.rectObj()
        max.minx, max.miny, max.maxx, max.maxy = (-100.0, -100.0, 100.0, 100.0)
        p = mapscript.pointObj()
        p.x, p.y = (50, 50)
        extent = self.mapobj1.extent
        scale = 10000
        self.mapobj1.zoomScale(scale, p, w, h, extent, max)
        new_extent = self.mapobj1.extent
        self.assertRectsEqual(new_extent, max)


if __name__ == "__main__":
    unittest.main()
