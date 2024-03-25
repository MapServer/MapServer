# Project:  MapServer
# Purpose:  xUnit style Python mapscript tests of OWS Requests
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
import xml.dom.minidom

import mapscript

from .testing import MapTestCase


class OWSRequestTestCase(MapTestCase):
    def testInit(self):
        """OWSRequestTestCase.testInit: OWS initializer works right"""
        request = mapscript.OWSRequest()
        request.setParameter("BBOX", "-0.3, 51.2, 0.3, 51.8")
        assert request.NumParams == 1
        assert request.getName(0) == "BBOX"
        assert request.getValue(0) == "-0.3, 51.2, 0.3, 51.8"

    def testGetParameter(self):
        """OWSRequestTestCase.testGetParameter: OWS can get request parameters by index"""
        request = mapscript.OWSRequest()
        request.setParameter("foo", "bar")
        assert request.getValue(0) == "bar"

    def testGetParameterByName(self):
        """OWSRequestTestCase.testGetParameterByName: OWS can get request parameters by name"""
        request = mapscript.OWSRequest()
        request.setParameter("foo", "bar")
        assert request.getValueByName("Foo") == "bar"

    def testResetParam(self):
        """OWSRequestTestCase.testResetParam: OWS can reset parameters by name"""
        request = mapscript.OWSRequest()
        request.setParameter("foo", "bar")
        assert request.NumParams == 1
        request.setParameter("Foo", "bra")
        assert request.NumParams == 1
        assert request.getValue(0) == "bra"

    def testLoadWMSRequest(self):
        """OWSRequestTestCase.testLoadWMSRequest: OWS can load a WMS request"""
        request = mapscript.OWSRequest()
        request.setParameter("REQUEST", "GetMap")
        request.setParameter("VERSION", "1.1.0")
        request.setParameter("FORMAT", "image/png")
        request.setParameter("LAYERS", "POINT")
        request.setParameter("BBOX", "-0.30, 51.20, 0.30, 51.80")
        request.setParameter("SRS", "EPSG:4326")
        request.setParameter("HEIGHT", "60")
        request.setParameter("WIDTH", "60")
        request.setParameter("STYLES", "")
        for i in range(self.map.numlayers):
            self.map.getLayer(i).status = mapscript.MS_ON
        status = self.map.loadOWSParameters(request)
        assert status == mapscript.MS_SUCCESS, status
        self.assertEqual(self.map.height, 60)
        self.assertEqual(self.map.width, 60)
        self.assertEqual(self.map.getProjection(), "+init=epsg:4326")
        # MapServer extents are from middle of the pixel
        self.assertAlmostEqual(self.map.extent.minx, -0.295)
        self.assertAlmostEqual(self.map.extent.miny, 51.205)
        self.assertAlmostEqual(self.map.extent.maxx, 0.295)
        self.assertAlmostEqual(self.map.extent.maxy, 51.795)
        img = self.map.draw()
        img.save("test_load_ows_request.png")

    def testWFSPostRequest(self):
        """OWSRequestTestCase.testLoadWMSRequest: OWS can POST a WFS request"""

        self.map.web.metadata.set("ows_onlineresource", "http://dummy.org/")
        request = mapscript.OWSRequest()
        request.contenttype = "application/xml"

        post_data = """<wfs:GetFeature xmlns:wfs="http://www.opengis.net/wfs" xmlns:ogc="http://www.opengis.net/ogc" service="WFS"
        version="1.1.0" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
        <wfs:Query typeName="*:POINT" xmlns:feature="http://www.openplans.org/topp">
            <ogc:Filter>
                <ogc:PropertyIsEqualTo>
                    <ogc:PropertyName>FID</ogc:PropertyName>
                    <ogc:Literal>1</ogc:Literal>
                </ogc:PropertyIsEqualTo>
            </ogc:Filter>
        </wfs:Query>
        </wfs:GetFeature>
        """

        qs = ""  # additional parameters can be passed via the querystring
        request.loadParamsFromPost(post_data, qs)
        mapscript.msIO_installStdoutToBuffer()
        status = self.map.OWSDispatch(request)
        assert status == mapscript.MS_SUCCESS, status

        mapscript.msIO_stripStdoutBufferContentHeaders()
        result = mapscript.msIO_getStdoutBufferBytes()
        dom = xml.dom.minidom.parseString(result)
        assert len(dom.getElementsByTagName("ms:POINT")) == 1


if __name__ == "__main__":
    unittest.main()
