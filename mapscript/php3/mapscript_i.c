/*
 * mapscript_i.c: interface file for MapServer PHP scripting extension 
 *                called MapScript.
 *
 * This file was originally based on the SWIG interface file mapscript.i
 *
 * $Id$
 *
 * $Log$
 * Revision 1.18  2001/03/28 02:10:15  assefa
 * Change loadProjectionString to msLoadProjectionString
 *
 * Revision 1.17  2001/03/21 21:55:28  dan
 * Added get/setMetaData() for layerObj and mapObj()
 *
 * Revision 1.16  2001/03/21 17:43:33  dan
 * Added layer->class->type ... in sync with mapscript.i v1.29
 *
 * Revision 1.15  2001/03/16 22:08:36  dan
 * Removed allitems param to msLayerGetShape()
 *
 * Revision 1.14  2001/03/12 19:02:46  dan
 * Added query-related stuff in PHP MapScript
 *
 * Revision 1.13  2001/03/09 19:33:13  dan
 * Updated PHP MapScript... still a few methods missing, and needs testing.
 *
 * Revision 1.12  2001/02/23 21:58:00  dan
 * PHP MapScript working with new 3.5 stuff, but query stuff is disabled
 *
 * Revision 1.11  2000/11/01 16:31:07  dan
 * Add missing functions (in sync with mapscript).
 *
 * Revision 1.10  2000/09/25 22:48:54  dan
 * Aded update access for msOpenShapeFile()  (sync with mapscript.i v1.19)
 *
 * Revision 1.9  2000/09/22 13:56:10  dan
 * Added msInitShape() in rectObj_draw()
 *
 * Revision 1.8  2000/09/13 21:04:34  dan
 * Use msInitShape() in shapeObj constructor
 *
 * Revision 1.7  2000/09/07 20:18:20  dan
 * Sync with mapscript.i version 1.16
 *
 * Revision 1.6  2000/08/24 05:46:22  dan
 * #ifdef everything related to featureObj
 *
 * Revision 1.5  2000/08/16 21:14:00  dan
 * Sync with mapscript.i version 1.12
 *
 * Revision 1.4  2000/07/12 20:19:30  dan
 * Sync with mapscript.i version 1.10
 *
 * Revision 1.3  2000/06/28 20:22:02  dan
 * Sync with mapscript.i version 1.9
 *
 * Revision 1.2  2000/05/23 20:46:45  dan
 * Sync with mapscript.i version 1.5
 *
 * Revision 1.1  2000/05/09 21:06:11  dan
 * Initial Import
 *
 * Revision 1.6  2000/04/26 16:10:02  daniel
 * Changes to build with ms_3.3.010
 *
 * Revision 1.5  2000/03/17 19:03:07  daniel
 * Update to 3.3.008: removed prototypes for addFeature() and initFeature()
 *
 * Revision 1.4  2000/03/15 00:36:31  daniel
 * Finished conversion of all cover functions to real C
 *
 * Revision 1.3  2000/03/11 21:53:27  daniel
 * Ported extension to MapServer version 3.3.008
 *
 */


#include "php_mapscript.h"

/* grab mapserver declarations to wrap
 */
#include "../../map.h"
#include "../../maperror.h"
#include "../../mapprimitive.h"
#include "../../mapsymbol.h"
#include "../../mapshape.h"

/**********************************************************************
 * class extensions for mapObj
 **********************************************************************/
mapObj *mapObj_new(char *filename) {
    mapObj *map=NULL;

    if(filename && strlen(filename))
      return msLoadMap(filename);
    else { /* create an empty map, no layers etc... */
        map = (mapObj *)malloc(sizeof(mapObj));
        if(!map) {
            msSetError(MS_MEMERR, NULL, "msLoadMap()");
            return NULL;
        }
        initMap(map);
    }
    return map;  
}

void  mapObj_destroy(mapObj* self) {
    msFreeMap(self);
  }

layerObj *mapObj_getLayer(mapObj* self, int i) {
    if(i >= 0 && i < self->numlayers)	
      return &(self->layers[i]); /* returns an EXISTING layer */
    else
      return NULL;
  }

layerObj *mapObj_getLayerByName(mapObj* self, char *name) {
    int i;

    i = msGetLayerIndex(self, name);

    if(i != -1)
      return &(self->layers[i]); /* returns an EXISTING layer */
    else
      return NULL;
  }

int mapObj_addColor(mapObj* self, int r, int g, int b) {
    return msAddColor(self, r, g, b);
  }

int mapObj_getSymbolByName(mapObj* self, int type, char *name) {
    int symbol;

    if((symbol = msGetSymbolIndex(&self->symbolset, name)) == -1)
      if((symbol = msAddImageSymbol(&self->symbolset, name)) == -1)
	return -1;

    return symbol;
  }

void mapObj_prepareQuery(mapObj* self) {
    self->scale = msCalculateScale(self->extent, self->units, 
                                   self->width, self->height);
  }

gdImagePtr mapObj_prepareImage(mapObj* self) {
    gdImagePtr img;

    if(self->width == -1 && self->height == -1) {
      msSetError(MS_MISCERR, "Image dimensions not specified.", "prepareImage()");
      return NULL;
    }

    if(self->width == -1 ||  self->height == -1)
      if(msAdjustImage(self->extent, &self->width, &self->height) == -1)
        return NULL;

    img = gdImageCreate(self->width, self->height);
    if(!img) {
      msSetError(MS_GDERR, "Unable to initialize image.", "prepareImage()");
      return NULL;
    }
  
    if(msLoadPalette(img, &(self->palette), self->imagecolor) == -1)
      return NULL;
  
    self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);
    self->scale = msCalculateScale(self->extent, self->units, self->width, self->height);

    return img;
  }

gdImagePtr mapObj_draw(mapObj* self) {
    return msDrawMap(self);
  }

gdImagePtr mapObj_drawQueryMap(mapObj* self) {
    return msDrawQueryMap(self);
  }

gdImagePtr mapObj_drawLegend(mapObj* self) {
    return msDrawLegend(self);
  }

gdImagePtr mapObj_drawScalebar(mapObj* self) {
    return msDrawScalebar(self);
  }

gdImagePtr mapObj_drawReferenceMap(mapObj* self) {
    return msDrawReferenceMap(self);
  }

int mapObj_embedScalebar(mapObj* self, gdImagePtr img) {	
    return msEmbedScalebar(self, img);
  }

int mapObj_embedLegend(mapObj* self, gdImagePtr img) {	
    return msEmbedLegend(self, img);
  }

int mapObj_drawLabelCache(mapObj* self, gdImagePtr img) {
    return msDrawLabelCache(img, self);
  }

labelCacheMemberObj *mapObj_nextLabel(mapObj* self) {
    static int i=0;

    if(i<self->labelcache.numlabels)
      return &(self->labelcache.labels[i++]);
    else
      return NULL;	
  }

int mapObj_queryByPoint(mapObj* self, pointObj *point, 
                         int mode, double buffer) {
    return msQueryByPoint(self, -1, mode, *point, buffer);
  }

int mapObj_queryByRect(mapObj* self, rectObj rect) {
    return msQueryByRect(self, -1, rect);
  }

int mapObj_queryByFeatures(mapObj* self, int slayer) {
    return msQueryByFeatures(self, -1, slayer);
  }

int mapObj_queryByShape(mapObj *self, shapeObj *shape) {
    return msQueryByShape(self, -1, shape);
  }

int mapObj_setProjection(mapObj* self, char *string) {
    return(msLoadProjectionString(&(self->projection), string));
  }

int mapObj_save(mapObj* self, char *filename) {
    return msSaveMap(self, filename);
  }

char *mapObj_getMetaData(mapObj *self, char *name) {
    return(msLookupHashTable(self->web.metadata, name));
  }

int mapObj_setMetaData(mapObj *self, char *name, char *value) {
    if (!self->web.metadata)
        self->web.metadata = msCreateHashTable();
    if (msInsertHashTable(self->web.metadata, name, value) == NULL)
	return MS_FAILURE;
    return MS_SUCCESS;
  }


/**********************************************************************
 * class extensions for layerObj, always within the context of a map
 **********************************************************************/
layerObj *layerObj_new(mapObj *map) {
    if(map->numlayers == MS_MAXLAYERS) // no room
      return(NULL);

    if(initLayer(&(map->layers[map->numlayers])) == -1)
      return(NULL);

    map->layers[map->numlayers].index = map->numlayers;
    map->numlayers++;

    return &(map->layers[map->numlayers-1]);
  }

void layerObj_destroy(layerObj *self) {
    return; // map deconstructor takes care of it
  }

int layerObj_open(layerObj *self, char *path) {
    return msLayerOpen(self, path);
  }

void layerObj_close(layerObj *self) {
    msLayerClose(self);
  }

int layerObj_getShape(layerObj *self, char *path, shapeObj *shape, 
                      int tileindex, int shapeindex, int allitems) {
    return msLayerGetShape(self, path, shape, tileindex, shapeindex);
  }

resultCacheMemberObj *layerObj_getResult(layerObj *self, int i) {
    if(!self->resultcache) return NULL;

    if(i >= 0 && i < self->resultcache->numresults)
      return &self->resultcache->results[i]; 
    else
      return NULL;
  }

classObj *layerObj_getClass(layerObj *self, int i) { // returns an EXISTING class
    if(i >= 0 && i < self->numclasses)
      return &(self->class[i]); 
    else
      return(NULL);
  }

int layerObj_prepare(layerObj *self) {
    // do scaling
  }

int layerObj_draw(layerObj *self, mapObj *map, gdImagePtr img) {
    if(self->features) {
#ifdef __TODO35__
      return msDrawInlineLayer(map, self, img);
#else
      // ????
      return msDrawLayer(map, self, img);
#endif
    } else {
      if(self->type == MS_LAYER_RASTER) {
        return msDrawRasterLayer(map, self, img);
      } else {
	return msDrawLayer(map, self, img);
      }
    }
  }

int layerObj_queryByPoint(layerObj *self, mapObj *map, 
                          pointObj *point, int mode, double buffer) {
    return msQueryByPoint(map, self->index, mode, *point, buffer);
  }

int layerObj_queryByRect(layerObj *self, mapObj *map, rectObj rect) {
    return msQueryByRect(map, self->index, rect);
  }

int layerObj_queryByFeatures(layerObj *self, mapObj *map, int slayer) {
    return msQueryByFeatures(map, self->index, slayer);
  }

int layerObj_queryByShape(layerObj *self, mapObj *map, shapeObj *shape) {
    return msQueryByShape(map, self->index, shape);
  }

int layerObj_setFilter(layerObj *self, char *string) {    
    return loadExpressionString(&self->filter, string);
  }

int layerObj_setProjection(layerObj *self, char *string) {
    return(msLoadProjectionString(&(self->projection), string));
  }

int layerObj_addFeature(layerObj *self, shapeObj *shape) {
    if(insertFeatureList(&(self->features), shape) == NULL) 
      return -1;
    else
      return 0;
  }

char *layerObj_getMetaData(layerObj *self, char *name) {
    return(msLookupHashTable(self->metadata, name));
  }

int layerObj_setMetaData(layerObj *self, char *name, char *value) {
    if (!self->metadata)
        self->metadata = msCreateHashTable();
    if (msInsertHashTable(self->metadata, name, value) == NULL)
	return MS_FAILURE;
    return MS_SUCCESS;
  }

/**********************************************************************
 * class extensions for classObj, always within the context of a layer
 **********************************************************************/
classObj *classObj_new(layerObj *layer) {
    if(layer->numclasses == MS_MAXCLASSES) // no room
      return NULL;

    if(initClass(&(layer->class[layer->numclasses])) == -1)
      return NULL;
    layer->class[layer->numclasses].type = layer->type;

    layer->numclasses++;

    return &(layer->class[layer->numclasses-1]);
  }

void  classObj_destroy(classObj *self) {
    return; /* do nothing, map deconstrutor takes care of it all */
  }

int classObj_setExpression(classObj *self, char *string) {
    return loadExpressionString(&self->expression, string);
  }

int classObj_setText(classObj *self, layerObj *layer, char *string) {
    return loadExpressionString(&self->text, string);
  }

/**********************************************************************
 * class extensions for pointObj, useful many places
 **********************************************************************/
pointObj *pointObj_new() {
    return (pointObj *)malloc(sizeof(pointObj));
  }

void pointObj_destroy(pointObj *self) {
    free(self);
  }

int pointObj_draw(pointObj *self, mapObj *map, layerObj *layer, 
                  gdImagePtr img, int class_index, char *label_string) {
    return msDrawPoint(map, layer, self, img, class_index, label_string);
  }

double pointObj_distanceToPoint(pointObj *self, pointObj *point) {
    return msDistanceBetweenPoints(self, point);
  }

double pointObj_distanceToLine(pointObj *self, pointObj *a, pointObj *b) {
    return msDistanceFromPointToLine(self, a, b);
  }

double pointObj_distanceToShape(pointObj *self, shapeObj *shape) {
    switch(shape->type) {
    case(MS_SHAPE_POINT):
      return msDistanceFromPointToMultipoint(self, &(shape->line[0]));
    case(MS_SHAPE_LINE):
      return msDistanceFromPointToPolyline(self, shape);
    case(MS_SHAPE_POLYGON):
      return msDistanceFromPointToPolygon(self, shape);
    }

    return -1;
  }

/**********************************************************************
 * class extensions for lineObj (eg. a line or group of points), 
 * useful many places
 **********************************************************************/
lineObj *lineObj_new() {
    lineObj *line;

    line = (lineObj *)malloc(sizeof(lineObj));
    if(!line)
      return(NULL);

    line->numpoints=0;
    line->point=NULL;

    return line;
  }

void lineObj_destroy(lineObj *self) {
    free(self->point);
    free(self);		
  }

pointObj *lineObj_get(lineObj *self, int i) {
    if(i<0 || i>=self->numpoints)
      return NULL;
    else
      return &(self->point[i]);
  }

int lineObj_add(lineObj *self, pointObj *p) {
    if(self->numpoints == 0) { /* new */	
      self->point = (pointObj *)malloc(sizeof(pointObj));      
      if(!self->point)
	return -1;
    } else { /* extend array */
      self->point = (pointObj *)realloc(self->point, sizeof(pointObj)*(self->numpoints+1));
      if(!self->point)
	return -1;
    }

    self->point[self->numpoints].x = p->x;
    self->point[self->numpoints].y = p->y;
    self->numpoints++;

    return 0;
  }


/**********************************************************************
 * class extensions for shapeObj
 **********************************************************************/
shapeObj *shapeObj_new(int type) {
    shapeObj *shape;

    shape = (shapeObj *)malloc(sizeof(shapeObj));
    if(!shape)
      return NULL;

    msInitShape(shape);
    shape->type = type;
    return shape;
  }

void shapeObj_destroy(shapeObj *self) {
    msFreeShape(self);
    free(self);		
  }

lineObj *shapeObj_get(shapeObj *self, int i) {
    if(i<0 || i>=self->numlines)
      return NULL;
    else
      return &(self->line[i]);
  }

int shapeObj_add(shapeObj *self, lineObj *line) {
    return msAddLine(self, line);
  }

int shapeObj_draw(shapeObj *self, mapObj *map, layerObj *layer, 
                  gdImagePtr img) {
    return msDrawShape(map, layer, self, img, MS_TRUE);
  }

void shapeObj_setBounds(shapeObj *self) {
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

int shapeObj_copy(shapeObj *self, shapeObj *dest) {
    return(msCopyShape(self, dest));
  }

int shapeObj_contains(shapeObj *self, pointObj *point) {
    if(self->type == MS_SHAPE_POLYGON)
      return msIntersectPointPolygon(point, self);

    return -1;
  }

int shapeObj_intersects(shapeObj *self, shapeObj *shape) {
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
	return msIntersectPolylines(self, shape);
      }
      break;
    }

    return -1;
  }

/**********************************************************************
 * class extensions for rectObj
 **********************************************************************/
rectObj *rectObj_new() {	
    rectObj *rect;

    rect = (rectObj *)calloc(1, sizeof(rectObj));
    if(!rect)
      return(NULL);
    
    return(rect);    	
  }

void rectObj_destroy(rectObj *self) {
    free(self);
  }

double rectObj_fit(rectObj *self, int width, int height) {
    return  msAdjustExtent(self, width, height);
  } 

int rectObj_draw(rectObj *self, mapObj *map, layerObj *layer,
                 gdImagePtr img, int classindex, char *text) {
    shapeObj shape;

    msInitShape(&shape);
    msRectToPolygon(*self, &shape);
    shape.classindex = classindex;
    shape.text = strdup(text);

    msDrawShape(map, layer, &shape, img, MS_TRUE);

    msFreeShape(&shape);
    
    return 0;
  }

/**********************************************************************
 * class extensions for shapefileObj
 **********************************************************************/
shapefileObj *shapefileObj_new(char *filename, int type) {    
    shapefileObj *shapefile;
    int status;

    shapefile = (shapefileObj *)malloc(sizeof(shapefileObj));
    if(!shapefile)
      return NULL;

    if(type == -1)
      status = msSHPOpenFile(shapefile, "rb", NULL, filename);
    else if (type == -2)
      status = msSHPOpenFile(shapefile, "rb+", NULL, filename);
    else
      status = msSHPCreateFile(shapefile, filename, type);

    if(status == -1) {
      msSHPCloseFile(shapefile);
      free(shapefile);
      return NULL;
    }
 
    return(shapefile);
  }

void shapefileObj_destroy(shapefileObj *self) {
    msSHPCloseFile(self);
    free(self);  
  }

int shapefileObj_get(shapefileObj *self, int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);

    return 0;
  }

int shapefileObj_getTransformed(shapefileObj *self, mapObj *map, 
                                int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);
    msTransformShape(shape, map->extent, map->cellsize);

    return 0;
  }

void shapefileObj_getExtent(shapefileObj *self, int i, rectObj *rect) {
    msSHPReadBounds(self->hSHP, i, rect);
  }

int shapefileObj_add(shapefileObj *self, shapeObj *shape) {
    return msSHPWriteShape(self->hSHP, shape);	
  }	

/**********************************************************************
 * class extensions for labelCacheObj - TP mods
 **********************************************************************/
void labelCacheObj_freeCache(labelCacheObj *self) {
    int i;
    for (i = 0; i < self->numlabels; i++) {
        free(self->labels[i].string);
        msFreeShape(self->labels[i].poly);
    }   
    self->numlabels = 0;
    for (i = 0; i < self->nummarkers; i++) {
        msFreeShape(self->markers[i].poly);
    }
    self->nummarkers = 0;
  }

/**********************************************************************
 * class extensions for DBFInfo - TP mods
 **********************************************************************/
char *DBFInfo_getFieldName(DBFInfo *self, int iField) {
        static char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pszFieldName;
    }

int DBFInfo_getFieldWidth(DBFInfo *self, int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnWidth;
    }

int DBFInfo_getFieldDecimals(DBFInfo *self, int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnDecimals;
    }
    
DBFFieldType DBFInfo_getFieldType(DBFInfo *self, int iField) {
	return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
    }    


