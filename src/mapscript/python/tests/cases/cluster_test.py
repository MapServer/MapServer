# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of clusterObj
# Author:   Seth Girvin
#
# ===========================================================================
# Copyright (c) 2019, Seth Girvin
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


class ClusterObjTestCase(unittest.TestCase):
    def testClusterObjUpdateFromString(self):
        """a cluster can be updated from a string"""
        c = mapscript.clusterObj()
        c.updateFromString("CLUSTER \n MAXDISTANCE 5 \n REGION \n 'rectangle' END")

        assert c.maxdistance == 5
        assert c.region == "rectangle"

        s = c.convertToString()
        assert s == 'CLUSTER\n  MAXDISTANCE 5\n  REGION "rectangle"\nEND # CLUSTER\n'

    def testClusterObjGetSetFilter(self):
        """a cluster filter can be set and read"""
        c = mapscript.clusterObj()
        filter = "[attr1] > 5"
        c.setFilter(filter)
        assert '"{}"'.format(filter) == c.getFilterString()

    def testClusterObjGetSetGroup(self):
        """a cluster filter can be set and read"""
        c = mapscript.clusterObj()
        exp = "100"  # TODO not sure what would be a relevant expression here
        c.setGroup(exp)
        assert '"{}"'.format(exp) == c.getGroupString()


if __name__ == "__main__":
    unittest.main()
