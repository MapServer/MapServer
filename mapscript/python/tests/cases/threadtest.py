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

def make_map():
    mo = mapscript.mapObj(TESTMAPFILE)
    #mo.setSize(200, 200)
    #mo.setExtent(view.ul.x, view.lr.y, view.lr.x, view.ul.y)
    #mo.setImageType(format)
    #mo.imagecolor.setHex(bgcolor)
    #mo.setProjection('init=%s' % view.srs)
    #mo.setSymbolSet(self.symbolset)
    #mo.setFontSet(self.fontset)
            
    lo = None
    co = None
    so = None

    for i in range(2):
        lo = mapscript.layerObj(mo)
        lo.name = str(i)
        lo.status = mapscript.MS_OFF
        co = mapscript.classObj(lo)
        so = mapscript.styleObj(co)
            
        #lo = mapscript.layerObj(mo)
        #lo.name = str(layer)
        #lo.type = eval('mapscript.MS_LAYER_%s' % (layer.geometrytype))
        #lo.data = layer.datastore.path
        #lo.setProjection('init=%s' % layer.srs)
        #lo.status = mapscript.MS_ON
          
        #    if ruling:
        #        for rule, symbolizers in ruling:
        #            co = mapscript.classObj(lo)
        #        
        #            if symbolizers:
        #                for symbolizer in symbolizers:
        #                    if isinstance(symbolizer, mapping.TextSymbolizer):
        #                        _labelize(co, lo, symbolizer)
        #                    else:
        #                        so = mapscript.styleObj(co)
        #                        _stylize(so, lo, mo, symbolizer, lock)
        #            else:
        #                so = mapscript.styleObj(co)
        #                _stylize(so, lo, mo, None, None)
        #    else:
        #        co = mapscript.classObj(lo)
        #        so = mapscript.styleObj(co)
        #        _stylize(so, lo, mo, None, None)
        #        
        #    # render layer
        #    #self._lock.acquire()
        #    #try:
        #    #lo.draw(mo, im)
        #    #finally:
        #    #    self._lock.release()
                
        #self._lock.acquire()
        #try:
        #mo.drawLabelCache(im)
    im = mo.draw()
        
    del lo
    del co
    del so
    del mo
    

class MultipleThreadsTestCase(unittest.TestCase):
    
    def testLayerAdditionMultiThreads(self):
        """mapscripting in multiple threads"""

        import threading
        
        for i in range(4):
            thread = threading.Thread(target=make_map)
            thread.start()

# -----------------------------------------------------------------------------
if __name__ == '__main__':
    unittest.main()
