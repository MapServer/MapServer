//
// mapscript.i: SWIG interface file for MapServer scripting extension called MapScript.
//

%module mapscript
%{
#include "../../map.h"
%}

%include typemaps.i
%include constraints.i

// grab mapserver declarations to wrap
%include "../../map.h"
%include "../../maperror.h"
%include "../../mapprimitive.h"
%include "../../mapshape.h"

%apply Pointer NONNULL { gdImagePtr img };

//
// class extensions for mapObj
//
%addmethods mapObj {
  mapObj(char *filename) {
    mapObj *map;

    if(filename)
      return msLoadMap(filename);
    else { /* create an empty map, no layers etc... */
      map = (mapObj *)malloc(sizeof(mapObj));
      if(!map) {
        msSetError(MS_MEMERR, NULL, "msLoadMap()");
        return NULL;
      }

      initMap(map);
      return map;
    }      
  }

  ~mapObj() {
    msFreeMap(self);
  }

  layerObj *getLayer(int i) {
    if(i >= 0 && i < self->numlayers)	
      return &(self->layers[i]); /* returns an EXISTING layer */
    else
      return NULL;
  }

  layerObj *getLayerByName(char *name) {
    int i;

    i = msGetLayerIndex(self, name);

    if(i != -1)
      return &(self->layers[i]); /* returns an EXISTING layer */
    else
      return NULL;
  }

  int addColor(int r, int g, int b) {
    return msAddColor(self, r, g, b);
  }

  int getSymbolByName(int type, char *name) {
    switch (type) {
    case(MS_MARKERSET):
      return msGetSymbolIndex(&self->markerset, name);
    case(MS_LINESET):
      return msGetSymbolIndex(&self->lineset, name);
    case(MS_SHADESET):
      return msGetSymbolIndex(&self->shadeset, name);
    default:
      return -1;
    }
  }

  void prepareQuery() {
    self->scale = msCalculateScale(self->extent, self->units, self->width, self->height);
    msApplyScale(self);
  }

  gdImagePtr prepareImage() {
    gdImagePtr img;

    if(self->width == -1 ||  self->height == -1)
      if(msAdjustImage(self->extent, &self->width, &self->height) == -1)
        return NULL;

    img = gdImageCreate(self->width, self->height);
    if(!img)
      return NULL;
  
    if(msLoadPalette(img, &(self->palette), self->imagecolor) == -1)
      return NULL;
  
    self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);
    self->scale = msCalculateScale(self->extent, self->units, self->width, self->height);

    msApplyScale(self);

    return img;
  }

  gdImagePtr draw() {
    return msDrawMap(self);
  }

  gdImagePtr drawQueryMap(queryResultObj *results) {
    return msDrawQueryMap(self, results);
  }

  gdImagePtr drawLegend() {
    return msDrawLegend(self);
  }

  gdImagePtr drawScalebar() {
    return msDrawScalebar(self);
  }

  gdImagePtr drawReferenceMap() {
    return msDrawReferenceMap(self);
  }

  int embedScalebar(gdImagePtr img) {	
    return msEmbedScalebar(self, img);
  }

  int embedLegend(gdImagePtr img) {	
    return msEmbedLegend(self, img);
  }

  int drawLabelCache(gdImagePtr img) {
    return msDrawLabelCache(img, self);
  }

  labelCacheMemberObj *nextLabel() {
    static int i=0;
    if(i<self->labelcache.numlabels)
      return &(self->labelcache.labels[i++]);
    else
      return NULL;	
  }

  queryResultObj *queryUsingPoint(pointObj *point, int mode, double buffer) {
    return msQueryUsingPoint(self, NULL, mode, *point, buffer);
  }

  queryResultObj *queryUsingRect(rectObj *rect) {
    return msQueryUsingRect(self, NULL, rect);
  }

  int queryUsingFeatures(queryResultObj *results) {
    return msQueryUsingFeatures(self, NULL, results);
  }

  queryResultObj *queryUsingShape(mapObj *map, shapeObj *shape) {
    return msQueryUsingShape(map, NULL, shape);
  }

  int setProjection(char *string) {
    return(loadProjectionString(&(self->projection), string));
  }
}

//
// class extentions for queryResultObj
//
%addmethods queryResultObj {
  queryResultObj(char *filename) {
    return msLoadQuery(filename);
  }

  ~queryResultObj() {
    if(self) msFreeQueryResults(self);
  }

  void free() {	
    if(self) msFreeQueryResults(self);
  }

  int save(char *filename) {
    return msSaveQuery(self, filename);
  }

  shapeResultObj next() {    
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

    result.layer = -1; /* no more results */
    result.shape = -1;
    result.query = -1;

    return result;
  }
}

//
// class extensions for layerObj, always within the context of a map
//
%addmethods layerObj {
  layerObj(mapObj *map) {
    if(map->numlayers == MS_MAXLAYERS) /* no room */
      return(NULL);

    if(initLayer(&(map->layers[map->numlayers])) == -1)
      return(NULL);

    map->layers[map->numlayers].index = map->numlayers;
    map->numlayers++;

    return &(map->layers[map->numlayers-1]);
  }

  ~layerObj() {
    return; /* do nothing, map deconstrutor takes care of it all */
  }

  classObj *getClass(int i) { /* returns an EXISTING class */
    if(i >= 0 && i < self->numclasses)
      return &(self->class[i]); 
    else
      return(NULL);
  }

  int draw(mapObj *map, gdImagePtr img) {
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

  queryResultObj *queryUsingPoint(mapObj *map, pointObj *point, int mode, double buffer) {
    return msQueryUsingPoint(map, self->name, mode, *point, buffer);
  }

  queryResultObj *queryUsingRect(mapObj *map, rectObj *rect) {
    return msQueryUsingRect(map, self->name, rect);
  }

  int queryUsingFeatures(mapObj *map, queryResultObj *results) {
    return msQueryUsingFeatures(map, self->name, results);
  }

  queryResultObj *queryUsingShape(mapObj *map, shapeObj *shape) {
    return msQueryUsingShape(map, self->name, shape);
  }

  int setProjection(char *string) {
    return(loadProjectionString(&(self->projection), string));
  }
}

//
// class extensions for classObj, always within the context of a layer
//
%addmethods classObj {
  classObj(layerObj *layer) {
    if(layer->numclasses == MS_MAXCLASSES) /* no room */
      return NULL;

    if(initClass(&(layer->class[layer->numclasses])) == -1)
      return NULL;

    layer->numclasses++;

    return &(layer->class[layer->numclasses-1]);
  }

  ~classObj() {
    return; /* do nothing, map deconstrutor takes care of it all */
  }

  int setExpression(char *string) {    
    return loadExpressionString(&self->expression, string);
  }

  int setText(layerObj *layer, char *string) {
    int status;

    status = loadExpressionString(&self->text, string);
    if(status != -1)	
      layer->annotate = MS_TRUE;

    return status;
  }
}

//
// class extensions for queryObj, always within the context of a layer
//
%addmethods queryObj {
  queryObj(layerObj *layer) {
    if(layer->numqueries == MS_MAXQUERIES) /* no room */
      return NULL;

    if(initQuery(&(layer->query[layer->numqueries])) == -1)
      return NULL;

    layer->numqueries++;

    return &(layer->query[layer->numqueries-1]);
  }

  ~queryObj() {
    return; /* do nothing, map deconstrutor takes care of it all */
  }

  int setExpression(char *string) {    
    return loadExpressionString(&self->expression, string);
  }
}

//
// class extensions for featureObj, always within the context of a layer
//
%addmethods featureObj {
  featureObj(layerObj *layer) {
    if(!layer->features) { /* new feature list */
      layer->features = initFeature();
      return layer->features;
    } else {
      return addFeature(layer->features);
    }
  }

  ~featureObj() {
    return; /* do nothing, map deconstrutor takes care of it all */
  }

  int add(lineObj *p) {
    return msAddLine(&self->shape, p);
  }
}

//
// class extensions for pointObj, useful many places
//
%addmethods pointObj {
  pointObj() {
    return (pointObj *)malloc(sizeof(pointObj));
  }

  ~pointObj() {
    free(self);
  }

  int draw(mapObj *map, layerObj *layer, gdImagePtr img, char *class_string, char *label_string) {
    return msDrawPoint(map, layer, self, img, class_string, label_string);
  }

  double distanceToPoint(pointObj *point) {
    return msDistanceBetweenPoints(self, point);
  }

  double distanceToLine(pointObj *a, pointObj *b) {
    return msDistanceFromPointToLine(self, a, b);
  }

  double distanceToShape(shapeObj *shape) {
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
}

//
// class extensions for lineObj (eg. a line or group of points), useful many places
//
%addmethods lineObj {
  lineObj() {
    lineObj *line;

    line = (lineObj *)malloc(sizeof(lineObj));
    if(!line)
      return(NULL);

    line->numpoints=0;
    line->point=NULL;

    return line;
  }

  ~lineObj() {
    free(self->point);
    free(self);		
  }

  pointObj *get(int i) {
    if(i<0 || i>=self->numpoints)
      return NULL;
    else
      return &(self->point[i]);
  }

  int add(pointObj *p) {
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
}

//
// class extensions for shapeObj
//
%addmethods shapeObj {
  shapeObj(int type) {
    shapeObj *shape;

    shape = (shapeObj *)malloc(sizeof(shapeObj));
    if(!shape)
      return NULL;

    shape->type=type;
    shape->numlines=0;
    shape->line=NULL;
    shape->bounds.minx = shape->bounds.miny = shape->bounds.maxx = shape->bounds.maxy = -1;

    return shape;
  }

  ~shapeObj() {
    msFreeShape(self);
    free(self);		
  }

  lineObj *get(int i) {
    if(i<0 || i>=self->numlines)
      return NULL;
    else
      return &(self->line[i]);
  }

  int add(lineObj *line) {
    return msAddLine(self, line);
  }

  int draw(mapObj *map, layerObj *layer, gdImagePtr img, char *class_string, char *label_string) {
    return msDrawShape(map, layer, self, img, class_string, label_string);
  }

  void setBounds() {
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

  int contains(pointObj *point) {
    if(self->type == MS_POLYGON)
      return msIntersectPointPolygon(point, self);

    return -1;
  }

  int intersects(shapeObj *shape) {
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
}

//
// class extensions for rectObj
//
%addmethods rectObj {
  rectObj() {	
    rectObj *rect;

    rect = (rectObj *)calloc(1, sizeof(rectObj));
    if(!rect)
      return(NULL);
    
    return(rect);    	
  }

  ~rectObj() {
    free(self);
  }

  double fit(int width, int height) {
    return  msAdjustExtent(self, width, height);
  } 

  int draw(mapObj *map, layerObj *layer, gdImagePtr img, char *class_string, char *label_string) {
    shapeObj shape;

    msRect2Polygon(*self, &shape);
    msDrawShape(map, layer, &shape, img, class_string, label_string);
    msFreeShape(&shape);
    
    return 0;
  }
}

//
// class extensions for shapefileObj
//
%addmethods shapefileObj {
  shapefileObj(char *filename, int type) {    
    shapefileObj *shapefile;
    int status;

    shapefile = (shapefileObj *)malloc(sizeof(shapefileObj));
    if(!shapefile)
      return(NULL);

    if(type == -1)
      status = msOpenSHPFile(shapefile, NULL, NULL, filename);
    else
      status = msCreateSHPFile(shapefile, filename, type);
    if(status == -1) {
      msCloseSHPFile(shapefile);
      free(shapefile);
      return(NULL);
    }
 
    return(shapefile);
  }

  ~shapefileObj() {
    msCloseSHPFile(self);
    free(self);  
  }

  int get(int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    SHPReadShape(self->hSHP, i, shape);

    return 0;
  }

  void getExtent(int i, rectObj *rect) {
    SHPReadBounds(self->hSHP, i, rect);
  }

  int add(shapeObj *shape) {
    return SHPWriteShape(self->hSHP, shape);	
  }	
}

