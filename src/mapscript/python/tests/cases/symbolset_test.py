# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of SymbolSet
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

import os
import unittest
import mapscript
from .testing import MapTestCase, XMARKS_IMAGE, TESTS_PATH

SYMBOLSET = os.path.join(TESTS_PATH, "symbols.txt")


class SymbolSetTestCase(unittest.TestCase):

    def testConstructorNoArgs(self):
        """new instance of symbolSetObj should have one symbol"""
        symbolset = mapscript.symbolSetObj()
        num = symbolset.numsymbols
        assert num == 1, num

    def testConstructorFile(self):
        """new instance of symbolSetObj from symbols.txt"""
        symbolset = mapscript.symbolSetObj(SYMBOLSET)
        num = symbolset.numsymbols
        assert num == 4, num

    def testAddSymbolToNewSymbolSet(self):
        """add two new symbols to a SymbolSet"""
        symbolset = mapscript.symbolSetObj(SYMBOLSET)
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        symbolset.appendSymbol(symbola)
        symbolset.appendSymbol(symbolb)
        num = symbolset.numsymbols
        assert num == 6, num
        names = [None, 'circle', 'xmarks-png', 'home-png', 'testa', 'testb']
        for i in range(symbolset.numsymbols):
            symbol = symbolset.getSymbol(i)
            assert symbol.name == names[i], symbol.name

    def testRemoveSymbolFromNewSymbolSet(self):
        """after removing a symbol, expect numsymbols -= 1"""
        symbolset = mapscript.symbolSetObj(SYMBOLSET)
        symbolset.removeSymbol(1)
        num = symbolset.numsymbols
        assert num == 3, num

    def testSaveNewSymbolSet(self):
        """save a new SymbolSet to disk"""
        symbolset = mapscript.symbolSetObj(SYMBOLSET)
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        symbolset.appendSymbol(symbola)
        symbolset.appendSymbol(symbolb)
        assert symbolset.save('new_symbols.txt') == mapscript.MS_SUCCESS

    def testError(self):
        symbolset = mapscript.symbolSetObj(SYMBOLSET)
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        symbolset.appendSymbol(symbola)
        symbolset.appendSymbol(symbolb)
        self.assertRaises(mapscript.MapServerError, symbolset.save, '/bogus/new_symbols.txt')


class MapSymbolSetTestCase(MapTestCase):

    def testGetNumSymbols(self):
        """expect getNumSymbols == 2 from test fixture test.map"""
        num = self.map.getNumSymbols()
        assert num == 4, num

    def testSymbolSetNumsymbols(self):
        """expect numsymbols == 2 from test fixture test.map"""
        num = self.map.symbolset.numsymbols
        assert num == 4, num

    def testSymbolSetSymbolNames(self):
        """test names of symbols in test fixture's symbolset"""
        set = self.map.symbolset
        names = [None, 'circle', 'xmarks-png', 'home-png']
        for i in range(set.numsymbols):
            symbol = set.getSymbol(i)
            assert symbol.name == names[i], symbol.name

    def testSymbolIndex(self):
        """expect index of 'circle' symbol to be 1 in test fixture symbolset"""
        i = self.map.symbolset.index('circle')
        assert i == 1, i

    def testBug1962(self):
        """resetting imagepath doesn't cause segfault"""
        layer = self.map.getLayerByName('POINT')
        style0 = layer.getClass(0).getStyle(0)
        sym0 = style0.symbol
        sym1 = self.map.symbolset.getSymbol(sym0)
        sym2 = mapscript.symbolObj('xxx')
        assert sym2 is not None
        sym1.setImagepath(XMARKS_IMAGE)
        # self.assertRaises(IOError, sym1.setImagepath, '/bogus/new_symbols.txt')

        msimg = self.map.draw()
        assert msimg.thisown == 1
        data = msimg.getBytes()
        filename = 'testBug1962.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()

    def testDrawNewSymbol(self):
        """draw using a new symbol added to the fixture"""
        symbol = mapscript.symbolObj('xmarks', XMARKS_IMAGE)
        symbol_index = self.map.symbolset.appendSymbol(symbol)
        assert symbol_index == 4, symbol_index
        num = self.map.symbolset.numsymbols
        assert num == 5, num
        inline_layer = self.map.getLayerByName('INLINE')
        s = inline_layer.getClass(0).getStyle(0)
        s.symbol = symbol_index
        # s.size = 24
        msimg = self.map.draw()
        assert msimg.thisown == 1
        filename = 'testDrawNewSymbol.png'
        fh = open(filename, 'wb')
        msimg.write(fh)
        fh.close()


if __name__ == '__main__':
    unittest.main()
