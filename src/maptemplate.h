/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Template processing related declarations.
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

#ifndef MAPTEMPLATE_H
#define MAPTEMPLATE_H

#include "mapserver.h"
#include "maphash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IDPATTERN "^[0-9A-Za-z]{1,63}$"
#define IDSIZE 64
#define TEMPLATE_TYPE(s)                                                       \
  (((strncmp("http://", s, 7) == 0) || (strncmp("https://", s, 8) == 0) ||     \
    (strncmp("ftp://", s, 6)) == 0)                                            \
       ? MS_URL                                                                \
       : MS_FILE)
#define MAXZOOM 25
#define MINZOOM -25
#define DEFAULT_DATE_FORMAT "%d/%b/%Y:%H:%M:%S %z"

enum coordSources {
  NONE,
  FROMIMGPNT,
  FROMIMGBOX,
  FROMIMGSHAPE,
  FROMREFPNT,
  FROMUSERPNT,
  FROMUSERBOX,
  FROMUSERSHAPE,
  FROMBUF,
  FROMSCALE,
  FROMTILE
};

enum modes {
  BROWSE,
  ZOOMIN,
  ZOOMOUT,
  MAP,
  LEGEND,
  LEGENDICON,
  REFERENCE,
  SCALEBAR,
  COORDINATE,
  QUERY,
  NQUERY,
  ITEMQUERY,
  ITEMNQUERY,
  FEATUREQUERY,
  FEATURENQUERY,
  ITEMFEATUREQUERY,
  ITEMFEATURENQUERY,
  INDEXQUERY,
  TILE,
  OWS,
  WFS,
  MAPLEGEND,
  MAPLEGENDICON
};

/* struct mapservObj
 * Global structure used by templates and mapserver CGI interface.
 *
 * This structur was created to seperate template functionality
 * from the main mapserv file. Instead of moving all template
 * related functions in a new file (maptemplate.c) and change
 * their signatures to pass all global variables, we created this
 * structure with all global variables needed by template.
 */
typedef struct {
  /* should the query and/or map be saved */
  int savemap, savequery;

  cgiRequestObj *request;

  int sendheaders; /* should mime-type header be output, default will be MS_TRUE
                    */

  mapObj *map;

  char **Layers;
  char *icon; /* layer:class combination that defines a legend icon */

  int NumLayers; /* number of layers specfied by a use */
  int MaxLayers; /* Allocated size of Layers[] array */

  layerObj *resultlayer;

  int UseShapes; /* are results of a query to be used in calculating an extent
                    of some sort */

  shapeObj resultshape;

  rectObj RawExt;

  pointObj mappnt;

  double fZoom, Zoom;
  int ZoomDirection; /* whether zooming in or out, default is pan or 0 */

  int Mode; /* can be BROWSE, QUERY, etc. */

  int TileMode;     /* can be GMAP, VE */
  char *TileCoords; /* for GMAP: 0 0 1; for VE: 013021023 */
  int TileWidth;
  int TileHeight;

  char Id[IDSIZE]; /* big enough for time + pid */

  int CoordSource;
  double ScaleDenom; /* used to create a map extent around a point */

  int ImgRows, ImgCols;
  rectObj ImgExt; /* Existing image's mapextent */
  rectObj ImgBox;

  pointObj RefPnt;
  pointObj ImgPnt;

  double Buffer;

  int SearchMap; /* apply pan/zoom BEFORE doing the query (e.g. query the output
                    image rather than the input image) */

  char *QueryFile;
  char *QueryLayer;
  char *SelectLayer;
  int QueryLayerIndex;
  int SelectLayerIndex;

  char *QueryItem;
  char *QueryString;

  int ShapeIndex;
  int TileIndex;

  int QueryCoordSource;

  int ZoomSize; /* zoom absolute magnitude (i.e. > 0) */

  /*
  ** variables for multiple query results processing
  */
  int RN;  /* overall result number */
  int LRN; /* result number within a layer */
  int NL;  /* total number of layers with results */
  int NR;  /* total number or results */
  int NLR; /* number of results in a layer */

  map_hittest *hittest;

} mapservObj;

/*! \fn msAllocMapServObj
 * Allocate memory for all variables in strusture
 * and initiate default values
 */
MS_DLL_EXPORT mapservObj *msAllocMapServObj(void);

/*! \fn msFreeMapServObj
 * free all variables in structure
 */
MS_DLL_EXPORT void msFreeMapServObj(mapservObj *msObj);

/* For Mapserv.c */
MS_DLL_EXPORT int isOn(mapservObj *msObj, char *name, char *group);
MS_DLL_EXPORT int checkWebScale(mapservObj *msObj);
MS_DLL_EXPORT int setExtent(mapservObj *msObj);

MS_DLL_EXPORT int msReturnPage(mapservObj *msObj, char *, int, char **);
MS_DLL_EXPORT int msReturnURL(mapservObj *msObj, const char *, int);
MS_DLL_EXPORT int msReturnNestedTemplateQuery(mapservObj *msObj,
                                              char *pszMimeType,
                                              char **papszBuffer);
MS_DLL_EXPORT int msReturnTemplateQuery(mapservObj *msObj, char *pszMimeType,
                                        char **papszBuffer);
MS_DLL_EXPORT int msReturnOpenLayersPage(mapservObj *mapserv);

MS_DLL_EXPORT int msRedirect(const char *url);

MS_DLL_EXPORT char *generateLegendTemplate(mapservObj *msObj);

MS_DLL_EXPORT int msGenerateImages(mapservObj *msObj, int bQueryMap,
                                   int bReturnOnError);

MS_DLL_EXPORT char *msProcessTemplate(mapObj *map, int bGenerateImages,
                                      char **names, char **values,
                                      int numentries);

MS_DLL_EXPORT char *msProcessLegendTemplate(mapObj *map, char **names,
                                            char **values, int numentries);

MS_DLL_EXPORT char *msProcessQueryTemplate(mapObj *map, int bGenerateImages,
                                           char **names, char **values,
                                           int numentries);

MS_DLL_EXPORT int msGrowMapservLayers(mapservObj *msObj);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
