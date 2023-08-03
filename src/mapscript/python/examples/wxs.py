#!/usr/bin/env python

"""
Output WMS GetCapabilities response for a Mapfile

Usage:

python wxs.py test.map

"""
import sys
import xml.dom.minidom
import mapscript


def main(map_file):

    map = mapscript.mapObj(map_file)
    map.web.metadata.set("ows_onlineresource", "http://dummy.org/")
    ows_req = mapscript.OWSRequest()

    ows_req.type = mapscript.MS_GET_REQUEST

    ows_req.setParameter("SERVICE", "WMS")
    ows_req.setParameter("VERSION", "1.1.0")
    ows_req.setParameter("REQUEST", "GetCapabilities")

    mapscript.msIO_installStdoutToBuffer()
    dispatch_status = map.OWSDispatch(ows_req)

    if dispatch_status != mapscript.MS_SUCCESS:
        print("An error occurred")

    content_type = mapscript.msIO_stripStdoutBufferContentType()
    mapscript.msIO_stripStdoutBufferContentHeaders()
    result = mapscript.msIO_getStdoutBufferBytes()

    # [('Content-Type', 'application/vnd.ogc.wms_xml; charset=UTF-8'), ('Content-Length', '11385')]
    response_headers = [('Content-Type', content_type),
                        ('Content-Length', str(len(result)))]

    assert int(response_headers[1][1]) > 0

    dom = xml.dom.minidom.parseString(result)
    print(dom.toprettyxml(indent="", newl=""))


def usage():
    """
    Display usage if program is used incorrectly
    """
    print("Syntax: %s <mapfile_path>" % sys.argv[0])
    sys.exit(2)


# make sure a filename argument is provided
if len(sys.argv) != 2:
    usage()

map_file = sys.argv[1]
main(map_file)
