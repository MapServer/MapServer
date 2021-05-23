/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript layerObj extensions
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

%extend layerObj 
{

    /**
    A :class:`layerObj` is associated with :class:`mapObj`. An instance of 
    :class:`layerObj` can exist outside of a :class:`mapObj`
    */
    layerObj(mapObj *map=NULL) 
    {
        layerObj *layer;
        int result;
        
        if (!map) {
            layer = (layerObj *) malloc(sizeof(layerObj));
            if (!layer) {
                msSetError(MS_MEMERR, "Failed to initialize Layer",
                                       "layerObj()");
                return NULL;
            } 
            result = initLayer(layer, NULL);
            if (result == MS_SUCCESS) {
                layer->index = -1;
                return layer;
            }
            else {
                msSetError(MS_MEMERR, "Failed to initialize Layer",
                                       "layerObj()");
                return NULL;
            }
        }
        else {
            if(msGrowMapLayers(map) == NULL)
                return(NULL);

            if (initLayer((map->layers[map->numlayers]), map) == -1)
                return(NULL);

            map->layers[map->numlayers]->index = map->numlayers;
            map->layerorder[map->numlayers] = map->numlayers;
            map->numlayers++;
        MS_REFCNT_INCR(map->layers[map->numlayers-1]);

            return (map->layers[map->numlayers-1]);
        }
    }
 
    /// Sets an opacity for the layer, where the value is an integer in range [0, 100]. 
    /// A new :ref:`composite` block is generated, containing this ``OPACITY`` value.
    void setOpacity(int opacity)
    {
      msSetLayerOpacity(self, opacity);
    }

    /// Returns the opacity value for the layer.
    int getOpacity() {
      if(self->compositer) return (self->compositer->opacity);
      return (100);
    }

    ~layerObj() 
    {
        /*if (!self->map) {*/
        if (self) {
            if(freeLayer(self)==MS_SUCCESS) {
                free(self);
        }
        }
    }

#if defined (SWIGJAVA) || defined(SWIGPHP)
    %newobject cloneLayer;
    layerObj *cloneLayer() 
#else
    %newobject clone;
    /**
    Return an independent copy of the layer with no parent map.
    */
    layerObj *clone() 
#endif
    {
        layerObj *layer;
        int result;

        layer = (layerObj *) malloc(sizeof(layerObj));
        if (!layer) {
            msSetError(MS_MEMERR, "Failed to initialize Layer",
                                  "layerObj()");
            return NULL;
        } 
        result = initLayer(layer, NULL);
        if (result != MS_SUCCESS) {
            msSetError(MS_MEMERR, "Failed to initialize Layer",
                                  "layerObj()");
            return NULL;
        }

        if (msCopyLayer(layer, self) != MS_SUCCESS) {
            freeLayer(layer);
            free(layer);
            layer = NULL;
        }
        layer->map = NULL;
        layer->index = -1;
        
        return layer;
    }

    /// Update a :class:`layerObj` from a string snippet. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int updateFromString(char *snippet)
    {
        return msUpdateLayerFromString(self, snippet, MS_FALSE);
    }
    
    %newobject convertToString;
    /// Output the :class:`layerObj` object as a Mapfile string. Provides the inverse option for :func:`layerObj.updateFromString`.
    char* convertToString()
    {
        return msWriteLayerToString(self);
    }

#ifdef SWIGCSHARP
%apply SWIGTYPE *SETREFERENCE {classObj *classobj};
#endif
    /// Insert a **copy** of the class into the layer at the requested *index*. 
    /// Default index of -1 means insertion at the end of the array of classes. Returns the index at which the class was inserted.
    int insertClass(classObj *classobj, int index=-1)
    {
        return msInsertClass(self, classobj, index);
    }
#ifdef SWIGCSHARP
%clear classObj *classobj;
#endif


    %newobject removeClass;
    /// Removes the class at *index* and returns a copy, or NULL in the case of a failure. 
    /// Note that subsequent classes will be renumbered by this operation. 
    /// The numclasses field contains the number of classes available.
    classObj *removeClass(int index) 
    {
        classObj* c = msRemoveClass(self, index);
    if (c != NULL) {
        MS_REFCNT_INCR(c);
    }
    return c;
    }

    /// Opens the underlying layer. This is required before operations like :func:`layerObj.getResult`
    /// will work, but is not required before a draw or query call.
    int open() 
    {
        int status;
        status =  msLayerOpen(self);
        if (status == MS_SUCCESS) {
            return msLayerGetItems(self);
        }
        return status;
    }

    /**
    Performs a spatial, and optionally an attribute based feature search. 
    The function basically prepares things so that candidate features can be accessed by 
    query or drawing functions (e.g using :func:`layerObj.nextShape` function). 
    Returns :data:`MS_SUCCESS`, :data:`MS_FAILURE` or :data:`MS_DONE`.
    MS_DONE is returned if the layer extent does not overlap *rect*.
    */
    int whichShapes(rectObj rect)
    {
        int oldconnectiontype = self->connectiontype;
        self->connectiontype = MS_INLINE;

        if(msLayerWhichItems(self, MS_TRUE, NULL) != MS_SUCCESS) {
            self->connectiontype = oldconnectiontype;
            return MS_FAILURE;
        }
        self->connectiontype = oldconnectiontype;

        return msLayerWhichShapes(self, rect, MS_FALSE);
    }    

    %newobject nextShape;
    /**
    Called after :func:`layerObj.whichShapes` has been called to actually 
    retrieve shapes within a given area returns a shape object or :data:`MS_FALSE`
    Example of usage:

    .. code-block::

        mapObj map = new mapObj("d:/msapps/gmap-ms40/htdocs/gmap75.map");
        layerObj layer = map.getLayerByName('road');
        int status = layer.open();
        status = layer.whichShapes(map.extent);
        shapeObj shape;
        while ((shape = layer.nextShape()) != null)
        {
          ...
        }
        layer.close();

    */
    shapeObj *nextShape() 
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

    /// Close the underlying layer.
    void close() 
    {
        msLayerClose(self);
    }

    %newobject getShape;
    /// Get a shape from layer data. Argument is a result cache member from :func:`layerObj.getResult`
    shapeObj *getShape(resultObj *record) 
    {
        int retval;
        shapeObj *shape;

        if (!record) return NULL;
    
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape) return NULL;

        msInitShape(shape);
        shape->type = self->type; /* is this right? */

        retval = msLayerGetShape(self, shape, record);
        if(retval != MS_SUCCESS) {
          msFreeShape(shape);
          free(shape);
          return NULL;
        } else
        return shape;
    }

    /// Returns the number of entries in the query result cache for this layer.
    int getNumResults() 
    {
        if (!self->resultcache) return 0;
        return self->resultcache->numresults;
    }

    %newobject getResultsBounds;
    /// Returns the bounds of the features in the result cache.
    rectObj *getResultsBounds() 
    {
        rectObj *bounds;
        if (!self->resultcache) return NULL;
        bounds = (rectObj *) malloc(sizeof(rectObj));
        MS_COPYRECT(bounds, &self->resultcache->bounds);
        return bounds;
    }

    /// Fetches the requested query result cache entry, or NULL if the index is outside 
    /// the range of available results. This method would normally only be used after 
    /// issuing a query operation.
    resultObj *getResult(int i) 
    {
        if (!self->resultcache) return NULL;
        if (i >= 0 && i < self->resultcache->numresults)
            return &self->resultcache->results[i]; 
        else
            return NULL;
    }

    %newobject getClass;
    /// Fetch the requested class object at *i*. Returns NULL if the class index is out of the legal range. 
    /// The numclasses field contains the number of classes available, and the first class is index 0.
    classObj *getClass(int i) 
    {
    classObj *result=NULL;
        if (i >= 0 && i < self->numclasses) {
            result=self->class[i]; 
        MS_REFCNT_INCR(result);
    }
    return result;
    }

    /// Returns the requested item. Items are attribute fields, and this method returns the 
    /// item name (field name). The numitems field contains the number of items available, and the first item is index zero.
    char *getItem(int i) 
    {
  
        if (i >= 0 && i < self->numitems)
            return (char *) (self->items[i]);
        else
            return NULL;
    }

    /// Set the items to be retrieved with a particular shape.
    int setItems(char **items, int numitems) {
        return msLayerSetItems(self, items, numitems);
    }

    /// Renders this layer into the target image, adding labels to the cache if required. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`. 
    // TODO: Does the map need to be the map on which the layer is defined? I suspect so.
    int draw(mapObj *map, imageObj *image) 
    {
        return msDrawLayer(map, self, image);    
    }

    /// Draw query map for a single layer into the target image. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`. 
    int drawQuery(mapObj *map, imageObj *image) 
    {
        return msDrawQueryLayer(map, self, image);    
    }

    /**
    Query by the filter *string*. 
    For querying, we switch layer status ON and then back to original
    value before returning. 
    */
    int queryByFilter(mapObj *map, char *string)
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

    /**
    Query layer for shapes that intersect current map extents. *qitem* is the item (attribute) 
    on which the query is performed, and *qstring* is the expression to match. 
    The query is performed on all the shapes that are part of a CLASS that contains a TEMPLATE value or that match any class in a layer that contains a LAYER TEMPLATE value.
    Note that the layer's FILTER/FILTERITEM are ignored by this function. *mode* is :data:`MS_SINGLE` or :data:`MS_MULTIPLE` depending on number of results you want. 
    Returns :data:`MS_SUCCESS` if shapes were found or :data:`MS_FAILURE` if nothing was found or if some other error happened.
    */
    int queryByAttributes(mapObj *map, char *qitem, char *qstring, int mode) 
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

    /**
    Query layer at point location specified in georeferenced map coordinates (i.e. not pixels). 
    The query is performed on all the shapes that are part of a CLASS that contains a TEMPLATE value or that match any class in a layer that contains a LAYER TEMPLATE value.
    Note that the layer's FILTER/FILTERITEM are ignored by this function. *mode* is :data:`MS_SINGLE` or :data:`MS_MULTIPLE` depending on number of results you want. 
    Returns :data:`MS_SUCCESS` if shapes were found or :data:`MS_FAILURE` if nothing was found or if some other error happened.
    */
    int queryByPoint(mapObj *map, pointObj *point, int mode, double buffer) 
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

    /**
    Query layer using a rectangle specified in georeferenced map coordinates (i.e. not pixels).
    The query is performed on all the shapes that are part of a CLASS that contains a TEMPLATE value or that match any class in a layer that contains a LAYER TEMPLATE value.
    Note that the layer's FILTER/FILTERITEM are ignored by this function. The :data:`MS_MULTIPLE` mode is set by default.
    Returns :data:`MS_SUCCESS` if shapes were found or :data:`MS_FAILURE` if nothing was found or if some other error happened.
    */
    int queryByRect(mapObj *map, rectObj rect) 
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

    /**
    Perform a query set based on a previous set of results from another layer. 
    At present the results MUST be based on a polygon layer. 
    Returns :data:`MS_SUCCESS` if shapes were found or :data:`MS_FAILURE` if nothing was found or if some other error happened.
    */
    int queryByFeatures(mapObj *map, int slayer) 
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

    /**
    Query layer based on a single shape, the shape has to be a polygon at this point. 
    Returns :data:`MS_SUCCESS` if shapes were found or :data:`MS_FAILURE` if nothing was found or if some other error happened.
    */
    int queryByShape(mapObj *map, shapeObj *shape) 
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

    /// Pop a query result member into the layer's result cache. By default clobbers existing cache.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int queryByIndex(mapObj *map, int tileindex, int shapeindex, int bAddToQuery=MS_FALSE)
    {
        int status;
        int retval;
        
        msInitQuery(&(map->query));

        map->query.type = MS_QUERY_BY_INDEX;
        map->query.mode = MS_QUERY_SINGLE;
        map->query.tileindex = tileindex;
        map->query.shapeindex = shapeindex;
        map->query.clear_resultcache = !bAddToQuery;
        map->query.layer = self->index;

        status = self->status;
        self->status = MS_ON;
        retval = msQueryByIndex(map);
        self->status = status;
        return retval;
    }
    
    /// Returns a reference to layer's result cache. Should be NULL prior to any query, or after a failed query or query with no results.
    resultCacheObj *getResults(void)
    {
        return self->resultcache;
    }

    /// Sets a filter expression similarly to the FILTER expression in a map file. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE` if the expression fails to parse.
    int setFilter(char *filter) 
    {
        if (!filter || strlen(filter) == 0) {
            msFreeExpression(&self->filter);
            return MS_SUCCESS;
        }
        else return msLoadExpressionString(&self->filter, filter);
    }

    %newobject getFilterString;
    /// Returns the current filter expression.
    char *getFilterString() 
    {
        return msGetExpressionString(&(self->filter));
    }

    /// Set the layer projection using OpenGIS Well Known Text format. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int setWKTProjection(char *wkt) 
    {
        self->project = MS_TRUE;
        return msOGCWKT2ProjectionObj(wkt, &(self->projection), self->debug);
    }

    %newobject getProjection;
    /// Returns the PROJ definition of the layer's projection.
    char *getProjection() 
    {    
        return (char *) msGetProjectionString(&(self->projection));
    }

    /**
    Set the layer projection using a PROJ format projection definition 
    (i.e. "+proj=utm +zone=11 +datum=WGS84" or "init=EPSG:26911"). 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int setProjection(char *proj4) 
    {
        self->project = MS_TRUE;
        return msLoadProjectionString(&(self->projection), proj4);
    }

    /// Add a new inline feature on a layer. Returns -1 on error. 
    /// TODO: Is this similar to inline features in a mapfile? Does it work for any kind of layer or connection type?
    int addFeature(shapeObj *shape) 
    {    
        self->connectiontype = MS_INLINE;
        if(self->features != NULL && self->features->tailifhead != NULL) 
            shape->index = self->features->tailifhead->shape.index + 1;
        else 
            shape->index = 0;
        if (insertFeatureList(&(self->features), shape) == NULL) 
        return MS_FAILURE;
        return MS_SUCCESS;
    }

    /// Returns the number of inline features in a layer. 
    /// TODO: is this really only online features or will it return the number of non-inline features on a regular layer?
    int getNumFeatures() 
    {
        return msLayerGetNumFeatures(self);
    }

    %newobject getExtent;
    /// Fetches the extents of the data in the layer. This normally requires a full read pass through the features of the layer and does not work for raster layers.
    rectObj *getExtent() 
    {
        rectObj *extent;
        extent = (rectObj *) malloc(sizeof(rectObj));
        msLayerGetExtent(self, extent);
        return extent;
    }

    /// Sets the extent of a layer. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int setExtent(double minx=-1.0, double miny=-1.0,
                  double maxx=-1.0, double maxy=-1.0)
    {
        if (minx > maxx || miny > maxy) {
            msSetError(MS_RECTERR,
                "{ 'minx': %f , 'miny': %f , 'maxx': %f , 'maxy': %f }",
                "layerObj::setExtent()", minx, miny, maxx, maxy);
            return MS_FAILURE;
        }

        return msLayerSetExtent(self, minx, miny, maxx, maxy);
    }

    %newobject getWMSFeatureInfoURL;
    /**
    Return a WMS GetFeatureInfo URL (works only for WMS layers) *clickX*, *clickY*
    is the location of to query in pixel coordinates with (0,0) at the top 
    left of the image. *featureCount* is the number of results to return. *infoFormat* 
    is the format the format in which the result should be requested. 
    Depends on remote server's capabilities. MapServer WMS servers support only "MIME" 
    (and should support "GML.1" soon). Returns "" and outputs a warning if layer is not a 
    WMS layer or if it is not queryable.
    */
    char *getWMSFeatureInfoURL(mapObj *map, int click_x, int click_y,
                               int feature_count, char *info_format)
    {
        return (char *) msWMSGetFeatureInfoURL(map, self, click_x, click_y,
               feature_count, info_format);
    }
 
    %newobject executeWFSGetFeature;
    /// Executes a GetFeature request on a WFS layer and returns the name of the temporary GML file created. Returns an empty string on error.
    char *executeWFSGetFeature(layerObj *layer) 
    {
        return (char *) msWFSExecuteGetFeature(layer);
    }

    /**
    Apply the SLD document to the layer object. The matching between the SLD document and the 
    layer will be done using the layer's name. If a *stylelayer* argument is passed (argument is optional), 
    the NamedLayer in the SLD that matches it will be used to style the layer. 
    See SLD HOWTO for more information on the SLD support.
    */
    int applySLD(char *sld, char *stylelayer) 
    {
      return msSLDApplySLD(self->map, sld, self->index, stylelayer, NULL);
    }

    /**
    Apply the SLD document pointed by the URL to the layer object. The matching between the SLD document and the 
    layer will be done using the layer's name. If a *stylelayer* argument is passed (argument is optional), 
    the NamedLayer in the SLD that matches it will be used to style the layer. 
    See SLD HOWTO for more information on the SLD support.
    */
    int applySLDURL(char *sld, char *stylelayer) 
    {
      return msSLDApplySLDURL(self->map, sld, self->index, stylelayer, NULL);
    }

    %newobject generateSLD; 
    /// Returns an SLD XML string based on all the classes found in the layer (the layer must have ``STATUS ON```).
    char *generateSLD() 
    {
        return (char *) msSLDGenerateSLD(self->map, self->index, NULL);
    }

    /// Returns :data:`MS_TRUE` or :data:`MS_FALSE` after considering the layer status, 
    /// minscaledenom, and maxscaledenom within the context of the parent map.
    int isVisible()
    {
        if (!self->map)
        {
            msSetError(MS_MISCERR,
                "visibility has no meaning outside of a map context",
                "isVisible()");
            return MS_FAILURE;
        }
        return msLayerIsVisible(self->map, self);
    }

    /**
    The class specified by the class index will be moved up in the array of classes. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`. For example ``moveClassUp(1)`` will have the effect 
    of moving class 1 up to position 0, and the class at position 0 will be moved to position 1.
    */
    int moveClassUp(int index) 
    {
        return msMoveClassUp(self, index);
    }

    /**
    The class specified by the class index will be moved up into the array of classes. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`. For example ``moveClassDown(1)`` will have the effect 
    of moving class 1 down to position 2, and the class at position 2 will be moved to position 1.
    */
    int moveClassDown(int index) 
    {
        return msMoveClassDown(self, index);
    }

    /**
    Adds or replaces a processing directive of the form "key=value". 
    Unlike the :func:`layerObj.addProcessing` function, this will replace an existing processing directive 
    for the given key value. Processing directives supported are specific to the layer type 
    and underlying renderer.
    */
    void setProcessingKey(const char *key, const char *value) 
    {
       msLayerSetProcessingKey( self, key, value );
    }
 
    /// Adds a new processing directive line to a layer, similar to the PROCESSING directive 
    /// in a map file. Processing directives supported are specific to the layer type and 
    /// underlying renderer.
    void addProcessing(const char *directive ) 
    {
        msLayerAddProcessing( self, directive );
    }

    /// Return the raster processing directive at *index*.
    char *getProcessing(int index) 
    {
        return (char *) msLayerGetProcessing(self, index);
    }

    /// Return the processing directive specified by *key*.
    char *getProcessingKey(const char *key)
    {
      return (char *) msLayerGetProcessingKey(self, key);
    }    

    /**
    Clears the layer's raster processing directives. 
    Returns the subsequent number of directives, which will equal :data:`MS_SUCCESS` if the 
    directives have been cleared.
    */
    int clearProcessing() 
    {
        return msLayerClearProcessing(self);
    }

    /**
    Changes the connectiontype of the layer and recreates the vtable according to the new 
    connection type. This method should be used instead of setting the *connectiontype*
    parameter directly. In case when the layer.connectiontype = :data:`MS_PLUGIN` the *library_str* 
    parameter should also be specified so as to select the library to load by MapServer. 
    For the other connection types this parameter is not used. 
    */
    int setConnectionType(int connectiontype, const char *library_str) 
    {    
        /* Caller is responsible to close previous layer correctly before
         * calling msConnectLayer() 
         */
        if (msLayerIsOpen(self))
          msLayerClose(self);
        return msConnectLayer(self, connectiontype, library_str);
    }

    /**
    Get the class index for a shape in the layer.
    */
    int getClassIndex(mapObj *map, shapeObj *shape, int *classgroup=NULL, int numclasses=0) {
        return msShapeGetClass(self, map, shape, classgroup, numclasses);
    }

    /**
    Return the geomtransform string for the layer. 
    */
    char *getGeomTransform() 
    {
      return self->_geomtransform.string;
    }

    /**
    Set the geomtransform string for the layer. 
    */
    void setGeomTransform(char *transform) 
    {
      msFree(self->_geomtransform.string);
      if (!transform || strlen(transform) > 0) {
        self->_geomtransform.string = msStrdup(transform);
        self->_geomtransform.type = MS_GEOMTRANSFORM_EXPRESSION;
      }
      else {
        self->_geomtransform.type = MS_GEOMTRANSFORM_NONE;
        self->_geomtransform.string = NULL;
      }
    }

    /**
    Returns the requested item's field type.
    A layer must be open to retrieve the item definition. 

    Pass in the attribute index to retrieve the type. The 
    layer's numitems property contains the number of items 
    available, and the first item is index zero.
    */
    char *getItemType(int i)
    {

        char *itemType = NULL;

        if (i >= 0 && i < self->numitems) {
            gmlItemListObj *item_list;
            item_list = msGMLGetItems(self, "G");
            if (item_list != NULL) {
                gmlItemObj *item = item_list->items + i;
                itemType = msStrdup(item->type);
                msGMLFreeItems(item_list); // destroy the original list
            }
        }

        return itemType;
    }
}
