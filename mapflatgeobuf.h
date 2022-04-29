/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  FlatGeobuf access API
 * Author:   Björn Harrtell.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef MAPFLATGEOBUF_H
#define MAPFLATGEOBUF_H

#include <stdio.h>
#include "mapprimitive.h"
#include "mapproject.h"

#include "cpl_vsi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SWIG
  typedef unsigned char uchar;

  typedef struct {
    VSILFILE  *fpFGB;

    int   nRecords;
    int   nMaxRecords;

    int   *panRecOffset;
    int   *panRecSize;
    ms_bitarray panRecLoaded;
    int   panRecAllLoaded;

    double  adBoundsMin[4];
    double  adBoundsMax[4];

    int   bUpdated;

    int   nBufSize; /* these used static vars in shape readers, moved to be thread-safe */
    uchar   *pabyRec;
    int   nPartMax;
    int   *panParts;

  } FGBInfo;
  typedef FGBInfo * FGBHandle;
#endif

  /************************************************************************/
  /*                          flatgeobufObj                                */
  /************************************************************************/

/**
 * An object representing a FlatGeobuf. There is no write access to this object
 * using MapScript.
 */
  typedef struct {
#ifdef SWIG
    %immutable;
#endif

    int numshapes; ///< Number of shapes
    rectObj bounds; ///< Extent of shapes

#ifndef SWIG
    char source[MS_PATH_LENGTH]; /* full path to this file data */
    int lastshape;
    ms_bitarray status;
    int isopen;
    FGBHandle hFGB; /* FlatGeobuf file pointer */
#endif

  } flatgeobufObj;

#ifndef SWIG
  MS_DLL_EXPORT int msFlatGeobufOpenHandle(flatgeobufObj *fgbfile, const char *filename, FGBHandle hFGB);
  MS_DLL_EXPORT int msFlatGeobufOpenVirtualFile(flatgeobufObj *fgbfile, const char *filename, VSILFILE * fpFGB, int log_failures);
  MS_DLL_EXPORT int msFlatGeobufOpen(flatgeobufObj *fgbfile, const char *filename, int log_failures);
  MS_DLL_EXPORT void msFlatGeobufClose(flatgeobufObj *fgbfile);
  MS_DLL_EXPORT int msFlatGeobufWhichShapes(flatgeobufObj *fgbfile, rectObj rect, int debug);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPFLATGEOBUF_H */
