# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of Map
# Author:   Seth Girvin, sgirvin@geographika.co.uk
#
# ===========================================================================
# Copyright (c) 2021, MapServer team
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
from .testing import TESTCONFIGFILE


class ConfigConstructorTestCase(unittest.TestCase):

    def testConfigConstructorFilenameArg(self):
        """ConfigConstructorTestCase.testConfigConstructorFilenameArg: map constructor with filename argument"""
        test_config = mapscript.configObj(TESTCONFIGFILE)
        assert test_config.__class__.__name__ == "configObj"
        assert test_config.thisown == 1


class ConfigHashTableTests(unittest.TestCase):

    def testGetEnv(self):

        test_config = mapscript.configObj(TESTCONFIGFILE)
        assert test_config.env.__class__.__name__ == "hashTableObj"
        assert test_config.env.numitems == 1

    def testGetMaps(self):

        test_config = mapscript.configObj(TESTCONFIGFILE)
        assert test_config.maps.__class__.__name__ == "hashTableObj"
        assert test_config.maps.numitems == 0

    def testGetPlugins(self):

        test_config = mapscript.configObj(TESTCONFIGFILE)
        assert test_config.plugins.__class__.__name__ == "hashTableObj"
        assert test_config.plugins.numitems == 0


if __name__ == '__main__':
    unittest.main()
