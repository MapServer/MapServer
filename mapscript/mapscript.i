// $Id$
//
// mapscript.i: SWIG interface file for MapServer scripting extension called MapScript.
//

// language specific initialization
#ifdef SWIGTCL8
%module Mapscript
%{

/* static global copy of Tcl interp */
static Tcl_Interp *SWIG_TCL_INTERP;

%}

%init %{
#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
  /* save Tcl interp pointer to be used in getImageToVar() */
  SWIG_TCL_INTERP = interp;
#endif
%}
#endif

%module mapscript
%{
#include "../../map.h"
#include "../../maptemplate.h"
%}

%include typemaps.i
%include constraints.i

/* Typemaps to support NULL in attribute accessor functions
 * provided to Steve Lime by David Beazley and tested for Python
 * only by Sean Gillies.  If these typemaps are going to be useful
 * for other scripting interfaces and can be tested, I welcome
 * the removal of the SWIGPYTHON ifdef */

#ifdef SWIGPYTHON
#ifdef __cplusplus
%typemap(memberin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
#else
%typemap(memberin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
#endif
#endif // SWIGPYTHON

//%rename (_class) class;

// grab mapserver declarations to wrap
%include "../../mapprimitive.h"
%include "../../mapshape.h"
%include "../../mapproject.h"
%include "../../map.h"
// %include "../../maperror.h"

%apply Pointer NONNULL { mapObj *map };
%apply Pointer NONNULL { layerObj *layer };

#ifdef SWIGPYTHON
// For Python, errors reported via the ms_error structure are translated
// into RuntimeError exceptions. (Chris Chamberlin <cdc@aracnet.com>)
%{
  static void _raise_ms_exception(void) {
    char errbuf[256];
    errorObj *ms_error = msGetErrorObj();
    snprintf(errbuf, 255, "%s: %s %s\n", ms_error->routine, msGetErrorString(ms_error->code), ms_error->message);
    _SWIG_exception(SWIG_RuntimeError, errbuf);
  }
  
  #define raise_ms_exception() { _raise_ms_exception(); return NULL; }
%}

%except {
  $function
    {
      errorObj *ms_error = msGetErrorObj();
      if ( (ms_error->code != MS_NOERR) && (ms_error->code != -1) )
        raise_ms_exception();
    }
}
#endif // SWIGPYTHON

//
// class extensions for mapObj, need to figure out how to deal with Dan's extension to msLoadMap()...
//
%extend mapObj {
  mapObj(char *filename) {
    if(filename && strlen(filename))
      return msLoadMap(filename, NULL);
    else { /* create an empty map, no layers etc... */
      return msNewMapObj();
    }      
  }

  ~mapObj() {
    msFreeMap(self);
  }

  mapObj *clone() {
    mapObj *dstMap;
    dstMap = msNewMapObj();
    if (msCopyMap(dstMap, self) != MS_SUCCESS)
    {
        msFreeMap(dstMap);
        dstMap = NULL;
    }
    return dstMap;
  }

  /* removeLayer() adjusts the layers array, the indices of
   * the remaining layers, the layersdrawing order, and numlayers
   */
  int removeLayer(int index) {
    int i, drawindex = MS_MAXLAYERS + 1;
    if ((index < 0) || (index >= self->numlayers)) {
      return MS_FAILURE;
    }
    for (i = index + 1; i < self->numlayers; i++) {
      self->layers[i].index--;
      self->layers[i-1] = self->layers[i];
    }
    for (i = 0; i < self->numlayers; i++) {
      if (self->layerorder[i] == index) {
        drawindex = i;
        break;
      }
      if (i > drawindex) {
        self->layerorder[i-1] = self->layerorder[i];
      }
    }
    self->numlayers--;
    self->layerorder[self->numlayers] = 0;
    return MS_SUCCESS;
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

  /*
  int addColor(int r, int g, int b) {
    return msAddColor(self, r, g, b);
  }
  */

  int getSymbolByName(char *name) {
    return msGetSymbolIndex(&self->symbolset, name);
  }

  void prepareQuery() {
    int status;

    status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scale);
    if(status != MS_SUCCESS) self->scale = -1; // degenerate extents ok here
  }

  imageObj *prepareImage() {
    int i, status;
    imageObj *image=NULL;

    if(self->width == -1 || self->height == -1) {
      msSetError(MS_MISCERR, "Image dimensions not specified.", "prepareImage()");
      return NULL;
    }

    msInitLabelCache(&(self->labelcache)); // this clears any previously allocated cache

    image = msImageCreate(self->width, self->height, self->outputformat,
                          self->web.imagepath, self->web.imageurl);
    if(!image) {
      msSetError(MS_GDERR, "Unable to initialize image.", "prepareImage()");
      return NULL;
    }

    /*if (MS_DRIVER_GD(self->outputformat))
    {    
        if(msLoadPalette(image->img.gd, &(self->palette), self->imagecolor) == -1)
          return NULL;
    }*/

    self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);
    status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scale);
    if(status != MS_SUCCESS) return NULL;

    // compute layer scale factors now
    for(i=0;i<self->numlayers; i++) {
      if(self->layers[i].symbolscale > 0 && self->scale > 0) {
    	if(self->layers[i].sizeunits != MS_PIXELS)
      	  self->layers[i].scalefactor = (inchesPerUnit[self->layers[i].sizeunits]/inchesPerUnit[self->units]) / self->cellsize; 
    	else
      	  self->layers[i].scalefactor = self->layers[i].symbolscale/self->scale;
      }
    }

    return image;
  }

  void setImageType( char * imagetype ) {
      outputFormatObj *format;

      format = msSelectOutputFormat( self, imagetype );
      if( format == NULL )
	  msSetError(MS_MISCERR, "Unable to find IMAGETYPE '%s'.", 
		     "setImageType()", imagetype );
      else
      {  
          msFree( self->imagetype );
          self->imagetype = strdup(imagetype);
          msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE, 
                               MS_NOOVERRIDE, MS_NOOVERRIDE );
      }
  }

  void setOutputFormat( outputFormatObj *format ) {
      msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE, 
                           MS_NOOVERRIDE, MS_NOOVERRIDE );
  }

  imageObj *draw() {
    return msDrawMap(self);
  }

  imageObj *drawQuery() {
    return msDrawQueryMap(self);
  }

  imageObj *drawLegend() {
    return msDrawLegend(self);
  }

  imageObj *drawScalebar() {
    return msDrawScalebar(self);
  }

  imageObj *drawReferenceMap() {
    return msDrawReferenceMap(self);
  }

  int embedScalebar(imageObj *image) {	
    return msEmbedScalebar(self, image->img.gd);
  }

  int embedLegend(imageObj *image) {	
    return msEmbedLegend(self, image->img.gd);
  }

  int drawLabelCache(imageObj *image) {
    return msDrawLabelCache(image, self);
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

  int setWKTProjection(char *string) {
    return msOGCWKT2ProjectionObj(string, &(self->projection), self->debug);
  }

  %new char *getProjection() {
    return msGetProjectionString(&(self->projection));
  }

  int setProjection(char *string) {
    return msLoadProjectionString(&(self->projection), string);
  }

  int save(char *filename) {
    return msSaveMap(self, filename);
  }

  int saveQuery(char *filename) {
    return msSaveQuery(self, filename);
  }

  int saveQueryAsGML(char *filename) {
    return msGMLWriteQuery(self, filename);
  }

  char *getMetaData(char *name) {
    return(msLookupHashTable(self->web.metadata, name));
  }

  int setMetaData(char *name, char *value) {
    if (!self->web.metadata)
        self->web.metadata = msCreateHashTable();
    if (msInsertHashTable(self->web.metadata, name, value) == NULL)
	return MS_FAILURE;
    return MS_SUCCESS;
  }
  
  int setSymbolSet(char *szFileName) {
    msFreeSymbolSet(&self->symbolset);
    msInitSymbolSet(&self->symbolset);
   
    // Set symbolset filename
    self->symbolset.filename = strdup(szFileName);

    // Symbolset shares same fontset as main mapfile
    self->symbolset.fontset = &(self->fontset);

    return msLoadSymbolSet(&self->symbolset, self);
  }

  int getNumSymbols() {
    return self->symbolset.numsymbols;
  }

  int setFontSet(char *szFileName) {
    msFreeFontSet(&(self->fontset));
    msInitFontSet(&(self->fontset));
   
    // Set fontset filename
    self->fontset.filename = strdup(szFileName);

    return msLoadFontSet(&(self->fontset), self);
  }

  int saveMapContext(char *szFileName) {
    return msSaveMapContext(self, szFileName);
  }

  int loadMapContext(char *szFileName) {
    return msLoadMapContext(self, szFileName);
  }

  int  moveLayerUp(int layerindex) {
    return msMoveLayerUp(self, layerindex);
  }

  int  moveLayerDown(int layerindex) {
    return msMoveLayerDown(self, layerindex);
  }

  int *getLayersDrawingOrder() {
    return  self->layerorder;
  }

#ifdef SWIGPYTHON 
  /* getLayerOrder() extension returns the map layerorder as a native
   * sequence
   */
   PyObject *getLayerOrder() {
     int i;
     PyObject *order;
     order = PyTuple_New(self->numlayers);
     for (i = 0; i < self->numlayers; i++) {
       PyTuple_SetItem(order, i, PyInt_FromLong((long)self->layerorder[i]));
     }
     return order;
   } 

  /* setLayerOrder() extension */
  int setLayerOrder(PyObject *order) {
    int i, size;
    size = PyTuple_Size(order);
    for (i = 0; i < size; i++) {
      self->layerorder[i] = (int)PyInt_AsLong(PyTuple_GetItem(order, i));
    }
    return MS_SUCCESS;
  }
#endif

  int setLayersDrawingOrder(int *panIndexes) {
    return  msSetLayersdrawingOrder(self, panIndexes); 
  }

  char *processTemplate(int bGenerateImages, char **names, char **values, int numentries) {
    return msProcessTemplate(self, bGenerateImages, names, values, numentries);
  }
  
  char *processLegendTemplate(char **names, char **values, int numentries) {
    return msProcessLegendTemplate(self, names, values, numentries);
  }
  
  char *processQueryTemplate(char **names, char **values, int numentries) {
    return msProcessQueryTemplate(self, names, values, numentries);
  }
}

//
// class extensions for layerObj, always within the context of a map
//
%extend layerObj {
  layerObj(mapObj *map) {
    if(map->numlayers == MS_MAXLAYERS) // no room
      return(NULL);

    if(initLayer(&(map->layers[map->numlayers]), map) == -1)
      return(NULL);

    map->layers[map->numlayers].index = map->numlayers;
    map->layerorder[map->numlayers] = map->numlayers;
    map->numlayers++;

    return &(map->layers[map->numlayers-1]);
  }

  ~layerObj() {
    return; // map deconstructor takes care of it
  }

  /* removeClass()
   */
  void removeClass(int index) {
    int i;
    for (i = index + 1; i < self->numclasses; i++) {
#ifndef __cplusplus
      self->class[i-1] = self->class[i];
#else
      self->_class[i-1] = self->_class[i];
#endif
    }
    self->numclasses--;
  }

  int open() {
    int status;
    status =  msLayerOpen(self);
    if (status == MS_SUCCESS) {
        return msLayerGetItems(self);
    }
    return status;
  }

  void close() {
    msLayerClose(self);
  }

  int getShape(shapeObj *shape, int tileindex, int shapeindex) {
    return msLayerGetShape(self, shape, tileindex, shapeindex);
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

  char *getItem(int i) { // returns an EXISTING item
    if(i >= 0 && i < self->numitems)
      return (self->items[i]);
    else
      return NULL;
  }

  /* raise and lower extensions have the same sense as the underlying
   * ms* functions.  'Raise' means to raise the layer within the virtual
   * stack, and draw it later.
   */

  int promote() {
    return msMoveLayerUp(self->map, self->index);

  }

  int demote() {
    return msMoveLayerDown(self->map, self->index);
  }
  
  int draw(mapObj *map, imageObj *image) {
    return msDrawLayer(map, self, image);    
  }

  int drawQuery(mapObj *map, imageObj *image) {
    return msDrawLayer(map, self, image);    
  }

  int queryByAttributes(mapObj *map, char *qitem, char *qstring, int mode) {
    return msQueryByAttributes(map, self->index, qitem, qstring, mode);
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

  %newobject getFilterString;
  char *getFilterString() {
    char exprstring[256];
    switch(self->filter.type) {
    case(MS_REGEX):
      snprintf(exprstring, 255, "/%s/", self->filter.string);
      return strdup(exprstring);
    case(MS_STRING):
      snprintf(exprstring, 255, "\"%s\"", self->filter.string);
      return strdup(exprstring);
    case(MS_EXPRESSION):
      snprintf(exprstring, 255, "(%s)", self->filter.string);
      return strdup(exprstring);
    }
    return NULL;
  }

  int setWKTProjection(char *string) {
    self->project = MS_TRUE;
    return msOGCWKT2ProjectionObj(string, &(self->projection), self->debug);
  }

  %new char *getProjection() {    
    return msGetProjectionString(&(self->projection));
  }

  int setProjection(char *string) {
    self->project = MS_TRUE;
    return msLoadProjectionString(&(self->projection), string);
  }

  int addFeature(shapeObj *shape) {    
    self->connectiontype = MS_INLINE; // set explicitly
    
    if(insertFeatureList(&(self->features), shape) == NULL) 
      return MS_FAILURE;

    return MS_SUCCESS;
  }

  char *getMetaData(char *name) {
    return(msLookupHashTable(self->metadata, name));
  }

  int setMetaData(char *name, char *value) {
    if (!self->metadata)
        self->metadata = msCreateHashTable();
    if (msInsertHashTable(self->metadata, name, value) == NULL)
	return MS_FAILURE;
    return MS_SUCCESS;
  }

  // No longer a memory leak, at least with swig 1.3.11. Swig makes a copy
  // of the returned string and then free's it.
  %new char *getWMSFeatureInfoURL(mapObj *map, int click_x, int click_y, int feature_count, char *info_format) {
    return(msWMSGetFeatureInfoURL(map, self, click_x, click_y, feature_count, info_format));
  }

  
}

//
// class extensions for classObj, always within the context of a layer
//
%extend classObj {
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

  %newobject getExpressionString;
  char *getExpressionString() {
    char exprstring[256];
    switch(self->expression.type) {
    case(MS_REGEX):
      snprintf(exprstring, 255, "/%s/", self->expression.string);
      return strdup(exprstring);
    case(MS_STRING):
      snprintf(exprstring, 255, "\"%s\"", self->expression.string);
      return strdup(exprstring);
    case(MS_EXPRESSION):
      snprintf(exprstring, 255, "(%s)", self->expression.string);
      return strdup(exprstring);
    }
    return NULL;
  }

  int setText(layerObj *layer, char *string) {
    return loadExpressionString(&self->text, string);
  }

  char *getMetaData(char *name) {
    return(msLookupHashTable(self->metadata, name));
  }

  int setMetaData(char *name, char *value) {
    if (!self->metadata)
        self->metadata = msCreateHashTable();
    if (msInsertHashTable(self->metadata, name, value) == NULL)
        return MS_FAILURE;
    return MS_SUCCESS;
  }
   
  int drawLegendIcon(mapObj *map, layerObj *layer, int width, int height, imageObj *dstImage, int dstX, int dstY) {
    return msDrawLegendIcon(map, layer, self, width, height, dstImage->img.gd, dstX, dstY);
  }
  
  imageObj *createLegendIcon(mapObj *map, layerObj *layer, int width, int height) {
    return msCreateLegendIcon(map, layer, self, width, height);
  }  
}

//
// class extensions for pointObj, useful many places
//
%extend pointObj {
  pointObj() {
    return (pointObj *)malloc(sizeof(pointObj));
  }

  ~pointObj() {
    free(self);
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectPoint(in, out, self);
  }	

  int draw(mapObj *map, layerObj *layer, imageObj *image, int classindex, char *text) {
    return msDrawPoint(map, layer, self, image, classindex, text);
  }

  double distanceToPoint(pointObj *point) {
    return msDistancePointToPoint(self, point);
  }

  double distanceToSegment(pointObj *a, pointObj *b) {
    return msDistancePointToSegment(self, a, b);
  }

  double distanceToShape(shapeObj *shape) {
    return msDistancePointToShape(self, shape);
  }
}

//
// class extensions for lineObj (eg. a line or group of points), useful many places
//
%extend lineObj {
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

  int project(projectionObj *in, projectionObj *out) {
    return msProjectLine(in, out, self);
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
	return MS_FAILURE;
    } else { /* extend array */
      self->point = (pointObj *)realloc(self->point, sizeof(pointObj)*(self->numpoints+1));
      if(!self->point)
	return MS_FAILURE;
    }

    self->point[self->numpoints].x = p->x;
    self->point[self->numpoints].y = p->y;
    self->numpoints++;

    return MS_SUCCESS;
  }

  int set(int i, pointObj *p) {
    if(i<0 || i>=self->numpoints) // invalid index
      return MS_FAILURE;

    self->point[i].x = p->x;
    self->point[i].y = p->y;
    return MS_SUCCESS;    
  }
}

//
// class extensions for shapeObj
//
%extend shapeObj {
  shapeObj(int type) {
    shapeObj *shape;

    shape = (shapeObj *)malloc(sizeof(shapeObj));
    if(!shape)
      return NULL;

    msInitShape(shape);
    if(type >= 0) shape->type = type;

    return shape;
  }

  ~shapeObj() {
    msFreeShape(self);
    free(self);		
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectShape(in, out, self);
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

  int draw(mapObj *map, layerObj *layer, imageObj *image) {
    return msDrawShape(map, layer, self, image, MS_TRUE);
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

  char *getValue(int i) { // returns an EXISTING value
    if(i >= 0 && i < self->numvalues)
      return (self->values[i]);
    else
      return NULL;
  }

  int contains(pointObj *point) {
    if(self->type == MS_SHAPE_POLYGON)
      return msIntersectPointPolygon(point, self);

    return -1;
  }

  double distanceToPoint(pointObj *point) {
    return msDistancePointToShape(point, self);
  }

  double distanceToShape(shapeObj *shape) {
    return msDistanceShapeToShape(self, shape);
  }

  int intersects(shapeObj *shape) {
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
}

//
// class extensions for rectObj
//
%extend rectObj {
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

  int project(projectionObj *in, projectionObj *out) {
    return msProjectRect(in, out, self);
  }

  double fit(int width, int height) {
    return  msAdjustExtent(self, width, height);
  } 

  /*
  int contrain(rectObj *bounds, double overlay) {
    return msConstrainRect(bounds,self, overlay);
  }
  */

  int draw(mapObj *map, layerObj *layer, imageObj *image, int classindex, char *text) {
    shapeObj shape;

    msInitShape(&shape);
    msRectToPolygon(*self, &shape);
    shape.classindex = classindex;
    shape.text = strdup(text);

    msDrawShape(map, layer, &shape, image, MS_TRUE);

    msFreeShape(&shape);
    
    return MS_SUCCESS;
  }
}

//
// class extensions for shapefileObj
//
%extend shapefileObj {
  shapefileObj(char *filename, char *shapepath, int type) {    
    shapefileObj *shapefile;
    int status;

    shapefile = (shapefileObj *)malloc(sizeof(shapefileObj));
    if(!shapefile)
      return NULL;

    if(type == -1)
      status = msSHPOpenFile(shapefile, "rb", filename);
    else if(type == -2)
      status = msSHPOpenFile(shapefile, "rb+", filename);
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
      return MS_FAILURE;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);

    return MS_SUCCESS;
  }

  int getPoint(int i, pointObj *point) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msSHPReadPoint(self->hSHP, i, point);
    return MS_SUCCESS;
  }

  int getTransformed(mapObj *map, int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);
    msTransformShapeToPixel(shape, map->extent, map->cellsize);

    return MS_SUCCESS;
  }

  void getExtent(int i, rectObj *rect) {
    msSHPReadBounds(self->hSHP, i, rect);
  }

  int add(shapeObj *shape) {
    return msSHPWriteShape(self->hSHP, shape);	
  }	

  int addPoint(pointObj *point) {    
    return msSHPWritePoint(self->hSHP, point);	
  }
}

//
// class extensions for imageObj
//
//TODO : should take image type as argument ??
%extend imageObj {
  imageObj(int width, int height) {
    imageObj *image=NULL;
    outputFormatObj *format;

    format = msCreateDefaultOutputFormat(NULL,"image/gif");
    if( format == NULL )
      format = msCreateDefaultOutputFormat(NULL,"image/png");
    if( format == NULL )
      format = msCreateDefaultOutputFormat(NULL,"image/jpeg");
    if( format == NULL )
      format = msCreateDefaultOutputFormat(NULL,"image/wbmp");
    
    image = msImageCreate(width, height, format, NULL, NULL);    

    return(image);
  }

  ~imageObj() {
    msFreeImage(self);    
    free(self);
  }

  void free() {
    msFreeImage(self);    
    free(self);
  }

  void save(char *filename) {

    // save image parameters ignored ... should be in outputFormatObj
    // used to create the imageObj.
    msSaveImage(NULL, self, filename );
  }

  // Method saveToString renders the imageObj into image data and returns
  // it as a string. Questions and comments to Sean Gillies <sgillies@frii.com>

#if defined (SWIGPYTHON) || defined (SWIGTCL8)

#ifdef SWIGPYTHON
  PyObject *saveToString() {
#elif defined (SWIGTCL8)
  Tcl_Obj *saveToString() {
#endif

    unsigned char *imgbytes;
    int size;

#ifdef SWIGPYTHON
    PyObject *imgstring; 
#elif defined (SWIGTCL8)
    Tcl_Obj *imgstring;
#endif

#if GD2_VERS > 1
    if(self->format->imagemode == MS_IMAGEMODE_RGBA) {
      gdImageSaveAlpha(self->img.gd, 1);
    } else if (self->format->imagemode == MS_IMAGEMODE_RGB) {
      gdImageSaveAlpha(self->img.gd, 0);
    }
#endif 

    if(strcasecmp("ON", msGetOutputFormatOption(self->format, "INTERLACE", "ON" )) == 0) {
      gdImageInterlace(self->img.gd, 1);
    }

    if(self->format->transparent) {
      gdImageColorTransparent(self->img.gd, 0);
    }

    if(strcasecmp(self->format->driver, "gd/gif") == 0) {

#ifdef USE_GD_GIF
      imgbytes = gdImageGifPtr(self->img.gd, &size);
#else
      msSetError(MS_MISCERR, "GIF output is not available.", "saveToString()");
      return(MS_FAILURE);
#endif

    } else if (strcasecmp(self->format->driver, "gd/png") == 0) {

#ifdef USE_GD_PNG
      imgbytes = gdImagePngPtr(self->img.gd, &size);
#else
      msSetError(MS_MISCERR, "PNG output is not available.", "saveToString()");
      return(MS_FAILURE);
#endif

    } else if (strcasecmp(self->format->driver, "gd/jpeg") == 0) {

#ifdef USE_GD_JPEG
       imgbytes = gdImageJpegPtr(self->img.gd, &size, atoi(msGetOutputFormatOption(self->format, "QUALITY", "75" )));
#else
       msSetError(MS_MISCERR, "JPEG output is not available.", "saveToString()");
       return(MS_FAILURE);
#endif

    } else if (strcasecmp(self->format->driver, "gd/wbmp") == 0) {

#ifdef USE_GD_WBMP
       imgbytes = gdImageWBMPPtr(self->img.gd, &size, 1);
#else
       msSetError(MS_MISCERR, "WBMP output is not available.", "saveToString()");
       return(MS_FAILURE);
#endif
        
    } else {
       msSetError(MS_MISCERR, "Unknown output image type driver: %s.", "saveToString()", self->format->driver );
       return(MS_FAILURE);
    } 

#ifdef SWIGPYTHON
    // Python implementation to create string
    imgstring = PyString_FromStringAndSize(imgbytes, size); 
#elif defined (SWIGTCL8)
    // Tcl implementation to create string
    imgstring = Tcl_NewByteArrayObj(imgbytes, size);    
#endif
    
    gdFree(imgbytes); // The gd docs recommend gdFree()

    return imgstring;
  }
#endif

}

// 
// class extensions for outputFormatObj
//
%extend outputFormatObj {
  outputFormatObj( const char *driver ) {
    outputFormatObj *format;

    format = msCreateDefaultOutputFormat( NULL, driver );
    if( format != NULL ) 
        format->refcount++;

    return format;
  }

  ~outputFormatObj() {
    if( --self->refcount < 1 )
      msFreeOutputFormat( self );
  }

  void setExtension( const char *extension ) {
    msFree( self->extension );
    self->extension = strdup(extension);
  }

  void setMimetype( const char *mimetype ) {
    msFree( self->mimetype );
    self->mimetype = strdup(mimetype);
  }

  void setOption( const char *key, const char *value ) {
    msSetOutputFormatOption( self, key, value );
  }
}

//
// class extensions for projectionObj
//
%extend projectionObj {
  projectionObj(char *string) {
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

  ~projectionObj() {
    msFreeProjection(self);
    free(self);		
  }
}


//
// class extensions for labelCacheObj - TP mods
//
%extend labelCacheObj {
  void freeCache() {
    msFreeLabelCache(self);    
  }
}

//
// class extensions for DBFInfo - TP mods
//
%extend DBFInfo {
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

    int getFieldType(int iField) {
	return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
    }    
}
