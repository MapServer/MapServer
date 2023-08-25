# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of shapefileObj
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
import os
import mapscript
from .testing import TESTS_PATH


class AddShapeTestCase(unittest.TestCase):

    def testAddEmpty(self):
        """expect an error rather than segfault when adding an empty shape"""
        # See bug 1201
        sf = mapscript.shapefileObj('testAddDud.shp', 1)
        so = mapscript.shapeObj(mapscript.MS_SHAPE_POINT)
        self.assertRaises(mapscript.MapServerError, sf.add, so)

    def testGetDBFInfo(self):
        """Fetch dbf information from shapefile"""
        pth = os.path.join(TESTS_PATH, "polygon.shp")
        sf = mapscript.shapefileObj(pth)
        assert sf.getDBF() is not None, sf.getDBF()
        assert sf.getDBF().nFields == 2, sf.getDBF().nFields
        assert sf.getDBF().getFieldName(0) == 'FID', sf.getDBF().getFieldName(0)
        assert sf.getDBF().getFieldName(1) == 'FNAME', sf.getDBF().getFieldName(1)


if __name__ == '__main__':
    unittest.main()
