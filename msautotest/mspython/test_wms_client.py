import os

import pytest

mapscript_available = False
try:
    import mapscript

    mapscript_available = True
except ImportError:
    print("no")
    pass

pytestmark = [
    pytest.mark.skipif(not mapscript_available, reason="mapscript not available"),
    pytest.mark.skipif(
        "SUPPORTS=WMS_CLIENT" not in mapscript.msGetVersion(),
        reason="WMS Client is not enabled",
    ),
]


def get_relpath_to_this(filename):
    return os.path.join(os.path.dirname(__file__), filename)


###############################################################################
# Test that WMS client GetMap request does not modify layer extent or projection


def test_wms_client_does_not_modify_layer():
    map = mapscript.mapObj(get_relpath_to_this("wms_client.map"))

    layerObj = map.getLayer(0)
    assert layerObj.getExtent().minx == -180
    assert layerObj.getExtent().miny == -90
    assert layerObj.getExtent().maxx == 180
    assert layerObj.getExtent().maxy == 90
    assert layerObj.getProjection() == "+init=epsg:4326"

    # Do a GetMap request in EPSG:32633
    request = mapscript.OWSRequest()
    mapscript.msIO_installStdoutToBuffer()
    request.loadParamsFromURL(
        "&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=client&STYLES=&WIDTH=300&HEIGHT=200&FORMAT=image/png&CRS=EPSG:32633&BBOX=920482.6044580448652,7662266.090040001087,1317920.914512843126,8079762.906353433616"
    )
    status = map.OWSDispatch(request)

    assert status == 0

    # Assert that layer extent and projection is not modified
    assert layerObj.getExtent().minx == -180
    assert layerObj.getExtent().miny == -90
    assert layerObj.getExtent().maxx == 180
    assert layerObj.getExtent().maxy == 90
    assert layerObj.getProjection() == "+init=epsg:4326"
