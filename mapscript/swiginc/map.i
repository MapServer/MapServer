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

    /// Create a new instance of :class:`mapObj`. Note that the *filename* is optional.
    mapObj(char *filename="", configObj *config=NULL) 
    {
        if (filename && strlen(filename))
            return msLoadMap(filename, NULL, config);
        else { /* create an empty map, no layers etc... */
            return msNewMapObj();
        }      
    }

#ifdef SWIGCSHARP

  mapObj(char *filename) 
  {
        if (filename && strlen(filename))
            return msLoadMap(filename, NULL, NULL);
        else { /* create an empty map, no layers etc... */
            return msNewMapObj();
        } 
  }

  mapObj(char *mapText, int isMapText /*used as signature only to differentiate this constructor from default constructor*/ ) 
  {
      return msLoadMapFromString(mapText, NULL, NULL);
  }

  mapObj(char *mapText, int isMapText, configObj *config) 
  {
      return msLoadMapFromString(mapText, NULL, config);
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
    /**
    Return an independent copy of the map, less any caches.

    .. note::

        In the Java & PHP modules this method is named ``cloneMap``.
    */
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
    /**
    Insert a copy of *layer* into the map at *index*.
    The default value of *index* is -1, which means the last possible index. 
    Returns the index of the new layer, or -1 in the case of a failure.
    */
    int insertLayer(layerObj *layer, int index=-1) 
    {
        return msInsertLayer(self, layer, index);  
    }
#ifdef SWIGCSHARP
%clear layerObj *layer;
#endif

    %newobject removeLayer;
    /// Remove the layer at *index*. Returns the :class:`layerObj`. 
    layerObj *removeLayer(int index) 
    {
        layerObj *layer=msRemoveLayer(self, index);
        MS_REFCNT_INCR(layer);
        return layer;
    }

    /**
    Set the map extent, returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`. 
    This method will correct the extents (width/height ratio) before setting the 
    minx, miny, maxx, maxy values. See extent properties to set up a custom extent from :class:`rectObj`.
    */
    int setExtent(double minx, double miny, double maxx, double maxy) {
        return msMapSetExtent( self, minx, miny, maxx, maxy );
    }

    /// Offset the map extent based on the given distances in map coordinates. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    int offsetExtent(double x, double y) {
        return msMapOffsetExtent( self, x, y );
    }

    /**
    Scale the map extent using the *zoomfactor* and ensure the extent within 
    the *minscaledenom* and *maxscaledenom* domain. If *minscaledenom* and/or *maxscaledenom*
    is 0 then the parameter is not taken into account. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int scaleExtent(double zoomfactor, double minscaledenom, double maxscaledenom) {
        return msMapScaleExtent( self, zoomfactor, minscaledenom, maxscaledenom );
    }

    /// Set the map center to the given map point. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    int setCenter(pointObj *center) {
        return msMapSetCenter( self, center );
    }

    /*
    Set map's image *width* and *height* together and carry out the necessary subsequent 
    geotransform computation. 

    Recent rotation work makes setSize the only reliable 
    method for changing the image size - direct access is deprecated. 

    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int setSize(int width, int height) 
    {
        return msMapSetSize(self, width, height);
    }

    /// TODO
    void pixelToGeoref(double pixPosX, double pixPosY, pointObj *geoPos)
    {
       geoPos->x = self->gt.geotransform[0] + self->gt.geotransform[1] * pixPosX + self->gt.geotransform[2] * pixPosY;
       geoPos->y = self->gt.geotransform[3] + self->gt.geotransform[4] * pixPosX + self->gt.geotransform[5] * pixPosY;
    }

    /// Returns the map rotation angle.
    double getRotation() 
    {
        return self->gt.rotation_angle;
    }

    /**
    Set map rotation angle. The map view rectangle (specified in EXTENTS) will be rotated by 
    the indicated angle in the counter- clockwise direction. Note that this implies the 
    rendered map will be rotated by the angle in the clockwise direction. 

    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int setRotation( double rotation_angle ) 
    {
        return msMapSetRotation( self, rotation_angle );
    }

    %newobject getLayer;
    /// Returns a reference to the layer at index *i*.
    layerObj *getLayer(int i) {
      if(i >= 0 && i < self->numlayers) {
          MS_REFCNT_INCR(self->layers[i]);
          return (self->layers[i]); /* returns an EXISTING layer */
      } else {
        return NULL;
      }
    }

    %newobject getLayerByName;
    /// Returns a reference to the named layer.
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

    /**
    Return the index of the named symbol in the map's symbolset.

    .. note::

        This method is poorly named and too indirect. It is preferable to use the 
        getSymbolByName method of :class:`symbolSetObj`, which really does return a :class:`symbolObj` 
        reference, or use the index method of symbolSetObj to get a symbol's index number.
    */
    int getSymbolByName(char *name) {
      return msGetSymbolIndex(&self->symbolset, name, MS_TRUE);
    }

    /// \**TODO** this function only calculates the scale or am I missing something?
    void prepareQuery() {
      int status;

      status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scaledenom);
      if(status != MS_SUCCESS) self->scaledenom = -1;
    }

    %newobject prepareImage;
    /// Returns an :class:`imageObj` initialized to map extents and outputformat.
    imageObj *prepareImage() 
    {
      return msPrepareImage(self, MS_FALSE);
    }

    /**
    Sets map outputformat to the named format.

    .. note::

        :func:`mapObj.setImageType` remains in the module but its use 
        is deprecated in favor of :func:`mapObj.selectOutputFormat`.
    */
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
            msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE);
        }
    }

    /**
    Set the map's active output format to the internal format named imagetype. 
    Built-in formats are "PNG", "PNG24", "JPEG", "GIF", "GTIFF".
    */
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
            msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE);
        }
    }

 
    %newobject getOutputFormat;
    /**
    Returns the output format at the specified *i* index from the output formats array or 
    null if *i* is beyond the array bounds. The number of outpuFormats can be 
    retrieved by calling :func:`mapObj.getNumoutputformats`.
    */
    outputFormatObj *getOutputFormat(int i) {
      if(i >= 0 && i < self->numoutputformats) {
          MS_REFCNT_INCR(self->outputformatlist[i]);
          return (self->outputformatlist[i]); 
      } else {
        return NULL;
      }
    }

    /// Sets map outputformat.
    void setOutputFormat( outputFormatObj *format ) {
        msApplyOutputFormat( &(self->outputformat), format, MS_NOOVERRIDE);
    }

    %newobject draw;
    /**
    Draw the map, processing layers according to their defined order and status. 
    Return an :class:`imageObj`.
    */
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

    %newobject drawQuery;
    /// Draw query map, returning an :class:`imageObj`.
    imageObj *drawQuery() {
      return msDrawMap(self, MS_TRUE);
    }

    %newobject drawLegend;
    /// Draw map legend, returning an :class:`imageObj`.
    imageObj *drawLegend(int scale_independent=MS_FALSE) {
      return msDrawLegend(self, scale_independent, NULL);
    }

    %newobject drawScalebar;
    /// Draw scale bar, returning an :class:`imageObj`.
    imageObj *drawScalebar() {
      return msDrawScalebar(self);
    }

    %newobject drawReferenceMap;
    /// Draw reference map, returning an :class:`imageObj`.
    imageObj *drawReferenceMap() {
      return msDrawReferenceMap(self);
    }

    /**
    Embed map's legend in *image*. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    */
    int embedScalebar(imageObj *image) {	
      return msEmbedScalebar(self, image);
    }

    /**
    Embed map's legend in *image*. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    */
    int embedLegend(imageObj *image) {	
      return msEmbedLegend(self, image);
    }

    /**
    Draw map's label cache on *image*.
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    */
    int drawLabelCache(imageObj *image) {
      return msDrawLabelCache(self,image);
    }

    /**
    Query map layers using the filter *string*. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    */
    int queryByFilter(char *string) {
      msInitQuery(&(self->query));

      self->query.type = MS_QUERY_BY_FILTER;
      self->query.mode = MS_QUERY_MULTIPLE;

      self->query.filter.string = msStrdup(string);
      self->query.filter.type = MS_EXPRESSION;

      self->query.rect = self->extent;

      return msQueryByFilter(self);
    }

    /**
    Query map layers, result sets contain one or more features, depending on mode, 
    that intersect *point* within a tolerance *buffer*. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    */
    int queryByPoint(pointObj *point, int mode, double buffer) {
      msInitQuery(&(self->query));

      self->query.type = MS_QUERY_BY_POINT;
      self->query.mode = mode;
      self->query.point = *point;
      self->query.buffer = buffer;

      return msQueryByPoint(self);
    }

    /**
    Query map layers, result sets contain features that intersect or are contained within 
    the features in the result set of the :data:`MS_LAYER_POLYGON` type layer at layer index *slayer*. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    */
    int queryByRect(rectObj rect) {
      msInitQuery(&(self->query));

      self->query.type = MS_QUERY_BY_RECT;
      self->query.mode = MS_QUERY_MULTIPLE;
      self->query.rect = rect;

      return msQueryByRect(self);
    }


    /**
    Query map layers, result sets contain features that intersect or are contained within 
    the features in the result set of the :data:`MS_LAYER_POLYGON` type layer at layer index *slayer*. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`.
    */
    int queryByFeatures(int slayer) {
      self->query.slayer = slayer;
      return msQueryByFeatures(self);
    }

    /**
    Query layer based on a single shape, the shape has to be a polygon at this point. 
    Returns :data:`MS_SUCCESS` if shapes were found or :data:`MS_FAILURE` if nothing was found 
    or if some other error happened
    */
    int queryByShape(shapeObj *shape) {
      msInitQuery(&(self->query));
      
      self->query.type = MS_QUERY_BY_SHAPE;
      self->query.mode = MS_QUERY_MULTIPLE;
      self->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
      msInitShape(self->query.shape);
      msCopyShape(shape, self->query.shape);

      return msQueryByShape(self);
    }


    /// Sets map projection from OGC definition *wkt*.
    int setWKTProjection(char *wkt) {
      return msOGCWKT2ProjectionObj(wkt, &(self->projection), self->debug);
    }

    %newobject getProjection;
    /// Returns the PROJ definition of the map's projection.
    char *getProjection() {
      return msGetProjectionString(&(self->projection));
    }

    /// Set map projection from PROJ definition string proj4.
    int setProjection(char *proj4) {
      return msLoadProjectionString(&(self->projection), proj4);
    }

    /// Save map to disk as a new map file. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int save(char *filename) {
      return msSaveMap(self, filename);
    }

    /// Save query to disk. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int saveQuery(char *filename, int results=MS_FALSE) {
      return msSaveQuery(self, filename, results);
    }

    /// Load a saved query. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int loadQuery(char *filename)  {
      return msLoadQuery(self, filename);
    }

    /// Clear layer query result caches. Default is -1, or all layers.
    void freeQuery(int qlayer=-1) {
      msQueryFree(self, qlayer);
    }

    /// Save query to disk. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int saveQueryAsGML(char *filename, const char *ns="GOMF") {
      return msGMLWriteQuery(self, filename, ns);
    }

    /// Load symbols defined in filename into map symbolset. The existing symbolset is cleared. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int setSymbolSet(char *szFileName) {
      msFreeSymbolSet(&self->symbolset);
      msInitSymbolSet(&self->symbolset);

      self->symbolset.filename = msStrdup(szFileName);

      /* Symbolset shares same fontset as main mapfile */
      self->symbolset.fontset = &(self->fontset);

      return msLoadSymbolSet(&self->symbolset, self);
    }

    /// Return the number of symbols in map
    int getNumSymbols() {
      return self->symbolset.numsymbols;
    }

    ///. Load fonts defined in filename into map fontset. The existing fontset is cleared. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
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

    /// Save map definition to disk as OGC-compliant XML. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int saveMapContext(char *szFileName) {
      return msSaveMapContext(self, szFileName);
    }

    /// Load an OGC map context file to define extents and layers of a map
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int loadMapContext(char *szFileName, int useUniqueNames=MS_FALSE) {
      return msLoadMapContext(self, szFileName, useUniqueNames);
    }
   
    /// Move the layer at layerindex up in the drawing order array, meaning 
    /// that it is drawn earlier.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int  moveLayerUp(int layerindex) {
      return msMoveLayerUp(self, layerindex);
    }

    /// Move the layer at layerindex down in the drawing order array, meaning 
    /// that it is drawn later. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int  moveLayerDown(int layerindex) {
      return msMoveLayerDown(self, layerindex);
    }

    %newobject getLayersDrawingOrder;
    /// Returns an array of layer indexes in drawing order.
    /// Note Unless the proper typemap is implemented for the module's language 
    /// a user is more likely to get back an unusable SWIG pointer to the integer array.
    intarray *getLayersDrawingOrder() {
      int i;
      intarray *order;
      order = new_intarray(self->numlayers);
      for (i=0; i<self->numlayers; i++) {
          // see http://www.swig.org/Doc4.0/SWIGDocumentation.html#Library_carrays for %array_class interface
          order[i] = self->layerorder[i];
      }
      return order;
    }

    /// Set map layer drawing order.
    /// Note Unless the proper typemap is implemented for the module's language 
    /// users will not be able to pass arrays or lists to this method and it will be unusable.
    int setLayersDrawingOrder(int *panIndexes) {
      return msSetLayersdrawingOrder(self, panIndexes); 
    }

    /// Set the indicated key configuration option to the indicated value. 
    /// Equivalent to including a CONFIG keyword in a map file.
    void setConfigOption(char *key, char *value) {
      msSetConfigOption(self,key,value);
    }

    /// Fetches the value of the requested configuration key if set. 
    /// Returns NULL if the key is not set.
    char *getConfigOption(char *key) {
      return (char *) msGetConfigOption(self,key);
    }

    /// Apply the defined configuration options set by setConfigOption()
    void applyConfigOptions() {
      msApplyMapConfigOptions( self );
    } 

    /* SLD */

    /// Parse the SLD XML string sldxml and apply to map layers. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int applySLD(char *sld) {
      return msSLDApplySLD(self, sld, -1, NULL, NULL);
    }

    /// Fetch SLD XML from the URL sldurl and apply to map layers.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int applySLDURL(char *sld) {
      return msSLDApplySLDURL(self, sld, -1, NULL, NULL);
    }

    %newobject generateSLD;
    /// Return SLD XML as a string for map layers that have STATUS on.
    char *generateSLD(char *sldVersion=NULL) {
        return (char *) msSLDGenerateSLD(self, -1, sldVersion);
    }

    %newobject processTemplate;
    /**
    Process MapServer template and return HTML.
    Note none of the three template processing methods will be usable unless the proper 
    typemaps are implemented in the module for the target language.Currently the typemaps 
    are not implemented.
    */
    char *processTemplate(int bGenerateImages, char **names, char **values,
                          int numentries)
    {
        return msProcessTemplate(self, bGenerateImages, names, values,
                                 numentries);
    }

    %newobject processLegendTemplate;
    /**
    Process MapServer legend template and return HTML.
    Note none of the three template processing methods will be usable unless the proper 
    typemaps are implemented in the module for the target language.Currently the typemaps 
    are not implemented.
    */
    char *processLegendTemplate(char **names, char **values, int numentries) {
        return msProcessLegendTemplate(self, names, values, numentries);
    }

    %newobject processQueryTemplate;
    /**
    Process MapServer query template and return HTML. 
    Note none of the three template processing methods will be usable unless the proper 
    typemaps are implemented in the module for the target language.Currently the typemaps 
    are not implemented.
    */
    char *processQueryTemplate(char **names, char **values, int numentries) {
        return msProcessQueryTemplate(self, 1, names, values, numentries);
    }

    /**
    Return the output format corresponding to driver name imagetype or to format name 
    imagetype. This works exactly the same as the IMAGETYPE directive in a mapfile, is 
    case insensitive and allows an output format to be found either by driver 
    (like 'AGG/PNG') or name (like 'png').
    */
    outputFormatObj *getOutputFormatByName(char *name) {
        return msSelectOutputFormat(self, name); 
    }

    /// Attach format to the map's output format list. 
    /// Returns the updated number of output formats.
    int appendOutputFormat(outputFormatObj *format) {
        return msAppendOutputFormat(self, format);
    }

    /// Removes the format named name from the map's output format list. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int removeOutputFormat(char *name) {
        return msRemoveOutputFormat(self, name);
    }

    /// Load OWS request parameters (BBOX, LAYERS, &c.) into map. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int loadOWSParameters(cgiRequestObj *request, char *wmtver_string="1.1.1") 
    {
        return msMapLoadOWSParameters(self, request, wmtver_string);
    }

    /**
    Processes and executes the passed OpenGIS Web Services request on the map. 
    Returns :data:`MS_DONE` (2) if there is no valid OWS request in the req object, 
    :data:`MS_SUCCESS` (0) if an OWS request was successfully processed and :data:`MS_FAILURE` (1) 
    if an OWS request was not successfully processed. OWS requests include WMS, WFS, WCS 
    and SOS requests supported by MapServer. Results of a dispatched request are written to 
    stdout and can be captured using the msIO services (i.e. :func:`mapscript.msIO_installStdoutToBuffer` 
    and :func:`mapscript.msIO_getStdoutBufferString()`
    */
    int OWSDispatch( cgiRequestObj *req )
    {
        return msOWSDispatch( self, req, MS_TRUE );
    }

    %newobject convertToString;
    /// Saves the object to a string. Provides the inverse option for updateFromString.
    char* convertToString()
    {
        return msWriteMapToString(self);
    }

    /// Apply any default values defined in a VALIDATION block used for runtime substitutions
    void applyDefaultSubstitutions()
    {
        msApplyDefaultSubstitutions(self);
    }

    /**
    Pass in runtime substitution keys and values and apply them to the map. 
    Note - this method is currently enabled for Python only. 
    Typemaps are needed for other MapScript languages.
    */
    void applySubstitutions(char **names, char **values, int npairs)
    {
        msApplySubstitutions(self, names, values, npairs);
    }
}
