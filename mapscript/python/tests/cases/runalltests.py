# $Id$
#
# Project:  MapServer
# Purpose:  Comprehensive xUnit style Python mapscript test runner
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
# Execute this script from mapserver/mapscript/python/tests/cases
#
#     $ cd tests/cases
#     $ python runalltests.py
#
# It runs all tests found in Python modules ending with "test.py" in the
# tests/cases directory.
# ===========================================================================

import getopt
import os
import sys
import unittest

verbosity = 1
try:
    opts, args = getopt.getopt(sys.argv[1:], 'v')
    if opts[0][0] == '-v':
        verbosity = verbosity + 1
except:
    pass

runner = unittest.TextTestRunner(verbosity=verbosity)
suite = unittest.TestSuite()
load = unittest.defaultTestLoader.loadTestsFromModule

tests = os.listdir(os.curdir)
tests = [n[:-3] for n in tests if n.endswith('test.py')]

for test in tests:
    m = __import__(test)
    suite.addTest(load(m))

# ---------------------------------------------------------------------------
# Run
if __name__ == '__main__':
    runner.run(suite)

