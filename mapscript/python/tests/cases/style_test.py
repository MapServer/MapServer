# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Map Styles
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


class DrawProgrammedStylesTestCase(MapTestCase):
    def testDrawPoints(self):
        """DrawProgrammedStylesTestCase.testDrawPoints: point drawing with styles works as advertised"""
        points = [mapscript.pointObj(-0.2, 51.6),
                  mapscript.pointObj(0.0, 51.2),
                  mapscript.pointObj(0.2, 51.6)]
        colors = [mapscript.colorObj(255, 0, 0),
                  mapscript.colorObj(0, 255, 0),
                  mapscript.colorObj(0, 0, 255)]
        img = self.map.prepareImage()
        layer = self.map.getLayerByName('POINT')
        # layer.draw(self.map, img)
        class0 = layer.getClass(0)
        for i in range(len(points)):
            style0 = class0.getStyle(0)
            style0.color = colors[i]
            # style0.color.pen = -4
            assert style0.color.toHex() == colors[i].toHex()
            points[i].draw(self.map, layer, img, 0, "foo")
        img.save('test_draw_points.png')


class NewStylesTestCase(MapTestCase):

    def testStyleConstructor(self):
        """a new style is properly initialized"""
        new_style = mapscript.styleObj()
        assert new_style.color.red == -1
        assert new_style.color.green == -1
        assert new_style.color.blue == -1

    def testCloneStyle(self):
        """check attributes of a cloned style"""
        new_style = mapscript.styleObj()
        new_style.color.setHex('#808080')
        clone = new_style.clone()
        assert clone.thisown == 1
        assert clone.color.toHex() == '#808080'

    def testStyleColorSettable(self):
        """a style can be set with a color tuple"""
        new_style = mapscript.styleObj()
        new_style.color.setRGB(1, 2, 3)
        assert new_style.color.red == 1
        assert new_style.color.green == 2
        assert new_style.color.blue == 3

    def testStyleBinding(self):
        """attribute binding can be set and get"""
        new_style = mapscript.styleObj()
        assert (not new_style.getBinding(mapscript.MS_STYLE_BINDING_COLOR))
        new_style.setBinding(mapscript.MS_STYLE_BINDING_COLOR, "NEW_BINDING")
        assert (new_style.getBinding(mapscript.MS_STYLE_BINDING_COLOR) == "NEW_BINDING")

    def testAppendNewStyle(self):
        """a new style can be appended properly"""
        p_layer = self.map.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        assert class0.numstyles == 2, class0.numstyles
        new_style = mapscript.styleObj()
        new_style.color.setRGB(0, 0, 0)
        new_style.setSymbolByName(self.map, 'circle')
        new_style.size = 3
        index = class0.insertStyle(new_style)
        assert index == 2, index
        assert class0.numstyles == 3, class0.numstyles
        msimg = self.map.draw()
        assert msimg.thisown == 1
        filename = 'testAppendNewStyle.png'
        fh = open(filename, 'wb')
        msimg.write(fh)
        fh.close()

    def testAppendNewStyleOldWay(self):
        """NewStylesTestCase.testAppendNewStyleOldWay: a new style can be appended properly using old method"""
        p_layer = self.map.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        assert class0.numstyles == 2, class0.numstyles
        new_style = mapscript.styleObj(class0)
        assert new_style.thisown == 1, new_style.thisown
        new_style.color.setRGB(0, 0, 0)
        new_style.symbol = 1
        new_style.size = 3
        msimg = self.map.draw()
        filename = 'testAppendNewStyleOldWay.png'
        fh = open(filename, 'wb')
        msimg.write(fh)
        fh.close()

    def testInsertNewStyleAtIndex0(self):
        """NewStylesTestCase.testInsertNewStyleAtIndex0: a new style can be inserted ahead of all others"""
        l_layer = self.map.getLayerByName('LINE')
        class0 = l_layer.getClass(0)
        new_style = mapscript.styleObj()
        new_style.color.setRGB(255, 255, 0)
        new_style.symbol = 1
        new_style.size = 7
        index = class0.insertStyle(new_style, 0)
        assert index == 0, index
        assert class0.numstyles == 2, class0.numstyles
        msimg = self.map.draw()
        assert msimg.thisown == 1
        filename = 'testInsertNewStyleAtIndex0.png'
        fh = open(filename, 'wb')
        msimg.write(fh)
        fh.close()

    def testRemovePointStyle(self):
        """NewStylesTestCase.testRemovePointStyle: a point style can be properly removed"""
        p_layer = self.map.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        rem_style = class0.removeStyle(1)
        assert rem_style is not None
        assert class0.numstyles == 1, class0.numstyles
        msimg = self.map.draw()
        filename = 'testRemovePointStyle.png'
        msimg.save(filename)

    def testModifyMultipleStyle(self):
        """NewStylesTestCase.testModifyMultipleStyle: multiple styles can be modified"""
        p_layer = self.map.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        style1 = class0.getStyle(1)
        style1.color.setRGB(255, 255, 0)
        msimg = self.map.draw()
        filename = 'testModifyMutiplePointStyle.png'
        msimg.save(filename)

    def testInsertStylePastEnd(self):
        """NewStylesTestCase.testInsertStylePastEnd: inserting a style past the end of
        the list raises the proper error"""
        p_layer = self.map.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        new_style = mapscript.styleObj()
        self.assertRaises(mapscript.MapServerChildError, class0.insertStyle, new_style, 6)

    def testInsertNULLStyle(self):
        """attempt to insert a NULL style should raise error"""
        p_layer = self.map.getLayerByName('POINT')
        class0 = p_layer.getClass(0)
        self.assertRaises(mapscript.MapServerChildError,
                          class0.insertStyle, None)

    def testConvertToString(self):
        """ensure styles can be created and output to strings
        in a round trip"""
        input_string = """
            STYLE
                SIZE 7.25
                COLOR 255 0 0
                OFFSET 10.5 20.75
            END
        """

        new_style = mapscript.fromstring(input_string)
        assert new_style.size == 7.25
        assert new_style.color.red == 255
        assert new_style.color.green == 0
        assert new_style.color.blue == 0

        assert new_style.offsetx == 10.5
        assert new_style.offsety == 20.75

        output_string = new_style.convertToString()
        new_style2 = mapscript.fromstring(output_string)

        # ensure attributes are output as doubles
        assert new_style.size == 7.25
        assert new_style2.offsetx == 10.5
        assert new_style2.offsety == 20.75

    def testPattern(self):
        """See https://github.com/mapserver/mapserver/issues/4943"""

        si = mapscript.styleObj()
        assert si.pattern == ()
        assert si.patternlength == 0

    def testPattern2(self):

        si = mapscript.styleObj()
        si.pattern = [2.0, 3, 4]
        assert si.pattern == (2.0, 3.0, 4.0)
        assert si.patternlength == 3

    def testPattern3(self):
        """a pattern must have at least 2 elements"""

        si = mapscript.styleObj()
        exception = None
        try:
            si.pattern = [1.0]
        except Exception:
            exception = True
        assert exception is True

    def testPattern4(self):
        """a pattern can have a max of 10 elements
        This is set in mapsymbol.h with #define MS_MAXPATTERNLENGTH 10"""

        si = mapscript.styleObj()
        exception = None
        try:
            si.pattern = [i for i in range(11)]
        except Exception:
            exception = True
        assert exception is True

    def testPattern5(self):
        """pattern length is read-only"""
        si = mapscript.styleObj()
        exception = None
        try:
            si.patternlength = 0
        except Exception:
            exception = True
        assert exception is True


class BrushCachingTestCase(MapTestCase):

    def testDrawMapWithSecondPolygon(self):
        """draw a blue polygon and a red polygon"""
        p = self.map.getLayerByName('POLYGON')
        ip = mapscript.layerObj(self.map)
        ip.type = mapscript.MS_LAYER_POLYGON
        ip.status = mapscript.MS_DEFAULT
        c0 = mapscript.classObj(ip)

        # turn off first polygon layer's color
        p.getClass(0).getStyle(0).color.setRGB(-1, -1, -1)

        # copy this style to inline polygon layer, then change outlinecolor
        c0.insertStyle(p.getClass(0).getStyle(0))
        st0 = c0.getStyle(0)
        st0.outlinecolor.setRGB(255, 0, 0)

        # pull out the first feature from polygon layer, shift it
        # and use this as an inline feature in new layer
        p.open()
        p.template = "FAKE"  # set a template so the layer can be queried
        p.queryByIndex(self.map, -1, 0, mapscript.MS_TRUE)
        res = p.getResult(0)
        s0 = p.getShape(res)
        p.close()
        r0 = s0.bounds
        r1 = mapscript.rectObj(r0.minx-0.1, r0.miny-0.1, r0.maxx-0.1, r0.maxy-0.1)
        s1 = r1.toPolygon()

        ip.addFeature(s1)
        img = self.map.draw()
        img.save('test_drawmapw2ndpolygon.png')


if __name__ == '__main__':
    unittest.main()
