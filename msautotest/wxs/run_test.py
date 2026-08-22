#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Test harnass script for MapServer autotest.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
###############################################################################

import os
import subprocess
import sys
import urllib.parse

import pytest

sys.path.append("../pymod")

import mstestlib


@pytest.mark.parametrize(
    "map,out_file,command",
    mstestlib.get_pytests(os.path.dirname(os.path.abspath(__file__))),
)
def test(map, out_file, command, extra_args):
    mstestlib.run_pytest(map, out_file, command, extra_args)


def test_wms_filter_operation_limit():
    test_dir = os.path.dirname(os.path.abspath(__file__))
    leaf = (
        "<PropertyIsEqualTo><PropertyName>NAME</PropertyName>"
        "<Literal>Charlottetown</Literal></PropertyIsEqualTo>"
    )
    request = urllib.parse.urlencode(
        {
            "map": "wms_filter.map",
            "SERVICE": "WMS",
            "VERSION": "1.3.0",
            "REQUEST": "GetMap",
            "CRS": "EPSG:4326",
            "BBOX": "40,-70,50,-60",
            "WIDTH": "10",
            "HEIGHT": "10",
            "LAYERS": "popplace",
            "STYLES": "",
            "FORMAT": "image/png",
            "EXCEPTIONS": "XML",
            "FILTER": "<Filter><Or>" + leaf * 300 + "</Or></Filter>",
        }
    )
    env = os.environ.copy()
    env.update(
        {
            "REQUEST_METHOD": "POST",
            "QUERY_STRING": "",
            "CONTENT_TYPE": "application/x-www-form-urlencoded",
            "CONTENT_LENGTH": str(len(request)),
            "MAPSERVER_CONFIG_FILE": os.path.abspath(
                os.path.join(test_dir, "..", "etc", "mapserv.conf")
            ),
        }
    )

    result = subprocess.run(
        ["mapserv"],
        input=request,
        text=True,
        capture_output=True,
        cwd=test_dir,
        env=env,
        check=False,
    )

    assert result.returncode == 0
    assert "InvalidParameterValue" in result.stdout


###############################################################################
# main()

if __name__ == "__main__":
    sys.exit(mstestlib.pytest_main())
