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

    %newobject clone;
    mapObj *clone() 
    {
        mapObj *dstMap;
        dstMap = msNewMapObj();
        if (msCopyMap(dstMap, self) != MS_SUCCESS) {
            msFreeMap(dstMap);
            dstMap = NULL;
        }
        return dstMap;
    }

    int insertLayer(layerObj *layer, int index=-1) 
    {
        return msInsertLayer(self, layer, index);  
    }
    
    %newobject removeLayer;
    layerObj *removeLayer(int index) 
    {
        return msRemoveLayer(self, index);
    }
    
    int setExtent(double minx, double miny, double maxx, double maxy) {	
	return msMapSetExtent( self, minx, miny, maxx, maxy );
    }

    int setRotation( double rotation_angle ) {
        return msMapSetRotation( self, rotation_angle );
    }
 
  /* removeLayer() adjusts the layers array, the indices of
   * the remaining layers, the layersdrawing order, and numlayers
   */
  /*int removeLayer(int index) {
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
  }*/

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
    return msGetSymbolIndex(&self->symbolset, name, MS_TRUE);
  }

  void prepareQuery() {
    int status;

    status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scale);
    if(status != MS_SUCCESS) self->scale = -1; // degenerate extents ok here
  }

  %newobject prepareImage;
  imageObj *prepareImage() 
  {
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
                              self->web.imagepath, self->web.imageurl, self);
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
    //if (!&(self->web.metadata))
    //    self->web.metadata = msCreateHashTable();
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
        return msSLDApplySLD(self, sld, -1, NULL);
    }

    int applySLDURL(char *sld) {
        return msSLDApplySLDURL(self, sld, -1, NULL);
    }
    
    %newobject generateSLD;
    char *generateSLD() {
        return (char *) msSLDGenerateSLD(self, -1);
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

    // wrapper for msOWSDispatch
    int loadOWSParameters(cgiRequestObj *request, char *wmtver_string="1.1.1") 
    {
        int wmtver;
        wmtver = msOWSParseVersionString(wmtver_string);
        return msWMSLoadGetMapParams(self, wmtver, request->ParamNames,
                                     request->ParamValues, request->NumParams);
    }
}
