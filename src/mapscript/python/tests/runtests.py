# $Id$
#
# Project:  MapServer
# Purpose:  Comprehensive xUnit style Python mapscript test suite
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
#     python tests/runtests.py -v
#
# ===========================================================================

import unittest

# Import test cases
import cases

from cases.recttest import RectObjTestCase

from cases.hashtest import HashTableTestCase, WebMetadataTestCase
from cases.hashtest import LayerMetadataTestCase, ClassMetadataTestCase

from cases.owstest import OWSRequestTestCase
from cases.clonetest import MapCloningTestCase

from cases.imagetest import ImageObjTestCase, ImageWriteTestCase

from cases.maptest import MapConstructorTestCase
from cases.maptest import MapLayersTestCase
from cases.maptest import MapExtentTestCase
from cases.maptest import MapExceptionTestCase
from cases.maptest import EmptyMapExceptionTestCase
from cases.maptest import MapSizeTestCase

from cases.layertest import LayerConstructorTestCase
from cases.layertest import LayerExtentTestCase
from cases.layertest import LayerRasterProcessingTestCase
from cases.layertest import LayerTestCase
from cases.layertest import RemoveLayerTestCase

from cases.zoomtest import ZoomPointTestCase
from cases.zoomtest import ZoomRectangleTestCase
from cases.zoomtest import ZoomScaleTestCase

from cases.linetest import LineObjTestCase
from cases.shapetest import ShapePointTestCase
from cases.pointtest import PointObjTestCase
from cases.shapetest import InlineFeatureTestCase

from cases.styletest import DrawProgrammedStylesTestCase
from cases.styletest import NewStylesTestCase
from cases.styletest import BrushCachingTestCase
from cases.colortest import ColorObjTestCase

from cases.symboltest import SymbolTestCase
from cases.symboltest import MapSymbolTestCase

from cases.symbolsettest import SymbolSetTestCase
from cases.symbolsettest import MapSymbolSetTestCase

from cases.outputformattest import OutputFormatTestCase
from cases.outputformattest import MapOutputFormatTestCase

from cases.labeltest import LabelCacheMemberTestCase

from cases.resultcachetest import ResultCacheTestCase
from cases.resultcachetest import PointQueryResultsTestCase
from cases.resultcachetest import DumpAndLoadTestCase

# Create a test suite
suite = unittest.TestSuite()

# Add tests to the suite
suite.addTests([RectObjTestCase()])

suite.addTests([HashTableTestCase(), WebMetadataTestCase(),
                LayerMetadataTestCase(), ClassMetadataTestCase()])

suite.addTests([MapCloningTestCase()])
suite.addTests([OWSRequestTestCase()])

suite.addTests([ImageObjTestCase(), ImageWriteTestCase()])

suite.addTests([MapConstructorTestCase(),
                MapLayersTestCase(),
                MapExtentTestCase(),
                MapExceptionTestCase(),
                EmptyMapExceptionTestCase()])

suite.addTests([LayerConstructorTestCase(),
                LayerExtentTestCase(),
                LayerRasterProcessingTestCase(),
                RemoveLayerTestCase()])

suite.addTests([LineObjTestCase(),
                ShapePointTestCase(),
                PointObjTestCase(),
                InlineFeatureTestCase()])


suite.addTests([DrawProgrammedStylesTestCase(),
                NewStylesTestCase(), BrushCachingTestCase(),
                ColorObjTestCase()])
                
suite.addTests([SymbolTestCase(),
                MapSymbolTestCase()])

suite.addTests([SymbolSetTestCase(),
                MapSymbolSetTestCase()])

suite.addTests([ZoomPointTestCase(),
                ZoomRectangleTestCase(),
                ZoomScaleTestCase()])
                
suite.addTests([OutputFormatTestCase(), MapOutputFormatTestCase()])

suite.addTest(LabelCacheMemberTestCase())

suite.addTests([ResultCacheTestCase(), PointQueryResultsTestCase(),
                DumpAndLoadTestCase()])

# ============================================================================
# If module is run as a script, execute every test case in the suite
if __name__ == '__main__':
    unittest.main()

