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

import unittest
import mapscript
from .testing import MapTestCase


class OutputFormatTestCase(unittest.TestCase):
    """http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=511"""
    def testOutputFormatConstructor(self):
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiff')
        # assert new_format.refcount == 1, new_format.refcount
        assert new_format.name == 'gtiff'
        assert new_format.mimetype == 'image/tiff'


class MapOutputFormatTestCase(MapTestCase):

    def testAppendNewOutputFormat(self):
        """test that a new output format can be created on-the-fly"""
        num = self.map.numoutputformats
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiffx')
        # assert new_format.refcount == 1, new_format.refcount
        self.map.appendOutputFormat(new_format)
        assert self.map.numoutputformats == num + 1
        # assert new_format.refcount == 2, new_format.refcount
        self.map.selectOutputFormat('gtiffx')
        self.map.save('testAppendNewOutputFormat.map')
        self.map.getLayerByName('INLINE-PIXMAP-RGBA').status = mapscript.MS_ON
        imgobj = self.map.draw()
        filename = 'testAppendNewOutputFormat.tif'
        imgobj.save(filename)

    def testRemoveOutputFormat(self):
        """testRemoveOutputFormat may fail depending on GD options"""
        num = self.map.numoutputformats
        new_format = mapscript.outputFormatObj('GDAL/GTiff', 'gtiffx')
        self.map.appendOutputFormat(new_format)
        assert self.map.numoutputformats == num + 1
        # assert new_format.refcount == 2, new_format.refcount
        assert self.map.removeOutputFormat('gtiffx') == mapscript.MS_SUCCESS
        # assert new_format.refcount == 1, new_format.refcount
        assert self.map.numoutputformats == num
        self.assertRaises(mapscript.MapServerError,
                          self.map.selectOutputFormat, 'gtiffx')
        self.map.selectOutputFormat('GTiff')
        assert self.map.outputformat.mimetype == 'image/tiff'

    def testBuiltInPNG24Format(self):
        """test built in PNG RGB format"""
        self.map.selectOutputFormat('PNG24')
        assert self.map.outputformat.mimetype == 'image/png'
        self.map.getLayerByName('INLINE-PIXMAP-RGBA').status = mapscript.MS_ON
        img = self.map.draw()
        assert img.format.mimetype == 'image/png'
        filename = 'testBuiltInPNG24Format.png'
        img.save(filename)

    def testBuiltInJPEGFormat(self):
        """test built in JPEG format"""
        self.map.selectOutputFormat('JPEG')
        assert self.map.outputformat.mimetype == 'image/jpeg'
        self.map.getLayerByName('INLINE-PIXMAP-RGBA').status = mapscript.MS_ON
        img = self.map.draw()
        assert img.format.mimetype == 'image/jpeg'
        filename = 'testBuiltInJPEGFormat.jpg'
        img.save(filename)

    def testSelectBuiltInJPEGFormat(self):
        """test selection of built-in JPEG format"""
        self.map.selectOutputFormat('JPEG')
        assert self.map.outputformat.mimetype == 'image/jpeg'


class UnsupportedFormatTestCase(unittest.TestCase):
    """I (Sean) don't ever configure for PDF, so this is the unsupported
    format."""

    def testCreateUnsupported(self):
        self.assertRaises(mapscript.MapServerError, mapscript.outputFormatObj,
                          'PDF')


if __name__ == '__main__':
    unittest.main()
