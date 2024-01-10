# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Test harnass for testing via Python MapScript.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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

from testlib import compare_result

keep_pass = 0


def get_item_value(layer, shape_obj, name):
    for i in range(layer.numitems):
        if layer.getItem(i) == name:
            return shape_obj.getValue(i)
    return None


###############################################################################
#


def check_items(layer, shape_obj, nv_list):
    for nv in nv_list:
        name = nv[0]
        expected_value = nv[1]

        actual_value = get_item_value(layer, shape_obj, name)
        assert actual_value is not None, "missing expected attribute %s" % name

        assert (
            str(expected_value) == actual_value
        ), 'attribute %s is "%s" instead of expected "%s"' % (
            name,
            actual_value,
            str(expected_value),
        )


###############################################################################
#


def compare_and_report(out_file, this_path="."):

    cmp = compare_result(out_file, this_path)

    if cmp == "match":
        if keep_pass == 0:
            os.remove(os.path.join(this_path, "result", out_file))

    elif cmp == "files_differ_image_match":
        if keep_pass == 0:
            os.remove(os.path.join(this_path, "result", out_file))
        print("result images match, though files differ.")

    elif cmp == "files_differ_image_nearly_match":
        if keep_pass == 0:
            os.remove(os.path.join(this_path, "result", out_file))
        print("result images perceptually match, though files differ.")

    elif cmp == "nomatch":
        assert False, "results dont match, TEST FAILED."

    elif cmp == "noresult":
        assert False, "no result file generated, TEST FAILED."

    elif cmp == "noexpected":
        print("no expected file exists, accepting result as expected.")
        os.rename(
            os.path.join(this_path, "result", out_file),
            os.path.join(this_path, "expected", out_file),
        )

    else:
        assert False, "unexpected cmp value: " + cmp
