# $Id$
#
# Project:  MapServer
# Purpose:  xUnit style Python mapscript testing utilities
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
# Purpose of this module is to export the locally built mapscript module
# prior to installation, do some name normalization that allows testing of
# the so-called next generation class names, and define some classes 
# useful to many test cases.
#
# All test case modules should import mapscript from testing
#
#     from .testing import mapscript
#
# ===========================================================================

import os
import sys
import sysconfig
import unittest

# define the path to mapserver test data
TESTS_PATH = '../../tests'

TESTMAPFILE = os.path.join(TESTS_PATH, 'test.map')
XMARKS_IMAGE = os.path.join(TESTS_PATH, 'xmarks.png')
TEST_IMAGE = os.path.join(TESTS_PATH, 'test.png')

# Put local build directory on head of python path
platformdir = '-'.join((sysconfig.get_platform(), 
                        '.'.join(map(str, sys.version_info[0:2]))))
sys.path.insert(0, os.path.join('build', 'lib.' + platformdir))

# import mapscript from the local build directory
import mapscript

# normalize names, allows testing of module that uses the experimental
# next generation names
classnames = [ 'mapObj', 'layerObj', 'classObj', 'styleObj', 'shapeObj',
            'lineObj', 'pointObj', 'rectObj', 'outputFormatObj', 'symbolObj',
            'symbolSetObj', 'colorObj', 'imageObj', 'shapefileObj',
            'projectionObj', 'fontSetObj', 'hashTableObj' ]

for name in classnames:
    try:
        new_name = name.replace('Obj', '')
        new_name = new_name.capitalize()
        new_object = getattr(mapscript, new_name)
        setattr(mapscript, name, new_object)
    except AttributeError:
        pass

