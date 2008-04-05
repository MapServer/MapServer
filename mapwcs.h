/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OpenGIS Web Coverage Server (WCS) Declarations.
 * Author:   Steve Lime, Frank Warmerdam and the MapServer Team
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

#ifndef MAPWCS_H
#define MAPWCS_H

#include "mapserver.h"
#include "mapowscommon.h"

/*
** Structure to hold metadata taken from the image or image tile index
*/
typedef struct {
  const char *srs;
  char srs_urn[500];
  rectObj extent, llextent;
  double geotransform[6];
  int xsize, ysize;
  double xresolution, yresolution;
  int bandcount; 
  int imagemode;
} coverageMetadataObj;

typedef struct {
  char *version;		/* 1.0.0 for now */
  char *updatesequence;		/* string, int or timestampe */
  char *request;		/* GetCapabilities|DescribeCoverage|GetCoverage */
  char *service;		/* MUST be WCS */
  char *section;		/* of capabilities document: /WCS_Capabilities/Service|/WCS_Capabilities/Capability|/WCS_Capabilities/ContentMetadata */
  char **coverages;		/* NULL terminated list of coverages (in the case of a GetCoverage there will only be 1) */
  char *crs;	        /* request coordinate reference system */
  char *response_crs;	/* response coordinate reference system */
  rectObj bbox;		    /* subset bounding box (3D), although we'll only use 2D */
  char *time;
  long width, height, depth;	/* image dimensions */
  double originx, originy;      /* WCS 1.1 GridOrigin */
  double resx, resy, resz;      /* resolution */
  char *interpolation;          /* interpolationMethod */
  char *format;
  char *exceptions;		/* exception MIME type, (default application=vnd.ogc.se_xml) */
} wcsParamsObj;

/* -------------------------------------------------------------------- */
/*      Prototypes from mapwcs.c used in mapwcs11.c.                    */
/*                                                                      */
/*      Note, all prototypes are deliberately not exported from DLL     */
/*      since they are for internal use within the core.                */
/* -------------------------------------------------------------------- */
void msWCSFreeParams(wcsParamsObj *params);
int msWCSException(mapObj *map, const char *code, const char *locator, 
                   const char *version);
int msWCSIsLayerSupported(layerObj *layer);
int msWCSGetCoverageMetadata( layerObj *layer, coverageMetadataObj *cm );
void msWCSSetDefaultBandsRangeSetInfo( wcsParamsObj *params,
                                       coverageMetadataObj *cm,
                                       layerObj *lp );
const char *msWCSGetRequestParameter(cgiRequestObj *request, char *name);

/* -------------------------------------------------------------------- */
/*      Some WCS 1.1 specific functions from mapwcs11.c                 */
/* -------------------------------------------------------------------- */
int msWCSGetCapabilities11(mapObj *map, wcsParamsObj *params, 
                           cgiRequestObj *req);
int msWCSDescribeCoverage11(mapObj *map, wcsParamsObj *params );
int msWCSReturnCoverage11( wcsParamsObj *params, mapObj *map, imageObj *image);
int msWCSGetCoverageBands11( mapObj *map, cgiRequestObj *request, 
                             wcsParamsObj *params, layerObj *lp,
                             char **p_bandlist );
int msWCSException11(mapObj *map, const char *locator, 
                     const char *exceptionCode, const char *version);

#endif /* nef MAPWCS_H */
