# Project:  MapServer
# Purpose:  xUnit style Python mapscript test of multi-threading
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

import os
import threading
import unittest

import mapscript

from .testing import INCOMING, TESTMAPFILE


def draw_map(name, save=0):

    # print("making map in thread %s" % (name))
    mo = mapscript.mapObj(TESTMAPFILE)
    im = mo.draw()
    if save:
        im.save("threadtest_%s.png" % (name))


def trigger_exception(name):

    # print("triggering exception in thread %s" % (name))
    mo = mapscript.mapObj(TESTMAPFILE)
    try:
        mo.setExtent(1, 50, -1, 51)
        raise Exception("We expected a MapServer exception")
    except mapscript.MapServerError:
        pass


class MultipleThreadsTestCase(unittest.TestCase):
    def testDrawMultiThreads(self):
        """map drawing with multiple threads"""

        workers = []
        for i in range(10):
            name = "d%d" % (i)
            thread = threading.Thread(target=draw_map, name=name, args=(name, 1))
            workers.append(thread)
            thread.start()

    def testExceptionsMultiThreads(self):
        """mapserver exceptions behave with multiple threads"""

        workers = []
        for i in range(10):
            name = "e%d" % (i)
            thread = threading.Thread(target=trigger_exception, name=name, args=(name,))
            workers.append(thread)
            thread.start()

    def testExceptionContainmentMultiThreads(self):
        """mapserver exceptions should be contained to a thread"""
        num = 100
        workers = []

        # Trigger an exception in the first started thread
        for i in range(0, 1):
            name = "c%d" % (i)
            thread = threading.Thread(target=trigger_exception, name=name, args=(name,))
            workers.append(thread)

        # Draw normally
        for i in range(1, num):
            name = "c%d" % (i)
            thread = threading.Thread(target=draw_map, name=name, args=(name,))
            workers.append(thread)

        # Start all threads
        for i in range(num):
            workers[i].start()


def draw_map_wfs(name, save=0):

    # print("making map in thread %s" % (name))
    mo = mapscript.mapObj(TESTMAPFILE)

    # WFS layer
    lo = mapscript.layerObj()
    lo.name = "cheapo_wfs"
    lo.setProjection("+init=epsg:4326")
    lo.connectiontype = mapscript.MS_WFS
    lo.connection = "http://zcologia.com:9001/mapserver/members/features.rpy?"
    lo.metadata.set("wfs_service", "WFS")
    lo.metadata.set("wfs_typename", "users")
    lo.metadata.set("wfs_version", "1.0.0")
    lo.type = mapscript.MS_LAYER_POINT
    lo.status = mapscript.MS_DEFAULT
    lo.labelitem = "zco:mid"

    so1 = mapscript.styleObj()
    so1.color.setHex("#FFFFFF")
    so1.size = 9
    so1.symbol = 1  # mo.symbolset.index('circle')

    so2 = mapscript.styleObj()
    so2.color.setHex("#333333")
    so2.size = 7
    so2.symbol = 1  # mo.symbolset.index('circle')

    co = mapscript.classObj()
    co.label.type = mapscript.MS_BITMAP
    co.label.size = mapscript.MS_SMALL
    co.label.color.setHex("#000000")
    co.label.outlinecolor.setHex("#FFFFFF")
    co.label.position = mapscript.MS_AUTO

    co.insertStyle(so1)
    co.insertStyle(so2)
    lo.insertClass(co)
    mo.insertLayer(lo)

    if not mo.web.imagepath:
        mo.web.imagepath = os.environ.get("TEMP", None) or INCOMING
    mo.debug = mapscript.MS_ON
    im = mo.draw()
    if save:
        im.save("threadtest_wfs_%s.png" % (name))


def draw_map_wms(name, save=0):

    # print("making map in thread %s" % (name))
    mo = mapscript.mapObj(TESTMAPFILE)
    # WFS layer
    lo = mapscript.layerObj()
    lo.name = "jpl_wms"
    lo.setProjection("+init=epsg:4326")
    lo.connectiontype = mapscript.MS_WMS
    lo.connection = "http://vmap0.tiles.osgeo.org/wms/vmap0?"
    lo.metadata.set("wms_service", "WMS")
    lo.metadata.set("wms_server_version", "1.1.1")
    lo.metadata.set("wms_name", "basic")
    lo.metadata.set("wms_style", "visual")
    lo.metadata.set("wms_format", "image/jpeg")
    lo.type = mapscript.MS_LAYER_RASTER
    lo.status = mapscript.MS_DEFAULT
    lo.debug = mapscript.MS_ON
    mo.insertLayer(lo)

    if not mo.web.imagepath:
        mo.web.imagepath = os.environ.get("TEMP", None) or INCOMING
    mo.debug = mapscript.MS_ON
    mo.selectOutputFormat("image/jpeg")
    im = mo.draw()
    if save:
        im.save("threadtest_wms_%s.jpg" % (name))


class OWSRequestTestCase(unittest.TestCase):

    # def testDrawWFS(self):
    #    workers = []
    #    for i in range(10):
    #        name = 'd%d' % (i)
    #        thread = threading.Thread(target=draw_map_wfs, name=name, args=(name, 1))
    #        workers.append(thread)
    #        thread.start()

    def testDrawWMS(self):
        workers = []
        for i in range(10):
            name = "d%d" % (i)
            thread = threading.Thread(target=draw_map_wms, name=name, args=(name, 1))
            workers.append(thread)
            thread.start()


if __name__ == "__main__":
    unittest.main()
