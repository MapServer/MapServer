//
// mapscript.i: SWIG interface file for MapServer scripting extension called MapScript.
//

// language specific initialization
#ifdef SWIGTCL8
%module Mapscript
%init %{
#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
#endif
%}
#endif

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
%apply Pointer NONNULL { mapObj *map };
%apply Pointer NONNULL { layerObj *layer };

#ifdef SWIGPYTHON
// For Python, errors reported via the ms_error structure are translated
// into RuntimeError exceptions. (Chris Chamberlin <cdc@aracnet.com>)
%{
  static void _raise_ms_exception(void) {
    char errbuf[256];
    snprintf(errbuf, 255, "%s: %s %s\n", ms_error.routine, msGetErrorString(ms_error.code), ms_error.message);
    _SWIG_exception(SWIG_RuntimeError, errbuf);
  }
  
  #define raise_ms_exception() { _raise_ms_exception(); return NULL; }
%}

%except {
  $function
    if ( (ms_error.code != MS_NOERR) && (ms_error.code != -1) )
      raise_ms_exception();
}
#endif // SWIGPYTHON

//
// class extensions for mapObj
//
%addmethods mapObj {
  mapObj(char *filename) {
    mapObj *map;

    if(filename && strlen(filename))
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
    int symbol;

    if((symbol = msGetSymbolIndex(&self->symbolset, name)) == -1)
      if((symbol = msAddImageSymbol(&self->symbolset, name)) == -1)
	return -1;

    return symbol;
  }

  void prepareQuery() {
    self->scale = msCalculateScale(self->extent, self->units, self->width, self->height);
  }

  gdImagePtr prepareImage() {
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

  gdImagePtr draw() {
    return msDrawMap(self);
  }

  gdImagePtr drawQueryMap() {
    return msDrawQueryMap(self);
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

  int queryByPoint(pointObj *point, int mode, double buffer) {
    return msQueryByPoint(self, -1, mode, *point, buffer);
  }

  int queryByRect(rectObj rect) {
    return msQueryByRect(self, -1, rect);
  }

  int queryByFeatures(int slayer) {
    return msQueryByFeatures(self, -1, slayer);
  }

  int queryByShape(shapeObj *shape) {
    return msQueryByShape(self, -1, shape);
  }

  int setProjection(char *string) {
    return(loadProjectionString(&(self->projection), string));
  }

  int save(char *filename) {
    return msSaveMap(self, filename);
  }
}

//
// class extensions for layerObj, always within the context of a map
//
%addmethods layerObj {
  layerObj(mapObj *map) {
    if(map->numlayers == MS_MAXLAYERS) // no room
      return(NULL);

    if(initLayer(&(map->layers[map->numlayers])) == -1)
      return(NULL);

    map->layers[map->numlayers].index = map->numlayers;
    map->numlayers++;

    return &(map->layers[map->numlayers-1]);
  }

  ~layerObj() {
    return; // map deconstructor takes care of it
  }

  int open(char *path) {
    return msLayerOpen(self, path);
  }

  void close() {
    msLayerClose(self);
  }

  int getShape(char *path, shapeObj *shape, int tileindex, int shapeindex, int allitems) {
    return msLayerGetShape(self, path, shape, tileindex, shapeindex, allitems);
  }

  resultCacheMemberObj *getResult(int i) {
    if(!self->resultcache) return NULL;

    if(i >= 0 && i < self->resultcache->numresults)
      return &self->resultcache->results[i]; 
    else
      return NULL;
  }

  classObj *getClass(int i) { // returns an EXISTING class
    if(i >= 0 && i < self->numclasses)
      return &(self->class[i]); 
    else
      return NULL;
  }

  int prepare() {
    // do scaling
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

  int queryByPoint(mapObj *map, pointObj *point, int mode, double buffer) {
    return msQueryByPoint(map, self->index, mode, *point, buffer);
  }

  int queryByRect(mapObj *map, rectObj rect) {
    return msQueryByRect(map, self->index, rect);
  }

  int queryByFeatures(mapObj *map, int slayer) {
    return msQueryByFeatures(map, self->index, slayer);
  }

  int queryByShape(mapObj *map, shapeObj *shape) {
    return msQueryByShape(map, self->index, shape);
  }

  int setFilter(char *string) {    
    return loadExpressionString(&self->filter, string);
  }

  int setProjection(char *string) {
    return(loadProjectionString(&(self->projection), string));
  }

  int addFeature(shapeObj *shape) {
    if(insertFeatureList(&(self->features), shape) == NULL) 
      return -1;
    else
      return 0;
  }
}

//
// class extensions for classObj, always within the context of a layer
//
%addmethods classObj {
  classObj(layerObj *layer) {
    if(layer->numclasses == MS_MAXCLASSES) // no room
      return NULL;

    if(initClass(&(layer->class[layer->numclasses])) == -1)
      return NULL;
    layer->class[layer->numclasses].type = layer->type;

    layer->numclasses++;

    return &(layer->class[layer->numclasses-1]);
  }

  ~classObj() {
    return; // do nothing, map deconstrutor takes care of it all
  }

  int setExpression(char *string) {    
    return loadExpressionString(&self->expression, string);
  }

  int setText(layerObj *layer, char *string) {
    return loadExpressionString(&self->text, string);
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

  int draw(mapObj *map, layerObj *layer, gdImagePtr img, int classindex, char *text) {
    return msDrawPoint(map, layer, self, img, classindex, text);
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

    msInitShape(shape);

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

  int draw(mapObj *map, layerObj *layer, gdImagePtr img) {
    return msDrawShape(map, layer, self, img, MS_TRUE);
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

  int copy(shapeObj *dest) {
    return(msCopyShape(self, dest));
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

  int draw(mapObj *map, layerObj *layer, gdImagePtr img, int classindex, char *text) {
    shapeObj shape;

    msInitShape(&shape);
    msRectToPolygon(*self, &shape);
    shape.classindex = classindex;
    shape.text = strdup(text);

    msDrawShape(map, layer, &shape, img, MS_TRUE);

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
      return NULL;

    if(type == -1)
      status = msSHPOpenFile(shapefile, "rb", NULL, filename);
    else if(type == -2)
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

  ~shapefileObj() {
    msSHPCloseFile(self);
    free(self);  
  }

  int get(int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);

    return 0;
  }

  int getTransformed(mapObj *map, int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);
    msTransformShape(shape, map->extent, map->cellsize);

    return 0;
  }

  void getExtent(int i, rectObj *rect) {
    msSHPReadBounds(self->hSHP, i, rect);
  }

  int add(shapeObj *shape) {
    return msSHPWriteShape(self->hSHP, shape);	
  }	
}

//
// class extensions for labelCacheObj - TP mods
//
%addmethods labelCacheObj {
  void freeCache() {
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
}

//
// class extensions for DBFInfo - TP mods
//
%addmethods DBFInfo {
    char *getFieldName(int iField) {
        static char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pszFieldName;
    }

    int getFieldWidth(int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnWidth;
    }

    int getFieldDecimals(int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnDecimals;
    }
    
    DBFFieldType getFieldType(int iField) {
	return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
    }    
}
