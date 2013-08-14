import mapscript

map = mapscript.mapObj('../../tests/test.map')
map.setMetaData( "ows_onlineresource", "http://dummy.org/" )
ows_req = mapscript.OWSRequest()

ows_req.type = mapscript.MS_GET_REQUEST

ows_req.setParameter( "SERVICE", "WMS" );
ows_req.setParameter( "VERSION", "1.1.0" );
ows_req.setParameter( "REQUEST", "GetCapabilities" );

mapscript.msIO_installStdoutToBuffer()
dispatch_status = map.OWSDispatch(ows_req)
if dispatch_status:
    status = '200 OK'
else:
    status = '500 Internal Server Error'
content_type = mapscript.msIO_stripStdoutBufferContentType()
mapscript.msIO_stripStdoutBufferContentHeaders()
result = mapscript.msIO_getStdoutBufferBytes()

try:
    # MapServer 6.0:
    mapscript.msCleanup()
except:
    # MapServer 6.1:
    mapscript.msCleanup(1)

response_headers = [('Content-Type', content_type),
                    ('Content-Length', str(len(result)))]
