# $Id$
#
# Tests of the HashTable object implemented in June 2004 by Sean Gillies
#
# See http://mapserver.gis.umn.edu/cgi-bin/wiki.pl?RefactoringMetadataObject
# and http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=726 for background
# other notes.

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
    
