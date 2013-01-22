# $Id$
#
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
#
# Execute this module as a script from mapserver/mapscript/python
#
#     python tests/cases/clonetest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript

from testing import mapscript
from testing import TEST_IMAGE as test_image
from testing import TESTMAPFILE
from testing import MapTestCase
from StringIO import StringIO
import cStringIO
import urllib


have_image = 0
try:
    from PIL import Image
    have_image = 1
except ImportError:
    pass

class SaveToStringTestCase(MapTestCase):
    def testSaveToString(self):
        """test that an image can be saved as a string"""
        msimg = self.map.draw()
        assert msimg.thisown == 1
        data = msimg.saveToString()
        filename = 'testSaveToString.png'
        fh = open(filename, 'wb')
        fh.write(data)
        fh.close()
        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.format == 'PNG'
            assert pyimg.size == (200, 200)
            assert pyimg.mode == 'P'
        else:
            assert 1
            
class ImageObjTestCase(unittest.TestCase):
    
    def testConstructor(self):
        """imageObj constructor works"""
        imgobj = mapscript.imageObj(10, 10)
        assert imgobj.thisown == 1
        assert imgobj.height == 10
        assert imgobj.width == 10
    
    def testConstructorWithFormat(self):
        """imageObj with an optional driver works"""
        driver = 'GD/PNG'
        format = mapscript.outputFormatObj(driver)
        imgobj = mapscript.imageObj(10, 10, format)
        assert imgobj.thisown == 1
        assert imgobj.format.driver == driver
        assert imgobj.height == 10
        assert imgobj.width == 10
    
    def testConstructorFilename(self):
        """imageObj with a filename works"""
        imgobj = mapscript.imageObj(test_image)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
        imgobj.save('testConstructorFilename.png')
    
    def testConstructorFilenameDriver(self):
        """imageObj with a filename and a driver works"""
        imgobj = mapscript.imageObj(test_image)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
        imgobj.save('testConstructorFilenameDriver.png')
        
    def testConstructorStream(self):
        """imageObj with a file stream works"""
        f = open(test_image, 'rb')
        imgobj = mapscript.imageObj(f)
        f.close()
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
    
    def testConstructorStringIO(self):
        """imageObj with a cStringIO works"""
        f = open(test_image, 'rb')
        data = f.read()
        f.close()
        s = cStringIO.StringIO(data)
        imgobj = mapscript.imageObj(s)
        assert imgobj.thisown == 1
        assert imgobj.height == 200
        assert imgobj.width == 200
    
    def testConstructorUrlStream(self):
        """imageObj with a URL stream works"""
        url = urllib.urlopen('http://mapserver.org/_static/banner.png')
        imgobj = mapscript.imageObj(url, 'GD/JPEG')
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
            assert pyimg.mode == 'P'
        else:
            assert 1

    def testImageWriteStringIO(self):
        """image writes data to a StringIO instance"""
        image = self.map.draw()
        assert image.thisown == 1
        
        s = cStringIO.StringIO()
        image.write(s)

        filename = 'testImageWriteStringIO.png'
        fh = open(filename, 'wb')
        fh.write(s.getvalue())
        fh.close()
        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.format == 'PNG'
            assert pyimg.size == (200, 200)
            assert pyimg.mode == 'P'
        else:
            assert 1

    def testImageGetBytes(self):
        """image returns bytes"""
        image = self.map.draw()
        assert image.thisown == 1
         
        s = cStringIO.StringIO(image.getBytes())
        
        filename = 'testImageGetBytes.png'
        fh = open(filename, 'wb')
        fh.write(s.getvalue())
        fh.close()
        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.format == 'PNG'
            assert pyimg.size == (200, 200)
            assert pyimg.mode == 'P'
        else:
            assert 1


if __name__ == '__main__':
    unittest.main()

