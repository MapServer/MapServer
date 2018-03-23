# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of hashTableObj
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
#     python tests/cases/hashtest.py -v
#
# ===========================================================================

import os, sys
import unittest

# the testing module helps us import the pre-installed mapscript
from .testing import mapscript, MapTestCase

# ===========================================================================
# Base class for hashtable tests.  Derived classes use these tests, but
# defined different values for self.table.

class HashTableBaseTestCase:

    keys = ['key1', 'key2', 'key3', 'key4']
    values = ['value1', 'value2', 'value3', 'value4']
        
    def testUseNonExistentKey(self):
        assert self.table.get('bogus') == None
    
    def testUseNonExistentKeyWithDefault(self):
        assert self.table.get('bogus', 'default') == 'default'

    def testGetValue(self):
        for key, value in zip(self.keys, self.values):
            assert self.table.get(key) == value
            assert self.table.get(key.upper()) == value
            assert self.table.get(key.capitalize()) == value

    def testGetValueWithDefault(self):
        for key, value in zip(self.keys, self.values):
            assert self.table.get(key, 'foo') == value
            assert self.table.get(key.upper(), 'foo') == value
            assert self.table.get(key.capitalize(), 'foo') == value
            
    def testClear(self):
        self.table.clear()
        for key in self.keys:
            assert self.table.get(key) == None

    def testRemoveItem(self):
        key = self.keys[0]
        self.table.remove(key)
        assert self.table.get(key) == None

    def testNextKey(self):
        key = self.table.nextKey()
        assert key == self.keys[0], key
        for i in range(1, len(self.keys)):
            key = self.table.nextKey(key)
            assert key == self.keys[i], key
        # We're at the end, next should be None
        key = self.table.nextKey(key)
        assert key == None, key

# TODO
#    def testKeys(self):
#        "get sequence of keys"
#        keys = self.table.keys()
#        assert keys == self.keys, keys
#
#    def testValues(self):
#        "get sequence of values"
#        values = self.table.values()
#        assert values == self.values, values


# ===========================================================================
# Test begins now

# ===========================================================================
# HashTable test case
#

class HashTableTestCase(unittest.TestCase, HashTableBaseTestCase):
    """Tests of the MapServer HashTable object"""

    def setUp(self):
        "our fixture is a HashTable with two items"
        self.table = mapscript.hashTableObj()
        for key, value in zip(self.keys, self.values):
            self.table.set(key, value)

    def tearDown(self):
        self.table = None

    def testConstructor(self):
        table = mapscript.hashTableObj()
        tabletype = type(table)
        assert str(tabletype) == "<class 'mapscript.hashTableObj'>", tabletype


# ==============================================================================
# following tests use the tests/test.map fixture

class WebMetadataTestCase(MapTestCase, HashTableBaseTestCase):

    def setUp(self):
        MapTestCase.setUp(self)
        self.table = self.map.web.metadata
        self.keys = ['key1', 'key2', 'key3', 'key4', 'ows_enable_request']
        self.values = ['value1', 'value2', 'value3', 'value4', '*']

    def tearDown(self):
        MapTestCase.tearDown(self)
        self.table = None


class LayerMetadataTestCase(WebMetadataTestCase):

    def setUp(self):
        MapTestCase.setUp(self)
        self.table = self.map.getLayer(1).metadata
       

class ClassMetadataTestCase(WebMetadataTestCase):

    def setUp(self):
        MapTestCase.setUp(self)
        self.table = self.map.getLayer(1).getClass(0).metadata        


# ===========================================================================
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
