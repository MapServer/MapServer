###############################################################################
# $Id: mstestlib.py 6763 2007-08-31 21:05:17Z warmerdam $
#
# Project:  MapServer
# Purpose:  Generic test machinery shared between mstestlib and pmstestlib.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
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

have_pdiff = None

###############################################################################
# strip_headers()


def strip_headers(filename):
    try:
        header = open(filename, "rb").read(1000)
    except Exception:
        return None

    from sys import version_info

    if version_info >= (3, 0, 0):
        header = str(header, "iso-8859-1")

    tmp_filename = None
    start_pos = header.find("\x49\x49\x2A\x00")
    if start_pos < 0:
        start_pos = header.find("\x49\x49\x00\x2A")
    if start_pos < 0:
        start_pos = header.find("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a")
    if start_pos > 0:
        f = open(filename, "rb")
        f.seek(start_pos, 0)
        data = f.read()
        f.close()
        tmp_filename = filename + ".tmp"
        f = open(tmp_filename, "wb")
        f.write(data)
        f.close()
        return tmp_filename

    return None


###################################################################
# Check image checksums with GDAL


def check_with_gdal(result_file, expected_file):

    from osgeo import gdal

    gdal.PushErrorHandler("CPLQuietErrorHandler")
    exp_ds = gdal.Open(expected_file)
    gdal.PopErrorHandler()
    if exp_ds == None:
        return "nomatch"

    res_ds = gdal.Open(result_file)

    match = 1
    for band_num in range(1, exp_ds.RasterCount + 1):
        if (
            res_ds.GetRasterBand(band_num).Checksum()
            != exp_ds.GetRasterBand(band_num).Checksum()
        ):
            match = 0

    if match == 1:

        # Special case for netCDF: we need to eliminate NC_GLOBAL#history
        # since it contains the current timedate
        if exp_ds.GetDriver().GetDescription() == "netCDF":
            if exp_ds.GetGeoTransform() != res_ds.GetGeoTransform():
                return "nomatch"
            if exp_ds.GetProjectionRef() != res_ds.GetProjectionRef():
                return "nomatch"
            exp_md = exp_ds.GetMetadata()
            if "NC_GLOBAL#history" in exp_md:
                del exp_md["NC_GLOBAL#history"]
            got_md = res_ds.GetMetadata()
            if "NC_GLOBAL#history" in got_md:
                del got_md["NC_GLOBAL#history"]
            if exp_md != got_md:
                return "nomatch"
            for band_num in range(1, exp_ds.RasterCount + 1):
                if (
                    res_ds.GetRasterBand(band_num).GetMetadata()
                    != exp_ds.GetRasterBand(band_num).GetMetadata()
                ):
                    return "nomatch"
            return "match"

        return "files_differ_image_match"

    return "nomatch"


###############################################################################
# compare_result()


def compare_result(filename, this_path="."):
    import filecmp

    result_file = os.path.join(this_path, "result", filename)
    expected_file = os.path.join(this_path, "expected", filename)

    try:
        os.stat(result_file)
    except OSError:
        return "noresult"

    try:
        os.stat(expected_file)
    except OSError:
        return "noexpected"

    # netCDF files contain a NC_GLOBAL#history that contains a datetime.
    # Binary comparison can't work
    if not expected_file.endswith(".nc"):
        if filecmp.cmp(expected_file, result_file, 0):
            return "match"

        expected_file_alternative = expected_file + ".alternative"
        if os.path.exists(expected_file_alternative):
            if filecmp.cmp(expected_file_alternative, result_file, 0):
                return "match"

        if expected_file[-4:] == ".xml":
            return "nomatch"

    ###################################################################
    # Check image checksums with GDAL if it is available.
    try:
        from osgeo import gdal

        gdal.VersionInfo(None)  # make pyflakes happy
        has_gdal = True
    except Exception:
        has_gdal = False

    if has_gdal:
        ret = check_with_gdal(result_file, expected_file)
        if ret != "nomatch":
            return ret

        expected_file_alternative = expected_file + ".alternative"
        if os.path.exists(expected_file_alternative):
            ret = check_with_gdal(result_file, expected_file_alternative)
            if ret != "nomatch":
                return ret

    ###################################################################
    # Test with perceptualdiff if this is tiff or png.  If we discover
    # we don't have it, then set have_pdiff to 'false' so we will know.

    global have_pdiff

    try:
        result = open(result_file, "rb").read(1000)
    except Exception:
        result = ""

    from sys import version_info

    if version_info >= (3, 0, 0):
        result = str(result, "iso-8859-1")

    run_perceptualdiff = have_pdiff != "false" and (
        "\x49\x49\x2A\x00" in result
        or "\x49\x49\x00\x2A" in result
        or "\x47\x49\x46\x38\x37\x61" in result
        or "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a" in result
    )

    if run_perceptualdiff:

        # Skip to real image if there's some HTTP headers before
        tmp_result_file = strip_headers(result_file)
        if tmp_result_file is not None:
            result_file = tmp_result_file
        tmp_expected_file = strip_headers(expected_file)
        if tmp_expected_file is not None:
            expected_file = tmp_expected_file

        try:
            cmd = "perceptualdiff %s %s -verbose > pd.out 2>&1 " % (
                result_file,
                expected_file,
            )
            os.system(cmd)
            pdout = open("pd.out").read()
            os.remove("pd.out")
        except Exception:
            pdout = ""
            pass

        if tmp_result_file is not None:
            os.remove(tmp_result_file)
        if tmp_expected_file is not None:
            os.remove(tmp_expected_file)

        if pdout.find("PASS:") != -1 and pdout.find("binary identical") != -1:
            return "files_differ_image_match"

        if pdout.find("PASS:") != -1 and pdout.find("indistinguishable") != -1:
            return "files_differ_image_nearly_match"

        if pdout.find("PASS:") == -1 and pdout.find("FAIL:") == -1:
            have_pdiff = "false"

    return "nomatch"
