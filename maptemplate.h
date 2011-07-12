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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.14  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.13  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.12  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef MAPTEMPLATE_H
#define MAPTEMPLATE_H

#include "map.h"
#include "maphash.h"

#define IDPATTERN "^[0-9A-Za-z]{1,63}$"
#define IDSIZE 64
#define TEMPLATE_TYPE(s)  (((strncmp("http://", s, 7) == 0) || (strncmp("https://", s, 8) == 0) || (strncmp("ftp://", s, 6)) == 0)  ? MS_URL : MS_FILE)
#define MAXZOOM 25
#define MINZOOM -25

enum coordSources {NONE, FROMIMGPNT, FROMIMGBOX, FROMIMGSHAPE, FROMREFPNT, FROMUSERPNT, FROMUSERBOX, FROMUSERSHAPE, FROMBUF, FROMSCALE};

enum modes {BROWSE, ZOOMIN, ZOOMOUT, MAP, LEGEND, REFERENCE, SCALEBAR, COORDINATE, 
            QUERY, QUERYMAP, NQUERY, NQUERYMAP, 
            ITEMQUERY, ITEMQUERYMAP, ITEMNQUERY, ITEMNQUERYMAP, 
            FEATUREQUERY, FEATUREQUERYMAP, FEATURENQUERY, FEATURENQUERYMAP, 
            ITEMFEATUREQUERY, ITEMFEATUREQUERYMAP,ITEMFEATURENQUERY,ITEMFEATURENQUERYMAP,
            INDEXQUERY,INDEXQUERYMAP};

/*! \srtuct mapservObj
 *  \brief Global structur used by template and mapserv
 * 
 * This structur was created to seperate template functionality
 * from the main mapserv file. Instead of moving all template
 * related functions in a new file (maptemplate.c) and change
 * their signatures to pass all global variables, we created this
 * structure with all global variables needed by template.
*/
typedef struct
{
   /* should the query and/or map be saved */
   int SaveMap, SaveQuery;

  cgiRequestObj *request;

   mapObj *Map;

   char *Layers[MS_MAXLAYERS];
   
   /* number of layers specfied by a use */
   int NumLayers;

   layerObj *ResultLayer;
   
   int UseShapes; /* are results of a query to be used in calculating an extent of some sort */


   shapeObj SelectShape, ResultShape;

   rectObj RawExt;

   pointObj MapPnt;

   /* default for browsing */
   double fZoom, Zoom;
   
   /* whether zooming in or out, default is pan or 0 */
   int ZoomDirection; 

   /* can be BROWSE, QUERY, etc. */
   int Mode; 
   
   /* big enough for time + pid */
   char Id[IDSIZE]; 
   
   int CoordSource;
   double Scale; /* used to create a map extent around a point */

   int ImgRows, ImgCols;
   rectObj ImgExt; /* Existing image's mapextent */
   rectObj ImgBox;
   
   pointObj RefPnt;
   pointObj ImgPnt;

   double Buffer;


   
   /* 
    ** variables for multiple query results processing 
    */
   int RN; /* overall result number */
   int LRN; /* result number within a layer */
   int NL; /* total number of layers with results */
   int NR; /* total number or results */
   int NLR; /* number of results in a layer */
} mapservObj;
   
   

/*! \fn msAllocMapServObj
 * Allocate memory for all variables in strusture
 * and initiate default values
*/
MS_DLL_EXPORT mapservObj* msAllocMapServObj(void);

/*! \fn msFreeMapServObj
 * free all variables in structure
*/
MS_DLL_EXPORT void msFreeMapServObj(mapservObj* msObj);

/* For Mapserv.c */
MS_DLL_EXPORT int isOn(mapservObj* msObj, char *name, char *group);
MS_DLL_EXPORT int checkWebScale(mapservObj *msObj);
MS_DLL_EXPORT int setExtent(mapservObj *msObj);

MS_DLL_EXPORT int msReturnPage(mapservObj* msObj, char* , int, char **);
MS_DLL_EXPORT int msReturnURL(mapservObj* msObj, char*, int);
MS_DLL_EXPORT int msReturnQuery(mapservObj* msObj, char* pszMimeType, char **papszBuffer);

MS_DLL_EXPORT int msReturnTemplateQuery(mapservObj *msObj, char* pszMimeType, 
                          char **papszBuffer);

MS_DLL_EXPORT int msRedirect(char* url);

MS_DLL_EXPORT char *generateLegendTemplate(mapservObj *msObj);

MS_DLL_EXPORT int msGenerateImages(mapservObj *msObj, char *szQuery, int bReturnOnError);


MS_DLL_EXPORT char *msProcessTemplate(mapObj *map, int bGenerateImages, 
                         char **names, char **values, 
                         int numentries);

MS_DLL_EXPORT char *msProcessLegendTemplate(mapObj *map,
                              char **names, char **values, 
                              int numentries);

MS_DLL_EXPORT char *msProcessQueryTemplate(mapObj *map,
                             int bGenerateImages, 
                             char **names, char **values, 
                             int numentries);

#endif



