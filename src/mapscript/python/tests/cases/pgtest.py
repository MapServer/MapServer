# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of PostGIS Layer
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
#     python tests/cases/pgtest.py -v
#
# NOTE: these tests use a fixture included in mapserver/tests. To produce
# the testing database, follow these steps:
#
# $ cd mapserver/tests
# $ shp2pgsql -s 4326 -I polygon.shp postgis.polygon > polygon.sql
# $ su postgres
# $ make -f makefile_postgis
#
# ===========================================================================

import unittest

import mapscript

PG_CONNECTION_STRING = "dbname=mapserver_test user=postgres"


class TableTest(unittest.TestCase):
    def setUp(self):
        self.mo = mapscript.mapObj()
        lo = mapscript.layerObj()
        lo.name = "pg_layer"
        lo.type = mapscript.MS_LAYER_POLYGON
        lo.connectiontype = mapscript.MS_POSTGIS
        lo.connection = PG_CONNECTION_STRING
        lo.data = "the_geom from polygon"
        li = self.mo.insertLayer(lo)
        self.lo = self.mo.getLayer(li)

    def test_getfeature(self):
        self.lo.open()
        f = self.lo.getFeature(1, -1)
        atts = [f.getValue(i) for i in range(self.lo.numitems)]
        self.lo.close()
        self.assertEqual(atts, ["1", "1", "A Polygon"], atts)


class ViewTest(unittest.TestCase):
    def setUp(self):
        self.mo = mapscript.mapObj()
        lo = mapscript.layerObj()
        lo.name = "pg_sub_layer"
        lo.type = mapscript.MS_LAYER_POLYGON
        lo.connectiontype = mapscript.MS_POSTGIS
        lo.connection = PG_CONNECTION_STRING
        lo.data = "the_geom from polygon_v using unique gid using srid=4326"
        li = self.mo.insertLayer(lo)
        self.lo = self.mo.getLayer(li)

    def test_getfeature(self):
        self.lo.open()
        f = self.lo.getFeature(1, -1)
        atts = [f.getValue(i) for i in range(self.lo.numitems)]
        self.lo.close()
        self.assertEqual(atts, ["1", "1", "A Polygon"], atts)


class SubSelectTest(unittest.TestCase):
    def setUp(self):
        self.mo = mapscript.mapObj()
        lo = mapscript.layerObj()
        lo.name = "pg_sub_layer"
        lo.type = mapscript.MS_LAYER_POLYGON
        lo.connectiontype = mapscript.MS_POSTGIS
        lo.connection = PG_CONNECTION_STRING
        lo.data = "the_geom from (select * from polygon) as foo using unique gid using srid=4326"
        li = self.mo.insertLayer(lo)
        self.lo = self.mo.getLayer(li)

    def test_getfeature(self):
        self.lo.open()
        f = self.lo.getFeature(1, -1)
        atts = [f.getValue(i) for i in range(self.lo.numitems)]
        self.lo.close()
        self.assertEqual(atts, ["1", "1", "A Polygon"], atts)


class SubSelectNoSRIDTest(unittest.TestCase):
    """SRID should be diagnosed from the selection"""

    def setUp(self):
        self.mo = mapscript.mapObj()
        lo = mapscript.layerObj()
        lo.name = "pg_sub_layer"
        lo.type = mapscript.MS_LAYER_POLYGON
        lo.connectiontype = mapscript.MS_POSTGIS
        lo.connection = PG_CONNECTION_STRING
        lo.data = "the_geom from (select * from polygon) as foo using unique gid"
        li = self.mo.insertLayer(lo)
        self.lo = self.mo.getLayer(li)

    def test_getfeature(self):
        self.lo.open()
        f = self.lo.getFeature(1, -1)
        atts = [f.getValue(i) for i in range(self.lo.numitems)]
        self.lo.close()
        self.assertEqual(atts, ["1", "1", "A Polygon"], atts)


if __name__ == "__main__":
    unittest.main()
