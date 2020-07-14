/* ===========================================================================
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
    %feature("docstring")
    ""
    mapObj(char *filename="") 
    {
        if (filename && strlen(filename))
            return msLoadMap(filename, NULL);
        else { /* create an empty map, no layers etc... */
            return msNewMapObj();
        }      
    }

#ifdef SWIGCSHARP      
    mapObj(char *mapText, int isMapText /*used as signature only to differentiate this constructor from default constructor*/ ) 
    {
        return msLoadMapFromString(mapText, NULL);
    }
#endif 
    
    ~mapObj() 
    {
        msFreeMap(self);
    }

#if defined (SWIGJAVA) || defined (SWIGPHP)
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
    %feature("docstring")
    ""
    int insertLayer(layerObj *layer, int index=-1) 
    {
        return msInsertLayer(self, layer, index);  
    }
#ifdef SWIGCSHARP
%clear layerObj *layer;
#endif

    %feature("docstring")
    ""
    %newobject removeLayer;
    layerObj *removeLayer(int index) 
    {
        layerObj *layer=msRemoveLayer(self, index);
        MS_REFCNT_INCR(layer);
        return layer;
    }

    %feature("docstring")
    ""
    int setExtent(double minx, double miny, double maxx, double maxy) {
        return msMapSetExtent( self, minx, miny, maxx, maxy );
    }

    %feature("docstring")
    ""
    int offsetExtent(double x, double y) {
        return msMapOffsetExtent( self, x, y );
    }

    %feature("docstring")
    ""
    int scaleExtent(double zoomfactor, double minscaledenom, double maxscaledenom) {
        return msMapScaleExtent( self, zoomfactor, minscaledenom, maxscaledenom );
    }

    %feature("docstring")
    ""
    int setCenter(pointObj *center) {
        return msMapSetCenter( self, center );
    }

    /* recent rotation work makes setSize the only reliable 
       method for changing the image size.  direct access is deprecated. */
    %feature("docstring")
    ""
    int setSize(int width, int height) 
    {
        return msMapSetSize(self, width, height);
    }

    %feature("docstring")
    ""
    void pixelToGeoref(double pixPosX, double pixPosY, pointObj *geoPos)
    {
       geoPos->x = self->gt.geotransform[0] + self->gt.geotransform[1] * pixPosX + self->gt.geotransform[2] * pixPosY;
       geoPos->y = self->gt.geotransform[3] + self->gt.geotransform[4] * pixPosX + self->gt.geotransform[5] * pixPosY;
    }

    %feature("docstring")
    ""
    double getRotation() 
    {
        return self->gt.rotation_angle;
    }

    %feature("docstring")
    ""
    int setRotation( double rotation_angle ) 
    {
        return msMapSetRotation( self, rotation_angle );
    }

  %feature("docstring")
  ""
  %newobject getLayer;
  layerObj *getLayer(int i) {
    if(i >= 0 && i < self->numlayers) {
        MS_REFCNT_INCR(self->layers[i]);
        return (self->layers[i]); /* returns an EXISTING layer */
    } else {
      return NULL;
    }
  }

  %feature("docstring")
  ""
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

  %feature("docstring")
  ""
  int getSymbolByName(char *name) {
    return msGetSymbolIndex(&self->symbolset, name, MS_TRUE);
  }

  %feature("docstring")
  ""
  void prepareQuery() {
    int status;

    status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scaledenom);
    if(status != MS_SUCCESS) self->scaledenom = -1;
  }

  %feature("docstring")
  ""
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
          self->imagetype = msStrdup(imagetype);
          msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE, 
                               MS_NOOVERRIDE, MS_NOOVERRIDE );
      }
  }

    %feature("docstring")
    ""
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
            self->imagetype = msStrdup(imagetype);
            msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE, 
                                 MS_NOOVERRIDE, MS_NOOVERRIDE );
        }
    }

  %feature("docstring")
  ""
  %newobject getOutputFormat;
  outputFormatObj *getOutputFormat(int i) {
    if(i >= 0 && i < self->numoutputformats) {
        MS_REFCNT_INCR(self->outputformatlist[i]);
        return (self->outputformatlist[i]); 
    } else {
      return NULL;
    }
  }

  %feature("docstring")
  ""
  void setOutputFormat( outputFormatObj *format ) {
      msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE, 
                           MS_NOOVERRIDE, MS_NOOVERRIDE );
  }

  %feature("docstring")
  ""
  %newobject draw;
  imageObj *draw() {
#if defined(WIN32) && defined(SWIGCSHARP)
    __try {
        return msDrawMap(self, MS_FALSE);
    }    
    __except(1 /*EXCEPTION_EXECUTE_HANDLER, catch every exception so it doesn't crash IIS*/) {  
        msSetError(MS_IMGERR, "Unhandled exception in drawing map image 0x%08x", "msDrawMap()", GetExceptionCode());
    }
#else    
    return msDrawMap(self, MS_FALSE);
#endif    
  }

  %feature("docstring")
  ""
  %newobject drawQuery;
  imageObj *drawQuery() {
    return msDrawMap(self, MS_TRUE);
  }

  %feature("docstring")
  ""
  %newobject drawLegend;
  imageObj *drawLegend() {
    return msDrawLegend(self, MS_FALSE, NULL);
  }

  %feature("docstring")
  ""
  %newobject drawScalebar;
  imageObj *drawScalebar() {
    return msDrawScalebar(self);
  }

  %feature("docstring")
  ""
  %newobject drawReferenceMap;
  imageObj *drawReferenceMap() {
    return msDrawReferenceMap(self);
  }

  %feature("docstring")
  ""
  int embedScalebar(imageObj *image) {	
    return msEmbedScalebar(self, image);
  }

  %feature("docstring")
  ""
  int embedLegend(imageObj *image) {	
    return msEmbedLegend(self, image);
  }

  int drawLabelCache(imageObj *image) {
    return msDrawLabelCache(self,image);
  }

  %feature("docstring")
  ""
  int queryByFilter(char *string) {
    msInitQuery(&(self->query));

    self->query.type = MS_QUERY_BY_FILTER;
    self->query.mode = MS_QUERY_MULTIPLE;

    self->query.filter.string = msStrdup(string);
    self->query.filter.type = MS_EXPRESSION;

    self->query.rect = self->extent;

    return msQueryByFilter(self);
  }

  %feature("docstring")
  ""
  int queryByPoint(pointObj *point, int mode, double buffer) {
    msInitQuery(&(self->query));

    self->query.type = MS_QUERY_BY_POINT;
    self->query.mode = mode;
    self->query.point = *point;
    self->query.buffer = buffer;

    return msQueryByPoint(self);
  }

  %feature("docstring")
  ""
  int queryByRect(rectObj rect) {
    msInitQuery(&(self->query));

    self->query.type = MS_QUERY_BY_RECT;
    self->query.mode = MS_QUERY_MULTIPLE;
    self->query.rect = rect;

    return msQueryByRect(self);
  }

  %feature("docstring")
  ""
  int queryByFeatures(int slayer) {
    self->query.slayer = slayer;
    return msQueryByFeatures(self);
  }

  %feature("docstring")
  ""
  int queryByShape(shapeObj *shape) {
    msInitQuery(&(self->query));
    
    self->query.type = MS_QUERY_BY_SHAPE;
    self->query.mode = MS_QUERY_MULTIPLE;
    self->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(self->query.shape);
    msCopyShape(shape, self->query.shape);

    return msQueryByShape(self);
  }

  %feature("docstring")
  ""
  int setWKTProjection(char *wkt) {
    return msOGCWKT2ProjectionObj(wkt, &(self->projection), self->debug);
  }

  %feature("docstring")
  ""
  %newobject getProjection;
  char *getProjection() {
    return msGetProjectionString(&(self->projection));
  }

  %feature("docstring")
  ""
  int setProjection(char *proj4) {
    return msLoadProjectionString(&(self->projection), proj4);
  }

  %feature("docstring")
  ""
  int save(char *filename) {
    return msSaveMap(self, filename);
  }

  %feature("docstring")
  ""
  int saveQuery(char *filename, int results=MS_FALSE) {
    return msSaveQuery(self, filename, results);
  }

  %feature("docstring")
  ""
  int loadQuery(char *filename)  {
    return msLoadQuery(self, filename);
  }

  %feature("docstring")
  ""
  void freeQuery(int qlayer=-1) {
    msQueryFree(self, qlayer);
  }

  %feature("docstring")
  ""
  int saveQueryAsGML(char *filename, const char *ns="GOMF") {
    return msGMLWriteQuery(self, filename, ns);
  }

  %feature("docstring")
  ""
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

  %feature("docstring")
  ""
  int setMetaData(char *name, char *value) {
    if (msInsertHashTable(&(self->web.metadata), name, value) == NULL)
        return MS_FAILURE;
    return MS_SUCCESS;
  }

  %feature("docstring")
  ""
  int removeMetaData(char *name) {
    return(msRemoveHashTable(&(self->web.metadata), name));
  }

  %feature("docstring")
  ""
  char *getFirstMetaDataKey() {
    return (char *) msFirstKeyFromHashTable(&(self->web.metadata));
  }

  %feature("docstring")
  ""
  char *getNextMetaDataKey(char *lastkey) {
    return (char *) msNextKeyFromHashTable(&(self->web.metadata), lastkey);
  }

  %feature("docstring")
  ""
  int setSymbolSet(char *szFileName) {
    msFreeSymbolSet(&self->symbolset);
    msInitSymbolSet(&self->symbolset);
   
    self->symbolset.filename = msStrdup(szFileName);

    /* Symbolset shares same fontset as main mapfile */
    self->symbolset.fontset = &(self->fontset);

    return msLoadSymbolSet(&self->symbolset, self);
  }

  %feature("docstring")
  ""
  int getNumSymbols() {
    return self->symbolset.numsymbols;
  }

  %feature("docstring")
  ""
  int setFontSet(char *filename) {
    msFreeFontSet(&(self->fontset));
    msInitFontSet(&(self->fontset));
   
    self->fontset.filename = msStrdup(filename);

    return msLoadFontSet(&(self->fontset), self);
  }

  /* I removed a method to get the fonset filename. Instead I updated mapserver.h
   to allow SWIG access to the fonset, although the numfonts and filename
   members are read-only. Use the setFontSet method to actually change the
   fontset. To get the filename do $map->{fontset}->{filename}; -- SG */

  %feature("docstring")
  ""
  int saveMapContext(char *szFileName) {
    return msSaveMapContext(self, szFileName);
  }

  %feature("docstring")
  ""
  int loadMapContext(char *szFileName, int useUniqueNames=MS_FALSE) {
    return msLoadMapContext(self, szFileName, useUniqueNames);
  }

  %feature("docstring")
  "Move the layer at layerindex up in the drawing order array, meaning that it is drawn earlier."
  "Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`"
  int  moveLayerUp(int layerindex) {
    return msMoveLayerUp(self, layerindex);
  }

  %feature("docstring")
  "Move the layer at layerindex down in the drawing order array, meaning that it is drawn later. "
  "Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`"
  int  moveLayerDown(int layerindex) {
    return msMoveLayerDown(self, layerindex);
  }

  %feature("docstring")
  "Returns an array of layer indexes in drawing order."
  "Note Unless the proper typemap is implemented for the module's language a user is more likely to get back an unusable SWIG pointer to the integer array."
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

  %feature("docstring")
  "Set map layer drawing order."
  "Note Unless the proper typemap is implemented for the module's language users will not be able to pass arrays or lists to this method and it will be unusable."
  int setLayersDrawingOrder(int *panIndexes) {
    return msSetLayersdrawingOrder(self, panIndexes); 
  }

  %feature("docstring")
  "Set the indicated key configuration option to the indicated value. Equivalent to including a CONFIG keyword in a map file."
  void setConfigOption(char *key, char *value) {
    msSetConfigOption(self,key,value);
  }

  %feature("docstring")
  "Fetches the value of the requested configuration key if set. Returns NULL if the key is not set."
  char *getConfigOption(char *key) {
    return (char *) msGetConfigOption(self,key);
  }

  %feature("docstring")
  "Apply the defined configuration options set by setConfigOption()"
  void applyConfigOptions() {
    msApplyMapConfigOptions( self );
  } 

  /* SLD */

    %feature("docstring")
    "Parse the SLD XML string sldxml and apply to map layers. "
    "Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`"
    int applySLD(char *sld) {
      return msSLDApplySLD(self, sld, -1, NULL, NULL);
    }

    %feature("docstring")
    "Fetch SLD XML from the URL sldurl and apply to map layers."
    "Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`"
    int applySLDURL(char *sld) {
      return msSLDApplySLDURL(self, sld, -1, NULL, NULL);
    }

    %feature("docstring")
    "Return SLD XML as a string for map layers that have STATUS on."
    %newobject generateSLD;
    char *generateSLD(char *sldVersion=NULL) {
        return (char *) msSLDGenerateSLD(self, -1, sldVersion);
    }

    %feature("docstring")
    "Process MapServer template and return HTML."
    "Note None of the three template processing methods will be usable unless the proper "
    "typemaps are implemented in the module for the target language.Currently the typemaps "
    "are not implemented."
    %newobject processTemplate;
    char *processTemplate(int bGenerateImages, char **names, char **values,
                          int numentries)
    {
        return msProcessTemplate(self, bGenerateImages, names, values,
                                 numentries);
    }

    %feature("docstring")
    "Process MapServer legend template and return HTML."
    "Note None of the three template processing methods will be usable unless the proper "
    "typemaps are implemented in the module for the target language.Currently the typemaps "
    "are not implemented."
    %newobject processLegendTemplate;
    char *processLegendTemplate(char **names, char **values, int numentries) {
        return msProcessLegendTemplate(self, names, values, numentries);
    }

    %feature("docstring")
    "Process MapServer query template and return HTML. \n"
    "Note None of the three template processing methods will be usable unless the proper "
    "typemaps are implemented in the module for the target language.Currently the typemaps "
    "are not implemented."
    %newobject processQueryTemplate;
    char *processQueryTemplate(char **names, char **values, int numentries) {
        return msProcessQueryTemplate(self, 1, names, values, numentries);
    }

    %feature("docstring")
    "Return the output format corresponding to driver name imagetype or to format name "
    "imagetype. This works exactly the same as the IMAGETYPE directive in a mapfile, is "
    "case insensitive and allows an output format to be found either by driver "
    "(like 'GD/PNG') or name (like 'PNG24')."
    outputFormatObj *getOutputFormatByName(char *name) {
        return msSelectOutputFormat(self, name); 
    }

    %feature("docstring")
    "Attach format to the map's output format list. Returns the updated number of output formats."
    int appendOutputFormat(outputFormatObj *format) {
        return msAppendOutputFormat(self, format);
    }

    %feature("docstring")
    "Removes the format named name from the map's output format list. "
    "Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`"
    int removeOutputFormat(char *name) {
        return msRemoveOutputFormat(self, name);
    }

    %feature("docstring")
    "Load OWS request parameters (BBOX, LAYERS, &c.) into map. "
    "Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`"
    int loadOWSParameters(cgiRequestObj *request, char *wmtver_string="1.1.1") 
    {
        return msMapLoadOWSParameters(self, request, wmtver_string);
    }

    %feature("docstring")
    "Processes and executes the passed OpenGIS Web Services request on the map. "
    "Returns :data:`MS_DONE` (2) if there is no valid OWS request in the req object, "
    ":data:`MS_SUCCESS` (0) if an OWS request was successfully processed and :data:`MS_FAILURE` (1) "
    "if an OWS request was not successfully processed. OWS requests include WMS, WFS, WCS "
    "and SOS requests supported by MapServer. Results of a dispatched request are written to "
    "stdout and can be captured using the msIO services (i.e. ``msIO_installStdoutToBuffer`` "
    "and ``msIO_getStdoutBufferString``)"
    int OWSDispatch( cgiRequestObj *req )
    {
        return msOWSDispatch( self, req, MS_TRUE );
    }

    %feature("docstring")
    "Saves the object to a string. Provides the inverse option for updateFromString." 
    %newobject convertToString;
    char* convertToString()
    {
        return msWriteMapToString(self);
    }

    %feature("docstring")
    "Apply any default values defined in a VALIDATION block used for runtime substitutions"
    void applyDefaultSubstitutions()
    {
        msApplyDefaultSubstitutions(self);
    }

    %feature("docstring")
    "Pass in runtime substitution keys and values and apply them to the map. "
    "**Note** This method is currently enabled for Python only. "
    "Typemaps are needed for other mapscript languages."
    void applySubstitutions(char **names, char **values, int npairs)
    {
        msApplySubstitutions(self, names, values, npairs);
    }
}
