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
from testing import mapscript

# ===========================================================================
# HashTable test case
#
# tests: 10
#

class HashTableTestCase(unittest.TestCase):
    """Tests of the MapServer HashTable object"""

    def setUp(self):
        "our fixture is a HashTable with two items"
        self.keys = ['key1', 'key2']
        self.values = ['value1', 'value2']
        self.table = mapscript.hashTableObj()
        for key, value in zip(self.keys, self.values):
            self.table.set(key, value)

    def tearDown(self):
        self.table = None

    def testConstructor(self):
        table = mapscript.hashTableObj()
        tabletype = type(table)
        assert str(tabletype) == "<class 'mapscript.hashTableObj'>", tabletype

    def testSanity(self):
        "sanity check"
        self.table.set('foo', 'bar')
        assert self.table.get('foo') == 'bar'

    def testUseNonExistentKey(self):
        "accessing non-existent key should raise exception"
        self.assertRaises(mapscript.MapServerError, self.table.get, 'bogus')

    def testGetValue(self):
        "get value at a key using either case"
        for key, value in zip(self.keys, self.values):
            assert self.table.get(key) == value
            assert self.table.get(key.upper()) == value
            assert self.table.get(key.capitalize()) == value

    def testClear(self):
        "clear table, expect exceptions"
        self.table.clear()
        for key in self.keys:
            self.assertRaises(mapscript.MapServerError, self.table.get, key)

    def testRemoveItem(self):
        "remove item and expect expection on access"
        key = self.keys[0]
        self.table.remove(key)
        self.assertRaises(mapscript.MapServerError, self.table.get, key)

    def testNextKey(self):
        key = self.table.nextKey()
        assert key == self.keys[0], key
        for i in range(1, len(self.keys)):
            key = self.table.nextKey(key)
            assert key == self.keys[1], key
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
# Run the tests outside of the main suite

if __name__ == '__main__':
    unittest.main()
    
