/*
 * mapscript_i.c: interface file for MapServer PHP scripting extension 
 *                called MapScript.
 *
 * This file was originally based on the SWIG interface file mapscript.i
 *
 * $Id$
 *
 * $Log$
 * Revision 1.61  2003/03/24 15:31:14  attila
 * mapimage module can now generate DXF imagemaps
 *
 * Revision 1.60  2003/02/24 02:19:43  dan
 * Added map->clone() method
 *
 * Revision 1.59  2003/02/21 16:55:12  assefa
 * Add function querybyindex and freequery.
 *
 * Revision 1.58  2003/02/14 20:17:27  assefa
 * Add savequery and loadquery functions.
 *
 * Revision 1.57  2003/02/11 19:01:08  assefa
 * Distnace points functions have changed names.
 *
 * Revision 1.56  2003/01/11 00:06:40  dan
 * Added setWKTProjection() to mapObj and layerObj
 *
 * Revision 1.55  2003/01/08 15:00:16  assefa
 * Add setsymbolbyname in the style class.
 *
 * Revision 1.54  2002/12/24 03:26:24  dan
 * Make msCreateLegendIcon() return imageObj + properly handle format (bug 227)
 *
 * Revision 1.53  2002/12/20 21:40:46  julien
 * Create default output format even if format are specified in mapfile
 *
 * Revision 1.52  2002/12/12 19:30:49  assefa
 * Correct call to msDrawShape function.
 *
 * Revision 1.51  2002/10/28 20:31:21  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.3  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
 * Revision 1.50  2002/10/24 18:10:08  assefa
 * Add map argument to initLayer function.
 *
 * Revision 1.49  2002/10/23 19:44:08  assefa
 * Add setcolor functions for style and label objects.
 * Add function to select the output format.
 * Correct PrepareImage and PasteImage functions.
 *
 * Revision 1.48  2002/09/17 13:08:30  julien
 * Remove all chdir() function and replace them with the new msBuildPath 
 * function.
 * This have been done to make MapServer thread safe. (Bug 152)
 *
 * Revision 1.47  2002/08/09 22:55:38  assefa
 * Update code to be in sync with mapserver addition of Styles.
 *
 * Revision 1.46  2002/07/08 19:07:06  dan
 * Added map->setFontSet() to MapScript
 *
 * Revision 1.45  2002/06/11 23:47:11  assefa
 * Upgrade code to support new outputformat support.
 *
 * Revision 1.44  2002/05/31 16:09:10  assefa
 * Change call tp msTransformShapeToPixel.
 *
 * Revision 1.43  2002/05/10 19:16:29  dan
 * Added qitem,qstring args to PHP version of layer->queryByAttributes()
 *
 * Revision 1.42  2002/05/08 19:09:49  dan
 * Attempt at fixing class.createLegendIcon()
 *
 * Revision 1.41  2002/05/02 15:55:51  assefa
 * Adapt code to support imageObj.
 *
 * Revision 1.40  2002/04/23 15:40:21  dan
 * Call msGetSymbolIndex() directly in mapObj_getSymbolByName()
 *
 * Revision 1.39  2002/04/22 20:23:56  dan
 * Fixed map->setSymbolSet(): reference to map->fontset was left NULL and
 * TTF symbols were causing a crash.
 *
 * Revision 1.38  2002/04/22 19:31:57  dan
 * Added optional new_map_path arg to msLoadMap()
 *
 * Revision 1.37  2002/04/12 15:44:30  sacha
 * Change msGetSymbolIdByName by msGetSymbolIndex.
 *
 * Revision 1.36  2002/03/14 21:36:12  sacha
 * Add two mapscript function (in PHP and perl)
 * setSymbolSet(filename) that load a symbol file dynanictly
 * getNumSymbols() return the number of symbol in map.
 *
 * Revision 1.35  2002/03/07 22:31:01  assefa
 * Add template processing functions.
 *
 * Revision 1.34  2002/02/08 19:13:05  dan
 * Opps... I have deleted mapObj_getSymbolByName() by accident
 *
 * Revision 1.33  2002/02/08 18:51:11  dan
 * Remove class and layer args to setSymbolByName()
 *
 * Revision 1.32  2002/02/08 18:25:39  sacha
 * let mapserv add a new symbol when we use the classobj setproperty function
 * with "symbolname" and "overlaysymbolname" arg.
 *
 * Revision 1.31  2002/01/29 23:38:32  assefa
 * Add write support for measured shape files.
 *
 * Revision 1.30  2002/01/22 21:19:01  sacha
 * Add two functions in maplegend.c
 * - msDrawLegendIcon that draw an class legend icon over an existing image.
 * - msCreateLegendIcon that draw an class legend icon and return the newly
 * created image.
 * Also, an php examples in mapscript/php3/examples/test_draw_legend_icon.phtml
 *
 * Revision 1.29  2001/12/19 03:46:02  assefa
 * Support of Measured shpe files.
 *
 * Revision 1.28  2001/11/01 21:10:09  assefa
 * Add getProjection on map and layer object.
 *
 * Revision 1.27  2001/11/01 02:47:06  dan
 * Added layerObj->getWMSFeatureInfoURL()
 *
 * Revision 1.26  2001/10/23 19:17:38  assefa
 * Use layerorder instead of panPrioList.
 *
 * Revision 1.25  2001/10/23 01:32:46  assefa
 * Add Drawing Priority support.
 *
 * Revision 1.24  2001/10/03 12:41:04  assefa
 * Add function getLayersIndexByGroup.
 *
 * Revision 1.23  2001/08/29 14:36:06  dan
 * Changes to msCalculateScale() args.  Sync with mapscript.i v1.42
 *
 * Revision 1.22  2001/08/01 13:52:59  dan
 * Sync with mapscript.i v1.39: add QueryByAttributes() and take out type arg
 * to getSymbolByName().
 *
 * Revision 1.21  2001/07/26 19:50:08  assefa
 * Add projection class and related functions.
 *
 * Revision 1.20  2001/04/19 15:11:34  dan
 * Sync with mapscript.i v.1.32
 *
 * Revision 1.19  2001/03/30 04:16:14  dan
 * Removed shapepath parameter to layer->getshape()
 *
 * Revision 1.18  2001/03/28 02:10:15  assefa
 * Change loadProjectionString to msLoadProjectionString
 *
 * Revision 1.17  2001/03/21 21:55:28  dan
 * Added get/setMetaData() for layerObj and mapObj()
 *
 * Revision 1.16  2001/03/21 17:43:33  dan
 * Added layer->class->type ... in sync with mapscript.i v1.29
 *
 * Revision 1.15  2001/03/16 22:08:36  dan
 * Removed allitems param to msLayerGetShape()
 *
 * Revision 1.14  2001/03/12 19:02:46  dan
 * Added query-related stuff in PHP MapScript
 *
 * Revision 1.13  2001/03/09 19:33:13  dan
 * Updated PHP MapScript... still a few methods missing, and needs testing.
 *
 * Revision 1.12  2001/02/23 21:58:00  dan
 * PHP MapScript working with new 3.5 stuff, but query stuff is disabled
 *
 * Revision 1.11  2000/11/01 16:31:07  dan
 * Add missing functions (in sync with mapscript).
 *
 * Revision 1.10  2000/09/25 22:48:54  dan
 * Aded update access for msOpenShapeFile()  (sync with mapscript.i v1.19)
 *
 * Revision 1.9  2000/09/22 13:56:10  dan
 * Added msInitShape() in rectObj_draw()
 *
 * Revision 1.8  2000/09/13 21:04:34  dan
 * Use msInitShape() in shapeObj constructor
 *
 * Revision 1.7  2000/09/07 20:18:20  dan
 * Sync with mapscript.i version 1.16
 *
 * Revision 1.6  2000/08/24 05:46:22  dan
 * #ifdef everything related to featureObj
 *
 * Revision 1.5  2000/08/16 21:14:00  dan
 * Sync with mapscript.i version 1.12
 *
 * Revision 1.4  2000/07/12 20:19:30  dan
 * Sync with mapscript.i version 1.10
 *
 * Revision 1.3  2000/06/28 20:22:02  dan
 * Sync with mapscript.i version 1.9
 *
 * Revision 1.2  2000/05/23 20:46:45  dan
 * Sync with mapscript.i version 1.5
 *
 * Revision 1.1  2000/05/09 21:06:11  dan
 * Initial Import
 *
 * Revision 1.6  2000/04/26 16:10:02  daniel
 * Changes to build with ms_3.3.010
 *
 * Revision 1.5  2000/03/17 19:03:07  daniel
 * Update to 3.3.008: removed prototypes for addFeature() and initFeature()
 *
 * Revision 1.4  2000/03/15 00:36:31  daniel
 * Finished conversion of all cover functions to real C
 *
 * Revision 1.3  2000/03/11 21:53:27  daniel
 * Ported extension to MapServer version 3.3.008
 *
 */


#include "php_mapscript.h"

/* grab mapserver declarations to wrap
 */
#include "../../map.h"
#include "../../maperror.h"
#include "../../mapprimitive.h"
#include "../../mapsymbol.h"
#include "../../mapshape.h"
#include "../../mapproject.h"

/**********************************************************************
 * class extensions for mapObj
 **********************************************************************/
mapObj *mapObj_new(char *filename, char *new_path) {
    if(filename && strlen(filename))
      return msLoadMap(filename, new_path);
    else { /* create an empty map, no layers etc... */
      return msNewMapObj();
    } 
}

void  mapObj_destroy(mapObj* self) {
    msFreeMap(self);
  }

mapObj *mapObj_clone(mapObj* self) {
    mapObj *dstMap;
    dstMap = msNewMapObj();
    if (msCopyMap(dstMap, self) != MS_SUCCESS)
    {
        msFreeMap(dstMap);
        dstMap = NULL;
    }
    return dstMap;
  }


layerObj *mapObj_getLayer(mapObj* self, int i) {
    if(i >= 0 && i < self->numlayers)	
      return &(self->layers[i]); /* returns an EXISTING layer */
    else
      return NULL;
  }

layerObj *mapObj_getLayerByName(mapObj* self, char *name) {
    int i;

    i = msGetLayerIndex(self, name);

    if(i != -1)
      return &(self->layers[i]); /* returns an EXISTING layer */
    else
      return NULL;
  }

int *mapObj_getLayersIndexByGroup(mapObj* self, char *groupname, 
                                 int *pnCount) {
    return msGetLayersIndexByGroup(self, groupname, pnCount);
}


int mapObj_getSymbolByName(mapObj* self, char *name) {
    return msGetSymbolIndex(&self->symbolset, name);
  }

void mapObj_prepareQuery(mapObj* self) {
    int status;

    status = msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &self->scale);
    if(status != MS_SUCCESS) self->scale = -1; // degenerate extents ok here
  }

imageObj *mapObj_prepareImage(mapObj* self) {
    int status;
    imageObj *img = NULL;

    if(self->width == -1 && self->height == -1) {
      msSetError(MS_MISCERR, "Image dimensions not specified.", "prepareImage()");
      return NULL;
    }

    if(self->width == -1 ||  self->height == -1)
      if(msAdjustImage(self->extent, &self->width, &self->height) == -1)
        return NULL;

    if( MS_RENDERER_GD(self->outputformat) )
    {
        img = msImageCreateGD(self->width, self->height, self->outputformat, 
				self->web.imagepath, self->web.imageurl);        
        if( img != NULL ) msImageInitGD( img, &self->imagecolor );
    }
    else if( MS_RENDERER_RAWDATA(self->outputformat) )
    {
        img = msImageCreate(self->width, self->height, self->outputformat,
                            self->web.imagepath, self->web.imageurl);
    }
#ifdef USE_MING_FLASH
    else if( MS_RENDERER_SWF(self->outputformat) )
    {
        img = msImageCreateSWF(self->width, self->height, self->outputformat,
                               self->web.imagepath, self->web.imageurl,
                               self);
    }
#endif
#ifdef USE_PDF
    else if( MS_RENDERER_PDF(self->outputformat) )
    {
        img = msImageCreatePDF(self->width, self->height, self->outputformat,
                               self->web.imagepath, self->web.imageurl,
                               self);
	}
#endif
    
    if(!img) {
      msSetError(MS_GDERR, "Unable to initialize image.", "prepareImage()");
      return NULL;
    }
  
   
    self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);
    status = msCalculateScale(self->extent, self->units, self->width, self->height, 
                              self->resolution, &self->scale);
    if(status != MS_SUCCESS)
      return NULL;

    return img;
  }

imageObj *mapObj_draw(mapObj* self) {
    return msDrawMap(self);
  }

imageObj *mapObj_drawQuery(mapObj* self) {
    return msDrawQueryMap(self);
  }

imageObj *mapObj_drawLegend(mapObj* self) {
    return msDrawLegend(self);
  }


imageObj *mapObj_drawScalebar(mapObj* self) {
    return msDrawScalebar(self);
  }

imageObj *mapObj_drawReferenceMap(mapObj* self) {
    return msDrawReferenceMap(self);
  }

//TODO
int mapObj_embedScalebar(mapObj* self, imageObj *img) {	
    return msEmbedScalebar(self, img->img.gd);
  }

//TODO
int mapObj_embedLegend(mapObj* self, imageObj *img) {	
    return msEmbedLegend(self, img->img.gd);
  }

int mapObj_drawLabelCache(mapObj* self, imageObj *img) {
    return msDrawLabelCache(img, self);
  }

labelCacheMemberObj *mapObj_nextLabel(mapObj* self) {
    static int i=0;

    if(i<self->labelcache.numlabels)
      return &(self->labelcache.labels[i++]);
    else
      return NULL;	
  }

int mapObj_queryByPoint(mapObj* self, pointObj *point, 
                         int mode, double buffer) {
    return msQueryByPoint(self, -1, mode, *point, buffer);
  }

int mapObj_queryByRect(mapObj* self, rectObj rect) {
    return msQueryByRect(self, -1, rect);
  }

int mapObj_queryByFeatures(mapObj* self, int slayer) {
    return msQueryByFeatures(self, -1, slayer);
  }

int mapObj_queryByShape(mapObj *self, shapeObj *shape) {
    return msQueryByShape(self, -1, shape);
  }

int mapObj_queryByIndex(mapObj *self, int qlayer, int tileindex, int shapeindex,
                        int bAddToQuery) {
    if (bAddToQuery)
      return msQueryByIndexAdd(self, qlayer, tileindex, shapeindex);
    else
      return msQueryByIndex(self, qlayer, tileindex, shapeindex);
}

int mapObj_saveQuery(mapObj *self, char *filename) {
  return msSaveQuery(self, filename);
}
int mapObj_loadQuery(mapObj *self, char *filename) {
  return msLoadQuery(self, filename);
}
void mapObj_freeQuery(mapObj *self, int qlayer) {
  msQueryFree(self, qlayer);
}

int mapObj_setWKTProjection(mapObj *self, char *string) {
    return msLoadWKTProjectionString(string, &(self->projection));
  }

char *mapObj_getProjection(mapObj* self) {
    return msGetProjectionString(&self->projection);
 }

int mapObj_setProjection(mapObj* self, char *string) {
    return(msLoadProjectionString(&(self->projection), string));
  }

int mapObj_save(mapObj* self, char *filename) {
    return msSaveMap(self, filename);
  }

char *mapObj_getMetaData(mapObj *self, char *name) {
    return(msLookupHashTable(self->web.metadata, name));
  }

int mapObj_setMetaData(mapObj *self, char *name, char *value) {
    if (!self->web.metadata)
        self->web.metadata = msCreateHashTable();
    if (msInsertHashTable(self->web.metadata, name, value) == NULL)
	return MS_FAILURE;
    return MS_SUCCESS;
  }


int mapObj_moveLayerup(mapObj *self, int layerindex){
    return msMoveLayerUp(self, layerindex);
}

int mapObj_moveLayerdown(mapObj *self, int layerindex){
    return msMoveLayerDown(self, layerindex);
}

int *mapObj_getLayersdrawingOrder(mapObj *self){
    return self->layerorder;
}
 
int mapObj_setLayersdrawingOrder(mapObj *self, int *panIndexes){
    return msSetLayersdrawingOrder(self, panIndexes);
}

char *mapObj_processTemplate(mapObj *self, int bGenerateImages, 
                             char **names, char **values, 
                             int numentries)
{
    return msProcessTemplate(self, bGenerateImages,
                             names, values, numentries);
}
  
char *mapObj_processLegendTemplate(mapObj *self,
                                   char **names, char **values, 
                                   int numentries)
{
    return msProcessLegendTemplate(self, names, values, numentries);
}
  

char *mapObj_processQueryTemplate(mapObj *self,
                                   char **names, char **values, 
                                   int numentries)
{
    return msProcessQueryTemplate(self, names, values, numentries);
}

int mapObj_setSymbolSet(mapObj *self,
                        char *szFileName)
{
    msFreeSymbolSet(&self->symbolset);
    msInitSymbolSet(&self->symbolset);
   
    // Set symbolset filename
    self->symbolset.filename = strdup(szFileName);

    // Symbolset shares same fontset as main mapfile
    self->symbolset.fontset = &(self->fontset);

    return msLoadSymbolSet(&self->symbolset, self);
}

int mapObj_getNumSymbols(mapObj *self)
{
    return self->symbolset.numsymbols;
}

int mapObj_setFontSet(mapObj *self, char *szFileName)
{
    msFreeFontSet(&(self->fontset));
    msInitFontSet(&(self->fontset));
   
    // Set fontset filename
    self->fontset.filename = strdup(szFileName);

    return msLoadFontSet(&(self->fontset), self);
}

int mapObj_saveMapContext(mapObj *self, char *szFilename)
{
    return msSaveMapContext(self, szFilename);
}

int mapObj_loadMapContext(mapObj *self, char *szFilename)
{
    return msLoadMapContext(self, szFilename);
}


int mapObj_selectOutputFormat(mapObj *self,
                              const char *imagetype)
{
    outputFormatObj *format = NULL;
    
    format = msSelectOutputFormat(self, imagetype);
    if (format)
    {
        msApplyOutputFormat( &(self->outputformat), format, 
                             self->transparent, self->interlace, 
                             self->imagequality );
        return(MS_SUCCESS);
    }
    return(MS_FAILURE);
    
}
      
/**********************************************************************
 * class extensions for layerObj, always within the context of a map
 **********************************************************************/
layerObj *layerObj_new(mapObj *map) {
    if(map->numlayers == MS_MAXLAYERS) // no room
      return(NULL);

    if(initLayer(&(map->layers[map->numlayers]), map) == -1)
      return(NULL);

    map->layers[map->numlayers].index = map->numlayers;
      //Update the layer order list with the layer's index.
    map->layerorder[map->numlayers] = map->numlayers;

    map->numlayers++;

    return &(map->layers[map->numlayers-1]);
  }

void layerObj_destroy(layerObj *self) {
    return; // map deconstructor takes care of it
  }

int layerObj_open(layerObj *self) {
    return msLayerOpen(self);
  }

void layerObj_close(layerObj *self) {
    msLayerClose(self);
  }

int layerObj_getShape(layerObj *self, shapeObj *shape, 
                      int tileindex, int shapeindex) {
    return msLayerGetShape(self, shape, tileindex, shapeindex);
  }

resultCacheMemberObj *layerObj_getResult(layerObj *self, int i) {
    if(!self->resultcache) return NULL;

    if(i >= 0 && i < self->resultcache->numresults)
      return &self->resultcache->results[i]; 
    else
      return NULL;
  }

classObj *layerObj_getClass(layerObj *self, int i) { // returns an EXISTING class
    if(i >= 0 && i < self->numclasses)
      return &(self->class[i]); 
    else
      return(NULL);
  }

int layerObj_draw(layerObj *self, mapObj *map, imageObj *img) {
    return msDrawLayer(map, self, img);
  }

int layerObj_drawQuery(layerObj *self, mapObj *map, imageObj *img) {
    return msDrawLayer(map, self, img);    
  }

int layerObj_queryByAttributes(layerObj *self, mapObj *map, char *qitem, char *qstring, int mode) {
    return msQueryByAttributes(map, self->index, qitem, qstring, mode);
  }

int layerObj_queryByPoint(layerObj *self, mapObj *map, 
                          pointObj *point, int mode, double buffer) {
    return msQueryByPoint(map, self->index, mode, *point, buffer);
  }

int layerObj_queryByRect(layerObj *self, mapObj *map, rectObj rect) {
    return msQueryByRect(map, self->index, rect);
  }

int layerObj_queryByFeatures(layerObj *self, mapObj *map, int slayer) {
    return msQueryByFeatures(map, self->index, slayer);
  }

int layerObj_queryByShape(layerObj *self, mapObj *map, shapeObj *shape) {
    return msQueryByShape(map, self->index, shape);
  }

int layerObj_setFilter(layerObj *self, char *string) {
    return loadExpressionString(&self->filter, string);

  }

int layerObj_setWKTProjection(layerObj *self, char *string) {
    self->project = MS_TRUE;
    return msLoadWKTProjectionString(string, &(self->projection));
  }

char *layerObj_getProjection(layerObj* self) {
    return msGetProjectionString(&self->projection);
 }

int layerObj_setProjection(layerObj *self, char *string) {
    return(msLoadProjectionString(&(self->projection), string));
  }

int layerObj_addFeature(layerObj *self, shapeObj *shape) {
    if(insertFeatureList(&(self->features), shape) == NULL) 
      return -1;
    else
      return 0;
  }

char *layerObj_getMetaData(layerObj *self, char *name) {
    return(msLookupHashTable(self->metadata, name));
  }

int layerObj_setMetaData(layerObj *self, char *name, char *value) {
    if (!self->metadata)
        self->metadata = msCreateHashTable();
    if (msInsertHashTable(self->metadata, name, value) == NULL)
	return MS_FAILURE;
    return MS_SUCCESS;
  }

char *layerObj_getWMSFeatureInfoURL(layerObj *self, mapObj *map, int click_x, int click_y,     
                                    int feature_count, char *info_format) {
    // NOTE: the returned string should be freed by the caller but right 
    // now we're leaking it.
    return(msWMSGetFeatureInfoURL(map, self, click_x, click_y,
                                  feature_count, info_format));
  }

/**********************************************************************
 * class extensions for classObj, always within the context of a layer
 **********************************************************************/
classObj *classObj_new(layerObj *layer) {
    if(layer->numclasses == MS_MAXCLASSES) // no room
      return NULL;

    if(initClass(&(layer->class[layer->numclasses])) == -1)
      return NULL;
    layer->class[layer->numclasses].type = layer->type;

    layer->numclasses++;

    return &(layer->class[layer->numclasses-1]);
  }

void  classObj_destroy(classObj *self) {
    return; /* do nothing, map deconstrutor takes care of it all */
  }

int classObj_setExpression(classObj *self, char *string) {
    return loadExpressionString(&self->expression, string);
  }

int classObj_setText(classObj *self, layerObj *layer, char *string) {
    return loadExpressionString(&self->text, string);
  }

int classObj_drawLegendIcon(classObj *self, mapObj *map, layerObj *layer, int width, int height, gdImagePtr dstImg, int dstX, int dstY) {
    return msDrawLegendIcon(map, layer, self, width, height, dstImg, dstX, dstY);
}

imageObj *classObj_createLegendIcon(classObj *self, mapObj *map, layerObj *layer, int width, int height) {
    return msCreateLegendIcon(map, layer, self, width, height);
  }


int classObj_setSymbolByName(classObj *self, mapObj *map, char* pszSymbolName) {
  /*
   self->symbol = msGetSymbolIndex(&map->symbolset, pszSymbolName);
    return self->symbol;
  */
  return -1;
}
  
int classObj_setOverlaySymbolByName(classObj *self, mapObj *map, char* pszOverlaySymbolName) {
  /*
    self->overlaysymbol = msGetSymbolIndex(&map->symbolset, pszOverlaySymbolName);
    return self->overlaysymbol;
  */
  return -1;

  }


/**********************************************************************
 * class extensions for pointObj, useful many places
 **********************************************************************/
pointObj *pointObj_new() {
    return (pointObj *)malloc(sizeof(pointObj));
  }

void pointObj_destroy(pointObj *self) {
    free(self);
  }

int pointObj_project(pointObj *self, projectionObj *in, projectionObj *out) {
    return msProjectPoint(in, out, self);
  }	

int pointObj_draw(pointObj *self, mapObj *map, layerObj *layer, 
                  imageObj *img, int class_index, char *label_string) {
    return msDrawPoint(map, layer, self, img, class_index, label_string);
  }

double pointObj_distanceToPoint(pointObj *self, pointObj *point) {
    return msDistancePointToPoint(self, point);
  }

double pointObj_distanceToLine(pointObj *self, pointObj *a, pointObj *b) {
    return msDistancePointToSegment(self, a, b);
  }

double pointObj_distanceToShape(pointObj *self, shapeObj *shape) {
   return msDistancePointToShape(self, shape);
  }

/**********************************************************************
 * class extensions for lineObj (eg. a line or group of points), 
 * useful many places
 **********************************************************************/
lineObj *lineObj_new() {
    lineObj *line;

    line = (lineObj *)malloc(sizeof(lineObj));
    if(!line)
      return(NULL);

    line->numpoints=0;
    line->point=NULL;

    return line;
  }

void lineObj_destroy(lineObj *self) {
    free(self->point);
    free(self);		
  }

int lineObj_project(lineObj *self, projectionObj *in, projectionObj *out) {
    return msProjectLine(in, out, self);
  }

pointObj *lineObj_get(lineObj *self, int i) {
    if(i<0 || i>=self->numpoints)
      return NULL;
    else
      return &(self->point[i]);
  }

int lineObj_add(lineObj *self, pointObj *p) {
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
    self->point[self->numpoints].m = p->m;
    self->numpoints++;

    return 0;
  }


/**********************************************************************
 * class extensions for shapeObj
 **********************************************************************/
shapeObj *shapeObj_new(int type) {
    shapeObj *shape;

    shape = (shapeObj *)malloc(sizeof(shapeObj));
    if(!shape)
      return NULL;

    msInitShape(shape);
    shape->type = type;
    return shape;
  }

void shapeObj_destroy(shapeObj *self) {
    msFreeShape(self);
    free(self);		
  }

int shapeObj_project(shapeObj *self, projectionObj *in, projectionObj *out) {
    return msProjectShape(in, out, self);
  }

lineObj *shapeObj_get(shapeObj *self, int i) {
    if(i<0 || i>=self->numlines)
      return NULL;
    else
      return &(self->line[i]);
  }

int shapeObj_add(shapeObj *self, lineObj *line) {
    return msAddLine(self, line);
  }

int shapeObj_draw(shapeObj *self, mapObj *map, layerObj *layer, 
                  imageObj *img) {
    return msDrawShape(map, layer, self, img, -1);
  }

void shapeObj_setBounds(shapeObj *self) {
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

int shapeObj_copy(shapeObj *self, shapeObj *dest) {
    return(msCopyShape(self, dest));
  }

int shapeObj_contains(shapeObj *self, pointObj *point) {
    if(self->type == MS_SHAPE_POLYGON)
      return msIntersectPointPolygon(point, self);

    return -1;
  }

int shapeObj_intersects(shapeObj *self, shapeObj *shape) {
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

pointObj *shapeObj_getpointusingmeasure(shapeObj *self, double m) 
{       
   if (self)
     return getPointUsingMeasure(self, m);
   
   return NULL;
}

pointObj *shapeObj_getmeasureusingpoint(shapeObj *self, pointObj *point) 
{       
   if (self)
     return getMeasureUsingPoint(self, point);
   
   return NULL;
}

/**********************************************************************
 * class extensions for rectObj
 **********************************************************************/
rectObj *rectObj_new() {	
    rectObj *rect;

    rect = (rectObj *)calloc(1, sizeof(rectObj));
    if(!rect)
      return(NULL);
    
    return(rect);    	
  }

void rectObj_destroy(rectObj *self) {
    free(self);
  }

int rectObj_project(rectObj *self, projectionObj *in, projectionObj *out) {
    return msProjectRect(in, out, self);
  }

double rectObj_fit(rectObj *self, int width, int height) {
    return  msAdjustExtent(self, width, height);
  } 

int rectObj_draw(rectObj *self, mapObj *map, layerObj *layer,
                 imageObj *img, int classindex, char *text) {
    shapeObj shape;

    msInitShape(&shape);
    msRectToPolygon(*self, &shape);
    shape.classindex = classindex;
    shape.text = strdup(text);

    msDrawShape(map, layer, &shape, img, -1);

    msFreeShape(&shape);
    
    return 0;
  }

/**********************************************************************
 * class extensions for shapefileObj
 **********************************************************************/
shapefileObj *shapefileObj_new(char *filename, int type) {    
    shapefileObj *shapefile;
    int status;

    shapefile = (shapefileObj *)malloc(sizeof(shapefileObj));
    if(!shapefile)
      return NULL;

    if(type == -1)
      status = msSHPOpenFile(shapefile, "rb", filename);
    else if (type == -2)
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

void shapefileObj_destroy(shapefileObj *self) {
    msSHPCloseFile(self);
    free(self);  
  }

int shapefileObj_get(shapefileObj *self, int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);

    return 0;
  }

int shapefileObj_getPoint(shapefileObj *self, int i, pointObj *point) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msSHPReadPoint(self->hSHP, i, point);
    return 0;
  }

int shapefileObj_getTransformed(shapefileObj *self, mapObj *map, 
                                int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return -1;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);
    msTransformShapeToPixel(shape, map->extent, map->cellsize);

    return 0;
  }

void shapefileObj_getExtent(shapefileObj *self, int i, rectObj *rect) {
    msSHPReadBounds(self->hSHP, i, rect);
  }

int shapefileObj_add(shapefileObj *self, shapeObj *shape) {
    return msSHPWriteShape(self->hSHP, shape);	
  }	

int shapefileObj_addPoint(shapefileObj *self, pointObj *point) {    
    return msSHPWritePoint(self->hSHP, point);	
  }

/**********************************************************************
 * class extensions for projectionObj
 **********************************************************************/
projectionObj *projectionObj_new(char *string) {	

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


void projectionObj_destroy(projectionObj *self) {
    msFreeProjection(self);
    free(self);
  }

/**********************************************************************
 * class extensions for labelCacheObj - TP mods
 **********************************************************************/
void labelCacheObj_freeCache(labelCacheObj *self) {
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

/**********************************************************************
 * class extensions for DBFInfo - TP mods
 **********************************************************************/
char *DBFInfo_getFieldName(DBFInfo *self, int iField) {
        static char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pszFieldName;
    }

int DBFInfo_getFieldWidth(DBFInfo *self, int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnWidth;
    }

int DBFInfo_getFieldDecimals(DBFInfo *self, int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnDecimals;
    }
    
DBFFieldType DBFInfo_getFieldType(DBFInfo *self, int iField) {
	return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
    }    

/**********************************************************************
 * class extensions for styleObj, always within the context of a class
 **********************************************************************/
styleObj *styleObj_new(classObj *class) {
    if(class->numstyles == MS_MAXSTYLES) // no room
      return NULL;

    if(initStyle(&(class->styles[class->numstyles])) == -1)
      return NULL;

    class->numstyles++;

    return &(class->styles[class->numstyles-1]);
  }

void  styleObj_destroy(styleObj *self) {
    return; /* do nothing, map deconstrutor takes care of it all */
  }


int styleObj_setSymbolByName(styleObj *self, mapObj *map, char* pszSymbolName) {
    self->symbol = msGetSymbolIndex(&map->symbolset, pszSymbolName);
    return self->symbol;
}
