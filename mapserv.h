/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations supporting mapserv.c.
 * Author:   Steve Lime and the MapServer team.
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

#ifndef MAPSERV_H
#define MAPSERV_H

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#endif

#include <ctype.h>
#include <time.h>

#include "maptemplate.h"
#include "maptile.h"

#include "cgiutil.h"

/*
** Defines
*/
#define NUMEXP "[-]?(([0-9]+)|([0-9]*[.][0-9]+)([eE][-+]?[0-9]+)?)"
#define EXTENT_PADDING .05

/*
** Macros
*/
#define TEMPLATE_TYPE(s)                                                       \
  (((strncmp("http://", s, 7) == 0) || (strncmp("https://", s, 8) == 0) ||     \
    (strncmp("ftp://", s, 6)) == 0)                                            \
       ? MS_URL                                                                \
       : MS_FILE)

MS_DLL_EXPORT void msCGIWriteError(mapservObj *mapserv);
MS_DLL_EXPORT mapObj *msCGILoadMap(mapservObj *mapserv, configObj *context);
int msCGISetMode(mapservObj *mapserv);
int msCGILoadForm(mapservObj *mapserv);
int msCGIDispatchBrowseRequest(mapservObj *mapserv);
int msCGIDispatchCoordinateRequest(mapservObj *mapserv);
int msCGIDispatchQueryRequest(mapservObj *mapserv);
int msCGIDispatchImageRequest(mapservObj *mapserv);
int msCGIDispatchLegendRequest(mapservObj *mapserv);
int msCGIDispatchLegendIconRequest(mapservObj *mapserv);
MS_DLL_EXPORT int msCGIDispatchRequest(mapservObj *mapserv);

MS_DLL_EXPORT int msCGIIsAPIRequest(mapservObj *mapserv);
MS_DLL_EXPORT int msCGIDispatchAPIRequest(mapservObj *mapserv);

#endif /* MAPSERV_H */
