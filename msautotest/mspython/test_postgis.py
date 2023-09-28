#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Regression test for mappostgis
# Author:   Even Rouault
#
###############################################################################
#  Copyright (c) 2020, Even Rouault,<even.rouault at spatialys.com>
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
###############################################################################

import sys

sys.path.append("../pymod")
import pytest

mapscript_available = False
try:
    import mapscript

    mapscript_available = True
except ImportError:
    pass


def is_postgis_db_installed():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            NAME mylayer
            DATA "the_geom from (select * from multipolygon3d order by id) as foo using srid=27700 using unique id"
            TYPE POLYGON
        END
        """
    )
    try:
        layer.getNumFeatures()
        return True
    except mapscript.MapServerError:
        return False


pytestmark = [
    pytest.mark.skipif(not mapscript_available, reason="mapscript not available"),
    pytest.mark.skipif(
        mapscript_available and "INPUT=POSTGIS" not in mapscript.msGetVersion(),
        reason="PostGIS support missing",
    ),
    pytest.mark.skipif(
        mapscript_available
        and "INPUT=POSTGIS" in mapscript.msGetVersion()
        and not is_postgis_db_installed(),
        reason="PostGIS 'msautotest' database not setup",
    ),
]

###############################################################################
#


def test_postgis_numfeatures():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            NAME mylayer
            DATA "the_geom from (select * from multipolygon3d order by id) as foo using srid=27700 using unique id"
            TYPE POLYGON
        END
        """
    )
    numfeatures = layer.getNumFeatures()
    assert numfeatures == 1


###############################################################################
#


def test_postgis_invalid_dbname():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=invalid_dbname"
            NAME mylayer
            DATA "the_geom from (select * from multipolygon3d order by id) as foo using srid=27700 using unique id"
            TYPE POLYGON
        END
        """
    )
    with pytest.raises(mapscript.MapServerError):
        layer.getNumFeatures()


###############################################################################
#


def test_postgis_missing_data():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            NAME mylayer
            TYPE POLYGON
        END
        """
    )
    with pytest.raises(mapscript.MapServerError):
        layer.getNumFeatures()


###############################################################################
#


@pytest.mark.parametrize(
    "data",
    [
        "the_geom from",
        "the_geom from non_existing_table",
        "the_geom from (select * from multipolygon3d)",  # missing as foo
        "the_geom from (select * from multipolygon3d) as foo using unique",  # missing value for unique
        "the_geom from (select * from multipolygon3d) as foo using unique id using srid=",  # missing value for srid=
        "the_geom from (select * from non_existing_table order by id) as foo using srid=27700 using unique id",
        "the_geom from (select * from multipolygon3d order by id) using srid=27700 using unique id",  # missing as foo
        "the_geom from (select * from multipolygon3d order by id) as foo using srid=27700",  # no using unique but subselect used
    ],
)
def test_postgis_invalid_data(data):

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            NAME mylayer
            DATA "%s"
            TYPE POLYGON
        END
        """
        % data
    )
    with pytest.raises(mapscript.MapServerError):
        layer.getNumFeatures()


###############################################################################
#


@pytest.mark.parametrize(
    "data",
    [
        "the_geom from multipolygon3d",
        "the_geom from (select * from multipolygon3d) as foo using unique id",
        "the_geom FROM (SELECT * FROM multipolygon3d) AS foo USING UNIQUE id",
        "the_geom from multipolygon3d using unique id",
        "the_geom from multipolygon3d using srid=27700",
        "the_geom from multipolygon3d using srid=27700 using unique id",
        "the_geom from (select * from multipolygon3d) as foo using unique non_existing_column",
    ],
)
def test_postgis_valid_data(data):

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            NAME mylayer
            DATA "%s"
            TYPE POLYGON
        END
        """
        % data
    )
    assert layer.getNumFeatures() == 1


###############################################################################
#


def test_postgis_box_substitution_getExtent():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from multilinestring3d WHERE ST_Intersects(wkb_geometry,!BOX!) as foo using unique id"
            NAME mylayer
            TYPE LINE
        END
        """
    )
    extent = layer.getExtent()
    assert extent.minx == 1
    assert extent.miny == 2
    assert extent.maxx == 10
    assert extent.maxy == 11


###############################################################################
#


def test_postgis_box_substitution_getNumFeatures():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from multilinestring3d WHERE ST_Intersects(wkb_geometry,!BOX!) as foo using unique id"
            NAME mylayer
            TYPE LINE
        END
        """
    )
    assert layer.getNumFeatures() == 1


###############################################################################
#


def test_postgis_box_bind_values_getExtent():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from province where name_e = $1) as foo using unique gid"
            NAME mylayer
            BINDVALS
                    "1"  "Nova Scotia"
            END
            TYPE POLYGON
        END
        """
    )

    extent = layer.getExtent()
    assert extent.minx < extent.maxx
    assert extent.miny < extent.maxy


###############################################################################
#


def test_postgis_box_bind_values_getNumFeatures():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from province where name_e = $1) as foo using unique gid"
            NAME mylayer
            BINDVALS
                    "1"  "Nova Scotia"
            END
            TYPE POLYGON
        END
        """
    )

    assert layer.getNumFeatures() == 15


###############################################################################
#


def test_postgis_box_bind_values_queryByRect():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from province where name_e = $1) as foo using unique gid"
            NAME mylayer
            BINDVALS
                    "1"  "Nova Scotia"
            END
            TYPE POLYGON
            TEMPLATE "junk.tmpl"
        END
        """
    )

    rect = mapscript.rectObj()
    rect.minx = -1e7
    rect.miny = -1e7
    rect.maxx = 1e7
    rect.maxy = 1e7

    layer.open()
    layer.queryByRect(map, rect)

    count = 0
    for i in range(1000):
        result = layer.getResult(i)
        if result is None:
            break
        count += 1
    assert count == 15


###############################################################################
# Test fix for https://github.com/MapServer/MapServer/pull/5520


def test_postgis_queryByFilter_bad_filteritem():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from province order by gid) as foo using unique gid"
            NAME mylayer
            TYPE POLYGON
            TEMPLATE "junk.tmpl"
            FILTER 'Cape Breton Island'
            FILTERITEM 'bad_filter_item'
        END
        """
    )

    layer.open()
    try:
        layer.queryByFilter(map, "1 = 1")
        assert False
    except mapscript.MapServerError:
        pass

    # Check that original filter and filteritem are properly restored
    assert layer.getFilterString() == '"Cape Breton Island"'
    assert layer.filteritem == "bad_filter_item"


###############################################################################
#


def test_postgis_queryByFilter_bad_expression():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from province order by gid) as foo using unique gid"
            NAME mylayer
            TYPE POLYGON
            TEMPLATE "junk.tmpl"
        END
        """
    )

    layer.open()
    try:
        layer.queryByFilter(map, "ERROR")
    except mapscript.MapServerError:
        pass


###############################################################################
#


def test_postgis_trim_char_fields():

    map = mapscript.mapObj()
    layer = mapscript.layerObj(map)
    layer.updateFromString(
        """
        LAYER
            CONNECTIONTYPE postgis
            CONNECTION "dbname=msautotest user=postgres"
            DATA "the_geom from (select * from text_datatypes order by id) as foo using srid=27700 using unique id"
            NAME mylayer
            TYPE POINT
            TEMPLATE "junk.tmpl"
        END
        """
    )

    layer.open()
    rect = mapscript.rectObj()
    rect.minx = -1000
    rect.maxx = 1000
    rect.miny = -1000
    rect.maxy = 1000

    layer.queryByRect(map, rect)
    result = layer.getResult(0)
    assert result is not None
    s = layer.getShape(result)
    assert s is not None
    assert [
        s.getValue(i)
        for i in filter(lambda i: layer.getItem(i) == "mychar5", range(layer.numitems))
    ] == ["abc"]
    assert [
        s.getValue(i)
        for i in filter(
            lambda i: layer.getItem(i) == "myvarchar5", range(layer.numitems)
        )
    ] == ["def  "]
    assert [
        s.getValue(i)
        for i in filter(lambda i: layer.getItem(i) == "mytext", range(layer.numitems))
    ] == ["ghi "]
