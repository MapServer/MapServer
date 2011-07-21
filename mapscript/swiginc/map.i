/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript mapObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 1996-2001 Regents of the University of Minnesota.
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================
*/

%extend mapObj 
{

    mapObj(char *filename="") 
    {
        if (filename && strlen(filename))
            return msLoadMap(filename, NULL);
        else { /* create an empty map, no layers etc... */
            return msNewMapObj();
        }      
    }

    ~mapObj() 
    {
        msFreeMap(self);
    }

#ifdef SWIGJAVA
    %newobject cloneMap;
    mapObj *cloneMap() 
#else
    %newobject clone;
    mapObj *clone() 
#endif
    {
        mapObj *dstMap;
        dstMap = msNewMapObj();
        if (msCopyMap(dstMap, self) != MS_SUCCESS) {
            msFreeMap(dstMap);
            dstMap = NULL;
        }
        return dstMap;
    }

#ifdef SWIGCSHARP
%apply SWIGTYPE *SETREFERENCE {layerObj *layer};
#endif
    int insertLayer(layerObj *layer, int index=-1) 
    {
        return msInsertLayer(self, layer, index);  
    }
#ifdef SWIGCSHARP
%clear layerObj *layer;
#endif
    
    %newobject removeLayer;
    layerObj *removeLayer(int index) 
    {
    	layerObj *layer=msRemoveLayer(self, index);
	MS_REFCNT_INCR(layer);
        return layer;
    }
    
    int setExtent(double minx, double miny, double maxx, double maxy) {	
	return msMapSetExtent( self, minx, miny, maxx, maxy );
    }
    
    int offsetExtent(double x, double y) {	
	return msMapOffsetExtent( self, x, y );
    }
    
    int scaleExtent(double zoomfactor, 
					 double minscaledenom, double maxscaledenom) {	
	return msMapScaleExtent( self, zoomfactor, minscaledenom, maxscaledenom );
    }
    
    int setCenter(pointObj *center) {	
	return msMapSetCenter( self, center );
    }

    /* recent rotation work makes setSize the only reliable 
       method for changing the image size.  direct access is deprecated. */
    int setSize(int width, int height) 
    {
        return msMapSetSize(self, width, height);
    }
    
    int setRotation( double rotation_angle ) 
    {
        return msMapSetRotation( self, rotation_angle );
    }
  
  %newobject getLayer;
  layerObj *getLayer(int i) {
    if(i >= 0 && i < self->numlayers) {
    	MS_REFCNT_INCR(self->layers[i]);
      	return (self->layers[i]); /* returns an EXISTING layer */
    } else {
      return NULL;
    }
  }

  %newobject getLayerByName;
  layerObj *getLayerByName(char *name) {
    int i;

    i = msGetLayerIndex(self, name);

    if(i != -1) {
      MS_REFCNT_INCR(self->layers[i]);
      return (self->layers[i]); /* returns an EXISTING layer */
    }
    else
      return NULL;
  }

  int getSymbolByName(char *name) {
    return msGetSymbolIndex(&self->symbolset, name, MS_TRUE);
  }

  void prepareQuery() {
    int status;

    status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scaledenom);
    if(status != MS_SUCCESS) self->scaledenom = -1;
  }

  %newobject prepareImage;
  imageObj *prepareImage() 
  {
    return msPrepareImage(self, MS_FALSE);
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

    void selectOutputFormat( char *imagetype )
    {
        outputFormatObj *format;

        format = msSelectOutputFormat( self, imagetype );
        if ( format == NULL )
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
    return msDrawMap(self, MS_FALSE);
  }

  %newobject drawQuery;
  imageObj *drawQuery() {
    return msDrawMap(self, MS_TRUE);
  }

  %newobject drawLegend;
  imageObj *drawLegend() {
    return msDrawLegend(self, MS_FALSE);
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
    return msEmbedScalebar(self, image);
  }

  int embedLegend(imageObj *image) {	
    return msEmbedLegend(self, image);
  }

  int drawLabelCache(imageObj *image) {
    return msDrawLabelCache(image, self);
  }

  labelCacheMemberObj *getLabel(int i) {
    return msGetLabelCacheMember(&(self->labelcache), i);
  }

  labelCacheMemberObj *nextLabel() {
    static int i=0;

    if(i<self->labelcache.numlabels)
      return msGetLabelCacheMember(&(self->labelcache), i++);
    else
      return NULL;	
  }

  int queryByFilter(char *string) {
    msInitQuery(&(self->query));

    self->query.type = MS_QUERY_BY_FILTER;

    self->query.filter = (expressionObj *) malloc(sizeof(expressionObj));
    self->query.filter->string = strdup(string);
    self->query.filter->type = 2000; /* MS_EXPRESSION: lot's of conflicts in mapfile.h */

    self->query.rect = self->extent;

    return msQueryByFilter(self);
  }

  int queryByPoint(pointObj *point, int mode, double buffer) {
    msInitQuery(&(self->query));

    self->query.type = MS_QUERY_BY_POINT;
    self->query.mode = mode;
    self->query.point = *point;
    self->query.buffer = buffer;

    return msQueryByPoint(self);
  }

  int queryByRect(rectObj rect) {
    msInitQuery(&(self->query));

    self->query.type = MS_QUERY_BY_RECT;
    self->query.mode = MS_QUERY_MULTIPLE;
    self->query.rect = rect;

    return msQueryByRect(self);
  }

  int queryByFeatures(int slayer) {
    self->query.slayer = slayer;
    return msQueryByFeatures(self);
  }

  int queryByShape(shapeObj *shape) {
    msInitQuery(&(self->query));
    
    self->query.type = MS_QUERY_BY_SHAPE;
    self->query.mode = MS_QUERY_MULTIPLE;
    self->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(self->query.shape);
    msCopyShape(shape, self->query.shape);

    return msQueryByShape(self);
  }

  int setWKTProjection(char *wkt) {
    return msOGCWKT2ProjectionObj(wkt, &(self->projection), self->debug);
  }

  %newobject getProjection;
  char *getProjection() {
    return msGetProjectionString(&(self->projection));
  }

  int setProjection(char *proj4) {
    return msLoadProjectionString(&(self->projection), proj4);
  }

  int save(char *filename) {
    return msSaveMap(self, filename);
  }

  int saveQuery(char *filename, int results=MS_FALSE) {
    return msSaveQuery(self, filename, results);
  }

  int loadQuery(char *filename)  {
    return msLoadQuery(self, filename);
  }

  void freeQuery(int qlayer=-1) {
    msQueryFree(self, qlayer);
  }

  int saveQueryAsGML(char *filename, const char *ns="GOMF") {
    return msGMLWriteQuery(self, filename, ns);
  }

  char *getMetaData(char *name) {
    char *value = NULL;
    if (!name) {
      msSetError(MS_HASHERR, "NULL key", "getMetaData");
    }
     
    value = (char *) msLookupHashTable(&(self->web.metadata), name);
    if (!value) {
      msSetError(MS_HASHERR, "Key %s does not exist", "getMetaData", name);
      return NULL;
    }
    return value;
  }

  int setMetaData(char *name, char *value) {
    if (msInsertHashTable(&(self->web.metadata), name, value) == NULL)
	return MS_FAILURE;
    return MS_SUCCESS;
  }
  
  int removeMetaData(char *name) {
    return(msRemoveHashTable(&(self->web.metadata), name));
  }

  char *getFirstMetaDataKey() {
    return (char *) msFirstKeyFromHashTable(&(self->web.metadata));
  }
 
  char *getNextMetaDataKey(char *lastkey) {
    return (char *) msNextKeyFromHashTable(&(self->web.metadata), lastkey);
  }
  
  int setSymbolSet(char *szFileName) {
    msFreeSymbolSet(&self->symbolset);
    msInitSymbolSet(&self->symbolset);
   
    self->symbolset.filename = strdup(szFileName);

    /* Symbolset shares same fontset as main mapfile */
    self->symbolset.fontset = &(self->fontset);

    return msLoadSymbolSet(&self->symbolset, self);
  }

  int getNumSymbols() {
    return self->symbolset.numsymbols;
  }

  int setFontSet(char *filename) {
    msFreeFontSet(&(self->fontset));
    msInitFontSet(&(self->fontset));
   
    self->fontset.filename = strdup(filename);

    return msLoadFontSet(&(self->fontset), self);
  }

  /* I removed a method to get the fonset filename. Instead I updated mapserver.h
   to allow SWIG access to the fonset, although the numfonts and filename
   members are read-only. Use the setFontSet method to actually change the
   fontset. To get the filename do $map->{fontset}->{filename}; -- SG */
  
  int saveMapContext(char *szFileName) {
    return msSaveMapContext(self, szFileName);
  }

  int loadMapContext(char *szFileName, int useUniqueNames=MS_FALSE) {
    return msLoadMapContext(self, szFileName, useUniqueNames);
  }

  int  moveLayerUp(int layerindex) {
    return msMoveLayerUp(self, layerindex);
  }

  int  moveLayerDown(int layerindex) {
    return msMoveLayerDown(self, layerindex);
  }

  %newobject getLayersDrawingOrder;
  intarray *getLayersDrawingOrder() {
    int i;
    intarray *order;
    order = new_intarray(self->numlayers);
    for (i=0; i<self->numlayers; i++)
        #if (defined(SWIGPYTHON) || defined(SWIGRUBY) ) && SWIG_VERSION >= 0x010328 /* 1.3.28 */
        intarray___setitem__(order, i, self->layerorder[i]);
        #else
        intarray_setitem(order, i, self->layerorder[i]);
        #endif
    return order;
  }

  int setLayersDrawingOrder(int *panIndexes) {
    return  msSetLayersdrawingOrder(self, panIndexes); 
  }

  void setConfigOption(char *key, char *value) {
    msSetConfigOption(self,key,value);
  }

  char *getConfigOption(char *key) {
    return (char *) msGetConfigOption(self,key);
  }

  void applyConfigOptions() {
    msApplyMapConfigOptions( self );
  } 

  /* SLD */
  
    int applySLD(char *sld) {
      return msSLDApplySLD(self, sld, -1, NULL, NULL);
    }

    int applySLDURL(char *sld) {
      return msSLDApplySLDURL(self, sld, -1, NULL, NULL);
    }
    
    %newobject generateSLD;
    char *generateSLD() {
        return (char *) msSLDGenerateSLD(self, -1, NULL);
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

    int loadOWSParameters(cgiRequestObj *request, char *wmtver_string="1.1.1") 
    {
        return msMapLoadOWSParameters(self, request, wmtver_string);
    }

    int OWSDispatch( cgiRequestObj *req )
    {
	return msOWSDispatch( self, req, MS_TRUE );
    }

}
