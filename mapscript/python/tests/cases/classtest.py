# $Id$
#
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
#
# Execute this module as a script from mapserver/mapscript/python
#
#     python tests/cases/classtest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from .testing import mapscript, MapTestCase

# ===========================================================================
# Test begins now

class ClassObjTestCase(unittest.TestCase):
    
    def testConstructorNoArg(self):
        c = mapscript.classObj()
        assert c.thisown == 1
        assert c.layer == None 
        assert c.numstyles == 0

    def testConstructorWithArg(self):
        l = mapscript.layerObj()
        l.name = 'foo'
        c = mapscript.classObj(l)
        assert c.thisown == 1
        assert c.layer.name == l.name
        assert c.numstyles == 0

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

        
# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
