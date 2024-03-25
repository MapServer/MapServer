# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Symbol
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
from io import BytesIO

import mapscript

from .testing import HOME_IMAGE, XMARKS_IMAGE, MapTestCase


class SymbolTestCase(unittest.TestCase):
    def testConstructor(self):
        """create new instance of symbolObj"""
        symbol = mapscript.symbolObj("test")
        assert symbol.name == "test"
        assert symbol.thisown == 1

    def testConstructorImage(self):
        """create new instance of symbolObj from an image"""
        symbol = mapscript.symbolObj("xmarks", XMARKS_IMAGE)
        assert symbol.name == "xmarks"
        assert symbol.type == mapscript.MS_SYMBOL_VECTOR  # was MS_SYMBOL_PIXMAP!
        format = mapscript.outputFormatObj("AGG/PNG")
        img = symbol.getImage(format)
        img.save("sym-%s.%s" % (symbol.name, img.format.extension))


class DynamicGraphicSymbolTestCase:
    def setUp(self):
        MapTestCase.setUp(self)
        with open(HOME_IMAGE, "rb") as fh:
            s = BytesIO(fh.read())
        symb_img = mapscript.imageObj(s)
        self.h_symbol = mapscript.symbolObj("house")
        self.h_symbol.type = mapscript.MS_SYMBOL_PIXMAP
        self.h_symbol.setImage(symb_img)
        with open(XMARKS_IMAGE, "rb") as fh:
            s = BytesIO(fh.read())
        symb_img = mapscript.imageObj(s)
        self.x_symbol = mapscript.symbolObj("xmarks")
        self.x_symbol.type = mapscript.MS_SYMBOL_PIXMAP
        self.x_symbol.setImage(symb_img)

    def testSetPCTImage(self):
        """set image of new symbolObj"""
        assert self.h_symbol.name == "house"
        assert self.h_symbol.type == mapscript.MS_SYMBOL_PIXMAP
        format = mapscript.outputFormatObj("AGG/PNG")
        format.transparent = mapscript.MS_ON
        img = self.h_symbol.getImage(format)
        img.save("set-%s.%s" % (self.h_symbol.name, img.format.extension))

    def testDrawSetPCTImage(self):
        """draw a map using the set image symbol"""
        symbol_index = self.map.symbolset.appendSymbol(self.h_symbol)
        assert symbol_index == 4, symbol_index
        num = self.map.symbolset.numsymbols
        assert num == 5, num
        inline_layer = self.map.getLayerByName("INLINE")
        s = inline_layer.getClass(0).getStyle(0)
        s.symbol = symbol_index
        s.size = -1  # pixmap's own size
        img = self.map.draw()
        img.save("testDrawSetPCTImage.%s" % (img.format.extension))

    def testSetRGBAImage(self):
        """set image of new symbolObj"""
        assert self.x_symbol.name == "xmarks"
        assert self.x_symbol.type == mapscript.MS_SYMBOL_PIXMAP
        format = mapscript.outputFormatObj("AGG/PNG")
        img = self.x_symbol.getImage(format)
        img.save("set-%s.%s" % (self.x_symbol.name, img.format.extension))

    def testDrawSetRGBAImage(self):
        """draw a map using the set image symbol"""
        symbol_index = self.map.symbolset.appendSymbol(self.x_symbol)
        inline_layer = self.map.getLayerByName("INLINE")
        s = inline_layer.getClass(0).getStyle(0)
        s.symbol = symbol_index
        s.size = -1  # pixmap's own size
        self.map.selectOutputFormat("PNG24")
        img = self.map.draw()
        img.save("testDrawSetRGBAImage.%s" % (img.format.extension))


class MapSymbolTestCase(MapTestCase):
    def testGetPoints(self):
        """get symbol points as line and test coords"""
        symbol = self.map.symbolset.getSymbol(1)
        assert symbol.name == "circle"
        line = symbol.getPoints()
        assert line.numpoints == 1, line.numpoints
        pt = self.getPointFromLine(line, 0)
        self.assertPointsEqual(pt, mapscript.pointObj(1.0, 1.0))

    def testSetPoints(self):
        """add lines of points to an existing symbol"""
        symbol = self.map.symbolset.getSymbol(1)
        assert symbol.name == "circle"
        line = mapscript.lineObj()
        self.addPointToLine(line, mapscript.pointObj(2.0, 2.0))
        self.addPointToLine(line, mapscript.pointObj(3.0, 3.0))
        assert symbol.setPoints(line) == 2
        assert symbol.numpoints == 2
        line = symbol.getPoints()
        assert line.numpoints == 2, line.numpoints
        pt = self.getPointFromLine(line, 1)
        self.assertPointsEqual(pt, mapscript.pointObj(3.0, 3.0))

    def testSetStyle(self):
        """expect success after setting an existing symbol's style"""
        symbol = self.map.symbolset.getSymbol(1)
        # default MapScript API setPattern has been extended to allow lists
        # assert symbol.setPattern(0, 1) == mapscript.MS_SUCCESS
        symbol.pattern = [0, 1]
        assert symbol.pattern == [0, 1]

    def testRGBASymbolInPNG24(self):
        """draw a RGBA PNG pixmap on PNG canvas"""
        self.map.setImageType("PNG24")
        # self.map.getLayerByName('INLINE-PIXMAP-RGBA').status \
        #    == mapscript.MS_DEFAULT
        img = self.map.draw()
        img.save("pixmap-rgba-24.png")

    def testRGBASymbolInJPEG(self):
        """draw a RGBA PNG pixmap on JPEG canvas"""
        self.map.setImageType("JPEG")
        # self.map.getLayerByName('INLINE-PIXMAP-RGBA').status \
        #    == mapscript.MS_DEFAULT
        img = self.map.draw()
        img.save("pixmap-rgba.jpg")


if __name__ == "__main__":
    unittest.main()
