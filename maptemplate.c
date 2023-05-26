/******************************************************************************
 * $id: maptemplate.c 7725 2008-06-21 15:56:58Z sdlime $
 *
 * Project:  MapServer
 * Purpose:  Various template processing functions.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
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

#define NEED_IGNORE_RET_VAL

#include "maptemplate.h"
#include "maphash.h"
#include "mapserver.h"
#include "maptile.h"
#include "mapows.h"

#include "cpl_conv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <assert.h>
#include <ctype.h>


static inline void IGUR_sizet(size_t ignored) { (void)ignored; }  /* Ignore GCC Unused Result */
static inline void IGUR_voidp(void* ignored) { (void)ignored; }  /* Ignore GCC Unused Result */

static const char *const olUrl = "//www.mapserver.org/lib/OpenLayers-ms60.js";
static const char *const olTemplate = \
                          "<html>\n"
                          "<head>\n"
                          "<meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\">\n"
                          "  <title>MapServer Simple Viewer</title>\n"
                          "    <script type=\"text/javascript\" src=\"[openlayers_js_url]\"></script>\n"
                          "    <link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"//www.mapserver.org/_static/mapserver.ico\"/>\n"
                          "    </head>\n"
                          "    <body>\n"
                          "      <div style=\"width:[mapwidth]; height:[mapheight]\" id=\"map\"></div>\n"
                          "      <script defer=\"defer\" type=\"text/javascript\">\n"
                          "        var map = new OpenLayers.Map('map',\n"
                          "                                     {maxExtent: new OpenLayers.Bounds([minx],[miny],[maxx],[maxy]),\n"
                          "                                      maxResolution: [cellsize]});\n"
                          "        [openlayers_layer];\n"
                          "        map.addLayer(mslayer);\n"
                          "        map.zoomToMaxExtent();\n"
                          "      </script>\n"
                          "</body>\n"
                          "</html>";

static const char *const olLayerMapServerTag = \
                                   "var mslayer = new OpenLayers.Layer.MapServer( \"MapServer Layer\",\n"
                                   "                                              \"[mapserv_onlineresource]\",\n"
                                   "                                              {layers: '[layers]'},\n"
                                   "                                              {singleTile: \"true\", ratio:1} )";

static const char *const olLayerWMSTag = \
                             "var mslayer = new OpenLayers.Layer.WMS('MapServer Simple Viewer\',\n"
                             "                                   '[mapserv_onlineresource]',\n"
                             "                                   {layers: '[LAYERS]',\n"
                             "                                   bbox: '[minx],[miny],[maxx],[maxy]',\n"
                             "                                   width: [mapwidth], height: [mapheight], version: '[VERSION]', format:'[openlayers_format]'},"
                             "                                   {singleTile: \"true\", ratio:1, projection: '[openlayers_projection]'});\n";

static char *processLine(mapservObj *mapserv, const char *instr, FILE *stream, int mode);

static int isValidTemplate(FILE *stream, const char *filename)
{
  char buffer[MS_BUFFER_LENGTH];

  if(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) {
    if(!strcasestr(buffer, MS_TEMPLATE_MAGIC_STRING)) {
      msSetError(MS_WEBERR, "Missing magic string, %s doesn't look like a MapServer template.", "isValidTemplate()", filename);
      return MS_FALSE;
    }
  }

  return MS_TRUE;
}

/*
 * Redirect to (only use in CGI)
 *
*/
int msRedirect(char *url)
{
  msIO_setHeader("Status","302 Found");
  msIO_setHeader("Uri","%s",url);
  msIO_setHeader("Location","%s",url);
  msIO_setHeader("Content-Type","text/html");
  msIO_sendHeaders();
  return MS_SUCCESS;
}

/*
** Sets the map extent under a variety of scenarios.
*/
int setExtent(mapservObj *mapserv)
{
  double cellx,celly,cellsize;

  if(mapserv->Mode == TILE) {
    if(MS_SUCCESS != msTileSetExtent(mapserv)) {
      return MS_FAILURE;
    }
  }
  switch(mapserv->CoordSource) {
    case FROMUSERBOX: /* user passed in a map extent */
      break;
    case FROMIMGBOX: /* fully interactive web, most likely with java front end */
      cellx = MS_CELLSIZE(mapserv->ImgExt.minx, mapserv->ImgExt.maxx, mapserv->ImgCols);
      celly = MS_CELLSIZE(mapserv->ImgExt.miny, mapserv->ImgExt.maxy, mapserv->ImgRows);
      mapserv->map->extent.minx = MS_IMAGE2MAP_X(mapserv->ImgBox.minx, mapserv->ImgExt.minx, cellx);
      mapserv->map->extent.maxx = MS_IMAGE2MAP_X(mapserv->ImgBox.maxx, mapserv->ImgExt.minx, cellx);
      mapserv->map->extent.maxy = MS_IMAGE2MAP_Y(mapserv->ImgBox.miny, mapserv->ImgExt.maxy, celly); /* y's are flip flopped because img/map coordinate systems are */
      mapserv->map->extent.miny = MS_IMAGE2MAP_Y(mapserv->ImgBox.maxy, mapserv->ImgExt.maxy, celly);
      break;
    case FROMIMGPNT:
      cellx = MS_CELLSIZE(mapserv->ImgExt.minx, mapserv->ImgExt.maxx, mapserv->ImgCols);
      celly = MS_CELLSIZE(mapserv->ImgExt.miny, mapserv->ImgExt.maxy, mapserv->ImgRows);
      mapserv->mappnt.x = MS_IMAGE2MAP_X(mapserv->ImgPnt.x, mapserv->ImgExt.minx, cellx);
      mapserv->mappnt.y = MS_IMAGE2MAP_Y(mapserv->ImgPnt.y, mapserv->ImgExt.maxy, celly);

      mapserv->map->extent.minx = mapserv->mappnt.x - .5*((mapserv->ImgExt.maxx - mapserv->ImgExt.minx)/mapserv->fZoom); /* create an extent around that point */
      mapserv->map->extent.miny = mapserv->mappnt.y - .5*((mapserv->ImgExt.maxy - mapserv->ImgExt.miny)/mapserv->fZoom);
      mapserv->map->extent.maxx = mapserv->mappnt.x + .5*((mapserv->ImgExt.maxx - mapserv->ImgExt.minx)/mapserv->fZoom);
      mapserv->map->extent.maxy = mapserv->mappnt.y + .5*((mapserv->ImgExt.maxy - mapserv->ImgExt.miny)/mapserv->fZoom);
      break;
    case FROMREFPNT:
      cellx = MS_CELLSIZE(mapserv->map->reference.extent.minx, mapserv->map->reference.extent.maxx, mapserv->map->reference.width);
      celly = MS_CELLSIZE(mapserv->map->reference.extent.miny, mapserv->map->reference.extent.maxy, mapserv->map->reference.height);
      mapserv->mappnt.x = MS_IMAGE2MAP_X(mapserv->RefPnt.x, mapserv->map->reference.extent.minx, cellx);
      mapserv->mappnt.y = MS_IMAGE2MAP_Y(mapserv->RefPnt.y, mapserv->map->reference.extent.maxy, celly);

      mapserv->map->extent.minx = mapserv->mappnt.x - .5*(mapserv->ImgExt.maxx - mapserv->ImgExt.minx); /* create an extent around that point */
      mapserv->map->extent.miny = mapserv->mappnt.y - .5*(mapserv->ImgExt.maxy - mapserv->ImgExt.miny);
      mapserv->map->extent.maxx = mapserv->mappnt.x + .5*(mapserv->ImgExt.maxx - mapserv->ImgExt.minx);
      mapserv->map->extent.maxy = mapserv->mappnt.y + .5*(mapserv->ImgExt.maxy - mapserv->ImgExt.miny);
      break;
    case FROMBUF:
      mapserv->map->extent.minx = mapserv->mappnt.x - mapserv->Buffer; /* create an extent around that point, using the buffer */
      mapserv->map->extent.miny = mapserv->mappnt.y - mapserv->Buffer;
      mapserv->map->extent.maxx = mapserv->mappnt.x + mapserv->Buffer;
      mapserv->map->extent.maxy = mapserv->mappnt.y + mapserv->Buffer;
      break;
    case FROMSCALE:
      cellsize = (mapserv->ScaleDenom/mapserv->map->resolution)/msInchesPerUnit(mapserv->map->units,0); /* user supplied a point and a scale denominator */
      mapserv->map->extent.minx = mapserv->mappnt.x - cellsize*(mapserv->map->width-1)/2.0;
      mapserv->map->extent.miny = mapserv->mappnt.y - cellsize*(mapserv->map->height-1)/2.0;
      mapserv->map->extent.maxx = mapserv->mappnt.x + cellsize*(mapserv->map->width-1)/2.0;
      mapserv->map->extent.maxy = mapserv->mappnt.y + cellsize*(mapserv->map->height-1)/2.0;
      break;
    default: /* use the default in the mapfile if it exists */
      if((mapserv->map->extent.minx == mapserv->map->extent.maxx) && (mapserv->map->extent.miny == mapserv->map->extent.maxy)) {
        msSetError(MS_WEBERR, "No way to generate map extent.", "mapserv()");
        return MS_FAILURE;
      }
  }

  mapserv->RawExt = mapserv->map->extent; /* save unaltered extent */

  return MS_SUCCESS;
}

int checkWebScale(mapservObj *mapserv)
{
  int status;
  rectObj work_extent = mapserv->map->extent;

  mapserv->map->cellsize = msAdjustExtent(&(work_extent), mapserv->map->width, mapserv->map->height); /* we do this cause we need a scale */
  if((status = msCalculateScale(work_extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) return status;

  if((mapserv->map->scaledenom < mapserv->map->web.minscaledenom) && (mapserv->map->web.minscaledenom > 0)) {
    if(mapserv->map->web.mintemplate) { /* use the template provided */
      if(TEMPLATE_TYPE(mapserv->map->web.mintemplate) == MS_FILE) {
        if((status = msReturnPage(mapserv, mapserv->map->web.mintemplate, BROWSE, NULL)) != MS_SUCCESS) return status;
      } else {
        if((status = msReturnURL(mapserv, mapserv->map->web.mintemplate, BROWSE)) != MS_SUCCESS) return status;
      }
    } else { /* force zoom = 1 (i.e. pan) */
      mapserv->fZoom = mapserv->Zoom = 1;
      mapserv->ZoomDirection = 0;
      mapserv->CoordSource = FROMSCALE;
      mapserv->ScaleDenom = mapserv->map->web.minscaledenom;
      mapserv->mappnt.x = (mapserv->map->extent.maxx + mapserv->map->extent.minx)/2; /* use center of bad extent */
      mapserv->mappnt.y = (mapserv->map->extent.maxy + mapserv->map->extent.miny)/2;
      setExtent(mapserv);
      mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height);
      if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) return status;
    }
  } else {
    if((mapserv->map->scaledenom > mapserv->map->web.maxscaledenom) && (mapserv->map->web.maxscaledenom > 0)) {
      if(mapserv->map->web.maxtemplate) { /* use the template provided */
        if(TEMPLATE_TYPE(mapserv->map->web.maxtemplate) == MS_FILE) {
          if((status = msReturnPage(mapserv, mapserv->map->web.maxtemplate, BROWSE, NULL)) != MS_SUCCESS) return status;
        } else {
          if((status = msReturnURL(mapserv, mapserv->map->web.maxtemplate, BROWSE)) != MS_SUCCESS) return status;
        }
      } else { /* force zoom = 1 (i.e. pan) */
        mapserv->fZoom = mapserv->Zoom = 1;
        mapserv->ZoomDirection = 0;
        mapserv->CoordSource = FROMSCALE;
        mapserv->ScaleDenom = mapserv->map->web.maxscaledenom;
        mapserv->mappnt.x = (mapserv->map->extent.maxx + mapserv->map->extent.minx)/2; /* use center of bad extent */
        mapserv->mappnt.y = (mapserv->map->extent.maxy + mapserv->map->extent.miny)/2;
        setExtent(mapserv);
        mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height);
        if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) return status;
      }
    }
  }

  return MS_SUCCESS;
}

int msReturnTemplateQuery(mapservObj *mapserv, char *queryFormat, char **papszBuffer)
{
  imageObj *img = NULL;
  int i, status;

  outputFormatObj *outputFormat=NULL;
  mapObj *map = mapserv->map;

  if(!queryFormat) {
    msSetError(MS_WEBERR, "Return format/mime-type not specified.", "msReturnTemplateQuery()");
    return MS_FAILURE;
  }

  msApplyDefaultOutputFormats(map);

  i = msGetOutputFormatIndex(map, queryFormat); /* queryFormat can be a mime-type or name */
  if(i >= 0) outputFormat = map->outputformatlist[i];

  if(outputFormat) {
    if( MS_RENDERER_PLUGIN(outputFormat) ) {
      msInitializeRendererVTable(outputFormat);
    }
    if( MS_RENDERER_OGR(outputFormat) ) {
      checkWebScale(mapserv);

      status = msOGRWriteFromQuery(map, outputFormat, mapserv->sendheaders);

      return status;
    }

    if( !MS_RENDERER_TEMPLATE(outputFormat) ) { /* got an image format, return the query results that way */
      outputFormatObj *tempOutputFormat = map->outputformat; /* save format */

      checkWebScale(mapserv);

      map->outputformat = outputFormat; /* override what was given for IMAGETYPE */
      img = msDrawMap(map, MS_TRUE);
      if(!img) return MS_FAILURE;
      map->outputformat = tempOutputFormat; /* restore format */

      if(mapserv->sendheaders) {
        msIO_setHeader("Content-Type", "%s", MS_IMAGE_MIME_TYPE(outputFormat));
        msIO_sendHeaders();
      }
      status = msSaveImage(map, img, NULL);
      msFreeImage(img);

      return status;
    }
  }

  /*
  ** At this point we know we have a template of some sort, either the new style that references a or the old
  ** style made up of external files slammed together. Either way we may have to compute a query map and other
  ** images. We only create support images IF the querymap has status=MS_ON.
  */
  if(map->querymap.status) {
    checkWebScale(mapserv);
    if(msGenerateImages(mapserv, MS_TRUE, MS_TRUE) != MS_SUCCESS)
      return MS_FAILURE;
  }

  if(outputFormat) {
    const char *file = msGetOutputFormatOption( outputFormat, "FILE", NULL );
    if(!file) {
      msSetError(MS_WEBERR, "Template driver requires \"FILE\" format option be set.", "msReturnTemplateQuery()");
      return MS_FAILURE;
    }

    if(mapserv->sendheaders) {
      const char *attachment = msGetOutputFormatOption( outputFormat, "ATTACHMENT", NULL );
      if(attachment)
        msIO_setHeader("Content-disposition","attachment; filename=%s", attachment);
      msIO_setHeader("Content-Type", "%s", outputFormat->mimetype);
      msIO_sendHeaders();
    }
    if((status = msReturnPage(mapserv, (char *) file, BROWSE, papszBuffer)) != MS_SUCCESS)
      return status;
  } else {
    if((status = msReturnNestedTemplateQuery(mapserv, queryFormat, papszBuffer)) != MS_SUCCESS)
      return status;
  }

  return MS_SUCCESS;
}

/*
** Is a particular layer or group on, that is was it requested explicitly by the user.
*/
int isOn(mapservObj *mapserv, char *name, char *group)
{
  int i;

  for(i=0; i<mapserv->NumLayers; i++) {
    if(name && strcmp(mapserv->Layers[i], name) == 0)  return(MS_TRUE);
    if(group && strcmp(mapserv->Layers[i], group) == 0) return(MS_TRUE);
  }

  return(MS_FALSE);
}

/************************************************************************/
/*            int sortLayerByOrder(mapObj *map, char* pszOrder)         */
/*                                                                      */
/*      sorth the displaying in ascending or descending order.          */
/************************************************************************/
int sortLayerByOrder(mapObj *map, const char* pszOrder)
{
  int *panCurrentOrder = NULL;

  if(!map) {
    msSetError(MS_WEBERR, "Invalid pointer.", "sortLayerByOrder()");
    return MS_FAILURE;
  }
  /* ==================================================================== */
  /*      The flag "ascending" is in fact not useful since the            */
  /*      default ordering is ascending.                                  */
  /* ==================================================================== */

  /* -------------------------------------------------------------------- */
  /*      the map->layerorder should be set at this point in the          */
  /*      sortLayerByMetadata.                                            */
  /* -------------------------------------------------------------------- */
  if(map->layerorder) {
    panCurrentOrder = (int*)msSmallMalloc(map->numlayers * sizeof(int));
    for (int i=0; i<map->numlayers ; i++)
      panCurrentOrder[i] = map->layerorder[i];

    if(strcasecmp(pszOrder, "DESCENDING") == 0) {
      for (int i=0; i<map->numlayers; i++)
        map->layerorder[i] = panCurrentOrder[map->numlayers-1-i];
    }

    free(panCurrentOrder);
  }

  return MS_SUCCESS;
}


/*!
 * This function set the map->layerorder
 * index order by the metadata collumn name
*/
static int sortLayerByMetadata(mapObj *map, const char* pszMetadata)
{
  int nLegendOrder1;
  int nLegendOrder2;
  int i, j;
  int tmp;

  if(!map) {
    msSetError(MS_WEBERR, "Invalid pointer.", "sortLayerByMetadata()");
    return MS_FAILURE;
  }

  /*
   * Initiate to default order (Reverse mapfile order)
   */
  if(map->layerorder) {
    int *pnLayerOrder;

    /* Backup the original layer order to be able to reverse it */
    pnLayerOrder = (int*)msSmallMalloc(map->numlayers * sizeof(int));
    for (i=0; i<map->numlayers ; i++)
      pnLayerOrder[i] = map->layerorder[i];

    /* Get a new layerorder array */
    free(map->layerorder);
    map->layerorder = (int*)msSmallMalloc(map->numlayers * sizeof(int));

    /* Reverse the layerorder array */
    for (i=0; i<map->numlayers ; i++)
      map->layerorder[i] = pnLayerOrder[map->numlayers - i - 1];

    free(pnLayerOrder);
  } else {
    map->layerorder = (int*)msSmallMalloc(map->numlayers * sizeof(int));

    for (i=0; i<map->numlayers ; i++)
      map->layerorder[i] = map->numlayers - i - 1;
  }

  if(!pszMetadata)
    return MS_SUCCESS;

  /*
   * Bubble sort algo (not very efficient)
   * should implement a kind of quick sort
   * alog instead
  */
  for (i=0; i<map->numlayers-1; i++) {
    for (j=0; j<map->numlayers-1-i; j++) {
      const char* pszLegendOrder1 = msLookupHashTable(&(GET_LAYER(map, map->layerorder[j+1])->metadata), pszMetadata);
      const char* pszLegendOrder2 = msLookupHashTable(&(GET_LAYER(map, map->layerorder[j])->metadata), pszMetadata);

      if(!pszLegendOrder1 || !pszLegendOrder2)
        continue;

      nLegendOrder1 = atoi(pszLegendOrder1);
      nLegendOrder2 = atoi(pszLegendOrder2);

      if(nLegendOrder1 < nLegendOrder2) {  /* compare the two neighbors */
        tmp = map->layerorder[j];         /* swap a[j] and a[j+1]      */
        map->layerorder[j] = map->layerorder[j+1];
        map->layerorder[j+1] = tmp;
      }
    }
  }

  return MS_SUCCESS;
}

/*
** This function return a pointer
** at the begining of the first occurence
** of pszTag in pszInstr.
**
** Tag can be [TAG] or [TAG something]
*/
static const char *findTag(const char *pszInstr, const char *pszTag)
{
  const char* pszStart=NULL;
  int done=MS_FALSE;

  if(!pszInstr || !pszTag) {
    msSetError(MS_WEBERR, "Invalid pointer.", "findTag()");
    return NULL;
  }

  const int length = strlen(pszTag) + 1; /* adding [ character to the beginning */
  char* pszTag1 = (char*) msSmallMalloc(length+1);

  strcpy(pszTag1, "[");
  strcat(pszTag1, pszTag);

  const char* pszTemp = pszInstr;
  do {
    pszStart = strstr(pszTemp, pszTag1);

    if(pszStart == NULL)
      done = MS_TRUE; /* tag not found */
    else if((*(pszStart+length) == ']' || *(pszStart+length) == ' '))
      done = MS_TRUE; /* valid tag */
    else
      pszTemp += length; /* skip ahead and start over */
  } while(!done);

  free(pszTag1);

  return pszStart;
}

/*
** This function return a pointer
** to the end of the tag in pszTag
**
** The end of a tag is the next
** non-quoted ']' character.
** Return NULL if not found.
*/

char *findTagEnd(const char *pszTag)
{
  char *pszEnd = NULL,
        *pszTmp = (char*)pszTag;

  while (pszTmp != NULL) {
    if (*pszTmp == '"')
      pszTmp = strchr(pszTmp+1,'"');
    if ((pszTmp == NULL) || (*pszTmp == ']')) {
      pszEnd = pszTmp;
      pszTmp = NULL;
    } else
      pszTmp++;
  }

  return pszEnd;
}

/*
** Return a hashtableobj from instr of all
** arguments. hashtable must be freed by caller.
*/
static int getTagArgs(const char* pszTag, const char* pszInstr, hashTableObj **ppoHashTable)
{
  const char *pszStart, *pszEnd;
  int nLength;
  char **papszArgs, **papszVarVal;
  int nArgs, nDummy;
  int i;

  if(!pszTag || !pszInstr) {
    msSetError(MS_WEBERR, "Invalid pointer.", "getTagArgs()");
    return MS_FAILURE;
  }

  /* set position to the begining of tag */
  pszStart = findTag(pszInstr, pszTag);

  if(pszStart) {
    /* find ending position */
    pszEnd = findTagEnd(pszStart);

    if(pszEnd) {
      /* skip the tag name */
      pszStart = pszStart + strlen(pszTag) + 1;

      /* get length of all args */
      nLength = pszEnd - pszStart;

      if(nLength > 0) { /* is there arguments ? */
        char* pszArgs = (char*)msSmallMalloc(nLength + 1);
        strlcpy(pszArgs, pszStart, nLength+1);

        if(!(*ppoHashTable))
          *ppoHashTable = msCreateHashTable();

        /* put all arguments seperate by space in a hash table */
        papszArgs = msStringTokenize(pszArgs, " ", &nArgs, MS_TRUE);

        /* msReturnTemplateQuerycheck all argument if they have values */
        for (i=0; i<nArgs; i++) {
          if(strlen(papszArgs[i]) == 0) {
            free(papszArgs[i]);
            continue;
          }

          if(strchr(papszArgs[i], '=')) {
            papszVarVal = msStringTokenize(papszArgs[i], "=", &nDummy, MS_FALSE);
            msInsertHashTable(*ppoHashTable, papszVarVal[0],
                              papszVarVal[1]);
            free(papszVarVal[0]);
            free(papszVarVal[1]);
            free(papszVarVal);
          } else /* no value specified. set it to 1 */
            msInsertHashTable(*ppoHashTable, papszArgs[i], "1");

          free(papszArgs[i]);
        }
        free(papszArgs);
        free(pszArgs);
      }
    }
  }

  return MS_SUCCESS;
}

/*
** Return a substring from instr between [tag] and [/tag]
** char * returned must be freed by caller.
** pszNextInstr will be a pointer at the end of the
** first occurence found.
*/
static int getInlineTag(const char *pszTag, const char *pszInstr, char **pszResult)
{
  const char *pszEnd=NULL;
  int nInst=0;
  int nLength;

  *pszResult = NULL;

  if(!pszInstr || !pszTag) {
    msSetError(MS_WEBERR, "Invalid pointer.", "getInlineTag()");
    return MS_FAILURE;
  }

  char* pszEndTag = (char*)msSmallMalloc(strlen(pszTag) + 3);
  strcpy(pszEndTag, "[/");
  strcat(pszEndTag, pszTag);

  /* find start tag */
  const char* pszPatIn  = findTag(pszInstr, pszTag);
  const char* pszPatOut = strstr(pszInstr, pszEndTag);

  const char* pszStart = pszPatIn;

  const char* pszTmp = pszInstr;

  if(pszPatIn) {
    do {
      if(pszPatIn && pszPatIn < pszPatOut) {
        nInst++;

        pszTmp = pszPatIn;
      }

      if(pszPatOut && ((pszPatIn == NULL) || pszPatOut < pszPatIn)) {
        pszEnd = pszPatOut;
        nInst--;

        pszTmp = pszPatOut;
      }

      pszPatIn  = findTag(pszTmp+1, pszTag);
      pszPatOut = strstr(pszTmp+1, pszEndTag);

    } while (pszTmp != NULL && nInst > 0);
  }

  if(pszStart && pszEnd) {
    /* find end of start tag */
    pszStart = strchr(pszStart, ']');

    if(pszStart) {
      pszStart++;

      nLength = pszEnd - pszStart;

      if(nLength > 0) {
        *pszResult = (char*)msSmallMalloc(nLength + 1);

        /* copy string beetween start and end tag */
        strlcpy(*pszResult, pszStart, nLength+1);

        (*pszResult)[nLength] = '\0';
      }
    } else {
      msSetError(MS_WEBERR, "Malformed [%s] tag.", "getInlineTag()", pszTag);
      return MS_FAILURE;
    }
  }

  msFree(pszEndTag);

  return MS_SUCCESS;
}

/*!
 * this function process all if tag in pszInstr.
 * this function return a modified pszInstr.
 * ht mus contain all variables needed by the function
 * to interpret if expression.
 *
 * If bLastPass is true then all tests for 'null' values will be
 * considered true if the corresponding value is not set.
*/
int processIfTag(char **pszInstr, hashTableObj *ht, int bLastPass)
{
  /*   char *pszNextInstr = pszInstr; */
  const char *pszEnd=NULL;
  const char *pszName, *pszValue, *pszOperator, *pszHTValue;
  char *pszThen=NULL;
  char *pszIfTag;
  int nInst = 0;
  int nLength;

  hashTableObj *ifArgs=NULL;

  if(!*pszInstr) {
    msSetError(MS_WEBERR, "Invalid pointer.", "processIfTag()");
    return MS_FAILURE;
  }

  /* find the if start tag */

  const char* pszStart  = findTag(*pszInstr, "if");

  while (pszStart) {
    const char* pszPatIn  = findTag(pszStart, "if");
    const char* pszPatOut = strstr(pszStart, "[/if]");
    const char* pszTmp = pszPatIn;

    do {
      if(pszPatIn && pszPatIn < pszPatOut) {
        nInst++;
        pszTmp = pszPatIn;
      }

      if(pszPatOut && ((pszPatIn == NULL) || pszPatOut < pszPatIn)) {
        pszEnd = pszPatOut;
        nInst--;
        pszTmp = pszPatOut;
      }

      pszPatIn  = findTag(pszTmp+1, "if");
      pszPatOut = strstr(pszTmp+1, "[/if]");

    } while (nInst > 0);

    /* get the then string (if expression is true) */
    if(getInlineTag("if", pszStart, &pszThen) != MS_SUCCESS) {
      msSetError(MS_WEBERR, "Malformed then if tag.", "processIfTag()");
      return MS_FAILURE;
    }

    /* retrieve if tag args */
    if(getTagArgs("if", pszStart, &ifArgs) != MS_SUCCESS) {
      msSetError(MS_WEBERR, "Malformed args if tag.", "processIfTag()");
      return MS_FAILURE;
    }

    pszName = msLookupHashTable(ifArgs, "name");
    pszValue = msLookupHashTable(ifArgs, "value");
    pszOperator = msLookupHashTable(ifArgs, "oper");
    if(pszOperator == NULL) /* Default operator if not set is "eq" */
      pszOperator = "eq";

    int bEmpty = 0;

    if(pszName) {
      /* build the complete if tag ([if all_args]then string[/if]) */
      /* to replace if by then string if expression is true */
      /* or by a white space if not. */
      nLength = pszEnd - pszStart;
      pszIfTag = (char*)msSmallMalloc(nLength + 6);
      strlcpy(pszIfTag, pszStart, nLength+1);
      pszIfTag[nLength] = '\0';
      strcat(pszIfTag, "[/if]");

      pszHTValue = msLookupHashTable(ht, pszName);

      if(strcmp(pszOperator, "neq") == 0) {
        if(pszValue && pszHTValue && strcasecmp(pszValue, pszHTValue) != 0) {
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, pszThen);
        } else if(pszHTValue) {
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, "");
          bEmpty = 1;
        }
      } else if(strcmp(pszOperator, "eq") == 0) {
        if(pszValue && pszHTValue && strcasecmp(pszValue, pszHTValue) == 0) {
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, pszThen);
        } else if(pszHTValue) {
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, "");
          bEmpty = 1;
        }
      } else if(strcmp(pszOperator, "isnull") == 0) {
        if(pszHTValue != NULL) {
          /* We met a non-null value... condition is false */
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, "");
          bEmpty = 1;
        } else if(bLastPass) {
          /* On last pass, if value is still null then condition is true */
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, pszThen);
        }
      } else if(strcmp(pszOperator, "isset") == 0) {
        if(pszHTValue != NULL) {
          /* Found a non-null value... condition is true */
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, pszThen);
        } else if(bLastPass) {
          /* On last pass, if value still not set then condition is false */
          *pszInstr = msReplaceSubstring(*pszInstr, pszIfTag, "");
          bEmpty = 1;
        }
      } else {
        msSetError(MS_WEBERR, "Unsupported operator (%s) in if tag.",  "processIfTag()", pszOperator);
        return MS_FAILURE;
      }

      free(pszIfTag);
      pszIfTag = NULL;
    }

    if(pszThen)
      free (pszThen);

    pszThen=NULL;

    msFreeHashTable(ifArgs);
    ifArgs=NULL;

    /* find the if start tag */
    if(bEmpty)
      pszStart = findTag(pszStart, "if");
    else
      pszStart = findTag(pszStart + 1, "if");
  }

  return MS_SUCCESS;
}

/* Helper function to return the text before the supplied string2 in string1. */
static char *getPreTagText(const char *string1, const char *string2)
{
  int n;
  char *result, *tmpstr;

  if((tmpstr = strstr(string1, string2)) == NULL) return msStrdup(""); /* return an empty string */

  n = strlen(string1) - strlen(tmpstr);
  result = (char *) msSmallMalloc(n + 1);
  strlcpy(result, string1, n+1);

  return result;
}

/* Helper function to retunr the text after the supplied string2 in string1. */
static char *getPostTagText(const char *string1, const char *string2)
{
  char *tmpstr;

  if((tmpstr = strstr(string1, string2)) == NULL) return msStrdup(""); /* return an empty string */

  tmpstr += strlen(string2); /* skip string2 */
  return msStrdup(tmpstr);
}

/*
** Function to process a [feature ...] tag. This tag can *only* be found within
** a [resultset ...][/resultset] block.
*/
static int processFeatureTag(mapservObj *mapserv, char **line, layerObj *layer)
{
  char *preTag, *postTag; /* text before and after the tag */

  const char *argValue;
  char *tag, *tagInstance;
  hashTableObj *tagArgs=NULL;

  int limit=-1;
  const char *trimLast=NULL;

  int i, j, status;

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processFeatureTag()");
    return(MS_FAILURE);
  }

  const char* tagStart = findTag(*line, "feature");
  if(!tagStart) return(MS_SUCCESS); /* OK, just return; */

  /* check for any tag arguments */
  if(getTagArgs("feature", tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);
  if(tagArgs) {
    argValue = msLookupHashTable(tagArgs, "limit");
    if(argValue) limit = atoi(argValue);

    argValue = msLookupHashTable(tagArgs, "trimlast");
    if(argValue) trimLast = argValue;
  }

  if(strstr(*line, "[/feature]") == NULL) { /* we know the closing tag must be here, if not throw an error */
    msSetError(MS_WEBERR, "[feature] tag found without closing [/feature].", "processFeatureTag()");
    msFreeHashTable(tagArgs);
    return(MS_FAILURE);
  }

  if(getInlineTag("feature", *line, &tag) != MS_SUCCESS) {
    msSetError(MS_WEBERR, "Malformed feature tag.", "processFeatureTag()");
    msFreeHashTable(tagArgs);
    return MS_FAILURE;
  }

  preTag = getPreTagText(*line, "[feature");
  postTag = getPostTagText(*line, "[/feature]");

  /* start rebuilding **line */
  free(*line);
  *line = preTag;

  /* we know the layer has query results or we wouldn't be in this code */

#if 0
  status = msLayerOpen(layer); /* open the layer */
  if(status != MS_SUCCESS) return status;
  status = msLayerGetItems(layer); /* retrieve all the item names */
  if(status != MS_SUCCESS) return status;
#endif

  if(layer->numjoins > 0) { /* initialize necessary JOINs here */
    for(j=0; j<layer->numjoins; j++) {
      status = msJoinConnect(layer, &(layer->joins[j]));
      if(status != MS_SUCCESS) {
        msFreeHashTable(tagArgs);
        msFree(postTag);
        msFree(tag);
        return status;
      }
    }
  }

  mapserv->LRN = 1; /* layer result counter */
  mapserv->resultlayer = layer;
  msInitShape(&(mapserv->resultshape));

  if(limit == -1) /* return all */
    limit = layer->resultcache->numresults;
  else
    limit = MS_MIN(limit, layer->resultcache->numresults);

  for(i=0; i<limit; i++) {
    status = msLayerGetShape(layer, &(mapserv->resultshape), &(layer->resultcache->results[i]));
    if(status != MS_SUCCESS) {
      msFreeHashTable(tagArgs);
      msFree(postTag);
      msFree(tag);
      return status;
    }

    mapserv->resultshape.classindex = msShapeGetClass(layer, layer->map, &mapserv->resultshape,  NULL, -1);

    /* prepare any necessary JOINs here (one-to-one only) */
    if(layer->numjoins > 0) {
      for(j=0; j<layer->numjoins; j++) {
        if(layer->joins[j].type == MS_JOIN_ONE_TO_ONE) {
          msJoinPrepare(&(layer->joins[j]), &(mapserv->resultshape));
          msJoinNext(&(layer->joins[j])); /* fetch the first row */
        }
      }
    }

    /*
    ** if necessary trim a few characters off the end of the tag
    */
    if(trimLast && (i == limit-1)) {
      char *ptr;
      if((ptr = strrstr(tag, trimLast)) != NULL)
        *ptr = '\0';
    }

    /* process the tag */
    tagInstance = processLine(mapserv, tag, NULL, QUERY); /* do substitutions */
    *line = msStringConcatenate(*line, tagInstance); /* grow the line */

    free(tagInstance);
    msFreeShape(&(mapserv->resultshape)); /* init too */

    mapserv->RN++; /* increment counters */
    mapserv->LRN++;
  }

  /* msLayerClose(layer); */
  mapserv->resultlayer = NULL; /* necessary? */

  *line = msStringConcatenate(*line, postTag);

  /*
  ** clean up
  */
  free(postTag);
  free(tag);
  msFreeHashTable(tagArgs);

  return(MS_SUCCESS);
}

/*
** Function to process a [resultset ...] tag.
*/
static int processResultSetTag(mapservObj *mapserv, char **line, FILE *stream)
{
  char lineBuffer[MS_BUFFER_LENGTH];
  int foundTagEnd;

  char *preTag, *postTag; /* text before and after the tag */

  char *tag;
  hashTableObj *tagArgs=NULL;

  const char *layerName=NULL;
  const char *nodata=NULL;

  layerObj *lp;

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processResultSetTag()");
    return(MS_FAILURE);
  }

  const char* tagStart = findTag(*line, "resultset");
  if(!tagStart) return(MS_SUCCESS); /* OK, just return; */

  while (tagStart) {
    /* initialize the tag arguments */
    layerName = NULL;


    /* check for any tag arguments */
    if(getTagArgs("resultset", tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);
    if(tagArgs) {
      layerName = msLookupHashTable(tagArgs, "layer");
      nodata = msLookupHashTable(tagArgs, "nodata");
    }

    if(!layerName) {
      msSetError(MS_WEBERR, "[resultset] tag missing required 'layer' argument.", "processResultSetTag()");
      msFreeHashTable(tagArgs);
      return(MS_FAILURE);
    }

    const int layerIndex = msGetLayerIndex(mapserv->map, layerName);
    if(layerIndex>=mapserv->map->numlayers || layerIndex<0) {
      msSetError(MS_MISCERR, "Layer named '%s' does not exist.", "processResultSetTag()", layerName);
      msFreeHashTable(tagArgs);
      return MS_FAILURE;
    }
    lp = GET_LAYER(mapserv->map, layerIndex);

    if(strstr(*line, "[/resultset]") == NULL) { /* read ahead */
      if(!stream) {
        msSetError(MS_WEBERR, "Invalid file pointer.", "processResultSetTag()");
        msFreeHashTable(tagArgs);
        return(MS_FAILURE);
      }

      foundTagEnd = MS_FALSE;
      while(!foundTagEnd) {
        if(fgets(lineBuffer, MS_BUFFER_LENGTH, stream) != NULL) {
          *line = msStringConcatenate(*line, lineBuffer);
          if(strstr(*line, "[/resultset]") != NULL)
            foundTagEnd = MS_TRUE;
        } else
          break; /* ran out of file */
      }
      if(foundTagEnd == MS_FALSE) {
        msSetError(MS_WEBERR, "[resultset] tag found without closing [/resultset].", "processResultSetTag()");
        msFreeHashTable(tagArgs);
        return(MS_FAILURE);
      }
    }

    if(getInlineTag("resultset", *line, &tag) != MS_SUCCESS) {
      msSetError(MS_WEBERR, "Malformed resultset tag.", "processResultSetTag()");
      msFreeHashTable(tagArgs);
      return MS_FAILURE;
    }

    preTag = getPreTagText(*line, "[resultset"); /* TODO: need to handle tags in these */
    postTag = getPostTagText(*line, "[/resultset]");

    /* start rebuilding **line */
    free(*line);
    *line = preTag;

    if(lp->resultcache && lp->resultcache->numresults > 0) {
      /* probably will need a while-loop here to handle multiple instances of [feature ...] tags */
      if(processFeatureTag(mapserv, &tag, lp) != MS_SUCCESS) {
        msFreeHashTable(tagArgs);
        return(MS_FAILURE); /* TODO: how to handle */
      }
      *line = msStringConcatenate(*line, tag);
    } else if(nodata) {
      *line = msStringConcatenate(*line, nodata);
    }

    *line = msStringConcatenate(*line, postTag);

    /* clean up */
    free(postTag);
    free(tag);

    tagStart = findTag(*line, "resultset");
  }
  msFreeHashTable(tagArgs);

  return(MS_SUCCESS);
}

/*
** Function process a [include src="..."] tag.
**
** TODO's:
**   - allow URLs
*/
static int processIncludeTag(mapservObj *mapserv, char **line, FILE *stream, int mode)
{
  char *tag, *tagEnd;
  hashTableObj *tagArgs=NULL;
  int tagOffset, tagLength;

  char *content=NULL, *processedContent=NULL;
  const char *src=NULL;

  FILE *includeStream;
  char buffer[MS_BUFFER_LENGTH], path[MS_MAXPATHLEN];

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processIncludeTag()");
    return(MS_FAILURE);
  }

  const char* tagStart = findTag(*line, "include");

  /* It is OK to have no include tags, just return. */
  if( !tagStart ) return MS_SUCCESS;

  while( tagStart ) {
    tagOffset = tagStart - *line;

    /* check for any tag arguments */
    if(getTagArgs("include", tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);
    if(tagArgs) {
      src = msLookupHashTable(tagArgs, "src");
    }

    if(!src) return(MS_SUCCESS); /* don't process the tag, could be something else so return MS_SUCCESS */

    if((includeStream = fopen(msBuildPath(path, mapserv->map->mappath, src), "r")) == NULL) {
      msSetError(MS_IOERR, "%s", "processIncludeTag()", src);
      return MS_FAILURE;
    }

    if(isValidTemplate(includeStream, src) != MS_TRUE) {
      fclose(includeStream);
      return MS_FAILURE;
    }

    while(fgets(buffer, MS_BUFFER_LENGTH, includeStream) != NULL)
      content = msStringConcatenate(content, buffer);

    /* done with included file handle */
    fclose(includeStream);

    /* find the end of the tag */
    tagEnd = findTagEnd(tagStart);
    tagEnd++;

    /* build the complete tag so we can do substitution */
    tagLength = tagEnd - tagStart;
    tag = (char *) msSmallMalloc(tagLength + 1);
    strlcpy(tag, tagStart, tagLength+1);

    /* process any other tags in the content */
    processedContent = processLine(mapserv, content, stream, mode);

    /* do the replacement */
    *line = msReplaceSubstring(*line, tag, processedContent);

    /* clean up */
    free(tag);
    tag = NULL;
    msFreeHashTable(tagArgs);
    tagArgs=NULL;
    free(content);
    free(processedContent);

    if((*line)[tagOffset] != '\0')
      tagStart = findTag(*line+tagOffset+1, "include");
    else
      tagStart = NULL;
  }

  return(MS_SUCCESS);
}

/*
** Function to process an [item ...] tag: line contains the tag, shape holds the attributes.
*/
enum ITEM_ESCAPING {ESCAPE_HTML, ESCAPE_URL, ESCAPE_JSON, ESCAPE_NONE};

static int processItemTag(layerObj *layer, char **line, shapeObj *shape)
{
  int i;

  char *tag, *tagEnd;
  hashTableObj *tagArgs=NULL;
  int tagLength;
  char *encodedTagValue=NULL, *tagValue=NULL;

  const char *argValue=NULL;

  const char *name=NULL, *pattern=NULL;
  const char *format=NULL, *nullFormat=NULL;
  int precision;
  int padding;
  int uc, lc, commify;
  int escape;

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processItemTag()");
    return(MS_FAILURE);
  }

  const char* tagStart = findTag(*line, "item");

  if(!tagStart) return(MS_SUCCESS); /* OK, just return; */

  while (tagStart) {
    format = "$value"; /* initialize the tag arguments */
    nullFormat = "";
    precision = -1;
    padding = -1;
    name = pattern = NULL;
    uc = lc = commify = MS_FALSE;
    escape=ESCAPE_HTML;

    /* check for any tag arguments */
    if(getTagArgs("item", tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);
    if(tagArgs) {
      argValue = msLookupHashTable(tagArgs, "name");
      if(argValue) name = argValue;

      argValue = msLookupHashTable(tagArgs, "pattern");
      if(argValue) pattern = argValue;

      argValue = msLookupHashTable(tagArgs, "precision");
      if(argValue) precision = atoi(argValue);

      argValue = msLookupHashTable(tagArgs, "padding");
      if (argValue) padding = atoi(argValue);

      argValue = msLookupHashTable(tagArgs, "format");
      if(argValue) format = argValue;

      argValue = msLookupHashTable(tagArgs, "nullformat");
      if(argValue) nullFormat = argValue;

      argValue = msLookupHashTable(tagArgs, "uc");
      if(argValue && strcasecmp(argValue, "true") == 0) uc = MS_TRUE;

      argValue = msLookupHashTable(tagArgs, "lc");
      if(argValue && strcasecmp(argValue, "true") == 0) lc = MS_TRUE;

      argValue = msLookupHashTable(tagArgs, "commify");
      if(argValue && strcasecmp(argValue, "true") == 0) commify = MS_TRUE;

      argValue = msLookupHashTable(tagArgs, "escape");
      if(argValue && strcasecmp(argValue, "url") == 0) escape = ESCAPE_URL;
      else if(argValue && strcasecmp(argValue, "none") == 0) escape = ESCAPE_NONE;
      else if(argValue && strcasecmp(argValue, "json") == 0) escape = ESCAPE_JSON;

      /* TODO: deal with sub strings */
    }

    if(!name) {
      msSetError(MS_WEBERR, "Item tag contains no name attribute.", "processItemTag()");
      return(MS_FAILURE);
    }

    for(i=0; i<layer->numitems; i++)
      if(strcasecmp(name, layer->items[i]) == 0) break;

    if(i == layer->numitems) {
      msSetError(MS_WEBERR, "Item name (%s) not found in layer item list.", "processItemTag()", name);
      return(MS_FAILURE);
    }

    /*
    ** now we know which item so build the tagValue
    */
    if(shape->values[i] && strlen(shape->values[i]) > 0) {
      char *itemValue=NULL;

      /* set tag text depending on pattern (if necessary), nullFormat can contain $value (#3637) */
      if(pattern && msEvalRegex(pattern, shape->values[i]) != MS_TRUE)
        tagValue = msStrdup(nullFormat);
      else
        tagValue = msStrdup(format);

      if(precision != -1) {
        char numberFormat[16];

        itemValue = (char *) msSmallMalloc(64); /* plenty big */
        snprintf(numberFormat, sizeof(numberFormat), "%%.%dlf", precision);
        snprintf(itemValue, 64, numberFormat, atof(shape->values[i]));
      } else
        itemValue = msStrdup(shape->values[i]);

      if(commify == MS_TRUE)
        itemValue = msCommifyString(itemValue);

      /* apply other effects */
      if(uc == MS_TRUE)
        for(unsigned j=0; j<strlen(itemValue); j++) itemValue[j] = toupper(itemValue[j]);
      if(lc == MS_TRUE)
        for(unsigned j=0; j<strlen(itemValue); j++) itemValue[j] = tolower(itemValue[j]);

      tagValue = msReplaceSubstring(tagValue, "$value", itemValue);
      msFree(itemValue);

      if (padding > 0 && padding < 1000) {
          int paddedSize = strlen(tagValue) + padding + 1;
          char *paddedValue = NULL;
          paddedValue = (char *) msSmallMalloc(paddedSize);
          snprintf(paddedValue, paddedSize, "%-*s", padding, tagValue);
          msFree(tagValue);
          tagValue = paddedValue;
      }

      if(!tagValue) {
        msSetError(MS_WEBERR, "Error applying item format.", "processItemTag()");
        return(MS_FAILURE); /* todo leaking... */
      }
    } else {
      tagValue = msStrdup(nullFormat); /* attribute value is NULL or empty */
    }

    /* find the end of the tag */
    tagEnd = findTagEnd(tagStart);
    tagEnd++;

    /* build the complete tag so we can do substitution */
    tagLength = tagEnd - tagStart;
    tag = (char *) msSmallMalloc(tagLength + 1);
    strlcpy(tag, tagStart, tagLength+1);

    /* do the replacement */
    switch(escape) {
      case ESCAPE_HTML:
        encodedTagValue = msEncodeHTMLEntities(tagValue);
        *line = msReplaceSubstring(*line, tag, encodedTagValue);
        break;
      case ESCAPE_JSON:
        encodedTagValue = msEscapeJSonString(tagValue);
        *line = msReplaceSubstring(*line, tag, encodedTagValue);
        break;
      case ESCAPE_URL:
        encodedTagValue = msEncodeUrl(tagValue);
        *line = msReplaceSubstring(*line, tag, encodedTagValue);
        break;
      case ESCAPE_NONE:
        *line = msReplaceSubstring(*line, tag, tagValue);
        break;
      default:
        break;
    }

    /* clean up */
    free(tag);
    tag = NULL;
    msFreeHashTable(tagArgs);
    tagArgs=NULL;
    msFree(tagValue);
    tagValue=NULL;
    msFree(encodedTagValue);
    encodedTagValue=NULL;

    tagStart = findTag(*line, "item");
  }

  return(MS_SUCCESS);
}

/*
** Function process any number of MapServer extent tags (e.g. shpext, mapext, etc...).
*/
static int processExtentTag(mapservObj *mapserv, char **line, char *name, rectObj *extent, projectionObj *rectProj)
{
  rectObj tempExtent;

  char number[64]; /* holds a single number in the extent */
  char numberFormat[16];

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processExtentTag()");
    return(MS_FAILURE);
  }

  const char* tagStart = findTag(*line, name); /* this supports any extent */

  /* It is OK to have no include tags, just return. */
  if(!tagStart) return MS_SUCCESS;

  while(tagStart) {
    double xExpand = 0, yExpand = 0; /* set tag argument defaults */
    int precision = -1;
    const char* format = "$minx $miny $maxx $maxy";
    const char* projectionString = NULL;

    /* hack to handle tags like 'mapext_esc' easily */
    int escape;
    if(strstr(name, "_esc"))
      escape = ESCAPE_URL;
    else
      escape = ESCAPE_HTML;

    const int tagOffset = tagStart - *line;

    /* check for any tag arguments */
    hashTableObj *tagArgs=NULL;
    if(getTagArgs(name, tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);
    if(tagArgs) {
      const char* argValue = msLookupHashTable(tagArgs, "expand");
      if(argValue) {
        if(strchr(argValue, '%') != NULL) {
          float f;
          sscanf(argValue, "%f%%", &f);
          xExpand = ((f/100.0)*(extent->maxx-extent->minx))/2;
          yExpand = ((f/100.0)*(extent->maxy-extent->miny))/2;
        } else {
          xExpand = atof(argValue);
          yExpand = xExpand;
        }
      }

      argValue = msLookupHashTable(tagArgs, "escape");
      if(argValue && strcasecmp(argValue, "url") == 0) escape = ESCAPE_URL;
      else if(argValue && strcasecmp(argValue, "none") == 0) escape = ESCAPE_NONE;

      argValue = msLookupHashTable(tagArgs, "format");
      if(argValue) format = argValue;

      argValue = msLookupHashTable(tagArgs, "precision");
      if(argValue) precision = atoi(argValue);

      argValue = msLookupHashTable(tagArgs, "proj");
      if(argValue) projectionString = argValue;
    }

    tempExtent.minx = extent->minx - xExpand;
    tempExtent.miny = extent->miny - yExpand;
    tempExtent.maxx = extent->maxx + xExpand;
    tempExtent.maxy = extent->maxy + yExpand;

    /* no big deal to convert from file to image coordinates, but what are the image parameters */
    if(rectProj && projectionString && strcasecmp(projectionString,"image") == 0) {
      precision = 0;

      /* if necessary, project the shape to match the map */
      if(msProjectionsDiffer(rectProj, &(mapserv->map->projection)))
        msProjectRect(rectProj, &mapserv->map->projection, &tempExtent);

      /* convert tempExtent to image coordinates based on the map extent and cellsize */
      tempExtent.minx = MS_MAP2IMAGE_X(tempExtent.minx, mapserv->map->extent.minx, mapserv->map->cellsize);
      tempExtent.miny = MS_MAP2IMAGE_Y(tempExtent.miny, mapserv->map->extent.maxy, mapserv->map->cellsize);
      tempExtent.maxx = MS_MAP2IMAGE_X(tempExtent.minx, mapserv->map->extent.minx, mapserv->map->cellsize);
      tempExtent.maxy = MS_MAP2IMAGE_Y(tempExtent.miny, mapserv->map->extent.maxy, mapserv->map->cellsize);
    } else if(rectProj && projectionString) {
      projectionObj projection;
      msInitProjection(&projection);
      msProjectionInheritContextFrom(&projection, &mapserv->map->projection);

      if(MS_SUCCESS != msLoadProjectionString(&projection, projectionString)) return MS_FAILURE;

      if(msProjectionsDiffer(rectProj, &projection))
        msProjectRect(rectProj, &projection, &tempExtent);
    }

    char* tagValue = msStrdup(format);

    if(precision != -1)
      snprintf(numberFormat, sizeof(numberFormat), "%%.%dlf", precision);
    else
      snprintf(numberFormat, sizeof(numberFormat), "%%f");

    snprintf(number, sizeof(number), numberFormat, tempExtent.minx);
    tagValue = msReplaceSubstring(tagValue, "$minx", number);
    snprintf(number, sizeof(number), numberFormat, tempExtent.miny);
    tagValue = msReplaceSubstring(tagValue, "$miny", number);
    snprintf(number, sizeof(number), numberFormat, tempExtent.maxx);
    tagValue = msReplaceSubstring(tagValue, "$maxx", number);
    snprintf(number, sizeof(number), numberFormat, tempExtent.maxy);
    tagValue = msReplaceSubstring(tagValue, "$maxy", number);

    /* find the end of the tag */
    char* tagEnd = findTagEnd(tagStart);
    tagEnd++;

    /* build the complete tag so we can do substitution */
    const int tagLength = tagEnd - tagStart;
    char* tag = (char *) msSmallMalloc(tagLength + 1);
    strlcpy(tag, tagStart, tagLength+1);

    /* do the replacement */
    switch(escape) {
      case ESCAPE_HTML:
      {
        char* encodedTagValue = msEncodeHTMLEntities(tagValue);
        *line = msReplaceSubstring(*line, tag, encodedTagValue);
        msFree(encodedTagValue);
        break;
      }
      case ESCAPE_URL:
      {
        char* encodedTagValue = msEncodeUrl(tagValue);
        *line = msReplaceSubstring(*line, tag, encodedTagValue);
        msFree(encodedTagValue);
        break;
      }
      case ESCAPE_NONE:
        *line = msReplaceSubstring(*line, tag, tagValue);
        break;
      default:
        break;
    }

    /* clean up */
    free(tag);
    msFreeHashTable(tagArgs);
    msFree(tagValue);

    if((*line)[tagOffset] != '\0')
      tagStart = findTag(*line+tagOffset+1, name);
    else
      tagStart = NULL;
  }

  return(MS_SUCCESS);
}

// RFC 77 TODO: Need to validate these changes with Assefa...
static int processShplabelTag(layerObj *layer, char **line, shapeObj *origshape)
{
  char *tagEnd;
  shapeObj tShape;
  int precision=0;
  int clip_to_map=MS_TRUE;
  int use_label_settings=MS_FALSE;
  int labelposvalid = MS_FALSE;
  pointObj labelPos;
  int status;
  char number[64]; /* holds a single number in the extent */
  char numberFormat[16];
  shapeObj *shape = NULL;

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processShplabelTag()");
    return(MS_FAILURE);
  }
  if(msCheckParentPointer(layer->map,"map") == MS_FAILURE)
    return MS_FAILURE;

  const char* tagStart = findTag(*line, "shplabel");

  /* It is OK to have no shplabel tags, just return. */
  if(!tagStart)
    return MS_SUCCESS;

  if(!origshape || origshape->numlines <= 0) { /* I suppose we need to make sure the part has vertices (need shape checker?) */
    msSetError(MS_WEBERR, "Null or empty shape.", "processShplabelTag()");
    return(MS_FAILURE);
  }

  while(tagStart) {
    if(shape) msFreeShape(shape);
    shape = (shapeObj *) msSmallMalloc(sizeof(shapeObj));
    msInitShape(shape);
    msCopyShape(origshape, shape);

    const char* projectionString = NULL;
    const char* format = "$x,$y";
    const int tagOffset = tagStart - *line;

    hashTableObj *tagArgs=NULL;
    if(getTagArgs("shplabel", tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);
    if(tagArgs) {
      const char* argValue = msLookupHashTable(tagArgs, "format");
      if(argValue) format = argValue;

      argValue = msLookupHashTable(tagArgs, "precision");
      if(argValue) precision = atoi(argValue);

      argValue = msLookupHashTable(tagArgs, "proj");
      if(argValue) projectionString = argValue;

      argValue = msLookupHashTable(tagArgs, "clip_to_map");
      if(argValue) {
        if(strcasecmp(argValue,"false") == 0) clip_to_map = MS_FALSE;
      }

      argValue = msLookupHashTable(tagArgs, "use_label_settings");
      if(argValue) {
        if(strcasecmp(argValue,"true") == 0) use_label_settings = MS_TRUE;
      }
    }

    labelPos.x = -1;
    labelPos.y = -1;
    msInitShape(&tShape);

    tShape.type = MS_SHAPE_LINE;
    tShape.line = (lineObj *) msSmallMalloc(sizeof(lineObj));
    tShape.numlines = 1;
    tShape.line[0].point = NULL; /* initialize the line */
    tShape.line[0].numpoints = 0;

    double cellsize;
    if(layer->map->cellsize <= 0)
      cellsize = MS_MAX(MS_CELLSIZE(layer->map->extent.minx, layer->map->extent.maxx, layer->map->width), MS_CELLSIZE(layer->map->extent.miny, layer->map->extent.maxy, layer->map->height));
    else
      cellsize = layer->map->cellsize ;

    if(shape->type == MS_SHAPE_POINT) {
      labelposvalid = MS_FALSE;
      if(shape->numlines > 0 && shape->line[0].numpoints > 0) {
        labelposvalid = MS_TRUE;
        labelPos = shape->line[0].point[0];
        if(layer->transform == MS_TRUE) {
          if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
          {
            if( layer->reprojectorLayerToMap == NULL )
            {
                layer->reprojectorLayerToMap = msProjectCreateReprojector(
                    &layer->projection, &layer->map->projection);
            }
            if( layer->reprojectorLayerToMap )
            {
                msProjectShapeEx(layer->reprojectorLayerToMap, shape);
            }
          }

          labelPos = shape->line[0].point[0];
          labelPos.x = MS_MAP2IMAGE_X(labelPos.x, layer->map->extent.minx, cellsize);
          labelPos.y = MS_MAP2IMAGE_Y(labelPos.y, layer->map->extent.maxy, cellsize);
        }
      }
    } else if(shape->type == MS_SHAPE_LINE) {
      labelposvalid = MS_FALSE;
      if(layer->transform == MS_TRUE) {
        if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
        {
            if( layer->reprojectorLayerToMap == NULL )
            {
                layer->reprojectorLayerToMap = msProjectCreateReprojector(
                    &layer->projection, &layer->map->projection);
            }
            if( layer->reprojectorLayerToMap )
            {
                msProjectShapeEx(layer->reprojectorLayerToMap, shape);
            }
        }

        if(clip_to_map)
          msClipPolylineRect(shape, layer->map->extent);

        msTransformShapeToPixelRound(shape, layer->map->extent, cellsize);
      } else
        msOffsetShapeRelativeTo(shape, layer);

      if(shape->numlines > 0) {
        struct label_auto_result lar;
        memset(&lar,0,sizeof(struct label_auto_result));
        if(MS_UNLIKELY(MS_FAILURE == msPolylineLabelPoint(layer->map, shape, NULL, NULL, &lar, 0))) {
          free(lar.angles);
          free(lar.label_points);
          return MS_FAILURE;
        }
        if(lar.num_label_points > 0) {
          /* convert to geo */
          labelPos.x = lar.label_points[0].x;
          labelPos.y = lar.label_points[0].y;
          labelposvalid = MS_TRUE;
        }
        free(lar.angles);
        free(lar.label_points);
      }
    } else if (shape->type == MS_SHAPE_POLYGON) {
      labelposvalid = MS_FALSE;
      if(layer->transform == MS_TRUE) {
        if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
        {
            if( layer->reprojectorLayerToMap == NULL )
            {
                layer->reprojectorLayerToMap = msProjectCreateReprojector(
                    &layer->projection, &layer->map->projection);
            }
            if( layer->reprojectorLayerToMap )
            {
                msProjectShapeEx(layer->reprojectorLayerToMap, shape);
            }
        }

        if(clip_to_map)
          msClipPolygonRect(shape, layer->map->extent);

        msTransformShapeToPixelRound(shape, layer->map->extent, cellsize);
      } else
        msOffsetShapeRelativeTo(shape, layer);

      if(shape->numlines > 0) {
        if(msPolygonLabelPoint(shape, &labelPos, -1) == MS_SUCCESS) {
          if(labelPos.x == -1 && labelPos.y == -1)
            labelposvalid = MS_FALSE;
          else
            labelposvalid = MS_TRUE;
        }
      }
    }

    if(labelposvalid == MS_TRUE) {
      pointObj p1 = {0}; // initialize
      pointObj p2 = {0};
      int label_offset_x, label_offset_y;
      labelObj *label=NULL;
      label_bounds lbounds;
      lineObj lbounds_line;
      pointObj lbounds_point[5];
      double tmp;

      p1.x =labelPos.x;
      p1.y =labelPos.y;

      p2.x =labelPos.x;
      p2.y =labelPos.y;
      if(use_label_settings == MS_TRUE) {

        /* RFC 77: classes (and shapes) can have more than 1 piece of annotation, here we only use the first (index=0) */
        if(shape->classindex >= 0  && layer->class[shape->classindex]->numlabels > 0) {
          label = layer->class[shape->classindex]->labels[0];
          if(msGetLabelStatus(layer->map,layer,shape,label) == MS_ON) {
            char *annotext = msShapeGetLabelAnnotation(layer,shape,label);
            if(annotext) {
              textSymbolObj ts;
              initTextSymbol(&ts);
              msPopulateTextSymbolForLabelAndString(&ts, label, annotext, layer->scalefactor, 1.0, 0);

              label_offset_x = (int)(label->offsetx*layer->scalefactor);
              label_offset_y = (int)(label->offsety*layer->scalefactor);
              lbounds.poly = &lbounds_line;
              lbounds_line.numpoints = 5;
              lbounds_line.point = lbounds_point;

              p1 = get_metrics(&labelPos, label->position, ts.textpath, label_offset_x, label_offset_y, label->angle* MS_DEG_TO_RAD, 0, &lbounds);

              /* should we use the point returned from  get_metrics?. From few test done, It seems
                to return the UL corner of the text. For now use the bounds.minx/miny */
              p1.x = lbounds.bbox.minx;
              p1.y = lbounds.bbox.miny;
              p2.x = lbounds.bbox.maxx;
              p2.y = lbounds.bbox.maxy;
              freeTextSymbol(&ts);
            }
          }
        }
      }

      /* y's are flipped because it is in image coordinate systems */
      p1.x = MS_IMAGE2MAP_X(p1.x, layer->map->extent.minx, cellsize);
      tmp = p1.y;
      p1.y = MS_IMAGE2MAP_Y(p2.y, layer->map->extent.maxy, cellsize);
      p2.x = MS_IMAGE2MAP_X(p2.x, layer->map->extent.minx, cellsize);
      p2.y = MS_IMAGE2MAP_Y(tmp, layer->map->extent.maxy, cellsize);
      if(layer->transform == MS_TRUE) {
        if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection))) {
          msProjectPoint(&layer->map->projection, &layer->projection, &p1);
          msProjectPoint(&layer->map->projection, &layer->projection, &p2);
        }
      }
      msAddPointToLine(&(tShape.line[0]), &p1);
      msAddPointToLine(&(tShape.line[0]), &p2);
    } else
      tShape.numlines = 0;

    if(projectionString && strcasecmp(projectionString,"image") == 0) {
      precision = 0;

      /* if necessary, project the shape to match the map */
      if(msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
      {
        if( layer->reprojectorLayerToMap == NULL )
        {
            layer->reprojectorLayerToMap = msProjectCreateReprojector(
                &layer->projection, &layer->map->projection);
        }
        if( layer->reprojectorLayerToMap )
        {
            msProjectShapeEx(layer->reprojectorLayerToMap, &tShape);
        }
      }

      msClipPolylineRect(&tShape, layer->map->extent);

      msTransformShapeToPixelRound(&tShape, layer->map->extent, layer->map->cellsize);
    } else if(projectionString) {
      projectionObj projection;
      msInitProjection(&projection);
      msProjectionInheritContextFrom(&projection, &layer->map->projection);

      status = msLoadProjectionString(&projection, projectionString);
      if(status != MS_SUCCESS) return MS_FAILURE;

      if(msProjectionsDiffer(&(layer->projection), &projection))
        msProjectShape(&layer->projection, &projection, &tShape);
    }

    /* do the replacement */
    char* tagValue = msStrdup(format);
    if(precision > 0)
      snprintf(numberFormat, sizeof(numberFormat), "%%.%dlf", precision);
    else
      snprintf(numberFormat, sizeof(numberFormat), "%%f");

    if(tShape.numlines > 0) {
      if(strcasestr(tagValue, "$x") != 0) {
        snprintf(number, sizeof(number), numberFormat, tShape.line[0].point[0].x);
        tagValue = msReplaceSubstring(tagValue, "$x", number);
      }
      if(strcasestr(tagValue, "$y") != 0) {
        snprintf(number, sizeof(number), numberFormat, tShape.line[0].point[0].y);
        tagValue = msReplaceSubstring(tagValue, "$y", number);
      }

      if(strcasestr(tagValue, "$minx") != 0) {
        snprintf(number, sizeof(number), numberFormat, tShape.line[0].point[0].x);
        tagValue = msReplaceSubstring(tagValue, "$minx", number);
      }
      if(strcasestr(tagValue, "$miny") != 0) {
        snprintf(number, sizeof(number), numberFormat, tShape.line[0].point[0].y);
        tagValue = msReplaceSubstring(tagValue, "$miny", number);
      }
      if(strcasestr(tagValue, "$maxx") != 0) {
        snprintf(number, sizeof(number), numberFormat, tShape.line[0].point[1].x);
        tagValue = msReplaceSubstring(tagValue, "$maxx", number);
      }
      if(strcasestr(tagValue, "$maxy") != 0) {
        snprintf(number, sizeof(number), numberFormat, tShape.line[0].point[1].y);
        tagValue = msReplaceSubstring(tagValue, "$maxy", number);
      }
    }

    /* find the end of the tag */
    tagEnd = findTagEnd(tagStart);
    tagEnd++;

    /* build the complete tag so we can do substitution */
    const int tagLength = tagEnd - tagStart;
    char* tag = (char *) msSmallMalloc(tagLength + 1);
    strlcpy(tag, tagStart, tagLength+1);

    *line = msReplaceSubstring(*line, tag, tagValue);

    /* clean up */
    msFreeShape(&tShape);
    free(tag);
    msFreeHashTable(tagArgs);
    msFree(tagValue);

    if((*line)[tagOffset] != '\0')
      tagStart = findTag(*line+tagOffset+1, "shplabel");
    else
      tagStart = NULL;
  }
  if(shape)
    msFreeShape(shape);

  return(MS_SUCCESS);
}

/*
** Function to process a [date ...] tag
*/
static int processDateTag(char **line)
{
  struct tm *datetime;
  time_t t;
  int result;
  char *tag=NULL, *tagEnd;
  hashTableObj *tagArgs=NULL;
  int tagOffset, tagLength;
#define DATE_BUFLEN 1024
  char datestr[DATE_BUFLEN];
  const char *argValue=NULL;
  const char *format, *tz; /* tag parameters */

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processDateTag()");
    return(MS_FAILURE);
  }

  const char* tagStart = findTag(*line, "date");

  /* It is OK to have no date tags, just return. */
  if( !tagStart )
    return MS_SUCCESS;

  while (tagStart) {
    /* set tag params to defaults */
    format = DEFAULT_DATE_FORMAT;
    tz = "";

    tagOffset = tagStart - *line;

    /* check for any tag arguments */
    if(getTagArgs("date", tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);

    if(tagArgs) {
      argValue = msLookupHashTable(tagArgs, "format");
      if(argValue) format = argValue;
      argValue = msLookupHashTable(tagArgs, "tz");
      if(argValue) tz = argValue;
    }

    t = time(NULL);
    if( strncasecmp( tz, "gmt", 4 ) == 0 ) {
      datetime = gmtime(&t);
    } else {
      datetime = localtime(&t);
    }
    result = strftime(datestr, DATE_BUFLEN, format, datetime);

    /* Only do the replacement if the date was successfully written */
    if( result > 0 ) {
      /* find the end of the tag */
      tagEnd = findTagEnd(tagStart);
      tagEnd++;

      /* build the complete tag so we can do substitution */
      tagLength = tagEnd - tagStart;
      tag = (char *) msSmallMalloc(tagLength + 1);
      strlcpy(tag, tagStart, tagLength+1);

      /* do the replacement */
      *line = msReplaceSubstring(*line, tag, datestr);
    }

    /* clean up */
    msFree(tag);
    tag = NULL;
    msFreeHashTable(tagArgs);
    tagArgs=NULL;

    if((*line)[tagOffset] != '\0')
      tagStart = findTag(*line+tagOffset+1, "date");
    else
      tagStart = NULL;
  }

  return(MS_SUCCESS);

}

/*
** Function to process a [shpxy ...] tag: line contains the tag, shape holds the coordinates.
**
** TODO's:
**   - May need to change attribute names.
**   - Need generalization routines (not here, but in mapprimative.c).
**   - Try to avoid all the realloc calls.
*/
static int processShpxyTag(layerObj *layer, char **line, shapeObj *shape)
{
  int i,j,p;
  int status;

  char *tagEnd;

  shapeObj tShape;
  char point[128];

  if(!*line) {
    msSetError(MS_WEBERR, "Invalid line pointer.", "processShpxyTag()");
    return(MS_FAILURE);
  }
  if( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  const char* tagStart = findTag(*line, "shpxy");

  /* It is OK to have no shpxy tags, just return. */
  if( !tagStart )
    return MS_SUCCESS;

  if(!shape || shape->numlines <= 0) { /* I suppose we need to make sure the part has vertices (need shape checker?) */
    msSetError(MS_WEBERR, "Null or empty shape.", "processShpxyTag()");
    return(MS_FAILURE);
  }

  while (tagStart) {
#ifdef USE_GEOS
    double buffer = 0;
    int bufferUnits = -1;
#endif

    /*
    ** Pointers to static strings, naming convention is:
    **   char 1/2 - x=x, y=y, c=coordinate, p=part, s=shape, ir=inner ring, or=outer ring
    **   last char - h=header, f=footer, s=seperator
    */
    const char *xh = "", *yh = "";
    const char *xf = ",";
    const char *yf = "";
    const char *cs = " ";
    const char *ph = "", *pf = "";
    const char *ps = " ";
    const char *sh = "", *sf = "";
    const char *irh = "", *irf = ""; /* inner ring: necessary for complex polygons */
    const char *orh = "", *orf = ""; /* outer ring */

    int centroid = MS_FALSE;
    int precision = 0;
    double scale_x = 1.0;
    double scale_y = 1.0;

    const char* projectionString = NULL;

    const int tagOffset = tagStart - *line;

    /* check for any tag arguments */
    hashTableObj *tagArgs=NULL;
    if(getTagArgs("shpxy", tagStart, &tagArgs) != MS_SUCCESS) return(MS_FAILURE);
    if(tagArgs) {
      const char* argValue = msLookupHashTable(tagArgs, "xh");
      if(argValue) xh = argValue;
      argValue = msLookupHashTable(tagArgs, "xf");
      if(argValue) xf = argValue;

      argValue = msLookupHashTable(tagArgs, "yh");
      if(argValue) yh = argValue;
      argValue = msLookupHashTable(tagArgs, "yf");
      if(argValue) yf = argValue;

      argValue = msLookupHashTable(tagArgs, "cs");
      if(argValue) cs = argValue;

      argValue = msLookupHashTable(tagArgs, "irh");
      if(argValue) irh = argValue;
      argValue = msLookupHashTable(tagArgs, "irf");
      if(argValue) irf = argValue;

      argValue = msLookupHashTable(tagArgs, "orh");
      if(argValue) orh = argValue;
      argValue = msLookupHashTable(tagArgs, "orf");
      if(argValue) orf = argValue;

      argValue = msLookupHashTable(tagArgs, "ph");
      if(argValue) ph = argValue;
      argValue = msLookupHashTable(tagArgs, "pf");
      if(argValue) pf = argValue;
      argValue = msLookupHashTable(tagArgs, "ps");
      if(argValue) ps = argValue;

      argValue = msLookupHashTable(tagArgs, "sh");
      if(argValue) sh = argValue;
      argValue = msLookupHashTable(tagArgs, "sf");
      if(argValue) sf = argValue;

#ifdef USE_GEOS
      argValue = msLookupHashTable(tagArgs, "buffer");
      if(argValue) {
        buffer = atof(argValue);
        if(strstr(argValue, "px")) bufferUnits = MS_PIXELS; /* may support others at some point */
      }
#endif

      argValue = msLookupHashTable(tagArgs, "precision");
      if(argValue) precision = atoi(argValue);

      argValue = msLookupHashTable(tagArgs, "scale");
      if(argValue) {
        scale_x = atof(argValue);
        scale_y = scale_x;
      }

      argValue = msLookupHashTable(tagArgs, "scale_x");
      if(argValue) scale_x = atof(argValue);

      argValue = msLookupHashTable(tagArgs, "scale_y");
      if(argValue) scale_y = atof(argValue);

      argValue = msLookupHashTable(tagArgs, "centroid");
      if(argValue)
        if(strcasecmp(argValue,"true") == 0) centroid = MS_TRUE;

      argValue = msLookupHashTable(tagArgs, "proj");
      if(argValue) projectionString = argValue;
    }

    /* build the per point format strings (version 1 contains the coordinate seperator, version 2 doesn't) */
    const int pointFormatLength = strlen(xh) + strlen(xf) + strlen(yh) + strlen(yf) + strlen(cs) + 12 + 1;
    char* pointFormat1 = (char *) msSmallMalloc(pointFormatLength);
    snprintf(pointFormat1, pointFormatLength, "%s%%.%dlf%s%s%%.%dlf%s%s", xh, precision, xf, yh, precision, yf, cs);
    char* pointFormat2 = (char *) msSmallMalloc(pointFormatLength);
    snprintf(pointFormat2, pointFormatLength, "%s%%.%dlf%s%s%%.%dlf%s", xh, precision, xf, yh, precision, yf);

    /* make a copy of the original shape or compute a centroid if necessary */
    msInitShape(&tShape);
    if(centroid == MS_TRUE) {
      pointObj p;

      p.x = (shape->bounds.minx + shape->bounds.maxx)/2;
      p.y = (shape->bounds.miny + shape->bounds.maxy)/2;

      tShape.type = MS_SHAPE_POINT;
      tShape.line = (lineObj *) msSmallMalloc(sizeof(lineObj));
      tShape.numlines = 1;
      tShape.line[0].point = NULL; /* initialize the line */
      tShape.line[0].numpoints = 0;

      msAddPointToLine(&(tShape.line[0]), &p);
    }

#ifdef USE_GEOS
    else if(buffer != 0 && bufferUnits != MS_PIXELS) {
      shapeObj *bufferShape=NULL;

      bufferShape = msGEOSBuffer(shape, buffer);
      if(!bufferShape) {
        free(pointFormat1);
        free(pointFormat2);
        return(MS_FAILURE); /* buffer failed */
      }
      msCopyShape(bufferShape, &tShape);
      msFreeShape(bufferShape);
    }
#endif
    else {
      status = msCopyShape(shape, &tShape);
      if(status != 0) {
        free(pointFormat1);
        free(pointFormat2);
        return(MS_FAILURE); /* copy failed */
      }
    }

    /* no big deal to convert from file to image coordinates, but what are the image parameters */
    if(projectionString && strcasecmp(projectionString,"image") == 0) {
      precision = 0;

      /* if necessary, project the shape to match the map */
      if(msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
      {
        if( layer->reprojectorLayerToMap == NULL )
        {
            layer->reprojectorLayerToMap = msProjectCreateReprojector(
                &layer->projection, &layer->map->projection);
        }
        if( layer->reprojectorLayerToMap )
        {
            msProjectShapeEx(layer->reprojectorLayerToMap, &tShape);
        }
      }

      switch(tShape.type) {
        case(MS_SHAPE_POINT):
          /* no clipping necessary */
          break;
        case(MS_SHAPE_LINE):
          msClipPolylineRect(&tShape, layer->map->extent);
          break;
        case(MS_SHAPE_POLYGON):
          msClipPolygonRect(&tShape, layer->map->extent);
          break;
        default:
          /* TO DO: need an error message here */
          return(MS_FAILURE);
          break;
      }
      msTransformShapeToPixelRound(&tShape, layer->map->extent, layer->map->cellsize);

#ifdef USE_GEOS
      if(buffer != 0 && bufferUnits == MS_PIXELS) {
        shapeObj *bufferShape=NULL;

        bufferShape = msGEOSBuffer(&tShape, buffer);
        if(!bufferShape) {
          if(!msIsDegenerateShape(&tShape)) /* If shape is degenerate this is expected. */
            return(MS_FAILURE); /* buffer failed */
        } else {
          msFreeShape(&tShape); /* avoid memory leak */
          msCopyShape(bufferShape, &tShape);
          msFreeShape(bufferShape);
        }
      }
#endif

    } else if(projectionString) {
      projectionObj projection;
      msInitProjection(&projection);
      msProjectionInheritContextFrom(&projection, &(layer->projection));

      status = msLoadProjectionString(&projection, projectionString);
      if(status != MS_SUCCESS) return MS_FAILURE;

      if(msProjectionsDiffer(&(layer->projection), &projection))
        msProjectShape(&layer->projection, &projection, &tShape);
    }

    /* TODO: add thinning support here */

    /*
    ** build the coordinate string
    */

    char* coords = NULL;
    if(strlen(sh) > 0) coords = msStringConcatenate(coords, sh);

    /* do we need to handle inner/outer rings */
    if(tShape.type == MS_SHAPE_POLYGON && strlen(orh) > 0 && strlen(irh) > 0) {
      int *outers;
      int firstPart; /* to keep track of inserting part separators before each part after the first */
      outers = msGetOuterList( &tShape );
      firstPart = 1;
      /* loop over rings looking for outers*/
      for(i=0; i<tShape.numlines; i++) {
        int *inners;
        if( outers[i] ) {
          /* this is an outer ring */
          if((!firstPart) && (strlen(ps) > 0)) coords = msStringConcatenate(coords, ps);
          firstPart = 0;
          if(strlen(ph) > 0) coords = msStringConcatenate(coords, ph);
          coords = msStringConcatenate(coords, orh);
          for(p=0; p<tShape.line[i].numpoints-1; p++) {
            snprintf(point, sizeof(point), pointFormat1, scale_x*tShape.line[i].point[p].x, scale_y*tShape.line[i].point[p].y);
            coords = msStringConcatenate(coords, point);
          }
          snprintf(point, sizeof(point), pointFormat2, scale_x*tShape.line[i].point[p].x, scale_y*tShape.line[i].point[p].y);
          coords = msStringConcatenate(coords, point);
          coords = msStringConcatenate(coords, orf);

          inners = msGetInnerList(&tShape, i, outers);
          /* loop over rings looking for inners to this outer */
          for(j=0; j<tShape.numlines; j++) {
            if( inners[j] ) {
              /* j is an inner ring of i */
              coords = msStringConcatenate(coords, irh);
              for(p=0; p<tShape.line[j].numpoints-1; p++) {
                snprintf(point, sizeof(point), pointFormat1, scale_x*tShape.line[j].point[p].x, scale_y*tShape.line[j].point[p].y);
                coords = msStringConcatenate(coords, point);
              }
              snprintf(point, sizeof(point), pointFormat2, scale_x*tShape.line[j].point[p].x, scale_y*tShape.line[j].point[p].y);
              coords = msStringConcatenate(coords, irf);
            }
          }
          free( inners );
          if(strlen(pf) > 0) coords = msStringConcatenate(coords, pf);
        }
      } /* end of loop over outer rings */
      free( outers );
    } else { /* output without ring formatting */

      for(i=0; i<tShape.numlines; i++) { /* e.g. part */

        /* skip degenerate parts, really should only happen with pixel output */
        if((tShape.type == MS_SHAPE_LINE && tShape.line[i].numpoints < 2) ||
            (tShape.type == MS_SHAPE_POLYGON && tShape.line[i].numpoints < 3))
          continue;

        if(strlen(ph) > 0) coords = msStringConcatenate(coords, ph);

        for(p=0; p<tShape.line[i].numpoints-1; p++) {
          snprintf(point, sizeof(point), pointFormat1, scale_x*tShape.line[i].point[p].x, scale_y*tShape.line[i].point[p].y);
          coords = msStringConcatenate(coords, point);
        }
        snprintf(point, sizeof(point), pointFormat2, scale_x*tShape.line[i].point[p].x, scale_y*tShape.line[i].point[p].y);
        coords = msStringConcatenate(coords, point);

        if(strlen(pf) > 0) coords = msStringConcatenate(coords, pf);

        if((i < tShape.numlines-1) && (strlen(ps) > 0)) coords = msStringConcatenate(coords, ps);
      }
    }
    if(strlen(sf) > 0) coords = msStringConcatenate(coords, sf);

    msFreeShape(&tShape);

    /* find the end of the tag */
    tagEnd = findTagEnd(tagStart);
    tagEnd++;

    /* build the complete tag so we can do substitution */
    const int tagLength = tagEnd - tagStart;
    char* tag = (char *) msSmallMalloc(tagLength + 1);
    strlcpy(tag, tagStart, tagLength+1);

    /* do the replacement */
    *line = msReplaceSubstring(*line, tag, coords);

    /* clean up */
    free(tag);
    msFreeHashTable(tagArgs);
    free(pointFormat1);
    free(pointFormat2);
    free(coords);

    if((*line)[tagOffset] != '\0')
      tagStart = findTag(*line+tagOffset+1, "shpxy");
    else
      tagStart = NULL;
  }

  return(MS_SUCCESS);
}

/*!
 * this function process all metadata
 * in pszInstr. ht mus contain all corresponding
 * metadata value.
 *
 * this function return a modified pszInstr
*/
int processMetadata(char** pszInstr, hashTableObj *ht)
{
  /* char *pszNextInstr = pszInstr; */
  char *pszEnd;
  char *pszMetadataTag;
  const char *pszHashName;
  const char *pszHashValue;
  int nLength, nOffset;

  hashTableObj *metadataArgs = NULL;

  if(!*pszInstr) {
    msSetError(MS_WEBERR, "Invalid pointer.", "processMetadata()");
    return MS_FAILURE;
  }

  /* set position to the begining of metadata tag */
  const char* pszStart = findTag(*pszInstr, "metadata");

  while (pszStart) {
    /* get metadata args */
    if(getTagArgs("metadata", pszStart, &metadataArgs) != MS_SUCCESS)
      return MS_FAILURE;

    pszHashName = msLookupHashTable(metadataArgs, "name");
    pszHashValue = msLookupHashTable(ht, pszHashName);

    nOffset = pszStart - *pszInstr;

    if(pszHashName && pszHashValue) {
      /* set position to the end of metadata start tag */
      pszEnd = strchr(pszStart, ']');
      pszEnd++;

      /* build the complete metadata tag ([metadata all_args]) */
      /* to replace it by the corresponding value from ht */
      nLength = pszEnd - pszStart;
      pszMetadataTag = (char*)msSmallMalloc(nLength + 1);
      strlcpy(pszMetadataTag, pszStart, nLength+1);

      *pszInstr = msReplaceSubstring(*pszInstr, pszMetadataTag, pszHashValue);

      free(pszMetadataTag);
      pszMetadataTag=NULL;
    }

    msFreeHashTable(metadataArgs);
    metadataArgs=NULL;


    /* set position to the begining of the next metadata tag */
    if((*pszInstr)[nOffset] != '\0')
      pszStart = findTag(*pszInstr+nOffset+1, "metadata");
    else
      pszStart = NULL;
  }

  return MS_SUCCESS;
}

/*!
 * this function process all icon tag
 * from pszInstr.
 *
 * This func return a modified pszInstr.
*/
int processIcon(mapObj *map, int nIdxLayer, int nIdxClass, char** pszInstr, char* pszPrefix)
{
  int nWidth, nHeight, nLen;
  char szImgFname[1024], *pszImgTag;
  char szPath[MS_MAXPATHLEN];
  hashTableObj *myHashTable=NULL;
  FILE *fIcon;

  if(!map ||
      nIdxLayer > map->numlayers ||
      nIdxLayer < 0 ) {
    msSetError(MS_WEBERR, "Invalid pointer.", "processIcon()");
    return MS_FAILURE;
  }

  /* find the begining of tag */
  pszImgTag = strstr(*pszInstr, "[leg_icon");

  while (pszImgTag) {
    int i;
    char szStyleCode[512] = "";
    classObj *thisClass=NULL;

    /* It's okay to have no classes... we'll generate an empty icon in this case */
    if(nIdxClass >= 0 && nIdxClass < GET_LAYER(map, nIdxLayer)->numclasses)
      thisClass = GET_LAYER(map, nIdxLayer)->class[nIdxClass];

    if(getTagArgs("leg_icon", pszImgTag, &myHashTable) != MS_SUCCESS)
      return MS_FAILURE;

    /* if no specified width or height, set them to map default */
    if(!msLookupHashTable(myHashTable, "width") || !msLookupHashTable(myHashTable, "height")) {
      nWidth = map->legend.keysizex;
      nHeight= map->legend.keysizey;
    } else {
      nWidth  = atoi(msLookupHashTable(myHashTable, "width"));
      nHeight = atoi(msLookupHashTable(myHashTable, "height"));
    }

    /* Create a unique and predictable filename to cache the legend icons.
     * Include some key parameters from the first 2 styles
     */
    for(i=0; i<2 && thisClass && i<thisClass->numstyles; i++) {
      styleObj *style;
      char *pszSymbolNameHash = NULL;
      style = thisClass->styles[i];
      if(style->symbolname)
        pszSymbolNameHash = msHashString(style->symbolname);

      snprintf(szStyleCode+strlen(szStyleCode), 255,
               "s%d_%x_%x_%d_%s_%g",
               i, MS_COLOR_GETRGB(style->color),MS_COLOR_GETRGB(style->outlinecolor),
               style->symbol, pszSymbolNameHash?pszSymbolNameHash:"",
               style->angle);
      msFree(pszSymbolNameHash);
    }

    snprintf(szImgFname, sizeof(szImgFname), "%s_%d_%d_%d_%d_%s.%s%c",
             pszPrefix, nIdxLayer, nIdxClass, nWidth, nHeight,
             szStyleCode, MS_IMAGE_EXTENSION(map->outputformat),'\0');

    char* pszFullImgFname = msStrdup(msBuildPath3(szPath, map->mappath,
                                            map->web.imagepath, szImgFname));

    /* check if icon already exist in cache */
    if((fIcon = fopen(pszFullImgFname, "r")) != NULL) {
      /* File already exists. No need to generate it again */
      fclose(fIcon);
    } else {
      /* Create an image corresponding to the current class */
      imageObj *img=NULL;

      if(thisClass == NULL) {
        /* Nonexistent class.  Create an empty image */
        img = msCreateLegendIcon(map, NULL, NULL, nWidth, nHeight, MS_TRUE);
      } else {
        img = msCreateLegendIcon(map, GET_LAYER(map, nIdxLayer),
                                 thisClass, nWidth, nHeight, MS_TRUE);
      }

      if(!img) {
        if(myHashTable)
          msFreeHashTable(myHashTable);

        msSetError(MS_IMGERR, "Error while creating image.", "processIcon()");
        msFree(pszFullImgFname);
        return MS_FAILURE;
      }

      /* save it with a unique file name */
      if(msSaveImage(map, img, pszFullImgFname) != MS_SUCCESS) {
        if(myHashTable)
          msFreeHashTable(myHashTable);

        msFreeImage(img);

        msSetError(MS_IOERR, "Error saving GD image to disk (%s).", "processIcon()", pszFullImgFname);
        msFree(pszFullImgFname);
        return MS_FAILURE;
      }

      msFreeImage(img);
    }

    msFree(pszFullImgFname);

    nLen = (strchr(pszImgTag, ']') + 1) - pszImgTag;

    if(nLen > 0) {
      char *pszTag;

      /* rebuid image tag ([leg_class_img all_args]) */
      /* to replace it by the image url */
      pszTag = (char*)msSmallMalloc(nLen + 1);
      strlcpy(pszTag, pszImgTag, nLen+1);

      char* pszFullImgUrlFname = (char*)msSmallMalloc(strlen(map->web.imageurl) + strlen(szImgFname) + 1);
      strcpy(pszFullImgUrlFname, map->web.imageurl);
      strcat(pszFullImgUrlFname, szImgFname);

      *pszInstr = msReplaceSubstring(*pszInstr, pszTag, pszFullImgUrlFname);

      msFree(pszFullImgUrlFname);
      msFree(pszTag);

      /* find the begining of tag */
      pszImgTag = strstr(*pszInstr, "[leg_icon");
    } else {
      pszImgTag = NULL;
    }

    if(myHashTable) {
      msFreeHashTable(myHashTable);
      myHashTable = NULL;
    }
  }

  return MS_SUCCESS;
}

/*!
 * Replace all tags from group template
 * with correct value.
 *
 * this function return a buffer containing
 * the template with correct values.
 *
 * buffer must be freed by caller.
*/
int generateGroupTemplate(char* pszGroupTemplate, mapObj *map, char* pszGroupName, hashTableObj *oGroupArgs, char **pszTemp, char* pszPrefix)
{
  hashTableObj *myHashTable;
  char pszStatus[3];
  char *pszClassImg;
  const char *pszOptFlag = NULL;
  int i, j;
  int nOptFlag = 15;
  int bShowGroup;

  *pszTemp = NULL;

  if(!pszGroupName || !pszGroupTemplate) {
    msSetError(MS_WEBERR, "Invalid pointer.", "generateGroupTemplate()");
    return MS_FAILURE;
  }

  /*
   * Get the opt_flag is any.
   */
  if(oGroupArgs)
    pszOptFlag = msLookupHashTable(oGroupArgs, "opt_flag");

  if(pszOptFlag)
    nOptFlag = atoi(pszOptFlag);

  /*
   * Check all layers, if one in the group
   * should be visible, print the group.
   * (Check for opt_flag)
   */
  bShowGroup = 0;
  for (j=0; j<map->numlayers; j++) {
    if(GET_LAYER(map, map->layerorder[j])->group &&
        strcmp(GET_LAYER(map, map->layerorder[j])->group, pszGroupName) == 0) {
      /* dont display layer is off. */
      if( (nOptFlag & 2) == 0 &&
          GET_LAYER(map, map->layerorder[j])->status == MS_OFF )
        bShowGroup = 0;
      else
        bShowGroup = 1;

      /* dont display layer is query. */
      if( (nOptFlag & 4) == 0  &&
          GET_LAYER(map, map->layerorder[j])->type == MS_LAYER_QUERY )
        bShowGroup = 0;

      /* dont display layer if out of scale. */
      if((nOptFlag & 1) == 0) {
        if(map->scaledenom > 0) {
          if((GET_LAYER(map, map->layerorder[j])->maxscaledenom > 0) &&
              (map->scaledenom > GET_LAYER(map, map->layerorder[j])->maxscaledenom))
            bShowGroup = 0;
          if((GET_LAYER(map, map->layerorder[j])->minscaledenom > 0) &&
              (map->scaledenom <= GET_LAYER(map, map->layerorder[j])->minscaledenom))
            bShowGroup = 0;
        }
      }

      /* The group contains one visible layer */
      /* Draw the group */
      if( bShowGroup )
        break;
    }
  }

  if( ! bShowGroup )
    return MS_SUCCESS;

  /*
   * Work from a copy
   */
  *pszTemp = (char*)msSmallMalloc(strlen(pszGroupTemplate) + 1);
  strcpy(*pszTemp, pszGroupTemplate);

  /*
   * Change group tags
   */
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_group_name]", pszGroupName);


  /*
   * Create a hash table that contain info
   * on current layer
   */
  myHashTable = msCreateHashTable();

  /*
   * Check for the first layer
   * that belong to this group.
   * Get his status and check for if.
   */
  for (j=0; j<map->numlayers; j++) {
    if(GET_LAYER(map, map->layerorder[j])->group && strcmp(GET_LAYER(map, map->layerorder[j])->group, pszGroupName) == 0) {
      snprintf(pszStatus, sizeof(pszStatus), "%d", GET_LAYER(map, map->layerorder[j])->status);
      msInsertHashTable(myHashTable, "layer_status", pszStatus);
      msInsertHashTable(myHashTable, "layer_visible", msLayerIsVisible(map, GET_LAYER(map, map->layerorder[j]))?"1":"0" );
      msInsertHashTable(myHashTable, "layer_queryable", msIsLayerQueryable(GET_LAYER(map, map->layerorder[j]))?"1":"0" );
      msInsertHashTable(myHashTable, "group_name", pszGroupName);

      if(processIfTag(pszTemp, myHashTable, MS_FALSE) != MS_SUCCESS)
        return MS_FAILURE;

      if(processIfTag(pszTemp, &(GET_LAYER(map, map->layerorder[j])->metadata), MS_FALSE) != MS_SUCCESS)
        return MS_FAILURE;

      if(processMetadata(pszTemp, &GET_LAYER(map, map->layerorder[j])->metadata) != MS_SUCCESS)
        return MS_FAILURE;

      break;
    }
  }

  msFreeHashTable(myHashTable);

  /*
   * Process all metadata tags
   * only web object is accessible
  */
  if(processMetadata(pszTemp, &(map->web.metadata)) != MS_SUCCESS)
    return MS_FAILURE;

  /*
   * check for if tag
  */
  if(processIfTag(pszTemp, &(map->web.metadata), MS_TRUE) != MS_SUCCESS)
    return MS_FAILURE;

  /*
   * Check if leg_icon tag exist
   * if so display the first layer first class icon
   */
  pszClassImg = strstr(*pszTemp, "[leg_icon");
  if(pszClassImg) {
    /* find first layer of this group */
    for (i=0; i<map->numlayers; i++)
      if(GET_LAYER(map, map->layerorder[i])->group && strcmp(GET_LAYER(map, map->layerorder[i])->group, pszGroupName) == 0)
        processIcon(map, map->layerorder[i], 0, pszTemp, pszPrefix);
  }

  return MS_SUCCESS;
}

/*!
 * Replace all tags from layer template
 * with correct value.
 *
 * this function return a buffer containing
 * the template with correct values.
 *
 * buffer must be freed by caller.
*/
int generateLayerTemplate(char *pszLayerTemplate, mapObj *map, int nIdxLayer, hashTableObj *oLayerArgs, char **pszTemp, char* pszPrefix)
{
  hashTableObj *myHashTable;
  char szStatus[10];
  char szType[10];

  int nOptFlag=0;
  const char *pszOptFlag = NULL;
  char *pszClassImg;

  char szTmpstr[128]; /* easily big enough for the couple of instances we need */

  *pszTemp = NULL;

  if(!pszLayerTemplate ||
      !map ||
      nIdxLayer > map->numlayers ||
      nIdxLayer < 0 ) {
    msSetError(MS_WEBERR, "Invalid pointer.", "generateLayerTemplate()");
    return MS_FAILURE;
  }

  if(oLayerArgs)
    pszOptFlag = msLookupHashTable(oLayerArgs, "opt_flag");

  if(pszOptFlag)
    nOptFlag = atoi(pszOptFlag);

  /* don't display deleted layers */
  if(GET_LAYER(map, nIdxLayer)->status == MS_DELETE)
    return MS_SUCCESS;

  /* dont display layer is off. */
  /* check this if Opt flag is not set */
  if((nOptFlag & 2) == 0 && GET_LAYER(map, nIdxLayer)->status == MS_OFF)
    return MS_SUCCESS;

  /* dont display layer is query. */
  /* check this if Opt flag is not set */
  if((nOptFlag & 4) == 0  && GET_LAYER(map, nIdxLayer)->type == MS_LAYER_QUERY)
    return MS_SUCCESS;

  /* dont display layer if out of scale. */
  /* check this if Opt flag is not set             */
  if((nOptFlag & 1) == 0) {
    if(map->scaledenom > 0) {
      if((GET_LAYER(map, nIdxLayer)->maxscaledenom > 0) && (map->scaledenom > GET_LAYER(map, nIdxLayer)->maxscaledenom))
        return MS_SUCCESS;
      if((GET_LAYER(map, nIdxLayer)->minscaledenom > 0) && (map->scaledenom <= GET_LAYER(map, nIdxLayer)->minscaledenom))
        return MS_SUCCESS;
    }
  }

  /*
   * Work from a copy
   */
  *pszTemp = msStrdup(pszLayerTemplate);

  /*
   * Change layer tags
   */
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_name]", GET_LAYER(map, nIdxLayer)->name);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_group]", GET_LAYER(map, nIdxLayer)->group);

  snprintf(szTmpstr, sizeof(szTmpstr), "%d", nIdxLayer);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_index]", szTmpstr);

  snprintf(szTmpstr, sizeof(szTmpstr), "%g", GET_LAYER(map, nIdxLayer)->minscaledenom);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_minscale]", szTmpstr);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_minscaledenom]", szTmpstr);
  snprintf(szTmpstr, sizeof(szTmpstr), "%g", GET_LAYER(map, nIdxLayer)->maxscaledenom);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_maxscale]", szTmpstr);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_maxscaledenom]", szTmpstr);

  /*
   * Create a hash table that contain info
   * on current layer
   */
  myHashTable = msCreateHashTable();

  /*
   * for now, only status and type is required by template
   */
  snprintf(szStatus, sizeof(szStatus), "%d", GET_LAYER(map, nIdxLayer)->status);
  msInsertHashTable(myHashTable, "layer_status", szStatus);

  snprintf(szType, sizeof(szType), "%d", GET_LAYER(map, nIdxLayer)->type);
  msInsertHashTable(myHashTable, "layer_type", szType);

  msInsertHashTable(myHashTable, "layer_name", (GET_LAYER(map, nIdxLayer)->name)? GET_LAYER(map, nIdxLayer)->name : "");
  msInsertHashTable(myHashTable, "layer_group", (GET_LAYER(map, nIdxLayer)->group)? GET_LAYER(map, nIdxLayer)->group : "");
  msInsertHashTable(myHashTable, "layer_visible", msLayerIsVisible(map, GET_LAYER(map, nIdxLayer))?"1":"0" );
  msInsertHashTable(myHashTable, "layer_queryable", msIsLayerQueryable(GET_LAYER(map, nIdxLayer))?"1":"0" );

  if(processIfTag(pszTemp, myHashTable, MS_FALSE) != MS_SUCCESS)
    return MS_FAILURE;

  if(processIfTag(pszTemp, &(GET_LAYER(map, nIdxLayer)->metadata), MS_FALSE) != MS_SUCCESS)
    return MS_FAILURE;

  if(processIfTag(pszTemp, &(map->web.metadata), MS_TRUE) != MS_SUCCESS)
    return MS_FAILURE;

  msFreeHashTable(myHashTable);

  /*
   * Check if leg_icon tag exist
   * if so display the first class icon
   */
  pszClassImg = strstr(*pszTemp, "[leg_icon");
  if(pszClassImg) {
    processIcon(map, nIdxLayer, 0, pszTemp, pszPrefix);
  }

  /* process all metadata tags
   * only current layer and web object
   * metaddata are accessible
  */
  if(processMetadata(pszTemp, &GET_LAYER(map, nIdxLayer)->metadata) != MS_SUCCESS)
    return MS_FAILURE;

  if(processMetadata(pszTemp, &(map->web.metadata)) != MS_SUCCESS)
    return MS_FAILURE;

  return MS_SUCCESS;
}

/*!
 * Replace all tags from class template
 * with correct value.
 *
 * this function return a buffer containing
 * the template with correct values.
 *
 * buffer must be freed by caller.
*/
int generateClassTemplate(char* pszClassTemplate, mapObj *map, int nIdxLayer, int nIdxClass, hashTableObj *oClassArgs, char **pszTemp, char* pszPrefix)
{
  hashTableObj *myHashTable;
  char szStatus[10];
  char szType[10];

  char *pszClassImg;
  int nOptFlag=0;
  const char *pszOptFlag = NULL;

  char szTmpstr[128]; /* easily big enough for the couple of instances we need */

  *pszTemp = NULL;

  if(!pszClassTemplate ||
      !map ||
      nIdxLayer > map->numlayers ||
      nIdxLayer < 0 ||
      nIdxClass > GET_LAYER(map, nIdxLayer)->numclasses ||
      nIdxClass < 0) {

    msSetError(MS_WEBERR, "Invalid pointer.", "generateClassTemplate()");
    return MS_FAILURE;
  }

  if(oClassArgs)
    pszOptFlag = msLookupHashTable(oClassArgs, "Opt_flag");

  if(pszOptFlag)
    nOptFlag = atoi(pszOptFlag);

  /* don't display deleted layers */
  if(GET_LAYER(map, nIdxLayer)->status == MS_DELETE)
    return MS_SUCCESS;

  /* dont display class if layer is off. */
  /* check this if Opt flag is not set */
  if((nOptFlag & 2) == 0 && GET_LAYER(map, nIdxLayer)->status == MS_OFF)
    return MS_SUCCESS;

  /* dont display class if layer is query. */
  /* check this if Opt flag is not set       */
  if((nOptFlag & 4) == 0 && GET_LAYER(map, nIdxLayer)->type == MS_LAYER_QUERY)
    return MS_SUCCESS;

  /* dont display layer if out of scale. */
  /* check this if Opt flag is not set */
  if((nOptFlag & 1) == 0) {
    if(map->scaledenom > 0) {
      if((GET_LAYER(map, nIdxLayer)->maxscaledenom > 0) && (map->scaledenom > GET_LAYER(map, nIdxLayer)->maxscaledenom))
        return MS_SUCCESS;
      if((GET_LAYER(map, nIdxLayer)->minscaledenom > 0) && (map->scaledenom <= GET_LAYER(map, nIdxLayer)->minscaledenom))
        return MS_SUCCESS;
    }
  }

  /*
   * Work from a copy
   */
  *pszTemp = (char*)msSmallMalloc(strlen(pszClassTemplate) + 1);
  strcpy(*pszTemp, pszClassTemplate);

  /*
   * Change class tags
   */
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_class_name]", GET_LAYER(map, nIdxLayer)->class[nIdxClass]->name);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_class_title]", GET_LAYER(map, nIdxLayer)->class[nIdxClass]->title);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_layer_name]", GET_LAYER(map, nIdxLayer)->name);

  snprintf(szTmpstr, sizeof(szTmpstr), "%d", nIdxClass);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_class_index]", szTmpstr);

  snprintf(szTmpstr, sizeof(szTmpstr), "%g", GET_LAYER(map, nIdxLayer)->class[nIdxClass]->minscaledenom);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_class_minscale]", szTmpstr);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_class_minscaledenom]", szTmpstr);
  snprintf(szTmpstr, sizeof(szTmpstr), "%g", GET_LAYER(map, nIdxLayer)->class[nIdxClass]->maxscaledenom);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_class_maxscale]", szTmpstr);
  *pszTemp = msReplaceSubstring(*pszTemp, "[leg_class_maxscaledenom]", szTmpstr);

  /*
   * Create a hash table that contain info
   * on current layer
   */
  myHashTable = msCreateHashTable();

  /*
   * for now, only status, type, name and group are  required by template
   */
  snprintf(szStatus, sizeof(szStatus), "%d", GET_LAYER(map, nIdxLayer)->status);
  msInsertHashTable(myHashTable, "layer_status", szStatus);

  snprintf(szType, sizeof(szType), "%d", GET_LAYER(map, nIdxLayer)->type);
  msInsertHashTable(myHashTable, "layer_type", szType);

  msInsertHashTable(myHashTable, "layer_name",
                    (GET_LAYER(map, nIdxLayer)->name)? GET_LAYER(map, nIdxLayer)->name : "");
  msInsertHashTable(myHashTable, "layer_group",
                    (GET_LAYER(map, nIdxLayer)->group)? GET_LAYER(map, nIdxLayer)->group : "");
  msInsertHashTable(myHashTable, "layer_visible", msLayerIsVisible(map, GET_LAYER(map, nIdxLayer))?"1":"0" );
  msInsertHashTable(myHashTable, "layer_queryable", msIsLayerQueryable(GET_LAYER(map, nIdxLayer))?"1":"0" );
  msInsertHashTable(myHashTable, "class_name",
                    (GET_LAYER(map, nIdxLayer)->class[nIdxClass]->name)? GET_LAYER(map, nIdxLayer)->class[nIdxClass]->name : "");

  if(processIfTag(pszTemp, myHashTable, MS_FALSE) != MS_SUCCESS)
    return MS_FAILURE;

  if(processIfTag(pszTemp, &(GET_LAYER(map, nIdxLayer)->metadata), MS_FALSE) != MS_SUCCESS)
    return MS_FAILURE;

  if(processIfTag(pszTemp, &(map->web.metadata), MS_TRUE) != MS_SUCCESS)
    return MS_FAILURE;

  msFreeHashTable(myHashTable);

  /*
   * Check if leg_icon tag exist
   */
  pszClassImg = strstr(*pszTemp, "[leg_icon");
  if(pszClassImg) {
    processIcon(map, nIdxLayer, nIdxClass, pszTemp, pszPrefix);
  }

  /* process all metadata tags
   * only current layer and web object
   * metaddata are accessible
  */
  if(processMetadata(pszTemp, &GET_LAYER(map, nIdxLayer)->metadata) != MS_SUCCESS)
    return MS_FAILURE;

  if(processMetadata(pszTemp, &(map->web.metadata)) != MS_SUCCESS)
    return MS_FAILURE;

  return MS_SUCCESS;
}

char *generateLegendTemplate(mapservObj *mapserv)
{
  FILE *stream;
  char *file = NULL;
  int length;
  char *pszResult = NULL;
  char *legGroupHtml = NULL;
  char *legLayerHtml = NULL;
  char *legClassHtml = NULL;
  char *legLayerHtmlCopy = NULL;
  char *legClassHtmlCopy = NULL;
  char *legGroupHtmlCopy = NULL;

  char *legHeaderHtml = NULL;
  char *legFooterHtml = NULL;

  char *pszPrefix = NULL;
  char *pszMapFname = NULL;

  struct stat tmpStat;

  const char *pszOrderMetadata = NULL;
  const char *pszOrder = NULL;

  int i,j,k;
  char **papszGroups = NULL;
  int nGroupNames = 0;

  const char *pszOrderValue;

  hashTableObj *groupArgs = NULL;
  hashTableObj *layerArgs = NULL;
  hashTableObj *classArgs = NULL;

  ms_regex_t re; /* compiled regular expression to be matched */

  int  *panCurrentDrawingOrder = NULL;
  char szPath[MS_MAXPATHLEN];

  if(ms_regcomp(&re, MS_TEMPLATE_EXPR, MS_REG_EXTENDED|MS_REG_NOSUB|MS_REG_ICASE) != 0) {
    msSetError(MS_IOERR, "Error regcomp.", "generateLegendTemplate()");
    return NULL;
  }

  if(ms_regexec(&re, mapserv->map->legend.template, 0, NULL, 0) != 0) { /* no match */
    msSetError(MS_IOERR, "Invalid template file name.", "generateLegendTemplate()");
    ms_regfree(&re);
    return NULL;
  }
  ms_regfree(&re);

  /* -------------------------------------------------------------------- */
  /*      Save the current drawing order. The drawing order is reset      */
  /*      at the end of the function.                                     */
  /* -------------------------------------------------------------------- */
  if(mapserv->map->numlayers > 0) {
    panCurrentDrawingOrder =
      (int *)msSmallMalloc(sizeof(int)*mapserv->map->numlayers);

    for (i=0; i<mapserv->map->numlayers; i++) {
      if(mapserv->map->layerorder)
        panCurrentDrawingOrder[i] = mapserv->map->layerorder[i];
      else
        panCurrentDrawingOrder[i] = i;
    }
  }

  /*
   * build prefix filename
   * for legend icon creation
  */
  for(i=0; i<mapserv->request->NumParams; i++) /* find the mapfile parameter first */
    if(strcasecmp(mapserv->request->ParamNames[i], "map") == 0) break;

  if(i == mapserv->request->NumParams) {
    const char *ms_mapfile = CPLGetConfigOption("MS_MAPFILE", NULL);
    if(ms_mapfile)
      pszMapFname = msStringConcatenate(pszMapFname, ms_mapfile);
  } else {
    if(getenv(mapserv->request->ParamValues[i])) /* an environment references the actual file to use */
      pszMapFname = msStringConcatenate(pszMapFname, getenv(mapserv->request->ParamValues[i]));
    else
      pszMapFname = msStringConcatenate(pszMapFname, mapserv->request->ParamValues[i]);
  }

  if(pszMapFname) {
    if(stat(pszMapFname, &tmpStat) != -1) {
      int nLen;

      nLen = (mapserv->map->name?strlen(mapserv->map->name):0)  + 50;
      pszPrefix = (char*)msSmallMalloc((nLen+1) * sizeof(char));
      snprintf(pszPrefix, nLen, "%s_%ld_%ld",
               mapserv->map->name,
               (long) tmpStat.st_size,
               (long) tmpStat.st_mtime);
      pszPrefix[nLen] = '\0';
    }

    free(pszMapFname);
    pszMapFname = NULL;
  } else {
    /* -------------------------------------------------------------------- */
    /*      map file name may not be avaible when the template functions    */
    /*      are called from mapscript. Use the time stamp as prefix.        */
    /* -------------------------------------------------------------------- */
    char pszTime[20];

    snprintf(pszTime, sizeof(pszTime), "%ld", (long)time(NULL));
    pszPrefix = msStringConcatenate(pszPrefix, pszTime);
  }

  /* open template */
  if((stream = fopen(msBuildPath(szPath, mapserv->map->mappath, mapserv->map->legend.template), "r")) == NULL) {
    msSetError(MS_IOERR, "Error while opening template file.", "generateLegendTemplate()");
    free(pszResult);
    pszResult=NULL;
    goto error;
  }

  fseek(stream, 0, SEEK_END);
  long lengthLong = ftell(stream);
  rewind(stream);
  if( lengthLong < 0 || lengthLong > INT_MAX - 1 )
  {
    msSetError(MS_IOERR, "Too large template file.", "generateLegendTemplate()");
    free(pszResult);
    pszResult=NULL;
    goto error;
  }
  length = (int)lengthLong;

  file = (char*)malloc(length + 1);
  if( file == NULL )
  {
    msSetError(MS_IOERR, "Cannot allocate memory for template file.", "generateLegendTemplate()");
    free(pszResult);
    pszResult=NULL;
    goto error;
  }

  /*
   * Read all the template file
   */
  IGUR_sizet(fread(file, length, 1, stream));
  /* E. Rouault: the below issue is due to opening in "r" mode, which is a
   * synonymous of "rt" on Windows. In that mode \r\n are turned into \n,
   * consequently less bytes are written in the output buffer than requested.
   * A potential fix might be to open in "rb" mode, but is the code ready
   * to deal with Windows \r\n end of lines ? */
  /* Disabled for now due to Windows issue, see ticket #3814
     if( 1 != fread(file, length, 1, stream)) {
       msSetError(MS_IOERR, "Error while reading template file.", "generateLegendTemplate()");
       free(file);
       fclose(stream);
       return NULL;
     }
  */
  file[length] = '\0';

  if(msValidateContexts(mapserv->map) != MS_SUCCESS) { /* make sure there are no recursive REQUIRES or LABELREQUIRES expressions */
    if(pszResult)
      free(pszResult);
    pszResult=NULL;
    goto error;
  }

  /*
   * Seperate header/footer, groups, layers and class
   */
  getInlineTag("leg_header_html", file, &legHeaderHtml);
  getInlineTag("leg_footer_html", file, &legFooterHtml);
  getInlineTag("leg_group_html", file, &legGroupHtml);
  getInlineTag("leg_layer_html", file, &legLayerHtml);
  getInlineTag("leg_class_html", file, &legClassHtml);

  /*
   * Retrieve arguments of all three parts
   */
  if(legGroupHtml)
    if(getTagArgs("leg_group_html", file, &groupArgs) != MS_SUCCESS) {
      if(pszResult)
        free(pszResult);
      pszResult=NULL;
      goto error;
    }

  if(legLayerHtml)
    if(getTagArgs("leg_layer_html", file, &layerArgs) != MS_SUCCESS) {
      if(pszResult)
        free(pszResult);
      pszResult=NULL;
      goto error;
    }

  if(legClassHtml)
    if(getTagArgs("leg_class_html", file, &classArgs) != MS_SUCCESS) {
      if(pszResult)
        free(pszResult);
      pszResult=NULL;
      goto error;
    }


  mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent),
                                          mapserv->map->width,
                                          mapserv->map->height);
  if(msCalculateScale(mapserv->map->extent, mapserv->map->units,
                      mapserv->map->width, mapserv->map->height,
                      mapserv->map->resolution, &mapserv->map->scaledenom) != MS_SUCCESS) {
    if(pszResult)
      free(pszResult);
    pszResult=NULL;
    goto error;
  }

  /* start with the header if present */
  if(legHeaderHtml) pszResult = msStringConcatenate(pszResult, legHeaderHtml);

  /********************************************************************/

  /*
   * order layers if order_metadata args is set
   * If not, keep default order
   */
  pszOrderMetadata = msLookupHashTable(layerArgs, "order_metadata");

  if(sortLayerByMetadata(mapserv->map, pszOrderMetadata) != MS_SUCCESS) {
    if(pszResult)
      free(pszResult);
    pszResult=NULL;
    goto error;
  }

  /* -------------------------------------------------------------------- */
  /*      if the order tag is set to ascending or descending, the         */
  /*      current order will be changed to correspond to that.            */
  /* -------------------------------------------------------------------- */
  pszOrder = msLookupHashTable(layerArgs, "order");
  if(pszOrder && ((strcasecmp(pszOrder, "ASCENDING") == 0) ||
                  (strcasecmp(pszOrder, "DESCENDING") == 0))) {
    if(sortLayerByOrder(mapserv->map, pszOrder) != MS_SUCCESS) {
      if(pszResult)
        free(pszResult);
      pszResult=NULL;
      goto error;
    }
  }

  if(legGroupHtml) {
    /* retrieve group names */
    papszGroups = msGetAllGroupNames(mapserv->map, &nGroupNames);

    for (i=0; i<nGroupNames; i++) {
      /* process group tags */
      if(generateGroupTemplate(legGroupHtml, mapserv->map, papszGroups[i], groupArgs, &legGroupHtmlCopy, pszPrefix) != MS_SUCCESS) {
        if(pszResult)
          free(pszResult);
        pszResult=NULL;
        goto error;
      }

      /* concatenate it to final result */
      pszResult = msStringConcatenate(pszResult, legGroupHtmlCopy);

      /*
               if(!pszResult)
               {
                  if(pszResult)
                    free(pszResult);
                  pszResult=NULL;
                  goto error;
               }
      */

      if(legGroupHtmlCopy) {
        free(legGroupHtmlCopy);
        legGroupHtmlCopy = NULL;
      }

      /* for all layers in group */
      if(legLayerHtml) {
        for (j=0; j<mapserv->map->numlayers; j++) {
          /*
           * if order_metadata is set and the order
           * value is less than 0, dont display it
           */
          pszOrderMetadata = msLookupHashTable(layerArgs, "order_metadata");
          if(pszOrderMetadata) {
            pszOrderValue = msLookupHashTable(&(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->metadata), pszOrderMetadata);
            if(pszOrderValue) {
              const int nLegendOrder = atoi(pszOrderValue);
              if(nLegendOrder < 0)
                continue;
            }
          }
          if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].status == 0) {
            continue;
          }

          if(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->group && strcmp(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->group, papszGroups[i]) == 0) {
            /* process all layer tags */
            if(generateLayerTemplate(legLayerHtml, mapserv->map, mapserv->map->layerorder[j], layerArgs, &legLayerHtmlCopy, pszPrefix) != MS_SUCCESS) {
              if(pszResult)
                free(pszResult);
              pszResult=NULL;
              goto error;
            }


            /* concatenate to final result */
            pszResult = msStringConcatenate(pszResult, legLayerHtmlCopy);

            if(legLayerHtmlCopy) {
              free(legLayerHtmlCopy);
              legLayerHtmlCopy = NULL;
            }


            /* for all classes in layer */
            if(legClassHtml) {
              for (k=0; k<GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->numclasses; k++) {
                /* process all class tags */
                if(!GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->class[k]->name)
                  continue;
                if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].classhits[k].status == 0) {
                  continue;
                }

                if(generateClassTemplate(legClassHtml, mapserv->map, mapserv->map->layerorder[j], k, classArgs, &legClassHtmlCopy, pszPrefix) != MS_SUCCESS) {
                  if(pszResult)
                    free(pszResult);
                  pszResult=NULL;
                  goto error;
                }


                /* concatenate to final result */
                pszResult = msStringConcatenate(pszResult, legClassHtmlCopy);

                if(legClassHtmlCopy) {
                  free(legClassHtmlCopy);
                  legClassHtmlCopy = NULL;
                }
              }
            }
          }
        }
      } else if(legClassHtml) { /* no layer template specified but class and group template */
        for (j=0; j<mapserv->map->numlayers; j++) {
          /*
           * if order_metadata is set and the order
           * value is less than 0, dont display it
           */
          pszOrderMetadata = msLookupHashTable(layerArgs, "order_metadata");
          if(pszOrderMetadata) {
            pszOrderValue = msLookupHashTable(&(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->metadata), pszOrderMetadata);
            if(pszOrderValue) {
              const int nLegendOrder = atoi(pszOrderValue);
              if(nLegendOrder < 0)
                continue;
            }
          }
          if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].status == 0) {
            continue;
          }

          if(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->group && strcmp(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->group, papszGroups[i]) == 0) {
            /* for all classes in layer */

              for (k=0; k<GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->numclasses; k++) {
                /* process all class tags */
                if(!GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->class[k]->name)
                  continue;
                if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].classhits[k].status == 0) {
                  continue;
                }

                if(generateClassTemplate(legClassHtml, mapserv->map, mapserv->map->layerorder[j], k, classArgs, &legClassHtmlCopy, pszPrefix) != MS_SUCCESS) {
                  if(pszResult)
                    free(pszResult);
                  pszResult=NULL;
                  goto error;
                }


                /* concatenate to final result */
                pszResult = msStringConcatenate(pszResult, legClassHtmlCopy);

                if(legClassHtmlCopy) {
                  free(legClassHtmlCopy);
                  legClassHtmlCopy = NULL;
                }
              }

          }
        }
      }
    }
  } else {
    /* if no group template specified */
    if(legLayerHtml) {
      for (j=0; j<mapserv->map->numlayers; j++) {
        /*
         * if order_metadata is set and the order
         * value is less than 0, dont display it
         */
        pszOrderMetadata = msLookupHashTable(layerArgs, "order_metadata");
        if(pszOrderMetadata) {
          pszOrderValue = msLookupHashTable(&(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->metadata), pszOrderMetadata);
          if(pszOrderValue) {
            const int nLegendOrder = atoi(pszOrderValue);
            if(nLegendOrder < 0)
              continue;
          }
        }
        if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].status == 0) {
          continue;
        }

        /* process a layer tags */
        if(generateLayerTemplate(legLayerHtml, mapserv->map, mapserv->map->layerorder[j], layerArgs, &legLayerHtmlCopy, pszPrefix) != MS_SUCCESS) {
          if(pszResult)
            free(pszResult);
          pszResult=NULL;
          goto error;
        }

        /* concatenate to final result */
        pszResult = msStringConcatenate(pszResult, legLayerHtmlCopy);

        if(legLayerHtmlCopy) {
          free(legLayerHtmlCopy);
          legLayerHtmlCopy = NULL;
        }

        /* for all classes in layer */
        if(legClassHtml) {
          for (k=0; k<GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->numclasses; k++) {
            /* process all class tags */
            if(!GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->class[k]->name)
              continue;
            if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].classhits[k].status == 0) {
              continue;
            }

            if(generateClassTemplate(legClassHtml, mapserv->map, mapserv->map->layerorder[j], k, classArgs, &legClassHtmlCopy, pszPrefix) != MS_SUCCESS) {
              if(pszResult)
                free(pszResult);
              pszResult=NULL;
              goto error;
            }


            /* concatenate to final result */
            pszResult = msStringConcatenate(pszResult, legClassHtmlCopy);

            if(legClassHtmlCopy) {
              free(legClassHtmlCopy);
              legClassHtmlCopy = NULL;
            }
          }
        }
      }
    } else { /* if no group and layer template specified */
      if(legClassHtml) {
        for (j=0; j<mapserv->map->numlayers; j++) {
          /*
           * if order_metadata is set and the order
           * value is less than 0, dont display it
           */
          pszOrderMetadata = msLookupHashTable(layerArgs, "order_metadata");
          if(pszOrderMetadata) {
            pszOrderValue = msLookupHashTable(&(GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->metadata), pszOrderMetadata);
            if(pszOrderValue) {
              const int nLegendOrder = atoi(pszOrderValue);
              if(nLegendOrder < 0)
                continue;
            }
          }
          if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].status == 0) {
            continue;
          }

          for (k=0; k<GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->numclasses; k++) {
            if(!GET_LAYER(mapserv->map, mapserv->map->layerorder[j])->class[k]->name)
              continue;
            if(mapserv->hittest && mapserv->hittest->layerhits[mapserv->map->layerorder[j]].classhits[k].status == 0) {
              continue;
            }

            if(generateClassTemplate(legClassHtml, mapserv->map, mapserv->map->layerorder[j], k, classArgs, &legClassHtmlCopy, pszPrefix) != MS_SUCCESS) {
              if(pszResult)
                free(pszResult);
              pszResult=NULL;
              goto error;
            }


            pszResult = msStringConcatenate(pszResult, legClassHtmlCopy);

            if(legClassHtmlCopy) {
              free(legClassHtmlCopy);
              legClassHtmlCopy = NULL;
            }
          }
        }
      }
    }
  }

  /* finish with the footer if present */
  if(legFooterHtml) pszResult = msStringConcatenate(pszResult, legFooterHtml);

  /*
   * if we reach this point, that mean no error was generated.
   * So check if template is null and initialize it to <space>.
   */
  if(pszResult == NULL) {
    pszResult = msStringConcatenate(pszResult, " ");
  }


  /********************************************************************/

error:

  if(papszGroups) {
    for (i=0; i<nGroupNames; i++)
      msFree(papszGroups[i]);

    msFree(papszGroups);
  }

  msFreeHashTable(groupArgs);
  msFreeHashTable(layerArgs);
  msFreeHashTable(classArgs);

  msFree(file);

  msFree(legGroupHtmlCopy);
  msFree(legLayerHtmlCopy);
  msFree(legClassHtmlCopy);

  msFree(legHeaderHtml);
  msFree(legFooterHtml);

  msFree(legGroupHtml);
  msFree(legLayerHtml);
  msFree(legClassHtml);
  msFree(pszPrefix);

  if(stream)
    fclose(stream);

  /* -------------------------------------------------------------------- */
  /*      Reset the layerdrawing order.                                   */
  /* -------------------------------------------------------------------- */
  if(panCurrentDrawingOrder && mapserv->map->layerorder) {
    for (i=0; i<mapserv->map->numlayers; i++)
      mapserv->map->layerorder[i] =  panCurrentDrawingOrder[i];

    free(panCurrentDrawingOrder);
  }

  return pszResult;
}

char *processOneToManyJoin(mapservObj* mapserv, joinObj *join)
{
  int records=MS_FALSE;
  FILE *stream=NULL;
  char *outbuf;
  char line[MS_BUFFER_LENGTH], *tmpline;
  char szPath[MS_MAXPATHLEN];

  if((outbuf = msStrdup("")) == NULL) return(NULL); /* empty at first */

  msJoinPrepare(join, &(mapserv->resultshape)); /* execute the join */
  while(msJoinNext(join) == MS_SUCCESS) {
    /* First time through, deal with the header (if necessary) and open the main template. We only */
    /* want to do this if there are joined records. */
    if(records == MS_FALSE) {
      if(join->header != NULL) {
        /* coverity[dead_error_line] */
        if(stream) fclose(stream);
        if((stream = fopen(msBuildPath(szPath, mapserv->map->mappath, join->header), "r")) == NULL) {
          msSetError(MS_IOERR, "Error while opening join header file %s.", "processOneToManyJoin()", join->header);
          msFree(outbuf);
          return(NULL);
        }

        if(isValidTemplate(stream, join->header) != MS_TRUE) {
          fclose(stream);
          msFree(outbuf);
          return NULL;
        }

        /* echo file to the output buffer, no substitutions */
        while(fgets(line, MS_BUFFER_LENGTH, stream) != NULL) outbuf = msStringConcatenate(outbuf, line);

        fclose(stream);
        stream = NULL;
      }

      if((stream = fopen(msBuildPath(szPath, mapserv->map->mappath, join->template), "r")) == NULL) {
        msSetError(MS_IOERR, "Error while opening join template file %s.", "processOneToManyJoin()", join->template);
        msFree(outbuf);
        return(NULL);
      }

      if(isValidTemplate(stream, join->template) != MS_TRUE) {
        fclose(stream);
        msFree(outbuf);
        return NULL;
      }

      records = MS_TRUE;
    }

    while(fgets(line, MS_BUFFER_LENGTH, stream) != NULL) { /* now on to the end of the template */
      if(strchr(line, '[') != NULL) {
        tmpline = processLine(mapserv, line, NULL, QUERY); /* no multiline tags are allowed in a join */
        if(!tmpline) {
           msFree(outbuf);
           fclose(stream);
           return NULL;
        }
        outbuf = msStringConcatenate(outbuf, tmpline);
        free(tmpline);
      } else /* no subs, just echo */
        outbuf = msStringConcatenate(outbuf, line);
    }

    rewind(stream);
    IGUR_voidp(fgets(line, MS_BUFFER_LENGTH, stream)); /* skip the first line since it's the magic string */
  } /* next record */

  if(records==MS_TRUE && join->footer) {
    if(stream) fclose(stream);
    if((stream = fopen(msBuildPath(szPath, mapserv->map->mappath, join->footer), "r")) == NULL) {
      msSetError(MS_IOERR, "Error while opening join footer file %s.", "processOneToManyJoin()", join->footer);
      msFree(outbuf);
      return(NULL);
    }

    if(isValidTemplate(stream, join->footer) != MS_TRUE) {
      msFree(outbuf);
      fclose(stream);
      return NULL;
    }

    /* echo file to the output buffer, no substitutions */
    while(fgets(line, MS_BUFFER_LENGTH, stream) != NULL) outbuf = msStringConcatenate(outbuf, line);

    fclose(stream);
    stream = NULL;
  }

  /* clear any data associated with the join */
  msFreeCharArray(join->values, join->numitems);
  join->values = NULL;

  if(stream) fclose(stream);

  return(outbuf);
}

/*
** Process a single line in the template. A few tags (e.g. [resultset]...[/resultset]) can be multi-line so
** we pass the filehandle to look ahead if necessary.
*/
static char *processLine(mapservObj *mapserv, const char *instr, FILE *stream, int mode)
{
  int i, j;
#define PROCESSLINE_BUFLEN 5120
  char repstr[PROCESSLINE_BUFLEN], substr[PROCESSLINE_BUFLEN], *outstr; /* repstr = replace string, substr = sub string */
  struct hashObj *tp=NULL;
  char *encodedstr;

  rectObj llextent;
  pointObj llpoint;

  outstr = msStrdup(instr); /* work from a copy */

  if(strstr(outstr, "[version]")) outstr = msReplaceSubstring(outstr, "[version]",  msGetVersion());

  snprintf(repstr, PROCESSLINE_BUFLEN, "%s%s%s.%s", mapserv->map->web.imageurl, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));
  outstr = msReplaceSubstring(outstr, "[img]", repstr);
  snprintf(repstr, PROCESSLINE_BUFLEN, "%s%sref%s.%s", mapserv->map->web.imageurl, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));
  outstr = msReplaceSubstring(outstr, "[ref]", repstr);

  if(strstr(outstr, "[errmsg")) {
    char *errmsg = msGetErrorString(";");
    if(!errmsg) errmsg = msStrdup("Error message buffer is empty."); /* should never happen, but just in case... */
    outstr = msReplaceSubstring(outstr, "[errmsg]", errmsg);
    encodedstr = msEncodeUrl(errmsg);
    outstr = msReplaceSubstring(outstr, "[errmsg_esc]", encodedstr);
    free(errmsg);
    free(encodedstr);
  }

  if(strstr(outstr, "[legend]")) {
    /* if there's a template legend specified, use it */
    if(mapserv->map->legend.template) {
      char *legendTemplate;

      legendTemplate = generateLegendTemplate(mapserv);
      if(legendTemplate) {
        outstr = msReplaceSubstring(outstr, "[legend]", legendTemplate);

        free(legendTemplate);
      } else /* error already generated by (generateLegendTemplate()) */
        return NULL;
    } else { /* if not display gif image with all legend icon */
      snprintf(repstr, PROCESSLINE_BUFLEN, "%s%sleg%s.%s", mapserv->map->web.imageurl, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));
      outstr = msReplaceSubstring(outstr, "[legend]", repstr);
    }
  }

  snprintf(repstr, PROCESSLINE_BUFLEN, "%s%ssb%s.%s", mapserv->map->web.imageurl, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));
  outstr = msReplaceSubstring(outstr, "[scalebar]", repstr);

  if(mapserv->savequery) {
    snprintf(repstr, PROCESSLINE_BUFLEN, "%s%s%s%s", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id, MS_QUERY_EXTENSION);
    outstr = msReplaceSubstring(outstr, "[queryfile]", repstr);
  }

  if(mapserv->savemap) {
    snprintf(repstr, PROCESSLINE_BUFLEN, "%s%s%s.map", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id);
    outstr = msReplaceSubstring(outstr, "[map]", repstr);
  }

  if(strstr(outstr,"[mapserv_onlineresource]")) {
    char *ol;
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
    ol = msOWSGetOnlineResource(mapserv->map, "O", "onlineresource", mapserv->request);
#else
    ol = msBuildOnlineResource(mapserv->map, mapserv->request);
#endif
    outstr = msReplaceSubstring(outstr, "[mapserv_onlineresource]",ol);
    msFree(ol);
  }

  if(getenv("HTTP_HOST")) {
    snprintf(repstr, PROCESSLINE_BUFLEN, "%s", getenv("HTTP_HOST"));
    outstr = msReplaceSubstring(outstr, "[host]", repstr);
  }
  if(getenv("SERVER_PORT")) {
    snprintf(repstr, PROCESSLINE_BUFLEN, "%s", getenv("SERVER_PORT"));
    outstr = msReplaceSubstring(outstr, "[port]", repstr);
  }

  snprintf(repstr, PROCESSLINE_BUFLEN, "%s", mapserv->Id);
  outstr = msReplaceSubstring(outstr, "[id]", repstr);

  repstr[0] = '\0'; /* Layer list for a "POST" request */
  for(i=0; i<mapserv->NumLayers; i++) {
    strlcat(repstr, mapserv->Layers[i], sizeof(repstr));
    strlcat(repstr, " ", sizeof(repstr));
  }
  msStringTrimBlanks(repstr);
  encodedstr = msEncodeHTMLEntities(repstr);
  outstr = msReplaceSubstring(outstr, "[layers]", encodedstr);
  free(encodedstr);

  encodedstr = msEncodeUrl(repstr);
  outstr = msReplaceSubstring(outstr, "[layers_esc]", encodedstr);
  free(encodedstr);

  strcpy(repstr, ""); /* list of ALL layers that can be toggled */
  repstr[0] = '\0';
  for(i=0; i<mapserv->map->numlayers; i++) {
    if(GET_LAYER(mapserv->map, i)->status != MS_DEFAULT && GET_LAYER(mapserv->map, i)->name != NULL) {
      strlcat(repstr, GET_LAYER(mapserv->map, i)->name, sizeof(repstr));
      strlcat(repstr, " ", sizeof(repstr));
    }
  }
  msStringTrimBlanks(repstr);
  outstr = msReplaceSubstring(outstr, "[toggle_layers]", repstr);

  encodedstr = msEncodeUrl(repstr);
  outstr = msReplaceSubstring(outstr, "[toggle_layers_esc]", encodedstr);
  free(encodedstr);

  for(i=0; i<mapserv->map->numlayers; i++) { /* Set form widgets (i.e. checkboxes, radio and select lists), note that default layers don't show up here */
    if(isOn(mapserv, GET_LAYER(mapserv->map, i)->name, GET_LAYER(mapserv->map, i)->group) == MS_TRUE) {
      if(GET_LAYER(mapserv->map, i)->group) {
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_select]", GET_LAYER(mapserv->map, i)->group);
        outstr = msReplaceSubstring(outstr, substr, "selected=\"selected\"");
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_check]", GET_LAYER(mapserv->map, i)->group);
        outstr = msReplaceSubstring(outstr, substr, "checked=\"checked\"");
      }
      if(GET_LAYER(mapserv->map, i)->name) {
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_select]", GET_LAYER(mapserv->map, i)->name);
        outstr = msReplaceSubstring(outstr, substr, "selected=\"selected\"");
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_check]", GET_LAYER(mapserv->map, i)->name);
        outstr = msReplaceSubstring(outstr, substr, "checked=\"checked\"");
      }
    } else {
      if(GET_LAYER(mapserv->map, i)->group) {
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_select]", GET_LAYER(mapserv->map, i)->group);
        outstr = msReplaceSubstring(outstr, substr, "");
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_check]", GET_LAYER(mapserv->map, i)->group);
        outstr = msReplaceSubstring(outstr, substr, "");
      }
      if(GET_LAYER(mapserv->map, i)->name) {
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_select]", GET_LAYER(mapserv->map, i)->name);
        outstr = msReplaceSubstring(outstr, substr, "");
        snprintf(substr, PROCESSLINE_BUFLEN, "[%s_check]", GET_LAYER(mapserv->map, i)->name);
        outstr = msReplaceSubstring(outstr, substr, "");
      }
    }
  }

  for(i=-1; i<=1; i++) { /* make zoom direction persistant */
    if(mapserv->ZoomDirection == i) {
      snprintf(substr, sizeof(substr), "[zoomdir_%d_select]", i);
      outstr = msReplaceSubstring(outstr, substr, "selected=\"selected\"");
      snprintf(substr, sizeof(substr), "[zoomdir_%d_check]", i);
      outstr = msReplaceSubstring(outstr, substr, "checked=\"checked\"");
    } else {
      snprintf(substr, sizeof(substr), "[zoomdir_%d_select]", i);
      outstr = msReplaceSubstring(outstr, substr, "");
      snprintf(substr, sizeof(substr), "[zoomdir_%d_check]", i);
      outstr = msReplaceSubstring(outstr, substr, "");
    }
  }

  for(i=MINZOOM; i<=MAXZOOM; i++) { /* make zoom persistant */
    if(mapserv->Zoom == i) {
      snprintf(substr, sizeof(substr), "[zoom_%d_select]", i);
      outstr = msReplaceSubstring(outstr, substr, "selected=\"selected\"");
      snprintf(substr, sizeof(substr), "[zoom_%d_check]", i);
      outstr = msReplaceSubstring(outstr, substr, "checked=\"checked\"");
    } else {
      snprintf(substr, sizeof(substr), "[zoom_%d_select]", i);
      outstr = msReplaceSubstring(outstr, substr, "");
      snprintf(substr, sizeof(substr), "[zoom_%d_check]", i);
      outstr = msReplaceSubstring(outstr, substr, "");
    }
  }

  /* allow web object metadata access in template */

  /*
   * reworked by SG to use HashTable methods
   */

  if(&(mapserv->map->web.metadata) && strstr(outstr, "web_")) {
    for (j=0; j<MS_HASHSIZE; j++) {
      if(mapserv->map->web.metadata.items[j] != NULL) {
        for(tp=mapserv->map->web.metadata.items[j]; tp!=NULL; tp=tp->next) {
          snprintf(substr, PROCESSLINE_BUFLEN, "[web_%s]", tp->key);
          outstr = msReplaceSubstring(outstr, substr, tp->data);
          snprintf(substr, PROCESSLINE_BUFLEN, "[web_%s_esc]", tp->key);

          encodedstr = msEncodeUrl(tp->data);
          outstr = msReplaceSubstring(outstr, substr, encodedstr);
          free(encodedstr);
        }
      }
    }
  }

  /* allow layer metadata access in template */
  for(i=0; i<mapserv->map->numlayers; i++) {
    if(&(GET_LAYER(mapserv->map, i)->metadata) && GET_LAYER(mapserv->map, i)->name && strstr(outstr, GET_LAYER(mapserv->map, i)->name)) {
      for(j=0; j<MS_HASHSIZE; j++) {
        if(GET_LAYER(mapserv->map, i)->metadata.items[j] != NULL) {
          for(tp=GET_LAYER(mapserv->map, i)->metadata.items[j]; tp!=NULL; tp=tp->next) {
            snprintf(substr, PROCESSLINE_BUFLEN, "[%s_%s]", GET_LAYER(mapserv->map, i)->name, tp->key);
            if(GET_LAYER(mapserv->map, i)->status == MS_ON)
              outstr = msReplaceSubstring(outstr, substr, tp->data);
            else
              outstr = msReplaceSubstring(outstr, substr, "");
            snprintf(substr, PROCESSLINE_BUFLEN, "[%s_%s_esc]", GET_LAYER(mapserv->map, i)->name, tp->key);
            if(GET_LAYER(mapserv->map, i)->status == MS_ON) {
              encodedstr = msEncodeUrl(tp->data);
              outstr = msReplaceSubstring(outstr, substr, encodedstr);
              free(encodedstr);
            } else
              outstr = msReplaceSubstring(outstr, substr, "");
          }
        }
      }
    }
  }

  snprintf(repstr, sizeof(repstr), "%f", mapserv->mappnt.x);
  outstr = msReplaceSubstring(outstr, "[mapx]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->mappnt.y);
  outstr = msReplaceSubstring(outstr, "[mapy]", repstr);

  snprintf(repstr, sizeof(repstr), "%f", mapserv->map->extent.minx); /* Individual mapextent elements for spatial query building, deprecated. */
  outstr = msReplaceSubstring(outstr, "[minx]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->map->extent.maxx);
  outstr = msReplaceSubstring(outstr, "[maxx]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->map->extent.miny);
  outstr = msReplaceSubstring(outstr, "[miny]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->map->extent.maxy);
  outstr = msReplaceSubstring(outstr, "[maxy]", repstr);

  if(processDateTag( &outstr ) != MS_SUCCESS)
    return(NULL);

  if(processExtentTag(mapserv, &outstr, "mapext", &(mapserv->map->extent), &(mapserv->map->projection)) != MS_SUCCESS)
    return(NULL);
  if(processExtentTag(mapserv, &outstr, "mapext_esc", &(mapserv->map->extent), &(mapserv->map->projection)) != MS_SUCCESS) /* depricated */
    return(NULL);

  snprintf(repstr, sizeof(repstr), "%f", (mapserv->map->extent.maxx-mapserv->map->extent.minx)); /* useful for creating cachable extents (i.e. 0 0 dx dy) with legends and scalebars */
  outstr = msReplaceSubstring(outstr, "[dx]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", (mapserv->map->extent.maxy-mapserv->map->extent.miny));
  outstr = msReplaceSubstring(outstr, "[dy]", repstr);

  snprintf(repstr, sizeof(repstr), "%f", mapserv->RawExt.minx); /* Individual raw extent elements for spatial query building, deprecated. */
  outstr = msReplaceSubstring(outstr, "[rawminx]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->RawExt.maxx);
  outstr = msReplaceSubstring(outstr, "[rawmaxx]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->RawExt.miny);
  outstr = msReplaceSubstring(outstr, "[rawminy]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->RawExt.maxy);
  outstr = msReplaceSubstring(outstr, "[rawmaxy]", repstr);

  if(processExtentTag(mapserv, &outstr, "rawext", &(mapserv->RawExt), &(mapserv->map->projection)) != MS_SUCCESS)
    return(NULL);
  if(processExtentTag(mapserv, &outstr, "rawext_esc", &(mapserv->RawExt), &(mapserv->map->projection)) != MS_SUCCESS) /* depricated */
    return(NULL);

  if((strstr(outstr, "lat]") || strstr(outstr, "lon]") || strstr(outstr, "lon_esc]"))
      && mapserv->map->projection.proj != NULL
      && !msProjIsGeographicCRS(&(mapserv->map->projection)) ) {
    llextent=mapserv->map->extent;
    llpoint=mapserv->mappnt;
    msProjectRect(&(mapserv->map->projection), &(mapserv->map->latlon), &llextent);
    msProjectPoint(&(mapserv->map->projection), &(mapserv->map->latlon), &llpoint);

    snprintf(repstr, sizeof(repstr), "%f", llpoint.x);
    outstr = msReplaceSubstring(outstr, "[maplon]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", llpoint.y);
    outstr = msReplaceSubstring(outstr, "[maplat]", repstr);

    snprintf(repstr, sizeof(repstr), "%f", llextent.minx); /* map extent as lat/lon */
    outstr = msReplaceSubstring(outstr, "[minlon]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", llextent.maxx);
    outstr = msReplaceSubstring(outstr, "[maxlon]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", llextent.miny);
    outstr = msReplaceSubstring(outstr, "[minlat]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", llextent.maxy);
    outstr = msReplaceSubstring(outstr, "[maxlat]", repstr);

    if(processExtentTag(mapserv, &outstr, "mapext_latlon", &(llextent), NULL) != MS_SUCCESS)
      return(NULL);
    if(processExtentTag(mapserv, &outstr, "mapext_latlon_esc", &(llextent), NULL) != MS_SUCCESS) /* depricated */
      return(NULL);
  }

  /* submitted by J.F (bug 1102) */
  if(mapserv->map->reference.status == MS_ON) {
    snprintf(repstr, sizeof(repstr), "%f", mapserv->map->reference.extent.minx); /* Individual reference map extent elements for spatial query building, depricated. */
    outstr = msReplaceSubstring(outstr, "[refminx]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", mapserv->map->reference.extent.maxx);
    outstr = msReplaceSubstring(outstr, "[refmaxx]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", mapserv->map->reference.extent.miny);
    outstr = msReplaceSubstring(outstr, "[refminy]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", mapserv->map->reference.extent.maxy);
    outstr = msReplaceSubstring(outstr, "[refmaxy]", repstr);

    if(processExtentTag(mapserv, &outstr, "refext", &(mapserv->map->reference.extent), &(mapserv->map->projection)) != MS_SUCCESS)
      return(NULL);
    if(processExtentTag(mapserv, &outstr, "refext_esc", &(mapserv->map->reference.extent), &(mapserv->map->projection)) != MS_SUCCESS) /* depricated */
      return(NULL);
  }

  snprintf(repstr, sizeof(repstr), "%d %d", mapserv->map->width, mapserv->map->height);
  outstr = msReplaceSubstring(outstr, "[mapsize]", repstr);

  encodedstr = msEncodeUrl(repstr);
  outstr = msReplaceSubstring(outstr, "[mapsize_esc]", encodedstr);
  free(encodedstr);

  snprintf(repstr, sizeof(repstr), "%d", mapserv->map->width);
  outstr = msReplaceSubstring(outstr, "[mapwidth]", repstr);
  snprintf(repstr, sizeof(repstr), "%d", mapserv->map->height);
  outstr = msReplaceSubstring(outstr, "[mapheight]", repstr);

  snprintf(repstr, sizeof(repstr), "%f", mapserv->map->scaledenom);
  outstr = msReplaceSubstring(outstr, "[scale]", repstr);
  outstr = msReplaceSubstring(outstr, "[scaledenom]", repstr);
  snprintf(repstr, sizeof(repstr), "%f", mapserv->map->cellsize);
  outstr = msReplaceSubstring(outstr, "[cellsize]", repstr);

  snprintf(repstr, sizeof(repstr), "%.1f %.1f", (mapserv->map->width)/2.0, (mapserv->map->height)/2.0); /* not subtracting 1 from image dimensions (see bug 633) */
  outstr = msReplaceSubstring(outstr, "[center]", repstr);
  snprintf(repstr, sizeof(repstr), "%.1f", (mapserv->map->width)/2.0);
  outstr = msReplaceSubstring(outstr, "[center_x]", repstr);
  snprintf(repstr, sizeof(repstr), "%.1f", (mapserv->map->height)/2.0);
  outstr = msReplaceSubstring(outstr, "[center_y]", repstr);

  /* These are really for situations with multiple result sets only, but often used in header/footer   */
  snprintf(repstr, sizeof(repstr), "%d", mapserv->NR); /* total number of results */
  outstr = msReplaceSubstring(outstr, "[nr]", repstr);
  snprintf(repstr, sizeof(repstr), "%d", mapserv->NL); /* total number of layers with results */
  outstr = msReplaceSubstring(outstr, "[nl]", repstr);

  if(mapserv->resultlayer) {
    if(strstr(outstr, "[items]") != NULL) {
      char *itemstr=NULL;

      itemstr = msJoinStrings(mapserv->resultlayer->items, mapserv->resultlayer->numitems, ",");
      outstr = msReplaceSubstring(outstr, "[items]", itemstr);
      free(itemstr);
    }

    snprintf(repstr, sizeof(repstr), "%d", mapserv->NLR); /* total number of results within this layer */
    outstr = msReplaceSubstring(outstr, "[nlr]", repstr);
    snprintf(repstr, sizeof(repstr), "%d", mapserv->RN); /* sequential (eg. 1..n) result number within all layers */
    outstr = msReplaceSubstring(outstr, "[rn]", repstr);
    snprintf(repstr, sizeof(repstr), "%d", mapserv->LRN); /* sequential (eg. 1..n) result number within this layer */
    outstr = msReplaceSubstring(outstr, "[lrn]", repstr);
    outstr = msReplaceSubstring(outstr, "[cl]", mapserv->resultlayer->name); /* current layer name */
    /* if(resultlayer->description) outstr = msReplaceSubstring(outstr, "[cd]", resultlayer->description); */ /* current layer description */

    /* allow layer metadata access when there is a current result layer (implicitly a query template) */
    if(&(mapserv->resultlayer->metadata) && strstr(outstr, "[metadata_")) {
      for(i=0; i<MS_HASHSIZE; i++) {
        if(mapserv->resultlayer->metadata.items[i] != NULL) {
          for(tp=mapserv->resultlayer->metadata.items[i]; tp!=NULL; tp=tp->next) {
            snprintf(substr, PROCESSLINE_BUFLEN, "[metadata_%s]", tp->key);
            outstr = msReplaceSubstring(outstr, substr, tp->data);

            snprintf(substr, PROCESSLINE_BUFLEN, "[metadata_%s_esc]", tp->key);
            encodedstr = msEncodeUrl(tp->data);
            outstr = msReplaceSubstring(outstr, substr, encodedstr);
            free(encodedstr);
          }
        }
      }
    }
  }

  if(mode != QUERY) {
    if(processResultSetTag(mapserv, &outstr, stream) != MS_SUCCESS) {
      msFree(outstr);
      return(NULL);
    }
  } else { /* return shape and/or values */

    assert(mapserv->resultlayer);

    snprintf(repstr, sizeof(repstr), "%f %f", (mapserv->resultshape.bounds.maxx + mapserv->resultshape.bounds.minx)/2, (mapserv->resultshape.bounds.maxy + mapserv->resultshape.bounds.miny)/2);
    outstr = msReplaceSubstring(outstr, "[shpmid]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", (mapserv->resultshape.bounds.maxx + mapserv->resultshape.bounds.minx)/2);
    outstr = msReplaceSubstring(outstr, "[shpmidx]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", (mapserv->resultshape.bounds.maxy + mapserv->resultshape.bounds.miny)/2);
    outstr = msReplaceSubstring(outstr, "[shpmidy]", repstr);

    if(processExtentTag(mapserv, &outstr, "shpext", &(mapserv->resultshape.bounds), &(mapserv->resultlayer->projection)) != MS_SUCCESS)
      return(NULL);
    if(processExtentTag(mapserv, &outstr, "shpext_esc", &(mapserv->resultshape.bounds), &(mapserv->resultlayer->projection)) != MS_SUCCESS) /* depricated */
      return(NULL);

    snprintf(repstr, sizeof(repstr), "%d", mapserv->resultshape.classindex);
    outstr = msReplaceSubstring(outstr, "[shpclass]", repstr);

    if(processShpxyTag(mapserv->resultlayer, &outstr, &mapserv->resultshape) != MS_SUCCESS)
      return(NULL);

    if(processShplabelTag(mapserv->resultlayer, &outstr, &mapserv->resultshape) != MS_SUCCESS)
      return(NULL);

    snprintf(repstr, sizeof(repstr), "%f", mapserv->resultshape.bounds.minx);
    outstr = msReplaceSubstring(outstr, "[shpminx]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", mapserv->resultshape.bounds.miny);
    outstr = msReplaceSubstring(outstr, "[shpminy]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", mapserv->resultshape.bounds.maxx);
    outstr = msReplaceSubstring(outstr, "[shpmaxx]", repstr);
    snprintf(repstr, sizeof(repstr), "%f", mapserv->resultshape.bounds.maxy);
    outstr = msReplaceSubstring(outstr, "[shpmaxy]", repstr);

    snprintf(repstr, sizeof(repstr), "%ld", mapserv->resultshape.index);
    outstr = msReplaceSubstring(outstr, "[shpidx]", repstr);
    snprintf(repstr, sizeof(repstr), "%d", mapserv->resultshape.tileindex);
    outstr = msReplaceSubstring(outstr, "[tileidx]", repstr);

    /* return ALL attributes in one delimeted list */
    if(strstr(outstr, "[values]") != NULL) {
      char *valuestr=NULL;

      valuestr = msJoinStrings(mapserv->resultshape.values, mapserv->resultlayer->numitems, ",");
      outstr = msReplaceSubstring(outstr, "[values]", valuestr);
      free(valuestr);
    }

    for(i=0; i<mapserv->resultlayer->numitems; i++) {
      /* by default let's encode attributes for HTML presentation */
      snprintf(substr, PROCESSLINE_BUFLEN, "[%s]", mapserv->resultlayer->items[i]);
      if(strstr(outstr, substr) != NULL) {
        encodedstr = msEncodeHTMLEntities(mapserv->resultshape.values[i]);
        outstr = msReplaceSubstring(outstr, substr, encodedstr);
        free(encodedstr);
      }

      /* of course you might want to embed that data in URLs */
      snprintf(substr, PROCESSLINE_BUFLEN, "[%s_esc]", mapserv->resultlayer->items[i]);
      if(strstr(outstr, substr) != NULL) {
        encodedstr = msEncodeUrl(mapserv->resultshape.values[i]);
        outstr = msReplaceSubstring(outstr, substr, encodedstr);
        free(encodedstr);
      }

      /* or you might want to access the attributes unaltered */
      snprintf(substr, PROCESSLINE_BUFLEN, "[%s_raw]", mapserv->resultlayer->items[i]);
      if(strstr(outstr, substr) != NULL)
        outstr = msReplaceSubstring(outstr, substr, mapserv->resultshape.values[i]);
    }

    if(processItemTag(mapserv->resultlayer, &outstr, &mapserv->resultshape) != MS_SUCCESS)
      return(NULL);

    /* handle joins in this next section */
    for(i=0; i<mapserv->resultlayer->numjoins; i++) {
      if(mapserv->resultlayer->joins[i].values) { /* join has data */
        for(j=0; j<mapserv->resultlayer->joins[i].numitems; j++) {
          /* by default let's encode attributes for HTML presentation */
          snprintf(substr, PROCESSLINE_BUFLEN, "[%s_%s]", mapserv->resultlayer->joins[i].name, mapserv->resultlayer->joins[i].items[j]);
          if(strstr(outstr, substr) != NULL) {
            encodedstr = msEncodeHTMLEntities(mapserv->resultlayer->joins[i].values[j]);
            outstr = msReplaceSubstring(outstr, substr, encodedstr);
            free(encodedstr);
          }

          /* of course you might want to embed that data in URLs */
          snprintf(substr, PROCESSLINE_BUFLEN, "[%s_%s_esc]", mapserv->resultlayer->joins[i].name, mapserv->resultlayer->joins[i].items[j]);
          if(strstr(outstr, substr) != NULL) {
            encodedstr = msEncodeUrl(mapserv->resultlayer->joins[i].values[j]);
            outstr = msReplaceSubstring(outstr, substr, encodedstr);
            free(encodedstr);
          }

          /* or you might want to access the attributes unaltered */
          snprintf(substr, PROCESSLINE_BUFLEN, "[%s_%s_raw]", mapserv->resultlayer->joins[i].name, mapserv->resultlayer->joins[i].items[j]);
          if(strstr(outstr, substr) != NULL)
            outstr = msReplaceSubstring(outstr, substr, mapserv->resultlayer->joins[i].values[j]);
        }
      } else if(mapserv->resultlayer->joins[i].type ==  MS_JOIN_ONE_TO_MANY) { /* one-to-many join */
        char *joinTemplate=NULL;

        snprintf(substr, PROCESSLINE_BUFLEN, "[join_%s]", mapserv->resultlayer->joins[i].name);
        if(strstr(outstr, substr) != NULL) {
          joinTemplate = processOneToManyJoin(mapserv, &(mapserv->resultlayer->joins[i]));
          if(joinTemplate) {
            outstr = msReplaceSubstring(outstr, substr, joinTemplate);
            free(joinTemplate);
          } else
            return NULL;
        }
      }
    } /* next join */

  } /* end query mode specific substitutions */

  if(processIncludeTag(mapserv, &outstr, stream, mode) != MS_SUCCESS)
    return(NULL);

  for(i=0; i<mapserv->request->NumParams; i++) {
    /* Replace [variable] tags using values from URL. We cannot offer a
     * [variable_raw] option here due to the risk of XSS.
     *
     * Replacement is case-insensitive. (#4511)
     */
    snprintf(substr, PROCESSLINE_BUFLEN, "[%s]", mapserv->request->ParamNames[i]);
    encodedstr = msEncodeHTMLEntities(mapserv->request->ParamValues[i]);
    outstr = msCaseReplaceSubstring(outstr, substr, encodedstr);
    free(encodedstr);

    snprintf(substr, PROCESSLINE_BUFLEN, "[%s_esc]", mapserv->request->ParamNames[i]);
    encodedstr = msEncodeUrl(mapserv->request->ParamValues[i]);
    outstr = msCaseReplaceSubstring(outstr, substr, encodedstr);
    free(encodedstr);
  }

  return(outstr);
}

#define MS_TEMPLATE_BUFFER 1024 /* 1k */

int msReturnPage(mapservObj *mapserv, char *html, int mode, char **papszBuffer)
{
  FILE *stream;
  char line[MS_BUFFER_LENGTH], *tmpline;
  int   nBufferSize = 0;
  int   nCurrentSize = 0;

  ms_regex_t re; /* compiled regular expression to be matched */
  char szPath[MS_MAXPATHLEN];

  if(!html) {
    msSetError(MS_WEBERR, "No template specified", "msReturnPage()");
    return MS_FAILURE;
  }

  if(ms_regcomp(&re, MS_TEMPLATE_EXPR, MS_REG_EXTENDED|MS_REG_NOSUB|MS_REG_ICASE) != 0) {
    msSetError(MS_REGEXERR, NULL, "msReturnPage()");
    return MS_FAILURE;
  }

  if(ms_regexec(&re, html, 0, NULL, 0) != 0) { /* no match */
    ms_regfree(&re);
    msSetError(MS_WEBERR, "Malformed template name (%s).", "msReturnPage()", html);
    return MS_FAILURE;
  }
  ms_regfree(&re);

  if((stream = fopen(msBuildPath(szPath, mapserv->map->mappath, html), "r")) == NULL) {
    msSetError(MS_IOERR, "%s", "msReturnPage()", html);
    return MS_FAILURE;
  }

  if(isValidTemplate(stream, html) != MS_TRUE) {
    fclose(stream);
    return MS_FAILURE;
  }

  if(papszBuffer) {
    if((*papszBuffer) == NULL) {
      (*papszBuffer) = (char *)msSmallMalloc(MS_TEMPLATE_BUFFER);
      (*papszBuffer)[0] = '\0';
      nBufferSize = MS_TEMPLATE_BUFFER;
      nCurrentSize = 0;
    } else {
      nCurrentSize = strlen((*papszBuffer));
      nBufferSize = nCurrentSize;
    }
  }

  while(fgets(line, MS_BUFFER_LENGTH, stream) != NULL) { /* now on to the end of the file */

    if(strchr(line, '[') != NULL) {
      tmpline = processLine(mapserv, line, stream, mode);
      if(!tmpline) {
        fclose(stream);
        return MS_FAILURE;
      }

      if(papszBuffer) {
        if(nBufferSize <= (int)(nCurrentSize + strlen(tmpline) + 1)) {
          const int nExpandBuffer = (strlen(tmpline) /  MS_TEMPLATE_BUFFER) + 1;
          nBufferSize = MS_TEMPLATE_BUFFER*nExpandBuffer + strlen((*papszBuffer));
          (*papszBuffer) = (char *) msSmallRealloc((*papszBuffer),sizeof(char)*nBufferSize);
        }
        strcat((*papszBuffer), tmpline);
        nCurrentSize += strlen(tmpline);
      } else
        msIO_fwrite(tmpline, strlen(tmpline), 1, stdout);

      free(tmpline);
    } else {
      if(papszBuffer) {
        if(nBufferSize <= (int)(nCurrentSize + strlen(line))) {
          const int nExpandBuffer = (strlen(line) /  MS_TEMPLATE_BUFFER) + 1;
          nBufferSize = MS_TEMPLATE_BUFFER*nExpandBuffer + strlen((*papszBuffer));
          (*papszBuffer) = (char *)msSmallRealloc((*papszBuffer),sizeof(char)*nBufferSize);
        }
        strcat((*papszBuffer), line);
        nCurrentSize += strlen(line);
      } else
        msIO_fwrite(line, strlen(line), 1, stdout);
    }
    if(!papszBuffer)
      fflush(stdout);
  } /* next line */

  fclose(stream);

  return MS_SUCCESS;
}

int msReturnURL(mapservObj* ms, char* url, int mode)
{
  char *tmpurl;

  if(url == NULL) {
    msSetError(MS_WEBERR, "Empty URL.", "msReturnURL()");
    return MS_FAILURE;
  }

  tmpurl = processLine(ms, url, NULL, mode); /* URL templates can't handle multi-line tags, hence the NULL file pointer */

  if(!tmpurl)
    return MS_FAILURE;

  msRedirect(tmpurl);
  free(tmpurl);

  return MS_SUCCESS;
}

/*
** Legacy query template parsing where you use headers, footers and such...
*/
int msReturnNestedTemplateQuery(mapservObj* mapserv, char* pszMimeType, char **papszBuffer)
{
  int status;
  int i,j,k;
  char buffer[1024];
  int nBufferSize =0;
  int nCurrentSize = 0;
  int nExpandBuffer = 0;

  char *template;

  layerObj *lp=NULL;

  if(papszBuffer) {
    (*papszBuffer) = (char *)msSmallMalloc(MS_TEMPLATE_BUFFER);
    (*papszBuffer)[0] = '\0';
    nBufferSize = MS_TEMPLATE_BUFFER;
    nCurrentSize = 0;
    nExpandBuffer = 1;
  }

  msInitShape(&(mapserv->resultshape));

  if((mapserv->Mode == ITEMQUERY) || (mapserv->Mode == QUERY)) { /* may need to handle a URL result set since these modes return exactly 1 result */

    for(i=(mapserv->map->numlayers-1); i>=0; i--) {
      lp = (GET_LAYER(mapserv->map, i));

      if(!lp->resultcache) continue;
      if(lp->resultcache->numresults > 0) break;
    }

    if(i >= 0) { /* at least if no result found, mapserver will display an empty template. */

      if(lp->resultcache->results[0].classindex >= 0 && lp->class[(int)(lp->resultcache->results[0].classindex)]->template)
        template = lp->class[(int)(lp->resultcache->results[0].classindex)]->template;
      else
        template = lp->template;

      if( template == NULL ) {
        msSetError(MS_WEBERR, "No template for layer %s or it's classes.", "msReturnNestedTemplateQuery()", lp->name );
        return MS_FAILURE;
      }

      if(TEMPLATE_TYPE(template) == MS_URL) {
        mapserv->resultlayer = lp;

#if 0
        status = msLayerOpen(lp);
        if(status != MS_SUCCESS) return status;

        status = msLayerGetItems(lp); /* retrieve all the item names */
        if(status != MS_SUCCESS) return status;
#endif

        status = msLayerGetShape(lp, &(mapserv->resultshape), &(lp->resultcache->results[0]));
        if(status != MS_SUCCESS) return status;

        if(lp->numjoins > 0) {
          for(k=0; k<lp->numjoins; k++) {
            status = msJoinConnect(lp, &(lp->joins[k]));
            if(status != MS_SUCCESS) return status;

            msJoinPrepare(&(lp->joins[k]), &(mapserv->resultshape));
            msJoinNext(&(lp->joins[k])); /* fetch the first row */
          }
        }

        if(papszBuffer == NULL) {
          if(msReturnURL(mapserv, template, QUERY) != MS_SUCCESS) return MS_FAILURE;
        }

        msFreeShape(&(mapserv->resultshape));
        /* msLayerClose(lp); */
        mapserv->resultlayer = NULL;

        return MS_SUCCESS;
      }
    }
  }

  /*
  ** Now we know we're making a template sandwich
  */
  mapserv->NR = mapserv->NL = 0;
  for(i=0; i<mapserv->map->numlayers; i++) { /* compute some totals */
    lp = (GET_LAYER(mapserv->map, i));

    if(!lp->resultcache) continue;

    if(lp->resultcache->numresults > 0) {
      mapserv->NL++;
      mapserv->NR += lp->resultcache->numresults;
    }
  }

  /*
  ** Is this step really necessary for buffered output? Legend and browse templates don't deal with mime-types
  ** so why should this. Note that new-style templates don't buffer the mime-type either.
  */
  if(papszBuffer && mapserv->sendheaders) {
    snprintf(buffer, sizeof(buffer), "Content-Type: %s%c%c", pszMimeType, 10, 10);
    if(nBufferSize <= (int)(nCurrentSize + strlen(buffer) + 1)) {
      nExpandBuffer++;
      (*papszBuffer) = (char *)msSmallRealloc((*papszBuffer), MS_TEMPLATE_BUFFER*nExpandBuffer);
      nBufferSize = MS_TEMPLATE_BUFFER*nExpandBuffer;
    }
    strcat((*papszBuffer), buffer);
    nCurrentSize += strlen(buffer);
  } else if(mapserv->sendheaders) {
    msIO_setHeader("Content-Type","%s",pszMimeType);
    msIO_sendHeaders();
  }

  if(mapserv->map->web.header) {
    if(msReturnPage(mapserv, mapserv->map->web.header, BROWSE, papszBuffer) != MS_SUCCESS) return MS_FAILURE;
  }

  mapserv->RN = 1; /* overall result number */
  for(i=0; i<mapserv->map->numlayers; i++) {
    mapserv->resultlayer = lp = (GET_LAYER(mapserv->map, mapserv->map->layerorder[i]));

    if(!lp->resultcache) continue;
    if(lp->resultcache->numresults <= 0) continue;

    mapserv->NLR = lp->resultcache->numresults;

#if 0
    status = msLayerOpen(lp); /* open this layer */
    if(status != MS_SUCCESS) return status;

    status = msLayerGetItems(lp); /* retrieve all the item names */
    if(status != MS_SUCCESS) return status;
#endif

    if(lp->numjoins > 0) { /* open any necessary JOINs here */
      for(k=0; k<lp->numjoins; k++) {
        status = msJoinConnect(lp, &(lp->joins[k]));
        if(status != MS_SUCCESS) return status;
      }
    }

    if(lp->header) {
      if(msReturnPage(mapserv, lp->header, BROWSE, papszBuffer) != MS_SUCCESS) return MS_FAILURE;
    }

    mapserv->LRN = 1; /* layer result number */
    for(j=0; j<lp->resultcache->numresults; j++) {
      status = msLayerGetShape(lp, &(mapserv->resultshape), &(lp->resultcache->results[j]));
      if(status != MS_SUCCESS) return status;

      /* prepare any necessary JOINs here (one-to-one only) */
      if(lp->numjoins > 0) {
        for(k=0; k<lp->numjoins; k++) {
          if(lp->joins[k].type == MS_JOIN_ONE_TO_ONE) {
            msJoinPrepare(&(lp->joins[k]), &(mapserv->resultshape));
            msJoinNext(&(lp->joins[k])); /* fetch the first row */
          }
        }
      }

      if(lp->resultcache->results[j].classindex >= 0 && lp->class[(int)(lp->resultcache->results[j].classindex)]->template)
        template = lp->class[(int)(lp->resultcache->results[j].classindex)]->template;
      else
        template = lp->template;

      if(msReturnPage(mapserv, template, QUERY, papszBuffer) != MS_SUCCESS) {
        msFreeShape(&(mapserv->resultshape));
        return MS_FAILURE;
      }

      msFreeShape(&(mapserv->resultshape)); /* init too */

      mapserv->RN++; /* increment counters */
      mapserv->LRN++;
    }

    if(lp->footer) {
      if(msReturnPage(mapserv, lp->footer, BROWSE, papszBuffer) != MS_SUCCESS) return MS_FAILURE;
    }

    /* msLayerClose(lp); */
    mapserv->resultlayer = NULL;
  }

  if(mapserv->map->web.footer)
    return msReturnPage(mapserv, mapserv->map->web.footer, BROWSE, papszBuffer);

  return MS_SUCCESS;
}

int msReturnOpenLayersPage(mapservObj *mapserv)
{
  int i;
  char *buffer = NULL, *layer = NULL;
  const char *tmpUrl = NULL;
  const char *openlayersUrl = olUrl;
  char *projection = NULL;
  char *format = NULL;

  /* 2 CGI parameters are used in the template. we need to transform them
   * to be sure the case match during the template processing. We also
   * need to search the SRS/CRS parameter to get the projection info. OGC
   * services version >= 1.3.0 uses CRS rather than SRS */
  for( i=0; i<mapserv->request->NumParams; i++) {
    if( (strcasecmp(mapserv->request->ParamNames[i], "SRS") == 0) ||
        (strcasecmp(mapserv->request->ParamNames[i], "CRS") == 0) ) {
      projection = mapserv->request->ParamValues[i];
    } else if(strcasecmp(mapserv->request->ParamNames[i], "LAYERS") == 0) {
      free(mapserv->request->ParamNames[i]);
      mapserv->request->ParamNames[i] = msStrdup("LAYERS");
    } else if(strcasecmp(mapserv->request->ParamNames[i], "VERSION") == 0) {
      free(mapserv->request->ParamNames[i]);
      mapserv->request->ParamNames[i] = msStrdup("VERSION");
    }
  }
  if(mapserv->map->outputformat->mimetype && *mapserv->map->outputformat->mimetype) {
    format = mapserv->map->outputformat->mimetype;
  }

  /* check if the environment variable or config MS_OPENLAYERS_JS_URL is set */
  tmpUrl = msGetConfigOption(mapserv->map, "MS_OPENLAYERS_JS_URL");
  if(tmpUrl == NULL) tmpUrl = CPLGetConfigOption("MS_OPENLAYERS_JS_URL", NULL);
  
  if(tmpUrl)
    openlayersUrl = (char *)tmpUrl;

  if (mapserv->Mode == BROWSE) {
    msSetError(MS_WMSERR, "At least one layer name required in LAYERS.",
               "msWMSLoadGetMapParams()");
    layer = processLine(mapserv, olLayerMapServerTag, NULL, BROWSE);
  } else
    layer = processLine(mapserv, olLayerWMSTag, NULL, BROWSE);

  buffer = processLine(mapserv, olTemplate, NULL, BROWSE);
  buffer = msReplaceSubstring(buffer, "[openlayers_js_url]", openlayersUrl);
  buffer = msReplaceSubstring(buffer, "[openlayers_layer]", layer);
  if (projection)
    buffer = msReplaceSubstring(buffer, "[openlayers_projection]", projection);
  if (format)
    buffer = msReplaceSubstring(buffer, "[openlayers_format]", format);
  else
    buffer = msReplaceSubstring(buffer, "[openlayers_format]", "image/jpeg");
  msIO_fwrite(buffer, strlen(buffer), 1, stdout);
  free(layer);
  free(buffer);

  return MS_SUCCESS;
}

mapservObj *msAllocMapServObj()
{
  mapservObj *mapserv = msSmallMalloc(sizeof(mapservObj));

  mapserv->savemap=MS_FALSE;
  mapserv->savequery=MS_FALSE; /* should the query and/or map be saved  */

  mapserv->sendheaders = MS_TRUE;

  mapserv->request = msAllocCgiObj();

  mapserv->map=NULL;

  mapserv->NumLayers=0; /* number of layers specfied by a user */
  mapserv->MaxLayers=0; /* allocated size of Layers[] array */
  mapserv->Layers = NULL;

  mapserv->icon = NULL;

  mapserv->RawExt.minx=-1;
  mapserv->RawExt.miny=-1;
  mapserv->RawExt.maxx=-1;
  mapserv->RawExt.maxy=-1;

  mapserv->fZoom=1;
  mapserv->Zoom=1; /* default for browsing */

  mapserv->resultlayer=NULL;

  mapserv->UseShapes=MS_FALSE;

  mapserv->mappnt.x=-1;
  mapserv->mappnt.y=-1;

  mapserv->ZoomDirection=0; /* whether zooming in or out, default is pan or 0 */

  mapserv->Mode=BROWSE; /* can be BROWSE, QUERY, etc. */

  sprintf(mapserv->Id, "%ld%d", (long)time(NULL), (int)getpid());

  mapserv->CoordSource=NONE;
  mapserv->ScaleDenom=0;

  mapserv->ImgRows=-1;
  mapserv->ImgCols=-1;

  mapserv->ImgExt.minx=-1;
  mapserv->ImgExt.miny=-1;
  mapserv->ImgExt.maxx=-1;
  mapserv->ImgExt.maxy=-1;

  mapserv->ImgBox.minx=-1;
  mapserv->ImgBox.miny=-1;
  mapserv->ImgBox.maxx=-1;
  mapserv->ImgBox.maxy=-1;

  mapserv->RefPnt.x=-1;
  mapserv->RefPnt.y=-1;
  mapserv->ImgPnt.x=-1;
  mapserv->ImgPnt.y=-1;

  mapserv->Buffer=0;

  /*
  ** variables for multiple query results processing
  */
  mapserv->RN=0; /* overall result number */
  mapserv->LRN=0; /* result number within a layer */
  mapserv->NL=0; /* total number of layers with results */
  mapserv->NR=0; /* total number or results */
  mapserv->NLR=0; /* number of results in a layer */

  mapserv->SearchMap=MS_FALSE; /* apply pan/zoom BEFORE doing the query (e.g. query the output image rather than the input image) */

  mapserv->QueryFile=NULL;
  mapserv->QueryLayer=NULL;
  mapserv->SelectLayer=NULL;
  mapserv->QueryLayerIndex=-1;
  mapserv->SelectLayerIndex=-1;
  mapserv->QueryItem=NULL;
  mapserv->QueryString=NULL;
  mapserv->ShapeIndex=-1;
  mapserv->TileIndex=-1;
  mapserv->TileMode=TILE_GMAP;
  mapserv->TileCoords=NULL;
  mapserv->TileWidth=-1;
  mapserv->TileHeight=-1;
  mapserv->QueryCoordSource=NONE;
  mapserv->ZoomSize=0; /* zoom absolute magnitude (i.e. > 0) */

  mapserv->hittest = NULL;

  return mapserv;
}

void msFreeMapServObj(mapservObj* mapserv)
{
  int i;

  if(mapserv) {
    if( mapserv->map ) {
      if(mapserv->hittest) {
        freeMapHitTests(mapserv->map,mapserv->hittest);
        free(mapserv->hittest);
      }
      msFreeMap(mapserv->map);
      mapserv->map = NULL;
    }

    if( mapserv->request ) {
      msFreeCgiObj(mapserv->request);
      mapserv->request = NULL;
    }

    for(i=0; i<mapserv->NumLayers; i++)
      msFree(mapserv->Layers[i]);
    msFree(mapserv->Layers);

    msFree(mapserv->icon);

    msFree(mapserv->QueryItem);
    msFree(mapserv->QueryString);
    msFree(mapserv->QueryLayer);
    msFree(mapserv->SelectLayer);
    msFree(mapserv->QueryFile);

    msFree(mapserv->TileCoords);

    msFree(mapserv);
  }
}

/*
** Ensure there is at least one free entry in the Layers array.
**
** This function is safe to use for the initial allocation of the Layers[]
** array as well (i.e. when MaxLayers==0 and Layers==NULL)
**
** Returns MS_SUCCESS/MS_FAILURE
*/
int msGrowMapservLayers( mapservObj* mapserv )
{
  /* Do we need to increase the size of Layers[] by MS_LAYER_ALLOCSIZE? */
  if(mapserv->NumLayers == mapserv->MaxLayers) {
    int i;

    if(mapserv->MaxLayers == 0) {
      /* initial allocation of array */
      mapserv->MaxLayers += MS_LAYER_ALLOCSIZE;
      mapserv->NumLayers = 0;
      mapserv->Layers = (char**)msSmallMalloc(mapserv->MaxLayers*sizeof(char*));
    } else {
      /* realloc existing array */
      mapserv->MaxLayers += MS_LAYER_ALLOCSIZE;
      mapserv->Layers = (char**)msSmallRealloc(mapserv->Layers, mapserv->MaxLayers*sizeof(char*));
    }

    if(mapserv->Layers == NULL) {
      msSetError(MS_MEMERR, "Failed to allocate memory for Layers array.", "msGrowMappservLayers()");
      return MS_FAILURE;
    }

    for(i=mapserv->NumLayers; i<mapserv->MaxLayers; i++) {
      mapserv->Layers[i] = NULL;
    }
  }

  return MS_SUCCESS;
}

/*
** Utility function to generate map, legend, scalebar and reference images.
**
** Parameters:
**   - mapserv: mapserv object (used to extract the map object).
**   - bQueryMap: if set to TRUE a query map will be created instead of a regular map.
**   - bReturnOnError: if set to TRUE, the function will return on the first error, else it will try to generate all the images.
*/
int msGenerateImages(mapservObj *mapserv, int bQueryMap, int bReturnOnError)
{
  char buffer[1024];

  if(mapserv) {

    /* render the map OR query map */
    if((!bQueryMap && mapserv->map->status == MS_ON) || (bQueryMap && mapserv->map->querymap.status == MS_ON)) {
      imageObj *image = NULL;

      image = msDrawMap(mapserv->map, bQueryMap);
      if(image) {
        snprintf(buffer, sizeof(buffer), "%s%s%s.%s", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));

        if(msSaveImage(mapserv->map, image, buffer) != MS_SUCCESS && bReturnOnError) {
          msFreeImage(image);
          return MS_FAILURE;
        }
        msFreeImage(image);
      } else if(bReturnOnError)
        return MS_FAILURE;
    }

    /* render the legend */
    if(mapserv->map->legend.status == MS_ON) {
      imageObj *image = NULL;
      image = msDrawLegend(mapserv->map, MS_FALSE, NULL);
      if(image) {
        snprintf(buffer, sizeof(buffer), "%s%sleg%s.%s", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));

        if(msSaveImage(mapserv->map, image, buffer) != MS_SUCCESS && bReturnOnError) {
          msFreeImage(image);
          return MS_FAILURE;
        }
        msFreeImage(image);
      } else if(bReturnOnError)
        return MS_FAILURE;
    }

    /* render the scalebar */
    if(mapserv->map->scalebar.status == MS_ON) {
      imageObj *image = NULL;
      image = msDrawScalebar(mapserv->map);
      if(image) {
        snprintf(buffer, sizeof(buffer), "%s%ssb%s.%s", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));
        if(msSaveImage(mapserv->map, image, buffer) != MS_SUCCESS && bReturnOnError) {
          msFreeImage(image);
          return MS_FAILURE;
        }
        msFreeImage(image);
      } else if(bReturnOnError)
        return MS_FAILURE;
    }

    /* render the reference map */
    if(mapserv->map->reference.status == MS_ON) {
      imageObj *image;
      image = msDrawReferenceMap(mapserv->map);
      if(image) {
        snprintf(buffer, sizeof(buffer), "%s%sref%s.%s", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id, MS_IMAGE_EXTENSION(mapserv->map->outputformat));
        if(msSaveImage(mapserv->map, image, buffer) != MS_SUCCESS && bReturnOnError) {
          msFreeImage(image);
          return MS_FAILURE;
        }
        msFreeImage(image);
      } else if(bReturnOnError)
        return MS_FAILURE;
    }

  }

  return MS_SUCCESS;
}

/*
** Utility function to open a template file, process it and
** and return into a buffer the processed template. Uses the
** template file from the web object. Returns NULL if there is
** an error.
*/
char *msProcessTemplate(mapObj *map, int bGenerateImages, char **names, char **values, int numentries)
{
  char *pszBuffer = NULL;

  if(map) {

    /* Initialize object and set appropriate defaults. */
    mapservObj  *mapserv  = NULL;
    mapserv = msAllocMapServObj();

    mapserv->map = map;
    mapserv->Mode = BROWSE;

    if(names && values && numentries > 0) {
      msFreeCharArray(mapserv->request->ParamNames, mapserv->request->NumParams);
      msFreeCharArray(mapserv->request->ParamValues, mapserv->request->NumParams);
      mapserv->request->ParamNames = names;
      mapserv->request->ParamValues = values;
      mapserv->request->NumParams = numentries;
    }

    /*
    ** ISSUE/TODO : some of the name/values should be extracted and
    ** processed (ex imgext, layers, ...) as it is done in function
    ** loadform.
    */

    if(bGenerateImages)
      msGenerateImages(mapserv, MS_FALSE, MS_FALSE);

    /*
    ** Process the template.
    **
    ** TODO : use web minscaledenom/maxscaledenom depending on the scale.
    */
    if(msReturnPage(mapserv, mapserv->map->web.template, BROWSE, &pszBuffer) != MS_SUCCESS) {
      msFree(pszBuffer);
      pszBuffer = NULL;
    }

    /* Don't free the map and names and values arrays since they were passed by reference. */
    mapserv->map = NULL;
    mapserv->request->ParamNames = mapserv->request->ParamValues = NULL;
    mapserv->request->NumParams = 0;
    msFreeMapServObj(mapserv);
  }

  return pszBuffer;
}

/*
** Utility method to process the legend template.
*/
char *msProcessLegendTemplate(mapObj *map, char **names, char **values, int numentries)
{
  char *pszOutBuf = NULL;

  if(map && map->legend.template) {

    /* Initialize object and set appropriate defaults. */
    mapservObj  *mapserv  = NULL;
    mapserv = msAllocMapServObj();

    mapserv->map = map;
    mapserv->Mode = BROWSE;

    if(names && values && numentries > 0) {
      msFreeCharArray(mapserv->request->ParamNames, mapserv->request->NumParams);
      msFreeCharArray(mapserv->request->ParamValues, mapserv->request->NumParams);
      mapserv->request->ParamNames = names;
      mapserv->request->ParamValues = values;
      mapserv->request->NumParams = numentries;
    }

    pszOutBuf = generateLegendTemplate(mapserv);

    /* Don't free the map and names and values arrays since they were passed by reference. */
    mapserv->map = NULL;
    mapserv->request->ParamNames = mapserv->request->ParamValues = NULL;
    mapserv->request->NumParams = 0;
    msFreeMapServObj(mapserv);
  }

  return pszOutBuf;
}

/*
** Utility function that process a template file(s) used in the
** query and return the processed template(s) in a buffer.
*/
char *msProcessQueryTemplate(mapObj *map, int bGenerateImages, char **names, char **values, int numentries)
{
  char *pszBuffer = NULL;

  if(map) {

    /* Initialize object and set appropriate defaults. */
    mapservObj *mapserv = NULL;
    mapserv = msAllocMapServObj();

    mapserv->map = map;
    mapserv->Mode = QUERY;

    if(names && values && numentries > 0) {
      msFreeCharArray(mapserv->request->ParamNames, mapserv->request->NumParams);
      msFreeCharArray(mapserv->request->ParamValues, mapserv->request->NumParams);
      mapserv->request->ParamNames = names;
      mapserv->request->ParamValues = values;
      mapserv->request->NumParams = numentries;
    }

    if(bGenerateImages)
      msGenerateImages(mapserv, MS_TRUE, MS_FALSE);

    mapserv->sendheaders = MS_FALSE;
    IGNORE_RET_VAL(msReturnTemplateQuery(mapserv, mapserv->map->web.queryformat, &pszBuffer));

    mapserv->map = NULL;
    mapserv->request->ParamNames = mapserv->request->ParamValues = NULL;
    mapserv->request->NumParams = 0;
    msFreeMapServObj(mapserv);
  }

  return pszBuffer;
}
