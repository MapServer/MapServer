# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of map cloning
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

from .testing import TESTMAPFILE


class MapCloneTestCase(unittest.TestCase):
    """Base class for testing with a map fixture"""

    def setUp(self):
        self.mapobj_orig = mapscript.mapObj(TESTMAPFILE)
        self.mapobj_clone = self.mapobj_orig.clone()

    def tearDown(self):
        self.mapobj_orig = None
        self.mapobj_clone = None


class MapCloningTestCase(MapCloneTestCase):
    def testClonedName(self):
        """MapCloningTestCase.testClonedName: the name of a cloned map equals the original"""
        assert self.mapobj_clone.name == self.mapobj_orig.name

    def testClonedLayers(self):
        """MapCloningTestCase.testClonedLayers: the layers of a cloned map equal the original"""
        assert self.mapobj_clone.numlayers == self.mapobj_orig.numlayers
        assert self.mapobj_clone.getLayer(0).data == self.mapobj_orig.getLayer(0).data

    def testSetFontSet(self):
        """MapCloningTestCase.testSetFontSet: the number of fonts in a cloned map equal original"""
        assert self.mapobj_clone.fontset.numfonts == 2

    def testSetSymbolSet(self):
        """MapCloningTestCase.testSetSymbolSet: the number of symbols in a cloned map equal original"""
        num = self.mapobj_clone.symbolset.numsymbols
        assert num == 5, num

    def testDrawClone(self):
        """drawing a cloned map works properly"""
        msimg = self.mapobj_clone.draw()
        assert msimg.thisown == 1
        filename = "testClone.png"
        fh = open(filename, "wb")
        msimg.write(fh)
        fh.close()

    def testDrawCloneJPEG(self):
        """drawing a cloned map works properly"""
        self.mapobj_clone.selectOutputFormat("JPEG")
        self.mapobj_clone.getLayerByName("INLINE-PIXMAP-RGBA").status = mapscript.MS_ON
        msimg = self.mapobj_clone.draw()
        assert msimg.thisown == 1
        filename = "testClone.jpg"
        fh = open(filename, "wb")
        msimg.write(fh)
        fh.close()

    def testDrawClonePNG24(self):
        """drawing a cloned map works properly"""
        self.mapobj_clone.selectOutputFormat("PNG24")
        self.mapobj_clone.getLayerByName("INLINE-PIXMAP-RGBA").status = mapscript.MS_ON
        msimg = self.mapobj_clone.draw()
        assert msimg.thisown == 1
        filename = "testClonePNG24.png"
        fh = open(filename, "wb")
        msimg.write(fh)
        fh.close()


if __name__ == "__main__":
    unittest.main()
