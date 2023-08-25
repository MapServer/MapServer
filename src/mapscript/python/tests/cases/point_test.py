# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Point
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


have_image = False

try:
    from PIL import Image
    have_image = True
except ImportError:
    pass


class PointObjTestCase(unittest.TestCase):

    def testPointObjConstructorNoArgs(self):
        """point can be created with no arguments"""
        p = mapscript.pointObj()
        self.assertAlmostEqual(p.x, 0.0)
        self.assertAlmostEqual(p.y, 0.0)

    def testPointObjConstructorArgs(self):
        """point can be created with arguments"""
        p = mapscript.pointObj(1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)

    def testSetXY(self):
        """point can have its x and y reset"""
        p = mapscript.pointObj()
        p.setXY(1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
        if hasattr(p, 'm'):
            self.assertAlmostEqual(p.m, -2e38)

    def testSetXYM(self):
        """point can have its x and y reset (with m value)"""
        p = mapscript.pointObj()
        p.setXY(1.0, 1.0, 1.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 1.0)
        if hasattr(p, 'm'):
            self.assertAlmostEqual(p.m, 1.0)

    def testSetXYZ(self):
        """point can have its x, y, z reset (with m value)"""
        p = mapscript.pointObj()
        p.setXYZ(1.0, 2.0, 3.0, 4.0)
        self.assertAlmostEqual(p.x, 1.0)
        self.assertAlmostEqual(p.y, 2.0)
        if hasattr(p, 'z') and hasattr(p, 'm'):
            self.assertAlmostEqual(p.z, 3.0)
            self.assertAlmostEqual(p.m, 4.0)

    def testPointDraw(self):
        """Can create a point, add to a layer, and draw it directly"""

        map_string = """
        MAP
            EXTENT 0 0 90 90
            SIZE 500 500

            SYMBOL
            NAME "circle"
            TYPE ELLIPSE
            POINTS 1 1 END
            FILLED true
            END

            LAYER
            NAME "punkt"
            STATUS ON
            TYPE POINT
            END

        END"""

        test_map = mapscript.fromstring(map_string)
        layer = test_map.getLayerByName('punkt')
        cls = mapscript.classObj()
        style = mapscript.styleObj()
        style.outlinecolor.setHex('#00aa00ff')
        style.size = 10
        style.setSymbolByName(test_map, 'circle')

        cls.insertStyle(style)
        class_idx = layer.insertClass(cls)
        point = mapscript.pointObj(45, 45)

        img = test_map.prepareImage()
        point.draw(test_map, layer, img, class_idx, "test")

        filename = 'testDrawPoint.png'
        with open(filename, 'wb') as fh:
            img.write(fh)

        if have_image:
            pyimg = Image.open(filename)
            assert pyimg.size == (500, 500)

    def testPoint__str__(self):
        """return properly formatted string"""
        p = mapscript.pointObj(1.0, 1.0)
        if hasattr(p, 'z'):
            p_str = "{ 'x': %.16g, 'y': %.16g, 'z': %.16g }" % (p.x, p.y, p.z)
        else:
            p_str = "{ 'x': %.16g, 'y': %.16g }" % (p.x, p.y)
        assert str(p) == p_str, str(p)

    def testPointToString(self):
        """return properly formatted string in toString()"""
        p = mapscript.pointObj(1.0, 1.0, 0.002, 15.0)
        if hasattr(p, 'z') and hasattr(p, 'm'):
            p_str = "{ 'x': %.16g, 'y': %.16g, 'z': %.16g, 'm': %.16g }" \
                  % (p.x, p.y, p.z, p.m)
        else:
            p_str = "{ 'x': %.16g, 'y': %.16g }" % (p.x, p.y)

        assert p.toString() == p_str, p.toString()

    def testPointGeoInterface(self):
        """return point using the  __geo_interface__ protocol"""
        p = mapscript.pointObj(1.0, 1.0, 0.002, 15.0)
        if hasattr(p, 'z'):
            assert p.__geo_interface__ == {"type": "Point", "coordinates": (1.0, 1.0, 0.002)}
        else:
            assert p.__geo_interface__ == {"type": "Point", "coordinates": (1.0, 1.0)}


if __name__ == '__main__':
    unittest.main()
