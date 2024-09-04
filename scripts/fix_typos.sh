#!/bin/sh
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
#  Project:  GDAL
#  Purpose:  (Interactive) script to identify and fix typos
#  Author:   Even Rouault <even.rouault at spatialys.com>
#
###############################################################################
#  Copyright (c) 2016, Even Rouault <even.rouault at spatialys.com>
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

set -eu

SCRIPT_DIR=$(dirname "$0")
case $SCRIPT_DIR in
    "/"*)
        ;;
    ".")
        SCRIPT_DIR=$(pwd)
        ;;
    *)
        SCRIPT_DIR=$(pwd)"/"$(dirname "$0")
        ;;
esac
GDAL_ROOT=$SCRIPT_DIR/..
cd "$GDAL_ROOT"

if ! test -d fix_typos; then
    # Get our fork of codespell that adds --words-white-list and full filename support for -S option
    mkdir fix_typos
    (cd fix_typos
     git clone https://github.com/rouault/codespell
     (cd codespell && git checkout gdal_improvements)
     # Aggregate base dictionary + QGIS one + Debian Lintian one
     curl https://raw.githubusercontent.com/qgis/QGIS/master/scripts/spell_check/spelling.dat | sed "s/:/->/" | sed "s/:%//" | grep -v "colour->" | grep -v "colours->" > qgis.txt
     curl https://salsa.debian.org/lintian/lintian/-/raw/master/data/spelling/corrections | grep "||" | grep -v "#" | sed "s/||/->/" > debian.txt
     cat codespell/data/dictionary.txt qgis.txt debian.txt | awk 'NF' > gdal_dict.txt
     echo "difered->deferred" >> gdal_dict.txt
     echo "differed->deferred" >> gdal_dict.txt
     grep -v 404 < gdal_dict.txt > gdal_dict.txt.tmp
     mv gdal_dict.txt.tmp gdal_dict.txt
    )
fi

EXCLUDED_FILES="**/.git/*,*.pdf,*.svg,*.phar,*.dat*,./scripts/fix_typos.sh,src/mapscript/java/makefile.vc,src/mapscript/csharp/Makefile.vc,src/opengl/*,msautotest/api/test.json,msautotest/api/out.html,msautotest/php/maps/etc/vera/COPYRIGHT.TXT"
AUTHORIZED_LIST="CPL_SUPRESS_CPLUSPLUS,unpreciseMathCall,koordinates,msMetadataFreeParmsObj,metres,kilometre,kilometres,iColorParm,FO,Cartesian,msOWSPrintLatLonBoundingBox,msWFSFreeParmsObj,TeGenerateArc,TeGeometryAlgorithm,msProjectHasLonWrapOrOver,msProjectHasLonWrap,pdfLonWrap,dfLonWrap,LatLonBox,latLonBoxNode,ESPACE,MSUVRASTER_LON,needsLonLat,reprojectorToLonLat,lon,bHasLonWrap,LatLonBoundingBox,GEOSBUF_JOIN_MITRE,HB_VERSION_ATLEAST,msBuildRequestParms,E_GIF_ERR_NOT_WRITEABLE,WriteableBitmap,RUN_PARMS"

python3 fix_typos/codespell/codespell.py -w -i 3 -q 2 -S "$EXCLUDED_FILES,./build*/*" \
    -x scripts/typos_allowlist.txt --words-white-list=$AUTHORIZED_LIST \
    -D ./fix_typos/gdal_dict.txt src msautotest
