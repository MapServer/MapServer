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


/******************************************************************************
 * Supporting 'None' as an argument to attribute accessor functions
 ******************************************************************************
 *
 * Typemaps to support NULL in attribute accessor functions
 * provided to Steve Lime by David Beazley and tested for Python
 * only by Sean Gillies.
 *
 * With the use of these typemaps and some filtering on the mapscript
 * wrapper code to change the argument parsing of *_set() methods from
 * "Os" to "Oz", we can execute statements like
 *
 *   layer.group = None
 *
 *****************************************************************************/

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
#endif // __cplusplus

// Python-specific module code included here
#ifdef SWIGPYTHON
%include "pymodule.i"
#endif // SWIGPYTHON

// Ruby-specific module code included here
#ifdef SWIGRUBY
%include "rbmodule.i"
#endif

// Next Generation class names
#ifdef NEXT_GENERATION_NAMES
%rename(Map) map_obj;
%rename(Layer) layer_obj;
%rename(Class) class_obj;
%rename(Style) styleObj;
%rename(Image) imageObj;
%rename(Point) pointObj;
%rename(Line) lineObj;
%rename(Shape) shapeObj;
%rename(OutputFormat) outputFormatObj;
%rename(Symbol) symbolObj;
%rename(Color) colorObj;
%rename(Rect) rectObj;
%rename(Projection) projectionObj;
%rename(Shapefile) shapefileObj;
%rename(SymbolSet) symbolSetObj;
#endif

// grab mapserver declarations to wrap
%include "../../mapprimitive.h"
%include "../../mapshape.h"
%include "../../mapproject.h"
%include "../../map.h"

// try wrapping mapsymbol.h
%include "../../mapsymbol.h"

// wrap the errorObj and a few functions
%include "../../maperror.h"

%apply Pointer NONNULL { mapObj *map };
%apply Pointer NONNULL { layerObj *layer };

// Map zooming convenience methods included here
%include "../mapzoom.i"

// Language-specific extensions to mapserver classes are included here

#ifdef SWIGPYTHON
%include "pyextend.i"
#endif //SWIGPYTHON

#ifdef SWIGRUBY
%include "rbextend.i"
#endif

// A few things neccessary for automatically wrapped functions
%newobject msGetErrorString;

//
// class extensions for errorObj
//
%extend errorObj {
  errorObj() {    
    return msGetErrorObj();
  }

  ~errorObj() {}
 
  errorObj *next() {
    errorObj *ep;	

    if(self == NULL || self->next == NULL) return NULL;

    ep = msGetErrorObj();
    while(ep != self) {
      // We reached end of list of active errorObj and didn't find the errorObj... this is bad!
      if(ep->next == NULL) return NULL;
      ep = ep->next;
    }
    
    return ep->next;
  }
}

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

  %newobject clone;
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

    // Modeled after PHP-Mapscript method
    // 
    // Adjusts map extents and sets map scale
    //
    // Difference from PHP-Mapscript is the rectObj arg is optional, default
    // value is NULL, in which case the current extent values will be
    // used, making this function useful to set the scale of a new mapObj.
    int setExtent(double minx, double miny, double maxx, double maxy) {	
        // Check bounds
        if (minx > maxx || miny > maxy) {
            msSetError(MS_MISCERR, "Invalid bounds.", "setExtent()");
            return MS_FAILURE;
        }
        self->extent.minx = minx;
        self->extent.miny = miny;
        self->extent.maxx = maxx;
        self->extent.maxy = maxy;
        self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                        self->height);
        msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scale));
        return MS_SUCCESS;
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

  int getSymbolByName(char *name) {
    return msGetSymbolIndex(&self->symbolset, name);
  }

  void prepareQuery() {
    int status;

    status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scale);
    if(status != MS_SUCCESS) self->scale = -1; // degenerate extents ok here
  }

  %newobject prepareImage;
  imageObj *prepareImage() {
    int i, status;
    imageObj *image=NULL;

    if(self->width == -1 || self->height == -1) {
        msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawMap()");
        return(NULL);
    }

    msInitLabelCache(&(self->labelcache)); // this clears any previously allocated cache

    if(!self->outputformat) {
        msSetError(MS_GDERR, "Map outputformat not set!", "msDrawMap()");
        return(NULL);
    }
    else if( MS_RENDERER_GD(self->outputformat) )
    {
        image = msImageCreateGD(self->width, self->height, self->outputformat, 
				self->web.imagepath, self->web.imageurl);        
        if( image != NULL ) msImageInitGD( image, &self->imagecolor );
    }
    else if( MS_RENDERER_RAWDATA(self->outputformat) )
    {
        image = msImageCreate(self->width, self->height, self->outputformat,
                              self->web.imagepath, self->web.imageurl);
    }
#ifdef USE_MING_FLASH
    else if( MS_RENDERER_SWF(self->outputformat) )
    {
        image = msImageCreateSWF(self->width, self->height, self->outputformat,
                                 self->web.imagepath, self->web.imageurl,
                                 self);
    }
#endif
#ifdef USE_PDF
    else if( MS_RENDERER_PDF(self->outputformat) )
    {
        image = msImageCreatePDF(self->width, self->height, self->outputformat,
                                 self->web.imagepath, self->web.imageurl,
                                 self);
	}
#endif
    else
    {
        image = NULL;
    }
  
    if(!image) {
        msSetError(MS_GDERR, "Unable to initialize image.", "msDrawMap()");
        return(NULL);
    }

    self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);
    status = msCalculateScale(self->extent,self->units,self->width,self->height,
                              self->resolution, &self->scale);
    if(status != MS_SUCCESS) return(NULL);

    // compute layer scale factors now
    for(i=0;i<self->numlayers; i++) {
      if(self->layers[i].symbolscale > 0 && self->scale > 0) {
    	if(self->layers[i].sizeunits != MS_PIXELS)
      	  self->layers[i].scalefactor = (msInchesPerUnit(self->layers[i].sizeunits,0)/msInchesPerUnit(self->units,0)) / self->cellsize; 
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

  %newobject draw;
  imageObj *draw() {
    return msDrawMap(self);
  }

  %newobject drawQuery;
  imageObj *drawQuery() {
    return msDrawQueryMap(self);
  }

  %newobject drawLegend;
  imageObj *drawLegend() {
    return msDrawLegend(self);
  }

  %newobject drawScalebar;
  imageObj *drawScalebar() {
    return msDrawScalebar(self);
  }

  %newobject drawReferenceMap;
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

  %newobject getProjection;
  char *getProjection() {
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
  
  int removeMetaData(char *name) {
    return(msRemoveHashTable(self->web.metadata, name));
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

  int setFontSet(char *filename) {
    msFreeFontSet(&(self->fontset));
    msInitFontSet(&(self->fontset));
   
    // Set fontset filename
    self->fontset.filename = strdup(filename);

    return msLoadFontSet(&(self->fontset), self);
  }

  // I removed a method to get the fonset filename. Instead I updated map.h
  // to allow SWIG access to the fonset, although the numfonts and filename
  // members are read-only. Use the setFontSet method to actually change the
  // fontset. To get the filename do $map->{fontset}->{filename};
  
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

  int setLayersDrawingOrder(int *panIndexes) {
    return  msSetLayersdrawingOrder(self, panIndexes); 
  }

  /* SLD */
  
    int applySLD(char *sld) {
        return msSLDApplySLD(self, sld, -1, NULL);
    }

    int applySLDURL(char *sld) {
        return msSLDApplySLDURL(self, sld, -1, NULL);
    }
    
    %newobject generateSLD;
    char *generateSLD() {
        return msSLDGenerateSLD(self, -1);
    }


    %newobject processTemplate;
    char *processTemplate(int bGenerateImages, char **names, char **values,
                          int numentries)
    {
        return msProcessTemplate(self, bGenerateImages, names, values,
                                 numentries);
    }
  
    %newobject processLegendTemplate;
    char *processLegendTemplate(char **names, char **values, int numentries) {
        return msProcessLegendTemplate(self, names, values, numentries);
    }
  
    %newobject processQueryTemplate;
    char *processQueryTemplate(char **names, char **values, int numentries) {
        return msProcessQueryTemplate(self, 1, names, values, numentries);
    }

    outputFormatObj *getOutputFormatByName(char *name) {
        return msSelectOutputFormat(self, name); 
    }
    
    int appendOutputFormat(outputFormatObj *format) {
        return msAppendOutputFormat(self, format);
    }

    int removeOutputFormat(char *name) {
        return msRemoveOutputFormat(self, name);
    }
}

/* Full support for symbols and addition of them to the map symbolset
 * is done to resolve MapServer bug 579
 * http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=579 */

%extend symbolObj {
    symbolObj(char *symbolname, const char *imagefile=NULL) {
        symbolObj *symbol;
        symbol = (symbolObj *) malloc(sizeof(symbolObj));
        initSymbol(symbol);
        symbol->name = strdup(symbolname);
        if (imagefile) {
            msLoadImageSymbol(symbol, imagefile);
        }
        return symbol;
    }

    ~symbolObj() {
        if (self->name) free(self->name);
        if (self->img) gdImageDestroy(self->img);
        if (self->font) free(self->font);
        if (self->imagepath) free(self->imagepath);
    }

    int setPoints(lineObj *line) {
        int i;
        for (i=0; i<line->numpoints; i++) {
            msCopyPoint(&(self->points[i]), &(line->point[i]));
        }
        self->numpoints = line->numpoints;
        return self->numpoints;
    }

    %newobject getPoints;
    lineObj *getPoints() {
        int i;
        lineObj *line;
        line = (lineObj *) malloc(sizeof(lineObj));
        line->point = (pointObj *) malloc(sizeof(pointObj)*(self->numpoints));
        for (i=0; i<self->numpoints; i++) {
            line->point[i].x = self->points[i].x;
            line->point[i].y = self->points[i].y;
        }
        line->numpoints = self->numpoints;
        return line;
    }

    int setStyle(int index, int value) {
        if (index < 0 || index > MS_MAXSTYLELENGTH) {
            msSetError(MS_SYMERR, "Can't set style at index %d.", "setStyle()", index);
            return MS_FAILURE;
        }
        self->style[index] = value;
        return MS_SUCCESS;
    }

}

%extend symbolSetObj {

    symbolSetObj(const char *symbolfile=NULL) {
        symbolSetObj *symbolset;
        mapObj *temp_map=NULL;
        symbolset = (symbolSetObj *) malloc(sizeof(symbolSetObj));
        msInitSymbolSet(symbolset);
        if (symbolfile) {
            symbolset->filename = strdup(symbolfile);
            temp_map = msNewMapObj();
            msLoadSymbolSet(symbolset, temp_map);
            symbolset->map = NULL;
            msFreeMap(temp_map);
        }
        return symbolset;
    }
   
    ~symbolSetObj() {
        msFreeSymbolSet(self);
    }

    symbolObj *getSymbol(int i) {
        if(i >= 0 && i < self->numsymbols)	
            return (symbolObj *) &(self->symbol[i]);
        else
            return NULL;
    }

    symbolObj *getSymbolByName(char *name) {
        int i;

        if (!name) return NULL;

        i = msGetSymbolIndex(self, name);
        if (i == -1)
            return NULL; // no such symbol
        else
            return (symbolObj *) &(self->symbol[i]);
    }

    int appendSymbol(symbolObj *symbol) {
        return msAppendSymbol(self, symbol);
    }
 
    %newobject removeSymbol;
    symbolObj *removeSymbol(int index) {
        return msRemoveSymbol(self, index);
    }

    int save(const char *filename) {
        return msSaveSymbolSet(self, filename);
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

#ifdef NEXT_GENERATION_API
    %newobject getShape;
    shapeObj *getShape(int shapeindex, int tileindex=0) {
    /* This version properly returns shapeObj and also has its
     * arguments properly ordered so that users can ignore the
     * tileindex if they are not accessing a tileindexed layer.
     * See bug 586:
     * http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=586 */
        int retval;
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = self->type;
        retval = msLayerGetShape(self, shape, tileindex, shapeindex);
        return shape;
    }
#else
    int getShape(shapeObj *shape, int tileindex, int shapeindex) {
        return msLayerGetShape(self, shape, tileindex, shapeindex);
    }
#endif
  
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
    return msMoveLayerDown(self->map, self->index);
  }

  int demote() {
    return msMoveLayerUp(self->map, self->index);
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
      if (!string || strlen(string) == 0) {
          freeExpression(&self->filter);
          return MS_SUCCESS;
      }
      else return loadExpressionString(&self->filter, string);
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

  %newobject getProjection;
  char *getProjection() {    
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

  /*
  Returns the number of inline feature of a layer
  */
  int getNumFeatures() {
      return msLayerGetNumFeatures(self);
  }

  %newobject getExtent;
  rectObj *getExtent() {
      rectObj *extent;
      extent = (rectObj *) malloc(sizeof(rectObj));
      msLayerOpen(self);
      msLayerGetExtent(self, extent);
      msLayerClose(self);
      return extent;
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

  int removeMetaData(char *name) {
    return(msRemoveHashTable(self->metadata, name));
  }

    %newobject getWMSFeatureInfoURL;
    char *getWMSFeatureInfoURL(mapObj *map, int click_x, int click_y,
                               int feature_count, char *info_format)
    {
        return(msWMSGetFeatureInfoURL(map, self, click_x, click_y,
               feature_count, info_format));
    }
 
    %newobject executeWFSGetFeature;
    char *executeWFSGetFeature(layerObj *layer) {
        return (msWFSExecuteGetFeature(layer));
    }

    int applySLD(char *sld, char *stylelayer) {
        return msSLDApplySLD(self->map, sld, self->index, stylelayer);
    }

    int applySLDURL(char *sld, char *stylelayer) {
        return msSLDApplySLDURL(self->map, sld, self->index, stylelayer);
    }

    %newobject generateSLD; 
    char *generateSLD() {
        return msSLDGenerateSLD(self->map, self->index);
    }

    int moveClassUp(int index) {
        return msMoveClassUp(self, index);
    }

    int moveClassDown(int index) {
        return msMoveClassDown(self, index);
    }

}

// See Bugzilla issue 548 about work on styleObj and classObj
%extend styleObj {

    styleObj() {
        styleObj *style;
        int result;
        style = (styleObj *)calloc(1, sizeof(styleObj));
        if (!style) return NULL;
        result = initStyle(style);
        if (result == MS_SUCCESS) {
            return style;
        }
        else {
            msSetError(MS_MISCERR, "Failed to initialize styleObj", "styleObj()");
            return NULL;
        }
    }
    
}

//
// Class extensions for classObj, always within the context of a layer
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
      if (!string || strlen(string) == 0) {
          freeExpression(&self->expression);
          return MS_SUCCESS;
      }
      else return loadExpressionString(&self->expression, string);
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

  // Should be deprecated!  Completely bogus layer argument.  SG.
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
 
  %newobject createLegendIcon;
  imageObj *createLegendIcon(mapObj *map, layerObj *layer, int width, int height) {
    return msCreateLegendIcon(map, layer, self, width, height);
  } 

    // See Bugzilla issue 548 for more details about the *Style methods

    styleObj *getStyle(int i) {
        if (i >= 0 && i < self->numstyles)	
            return &(self->styles[i]);
        else {
            msSetError(MS_CHILDERR, "Invalid index: %d", "getStyle()", i);
            return NULL;
        }
    }

    int insertStyle(styleObj *style, int index=-1) {
        return msInsertStyle(self, style, index);
    }

    %newobject removeStyle;
    styleObj *removeStyle(int index) {
        return msRemoveStyle(self, index);
    }

    int moveStyleUp(int index) {
        return msMoveStyleUp(self, index);
    }

    int moveStyleDown(int index) {
       return msMoveStyleDown(self, index);
    }
}

//
// class extensions for pointObj, useful many places
//
%extend pointObj {
  pointObj(double x=0.0, double y=0.0) {
      pointObj *p;
      p = (pointObj *)malloc(sizeof(pointObj));
      if (!p) return NULL;
      p->x = x;
      p->y = y;
      return p;
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
    msComputeBounds(self);
    return;
  }

#ifdef NEXT_GENERATION_API
    %newobject copy;
    shapeObj *copy() {
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = self->type;
        msCopyShape(self, shape);
        return shape;
    }
#else
    int copy(shapeObj *dest) {
        return(msCopyShape(self, dest));
    }
#endif

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
  rectObj(double minx=0.0, double miny=0.0, double maxx=0.0, double maxy=0.0) {	
    rectObj *rect;
    
    // Check bounds
    if (minx > maxx || miny > maxy) {
        msSetError(MS_MISCERR, "Invalid bounds.", "rectObj()");
        return NULL;
    }
    
    rect = (rectObj *)calloc(1, sizeof(rectObj));
    if(!rect)
      return(NULL);
    
    rect->minx = minx;
    rect->miny = miny;
    rect->maxx = maxx;
    rect->maxy = maxy;

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

    %newobject toPolygon;
    shapeObj *toPolygon() {
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = MS_SHAPE_POLYGON;
        msRectToPolygon(*self, shape);
        return shape;
    }
    
}

//
// class extensions for shapefileObj
//
%extend shapefileObj {
  shapefileObj(char *filename, int type) {    
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
   
    /* imageObj constructor now takes filename as an optional argument.
     * If the target language is Python, we ignore this constructor and
     * instead use the one in python/pymodule.i. */
#ifndef SWIGPYTHON
    imageObj(int width, int height, const char *driver=NULL,
             const char *file=NULL)
    {
        imageObj *image=NULL;
        outputFormatObj *format;

        if (file) {
            return msImageLoadGD(file);
        }
        else {
            if (driver) {
                format = msCreateDefaultOutputFormat(NULL, driver);
            }
            else {
                format = msCreateDefaultOutputFormat(NULL, "GD/GIF");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/PNG");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/JPEG");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/WBMP");
            }
            if (format == NULL) {
                msSetError(MS_IMGERR, "Could not create output format %s",
                           "imageObj()", driver);
                return NULL;
            }
            image = msImageCreate(width, height, format, NULL, NULL);
            return image;
        }
    }
#endif // SWIGPYTHON

  ~imageObj() {
    msFreeImage(self);    
  }

  void free() {
    msFreeImage(self);    
  }

    /* saveGeo - see Bugzilla issue 549 */ 
    void save(char *filename, mapObj *map=NULL) {
        msSaveImage(map, self, filename );
    }

  // Method saveToString renders the imageObj into image data and returns
  // it as a string. Questions and comments to Sean Gillies <sgillies@frii.com>

#if defined SWIGTCL8

  Tcl_Obj *saveToString() {

    unsigned char *imgbytes;
    int size;
    Tcl_Obj *imgstring;

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

    // Tcl implementation to create string
    imgstring = Tcl_NewByteArrayObj(imgbytes, size);    
    
    gdFree(imgbytes); // The gd docs recommend gdFree()

    return imgstring;
  }
#endif

}

// 
// class extensions for outputFormatObj
//
%extend outputFormatObj {
    outputFormatObj(const char *driver, char *name=NULL) {
    outputFormatObj *format;

    format = msCreateDefaultOutputFormat(NULL, driver);
    if( format != NULL ) 
        format->refcount++;
    if (name != NULL)
        format->name = strdup(name);
    
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

    %newobject getOption;
    char *getOption(const char *key, const char *value="") {
        const char *retval;
        retval = msGetOutputFormatOption(self, key, value);
        return strdup(retval);
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

// extension to colorObj

%extend colorObj {
  
    colorObj(int red=0, int green=0, int blue=0) {
        colorObj *color;
        
        // Check colors
        if (red > 255 || green > 255 || blue > 255) {
            msSetError(MS_MISCERR, "Invalid color index.", "colorObj()");
            return NULL;
        }
    
        color = (colorObj *)calloc(1, sizeof(colorObj));
        if (!color)
            return(NULL);
    
        color->red = red;
        color->green = green;
        color->blue = blue;

        return(color);    	
    }

    ~colorObj() {
        free(self);
    }
 
    int setRGB(int red, int green, int blue) {
        // Check colors
        if (red > 255 || green > 255 || blue > 255) {
            msSetError(MS_MISCERR, "Invalid color index.", "setRGB()");
            return MS_FAILURE;
        }
    
        self->red = red;
        self->green = green;
        self->blue = blue;
        return MS_SUCCESS;
    }

    int setHex(char *psHexColor) {
        int red, green, blue;
        if (psHexColor && strlen(psHexColor)== 7 && psHexColor[0] == '#') {
            red = hex2int(psHexColor+1);
            green = hex2int(psHexColor+3);
            blue= hex2int(psHexColor+5);
            if (red > 255 || green > 255 || blue > 255) {
                msSetError(MS_MISCERR, "Invalid color index.", "setHex()");
                return MS_FAILURE;
            }
            self->red = red;
            self->green = green;
            self->blue = blue;
            return MS_SUCCESS;
        }
        else {
            msSetError(MS_MISCERR, "Invalid hex color.", "setHex()");
            return MS_FAILURE;
        }
    }   
    
    %newobject toHex;
    char *toHex() {
        char hexcolor[7];
        sprintf(hexcolor, "#%02x%02x%02x", self->red, self->green, self->blue);
        return strdup(hexcolor);
    }
}

