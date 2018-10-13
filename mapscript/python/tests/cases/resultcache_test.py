# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of ResultCache
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


class LayerQueryTestCase(MapTestCase):

    def setUp(self):
        MapTestCase.setUp(self)
        self.layer = self.map.getLayer(1)
        self.layer.template = 'some day i will fix this!'

    def pointquery(self):
        p = mapscript.pointObj(0.0, 51.5)
        self.layer.queryByPoint(self.map, p, mapscript.MS_MULTIPLE, -1)
        return self.layer.getResults()

    def indexquery(self):
        self.layer.queryByIndex(self.map, -1, 0, mapscript.MS_TRUE)
        return self.layer.getResults()


class ResultCacheTestCase(LayerQueryTestCase):

    def testNoDirectAccess(self):
        """denying direct access to layer's resultcache"""
        self.assertRaises(AttributeError, getattr, self.layer, 'resultcache')

    def testNullResultCache(self):
        """before a query layer's resultcache should be NULL"""
        results = self.layer.getResults()
        assert results is None


class PointQueryResultsTestCase(LayerQueryTestCase):

    def testCacheAfterQuery(self):
        """simple point query returns one result"""
        results = self.pointquery()
        assert results.numresults == 1

    def testCacheAfterFailedQuery(self):
        """point query with no results returns NULL"""
        p = mapscript.pointObj(0.0, 0.0)
        self.layer.queryByPoint(self.map, p, mapscript.MS_MULTIPLE, -1)
        results = self.layer.getResults()
        assert results is None

    def testDeletionOfResults(self):
        """deleting results should not harm the layer"""
        results = self.pointquery()
        assert results.thisown == 0
        del results
        results = self.layer.getResults()
        assert results.numresults == 1

    def testQueryResultBounds(self):
        """result bounds should equal layer bounds"""
        results = self.pointquery()
        e = self.layer.getExtent()
        self.assertRectsEqual(results.bounds, e)

    def testQueryResultMembers(self):
        """get the single result member"""
        results = self.pointquery()
        self.layer.open()
        res = results.getResult(0)
        feature = self.layer.getShape(res)
        self.layer.close()
        self.assertRectsEqual(results.bounds, feature.bounds)
        assert feature.getValue(1) == 'A Polygon'

    def testQueryResultsListComp(self):
        """get all results using list comprehension"""
        results = self.pointquery()
        result_list = [results.getResult(i) for i in range(results.numresults)]
        assert len(result_list) == 1
        assert result_list[0].shapeindex == 0

    def testQueryByIndex(self):
        """pop a result into the result set"""
        self.layer.queryByIndex(self.map, -1, 0, mapscript.MS_FALSE)
        results = self.layer.getResults()
        assert results.numresults == 1
        self.layer.queryByIndex(self.map, -1, 0, mapscript.MS_TRUE)
        results = self.layer.getResults()
        assert results.numresults == 2


class DumpAndLoadTestCase(LayerQueryTestCase):

    def testSaveAndLoadQuery(self):
        """test saving query to a file"""
        results = self.pointquery()
        self.map.saveQuery('test.qy')
        self.map.freeQuery()
        results = self.layer.getResults()
        assert results is None
        self.map.loadQuery('test.qy')
        results = self.layer.getResults()
        assert results is not None


class LayerOffQueryTestCase(LayerQueryTestCase):

    def testPointQueryOffLayer(self):
        """simple point query returns one result, even if layer is off"""
        self.layer.status = mapscript.MS_OFF
        results = self.pointquery()
        assert results.numresults == 1

    def testIndexQueryOffLayer(self):
        """simple index query returns one result, even if layer is off"""
        self.layer.status = mapscript.MS_OFF
        results = self.indexquery()
        assert results.numresults == 1


if __name__ == '__main__':
    unittest.main()
