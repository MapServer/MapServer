/*
 * mapscript_i.c: interface file for MapServer PHP scripting extension 
 *                called MapScript.
 *
 * This file was originally based on the SWIG interface file mapscript.i
 *
 * $Id$
 *
 * $Log$
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

gdImagePtr mapObj_drawQueryMap(mapObj* self, queryResultObj *results) {
    return msDrawQueryMap(self, results);
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

queryResultObj *mapObj_queryUsingPoint(mapObj* self, pointObj *point, 
                                       int mode, double buffer) {
    return msQueryUsingPoint(self, NULL, mode, *point, buffer);
  }

queryResultObj *mapObj_queryUsingRect(mapObj* self, rectObj *rect) {
    return msQueryUsingRect(self, NULL, rect);
  }

int mapObj_queryUsingFeatures(mapObj* self, queryResultObj *results) {
    return msQueryUsingFeatures(self, NULL, results);
  }

int mapObj_setProjection(mapObj* self, char *string) {
    return(loadProjectionString(&(self->projection), string));
  }

int mapObj_save(mapObj* self, char *filename) {
    return msSaveMap(self, filename);
  }

/**********************************************************************
 * class extensions for queryResultObj
 **********************************************************************/
queryResultObj *queryResultObj_new(char *filename) {
    return msLoadQuery(filename);
  }

void queryResultObj_destroy(queryResultObj *self) {
    if(self) msFreeQueryResults(self);
  }

void queryResultObj_free(queryResultObj *self) {	
    if(self) msFreeQueryResults(self);
  }

int queryResultObj_save(queryResultObj *self, char *filename) {
    return msSaveQuery(self, filename);
  }

shapeResultObj queryResultObj_next(queryResultObj *self) {    
    int i, j;
    shapeResultObj result;	

    result.tile = -1;

    for(i=self->currentlayer; i<self->numlayers; i++) {
      for(j=self->currentshape; j<self->layers[i].numshapes; j++) {
	if(msGetBit(self->layers[i].status,j)) {

	  self->currentlayer = i;     // defines where to start searching on
	  self->currentshape = j + 1; // next call to next() for this result set

	  result.layer = i;
	  result.shape = j;
	  result.query = self->layers[i].index[j];

          return result;
        }
      }
    }

    result.layer = -1; // no more results
    result.shape = -1;
    result.query = -1;

    return result;
  }

void queryResultObj_rewind(queryResultObj *self) {
    if (self) {
        self->currentlayer = 0;
        self->currenttile = 0;
        self->currentshape = 0;
    }
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

classObj *layerObj_getClass(layerObj *self, int i) { // returns an EXISTING class
    if(i >= 0 && i < self->numclasses)
      return &(self->class[i]); 
    else
      return(NULL);
  }

int layerObj_draw(layerObj *self, mapObj *map, gdImagePtr img) {
    if(self->features) {
      return msDrawInlineLayer(map, self, img);
    } else {
      if(self->type == MS_RASTER) {
        return msDrawRasterLayer(map, self, img);
      } else {
	return msDrawShapefileLayer(map, self, img, NULL);
      }
    }
  }

queryResultObj *layerObj_queryUsingPoint(layerObj *self, mapObj *map, 
                                         pointObj *point, int mode, 
                                         double buffer) {
    return msQueryUsingPoint(map, self->name, mode, *point, buffer);
  }

queryResultObj *layerObj_queryUsingRect(layerObj *self, mapObj *map, 
                                        rectObj *rect) {
    return msQueryUsingRect(map, self->name, rect);
  }

int layerObj_queryUsingFeatures(layerObj *self, mapObj *map, 
                                queryResultObj *results) {
    return msQueryUsingFeatures(map, self->name, results);
  }

int layerObj_setProjection(layerObj *self, char *string) {
    return(loadProjectionString(&(self->projection), string));
  }

int layerObj_addFeature(layerObj *self, shapeObj *shape) {
    if(insertFeatureList(&(self->features), *shape) == NULL) 
      return -1;
    else
      return 0;
  }

int layerObj_classify(layerObj *self, char *string) {
    return msGetClassIndex(self, string);
  }

/**********************************************************************
 * class extensions for classObj, always within the context of a layer
 **********************************************************************/
classObj *classObj_new(layerObj *layer) {
    if(layer->numclasses == MS_MAXCLASSES) // no room
      return NULL;

    if(initClass(&(layer->class[layer->numclasses])) == -1)
      return NULL;

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
 * class extensions for queryObj, always within the context of a layer
 **********************************************************************/
queryObj *queryObj_new(layerObj *layer) {
    if(layer->numqueries == MS_MAXQUERIES) /* no room */
      return NULL;

    if(initQuery(&(layer->query[layer->numqueries])) == -1)
      return NULL;

    layer->numqueries++;

    return &(layer->query[layer->numqueries-1]);
  }

void queryObj_destroy(queryObj *self) {
    return; // do nothing, map deconstrutor takes care of it all
  }

int queryObj_setExpression(queryObj *self, char *string) {    
    return loadExpressionString(&self->expression, string);
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
                  gdImagePtr img, char *class_string, char *label_string) {
    return msDrawPoint(map, layer, self, img, class_string, label_string);
  }

double pointObj_distanceToPoint(pointObj *self, pointObj *point) {
    return msDistanceBetweenPoints(self, point);
  }

double pointObj_distanceToLine(pointObj *self, pointObj *a, pointObj *b) {
    return msDistanceFromPointToLine(self, a, b);
  }

double pointObj_distanceToShape(pointObj *self, shapeObj *shape) {
    switch(shape->type) {
    case(MS_POINT):
      return msDistanceFromPointToMultipoint(self, &(shape->line[0]));
    case(MS_LINE):
      return msDistanceFromPointToPolyline(self, shape);
    case(MS_POLYGON):
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
                  gdImagePtr img, char *class_string, char *label_string) {
    return msDrawShape(map, layer, self, img, class_string, label_string);
  }

int shapeObj_contains(shapeObj *self, pointObj *point) {
    if(self->type == MS_POLYGON)
      return msIntersectPointPolygon(point, self);

    return -1;
  }

int shapeObj_intersects(shapeObj *self, shapeObj *shape) {
    switch(self->type) {
    case(MS_LINE):
      switch(shape->type) {
      case(MS_LINE):
	return msIntersectPolylines(self, shape);
      case(MS_POLYGON):
	return msIntersectPolylinePolygon(self, shape);
      }
      break;
    case(MS_POLYGON):
      switch(shape->type) {
      case(MS_LINE):
	return msIntersectPolylinePolygon(shape, self);
      case(MS_POLYGON):
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
                 gdImagePtr img, char *class_string, char *label_string) {
    shapeObj shape;

    msInitShape(&shape);
    msRect2Polygon(*self, &shape);
    msDrawShape(map, layer, &shape, img, class_string, label_string);
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
      status = msOpenSHPFile(shapefile, "rb", NULL, NULL, filename);
    else if (type == -2)
      status = msOpenSHPFile(shapefile, "rb+", NULL, NULL, filename);
    else
      status = msCreateSHPFile(shapefile, filename, type);

    if(status == -1) {
      msCloseSHPFile(shapefile);
      free(shapefile);
      return NULL;
    }
 
    return(shapefile);
  }

void shapefileObj_destroy(shapefileObj *self) {
    msCloseSHPFile(self);
    free(self);  
  }

int shapefileObj_get(shapefileObj *self, int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    SHPReadShape(self->hSHP, i, shape);

    return 0;
  }

int shapefileObj_getTransformed(shapefileObj *self, mapObj *map, 
                                int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    SHPReadShape(self->hSHP, i, shape);
    msTransformPolygon(map->extent, map->cellsize, shape);

    return 0;
  }

void shapefileObj_getExtent(shapefileObj *self, int i, rectObj *rect) {
    SHPReadBounds(self->hSHP, i, rect);
  }

int shapefileObj_add(shapefileObj *self, shapeObj *shape) {
    return SHPWriteShape(self->hSHP, shape);	
  }	

