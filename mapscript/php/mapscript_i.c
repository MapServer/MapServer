/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Interface file for MapServer PHP scripting extension
 *           called MapScript. This file was originally based on
 *           the SWIG interface file mapscript.i
 * Author:   Daniel Morissette, DM Solutions Group (dmorissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2000, 2007, Daniel Morissette, DM Solutions Group Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "php_mapscript.h"

/* grab mapserver declarations to wrap
 */
#include "../../mapserver.h"
#include "../../mapows.h"
#include "../../maperror.h"
#include "../../mapprimitive.h"
#include "../../mapsymbol.h"
#include "../../mapshape.h"
#include "../../mapproject.h"
#include "../../maphash.h"

/**********************************************************************
 * class extensions for mapObj
 **********************************************************************/
mapObj *mapObj_new(char *filename, char *new_path)
{
  if(filename && strlen(filename))
    return msLoadMap(filename, new_path, NULL);
  else { /* create an empty map, no layers etc... */
    return msNewMapObj();
  }
}

mapObj *mapObj_newFromString(char *map_text, char *new_path)
{

  if(map_text && strlen(map_text))
    return msLoadMapFromString(map_text, new_path);
  else { /* create an empty map, no layers etc... */
    return msNewMapObj();
  }
}

void  mapObj_destroy(mapObj* self)
{
  msFreeMap(self);
}

mapObj *mapObj_clone(mapObj* self)
{
  mapObj *dstMap;
  dstMap = msNewMapObj();
  if (msCopyMap(dstMap, self) != MS_SUCCESS) {
    msFreeMap(dstMap);
    dstMap = NULL;
  }
  return dstMap;
}

int mapObj_setRotation(mapObj* self, double rotation_angle )
{
  return msMapSetRotation( self, rotation_angle );
}

layerObj *mapObj_getLayer(mapObj* self, int i)
{
  if(i >= 0 && i < self->numlayers)
    return (self->layers[i]); /* returns an EXISTING layer */
  else
    return NULL;
}

layerObj *mapObj_getLayerByName(mapObj* self, char *name)
{
  int i;

  i = msGetLayerIndex(self, name);

  if(i != -1)
    return (self->layers[i]); /* returns an EXISTING layer */
  else
    return NULL;
}

int *mapObj_getLayersIndexByGroup(mapObj* self, char *groupname,
                                  int *pnCount)
{
  return msGetLayersIndexByGroup(self, groupname, pnCount);
}


int mapObj_getSymbolByName(mapObj* self, char *name)
{
  return msGetSymbolIndex(&self->symbolset, name, MS_TRUE);
}

void mapObj_prepareQuery(mapObj* self)
{
  int status;

  status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scaledenom);
  if(status != MS_SUCCESS) self->scaledenom = -1; // degenerate extents ok here
}

imageObj *mapObj_prepareImage(mapObj* self)
{
  return msPrepareImage(self, MS_FALSE);
}

imageObj *mapObj_draw(mapObj* self)
{
  return msDrawMap(self, MS_FALSE);
}

imageObj *mapObj_drawQuery(mapObj* self)
{
  return msDrawMap(self, MS_TRUE);
}

imageObj *mapObj_drawLegend(mapObj* self)
{
  return msDrawLegend(self, MS_FALSE, NULL);
}


imageObj *mapObj_drawScalebar(mapObj* self)
{
  return msDrawScalebar(self);
}

imageObj *mapObj_drawReferenceMap(mapObj* self)
{
  return msDrawReferenceMap(self);
}

//TODO
int mapObj_embedScalebar(mapObj* self, imageObj *img)
{
  return msEmbedScalebar(self, img);
}

//TODO
int mapObj_embedLegend(mapObj* self, imageObj *img)
{
  return msEmbedLegend(self, img);
}

int mapObj_drawLabelCache(mapObj* self, imageObj *img)
{
  return msDrawLabelCache(self, img);
}

labelCacheMemberObj* mapObj_getLabel(mapObj* self, int i)
{
  msSetError(MS_MISCERR, "LabelCacheMember access is not available", "getLabel()");
  return NULL;
}

int mapObj_queryByPoint(mapObj* self, pointObj *point, int mode, double buffer)
{
  msInitQuery(&(self->query));

  self->query.type = MS_QUERY_BY_POINT;
  self->query.mode = mode;
  self->query.point = *point;
  self->query.buffer = buffer;

  return msQueryByPoint(self);
}

int mapObj_queryByRect(mapObj* self, rectObj rect)
{
  msInitQuery(&(self->query));

  self->query.type = MS_QUERY_BY_RECT;
  self->query.mode = MS_QUERY_MULTIPLE;
  self->query.rect = rect;

  return msQueryByRect(self);
}

int mapObj_queryByFeatures(mapObj* self, int slayer)
{
  self->query.slayer = slayer;
  return msQueryByFeatures(self);
}

int mapObj_queryByFilter(mapObj* self, char *string)
{
  msInitQuery(&(self->query));

  self->query.type = MS_QUERY_BY_FILTER;
  self->query.mode = MS_QUERY_MULTIPLE;

  self->query.filter.string = msStrdup(string);
  self->query.filter.type = MS_EXPRESSION;

  self->query.rect = self->extent;

  return msQueryByFilter(self);
}

int mapObj_queryByShape(mapObj *self, shapeObj *shape)
{
  msInitQuery(&(self->query));

  self->query.type = MS_QUERY_BY_SHAPE;
  self->query.mode = MS_QUERY_MULTIPLE;
  self->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(self->query.shape);
  msCopyShape(shape, self->query.shape);

  return msQueryByShape(self);
}

int mapObj_queryByIndex(mapObj *self, int qlayer, int tileindex, int shapeindex, int bAddToQuery)
{
  msInitQuery(&(self->query));

  self->query.type = MS_QUERY_BY_INDEX;
  self->query.mode = MS_QUERY_SINGLE;
  self->query.tileindex = tileindex;
  self->query.shapeindex = shapeindex;
  self->query.clear_resultcache = !bAddToQuery;
  self->query.layer = qlayer;

  return msQueryByIndex(self);
}

int mapObj_saveQuery(mapObj *self, char *filename, int results)
{
  return msSaveQuery(self, filename, results);
}
int mapObj_loadQuery(mapObj *self, char *filename)
{
  return msLoadQuery(self, filename);
}
void mapObj_freeQuery(mapObj *self, int qlayer)
{
  msQueryFree(self, qlayer);
}

int mapObj_setWKTProjection(mapObj *self, char *string)
{
  return msOGCWKT2ProjectionObj(string, &(self->projection), self->debug);
}

char *mapObj_getProjection(mapObj* self)
{
  return msGetProjectionString(&self->projection);
}

int mapObj_setProjection(mapObj* self, char *string)
{
  return(msLoadProjectionString(&(self->projection), string));
}

int mapObj_save(mapObj* self, char *filename)
{
  return msSaveMap(self, filename);
}

const char *mapObj_getMetaData(mapObj *self, char *name)
{
  return(msLookupHashTable(&(self->web.metadata), name));
}

int mapObj_setMetaData(mapObj *self, char *name, char *value)
{
  if (msInsertHashTable(&(self->web.metadata), name, value) == NULL)
    return MS_FAILURE;
  return MS_SUCCESS;
}

int mapObj_removeMetaData(mapObj *self, char *name)
{
  return(msRemoveHashTable(&(self->web.metadata), name));
}

int mapObj_moveLayerup(mapObj *self, int layerindex)
{
  return msMoveLayerUp(self, layerindex);
}

int mapObj_moveLayerdown(mapObj *self, int layerindex)
{
  return msMoveLayerDown(self, layerindex);
}

int *mapObj_getLayersdrawingOrder(mapObj *self)
{
  return self->layerorder;
}

int mapObj_setLayersdrawingOrder(mapObj *self, int *panIndexes)
{
  return msSetLayersdrawingOrder(self, panIndexes);
}

char *mapObj_processTemplate(mapObj *self, int bGenerateImages,
                             char **names, char **values,
                             int numentries)
{
  return msProcessTemplate(self, bGenerateImages,
                           names, values, numentries);
}

char *mapObj_processLegendTemplate(mapObj *self,
                                   char **names, char **values,
                                   int numentries)
{
  return msProcessLegendTemplate(self, names, values, numentries);
}


char *mapObj_processQueryTemplate(mapObj *self, int bGenerateImages,
                                  char **names, char **values,
                                  int numentries)
{
  return msProcessQueryTemplate(self, bGenerateImages, names, values,
                                numentries);
}

int mapObj_setSymbolSet(mapObj *self,
                        char *szFileName)
{
  msFreeSymbolSet(&self->symbolset);
  msInitSymbolSet(&self->symbolset);

  // Set symbolset filename
  self->symbolset.filename = msStrdup(szFileName);

  // Symbolset shares same fontset as main mapfile
  self->symbolset.fontset = &(self->fontset);

  return msLoadSymbolSet(&self->symbolset, self);
}

int mapObj_getNumSymbols(mapObj *self)
{
  return self->symbolset.numsymbols;
}

int mapObj_setFontSet(mapObj *self, char *szFileName)
{
  msFreeFontSet(&(self->fontset));
  msInitFontSet(&(self->fontset));

  // Set fontset filename
  self->fontset.filename = msStrdup(szFileName);

  return msLoadFontSet(&(self->fontset), self);
}

int mapObj_saveMapContext(mapObj *self, char *szFilename)
{
  return msSaveMapContext(self, szFilename);
}

int mapObj_loadMapContext(mapObj *self, char *szFilename, int bUniqueLayerName)
{
  return msLoadMapContext(self, szFilename, bUniqueLayerName);
}



int mapObj_selectOutputFormat(mapObj *self,
                              const char *imagetype)
{
  outputFormatObj *format = NULL;

  format = msSelectOutputFormat(self, imagetype);
  if (format) {
    msApplyOutputFormat( &(self->outputformat), format,
                         format->transparent );
    return(MS_SUCCESS);
  }
  return(MS_FAILURE);

}

int mapObj_applySLD(mapObj *self, char *sld)
{
  return msSLDApplySLD(self, sld, -1, NULL, NULL);
}
int mapObj_applySLDURL(mapObj *self, char *sld)
{
  return msSLDApplySLDURL(self, sld, -1, NULL, NULL);
}

char *mapObj_generateSLD(mapObj *self)
{
  return msSLDGenerateSLD(self, -1, NULL);
}


int mapObj_loadOWSParameters(mapObj *self, cgiRequestObj *request,
                             char *wmtver_string)
{
  return msMapLoadOWSParameters(self, request, wmtver_string);
}

int mapObj_OWSDispatch(mapObj *self, cgiRequestObj *req )
{
  return msOWSDispatch( self, req, MS_TRUE);
}

int mapObj_insertLayer(mapObj *self, layerObj *layer, int index)
{
  if (self && layer)
    return msInsertLayer(self, layer, index);

  return -1;
}

layerObj *mapObj_removeLayer(mapObj *self, int layerindex)
{
  return msRemoveLayer(self, layerindex);
}

int mapObj_setCenter(mapObj *self, pointObj *center)
{
  return msMapSetCenter(self, center);
}

int mapObj_offsetExtent(mapObj *self, double x, double y)
{
  return msMapOffsetExtent(self, x, y);
}

int mapObj_scaleExtent(mapObj *self, double zoomfactor, double minscaledenom,
                       double maxscaledenom)
{
  return msMapScaleExtent(self, zoomfactor, minscaledenom, maxscaledenom);
}

char *mapObj_convertToString(mapObj *self)
{
  return msWriteMapToString(self);
}

/**********************************************************************
 * class extensions for layerObj, always within the context of a map
 **********************************************************************/
layerObj *layerObj_new(mapObj *map)
{
  if(msGrowMapLayers(map) == NULL)
    return(NULL);

  if(initLayer((map->layers[map->numlayers]), map) == -1)
    return(NULL);

  map->layers[map->numlayers]->index = map->numlayers;
  //Update the layer order list with the layer's index.
  map->layerorder[map->numlayers] = map->numlayers;

  map->numlayers++;

  return (map->layers[map->numlayers-1]);
}

void layerObj_destroy(layerObj *self)
{
  /* if the layer has a parent_map, let's the map object destroy it */
  if ((self->map == NULL) && (self->refcount == 1)) {
    /* if there is no other PHP Object that use this C layer object, delete it */
    freeLayer(self);
    free(self);
    self = NULL;
  } else {
    MS_REFCNT_DECR(self);
  }
  return;
}

layerObj *layerObj_clone(layerObj *layer)
{
  layerObj *dstLayer;
  dstLayer = (layerObj *)malloc(sizeof(layerObj));
  initLayer(dstLayer, layer->map);
  msCopyLayer(dstLayer, layer);
  return dstLayer;
}


int layerObj_updateFromString(layerObj *self, char *snippet)
{
  return msUpdateLayerFromString(self, snippet);
}

char *layerObj_convertToString(layerObj *self)
{
  return msWriteLayerToString(self);
}

int layerObj_open(layerObj *self)
{
  return msLayerOpen(self);
}


int layerObj_whichShapes(layerObj *self, rectObj *poRect)
{
  int oldconnectiontype = self->connectiontype;
  self->connectiontype = MS_INLINE;

  if(msLayerWhichItems(self, MS_FALSE, NULL) != MS_SUCCESS) {
    self->connectiontype = oldconnectiontype;
    return MS_FAILURE;
  }
  self->connectiontype = oldconnectiontype;

  return msLayerWhichShapes(self, *poRect, MS_FALSE);
}


shapeObj *layerObj_nextShape(layerObj *self)
{
  int status;
  shapeObj *shape;

  shape = (shapeObj *)malloc(sizeof(shapeObj));
  if (!shape) return NULL;
  msInitShape(shape);

  status = msLayerNextShape(self, shape);
  if(status != MS_SUCCESS) {
    msFreeShape(shape);
    free(shape);
    return NULL;
  } else
    return shape;
}

void layerObj_close(layerObj *self)
{
  msLayerClose(self);
}

resultObj *layerObj_getResult(layerObj *self, int i)
{
  if(!self->resultcache) return NULL;

  if(i >= 0 && i < self->resultcache->numresults)
    return &self->resultcache->results[i];
  else
    return NULL;
}

classObj *layerObj_getClass(layerObj *self, int i)   // returns an EXISTING class
{
  if(i >= 0 && i < self->numclasses)
    return (self->class[i]);
  else
    return(NULL);
}

int layerObj_getClassIndex(layerObj *self, mapObj *map, shapeObj *shape,
                           int *classgroup, int numclasses)
{
  return msShapeGetClass(self, map, shape, classgroup, numclasses);
}

int layerObj_draw(layerObj *self, mapObj *map, imageObj *img)
{
  return msDrawLayer(map, self, img);
}

int layerObj_drawQuery(layerObj *self, mapObj *map, imageObj *img)
{
  return msDrawQueryLayer(map, self, img);
}

int layerObj_queryByAttributes(layerObj *self, mapObj *map, char *qitem, char *qstring, int mode)
{
  int status;
  int retval;

  msInitQuery(&(map->query));
  
  map->query.type = MS_QUERY_BY_FILTER;
  map->query.mode = mode;

  if(qitem) map->query.filteritem = msStrdup(qitem);
  if(qstring) {
    msInitExpression(&map->query.filter);
    msLoadExpressionString(&map->query.filter, qstring);
  }

  map->query.layer = self->index;
  map->query.rect = map->extent;

  status = self->status;
  self->status = MS_ON;
  retval = msQueryByFilter(map);
  self->status = status;

  return retval;
}

int layerObj_queryByPoint(layerObj *self, mapObj *map, pointObj *point, int mode, double buffer)
{
  int status;
  int retval;

  msInitQuery(&(map->query));

  map->query.type = MS_QUERY_BY_POINT;
  map->query.mode = mode;
  map->query.point = *point;
  map->query.buffer = buffer;
  map->query.layer = self->index;

  status = self->status;
  self->status = MS_ON;
  retval = msQueryByPoint(map);
  self->status = status;

  return retval;
}

int layerObj_queryByRect(layerObj *self, mapObj *map, rectObj rect)
{
  int status;
  int retval;

  msInitQuery(&(map->query));

  map->query.type = MS_QUERY_BY_RECT;
  map->query.mode = MS_QUERY_MULTIPLE;
  map->query.rect = rect;
  map->query.layer = self->index;

  status = self->status;
  self->status = MS_ON;
  retval = msQueryByRect(map);
  self->status = status;

  return retval;
}

int layerObj_queryByFeatures(layerObj *self, mapObj *map, int slayer)
{
  int status;
  int retval;

  map->query.slayer = slayer;
  map->query.layer = self->index;

  status = self->status;
  self->status = MS_ON;
  retval = msQueryByFeatures(map);
  self->status = status;

  return retval;
}

int layerObj_queryByShape(layerObj *self, mapObj *map, shapeObj *shape)
{
  int status;
  int retval;

  msInitQuery(&(map->query));

  map->query.type = MS_QUERY_BY_SHAPE;
  map->query.mode = MS_QUERY_MULTIPLE;
  map->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(map->query.shape);
  msCopyShape(shape, map->query.shape);
  map->query.layer = self->index;

  status = self->status;
  self->status = MS_ON;
  retval = msQueryByShape(map);
  self->status = status;

  return retval;
}

int layerObj_queryByFilter(layerObj *self, mapObj *map, char *string)
{
  int status;
  int retval;

  msInitQuery(&(map->query));

  map->query.type = MS_QUERY_BY_FILTER;
  map->query.mode = MS_QUERY_MULTIPLE;

  map->query.filter.string = msStrdup(string);
  map->query.filter.type = MS_EXPRESSION;

  map->query.layer = self->index;
  map->query.rect = map->extent;

  status = self->status;
  self->status = MS_ON;
  retval = msQueryByFilter(map);
  self->status = status;

  return retval;
}

int layerObj_queryByIndex(layerObj *self, mapObj *map, int tileindex, int shapeindex, int addtoquery)
{
  int status;
  int retval;

  msInitQuery(&(map->query));

  map->query.type = MS_QUERY_BY_INDEX;
  map->query.mode = MS_QUERY_SINGLE;
  map->query.tileindex = tileindex;
  map->query.shapeindex = shapeindex;
  map->query.clear_resultcache = !addtoquery;
  map->query.layer = self->index;

  status = self->status;
  self->status = MS_ON;
  retval = msQueryByIndex(map);
  self->status = status;

  return retval;
}

int layerObj_setFilter(layerObj *self, char *string)
{
  if (!string || strlen(string) == 0) {
    msFreeExpression(&self->filter);
    return MS_SUCCESS;
  } else return msLoadExpressionString(&self->filter, string);
}

char *layerObj_getFilter(layerObj *self)
{
  return msGetExpressionString(&(self->filter));
}

int layerObj_setWKTProjection(layerObj *self, char *string)
{
  self->project = MS_TRUE;
  return msOGCWKT2ProjectionObj(string, &(self->projection), self->debug);
}

char *layerObj_getProjection(layerObj* self)
{
  return msGetProjectionString(&self->projection);
}

int layerObj_setProjection(layerObj *self, char *string)
{
  int nReturn;
  nReturn = msLoadProjectionString(&(self->projection), string);
  if (nReturn == MS_SUCCESS)
    self->project = MS_TRUE;
  return nReturn;

}

int layerObj_addFeature(layerObj *self, shapeObj *shape)
{
  if(self->features != NULL && self->features->tailifhead != NULL)
    shape->index = self->features->tailifhead->shape.index + 1;
  else
    shape->index = 0;
  if(insertFeatureList(&(self->features), shape) == NULL)
    return MS_FAILURE;
  else
    return MS_SUCCESS;
}

const char *layerObj_getMetaData(layerObj *self, char *name)
{
  return(msLookupHashTable(&(self->metadata), name));
}


int layerObj_setMetaData(layerObj *self, char *name, char *value)
{
  if (msInsertHashTable(&(self->metadata), name, value) == NULL)
    return MS_FAILURE;
  return MS_SUCCESS;
}

int layerObj_removeMetaData(layerObj *self, char *name)
{
  return(msRemoveHashTable(&(self->metadata), name));
}

char *layerObj_getWMSFeatureInfoURL(layerObj *self, mapObj *map, int click_x, int click_y,
                                    int feature_count, char *info_format)
{
  // NOTE: the returned string should be freed by the caller but right
  // now we're leaking it.
  return(msWMSGetFeatureInfoURL(map, self, click_x, click_y,
                                feature_count, info_format));
}

char *layerObj_executeWFSGetFeature(layerObj *self)
{
  return (msWFSExecuteGetFeature(self));
}

int layerObj_applySLD(layerObj *self, char *sld, char *stylelayer)
{
  return msSLDApplySLD(self->map, sld, self->index, stylelayer, NULL);
}
int layerObj_applySLDURL(layerObj *self, char *sld, char *stylelayer)
{
  return msSLDApplySLDURL(self->map, sld, self->index, stylelayer, NULL);
}

char *layerObj_generateSLD(layerObj *self)
{
  return msSLDGenerateSLD(self->map, self->index, NULL);
}

int layerObj_moveClassUp(layerObj *self, int index)
{
  return msMoveClassUp(self, index);
}

int layerObj_moveClassDown(layerObj *self, int index)
{
  return msMoveClassDown(self, index);
}

classObj *layerObj_removeClass(layerObj *self, int index)
{
  return msRemoveClass(self, index);
}

int layerObj_setConnectionType(layerObj *self, int connectiontype,
                               const char *library_str)
{
  /* Caller is responsible to close previous layer correctly before calling
   * msConnectLayer()
   */
  if (msLayerIsOpen(self))
    msLayerClose(self);

  return msConnectLayer(self, connectiontype, library_str);
}

/**********************************************************************
 * class extensions for labelObj
 **********************************************************************/
labelObj *labelObj_new()
{
  labelObj *label;

  label = (labelObj *)malloc(sizeof(labelObj));
  if(!label)
    return(NULL);

  initLabel(label);

  return label;
}

void labelObj_destroy(labelObj *self)
{
  if (freeLabel(self) == MS_SUCCESS)
    free(self);

}

labelObj *labelObj_clone(labelObj *label)
{
  labelObj *dstLabel;
  dstLabel = (labelObj *)malloc(sizeof(labelObj));
  initLabel(dstLabel);
  dstLabel->font = NULL;
  msCopyLabel(dstLabel, label);

  return dstLabel;
}

int labelObj_updateFromString(labelObj *self, char *snippet)
{
  return msUpdateLabelFromString(self, snippet);
}

char *labelObj_convertToString(labelObj *self)
{
  return msWriteLabelToString(self);
}

int labelObj_moveStyleUp(labelObj *self, int index)
{
  return msMoveLabelStyleUp(self, index);
}

int labelObj_moveStyleDown(labelObj *self, int index)
{
  return msMoveLabelStyleDown(self, index);
}

int labelObj_deleteStyle(labelObj *self, int index)
{
  return msDeleteLabelStyle(self, index);
}

int labelObj_setExpression(labelObj *self, char *string)
{
  if (!string || strlen(string) == 0) {
    msFreeExpression(&self->expression);
    return MS_SUCCESS;
  } else return msLoadExpressionString(&self->expression, string);
}

char *labelObj_getExpressionString(labelObj *self)
{
  return msGetExpressionString(&(self->expression));
}

int labelObj_setText(labelObj *self, layerObj *layer, char *string)
{
  if (!string || strlen(string) == 0) {
    msFreeExpression(&self->text);
    return MS_SUCCESS;
  }
  return msLoadExpressionString(&self->text, string);
}

char *labelObj_getTextString(labelObj *self)
{
  return msGetExpressionString(&(self->text));
}

/**********************************************************************
 * class extensions for legendObj
 **********************************************************************/
int legendObj_updateFromString(legendObj *self, char *snippet)
{
  return msUpdateLegendFromString(self, snippet);
}

char *legendObj_convertToString(legendObj *self)
{
  return msWriteLegendToString(self);
}

/**********************************************************************
 * class extensions for queryMapObj
 **********************************************************************/
int queryMapObj_updateFromString(queryMapObj *self, char *snippet)
{
  return msUpdateQueryMapFromString(self, snippet);
}

char *queryMapObj_convertToString(queryMapObj *self)
{
  return msWriteQueryMapToString(self);
}

/**********************************************************************
 * class extensions for referenceMapObj
 **********************************************************************/

int referenceMapObj_updateFromString(referenceMapObj *self, char *snippet)
{
  return msUpdateReferenceMapFromString(self, snippet);
}

char *referenceMapObj_convertToString(referenceMapObj *self)
{
  return msWriteReferenceMapToString(self);
}

/**********************************************************************
 * class extensions for scaleBarObj
 **********************************************************************/

int scalebarObj_updateFromString(scalebarObj *self, char *snippet)
{
  return msUpdateScalebarFromString(self, snippet);
}

char *scalebarObj_convertToString(scalebarObj *self)
{
  return msWriteScalebarToString(self);
}

/**********************************************************************
 * class extensions for webObj
 **********************************************************************/

int webObj_updateFromString(webObj *self, char *snippet)
{
  return msUpdateWebFromString(self, snippet);
}

char *webObj_convertToString(webObj *self)
{
  return msWriteWebToString(self);
}

/**********************************************************************
 * class extensions for classObj, always within the context of a layer
 **********************************************************************/
classObj *classObj_new(layerObj *layer, classObj *class)
{
  if(msGrowLayerClasses(layer) == NULL)
    return NULL;

  if(initClass((layer->class[layer->numclasses])) == -1)
    return NULL;

  if (class) {
    msCopyClass((layer->class[layer->numclasses]), class, layer);
    layer->class[layer->numclasses]->layer = layer;
  }

  layer->class[layer->numclasses]->layer = layer;

  layer->numclasses++;

  return (layer->class[layer->numclasses-1]);
}

int classObj_addLabel(classObj *self, labelObj *label)
{
  return msAddLabelToClass(self, label);
}

labelObj *classObj_removeLabel(classObj *self, int index)
{
  return msRemoveLabelFromClass(self, index);
}

labelObj *classObj_getLabel(classObj *self, int i)   // returns an EXISTING label
{
  if(i >= 0 && i < self->numlabels)
    return (self->labels[i]);
  else
    return(NULL);
}

int classObj_updateFromString(classObj *self, char *snippet)
{
  return msUpdateClassFromString(self, snippet);
}

char *classObj_convertToString(classObj *self)
{
  return msWriteClassToString(self);
}

void  classObj_destroy(classObj *self)
{
  return; /* do nothing, map deconstrutor takes care of it all */
}

int classObj_setExpression(classObj *self, char *string)
{
  if (!string || strlen(string) == 0) {
    msFreeExpression(&self->expression);
    return MS_SUCCESS;
  } else return msLoadExpressionString(&self->expression, string);
}

char *classObj_getExpressionString(classObj *self)
{
  return msGetExpressionString(&(self->expression));
}

int classObj_setText(classObj *self, layerObj *layer, char *string)
{
  if (!string || strlen(string) == 0) {
    msFreeExpression(&self->text);
    return MS_SUCCESS;
  }
  return msLoadExpressionString(&self->text, string);
}

char *classObj_getTextString(classObj *self)
{
  return msGetExpressionString(&(self->text));
}

int classObj_drawLegendIcon(classObj *self, mapObj *map, layerObj *layer, int width, int height, imageObj *dstImg, int dstX, int dstY)
{
#ifdef USE_GD
  msClearLayerPenValues(layer); // just in case the mapfile has already been processed
#endif
  return msDrawLegendIcon(map, layer, self, width, height, dstImg, dstX, dstY, MS_TRUE, NULL);
}

imageObj *classObj_createLegendIcon(classObj *self, mapObj *map, layerObj *layer, int width, int height)
{
  return msCreateLegendIcon(map, layer, self, width, height, MS_TRUE);
}


int classObj_setSymbolByName(classObj *self, mapObj *map, char* pszSymbolName)
{
  /*
   self->symbol = msGetSymbolIndex(&map->symbolset, pszSymbolName, MS_TRUE);
    return self->symbol;
  */
  return -1;
}

int classObj_setOverlaySymbolByName(classObj *self, mapObj *map, char* pszOverlaySymbolName)
{
  /*
    self->overlaysymbol = msGetSymbolIndex(&map->symbolset, pszOverlaySymbolName, MS_TRUE);
    return self->overlaysymbol;
  */
  return -1;

}

classObj *classObj_clone(classObj *class, layerObj *layer)
{
  classObj *dstClass;
  dstClass = (classObj *)malloc(sizeof(classObj));

  initClass(dstClass);
  msCopyClass(dstClass, class, layer);

  return dstClass;
}

int classObj_moveStyleUp(classObj *self, int index)
{
  return msMoveStyleUp(self, index);
}

int classObj_moveStyleDown(classObj *self, int index)
{
  return msMoveStyleDown(self, index);
}

int classObj_deleteStyle(classObj *self, int index)
{
  return msDeleteStyle(self, index);
}

/**********************************************************************
 * class extensions for pointObj, useful many places
 **********************************************************************/
pointObj *pointObj_new()
{
  return (pointObj *)malloc(sizeof(pointObj));
}

void pointObj_destroy(pointObj *self)
{
  free(self);
}

int pointObj_project(pointObj *self, projectionObj *in, projectionObj *out)
{
  return msProjectPoint(in, out, self);
}

int pointObj_draw(pointObj *self, mapObj *map, layerObj *layer,
                  imageObj *img, int class_index, char *label_string)
{
  return msDrawPoint(map, layer, self, img, class_index, label_string);
}

double pointObj_distanceToPoint(pointObj *self, pointObj *point)
{
  return msDistancePointToPoint(self, point);
}

double pointObj_distanceToLine(pointObj *self, pointObj *a, pointObj *b)
{
  return msDistancePointToSegment(self, a, b);
}

double pointObj_distanceToShape(pointObj *self, shapeObj *shape)
{
  return msDistancePointToShape(self, shape);
}

/**********************************************************************
 * class extensions for lineObj (eg. a line or group of points),
 * useful many places
 **********************************************************************/
lineObj *lineObj_new()
{
  lineObj *line;

  line = (lineObj *)malloc(sizeof(lineObj));
  if(!line)
    return(NULL);

  line->numpoints=0;
  line->point=NULL;

  return line;
}

void lineObj_destroy(lineObj *self)
{
  free(self->point);
  free(self);
}

lineObj *lineObj_clone(lineObj *line)
{
  lineObj *dstLine;
  dstLine = (lineObj *)malloc(sizeof(lineObj));
  dstLine->numpoints= line->numpoints;
  dstLine->point = (pointObj *)malloc(sizeof(pointObj)*(dstLine->numpoints));
  msCopyLine(dstLine, line);


  return dstLine;
}

int lineObj_project(lineObj *self, projectionObj *in, projectionObj *out)
{
  return msProjectLine(in, out, self);
}

pointObj *lineObj_get(lineObj *self, int i)
{
  if(i<0 || i>=self->numpoints)
    return NULL;
  else
    return &(self->point[i]);
}

int lineObj_add(lineObj *self, pointObj *p)
{
  if(self->numpoints == 0) { /* new */
    self->point = (pointObj *)malloc(sizeof(pointObj));
    if(!self->point)
      return MS_FAILURE;
  } else { /* extend array */
    self->point = (pointObj *)realloc(self->point, sizeof(pointObj)*(self->numpoints+1));
    if(!self->point)
      return MS_FAILURE;
  }

  self->point[self->numpoints].x = p->x;
  self->point[self->numpoints].y = p->y;
  self->point[self->numpoints].z = p->z;
  self->point[self->numpoints].m = p->m;
  self->numpoints++;

  return MS_SUCCESS;
}


/**********************************************************************
 * class extensions for shapeObj
 **********************************************************************/
shapeObj *shapeObj_new(int type)
{
  shapeObj *shape;

  shape = (shapeObj *)malloc(sizeof(shapeObj));
  if(!shape)
    return NULL;

  msInitShape(shape);
  shape->type = type;
  return shape;
}

void shapeObj_destroy(shapeObj *self)
{
  msFreeShape(self);
  free(self);
}

int shapeObj_project(shapeObj *self, projectionObj *in, projectionObj *out)
{
  return msProjectShape(in, out, self);
}

lineObj *shapeObj_get(shapeObj *self, int i)
{
  if(i<0 || i>=self->numlines)
    return NULL;
  else
    return &(self->line[i]);
}

int shapeObj_add(shapeObj *self, lineObj *line)
{
  return msAddLine(self, line);
}

int shapeObj_draw(shapeObj *self, mapObj *map, layerObj *layer,
                  imageObj *img)
{
  return msDrawShape(map, layer, self, img, -1, MS_DRAWMODE_FEATURES|MS_DRAWMODE_LABELS);
}

void shapeObj_setBounds(shapeObj *self)
{
  int i, j;

  self->bounds.minx = self->bounds.maxx = self->line[0].point[0].x;
  self->bounds.miny = self->bounds.maxy = self->line[0].point[0].y;

  for( i=0; i<self->numlines; i++ ) {
    for( j=0; j<self->line[i].numpoints; j++ ) {
      self->bounds.minx = MS_MIN(self->bounds.minx, self->line[i].point[j].x);
      self->bounds.maxx = MS_MAX(self->bounds.maxx, self->line[i].point[j].x);
      self->bounds.miny = MS_MIN(self->bounds.miny, self->line[i].point[j].y);
      self->bounds.maxy = MS_MAX(self->bounds.maxy, self->line[i].point[j].y);
    }
  }

  return;
}

int shapeObj_copy(shapeObj *self, shapeObj *dest)
{
  return(msCopyShape(self, dest));
}

int shapeObj_contains(shapeObj *self, pointObj *point)
{
  if(self->type == MS_SHAPE_POLYGON)
    return msIntersectPointPolygon(point, self);

  return -1;
}

int shapeObj_intersects(shapeObj *self, shapeObj *shape)
{
  switch(self->type) {
    case(MS_SHAPE_LINE):
      switch(shape->type) {
        case(MS_SHAPE_LINE):
          return msIntersectPolylines(self, shape);
        case(MS_SHAPE_POLYGON):
          return msIntersectPolylinePolygon(self, shape);
      }
      break;
    case(MS_SHAPE_POLYGON):
      switch(shape->type) {
        case(MS_SHAPE_LINE):
          return msIntersectPolylinePolygon(shape, self);
        case(MS_SHAPE_POLYGON):
          return msIntersectPolygons(self, shape);
      }
      break;
  }

  return -1;
}

pointObj *shapeObj_getpointusingmeasure(shapeObj *self, double m)
{
  if (self)
    return msGetPointUsingMeasure(self, m);

  return NULL;
}

pointObj *shapeObj_getmeasureusingpoint(shapeObj *self, pointObj *point)
{
  if (self)
    return msGetMeasureUsingPoint(self, point);

  return NULL;
}


shapeObj *shapeObj_buffer(shapeObj *self, double width)
{
  return msGEOSBuffer(self, width);
}


shapeObj *shapeObj_simplify(shapeObj *self, double tolerance)
{
  return msGEOSSimplify(self, tolerance);
}


shapeObj *shapeObj_topologypreservingsimplify(shapeObj *self, double tolerance)
{
  return msGEOSTopologyPreservingSimplify(self, tolerance);
}


shapeObj *shapeObj_convexHull(shapeObj *self)
{
  return msGEOSConvexHull(self);
}

shapeObj *shapeObj_boundary(shapeObj *self)
{
  return msGEOSBoundary(self);
}

int shapeObj_contains_geos(shapeObj *self, shapeObj *shape)
{
  return msGEOSContains(self, shape);
}

shapeObj *shapeObj_Union(shapeObj *self, shapeObj *shape)
{
  return msGEOSUnion(self, shape);
}

shapeObj *shapeObj_intersection(shapeObj *self, shapeObj *shape)
{
  return msGEOSIntersection(self, shape);
}

shapeObj *shapeObj_difference(shapeObj *self, shapeObj *shape)
{
  return msGEOSDifference(self, shape);
}

shapeObj *shapeObj_symdifference(shapeObj *self, shapeObj *shape)
{
  return msGEOSSymDifference(self, shape);
}

int shapeObj_overlaps(shapeObj *self, shapeObj *shape)
{
  return msGEOSOverlaps(self, shape);
}

int shapeObj_within(shapeObj *self, shapeObj *shape)
{
  return msGEOSWithin(self, shape);
}

int shapeObj_crosses(shapeObj *self, shapeObj *shape)
{
  return msGEOSCrosses(self, shape);
}

int shapeObj_touches(shapeObj *self, shapeObj *shape)
{
  return msGEOSTouches(self, shape);
}

int shapeObj_equals(shapeObj *self, shapeObj *shape)
{
  return msGEOSEquals(self, shape);
}

int shapeObj_disjoint(shapeObj *self, shapeObj *shape)
{
  return msGEOSDisjoint(self, shape);
}


pointObj *shapeObj_getcentroid(shapeObj *self)
{
  return msGEOSGetCentroid(self);
}

double shapeObj_getarea(shapeObj *self)
{
  return msGEOSArea(self);
}

double shapeObj_getlength(shapeObj *self)
{
  return msGEOSLength(self);
}

pointObj *shapeObj_getLabelPoint(shapeObj *self)
{
  pointObj *point = (pointObj *)calloc(1, sizeof(pointObj));
  if (point == NULL) {
    msSetError(MS_MEMERR, "Failed to allocate memory for point", "getLabelPoint()");
    return NULL;
  }

  if(self->type == MS_SHAPE_POLYGON && msPolygonLabelPoint(self, point, -1) == MS_SUCCESS)
    return point;

  free(point);
  return NULL;
}


/**********************************************************************
 * class extensions for rectObj
 **********************************************************************/
rectObj *rectObj_new()
{
  rectObj *rect;

  rect = (rectObj *)calloc(1, sizeof(rectObj));
  if(!rect)
    return(NULL);

  rect->minx = -1;
  rect->miny = -1;
  rect->maxx = -1;
  rect->maxy = -1;

  return(rect);
}

void rectObj_destroy(rectObj *self)
{
  free(self);
}

int rectObj_project(rectObj *self, projectionObj *in, projectionObj *out)
{
  return msProjectRect(in, out, self);
}

double rectObj_fit(rectObj *self, int width, int height)
{
  return  msAdjustExtent(self, width, height);
}

int rectObj_draw(rectObj *self, mapObj *map, layerObj *layer,
                 imageObj *img, int classindex, char *text)
{
  shapeObj shape;

  msInitShape(&shape);
  msRectToPolygon(*self, &shape);
  shape.classindex = classindex;

  if(text && layer->class[classindex]->numlabels > 0) {
    shape.text = msStrdup(text);
  }
  
  if(MS_SUCCESS != msDrawShape(map, layer, &shape, img, -1, MS_DRAWMODE_FEATURES|MS_DRAWMODE_LABELS)) {
    /* error message has been set already */
  }

  msFreeShape(&shape);

  return 0;
}

/**********************************************************************
 * class extensions for shapefileObj
 **********************************************************************/
shapefileObj *shapefileObj_new(char *filename, int type)
{
  shapefileObj *shapefile;
  int status;

  shapefile = (shapefileObj *)calloc(1,sizeof(shapefileObj));
  if(!shapefile)
    return NULL;

  if(type == -1)
    status = msShapefileOpen(shapefile, "rb", filename, MS_TRUE);
  else if (type == -2)
    status = msShapefileOpen(shapefile, "rb+", filename, MS_TRUE);
  else
    status = msShapefileCreate(shapefile, filename, type);

  if(status == -1) {
    msShapefileClose(shapefile);
    free(shapefile);
    return NULL;
  }

  return(shapefile);
}

void shapefileObj_destroy(shapefileObj *self)
{
  msShapefileClose(self);
  free(self);
}

int shapefileObj_get(shapefileObj *self, int i, shapeObj *shape)
{
  if(i<0 || i>=self->numshapes)
    return -1;

  msFreeShape(shape); /* frees all lines and points before re-filling */
  msSHPReadShape(self->hSHP, i, shape);
  self->lastshape = i;

  return MS_SUCCESS;
}

int shapefileObj_getPoint(shapefileObj *self, int i, pointObj *point)
{
  if(i<0 || i>=self->numshapes)
    return -1;

  return msSHPReadPoint(self->hSHP, i, point);
}

int shapefileObj_getTransformed(shapefileObj *self, mapObj *map,
                                int i, shapeObj *shape)
{
  if(i<0 || i>=self->numshapes)
    return -1;

  msFreeShape(shape); /* frees all lines and points before re-filling */
  msSHPReadShape(self->hSHP, i, shape);
  msTransformShapeSimplify(shape, map->extent, map->cellsize);

  return 0;
}

void shapefileObj_getExtent(shapefileObj *self, int i, rectObj *rect)
{
  msSHPReadBounds(self->hSHP, i, rect);
}

int shapefileObj_add(shapefileObj *self, shapeObj *shape)
{
  return msSHPWriteShape(self->hSHP, shape);
}

int shapefileObj_addPoint(shapefileObj *self, pointObj *point)
{
  return msSHPWritePoint(self->hSHP, point);
}

/**********************************************************************
 * class extensions for projectionObj
 **********************************************************************/
projectionObj *projectionObj_new(char *string)
{

  int status;
  projectionObj *proj=NULL;

  proj = (projectionObj *)malloc(sizeof(projectionObj));
  if(!proj) return NULL;
  msInitProjection(proj);

  status = msLoadProjectionString(proj, string);
  if(status == -1) {
    msFreeProjection(proj);
    free(proj);
    return NULL;
  }

  return proj;
}

int projectionObj_getUnits(projectionObj *self)
{
  return GetMapserverUnitUsingProj(self);
}

void projectionObj_destroy(projectionObj *self)
{
  msFreeProjection(self);
  free(self);
}

projectionObj *projectionObj_clone(projectionObj *projection)
{
  projectionObj *dstProjection;
  dstProjection = (projectionObj *)malloc(sizeof(projectionObj));
  msInitProjection(dstProjection);
  msCopyProjection(dstProjection, projection);

  return dstProjection;
}

/**********************************************************************
 * class extensions for labelCacheObj - TP mods
 **********************************************************************/
void labelCacheObj_freeCache(labelCacheObj *self)
{
  msFreeLabelCache(self);

}

/**********************************************************************
 * class extensions for DBFInfo - TP mods
 **********************************************************************/
char *DBFInfo_getFieldName(DBFInfo *self, int iField)
{
  static char pszFieldName[1000];
  int pnWidth;
  int pnDecimals;
  msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
  return pszFieldName;
}

int DBFInfo_getFieldWidth(DBFInfo *self, int iField)
{
  char pszFieldName[1000];
  int pnWidth;
  int pnDecimals;
  msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
  return pnWidth;
}

int DBFInfo_getFieldDecimals(DBFInfo *self, int iField)
{
  char pszFieldName[1000];
  int pnWidth;
  int pnDecimals;
  msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
  return pnDecimals;
}

DBFFieldType DBFInfo_getFieldType(DBFInfo *self, int iField)
{
  return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
}

/**********************************************************************
 * class extensions for styleObj, always within the context of a class
 **********************************************************************/
styleObj *styleObj_new(classObj *class, styleObj *style)
{
  if(msGrowClassStyles(class) == NULL)
    return NULL;

  if(initStyle(class->styles[class->numstyles]) == -1)
    return NULL;

  if (style)
    msCopyStyle(class->styles[class->numstyles], style);

  class->numstyles++;

  return class->styles[class->numstyles-1];
}

styleObj *styleObj_label_new(labelObj *label, styleObj *style)
{
  if(msGrowLabelStyles(label) == NULL)
    return NULL;

  if(initStyle(label->styles[label->numstyles]) == -1)
    return NULL;

  if (style)
    msCopyStyle(label->styles[label->numstyles], style);

  label->numstyles++;

  return label->styles[label->numstyles-1];
}

int styleObj_updateFromString(styleObj *self, char *snippet)
{
  return msUpdateStyleFromString(self, snippet);
}

char *styleObj_convertToString(styleObj *self)
{
  return msWriteStyleToString(self);
}

int styleObj_setSymbolByName(styleObj *self, mapObj *map, char* pszSymbolName)
{
  self->symbol = msGetSymbolIndex(&map->symbolset, pszSymbolName, MS_TRUE);
  return self->symbol;
}

styleObj *styleObj_clone(styleObj *style)
{
  styleObj *newstyle = NULL;
  if (!style)
    return NULL;

  newstyle = (styleObj *)malloc(sizeof(styleObj));
  initStyle(newstyle);

  msCopyStyle(newstyle, style);

  return newstyle;
}

void styleObj_setGeomTransform(styleObj *style, char *transform)
{
  if (!style)
    return;

  msStyleSetGeomTransform(style, transform);
}


cgiRequestObj *cgirequestObj_new()
{
  cgiRequestObj *request;
  request = msAllocCgiObj();

  return request;
}

int cgirequestObj_loadParams(cgiRequestObj *self,
                             char* (*getenv2)(const char*, void* thread_context),
                             char *raw_post_data,
                             ms_uint32 raw_post_data_length,
                             void* thread_context)
{
  self->NumParams = loadParams(self, getenv2, raw_post_data, raw_post_data_length, thread_context);
  return self->NumParams;
}


void cgirequestObj_setParameter(cgiRequestObj *self, char *name, char *value)
{
  int i;

  if (self->NumParams == MS_DEFAULT_CGI_PARAMS) {
    msSetError(MS_CHILDERR, "Maximum number of items, %d, has been reached", "setItem()", MS_DEFAULT_CGI_PARAMS);
  }

  for (i=0; i<self->NumParams; i++) {
    if (strcasecmp(self->ParamNames[i], name) == 0) {
      free(self->ParamValues[i]);
      self->ParamValues[i] = msStrdup(value);
      break;
    }
  }
  if (i == self->NumParams) {
    self->ParamNames[self->NumParams] = msStrdup(name);
    self->ParamValues[self->NumParams] = msStrdup(value);
    self->NumParams++;
  }
}

void cgirequestObj_addParameter(cgiRequestObj *self, char *name, char *value)
{
  if (self->NumParams == MS_DEFAULT_CGI_PARAMS) {
    msSetError(MS_CHILDERR, "Maximum number of items, %d, has been reached", "addParameter()", MS_DEFAULT_CGI_PARAMS);
  }
  self->ParamNames[self->NumParams] = msStrdup(name);
  self->ParamValues[self->NumParams] = msStrdup(value);
  self->NumParams++;
}

char *cgirequestObj_getName(cgiRequestObj *self, int index)
{
  if (index < 0 || index >= self->NumParams) {
    msSetError(MS_CHILDERR, "Invalid index, valid range is [0, %d]", "getName()", self->NumParams-1);
    return NULL;
  }
  return self->ParamNames[index];
}

char *cgirequestObj_getValue(cgiRequestObj *self, int index)
{
  if (index < 0 || index >= self->NumParams) {
    msSetError(MS_CHILDERR, "Invalid index, valid range is [0, %d]", "getValue()", self->NumParams-1);
    return NULL;
  }
  return self->ParamValues[index];
}

char *cgirequestObj_getValueByName(cgiRequestObj *self, const char *name)
{
  int i;
  for (i=0; i<self->NumParams; i++) {
    if (strcasecmp(self->ParamNames[i], name) == 0) {
      return self->ParamValues[i];
    }
  }
  return NULL;
}
void cgirequestObj_destroy(cgiRequestObj *self)
{
  free(self);
}


/**********************************************************************
 * class extensions hashTableObj
 **********************************************************************/

// New instance
hashTableObj *hashTableObj_new()
{
  return msCreateHashTable();
}

// set a hash item given key and value
int hashTableObj_set(hashTableObj *self, const char *key, const char *value)
{
  if (msInsertHashTable(self, key, value) == NULL) {
    return MS_FAILURE;
  }
  return MS_SUCCESS;
}

// get value from item by its key
const char *hashTableObj_get(hashTableObj *self, const char *key)
{
  return (msLookupHashTable(self, key));
}

// Remove one item from hash table
int hashTableObj_remove(hashTableObj *self, const char *key)
{
  return (msRemoveHashTable(self, key));
}

// Clear all items in hash table (to NULL)
void hashTableObj_clear(hashTableObj *self)
{
  msFreeHashItems(self);
  initHashTable(self);
}

// Return the next key or first key if previousKey == NULL
char *hashTableObj_nextKey(hashTableObj *self, const char *previousKey)
{
  return ((char *)msNextKeyFromHashTable(self, previousKey));
}

resultObj *resultObj_new()
{
  resultObj *r = (resultObj *) msSmallMalloc(sizeof(resultObj));
  r->tileindex = -1;
  r->shapeindex = -1;
  r->resultindex = -1;
  return r;
}

/**********************************************************************
 * class extensions clusterObj
 **********************************************************************/

int clusterObj_updateFromString(clusterObj *self, char *snippet)
{
  return msUpdateClusterFromString(self, snippet);
}

char *clusterObj_convertToString(clusterObj *self)
{
  return msWriteClusterToString(self);
}

int clusterObj_setGroup(clusterObj *self, char *string)
{
  if (!string || strlen(string) == 0) {
    msFreeExpression(&self->group);
    return MS_SUCCESS;
  } else return msLoadExpressionString(&self->group, string);
}

char *clusterObj_getGroupString(clusterObj *self)
{
  return msGetExpressionString(&(self->group));
}

int clusterObj_setFilter(clusterObj *self, char *string)
{
  if (!string || strlen(string) == 0) {
    msFreeExpression(&self->filter);
    return MS_SUCCESS;
  } else return msLoadExpressionString(&self->filter, string);
}

char *clusterObj_getFilterString(clusterObj *self)
{
  return msGetExpressionString(&(self->filter));
}

outputFormatObj* outputFormatObj_new(const char *driver, char *name)
{
  outputFormatObj *format;

  format = msCreateDefaultOutputFormat(NULL, driver, name, NULL);

  /* in the case of unsupported formats, msCreateDefaultOutputFormat
     should return NULL */
  if (!format) {
    msSetError(MS_MISCERR, "Unsupported format driver: %s",
               "outputFormatObj()", driver);
    return NULL;
  }

  msInitializeRendererVTable(format);

  /* Else, continue */
  format->refcount++;
  format->inmapfile = MS_TRUE;

  return format;
}

void  outputFormatObj_destroy(outputFormatObj* self)
{
  if ( --self->refcount < 1 )
    msFreeOutputFormat( self );
}

imageObj *symbolObj_getImage(symbolObj *self, outputFormatObj *input_format)
{
  imageObj *image = NULL;
  outputFormatObj *format = NULL;
  rendererVTableObj *renderer = NULL;
  int retval;

  if (self->type != MS_SYMBOL_PIXMAP) {
    msSetError(MS_SYMERR, "Can't return image from non-pixmap symbol",
               "getImage()");
    return NULL;
  }

  if (input_format) {
    format = input_format;
  } else {
    format = msCreateDefaultOutputFormat(NULL, "AGG/PNG", "png", NULL);

    if (format)
      msInitializeRendererVTable(format);
  }

  if (format == NULL) {
    msSetError(MS_IMGERR, "Could not create output format",
               "getImage()");
    return NULL;
  }

  renderer = format->vtable;
  msPreloadImageSymbol(renderer, self);
  if (self->pixmap_buffer) {
    image = msImageCreate(self->pixmap_buffer->width, self->pixmap_buffer->height, format, NULL, NULL,
                          MS_DEFAULT_RESOLUTION, MS_DEFAULT_RESOLUTION, NULL);
    if(!image) {
      return NULL;
    }
    retval = renderer->mergeRasterBuffer(image, self->pixmap_buffer, 1.0, 0, 0, 0, 0,
                                self->pixmap_buffer->width, self->pixmap_buffer->height);
    if(retval != MS_SUCCESS) {
      msFreeImage(image);
      return NULL;
    }
  }

  return image;
}

int symbolObj_setImage(symbolObj *self, imageObj *image)
{
  rendererVTableObj *renderer = NULL;

  renderer = image->format->vtable;

  if (self->pixmap_buffer) {
    msFreeRasterBuffer(self->pixmap_buffer);
    free(self->pixmap_buffer);
  }

  self->pixmap_buffer = (rasterBufferObj*)malloc(sizeof(rasterBufferObj));
  if (!self->pixmap_buffer) {
    msSetError(MS_MEMERR, NULL, "setImage()");
    return MS_FAILURE;
  }
  self->type = MS_SYMBOL_PIXMAP;
  if(renderer->getRasterBufferCopy(image, self->pixmap_buffer) != MS_SUCCESS)
    return MS_FAILURE;

  return MS_SUCCESS;
}
