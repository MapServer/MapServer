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
import threading

# the testing module helps us import the pre-installed mapscript
from testing import mapscript, TESTMAPFILE

def draw_map(name, save=0):
    #print "making map in thread %s" % (name)
    mo = mapscript.mapObj(TESTMAPFILE)
    im = mo.draw()
    if save:
        im.save('threadtest_%s.png' % (name))

def trigger_exception(name):
    #print "triggering exception in thread %s" % (name)
    mo = mapscript.mapObj(TESTMAPFILE)
    try:
        mo.setExtent(1, 50, -1, 51)
        raise Exception, "We expected a MapServer exception"
    except mapscript.MapServerError:
        pass

class MultipleThreadsTestCase(unittest.TestCase):
    
    def testDrawMultiThreads(self):
        """map drawing with multiple threads"""

        workers = []
        for i in range(10):
            name = 'd%d' % (i)
            thread = threading.Thread(target=draw_map, name=name, args=(name,1))
            workers.append(thread)
            thread.start()
         
    def testExceptionsMultiThreads(self):
        """mapserver exceptions behave with multiple threads"""
        
        workers = []
        for i in range(10):
            name = 'e%d' % (i)
            thread = threading.Thread(target=trigger_exception, name=name,
                                      args=(name,))
            workers.append(thread)
            thread.start()
         
    def testExceptionContainmentMultiThreads(self):
        """mapserver exceptions should be contained to a thread"""
        
        num = 100
        workers = []

        # Trigger an exception in the first started thread
        for i in range(0, 1):
            name = 'c%d' % (i)
            thread = threading.Thread(target=trigger_exception, name=name,
                                      args=(name,))
            workers.append(thread)
        
        # Draw normally
        for i in range(1, num):
            name = 'c%d' % (i)
            thread = threading.Thread(target=draw_map, name=name,
                                      args=(name,))
            workers.append(thread)
        
        # Start all threads
        for i in range(num):
            workers[i].start()

    
# -----------------------------------------------------------------------------
if __name__ == '__main__':
    unittest.main()
