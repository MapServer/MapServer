# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of outputFormatObj
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
#     python tests/cases/outputformattest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from testing import mapscript
from testing import MapTestCase

# ===========================================================================
# Test begins now

class OutputFormatTestCase(unittest.TestCase):
    """http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=511"""
    def testOutputFormatConstructor(self):
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiff')
        assert new_format.refcount == 1, new_format.refcount
        assert new_format.name == 'gtiff'
        assert new_format.mimetype == 'image/tiff'

class MapOutputFormatTestCase(MapTestCase):

    def testAppendNewOutputFormat(self):
        """MapOutputFormatTestCase.testAppendNewOutputFormat: test that a new output format can be created on-the-fly"""
        num = self.map.numoutputformats
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiffx')
        assert new_format.refcount == 1, new_format.refcount
        self.map.appendOutputFormat(new_format)
        assert self.map.numoutputformats == num + 1
        assert new_format.refcount == 2, new_format.refcount
        self.map.setImageType('gtiffx')
        self.map.save('testAppendNewOutputFormat.map')
        imgobj = self.map.draw()
        filename = 'testAppendNewOutputFormat.tif'
        imgobj.save(filename)
        
    def testRemoveOutputFormat(self):
        """MapOutputFormatTestCase.testRemoveOutputFormat: testRemoveOutputFormat may fail depending on GD options"""
        num = self.map.numoutputformats
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiffx')
        self.map.appendOutputFormat(new_format)
        assert self.map.numoutputformats == num + 1
        assert new_format.refcount == 2, new_format.refcount
        assert self.map.removeOutputFormat('gtiffx') == mapscript.MS_SUCCESS
        assert new_format.refcount == 1, new_format.refcount
        assert self.map.numoutputformats == num
        self.assertRaises(mapscript.MapServerError,
                          self.map.setImageType, 'gtiffx')
        self.map.setImageType('GTiff')
        assert self.map.outputformat.mimetype == 'image/tiff'


# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
