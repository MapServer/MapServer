# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Map
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
from .testing import MapTestCase, TESTMAPFILE


class MapConstructorTestCase(unittest.TestCase):

    def testMapConstructorNoArg(self):
        """MapConstructorTestCase.testMapConstructorNoArg: test map constructor with no argument"""
        test_map = mapscript.mapObj()
        assert test_map.__class__.__name__ == "mapObj"
        assert test_map.thisown == 1

    def testMapConstructorEmptyStringArg(self):
        """MapConstructorTestCase.testMapConstructorEmptyStringArg:
        test map constructor with old-style empty string argument"""
        test_map = mapscript.mapObj('')
        assert test_map.__class__.__name__ == "mapObj"
        assert test_map.thisown == 1

    def testMapConstructorFilenameArg(self):
        """MapConstructorTestCasetest.testMapConstructorEmptyStringArg: map constructor with filename argument"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        assert test_map.__class__.__name__ == "mapObj"
        assert test_map.thisown == 1


class MapExtentTestCase(MapTestCase):
    def testSetExtent(self):
        """MapExtentTestCase.testSetExtent: test the setting of a mapObj's extent"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        e = test_map.extent
        result = test_map.setExtent(e.minx, e.miny, e.maxx, e.maxy)
        self.assertAlmostEqual(test_map.scaledenom, 14.24445829)
        assert result == mapscript.MS_SUCCESS, result

    def testSetExtentBadly(self):
        """MapExtentTestCase.testSetExtentBadly: test that mapscript raises an error for an invalid mapObj extent"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        self.assertRaises(mapscript.MapServerError, test_map.setExtent,
                          1.0, -2.0, -3.0, 4.0)

    def testReBindingExtent(self):
        """test the rebinding of a map's extent"""
        test_map = mapscript.mapObj(TESTMAPFILE)
        rect1 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        rect2 = mapscript.rectObj(-10.0, -10.0, 10.0, 10.0)
        test_map.extent = rect1
        assert repr(test_map.extent) != repr(rect1), (test_map.extent, rect1)
        del rect1
        self.assertRectsEqual(test_map.extent, rect2)


class MapLayersTestCase(MapTestCase):

    def testMapInsertLayer(self):
        """MapLayersTestCase.testMapInsertLayer: test insertion of a new layer at default (last) index"""
        n = self.map.numlayers
        layer = mapscript.layerObj()
        layer.name = 'new'
        assert layer.map is None, layer.map
        index = self.map.insertLayer(layer)
        assert layer.map is not None, layer.map
        assert index == n, index
        assert self.map.numlayers == n + 1
        names = [self.map.getLayer(i).name for i in range(self.map.numlayers)]
        assert names == ['RASTER', 'POLYGON', 'LINE', 'POINT', 'INLINE',
                         'INLINE-PIXMAP-RGBA', 'INLINE-PIXMAP-PCT', 'new']
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2, 3, 4, 5, 6, 7), order

    def testMapInsertLayerAtZero(self):
        """MapLayersTestCase.testMapInsertLayerAtZero: test insertion of a new layer at first index"""
        n = self.map.numlayers
        layer = mapscript.layerObj()
        layer.name = 'new'
        assert layer.map is None, layer.map
        index = self.map.insertLayer(layer, 0)
        assert layer.map is not None, layer.map
        assert index == 0, index
        assert self.map.numlayers == n + 1
        names = [self.map.getLayer(i).name for i in range(self.map.numlayers)]
        assert names == ['new', 'RASTER', 'POLYGON', 'LINE', 'POINT', 'INLINE',
                         'INLINE-PIXMAP-RGBA', 'INLINE-PIXMAP-PCT'], names
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2, 3, 4, 5, 6, 7), order

    def testMapInsertLayerDrawingOrder(self):
        """MapLayersTestCase.testMapInsertLayerDrawingOrder: test affect of
        insertion of a new layer at index 1 on drawing order"""
        n = self.map.numlayers
        assert n == 7
        # reverse layer drawing order
        o_start = (6, 5, 4, 3, 2, 1, 0)
        self.map.setLayerOrder(o_start)
        # insert Layer
        layer = mapscript.layerObj()
        layer.name = 'new'
        index = self.map.insertLayer(layer, 1)
        assert index == 1, index
        # We expect our new layer to be at index 1 in drawing order as well
        order = self.map.getLayerOrder()
        assert order == (7, 1, 6, 5, 4, 3, 2, 0), order

    def testMapInsertLayerBadIndex(self):
        """MapLayersTestCase.testMapInsertLayerBadIndex: expect an exception when index is too large"""
        layer = mapscript.layerObj()
        self.assertRaises(mapscript.MapServerChildError, self.map.insertLayer,
                          layer, 1000)

    def testMapInsertNULLLayer(self):
        """expect an exception on attempt to insert a NULL Layer"""
        self.assertRaises(mapscript.MapServerChildError, self.map.insertLayer,
                          None)

    def testMapRemoveLayerAtTail(self):
        """removal of highest index (tail) layer"""
        n = self.map.numlayers
        layer = self.map.removeLayer(n-1)
        assert self.map.numlayers == n-1
        assert layer.name == 'INLINE-PIXMAP-PCT'
        assert layer.thisown == 1
        del layer
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2, 3, 4, 5), order

    def testMapRemoveLayerAtZero(self):
        """removal of lowest index (0) layer"""
        n = self.map.numlayers
        layer = self.map.removeLayer(0)
        assert self.map.numlayers == n-1
        assert layer.name == 'RASTER'
        order = self.map.getLayerOrder()
        assert order == (0, 1, 2, 3, 4, 5), order

    def testMapRemoveLayerDrawingOrder(self):
        """test affect of layer removal on drawing order"""
        n = self.map.numlayers
        # reverse layer drawing order
        o_start = (6, 5, 4, 3, 2, 1, 0)
        self.map.setLayerOrder(o_start)
        layer = self.map.removeLayer(1)
        assert self.map.numlayers == n-1
        assert layer.name == 'POLYGON'
        order = self.map.getLayerOrder()
        assert order == (5, 4, 3, 2, 1, 0), order
        names = [self.map.getLayer(i).name for i in range(self.map.numlayers)]
        assert names == ['RASTER', 'LINE', 'POINT', 'INLINE',
                         'INLINE-PIXMAP-RGBA', 'INLINE-PIXMAP-PCT'], names


class MapExceptionTestCase(MapTestCase):

    def testDrawBadData(self):
        """MapExceptionTestCase.testDrawBadData: a bad data descriptor in a layer returns proper error"""
        self.map.getLayerByName('POLYGON').data = 'foo'
        self.assertRaises(mapscript.MapServerError, self.map.draw)


class EmptyMapExceptionTestCase(unittest.TestCase):

    def setUp(self):
        self.map = mapscript.mapObj('')

    def tearDown(self):
        self.map = None

    def testDrawEmptyMap(self):
        """EmptyMapExceptionTestCase.testDrawEmptyMap: drawing an empty map returns proper error"""
        self.assertRaises(mapscript.MapServerError, self.map.draw)


class NoFontSetTestCase(unittest.TestCase):

    def testNoGetFontSetFile(self):
        """an empty map should have fontset filename == None"""
        self.map = mapscript.mapObj()
        assert self.map.fontset.filename is None


class MapFontSetTestCase(MapTestCase):

    def testGetFontSetFile(self):
        """expect fontset file to be 'fonts.txt'"""
        file = self.map.fontset.filename
        assert file == 'fonts.txt', file


class MapSizeTestCase(MapTestCase):

    def testDefaultSize(self):
        assert self.map.width == 200
        assert self.map.height == 200

    def testSetSize(self):
        retval = self.map.setSize(480, 480)
        assert retval == mapscript.MS_SUCCESS, retval
        assert self.map.width == 480
        assert self.map.height == 480


class MapSetWKTTestCase(MapTestCase):

    def testOGCWKT(self):
        self.map.setWKTProjection('''PROJCS["unnamed",GEOGCS["WGS 84",DATUM["WGS_1984",
                                     SPHEROID["WGS 84",6378137,298.257223563]],
                                     PRIMEM["Greenwich",0],
                                     UNIT["Degree",0.0174532925199433]],
                                     PROJECTION["Albers_Conic_Equal_Area"],
                                     PARAMETER["standard_parallel_1", 65], PARAMETER["standard_parallel_2", 55],
                                     PARAMETER["latitude_of_center", 0], PARAMETER["longitude_of_center", -153],
                                     PARAMETER["false_easting", -4943910.68], PARAMETER["false_northing", 0],
                                     UNIT["metre",1.0]
                                     ]''')
        proj4 = self.map.getProjection()

        assert proj4.find('+proj=aea') != -1
        assert proj4.find('+datum=WGS84') != -1
        assert mapscript.projectionObj(proj4).getUnits() != mapscript.MS_DD

    def testESRIWKT(self):
        self.map.setWKTProjection('ESRI::PROJCS["Pulkovo_1995_GK_Zone_2", GEOGCS["GCS_Pulkovo_1995", '
                                  'DATUM["D_Pulkovo_1995", SPHEROID["Krasovsky_1940", 6378245, 298.3]], '
                                  'PRIMEM["Greenwich", 0], '
                                  'UNIT["Degree", 0.017453292519943295]], PROJECTION["Gauss_Kruger"], '
                                  'PARAMETER["False_Easting", 2500000], '
                                  'PARAMETER["False_Northing", 0], PARAMETER["Central_Meridian", 9], '
                                  'PARAMETER["Scale_Factor", 1], '
                                  'PARAMETER["Latitude_Of_Origin", 0], UNIT["Meter", 1]]')
        proj4 = self.map.getProjection()

        assert proj4.find('+proj=tmerc') != -1
        assert proj4.find('+ellps=krass') != -1
        assert (mapscript.projectionObj(proj4)).getUnits() != mapscript.MS_DD

    def testWellKnownGEOGCS(self):
        self.map.setWKTProjection('WGS84')
        proj4 = self.map.getProjection()
        assert ('+proj=longlat' in proj4 and '+datum=WGS84' in proj4) or proj4 == '+init=epsg:4326', proj4
        assert (mapscript.projectionObj(proj4)).getUnits() != mapscript.MS_METERS


class MapRunSubTestCase(MapTestCase):

    def testDefaultSubstitutions(self):
        s = """
MAP
    WEB
        VALIDATION
            'key1' '.*'
            'default_key1' 'Test Title'
        END
        METADATA
            "wms_title" "%key1%"
        END
    END
END
        """
        map = mapscript.fromstring(s)
        map.applyDefaultSubstitutions()
        assert map.web.metadata["wms_title"] == "Test Title"

    def testRuntimeSubstitutions(self):
        """
        For supported parameters see https://mapserver.org/cgi/runsub.html#parameters-supported
        """
        s = """
MAP
    WEB
        VALIDATION
            'key1' '.*'
            'default_key1' 'Test Title'
        END
        METADATA
            "wms_title" "%key1%"
        END
    END
    LAYER
        TYPE POINT
        FILTER ([size]<%my_filter%)
        VALIDATION
            'my_filter' '^[0-9]$'
        END
    END
END
        """
        map = mapscript.fromstring(s)

        d = {"key1": "New Title", "my_filter": "3"}
        map.applySubstitutions(d)

        assert map.web.metadata["wms_title"] == "New Title"
        assert map.getLayer(0).getFilterString() == "([size]<3)"


class MapSLDTestCase(MapTestCase):
    def test(self):

        test_map = mapscript.mapObj(TESTMAPFILE)
        result = test_map.generateSLD()
        assert result.startswith('<StyledLayerDescriptor version="1.0.0"'), result

        result = test_map.generateSLD("1.1.0")
        assert result.startswith('<StyledLayerDescriptor version="1.1.0"'), result


if __name__ == '__main__':
    unittest.main()
