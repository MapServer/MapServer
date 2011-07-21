
/* ===========================================================================
   $Id$
 
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

    ~layerObj() 
    {
        /*if (!self->map) {*/
        if (self) {
            if(freeLayer(self)==MS_SUCCESS) {
            	free(self);
	    }
        }
    }

#ifdef SWIGJAVA
    %newobject cloneLayer;
    layerObj *cloneLayer() 
#else
    %newobject clone;
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

    int updateFromString(char *snippet)
    {
        return msUpdateLayerFromString(self, snippet, MS_FALSE);
    }

#ifdef SWIGCSHARP   
%apply SWIGTYPE *SETREFERENCE {classObj *classobj};
#endif
    int insertClass(classObj *classobj, int index=-1)
    {
        return msInsertClass(self, classobj, index);
    }
#ifdef SWIGCSHARP
%clear classObj *classobj;
#endif
    
    /* removeClass() */
    %newobject removeClass;
    classObj *removeClass(int index) 
    {
        classObj* c = msRemoveClass(self, index);
	if (c != NULL) {
		MS_REFCNT_INCR(c);
	}
	return c;
    }

    int open() 
    {
        int status;
        status =  msLayerOpen(self);
        if (status == MS_SUCCESS) {
            return msLayerGetItems(self);
        }
        return status;
    }

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

    void close() 
    {
        msLayerClose(self);
    }

    %newobject getShape;
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
        return shape;
    }

    int getNumResults() 
    {
        if (!self->resultcache) return 0;
        return self->resultcache->numresults;
    }

    %newobject getResultsBounds;
    rectObj *getResultsBounds() 
    {
        rectObj *bounds;
        if (!self->resultcache) return NULL;
        bounds = (rectObj *) malloc(sizeof(rectObj));
        MS_COPYRECT(bounds, &self->resultcache->bounds);
        return bounds;
    }

    resultObj *getResult(int i) 
    {
        if (!self->resultcache) return NULL;
        if (i >= 0 && i < self->resultcache->numresults)
            return &self->resultcache->results[i]; 
        else
            return NULL;
    }

    %newobject getClass;
    classObj *getClass(int i) 
    {
	classObj *result=NULL;
        if (i >= 0 && i < self->numclasses) {
            result=self->class[i]; 
	    MS_REFCNT_INCR(result);
	}
	return result;
    }

    char *getItem(int i) 
    {
  
        if (i >= 0 && i < self->numitems)
            return (char *) (self->items[i]);
        else
            return NULL;
    }

    int setItems(char **items, int numitems) {
        msLayerSetItems(self, items, numitems);
    }

    int draw(mapObj *map, imageObj *image) 
    {
        return msDrawLayer(map, self, image);    
    }

    int drawQuery(mapObj *map, imageObj *image) 
    {
        return msDrawQueryLayer(map, self, image);    
    }

    /* For querying, we switch layer status ON and then back to original
       value before returning. */

    int queryByFilter(mapObj *map, char *string)
    {
        int status;
        int retval;

        msInitQuery(&(map->query));

        map->query.type = MS_QUERY_BY_FILTER;

        map->query.filter = (expressionObj *) malloc(sizeof(expressionObj));
        map->query.filter->string = strdup(string);
	map->query.filter->type = 2000; /* MS_EXPRESSION: lot's of conflicts in mapfile.h */

        map->query.layer = self->index;
     	map->query.rect = map->extent;

	status = self->status;
	self->status = MS_ON;
        retval = msQueryByFilter(map);
        self->status = status;
	return retval;
    }

    int queryByAttributes(mapObj *map, char *qitem, char *qstring, int mode) 
    {
        int status;
        int retval;

        msInitQuery(&(map->query));
        
        map->query.type = MS_QUERY_BY_ATTRIBUTE;
        map->query.mode = mode;
        if(qitem) map->query.item = strdup(qitem);
        if(qstring) map->query.str = strdup(qstring);
        map->query.layer = self->index;
        map->query.rect = map->extent;

        status = self->status;
        self->status = MS_ON;
        retval = msQueryByAttributes(map);
        self->status = status;
        return retval;
    }

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
    
    resultCacheObj *getResults(void)
    {
        return self->resultcache;
    }
        
    int setFilter(char *filter) 
    {
        if (!filter || strlen(filter) == 0) {
            freeExpression(&self->filter);
            return MS_SUCCESS;
        }
        else return msLoadExpressionString(&self->filter, filter);
    }

    %newobject getFilterString;
    char *getFilterString() 
    {
        return msGetExpressionString(&(self->filter));
    }

    int setWKTProjection(char *wkt) 
    {
        self->project = MS_TRUE;
        return msOGCWKT2ProjectionObj(wkt, &(self->projection), self->debug);
    }

    %newobject getProjection;
    char *getProjection() 
    {    
        return (char *) msGetProjectionString(&(self->projection));
    }

    int setProjection(char *proj4) 
    {
        self->project = MS_TRUE;
        return msLoadProjectionString(&(self->projection), proj4);
    }

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

    /*
    Returns the number of inline feature of a layer
    */
    int getNumFeatures() 
    {
        return msLayerGetNumFeatures(self);
    }

    %newobject getExtent;
    rectObj *getExtent() 
    {
        rectObj *extent;
        extent = (rectObj *) malloc(sizeof(rectObj));
        msLayerGetExtent(self, extent);
        return extent;
    }

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
    
    /* 
    The following metadata methods are no longer needed since we have
    promoted the metadata member of layerObj to a first-class mapscript
    object.  See hashtable.i.  Not yet scheduled for deprecation but 
    perhaps in the next major release?  --SG
    */ 
    char *getMetaData(char *name) 
    {
        char *value = NULL;
        if (!name) {
            msSetError(MS_HASHERR, "NULL key", "getMetaData");
        }
     
        value = (char *) msLookupHashTable(&(self->metadata), name);
	/*
	Umberto, 05/17/2006
	Exceptions should be reserved for situations when a serious error occurred
	and normal program flow must be interrupted.
	In this case returning null should be more that enough.
	*/
#ifndef SWIGJAVA
        if (!value) {
            msSetError(MS_HASHERR, "Key %s does not exist", "getMetaData", name);
            return NULL;
        }
#endif
        return value;
    }

    int setMetaData(char *name, char *value) 
    {
        if (msInsertHashTable(&(self->metadata), name, value) == NULL)
        return MS_FAILURE;
        return MS_SUCCESS;
    }

    int removeMetaData(char *name) 
    {
        return(msRemoveHashTable(&(self->metadata), name));
    }

    char *getFirstMetaDataKey() 
    {
        return (char *) msFirstKeyFromHashTable(&(self->metadata));
    }
 
    char *getNextMetaDataKey(char *lastkey) 
    {
        return (char *) msNextKeyFromHashTable(&(self->metadata), lastkey);
    }
  
    %newobject getWMSFeatureInfoURL;
    char *getWMSFeatureInfoURL(mapObj *map, int click_x, int click_y,
                               int feature_count, char *info_format)
    {
        return (char *) msWMSGetFeatureInfoURL(map, self, click_x, click_y,
               feature_count, info_format);
    }
 
    %newobject executeWFSGetFeature;
    char *executeWFSGetFeature(layerObj *layer) 
    {
        return (char *) msWFSExecuteGetFeature(layer);
    }

    int applySLD(char *sld, char *stylelayer) 
    {
      return msSLDApplySLD(self->map, sld, self->index, stylelayer, NULL);
    }

    int applySLDURL(char *sld, char *stylelayer) 
    {
      return msSLDApplySLDURL(self->map, sld, self->index, stylelayer, NULL);
    }

    %newobject generateSLD; 
    char *generateSLD() 
    {
        return (char *) msSLDGenerateSLD(self->map, self->index, NULL);
    }

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

    int moveClassUp(int index) 
    {
        return msMoveClassUp(self, index);
    }

    int moveClassDown(int index) 
    {
        return msMoveClassDown(self, index);
    }

    void setProcessingKey(const char *key, const char *value) 
    {
	   msLayerSetProcessingKey( self, key, value );
    }
 
    /* this method is deprecated ... should use addProcessing() */
    void setProcessing(const char *directive ) 
    {
        msLayerAddProcessing( self, directive );
    }

    void addProcessing(const char *directive ) 
    {
        msLayerAddProcessing( self, directive );
    }

    char *getProcessing(int index) 
    {
        return (char *) msLayerGetProcessing(self, index);
    }

    char *getProcessingKey(const char *key)
    {
      return (char *) msLayerGetProcessingKey(self, key);
    }    

    int clearProcessing() 
    {
        return msLayerClearProcessing(self);
    }
    
    int setConnectionType(int connectiontype, const char *library_str) 
    {    
        /* Caller is responsible to close previous layer correctly before
         * calling msConnectLayer() 
         */
        if (msLayerIsOpen(self))
          msLayerClose(self);
        return msConnectLayer(self, connectiontype, library_str);
    }

    int getClassIndex(mapObj *map, shapeObj *shape, int *classgroup=NULL, int numclasses=0) {
        return msShapeGetClass(self, map, shape, classgroup, numclasses);
    }

}
