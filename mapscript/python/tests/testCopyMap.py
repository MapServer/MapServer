# $Id$
#
# Unit tests concerning the clone() extension to mapObj

import os, sys
import distutils.util
import unittest

# Construct the distutils build path, allowing us to test before
# installing the module
platformdir = '-'.join((distutils.util.get_platform(), 
                       '.'.join(map(str, sys.version_info[0:2]))))
sys.path.insert(0, 'build/lib.' + platformdir)

# The testing mapfile
testMapfile = 'tests/test.map'

# Import mapscript and define test case classes
from mapscript import *

class CopyMapObjTestCase(unittest.TestCase):
    def setUp(self):
        self.mapobj1 = mapObj(testMapfile)
        self.mapobj2 = self.mapobj1.clone()
    def tearDown(self):
        self.mapobj2 = None
    def testtemplatepattern(self):
        val = self.mapobj1.templatepattern
        self.mapobj1 = None
        assert self.mapobj2.templatepattern == val
    def testresolution(self):
        val = self.mapobj1.resolution
        self.mapobj1 = None
        assert self.mapobj2.resolution == val
    def testunits(self):
        val = self.mapobj1.units
        self.mapobj1 = None
        assert self.mapobj2.units == val
    def testinterlace(self):
        val = self.mapobj1.interlace
        #self.mapobj1 = None
        assert self.mapobj2.interlace == val
    def testwidth(self):
        val = self.mapobj1.width
        self.mapobj1 = None
        assert self.mapobj2.width == val
    def testcellsize(self):
        val = self.mapobj1.cellsize
        self.mapobj1 = None
        assert self.mapobj2.cellsize == val
    def testdatapattern(self):
        val = self.mapobj1.datapattern
        self.mapobj1 = None
        assert self.mapobj2.datapattern == val
    def testimagecolor(self):
        val = self.mapobj1.imagecolor.red
        self.mapobj1 = None
        assert self.mapobj2.imagecolor.red == val
    def testimagequality(self):
        val = self.mapobj1.imagequality
        self.mapobj1 = None
        assert self.mapobj2.imagequality == val
    def testname(self):
        val = self.mapobj1.name
        self.mapobj1 = None
        assert self.mapobj2.name == val
    def testscale(self):
        val = self.mapobj1.scale
        self.mapobj1 = None
        assert self.mapobj2.scale == val
    def testnumlayers(self):
        val = self.mapobj1.numlayers
        self.mapobj1 = None
        assert self.mapobj2.numlayers == val
    def testimagetype(self):
        val = self.mapobj1.imagetype
        self.mapobj1 = None
        assert self.mapobj2.imagetype == val
    def testtransparent(self):
        val = self.mapobj1.transparent
        self.mapobj1 = None
        assert self.mapobj2.transparent == val
    def testnumoutputformats(self):
        val = self.mapobj1.numoutputformats
        self.mapobj1 = None
        assert self.mapobj2.numoutputformats == val
    def testmappath(self):
        val = self.mapobj1.mappath
        self.mapobj1 = None
        assert self.mapobj2.mappath == val
    def testextent(self):
        val = self.mapobj1.extent.minx
        self.mapobj1 = None
        assert self.mapobj2.extent.minx == val
    def testheight(self):
        val = self.mapobj1.height
        self.mapobj1 = None
        assert self.mapobj2.height == val
    def testshapepath(self):
        val = self.mapobj1.shapepath
        self.mapobj1 = None
        assert self.mapobj2.shapepath == val
    def testoutputformat(self):
        val = self.mapobj1.outputformat.name
        self.mapobj1 = None
        assert self.mapobj2.outputformat.name == val
    def testdebug(self):
        val = self.mapobj1.debug
        self.mapobj1 = None
        assert self.mapobj2.debug == val
    def teststatus(self):
        val = self.mapobj1.status
        self.mapobj1 = None
        assert self.mapobj2.status == val
    def testdraw(self):
        img1 = self.mapobj1.draw()
        data1 = img1.saveToString()
        self.mapobj1 = None
        img2 = self.mapobj2.draw()
        data2 = img2.saveToString()
        assert data1 == data2

if __name__ == '__main__':
    unittest.main()

