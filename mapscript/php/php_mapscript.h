/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PHP/MapScript extension for MapServer. Header file
 *           - prototypes / module definitions
 * Author:   Daniel Morissette, DM Solutions Group (dmorissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2000-2005, Daniel Morissette, DM Solutions Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/


#ifndef PHP_MAPSCRIPT_H
#define PHP_MAPSCRIPT_H

#include "php.h"
#include "zend_interfaces.h"
#include "php_mapscript_util.h"

#ifdef USE_PHP_REGEX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <direct.h>
#include <memory.h>
#include <malloc.h>
#else
#include <unistd.h>
#endif

#include "../../mapserver.h"
#include "../../mapregex.h"
#endif /* USE_PHP_REGEX */


#include "../../maptemplate.h"
#include "../../mapogcsld.h"

#define MAPSCRIPT_VERSION "($Revision$ $Date$)"

extern zend_module_entry mapscript_module_entry;
#define phpext_mapscript_ptr &mapscript_module_entry

#ifndef zend_parse_parameters_none
#define zend_parse_parameters_none()        \
  zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")
#endif

/* it looks like that macro is not always defined: ticket #3926 */
#ifndef TRUE
#define TRUE 1
#endif

/* Taken from the CAIRO php extension */
/* turn error handling to exception mode and restore */
#if defined(PHP_VERSION_ID) && PHP_VERSION_ID >= 50300
/* 5.3 version of the macros */
#define PHP_MAPSCRIPT_ERROR_HANDLING(force_exceptions) \
  zend_error_handling error_handling; \
  if(force_exceptions || getThis()) { \
    zend_replace_error_handling(EH_THROW, mapscript_ce_mapscriptexception, &error_handling TSRMLS_CC); \
  }

#define PHP_MAPSCRIPT_RESTORE_ERRORS(force_exceptions) \
  if(force_exceptions || getThis()) { \
    zend_restore_error_handling(&error_handling TSRMLS_CC); \
  }

#else
/* 5.2 versions of the macros */
#define PHP_MAPSCRIPT_ERROR_HANDLING(force_exceptions) \
  if(force_exceptions || getThis()) { \
    php_set_error_handling(EH_THROW, mapscript_ce_mapscriptexception TSRMLS_CC); \
  }

#define PHP_MAPSCRIPT_RESTORE_ERRORS(force_exceptions) \
  if(force_exceptions || getThis()) { \
    php_std_error_handling(); \
  }

#endif

/* MapScript objects */
typedef struct _parent_object {
  zval *val; // the zval of the parent
  zval **child_ptr; // a ptr to a parent property, which point to the child object.
  // should be set to NULL when the child is destroyed
} parent_object;

typedef struct _php_color_object {
  zend_object std;
  parent_object parent;
  colorObj *color;
} php_color_object;

typedef struct _php_rect_object {
  zend_object std;
  parent_object parent;
  int is_ref;
  rectObj *rect;
} php_rect_object;

typedef struct _php_hashtable_object {
  zend_object std;
  parent_object parent;
  hashTableObj *hashtable;
} php_hashtable_object;

typedef struct _php_symbol_object {
  zend_object std;
  parent_object parent;
  symbolObj *symbol;
} php_symbol_object;

typedef struct _php_class_object {
  zend_object std;
  parent_object parent; //old layer
  zval *metadata;
  zval *leader;
  classObj *class;
} php_class_object;

typedef struct _php_image_object {
  zend_object std;
  imageObj *image;
} php_image_object;

typedef struct _php_web_object {
  zend_object std;
  parent_object parent;
  zval *extent;
  zval *metadata;
  zval *validation;
  webObj *web;
} php_web_object;

typedef struct _php_legend_object {
  zend_object std;
  parent_object parent;
  zval *outlinecolor;
  zval *label;
  zval *imagecolor;
  legendObj *legend;
} php_legend_object;

typedef struct _php_outputformat_object {
  zend_object std;
  parent_object parent;
  int is_ref;
  outputFormatObj *outputformat;
} php_outputformat_object;

typedef struct _php_querymap_object {
  zend_object std;
  parent_object parent;
  zval *color;
  queryMapObj *querymap;
} php_querymap_object;

typedef struct _php_grid_object {
  zend_object std;
  parent_object parent;
  graticuleObj *grid;
} php_grid_object;

typedef struct _php_error_object {
  zend_object std;
  errorObj *error;
} php_error_object;

typedef struct _php_referencemap_object {
  zend_object std;
  parent_object parent;
  zval *extent;
  zval *color;
  zval *outlinecolor;
  referenceMapObj *referencemap;
} php_referencemap_object;

typedef struct _php_label_object {
  zend_object std;
  parent_object parent;
  int is_ref;
  zval *color;
  zval *outlinecolor;
  zval *shadowcolor;
  zval *backgroundcolor;
  zval *backgroundshadowcolor;
  zval *leader;
  labelObj *label;
} php_label_object;

typedef struct _php_style_object {
  zend_object std;
  parent_object parent;
  zval *color;
  zval *outlinecolor;
  zval *backgroundcolor;
  zval *mincolor;
  zval *maxcolor;
  styleObj *style;
} php_style_object;

typedef struct _php_projection_object {
  zend_object std;
  parent_object parent;
  int is_ref;
  projectionObj *projection;
} php_projection_object;

typedef struct _php_point_object {
  zend_object std;
  parent_object parent;
  int is_ref;
  pointObj *point;
} php_point_object;

typedef struct _php_line_object {
  zend_object std;
  parent_object parent;
  int is_ref;
  lineObj *line;
} php_line_object;

typedef struct _php_shape_object {
  zend_object std;
  parent_object parent;
  zval *bounds;
  zval *values;
  int is_ref;
  shapeObj *shape;
} php_shape_object;

typedef struct _php_shapefile_object {
  zend_object std;
  zval *bounds;
  shapefileObj *shapefile;
} php_shapefile_object;

typedef struct _php_labelcache_object {
  zend_object std;
  parent_object parent;
  labelCacheObj *labelcache;
} php_labelcache_object;

typedef struct _php_labelleader_object {
  zend_object std;
  parent_object parent;
  labelLeaderObj *labelleader;
} php_labelleader_object;

typedef struct _php_labelcachemember_object {
  zend_object std;
  parent_object parent;
  zval *labels; /* should be immutable */
  zval *point; /* should be immutable */
  zval *styles; /* should be immutable */
  zval *poly; /* should be immutable */
  labelCacheMemberObj *labelcachemember;
} php_labelcachemember_object;

typedef struct _php_result_object {
  zend_object std;
  parent_object parent;
  resultObj *result;
} php_result_object;

typedef struct _php_scalebar_object {
  zend_object std;
  parent_object parent;
  zval *color;
  zval *backgroundcolor;
  zval *outlinecolor;
  zval *label;
  zval *imagecolor;
  scalebarObj *scalebar;
} php_scalebar_object;

typedef struct _php_owsrequest_object {
  zend_object std;
  cgiRequestObj *cgirequest;
} php_owsrequest_object;

typedef struct _php_layer_object {
  zend_object std;
  parent_object parent; //old map
  zval *offsite;
  zval *grid;
  zval *metadata;
  zval *bindvals;
  zval *projection;
  zval *cluster;
  zval *extent;
  int is_ref;
  layerObj *layer;
} php_layer_object;

typedef struct _php_map_object {
  zend_object std;
  zval *outputformat;
  zval *extent;
  zval *web;
  zval *reference;
  zval *imagecolor;
  zval *scalebar;
  zval *legend;
  zval *querymap;
  zval *labelcache;
  zval *projection;
  zval *metadata;
  zval *configoptions;
  mapObj *map;
} php_map_object;

typedef struct _php_cluster_object {
  zend_object std;
  parent_object parent;
  int is_ref;
  clusterObj *cluster;
} php_cluster_object;

/* Lifecyle functions*/
PHP_MINIT_FUNCTION(mapscript);
PHP_MINFO_FUNCTION(mapscript);
PHP_MSHUTDOWN_FUNCTION(mapscript);
PHP_RINIT_FUNCTION(mapscript);
PHP_RSHUTDOWN_FUNCTION(mapscript);

PHP_MINIT_FUNCTION(mapscript_error);
PHP_MINIT_FUNCTION(color);
PHP_MINIT_FUNCTION(label);
PHP_MINIT_FUNCTION(style);
PHP_MINIT_FUNCTION(symbol);
PHP_MINIT_FUNCTION(image);
PHP_MINIT_FUNCTION(web);
PHP_MINIT_FUNCTION(legend);
PHP_MINIT_FUNCTION(outputformat);
PHP_MINIT_FUNCTION(querymap);
PHP_MINIT_FUNCTION(grid);
PHP_MINIT_FUNCTION(error);
PHP_MINIT_FUNCTION(referencemap);
PHP_MINIT_FUNCTION(class);
PHP_MINIT_FUNCTION(projection);
PHP_MINIT_FUNCTION(labelcachemember);
PHP_MINIT_FUNCTION(labelcache);
PHP_MINIT_FUNCTION(labelleader);
PHP_MINIT_FUNCTION(result);
PHP_MINIT_FUNCTION(scalebar);
PHP_MINIT_FUNCTION(owsrequest);
PHP_MINIT_FUNCTION(point);
PHP_MINIT_FUNCTION(rect);
PHP_MINIT_FUNCTION(hashtable);
PHP_MINIT_FUNCTION(line);
PHP_MINIT_FUNCTION(shape);
PHP_MINIT_FUNCTION(shapefile);
PHP_MINIT_FUNCTION(layer);
PHP_MINIT_FUNCTION(map);
PHP_MINIT_FUNCTION(cluster);

/* mapscript functions */
PHP_FUNCTION(ms_GetVersion);
PHP_FUNCTION(ms_GetVersionInt);
PHP_FUNCTION(ms_GetErrorObj);
PHP_FUNCTION(ms_ResetErrorList);
PHP_FUNCTION(ms_getCwd);
PHP_FUNCTION(ms_getPid);
PHP_FUNCTION(ms_getScale);
PHP_FUNCTION(ms_tokenizeMap);
PHP_FUNCTION(ms_ioInstallStdoutToBuffer);
PHP_FUNCTION(ms_ioInstallStdinFromBuffer);
PHP_FUNCTION(ms_ioGetStdoutBufferString);
PHP_FUNCTION(ms_ioResetHandlers);
PHP_FUNCTION(ms_ioStripStdoutBufferContentType);
PHP_FUNCTION(ms_ioStripStdoutBufferContentHeaders);
PHP_FUNCTION(ms_ioGetStdoutBufferBytes);

/* object constructors */
PHP_FUNCTION(ms_newLineObj);
PHP_FUNCTION(ms_newRectObj);
PHP_FUNCTION(ms_newShapeObj);
PHP_FUNCTION(ms_shapeObjFromWkt);
PHP_FUNCTION(ms_newOWSRequestObj);
PHP_FUNCTION(ms_newShapeFileObj);
PHP_FUNCTION(ms_newMapObj);
PHP_FUNCTION(ms_newMapObjFromString);
PHP_FUNCTION(ms_newLayerObj);
PHP_FUNCTION(ms_newPointObj);
PHP_FUNCTION(ms_newProjectionObj);
PHP_FUNCTION(ms_newStyleObj);
PHP_FUNCTION(ms_newSymbolObj);
PHP_FUNCTION(ms_newClassObj);
PHP_FUNCTION(ms_newGridObj);

/* mapscript zend class entries */
extern zend_object_handlers mapscript_std_object_handlers;
extern zend_class_entry *mapscript_ce_mapscriptexception;
extern zend_class_entry *mapscript_ce_color;
extern zend_class_entry *mapscript_ce_label;
extern zend_class_entry *mapscript_ce_projection;
extern zend_class_entry *mapscript_ce_point;
extern zend_class_entry *mapscript_ce_rect;
extern zend_class_entry *mapscript_ce_hashtable;
extern zend_class_entry *mapscript_ce_style;
extern zend_class_entry *mapscript_ce_class;
extern zend_class_entry *mapscript_ce_symbol;
extern zend_class_entry *mapscript_ce_image;
extern zend_class_entry *mapscript_ce_web;
extern zend_class_entry *mapscript_ce_legend;
extern zend_class_entry *mapscript_ce_outputformat;
extern zend_class_entry *mapscript_ce_querymap;
extern zend_class_entry *mapscript_ce_grid;
extern zend_class_entry *mapscript_ce_error;
extern zend_class_entry *mapscript_ce_referencemap;
extern zend_class_entry *mapscript_ce_line;
extern zend_class_entry *mapscript_ce_shape;
extern zend_class_entry *mapscript_ce_shapefile;
extern zend_class_entry *mapscript_ce_labelcachemember;
extern zend_class_entry *mapscript_ce_labelcache;
extern zend_class_entry *mapscript_ce_labelleader;
extern zend_class_entry *mapscript_ce_result;
extern zend_class_entry *mapscript_ce_scalebar;
extern zend_class_entry *mapscript_ce_owsrequest;
extern zend_class_entry *mapscript_ce_layer;
extern zend_class_entry *mapscript_ce_map;
extern zend_class_entry *mapscript_ce_cluster;

/* PHP Object constructors */
extern zend_object_value mapscript_object_new(zend_object *zobj, zend_class_entry *ce,
    void (*zend_objects_free_object) TSRMLS_DC);
extern zend_object_value mapscript_object_new_ex(zend_object *zobj, zend_class_entry *ce,
    void (*zend_objects_free_object),
    zend_object_handlers *object_handlers TSRMLS_DC);
extern void mapscript_fetch_object(zend_class_entry *ce, zval* zval_parent, php_layer_object* layer,
                                   void *internal_object, zval **php_object_storage TSRMLS_DC);
extern void mapscript_create_color(colorObj *color, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_rect(rectObj *rect, parent_object php_parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_hashtable(hashTableObj *hashtable, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_label(labelObj *label, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_style(styleObj *style, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_symbol(symbolObj *symbol, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_class(classObj *class, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_labelcachemember(labelCacheMemberObj *labelcachemember,
    parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_labelcache(labelCacheObj *labelcache,
                                        parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_labelleader(labelLeaderObj *labelleader,
    parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_result(resultObj *result,
                                    parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_scalebar(scalebarObj *scalebar, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_owsrequest(cgiRequestObj *cgirequest, zval *return_value TSRMLS_DC);
extern void mapscript_create_image(imageObj *image, zval *return_value TSRMLS_DC);
extern void mapscript_create_web(webObj *web, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_legend(legendObj *legend, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_outputformat(outputFormatObj *outputformat, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_querymap(queryMapObj *querymap, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_grid(graticuleObj *grid, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_error(errorObj *error, zval *return_value TSRMLS_DC);
extern void mapscript_create_referencemap(referenceMapObj *referenceMap, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_point(pointObj *point, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_projection(projectionObj *projection,
                                        parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_line(lineObj *line, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_shape(shapeObj *shape, parent_object parent, php_layer_object *php_layer, zval *return_value TSRMLS_DC);
extern void mapscript_create_shapefile(shapefileObj *shapefile, zval *return_value TSRMLS_DC);
extern void mapscript_create_layer(layerObj *layer, parent_object parent, zval *return_value TSRMLS_DC);
extern void mapscript_create_map(mapObj *map, zval *return_value TSRMLS_DC);
extern void mapscript_create_cluster(clusterObj *cluster, parent_object php_parent, zval *return_value TSRMLS_DC);

/* Exported functions for PHP Mapscript API */
/* throw a MapScriptException */
extern zval * mapscript_throw_exception(char *format TSRMLS_DC, ...);
/* print all MapServer errors (as Warning) and throw a MapScriptException */
extern zval* mapscript_throw_mapserver_exception(char *format TSRMLS_DC, ...);
extern void mapscript_report_mapserver_error(int error_type TSRMLS_DC);
extern void mapscript_report_php_error(int error_type, char *format TSRMLS_DC, ...);

/*=====================================================================
 *                   Internal functions from mapscript_i.c
 *====================================================================*/

mapObj         *mapObj_new(char *filename, char *new_path);
mapObj         *mapObj_newFromString(char *map_text, char *new_path);
void            mapObj_destroy(mapObj* self);
mapObj         *mapObj_clone(mapObj* self);
int             mapObj_setRotation(mapObj* self, double rotation_angle );
layerObj       *mapObj_getLayer(mapObj* self, int i);
layerObj       *mapObj_getLayerByName(mapObj* self, char *name);
int             *mapObj_getLayersIndexByGroup(mapObj* self, char *groupname,
    int *pnCount);
int             mapObj_getSymbolByName(mapObj* self, char *name);
void            mapObj_prepareQuery(mapObj* self);
imageObj        *mapObj_prepareImage(mapObj* self);
imageObj        *mapObj_draw(mapObj* self);
imageObj        *mapObj_drawQuery(mapObj* self);
imageObj        *mapObj_drawLegend(mapObj* self);
imageObj        *mapObj_drawScalebar(mapObj* self);
imageObj        *mapObj_drawReferenceMap(mapObj* self);
int             mapObj_embedScalebar(mapObj* self, imageObj *img);
int             mapObj_embedLegend(mapObj* self, imageObj *img);
int             mapObj_drawLabelCache(mapObj* self, imageObj *img);
labelCacheMemberObj *mapObj_getLabel(mapObj* self, int i);
int             mapObj_queryByPoint(mapObj* self, pointObj *point,
                                    int mode, double buffer);
int             mapObj_queryByRect(mapObj* self, rectObj rect);
int             mapObj_queryByFeatures(mapObj* self, int slayer);
int             mapObj_queryByShape(mapObj *self, shapeObj *shape);
int              mapObj_queryByIndex(mapObj *self, int qlayer,
                                     int tileindex, int shapeindex,
                                     int bAddToQuery);
int             mapObj_queryByFilter(mapObj* self, char *string);
int             mapObj_saveQuery(mapObj *self, char *filename, int results);
int             mapObj_loadQuery(mapObj *self, char *filename);

int             mapObj_setWKTProjection(mapObj *self, char *string);
char*           mapObj_getProjection(mapObj* self);
int             mapObj_setProjection(mapObj* self, char *string);
int             mapObj_save(mapObj* self, char *filename);
char            *mapObj_getMetaData(mapObj *self, char *name);
int             mapObj_setMetaData(mapObj *self, char *name, char *value);
int             mapObj_removeMetaData(mapObj *self, char *name);
void            mapObj_freeQuery(mapObj *self, int qlayer);
int             mapObj_moveLayerup(mapObj *self, int layerindex);
int             mapObj_moveLayerdown(mapObj *self, int layerindex);
int             *mapObj_getLayersdrawingOrder(mapObj *self);
int             mapObj_setLayersdrawingOrder(mapObj *self, int *panIndexes);

char            *mapObj_processTemplate(mapObj *self, int bGenerateImages,
                                        char **names, char **values,
                                        int numentries);
char            *mapObj_processLegendTemplate(mapObj *self,
    char **names, char **values,
    int numentries);
char            *mapObj_processQueryTemplate(mapObj *self,
    int bGenerateImages,
    char **names, char **values,
    int numentries);
int             mapObj_setSymbolSet(mapObj *self, char *szFileName);
int             mapObj_getNumSymbols(mapObj *self);
int             mapObj_setFontSet(mapObj *self, char *szFileName);
int             mapObj_saveMapContext(mapObj *self, char *szFileName);
int             mapObj_loadMapContext(mapObj *self, char *szFileName,
                                      int bUniqueLayerName);
int             mapObj_selectOutputFormat(mapObj *self,
    const char *imagetype);
int             mapObj_applySLD(mapObj *self, char *sld);
int             mapObj_applySLDURL(mapObj *self, char *sld);
char            *mapObj_generateSLD(mapObj *self);
int             mapObj_loadOWSParameters(mapObj *self, cgiRequestObj *request,
    char *wmtver_string);
int             mapObj_OWSDispatch(mapObj *self, cgiRequestObj *req );
int             mapObj_insertLayer(mapObj *self, layerObj *layer, int index);
layerObj        *mapObj_removeLayer(mapObj *self, int layerindex);

int             mapObj_setCenter(mapObj *self, pointObj *center);
int             mapObj_offsetExtent(mapObj *self, double x, double y);
int             mapObj_scaleExtent(mapObj *self, double zoomfactor, double minscaledenom,
                                   double maxscaledenom);

layerObj       *layerObj_new(mapObj *map);
void            layerObj_destroy(layerObj* self);
int             layerObj_updateFromString(layerObj *self, char *snippet);
int             layerObj_open(layerObj *self);
int             layerObj_whichShapes(layerObj *self, rectObj *poRect);
shapeObj        *layerObj_nextShape(layerObj *self);
void            layerObj_close(layerObj *self);
int             layerObj_getShape(layerObj *self, shapeObj *shape,
                                  int tileindex, int shapeindex);
resultObj *layerObj_getResult(layerObj *self, int i);
classObj       *layerObj_getClass(layerObj *self, int i);
int layerObj_getClassIndex(layerObj *self, mapObj *map, shapeObj *shape,
                           int *classgroup, int numclasses);
int             layerObj_draw(layerObj *self, mapObj *map, imageObj *img);
int             layerObj_drawQuery(layerObj *self, mapObj *map, imageObj *img);
int             layerObj_queryByAttributes(layerObj *self, mapObj *map,
    char *qitem, char *qstring,
    int mode);
int             layerObj_queryByPoint(layerObj *self, mapObj *map,
                                      pointObj *point, int mode, double buffer);
int             layerObj_queryByRect(layerObj *self, mapObj *map,rectObj rect);
int             layerObj_queryByFeatures(layerObj *self, mapObj *map,
    int slayer);
int             layerObj_queryByShape(layerObj *self, mapObj *map,
                                      shapeObj *shape);
int             layerObj_queryByFilter(layerObj *self, mapObj *map, char *string);
int             layerObj_queryByIndex(layerObj *self, mapObj *map, int tileindex,
                                      int shapeindex, int addtoquery);
int             layerObj_setFilter(layerObj *self, char *string);
char*           layerObj_getFilter(layerObj *self);
int             layerObj_setWKTProjection(layerObj *self, char *string);
char*           layerObj_getProjection(layerObj *self);
int             layerObj_setProjection(layerObj *self, char *string);
int             layerObj_addFeature(layerObj *self, shapeObj *shape);
char            *layerObj_getMetaData(layerObj *self, char *name);
int             layerObj_setMetaData(layerObj *self, char *name, char *value);
int             layerObj_removeMetaData(layerObj *self, char *name);
char            *layerObj_getWMSFeatureInfoURL(layerObj *self, mapObj *map,
    int click_x, int click_y,
    int feature_count,
    char *info_format);
char            *layerObj_executeWFSGetFeature(layerObj *self);
int             layerObj_applySLD(layerObj *self, char *sld, char *stylelayer);
int             layerObj_applySLDURL(layerObj *self, char *sld, char *stylelayer);
char            *layerObj_generateSLD(layerObj *self);
int             layerObj_moveClassUp(layerObj *self, int index);
int             layerObj_moveClassDown(layerObj *self, int index);
classObj        *layerObj_removeClass(layerObj *self, int index);
int             layerObj_setConnectionType(layerObj *self, int connectiontype,
    const char *library_str) ;
layerObj        *layerObj_clone(layerObj *layer);

labelObj        *labelObj_new();
int             labelObj_updateFromString(labelObj *self, char *snippet);
void            labelObj_destroy(labelObj *self);
int             labelObj_moveStyleUp(labelObj *self, int index);
int             labelObj_moveStyleDown(labelObj *self, int index);
int             labelObj_deleteStyle(labelObj *self, int index);
labelObj        *labelObj_clone(labelObj *label);

int             legendObj_updateFromString(legendObj *self, char *snippet);

int             queryMapObj_updateFromString(queryMapObj *self, char *snippet);

int             referenceMapObj_updateFromString(referenceMapObj *self, char *snippet);

int             scalebarObj_updateFromString(scalebarObj *self, char *snippet);

int             webObj_updateFromString(webObj *self, char *snippet);

classObj       *classObj_new(layerObj *layer, classObj *class);
labelObj       *classObj_getLabel(classObj *self, int i);
int             classObj_addLabel(classObj *self, labelObj *label);
labelObj       *classObj_removeLabel(classObj *self, int index);
int             classObj_updateFromString(classObj *self, char *snippet);
void            classObj_destroy(classObj* self);
int             classObj_setExpression(classObj *self, char *string);
char            *classObj_getExpressionString(classObj *self);
int             classObj_setText(classObj *self,layerObj *layer,char *string);
char           *classObj_getTextString(classObj *self);
int             classObj_drawLegendIcon(classObj *self,
                                        mapObj *map,
                                        layerObj *layer,
                                        int width, int height,
                                        imageObj *im,
                                        int dstX, int dstY);
imageObj       *classObj_createLegendIcon(classObj *self,
    mapObj *map,
    layerObj *layer,
    int width, int height);
int             classObj_setSymbolByName(classObj *self,
    mapObj *map,
    char *pszSymbolName);
int             classObj_setOverlaySymbolByName(classObj *self,
    mapObj *map,
    char *pszOverlaySymbolName);
classObj        *classObj_clone(classObj *class, layerObj *layer);
int             classObj_moveStyleUp(classObj *self, int index);
int             classObj_moveStyleDown(classObj *self, int index);
int             classObj_deleteStyle(classObj *self, int index);
char            *classObj_getMetaData(classObj *self, char *name);
int             classObj_setMetaData(classObj *self, char *name, char *value);
int             classObj_removeMetaData(classObj *self, char *name);

pointObj       *pointObj_new();
void            pointObj_destroy(pointObj *self);
int             pointObj_project(pointObj *self, projectionObj *in,
                                 projectionObj *out);
int             pointObj_draw(pointObj *self, mapObj *map, layerObj *layer,
                              imageObj *img, int class_index,
                              char *label_string);
double          pointObj_distanceToPoint(pointObj *self, pointObj *point);
double          pointObj_distanceToLine(pointObj *self, pointObj *a,
                                        pointObj *b);
double          pointObj_distanceToShape(pointObj *self, shapeObj *shape);


lineObj        *lineObj_new();
void            lineObj_destroy(lineObj *self);
lineObj        *lineObj_clone(lineObj *line);
int             lineObj_project(lineObj *self, projectionObj *in,
                                projectionObj *out);
pointObj       *lineObj_get(lineObj *self, int i);
int             lineObj_add(lineObj *self, pointObj *p);


shapeObj       *shapeObj_new(int type);
void            shapeObj_destroy(shapeObj *self);
int             shapeObj_project(shapeObj *self, projectionObj *in,
                                 projectionObj *out);
lineObj        *shapeObj_get(shapeObj *self, int i);
int             shapeObj_add(shapeObj *self, lineObj *line);
int             shapeObj_draw(shapeObj *self, mapObj *map, layerObj *layer,
                              imageObj *img);
void            shapeObj_setBounds(shapeObj *self);
int             shapeObj_copy(shapeObj *self, shapeObj *dest);
int             shapeObj_contains(shapeObj *self, pointObj *point);
int             shapeObj_intersects(shapeObj *self, shapeObj *shape);
pointObj        *shapeObj_getpointusingmeasure(shapeObj *self, double m);
pointObj        *shapeObj_getmeasureusingpoint(shapeObj *self, pointObj *point);

shapeObj        *shapeObj_buffer(shapeObj *self, double width);
shapeObj        *shapeObj_simplify(shapeObj *self, double tolerance);
shapeObj        *shapeObj_topologypreservingsimplify(shapeObj *self, double tolerance);
shapeObj        *shapeObj_convexHull(shapeObj *self);
shapeObj        *shapeObj_boundary(shapeObj *self);
int             shapeObj_contains_geos(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_Union(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_intersection(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_difference(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_symdifference(shapeObj *self, shapeObj *poshape);
int             shapeObj_overlaps(shapeObj *self, shapeObj *shape);
int             shapeObj_within(shapeObj *self, shapeObj *shape);
int             shapeObj_crosses(shapeObj *self, shapeObj *shape);
int             shapeObj_touches(shapeObj *self, shapeObj *shape);
int             shapeObj_equals(shapeObj *self, shapeObj *shape);
int             shapeObj_disjoint(shapeObj *self, shapeObj *shape);
pointObj        *shapeObj_getcentroid(shapeObj *self);
double          shapeObj_getarea(shapeObj *self);
double          shapeObj_getlength(shapeObj *self);
pointObj        *shapeObj_getLabelPoint(shapeObj *self);

rectObj        *rectObj_new();
void            rectObj_destroy(rectObj *self);
int             rectObj_project(rectObj *self, projectionObj *in,
                                projectionObj *out);
double          rectObj_fit(rectObj *self, int width, int height);
int             rectObj_draw(rectObj *self, mapObj *map, layerObj *layer,
                             imageObj *img, int classindex, char *text);


shapefileObj   *shapefileObj_new(char *filename, int type);
void            shapefileObj_destroy(shapefileObj *self);
int             shapefileObj_get(shapefileObj *self, int i, shapeObj *shape);
int             shapefileObj_getPoint(shapefileObj *self, int i, pointObj *point);
int             shapefileObj_getTransformed(shapefileObj *self, mapObj *map,
    int i, shapeObj *shape);
void            shapefileObj_getExtent(shapefileObj *self, int i,
                                       rectObj *rect);
int             shapefileObj_add(shapefileObj *self, shapeObj *shape);
int             shapefileObj_addPoint(shapefileObj *self, pointObj *point);

projectionObj   *projectionObj_new(char *string);
projectionObj   *projectionObj_clone(projectionObj *projection);
int             projectionObj_getUnits(projectionObj *self);
void            projectionObj_destroy(projectionObj *self);

void            labelCacheObj_freeCache(labelCacheObj *self);

char           *DBFInfo_getFieldName(DBFInfo *self, int iField);
int             DBFInfo_getFieldWidth(DBFInfo *self, int iField);
int             DBFInfo_getFieldDecimals(DBFInfo *self, int iField);
DBFFieldType    DBFInfo_getFieldType(DBFInfo *self, int iField);

styleObj       *styleObj_new(classObj *class, styleObj *style);
styleObj       *styleObj_label_new(labelObj *label, styleObj *style);
int             styleObj_updateFromString(styleObj *self, char *snippet);
int             styleObj_setSymbolByName(styleObj *self, mapObj *map,
    char* pszSymbolName);
styleObj       *styleObj_clone(styleObj *style);
void           styleObj_setGeomTransform(styleObj *style, char *transform);

hashTableObj   *hashTableObj_new();
int             hashTableObj_set(hashTableObj *self, const char *key,
                                 const char *value);
const char     *hashTableObj_get(hashTableObj *self, const char *key);
int            hashTableObj_remove(hashTableObj *self, const char *key);
void           hashTableObj_clear(hashTableObj *self);
char           *hashTableObj_nextKey(hashTableObj *self, const char *prevkey);


cgiRequestObj *cgirequestObj_new();
int cgirequestObj_loadParams(cgiRequestObj *self,
                             char* (*getenv2)(const char*, void* thread_context),
                             char *raw_post_data,
                             ms_uint32 raw_post_data_length,
                             void* thread_context);
void cgirequestObj_setParameter(cgiRequestObj *self, char *name, char *value);
void cgirequestObj_addParameter(cgiRequestObj *self, char *name, char *value);
char *cgirequestObj_getName(cgiRequestObj *self, int index);
char *cgirequestObj_getValue(cgiRequestObj *self, int index);
char *cgirequestObj_getValueByName(cgiRequestObj *self, const char *name);
void cgirequestObj_destroy(cgiRequestObj *self);

resultObj *resultObj_new();

int clusterObj_updateFromString(clusterObj *self, char *snippet);
int clusterObj_setGroup(clusterObj *self, char *string);
char *clusterObj_getGroupString(clusterObj *self);
int clusterObj_setFilter(clusterObj *self, char *string);
char *clusterObj_getFilterString(clusterObj *self);

outputFormatObj *outputFormatObj_new(const char *driver, char *name);
void  outputFormatObj_destroy(outputFormatObj* self);

int symbolObj_setImage(symbolObj *self, imageObj *image);
imageObj *symbolObj_getImage(symbolObj *self, outputFormatObj *input_format);

#endif /* PHP_MAPSCRIPT_H */
