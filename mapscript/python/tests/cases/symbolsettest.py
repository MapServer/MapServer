# $Id$
#
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
#
# Execute this module as a script from mapserver/mapscript/python
#
#     python tests/cases/symbolsettest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from testing import mapscript, MapTestCase, TESTMAPFILE, XMARKS_IMAGE

# ===========================================================================
# Test begins now

class SymbolSetTestCase(unittest.TestCase):

    def testConstructorNoArgs(self):
        """SymbolSetTestCase.testConstructorNoArgs: new instance of symbolSetObj should have one symbol"""
        symbolset = mapscript.symbolSetObj()
        num = symbolset.numsymbols
        assert num == 1, num
        
    def testConstructorFile(self):
        """SymbolSetTestCase.testConstructorFile: new instance of symbolSetObj from symbols.txt"""
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        num = symbolset.numsymbols
        assert num == 4, num

    def testAddSymbolToNewSymbolSet(self):
        """SymbolSetTestCase.testAddSymbolToNewSymbolSet: add two new symbols to a SymbolSet"""
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        symbolset.appendSymbol(symbola) 
        symbolset.appendSymbol(symbolb) 
        num = symbolset.numsymbols
        assert num == 6, num
        names = [None, 'line', 'xmarks-png', 'home-png', 'testa', 'testb']
        for i in range(symbolset.numsymbols):
            symbol = symbolset.getSymbol(i)
            assert symbol.name == names[i], symbol.name
            
    def testRemoveSymbolFromNewSymbolSet(self):
        """SymbolSetTestCase.testRemoveSymbolFromNewSymbolSet: after removing a symbol, expect numsymbols -= 1"""
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        symbolset.removeSymbol(1)
        num = symbolset.numsymbols
        assert num == 3, num
        
    def testSaveNewSymbolSet(self):
        """SymbolSetTestCase.testSaveNewSymbolSet: save a new SymbolSet to disk"""
        symbolset = mapscript.symbolSetObj('../../tests/symbols.txt')
        symbola = mapscript.symbolObj('testa')
        symbolb = mapscript.symbolObj('testb')
        symbolset.appendSymbol(symbola) 
        symbolset.appendSymbol(symbolb)
        assert symbolset.save('new_symbols.txt') == mapscript.MS_SUCCESS


class MapSymbolSetTestCase(MapTestCase):

    def testGetNumSymbols(self):
        """MapSymbolSetTestCase.testGetNumSymbols: expect getNumSymbols == 2 from test fixture test.map"""
        num = self.map.getNumSymbols()
        assert num == 4, num
        
    def testSymbolSetNumsymbols(self):
        """MapSymbolSetTestCase.testSymbolSetNumsymbols: expect numsymbols == 2 from test fixture test.map"""
        num = self.map.symbolset.numsymbols
        assert num == 4, num
        
    def testSymbolSetSymbolNames(self):
        """MapSymbolSetTestCase.testSymbolSetSymbolNames: test names of symbols in test fixture's symbolset"""
        set = self.map.symbolset
        names = [None, 'line', 'xmarks-png', 'home-png']
        for i in range(set.numsymbols):
            symbol = set.getSymbol(i)
            assert symbol.name == names[i], symbol.name
            
    def testSymbolIndex(self):
        """MapSymbolSetTestCase.testSymbolIndex: expect index of 'line' symbol to be 1 in test fixture symbolset"""
        i = self.map.symbolset.index('line')
        assert i == 1, i
        
    def testDrawNewSymbol(self):
        """MapSymbolSetTestCase.testDrawNewSymbol: draw using a new symbol added to the fixture"""
        symbol = mapscript.symbolObj('xmarks', XMARKS_IMAGE)
        symbol_index = self.map.symbolset.appendSymbol(symbol)
        assert symbol_index == 4, symbol_index
        num = self.map.symbolset.numsymbols
        assert num == 5, num
        inline_layer = self.map.getLayerByName('INLINE')
        s = inline_layer.getClass(0).getStyle(0)
        s.symbol = symbol_index
        #s.size = 24
        msimg = self.map.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testDrawNewSymbol.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()

                
# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
