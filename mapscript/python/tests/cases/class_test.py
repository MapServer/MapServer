# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of classObj
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
from .testing import MapTestCase


class ClassObjTestCase(unittest.TestCase):

    def testConstructorNoArg(self):
        c = mapscript.classObj()
        assert c.thisown == 1
        assert c.layer is None
        assert c.numstyles == 0

    def testConstructorWithArg(self):
        lyr = mapscript.layerObj()
        lyr.name = 'foo'
        c = mapscript.classObj(lyr)
        assert c.thisown == 1
        assert c.layer.name == lyr.name
        assert c.numstyles == 0

    def testGetSetAttributes(self):
        c = mapscript.classObj()

        val = '/tmp/legend.png'
        c.keyimage = val
        assert c.keyimage == val

        c.debug = mapscript.MS_TRUE
        assert c.debug == mapscript.MS_TRUE

        val = 'Group1'
        c.group = val
        assert c.group == val

        val = 10000
        c.maxscaledenom = val
        assert c.maxscaledenom == val

        val = 3
        c.minfeaturesize = val
        assert c.minfeaturesize == val

        val = 1000
        c.minscaledenom = val
        assert c.minscaledenom == val

        val = 'Class1'
        c.name = val
        assert c.name == val

        c.status = mapscript.MS_OFF
        assert c.status == mapscript.MS_OFF

        val = 'template.html'
        c.template = val
        assert c.template == val

        val = 'Title1'
        c.title = val
        assert c.title == val


class ClassCloningTestCase(unittest.TestCase):

    def testCloneClass(self):
        """check attributes of a cloned class"""
        c = mapscript.classObj()
        c.minscaledenom = 5.0
        clone = c.clone()
        assert clone.thisown == 1
        assert clone.minscaledenom == 5.0


class ClassIconTestCase(MapTestCase):

    """testing for bug 1250"""

    def testAlphaTransparentPixmap(self):
        lo = self.map.getLayerByName('INLINE-PIXMAP-RGBA')
        co = lo.getClass(0)
        self.map.selectOutputFormat('PNG')
        im = co.createLegendIcon(self.map, lo, 48, 48)
        im.save('testAlphaTransparentPixmapIcon.png')

    def testAlphaTransparentPixmapPNG24(self):
        lo = self.map.getLayerByName('INLINE-PIXMAP-RGBA')
        co = lo.getClass(0)
        self.map.selectOutputFormat('PNG24')
        im = co.createLegendIcon(self.map, lo, 48, 48)
        im.save('testAlphaTransparentPixmapIcon24.png')

    def testAlphaTransparentPixmapJPG(self):
        lo = self.map.getLayerByName('INLINE-PIXMAP-RGBA')
        co = lo.getClass(0)
        self.map.selectOutputFormat('JPEG')
        im = co.createLegendIcon(self.map, lo, 48, 48)
        im.save('testAlphaTransparentPixmapIcon.jpg')

    def testIndexedTransparentPixmap(self):
        lo = self.map.getLayerByName('INLINE-PIXMAP-PCT')
        lo.type = mapscript.MS_LAYER_POINT
        co = lo.getClass(0)
        self.map.selectOutputFormat('PNG')
        im = co.createLegendIcon(self.map, lo, 32, 32)
        im.save('testIndexedTransparentPixmapIcon.png')

    def testIndexedTransparentPixmapJPG(self):
        lo = self.map.getLayerByName('INLINE-PIXMAP-PCT')
        lo.type = mapscript.MS_LAYER_POINT
        co = lo.getClass(0)
        self.map.selectOutputFormat('JPEG')
        im = co.createLegendIcon(self.map, lo, 32, 32)
        im.save('testIndexedTransparentPixmapIcon.jpg')


if __name__ == '__main__':
    unittest.main()
