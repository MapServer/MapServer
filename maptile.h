/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapServer Tile Access API
 * Author:   Paul Ramsey <pramsey@cleverelephant.ca>
 *
 ******************************************************************************
 * Copyright (c) 2008, Paul Ramsey
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

#include "mapserver.h"
#include "maptemplate.h"

#define USE_TILE_API 1

#define SPHEREMERC_PROJ4 "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +units=m +k=1.0 +nadgrids=@null"
#define SPHEREMERC_GROUND_SIZE (20037508.34*2)
#define SPHEREMERC_IMAGE_SIZE 0x0100

enum tileModes { TILE_GMAP, TILE_VE };

MS_DLL_EXPORT int msTileSetup(mapservObj *msObj);
MS_DLL_EXPORT int msTileSetExtent(mapservObj *msObj);
MS_DLL_EXPORT int msTileSetProjections(mapObj *map);
MS_DLL_EXPORT imageObj* msTileDraw(mapservObj *msObj);

typedef struct {
  int metatile_level; /* In zoom levels above tile request: best bet is 0, 1 or 2 */
  int tile_width; /* In pixels */
  int tile_height; /* In pixels */
  int map_edge_buffer; /* In pixels */
} tileParams;


