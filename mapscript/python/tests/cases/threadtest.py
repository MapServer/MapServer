# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript test of multi-threading
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
#     python tests/cases/threadtest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from testing import mapscript, MapTestCase, TESTMAPFILE

class MultipleLayerAdditionTestCase(MapTestCase):

    def testAdd2Layers(self):
        """add two layers to map"""
        layer_a = mapscript.layerObj(self.map)
        layer_a.name = 'a'
        layer_b = mapscript.layerObj(self.map)
        layer_b.name = 'b'
        assert self.map.numlayers == 9
        assert self.map.getLayerByName('a') != None
        assert self.map.getLayerByName('b') != None

def make_map(i):
    print "making map in thread %d" % (i)
    mo = mapscript.mapObj(TESTMAPFILE)
    im = mo.draw()

class MultipleThreadsTestCase(unittest.TestCase):
    
    def testLayerAdditionMultiThreads(self):
        """mapscripting in multiple threads"""

        import threading
        
        for i in range(10):
            thread = threading.Thread(target=make_map(i))
            thread.start()
         

# -----------------------------------------------------------------------------
if __name__ == '__main__':
    unittest.main()
