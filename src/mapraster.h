/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Private include file used by mapraster.c and maprasterquery.c
 * Author:   Even Rouault
 *
 ******************************************************************************
 * Copyright (c) 2013 Regents of the University of Minnesota.
 *
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
 *****************************************************************************/

#ifndef MAPRASTER_H
#define MAPRASTER_H


  int msDrawRasterSetupTileLayer(mapObj *map, layerObj *layer,
                                 rectObj* psearchrect,
                                 int is_query,
                                 int* ptilelayerindex, /* output */
                                 int* ptileitemindex, /* output */
                                 int* ptilesrsindex, /* output */
                                 layerObj **ptlp  /* output */ );

  void msDrawRasterCleanupTileLayer(layerObj* tlp,
                                    int tilelayerindex);

  int msDrawRasterIterateTileIndex(layerObj *layer,
                                   layerObj* tlp,
                                   shapeObj* ptshp, /* input-output */
                                   int tileitemindex,
                                   int tilesrsindex,
                                   char* tilename, /* output */
                                   size_t sizeof_tilename,
                                   char* tilesrsname, /* output */
                                   size_t sizeof_tilesrsname);

  int msDrawRasterBuildRasterPath(mapObj *map, layerObj *layer,
                                  const char* filename,
                                  char szPath[MS_MAXPATHLEN]);

  const char* msDrawRasterGetCPLErrorMsg(const char* decrypted_path,
                                         const char* szPath);

  int msDrawRasterLoadProjection(layerObj *layer,
                                 GDALDatasetH hDS,
                                 const char* filename,
                                 int tilesrsindex,
                                 const char* tilesrsname);

#endif /* MAPRASTER_H */
