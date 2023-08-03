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
from io import BytesIO
import urllib
import mapscript
from .testing import TEST_IMAGE as test_image
from .testing import MapTestCase


have_image = False

try:
    from PIL import Image
    have_image = True
except ImportError:
    pass


class ImageObjTestCase(unittest.TestCase):

    def testConstructor(self):
        """imageObj constructor works"""
        imgobj = mapscript.imageObj(10, 10)
        assert imgobj.thisown == 1
        assert imgobj.height == 10
        assert imgobj.width == 10

    def testConstructorWithFormat(self):
        """imageObj with an optional driver works"""
        driver = 'AGG/PNG'
        format = mapscript.outputFormatObj(driver)
        imgobj = mapscript.imageObj(10, 10, format)
        assert imgobj.thisown == 1
        assert imgobj.format.driver == driver
        assert imgobj.height == 10
        assert imgobj.width == 10

    def xtestConstructorFilename(self):
        """imageObj with a filename works"""
        imgobj = mapscript.imageObj(test_image)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
        imgobj.save('testConstructorFilename.png')

    def xtestConstructorFilenameDriver(self):
        """imageObj with a filename and a driver works"""
        imgobj = mapscript.imageObj(test_image)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
        imgobj.save('testConstructorFilenameDriver.png')

    def xtestConstructorStream(self):
        """imageObj with a file stream works"""
        f = open(test_image, 'rb')
        imgobj = mapscript.imageObj(f)
        f.close()
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200

    def xtestConstructorBytesIO(self):
        """imageObj with a BytesIO works"""
        with open(test_image, 'rb') as f:
            data = f.read()
        s = BytesIO(data)
        imgobj = mapscript.imageObj(s)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200

    def xtestConstructorUrlStream(self):
        """imageObj with a URL stream works"""
        url = urllib.urlopen('http://mapserver.org/_static/banner.png')
        imgobj = mapscript.imageObj(url, 'AGG/JPEG')
        assert imgobj.thisown == 1
        assert imgobj.height == 68
        assert imgobj.width == 439
        imgobj.save('testConstructorUrlStream.jpg')


class ImageWriteTestCase(MapTestCase):

    def testImageWrite(self):
        """image writes data to an open filehandle"""
        image = self.map.draw()
        assert image.thisown == 1
        filename = 'testImageWrite.png'
        fh = open(filename, 'wb')
        image.write(fh)
        fh.close()
        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.format == 'PNG'
            assert pyimg.size == (200, 200)
            assert pyimg.mode == 'RGB'

    def testImageWriteBytesIO(self):
        """image writes data to a BytesIO instance"""
        image = self.map.draw()
        assert image.thisown == 1

        s = BytesIO()
        image.write(s)

        filename = 'testImageWriteBytesIO.png'
        with open(filename, 'wb') as fh:
            fh.write(s.getvalue())
        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.format == 'PNG'
            assert pyimg.size == (200, 200)
            assert pyimg.mode == 'RGB'

    def testImageGetBytes(self):
        """image returns bytes"""
        image = self.map.draw()
        assert image.thisown == 1

        s = BytesIO(image.getBytes())

        filename = 'testImageGetBytes.png'
        with open(filename, 'wb') as fh:
            fh.write(s.getvalue())
        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.format == 'PNG'
            assert pyimg.size == (200, 200)
            assert pyimg.mode == 'RGB'


if __name__ == '__main__':
    unittest.main()
