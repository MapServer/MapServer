/**********************************************************************
 * $Id$
 *
 * Name:     php_mapscript.c
 * Project:  PHP/MapScript extension for MapServer.
 * Language: ANSI C
 * Purpose:  External interface functions
 * Author:   Daniel Morissette, morissette@dmsolutions.ca
 *
 **********************************************************************
 * Copyright (c) 2000, 2001, Daniel Morissette, DM Solutions Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log$
 * Revision 1.57  2001/10/12 00:34:26  assefa
 * Add utility function getAllGroupNames and getAllLayerNames.
 *
 * Revision 1.56  2001/10/11 02:21:09  assefa
 * Add missing connection types constants.
 *
 * Revision 1.55  2001/10/10 16:06:28  dan
 * Added shapeObj->set()
 *
 * Revision 1.54  2001/10/04 18:13:07  dan
 * Fixed ms_NewLayerObj() to update $map->numlayers.
 *
 * Revision 1.53  2001/10/03 12:41:04  assefa
 * Add function getLayersIndexByGroup.
 *
 * Revision 1.52  2001/09/24 15:59:03  assefa
 * Modify GetDeltaExtentsUsingScale to fit with msCalculateScale (mapscale.c) :
 * this corrects the zoomscale bug (#46).
 *
 * Revision 1.51  2001/09/13 20:58:01  dan
 * Fixed handling of objects and refcounts vs PHP4.0.6... thanks to Zeev Suraski
 * for his help.  See bug#30 and #40.
 *
 * Revision 1.50  2001/09/10 15:37:24  assefa
 * Add error messages in the zoom functions when the extents given are
 * wrong.
 *
 * Revision 1.49  2001/08/29 14:36:06  dan
 * Changes to msCalculateScale() args.  Sync with mapscript.i v1.42
 *
 * Revision 1.48  2001/08/21 19:09:02  dan
 * Made map->draw() and drawQuery() produce only a PHP warning so that the map
 * rendering errors can be trapped by the scripts using the '@' operator.
 *
 * Revision 1.47  2001/08/01 13:52:59  dan
 * Sync with mapscript.i v1.39: add QueryByAttributes() and take out type arg
 * to getSymbolByName().
 *
 * Revision 1.46  2001/07/26 19:50:08  assefa
 * Add projection class and related functions.
 *
 * Revision 1.45  2001/07/23 19:11:56  dan
 * Added missing "background..." properties in label_setProperty().
 *
 * Revision 1.44  2001/07/20 13:50:27  dan
 * Call zend_list_addref() when creating resource member variables
 *
 * Revision 1.43  2001/04/19 15:11:34  dan
 * Sync with mapscript.i v.1.32
 *
 * ...
 *
 * Revision 1.1  2000/05/09 21:06:11  dan
 * Initial Import
 *
 * Revision 1.1  2000/01/31 08:38:43  daniel
 * First working version - only mapObj implemented with read-only properties
 *
 **********************************************************************/

#include "php_mapscript.h"
#include "php_mapscript_util.h"

#ifdef PHP4
#include "php.h"
#include "php_globals.h"
#else
#include "phpdl.h"
#include "php3_list.h"
#include "functions/head.h"   /* php3_header() */
#endif

#include "maperror.h"

#include <time.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#else
#include <errno.h>
#endif

#define PHP3_MS_VERSION "(Oct 10, 2001)"

#ifdef PHP4
#define ZEND_DEBUG 0
#endif

#ifndef DLEXPORT 
#define DLEXPORT ZEND_DLEXPORT
#endif

/*=====================================================================
 *                         Prototypes
 *====================================================================*/

#ifdef ZEND_VERSION
PHP_MINFO_FUNCTION(mapscript);
#else
DLEXPORT void php3_info_mapscript(void);
#endif

DLEXPORT int  php3_init_mapscript(INIT_FUNC_ARGS);
DLEXPORT int  php3_end_mapscript(SHUTDOWN_FUNC_ARGS);

DLEXPORT void php3_ms_free_mapObj(mapObj *pMap);
DLEXPORT void php3_ms_free_image(gdImagePtr im);
DLEXPORT void php3_ms_free_point(pointObj *pPoint);
DLEXPORT void php3_ms_free_line(lineObj *pLine);
DLEXPORT void php3_ms_free_shape(shapeObj *pShape);
DLEXPORT void php3_ms_free_shapefile(shapefileObj *pShapefile);
DLEXPORT void php3_ms_free_rect(rectObj *pRect);
DLEXPORT void php3_ms_free_stub(void *ptr) ;
DLEXPORT void php3_ms_free_projection(projectionObj *pProj);

DLEXPORT void php3_ms_getversion(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setProjection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_addColor(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getSymbolByName(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawQuery(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_embedScalebar(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_embedLegend(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawLabelCache(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawLegend(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawReferenceMap(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawScaleBar(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getLayer(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getLayerByName(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getLayersIndexByGroup(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getAllLayerNames(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getAllGroupNames(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_prepareImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_prepareQuery(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_nextLabel(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getColorByIndex(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_queryByPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_queryByRect(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_queryByFeatures(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_queryByShape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_save(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setMetaData(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_setExtent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomRectangle(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomScale(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_getLatLongExtent(INTERNAL_FUNCTION_PARAMETERS);


DLEXPORT void php3_ms_img_saveImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_img_saveWebImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_img_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_lyr_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_drawQuery(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getClass(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_queryByPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_queryByAttributes(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_queryByRect(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_queryByFeatures(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_queryByShape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setProjection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_addFeature(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getNumResults(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getResult(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_open(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_close(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getShape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setMetaData(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_class_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setExpression(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setText(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_label_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_color_setRGB(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_rect_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_project(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_setExtent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_fit(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_point_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_setXY(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_project(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_distanceToPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_distanceToLine(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_distanceToShape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_line_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_project(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_add(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_addXY(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_point(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_shape_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_project(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_add(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_line(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_contains(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_intersects(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_getvalue(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_shapefile_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_addshape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_addpoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getshape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getpoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_gettransformed(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getextent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_web_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_referenceMap_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_projection_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_projection_free(INTERNAL_FUNCTION_PARAMETERS);

static long _phpms_build_img_object(gdImagePtr im, webObj *pweb,
                                    HashTable *list, pval *return_value);
static long _phpms_build_layer_object(layerObj *player, int parent_map_id,
                                      HashTable *list, pval *return_value);
static long _phpms_build_class_object(classObj *pclass, int parent_layer_id,
                                      HashTable *list, pval *return_value);
static long _phpms_build_label_object(labelObj *plabel,
                                      HashTable *list, pval *return_value);

static long _phpms_build_color_object(colorObj *pcolor,
                                      HashTable *list, pval *return_value);
static long _phpms_build_point_object(pointObj *ppoint, int handle_type,
                                      HashTable *list, pval *return_value);
static long _phpms_build_shape_object(shapeObj *pshape, int handle_type,
                                      layerObj *pLayer,
                                      HashTable *list, pval *return_value);
static long _phpms_build_web_object(webObj *pweb,
                                    HashTable *list, pval *return_value);
static long _phpms_build_rect_object(rectObj *prect, int handle_type,
                                     HashTable *list, pval *return_value);
static long _phpms_build_referenceMap_object(referenceMapObj *preferenceMap,
                                          HashTable *list, pval *return_value);
static long _phpms_build_resultcachemember_object(resultCacheMemberObj *pRes,
                                                  HashTable *list, 
                                                  pval *return_value);

static long _phpms_build_projection_object(projectionObj *pproj, 
                                           int handle_type, HashTable *list, 
                                           pval *return_value);

/* ==================================================================== */
/*      utility functions prototypes.                                   */
/* ==================================================================== */
DLEXPORT void php3_ms_getcwd(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_getpid(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_getscale(INTERNAL_FUNCTION_PARAMETERS);


static double GetDeltaExtentsUsingScale(double dfMinscale, int nUnits, 
                                        int nWidth, int resolution);

/*=====================================================================
 *               PHP Dynamically Loadable Library stuff
 *====================================================================*/

#define PHPMS_GLOBAL(a) a
static int le_msmap;
static int le_mslayer;
static int le_msimg;
static int le_msclass;
static int le_mslabel;
static int le_mscolor;
static int le_msrect_new;
static int le_msrect_ref;
static int le_mspoint_new;
static int le_mspoint_ref;
static int le_msline_new;
static int le_msline_ref;
static int le_msshape_new;
static int le_msshape_ref;
static int le_msshapefile;
static int le_msweb;
static int le_msrefmap;
static int le_msprojection_new;

static char tmpId[128]; /* big enough for time + pid */
static int  tmpCount;

/* -------------------------------------------------------------------- */
/*      class entries.                                                  */
/* -------------------------------------------------------------------- */
#ifdef PHP4
static zend_class_entry *map_class_entry_ptr;
static zend_class_entry *img_class_entry_ptr;
static zend_class_entry *rect_class_entry_ptr;
static zend_class_entry *color_class_entry_ptr;
static zend_class_entry *web_class_entry_ptr;
static zend_class_entry *reference_class_entry_ptr;
static zend_class_entry *layer_class_entry_ptr;
static zend_class_entry *label_class_entry_ptr;
static zend_class_entry *class_class_entry_ptr;
static zend_class_entry *point_class_entry_ptr;
static zend_class_entry *line_class_entry_ptr;
static zend_class_entry *shape_class_entry_ptr;
static zend_class_entry *shapefile_class_entry_ptr;
static zend_class_entry *projection_class_entry_ptr;

#endif

function_entry php3_ms_functions[] = {
    {"ms_getversion",   php3_ms_getversion,     NULL},
    {"ms_newmapobj",    php3_ms_map_new,        NULL},
    {"ms_newlayerobj",  php3_ms_lyr_new,        NULL},
    {"ms_newclassobj",  php3_ms_class_new,      NULL},
    {"ms_newpointobj",  php3_ms_point_new,      NULL},
    {"ms_newlineobj",   php3_ms_line_new,       NULL},
    {"ms_newshapeobj",  php3_ms_shape_new,      NULL},
    {"ms_newshapefileobj", php3_ms_shapefile_new,  NULL},
    {"ms_newrectobj",   php3_ms_rect_new,       NULL},
    {"ms_getcwd",       php3_ms_getcwd,         NULL},
    {"ms_getpid",       php3_ms_getpid,         NULL},
    {"ms_getscale",     php3_ms_getscale,       NULL},
    {"ms_newprojectionobj",   php3_ms_projection_new,       NULL},
    {NULL, NULL, NULL}
};


php3_module_entry php3_ms_module_entry = {
    "MapScript", php3_ms_functions, php3_init_mapscript, php3_end_mapscript,
    NULL, NULL,
#ifdef ZEND_VERSION
    PHP_MINFO(mapscript),
#else
    php3_info_mapscript,
#endif
    STANDARD_MODULE_PROPERTIES 
};


#if COMPILE_DL
DLEXPORT php3_module_entry *get_module(void) { return &php3_ms_module_entry; }
#endif

/* -------------------------------------------------------------------- */
/*      Class method definitions.                                       */
/*      We use those to initialize objects in both PHP3 and PHP4        */
/*      through _phpms_object_init()                                    */
/* -------------------------------------------------------------------- */
function_entry php_map_class_functions[] = {
    {"set",             php3_ms_map_setProperty,        NULL},
    {"setprojection",   php3_ms_map_setProjection,      NULL},
    {"addcolor",        php3_ms_map_addColor,           NULL},
    {"getsymbolbyname", php3_ms_map_getSymbolByName,    NULL},
    {"draw",            php3_ms_map_draw,               NULL},
    {"drawquery",       php3_ms_map_drawQuery,          NULL},
    {"embedscalebar",   php3_ms_map_embedScalebar,      NULL},
    {"embedlegend",     php3_ms_map_embedLegend,        NULL},
    {"drawlabelcache",  php3_ms_map_drawLabelCache,     NULL},
    {"drawlegend",      php3_ms_map_drawLegend,         NULL},
    {"drawreferencemap",php3_ms_map_drawReferenceMap,   NULL},
    {"drawscalebar",    php3_ms_map_drawScaleBar,       NULL},
    {"getlayer",        php3_ms_map_getLayer,           NULL},
    {"getlayerbyname",  php3_ms_map_getLayerByName,     NULL},
    {"getlayersindexbygroup",  php3_ms_map_getLayersIndexByGroup,     NULL},
    {"getalllayernames",  php3_ms_map_getAllLayerNames,     NULL},
    {"getallgroupnames",  php3_ms_map_getAllGroupNames,     NULL},
    {"getcolorbyindex", php3_ms_map_getColorByIndex,    NULL},
    {"setextent",       php3_ms_map_setExtent,          NULL},
    {"zoompoint",       php3_ms_map_zoomPoint,          NULL},
    {"zoomrectangle",   php3_ms_map_zoomRectangle,      NULL},
    {"zoomscale",       php3_ms_map_zoomScale,          NULL},
    {"querybypoint",    php3_ms_map_queryByPoint,       NULL},
    {"querybyrect",     php3_ms_map_queryByRect,        NULL},
    {"querybyfeatures", php3_ms_map_queryByFeatures,    NULL},
    {"querybyshape",    php3_ms_map_queryByShape,       NULL},
    {"save",            php3_ms_map_save,               NULL},
    {"getlatlongextent", php3_ms_map_getLatLongExtent,  NULL},
    {"getmetadata",     php3_ms_map_getMetaData,        NULL},
    {"setmetadata",     php3_ms_map_setMetaData,        NULL},
    {"prepareimage",    php3_ms_map_prepareImage,       NULL},
    {"preparequery",    php3_ms_map_prepareQuery,       NULL},
    {NULL, NULL, NULL}
};

function_entry php_img_class_functions[] = {
    {"saveimage",       php3_ms_img_saveImage,          NULL},
    {"savewebimage",    php3_ms_img_saveWebImage,       NULL},
    {"free",            php3_ms_img_free,               NULL},    
    {NULL, NULL, NULL}
};

function_entry php_rect_class_functions[] = {
    {"project",         php3_ms_rect_project,           NULL},    
    {"set",             php3_ms_rect_setProperty,       NULL},    
    {"setextent",       php3_ms_rect_setExtent,         NULL},    
    {"draw",            php3_ms_rect_draw,              NULL},
    {"fit",             php3_ms_rect_fit,               NULL},
    {"free",            php3_ms_rect_free,              NULL},    
    {NULL, NULL, NULL}
};

function_entry php_color_class_functions[] = {
    {"setrgb",          php3_ms_color_setRGB,           NULL},    
    {NULL, NULL, NULL}
};

function_entry php_web_class_functions[] = {
    {"set",             php3_ms_web_setProperty,        NULL},    
    {NULL, NULL, NULL}
};

function_entry php_reference_class_functions[] = {
    {"set",             php3_ms_referenceMap_setProperty,NULL},    
    {NULL, NULL, NULL}
};

function_entry php_layer_class_functions[] = {
    {"set",             php3_ms_lyr_setProperty,        NULL},    
    {"draw",            php3_ms_lyr_draw,               NULL},    
    {"drawquery",       php3_ms_lyr_drawQuery,          NULL},    
    {"getclass",        php3_ms_lyr_getClass,           NULL},    
    {"querybyattributes",php3_ms_lyr_queryByAttributes, NULL},    
    {"querybypoint",    php3_ms_lyr_queryByPoint,       NULL},    
    {"querybyrect",     php3_ms_lyr_queryByRect,        NULL},    
    {"querybyfeatures", php3_ms_lyr_queryByFeatures,    NULL},    
    {"querybyshape",    php3_ms_lyr_queryByShape,       NULL},    
    {"setprojection",   php3_ms_lyr_setProjection,      NULL},
    {"addfeature",      php3_ms_lyr_addFeature,         NULL},
    {"getnumresults",   php3_ms_lyr_getNumResults,      NULL},
    {"getresult",       php3_ms_lyr_getResult,          NULL},
    {"open",            php3_ms_lyr_open,               NULL},
    {"close",           php3_ms_lyr_close,              NULL},
    {"getshape",        php3_ms_lyr_getShape,           NULL},
    {"getmetadata",     php3_ms_lyr_getMetaData,        NULL},
    {"setmetadata",     php3_ms_lyr_setMetaData,        NULL},
    {NULL, NULL, NULL}
};

function_entry php_label_class_functions[] = {
    {"set",             php3_ms_label_setProperty,      NULL},    
    {NULL, NULL, NULL}
};

function_entry php_class_class_functions[] = {
    {"set",             php3_ms_class_setProperty,      NULL},    
    {"setexpression",   php3_ms_class_setExpression,    NULL},    
    {"settext",   php3_ms_class_setText,    NULL},    
    {NULL, NULL, NULL}
};

function_entry php_point_class_functions[] = {
    {"setxy",           php3_ms_point_setXY,            NULL},    
    {"project",         php3_ms_point_project,          NULL},    
    {"draw",            php3_ms_point_draw,             NULL},    
    {"distancetopoint", php3_ms_point_distanceToPoint,  NULL},    
    {"distancetoline",  php3_ms_point_distanceToLine,   NULL},    
    {"distancetoshape", php3_ms_point_distanceToShape,  NULL},    
    {"free",            php3_ms_point_free,             NULL},    
    {NULL, NULL, NULL}
};

function_entry php_line_class_functions[] = {
    {"project",         php3_ms_line_project,           NULL},    
    {"add",             php3_ms_line_add,               NULL},    
    {"addxy",           php3_ms_line_addXY,             NULL},    
    {"point",           php3_ms_line_point,             NULL},    
    {"free",            php3_ms_line_free,              NULL},    
    {NULL, NULL, NULL}
};

function_entry php_shape_class_functions[] = {
    {"set",             php3_ms_shape_setProperty,      NULL},    
    {"project",         php3_ms_shape_project,          NULL},    
    {"add",             php3_ms_shape_add,              NULL},    
    {"line",            php3_ms_shape_line,             NULL},    
    {"draw",            php3_ms_shape_draw,             NULL},    
    {"contains",        php3_ms_shape_contains,         NULL},    
    {"intersects",      php3_ms_shape_intersects,       NULL},    
    {"getvalue",        php3_ms_shape_getvalue,         NULL},    
    {"free",            php3_ms_shape_free,             NULL},    
    {NULL, NULL, NULL}
};

function_entry php_shapefile_class_functions[] = {
    {"getshape",        php3_ms_shapefile_getshape,     NULL},    
    {"getpoint",        php3_ms_shapefile_getpoint,     NULL},    
    {"gettransformed",  php3_ms_shapefile_gettransformed, NULL},    
    {"getextent",       php3_ms_shapefile_getextent,    NULL},    
    {"addshape",        php3_ms_shapefile_addshape,     NULL},    
    {"addpoint",        php3_ms_shapefile_addpoint,     NULL},    
    {"free",            php3_ms_shapefile_free,         NULL},    
    {NULL, NULL, NULL}
};

function_entry php_projection_class_functions[] = {
    {"free",            php3_ms_projection_free,              NULL},    
    {NULL, NULL, NULL}
};

#ifdef ZEND_VERSION 
PHP_MINFO_FUNCTION(mapscript)
#else
DLEXPORT void php3_info_mapscript(void) 
#endif
{
    php3_printf("MapScript Version %s<br>\n", PHP3_MS_VERSION);
    php3_printf("%s<br>\n", msGetVersion());
}

DLEXPORT int php3_init_mapscript(INIT_FUNC_ARGS)
{
#ifdef PHP4
    zend_class_entry tmp_class_entry;
#endif

    int const_flag = CONST_CS|CONST_PERSISTENT;

    PHPMS_GLOBAL(le_msmap)  = register_list_destructors(php3_ms_free_mapObj,
                                                        NULL);
    PHPMS_GLOBAL(le_msimg)  = register_list_destructors(php3_ms_free_image,
                                                        NULL);
    PHPMS_GLOBAL(le_mslayer)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msclass)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_mslabel)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_mscolor)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_mspoint_new)= register_list_destructors(php3_ms_free_point,
                                                        NULL);
    PHPMS_GLOBAL(le_mspoint_ref)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msline_new)= register_list_destructors(php3_ms_free_line,
                                                        NULL);
    PHPMS_GLOBAL(le_msline_ref)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msshape_new)= register_list_destructors(php3_ms_free_shape,
                                                        NULL);
    PHPMS_GLOBAL(le_msshape_ref)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msshapefile)= 
                              register_list_destructors(php3_ms_free_shapefile,
                                                        NULL);
    PHPMS_GLOBAL(le_msweb)= register_list_destructors(php3_ms_free_stub,
                                                      NULL);
    PHPMS_GLOBAL(le_msrefmap)= register_list_destructors(php3_ms_free_stub,
                                                             NULL);
    PHPMS_GLOBAL(le_msrect_new)= register_list_destructors(php3_ms_free_rect,
                                                           NULL);
    PHPMS_GLOBAL(le_msrect_ref)= register_list_destructors(php3_ms_free_stub,
                                                           NULL);
    PHPMS_GLOBAL(le_msprojection_new)= 
        register_list_destructors(php3_ms_free_projection, NULL);

    /* boolean constants*/
    REGISTER_LONG_CONSTANT("MS_TRUE",       MS_TRUE,        const_flag);
    REGISTER_LONG_CONSTANT("MS_FALSE",      MS_FALSE,       const_flag);
    REGISTER_LONG_CONSTANT("MS_ON",         MS_ON,          const_flag);
    REGISTER_LONG_CONSTANT("MS_OFF",        MS_OFF,         const_flag);
    REGISTER_LONG_CONSTANT("MS_YES",        MS_YES,         const_flag);
    REGISTER_LONG_CONSTANT("MS_NO",         MS_NO,          const_flag);

    /* map units constants*/
    REGISTER_LONG_CONSTANT("MS_INCHES",     MS_INCHES,      const_flag);
    REGISTER_LONG_CONSTANT("MS_FEET",       MS_FEET,        const_flag);
    REGISTER_LONG_CONSTANT("MS_MILES",      MS_MILES,       const_flag);
    REGISTER_LONG_CONSTANT("MS_METERS",     MS_METERS,      const_flag);
    REGISTER_LONG_CONSTANT("MS_KILOMETERS", MS_KILOMETERS,  const_flag);
    REGISTER_LONG_CONSTANT("MS_DD",         MS_DD,          const_flag);
    REGISTER_LONG_CONSTANT("MS_PIXELS",     MS_PIXELS,      const_flag);

    /* layer type constants*/
    REGISTER_LONG_CONSTANT("MS_LAYER_POINT",MS_LAYER_POINT, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_LINE", MS_LAYER_LINE,  const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_POLYGON",MS_LAYER_POLYGON, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_POLYLINE",MS_LAYER_POLYLINE, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_RASTER",MS_LAYER_RASTER, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_ANNOTATION",MS_LAYER_ANNOTATION,const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_QUERY",MS_LAYER_QUERY, const_flag);

    /* layer status constants (see also MS_ON, MS_OFF) */
    REGISTER_LONG_CONSTANT("MS_DEFAULT",    MS_DEFAULT,     const_flag);
    REGISTER_LONG_CONSTANT("MS_EMBED",      MS_EMBED,       const_flag);

    /* font type constants*/
    REGISTER_LONG_CONSTANT("MS_TRUETYPE",   MS_TRUETYPE,    const_flag);
    REGISTER_LONG_CONSTANT("MS_BITMAP",     MS_BITMAP,      const_flag);

    /* bitmap font style constants */
    REGISTER_LONG_CONSTANT("MS_TINY",       MS_TINY,        const_flag);
    REGISTER_LONG_CONSTANT("MS_SMALL",      MS_SMALL,       const_flag);
    REGISTER_LONG_CONSTANT("MS_MEDIUM",     MS_MEDIUM,      const_flag);
    REGISTER_LONG_CONSTANT("MS_LARGE",      MS_LARGE,       const_flag);
    REGISTER_LONG_CONSTANT("MS_GIANT",      MS_GIANT,       const_flag);

    /* label position constants*/
    REGISTER_LONG_CONSTANT("MS_UL",         MS_UL,          const_flag);
    REGISTER_LONG_CONSTANT("MS_LR",         MS_LR,          const_flag);
    REGISTER_LONG_CONSTANT("MS_UR",         MS_UR,          const_flag);
    REGISTER_LONG_CONSTANT("MS_LL",         MS_LL,          const_flag);
    REGISTER_LONG_CONSTANT("MS_CR",         MS_CR,          const_flag);
    REGISTER_LONG_CONSTANT("MS_CL",         MS_CL,          const_flag);
    REGISTER_LONG_CONSTANT("MS_UC",         MS_UC,          const_flag);
    REGISTER_LONG_CONSTANT("MS_LC",         MS_LC,          const_flag);
    REGISTER_LONG_CONSTANT("MS_AUTO",       MS_AUTO,        const_flag);
    REGISTER_LONG_CONSTANT("MS_XY",         MS_XY,          const_flag);

    /* shape type constants*/
    REGISTER_LONG_CONSTANT("MS_SHAPE_POINT",MS_SHAPE_POINT, const_flag);
    REGISTER_LONG_CONSTANT("MS_SHAPE_LINE",  MS_SHAPE_LINE, const_flag);
    REGISTER_LONG_CONSTANT("MS_SHAPE_POLYGON",MS_SHAPE_POLYGON, const_flag);
    REGISTER_LONG_CONSTANT("MS_SHAPE_NULL", MS_SHAPE_NULL,  const_flag);

    /* shapefile type constants*/
    /* Old names ... */
    REGISTER_LONG_CONSTANT("MS_SHP_POINT",  SHP_POINT,      const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_ARC",    SHP_ARC,        const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_POLYGON",SHP_POLYGON,    const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_MULTIPOINT",SHP_MULTIPOINT, const_flag);
    /* new names??? */
    REGISTER_LONG_CONSTANT("SHP_POINT",     SHP_POINT,      const_flag);
    REGISTER_LONG_CONSTANT("SHP_ARC",       SHP_ARC,        const_flag);
    REGISTER_LONG_CONSTANT("SHP_POLYGON",   SHP_POLYGON,    const_flag);
    REGISTER_LONG_CONSTANT("SHP_MULTIPOINT",SHP_MULTIPOINT, const_flag);

    /* query/join type constants*/
    REGISTER_LONG_CONSTANT("MS_SINGLE",     MS_SINGLE,      const_flag);
    REGISTER_LONG_CONSTANT("MS_MULTIPLE",   MS_MULTIPLE,    const_flag);

    /* connection type constants*/
    REGISTER_LONG_CONSTANT("MS_INLINE",     MS_INLINE,      const_flag);
    REGISTER_LONG_CONSTANT("MS_SHAPEFILE",  MS_SHAPEFILE,   const_flag);
    REGISTER_LONG_CONSTANT("MS_TILED_SHAPEFILE",MS_TILED_SHAPEFILE,const_flag);
    REGISTER_LONG_CONSTANT("MS_SDE",        MS_SDE,         const_flag);
    REGISTER_LONG_CONSTANT("MS_OGR",        MS_OGR,         const_flag);
    REGISTER_LONG_CONSTANT("MS_TILED_OGR",  MS_TILED_OGR,   const_flag);
    REGISTER_LONG_CONSTANT("MS_POSTGIS",    MS_POSTGIS,     const_flag);
    REGISTER_LONG_CONSTANT("MS_WMS",        MS_WMS,         const_flag);
    REGISTER_LONG_CONSTANT("MS_ORACLESPATIAL", MS_ORACLESPATIAL,const_flag);
 
    /* output image type constants*/
    REGISTER_LONG_CONSTANT("MS_GIF",        MS_GIF,         const_flag);
    REGISTER_LONG_CONSTANT("MS_PNG",        MS_PNG,         const_flag);
    REGISTER_LONG_CONSTANT("MS_JPEG",       MS_JPEG,        const_flag);
    REGISTER_LONG_CONSTANT("MS_WBMP",       MS_WBMP,        const_flag);

    /* querymap style constants */
    REGISTER_LONG_CONSTANT("MS_NORMAL",     MS_NORMAL,      const_flag);
    REGISTER_LONG_CONSTANT("MS_HILITE",     MS_HILITE,      const_flag);
    REGISTER_LONG_CONSTANT("MS_SELECTED",   MS_SELECTED,    const_flag);

    /* return value constants */
    REGISTER_LONG_CONSTANT("MS_SUCCESS",    MS_SUCCESS,     const_flag);
    REGISTER_LONG_CONSTANT("MS_FAILURE",    MS_FAILURE,     const_flag);
    REGISTER_LONG_CONSTANT("MS_DONE",       MS_DONE,        const_flag);
   
    /* We'll use tmpId and tmpCount to generate unique filenames */
    sprintf(PHPMS_GLOBAL(tmpId), "%ld%d",(long)time(NULL),(int)getpid());
    tmpCount = 0;

#ifdef PHP4
    INIT_CLASS_ENTRY(tmp_class_entry, "map", php_map_class_functions);
    map_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);
    
    INIT_CLASS_ENTRY(tmp_class_entry, "img", php_img_class_functions);
    img_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "rect", php_rect_class_functions);
    rect_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "color", php_color_class_functions);
    color_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "web", php_web_class_functions);
    web_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "reference", 
                     php_reference_class_functions);
    reference_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);
    
    INIT_CLASS_ENTRY(tmp_class_entry, "layer", php_layer_class_functions);
    layer_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "label", php_label_class_functions);
    label_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "class", php_class_class_functions);
    class_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "point", php_point_class_functions);
    point_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "line", php_line_class_functions);
    line_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "shape", php_shape_class_functions);
    shape_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);
    
    INIT_CLASS_ENTRY(tmp_class_entry, "shapefile", 
                     php_shapefile_class_functions);
    shapefile_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

    INIT_CLASS_ENTRY(tmp_class_entry, "projection", 
                     php_projection_class_functions);
    projection_class_entry_ptr = zend_register_internal_class(&tmp_class_entry);

#endif

    return SUCCESS;
}

DLEXPORT int php3_end_mapscript(SHUTDOWN_FUNC_ARGS)
{
    return SUCCESS;
}


/**********************************************************************
 *                     resource list destructors
 **********************************************************************/
DLEXPORT void php3_ms_free_mapObj(mapObj *pMap) 
{
    mapObj_destroy(pMap);
}

DLEXPORT void php3_ms_free_image(gdImagePtr im) 
{
    msFreeImage(im);
}

DLEXPORT void php3_ms_free_rect(rectObj *pRect) 
{
    rectObj_destroy(pRect);
}

DLEXPORT void php3_ms_free_point(pointObj *pPoint) 
{
    pointObj_destroy(pPoint);
}

DLEXPORT void php3_ms_free_line(lineObj *pLine) 
{
    lineObj_destroy(pLine);
}

DLEXPORT void php3_ms_free_shape(shapeObj *pShape) 
{
    shapeObj_destroy(pShape);
}

DLEXPORT void php3_ms_free_shapefile(shapefileObj *pShapefile) 
{
    shapefileObj_destroy(pShapefile);
}


DLEXPORT void php3_ms_free_stub(void *ptr)
{
    /* nothing to do... map destructor takes care of all objects. */
}

DLEXPORT void php3_ms_free_projection(projectionObj *pProj) 
{
    projectionObj_destroy(pProj);
}

/*=====================================================================
 *                 PHP function wrappers
 *====================================================================*/

/************************************************************************/
/*                         php3_ms_getversion()                         */
/*                                                                      */
/*      Returns a string with MapServer version and configuration       */
/*      params.                                                         */
/************************************************************************/
DLEXPORT void php3_ms_getversion(INTERNAL_FUNCTION_PARAMETERS)
{
    RETURN_STRING(msGetVersion(), 1);
}



/*=====================================================================
 *                 PHP function wrappers - mapObj class
 *====================================================================*/

/**********************************************************************
 *                        ms_newMapObj()
 **********************************************************************/

/* {{{ proto mapObj ms_newMapObj(string filename)
   Returns a new object to deal with a MapServer map file. */

DLEXPORT void php3_ms_map_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pFname;
    mapObj      *pNewObj = NULL;
    int         map_id;
#ifdef PHP4
    pval        *new_obj_ptr;
    HashTable   *list=NULL;
#else
    pval        new_obj_param;  /* No, it's not a pval * !!! */
    pval        *new_obj_ptr;
    new_obj_ptr = &new_obj_param;
#endif

    if (getParameters(ht, 1, &pFname) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    /* Attempt to open the MAP file 
     */
    convert_to_string(pFname);
#if defined(PHP4) && defined(WIN32)
    /* With PHP4 on WINNT, we have to use the virtual_cwd API... for now we'll
     * just make sure that the .map file path is absolute, but what we
     * should really do is make all of MapServer use the V_* macros and
     * avoid calling setcwd() from anywhere.
     */
    if (IS_ABSOLUTE_PATH(pFname->value.str.val, pFname->value.str.len))
        pNewObj = mapObj_new(pFname->value.str.val);
    else
    {
        char    szFname[MAXPATHLEN];
        if (virtual_getcwd(szFname, MAXPATHLEN) != NULL)
        {
            strcat(szFname, "\\");
            strcat(szFname, pFname->value.str.val);
            pNewObj = mapObj_new(szFname);
        }
    }
#else
    pNewObj = mapObj_new(pFname->value.str.val);
#endif
    if (pNewObj == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed to open map file %s", 
                            pFname->value.str.val);
        RETURN_FALSE;
    }

    /* Create a PHP object, add all mapObj methods, etc.
     */
    map_id = php3_list_insert(pNewObj, PHPMS_GLOBAL(le_msmap));

    _phpms_object_init(return_value, map_id, php_map_class_functions,
                       PHP4_CLASS_ENTRY(map_class_entry_ptr));

    /* read-only properties */
    add_property_long(return_value, "numlayers", pNewObj->numlayers);

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value, "name",      pNewObj->name);
    add_property_long(return_value,  "status",    pNewObj->status);
    add_property_long(return_value,  "width",     pNewObj->width);
    add_property_long(return_value,  "height",    pNewObj->height);
    add_property_long(return_value,  "transparent", pNewObj->transparent);
    add_property_long(return_value,  "interlace", pNewObj->interlace);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
#endif
    _phpms_build_rect_object(&(pNewObj->extent), PHPMS_GLOBAL(le_msrect_ref),
                             list, new_obj_ptr);
    _phpms_add_property_object(return_value, "extent", new_obj_ptr, E_ERROR);

    add_property_double(return_value,"cellsize",  pNewObj->cellsize);
    add_property_long(return_value,  "units",     pNewObj->units);
    add_property_double(return_value,"scale",     pNewObj->scale);
    PHPMS_ADD_PROP_STR(return_value, "shapepath", pNewObj->shapepath);
    add_property_long(return_value,  "keysizex",  pNewObj->legend.keysizex);
    add_property_long(return_value,  "keysizey",  pNewObj->legend.keysizey);
    add_property_long(return_value, "keyspacingx",pNewObj->legend.keyspacingx);
    add_property_long(return_value, "keyspacingy",pNewObj->legend.keyspacingy);


#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
#endif
    _phpms_build_color_object(&(pNewObj->imagecolor),list, new_obj_ptr);
    _phpms_add_property_object(return_value, "imagecolor",new_obj_ptr,E_ERROR);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
#endif
    _phpms_build_web_object(&(pNewObj->web), list, new_obj_ptr);
    _phpms_add_property_object(return_value, "web", new_obj_ptr, E_ERROR);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
#endif
    _phpms_build_referenceMap_object(&(pNewObj->reference), list, 
                                     new_obj_ptr);
    _phpms_add_property_object(return_value, "reference", new_obj_ptr,E_ERROR);

    return;
}
/* }}} */


/**********************************************************************
 *                        map->set()
 **********************************************************************/

/* {{{ proto int map.set(string property_name, new_value)
   Set map object property to new value. Returns -1 on error. */

DLEXPORT void php3_ms_map_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pPropertyName, *pNewValue;
    pval *pThis;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);


    IF_SET_STRING(     "name",        self->name)
    else IF_SET_LONG(  "status",      self->status)
    else IF_SET_LONG(  "width",       self->width)
    else IF_SET_LONG(  "height",      self->height)
    else IF_SET_LONG(  "transparent", self->transparent)
    else IF_SET_LONG(  "interlace",   self->interlace)
    else IF_SET_DOUBLE("cellsize",    self->cellsize)
    else IF_SET_LONG(  "units",       self->units)
    else IF_SET_DOUBLE("scale",       self->scale)
    else IF_SET_STRING("shapepath",   self->shapepath)
    else IF_SET_LONG(  "keysizex",    self->legend.keysizex)
    else IF_SET_LONG(  "keysizey",    self->legend.keysizey)
    else IF_SET_LONG(  "keyspacingx", self->legend.keyspacingx)
    else IF_SET_LONG(  "keyspacingy", self->legend.keyspacingy)
    else if (strcmp( "numlayers", pPropertyName->value.str.val) == 0 ||
             strcmp( "extent", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        map->setExtent()
 **********************************************************************/

/* {{{ proto int map.setextent(string property_name, new_value)
   Set map extent property to new value. Returns -1 on error. */

DLEXPORT void php3_ms_map_setExtent(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
#ifdef PHP4
    pval   **pExtent;
#else
    pval   *pExtent;
#endif

    pval   *pMinX, *pMinY, *pMaxX, *pMaxY;
    pval *pThis;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif
    

    if (pThis == NULL ||
        getParameters(ht, 4, &pMinX, &pMinY, &pMaxX, &pMaxY) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pMinX);
    convert_to_double(pMinY);
    convert_to_double(pMaxX);
    convert_to_double(pMaxY);

    self->extent.minx = pMinX->value.dval;
    self->extent.miny = pMinY->value.dval;
    self->extent.maxx = pMaxX->value.dval;
    self->extent.maxy = pMaxY->value.dval;
    
    self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                    self->height);      
    if (msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scale)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR); 
    _phpms_set_property_double(pThis,"scale", self->scale, E_ERROR); 

#ifdef PHP4
    if (zend_hash_find(pThis->value.obj.properties, "extent", sizeof("extent"), 
                       (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#else
    if (_php3_hash_find(pThis->value.ht, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double(pExtent,"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#endif

}

/**********************************************************************
 *                        map->setProjection()
 **********************************************************************/

/* {{{ proto int map.setProjection(string projection)
   Set projection and coord. system for the map. Returns -1 on error. */

DLEXPORT void php3_ms_map_setProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pProjString;
    pval   *pThis;
    int     nStatus = 0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pProjString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pProjString);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    if (self == NULL || 
        (nStatus = mapObj_setProjection(self, 
                                        pProjString->value.str.val)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(nStatus);
}
/* }}} */

/* ==================================================================== */
/*      Zoom related functions.                                         */
/* ==================================================================== */

/************************************************************************/
/*    static double Pix2Georef(int nPixPos, int nPixMin, double nPixMax,*/
/*                              double dfGeoMin, double dfGeoMax,       */
/*                              bool bULisYOrig)                        */
/*                                                                      */
/*        Utility function to convert a pixel pos to georef pos. If     */
/*      bULisYOrig parameter is set to true then the upper left is      */
/*      considered to be the Y origin.                                  */
/*                                                                      */
/************************************************************************/
static double Pix2Georef(int nPixPos, int nPixMin, int nPixMax, 
                         double dfGeoMin, double dfGeoMax, int bULisYOrig)

{
    double      dfWidthGeo = 0.0;
    int         nWidthPix = 0;
    double      dfPixToGeo = 0.0;
    double      dfPosGeo = 0.0;
    double      dfDeltaGeo = 0.0;
    int         nDeltaPix = 0;

    dfWidthGeo = dfGeoMax - dfGeoMin;
    nWidthPix = nPixMax - nPixMin;
   
    if (dfWidthGeo > 0.0 && nWidthPix > 0)
    {
        dfPixToGeo = dfWidthGeo / (double)nWidthPix;

        if (!bULisYOrig)
            nDeltaPix = nPixPos - nPixMin;
        else
            nDeltaPix = nPixMax - nPixPos;
        
        dfDeltaGeo = nDeltaPix * dfPixToGeo;

        dfPosGeo = dfGeoMin + dfDeltaGeo;
    }
    return (dfPosGeo);
}

/************************************************************************/
/*                           map->zoomPoint()                           */
/*                                                                      */
/*      Parmeters are :                                                 */
/*                                                                      */
/*       - Zoom factor : positive values do zoom in, negative values    */
/*                      zoom out. Factor of 1 will recenter.            */
/*                                                                      */
/*       - Pixel position (pointObj) : x, y coordinates of the click.   */
/*       - Width : width in pixel of the current image.                 */
/*       - Height : Height in pixel of the current image.               */
/*       - Georef extent (rectObj) : current georef extents.            */
/*       - MaxGeoref extent (rectObj) : (optional) maximum georef       */
/*                                      extents.                        */
/*                                                                      */
/************************************************************************/
DLEXPORT void php3_ms_map_zoomPoint(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj      *self;
    pval        *pThis;
#ifdef PHP4
    pval   **pExtent;
#else
    pval   *pExtent;
#endif
    pval        *pZoomFactor;
    pval        *pPixelPos;
    pval        *pWidth, *pHeight;
    pval        *pGeorefExt;
    pval        *pMaxGeorefExt;
    rectObj     *poGeorefExt = NULL;
    rectObj     *poMaxGeorefExt = NULL;
    pointObj    *poPixPos = NULL;
    double      dfGeoPosX, dfGeoPosY;
    double      dfDeltaX, dfDeltaY;
    rectObj     oNewGeorefExt;    
    double      dfNewScale = 0.0;
    int         bMaxExtSet = 0;
    int         nArgs = ARG_COUNT(ht);
    double      dfDeltaExt = -1.0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||   
        (nArgs != 5 && nArgs != 6))
    {
        WRONG_PARAM_COUNT;
    }
    
    if (getParameters(ht, nArgs, 
                      &pZoomFactor, &pPixelPos, &pWidth, &pHeight,
                      &pGeorefExt, &pMaxGeorefExt) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }



    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    if (nArgs == 6)
        bMaxExtSet =1;

    convert_to_long(pZoomFactor);
    convert_to_long(pWidth);
    convert_to_long(pHeight);
         
    poGeorefExt = (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                                  PHPMS_GLOBAL(le_msrect_ref),
                                                  PHPMS_GLOBAL(le_msrect_new),
                                                  list);
    poPixPos = (pointObj *)_phpms_fetch_handle2(pPixelPos, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list);
    
    if (bMaxExtSet)
        poMaxGeorefExt = 
            (rectObj *)_phpms_fetch_handle2(pMaxGeorefExt, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list);

/* -------------------------------------------------------------------- */
/*      check the validity of the parameters.                           */
/* -------------------------------------------------------------------- */
    if (pZoomFactor->value.lval == 0 || 
        pWidth->value.lval <= 0 ||
        pHeight->value.lval <= 0 ||
        poGeorefExt == NULL ||
        poPixPos == NULL ||
        (bMaxExtSet && poMaxGeorefExt == NULL))
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomPoint failed : incorrect parameters\n");
        RETURN_FALSE;
    }

/* -------------------------------------------------------------------- */
/*      check if the values passed are consistent min > max.             */
/* -------------------------------------------------------------------- */
    if (poGeorefExt->minx >= poGeorefExt->maxx)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomPoint failed : Georeferenced coordinates minx >= maxx\n");
    }
    if (poGeorefExt->miny >= poGeorefExt->maxy)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomPoint failed : Georeferenced coordinates miny >= maxy\n");
    }
    if (bMaxExtSet)
    {
        if (poMaxGeorefExt->minx >= poMaxGeorefExt->maxx)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "zoomPoint failed : Max Georeferenced coordinates minx >= maxx\n");
        }
        if (poMaxGeorefExt->miny >= poMaxGeorefExt->maxy)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "zoomPoint failed : Max Georeferenced coordinates miny >= maxy\n");
        }
    }

    
    dfGeoPosX = Pix2Georef((int)poPixPos->x, 0, (int)pWidth->value.lval, 
                           poGeorefExt->minx, poGeorefExt->maxx, 0); 
    dfGeoPosY = Pix2Georef((int)poPixPos->y, 0, (int)pHeight->value.lval, 
                           poGeorefExt->miny, poGeorefExt->maxy, 1); 
    dfDeltaX = poGeorefExt->maxx - poGeorefExt->minx;
    dfDeltaY = poGeorefExt->maxy - poGeorefExt->miny;

   
/* -------------------------------------------------------------------- */
/*      zoom in                                                         */
/* -------------------------------------------------------------------- */
    if (pZoomFactor->value.lval > 1)
    {
        oNewGeorefExt.minx = 
            dfGeoPosX - (dfDeltaX/(2*pZoomFactor->value.lval));        
         oNewGeorefExt.miny = 
             dfGeoPosY - (dfDeltaY/(2*pZoomFactor->value.lval));        
         oNewGeorefExt.maxx = 
             dfGeoPosX + (dfDeltaX/(2*pZoomFactor->value.lval));        
         oNewGeorefExt.maxy = 
             dfGeoPosY + (dfDeltaY/(2*pZoomFactor->value.lval));
    }

    if (pZoomFactor->value.lval < 0)
    {
        oNewGeorefExt.minx = 
            dfGeoPosX - (dfDeltaX/2)*(abs(pZoomFactor->value.lval));    
        oNewGeorefExt.miny = 
            dfGeoPosY - (dfDeltaY/2)*(abs(pZoomFactor->value.lval));    
        oNewGeorefExt.maxx = 
            dfGeoPosX + (dfDeltaX/2)*(abs(pZoomFactor->value.lval));    
        oNewGeorefExt.maxy = 
            dfGeoPosY + (dfDeltaY/2)*(abs(pZoomFactor->value.lval));
    }
    if (pZoomFactor->value.lval == 1)
    {
        oNewGeorefExt.minx = dfGeoPosX - (dfDeltaX/2);
        oNewGeorefExt.miny = dfGeoPosY - (dfDeltaY/2);
        oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaX/2);
        oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaY/2);
    }

/* -------------------------------------------------------------------- */
/*      if the min and max scale are set in the map file, we will       */
/*      use them to test before zooming.                                */
/* -------------------------------------------------------------------- */
    msAdjustExtent(&oNewGeorefExt, self->width, self->height);
    if (msCalculateScale(oNewGeorefExt, self->units, 
                         self->width, self->height, 
                         self->resolution, &dfNewScale) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    if (self->web.maxscale > 0)
    {
        if (pZoomFactor->value.lval < 0 && dfNewScale >  self->web.maxscale)
        {
            RETURN_FALSE;
        }
    }

/* ==================================================================== */
/*      we do a spcial case for zoom in : we try to zoom as much as     */
/*      possible using the mincale set in the .map.                     */
/* ==================================================================== */
    if (self->web.minscale > 0 && dfNewScale <  self->web.minscale &&
        pZoomFactor->value.lval > 1)
    {
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscale, self->units, 
                                      self->width, self->resolution);
        if (dfDeltaExt > 0.0)
        {
            oNewGeorefExt.minx = dfGeoPosX - (dfDeltaExt/2);
            oNewGeorefExt.miny = dfGeoPosY - (dfDeltaExt/2);
            oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaExt/2);
            oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaExt/2);
        }
        else
          RETURN_FALSE;
    }
/* -------------------------------------------------------------------- */
/*      If the buffer is set, make sure that the extents do not go      */
/*      beyond the buffer.                                              */
/* -------------------------------------------------------------------- */
    if (bMaxExtSet)
    {
        dfDeltaX = oNewGeorefExt.maxx - oNewGeorefExt.minx;
        dfDeltaY = oNewGeorefExt.maxy - oNewGeorefExt.miny;
        
        /* Make sure Current georef extents is not bigger than max extents */
        if (dfDeltaX > (poMaxGeorefExt->maxx-poMaxGeorefExt->minx))
            dfDeltaX = poMaxGeorefExt->maxx-poMaxGeorefExt->minx;
        if (dfDeltaY > (poMaxGeorefExt->maxy-poMaxGeorefExt->miny))
            dfDeltaY = poMaxGeorefExt->maxy-poMaxGeorefExt->miny;

        if (oNewGeorefExt.minx < poMaxGeorefExt->minx)
        {
            oNewGeorefExt.minx = poMaxGeorefExt->minx;
            oNewGeorefExt.maxx =  oNewGeorefExt.minx + dfDeltaX;
        }
        if (oNewGeorefExt.maxx > poMaxGeorefExt->maxx)
        {
            oNewGeorefExt.maxx = poMaxGeorefExt->maxx;
            oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
        }
        if (oNewGeorefExt.miny < poMaxGeorefExt->miny)
        {
            oNewGeorefExt.miny = poMaxGeorefExt->miny;
            oNewGeorefExt.maxy =  oNewGeorefExt.miny + dfDeltaY;
        }
        if (oNewGeorefExt.maxy > poMaxGeorefExt->maxy)
        {
            oNewGeorefExt.maxy = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }
    
/* -------------------------------------------------------------------- */
/*      set the map extents with new values.                            */
/* -------------------------------------------------------------------- */
    self->extent.minx = oNewGeorefExt.minx;
    self->extent.miny = oNewGeorefExt.miny;
    self->extent.maxx = oNewGeorefExt.maxx;
    self->extent.maxy = oNewGeorefExt.maxy;
    
    self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                    self->height);      
    dfDeltaX = self->extent.maxx - self->extent.minx;
    dfDeltaY = self->extent.maxy - self->extent.miny; 

    if (bMaxExtSet)
    {
        if (self->extent.minx < poMaxGeorefExt->minx)
        {
            self->extent.minx = poMaxGeorefExt->minx;
            self->extent.maxx = self->extent.minx + dfDeltaX;
        }
        if (self->extent.maxx > poMaxGeorefExt->maxx)
        {
            self->extent.maxx = poMaxGeorefExt->maxx;
            oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
        }
        if (self->extent.miny < poMaxGeorefExt->miny)
        {
            self->extent.miny = poMaxGeorefExt->miny;
            self->extent.maxy =  self->extent.miny + dfDeltaY;
        }
        if (self->extent.maxy > poMaxGeorefExt->maxy)
        {
            self->extent.miny = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }
    
    if (msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scale)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR); 
    _phpms_set_property_double(pThis,"scale", self->scale, E_ERROR); 

#ifdef PHP4
    if (zend_hash_find(pThis->value.obj.properties, "extent", sizeof("extent"), 
                       (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#else
    if (_php3_hash_find(pThis->value.ht, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double(pExtent,"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#endif

     RETURN_TRUE;

}

/************************************************************************/
/*                           map->zoomRectange()                        */
/*                                                                      */
/*      Parmeters are :                                                 */
/*                                                                      */
/*       - Pixel position (rectObj) : extents in pixels.                */
/*       - Width : width in pixel of the current image.                 */
/*       - Height : Height in pixel of the current image.               */
/*       - Georef extent (rectObj) : current georef extents.            */
/*       - MaxGeoref extent (rectObj) : (optional) maximum georef       */
/*                                      extents.                        */
/*                                                                      */
/************************************************************************/
DLEXPORT void php3_ms_map_zoomRectangle(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj      *self;
    pval        *pThis;
#ifdef PHP4
    pval   **pExtent;
#else
    pval   *pExtent;
#endif

    pval        *pPixelExt;
    pval        *pWidth, *pHeight;
    pval        *pGeorefExt;
    pval        *pMaxGeorefExt;
    rectObj     *poGeorefExt = NULL;
    rectObj     *poPixExt = NULL;
    rectObj     *poMaxGeorefExt = NULL;
    double      dfNewScale = 0.0;
    rectObj     oNewGeorefExt;    
    double      dfMiddleX =0.0;
    double      dfMiddleY =0.0;
    double      dfDeltaExt =0.0;
    int         bMaxExtSet = 0;
    int         nArgs = ARG_COUNT(ht);
    double      dfDeltaX = 0;
    double      dfDeltaY = 0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif
    
#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        (nArgs != 4 && nArgs != 5))
    {
        WRONG_PARAM_COUNT;
    }
    if (getParameters(ht, nArgs, &pPixelExt, &pWidth, &pHeight,
                      &pGeorefExt, &pMaxGeorefExt) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    if (nArgs == 5)
      bMaxExtSet =1;

    convert_to_long(pWidth);
    convert_to_long(pHeight);
    
    poGeorefExt = (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                                  PHPMS_GLOBAL(le_msrect_ref),
                                                  PHPMS_GLOBAL(le_msrect_new),
                                                  list);
    poPixExt = (rectObj *)_phpms_fetch_handle2(pPixelExt, 
                                               PHPMS_GLOBAL(le_msrect_ref), 
                                               PHPMS_GLOBAL(le_msrect_new),
                                               list);

    if (bMaxExtSet)
        poMaxGeorefExt = 
            (rectObj *)_phpms_fetch_handle2(pMaxGeorefExt, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list);
/* -------------------------------------------------------------------- */
/*      check the validity of the parameters.                           */
/* -------------------------------------------------------------------- */
    if (pWidth->value.lval <= 0 ||
        pHeight->value.lval <= 0 ||
        poGeorefExt == NULL ||
        poPixExt == NULL ||
        (bMaxExtSet && poMaxGeorefExt == NULL))
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomRectangle failed : incorrect parameters\n");
            /*RETURN_FALSE;*/
    }

/* -------------------------------------------------------------------- */
/*      check if the values passed are consistent min > max.            */
/* -------------------------------------------------------------------- */
    if (poGeorefExt->minx >= poGeorefExt->maxx)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomRectangle failed : Georeferenced coordinates minx >= maxx\n");
    }
    if (poGeorefExt->miny >= poGeorefExt->maxy)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomRectangle failed : Georeferenced coordinates miny >= maxy\n");
    }
    if (bMaxExtSet)
    {
        if (poMaxGeorefExt->minx >= poMaxGeorefExt->maxx)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "zoomRectangle failed : Max Georeferenced coordinates minx >= maxx\n");
        }
        if (poMaxGeorefExt->miny >= poMaxGeorefExt->maxy)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "zoomRectangle failed : Max Georeferenced coordinates miny >= maxy\n");
        }
    }


    oNewGeorefExt.minx = Pix2Georef((int)poPixExt->minx, 0, 
                                    (int)pWidth->value.lval, 
                                    poGeorefExt->minx, poGeorefExt->maxx, 0); 
    oNewGeorefExt.maxx = Pix2Georef((int)poPixExt->maxx, 0, 
                                    (int)pWidth->value.lval, 
                                    poGeorefExt->minx, poGeorefExt->maxx, 0); 
    oNewGeorefExt.miny = Pix2Georef((int)poPixExt->miny, 0, 
                                    (int)pHeight->value.lval, 
                                    poGeorefExt->miny, poGeorefExt->maxy, 1); 
    oNewGeorefExt.maxy = Pix2Georef((int)poPixExt->maxy, 0, 
                                    (int)pHeight->value.lval, 
                                    poGeorefExt->miny, poGeorefExt->maxy, 1); 
    

    msAdjustExtent(&oNewGeorefExt, self->width, self->height);

/* -------------------------------------------------------------------- */
/*      if the min and max scale are set in the map file, we will       */
/*      use them to test before settting extents.                       */
/* -------------------------------------------------------------------- */
    if (msCalculateScale(oNewGeorefExt, self->units, 
                         self->width, self->height, 
                         self->resolution, &dfNewScale) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    if (self->web.maxscale > 0 &&  dfNewScale > self->web.maxscale)
    {
        RETURN_FALSE;
    }

    if (self->web.minscale > 0 && dfNewScale <  self->web.minscale)
    {
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscale, self->units, 
                                      self->width, self->resolution);
        dfMiddleX = oNewGeorefExt.minx + 
            ((oNewGeorefExt.maxx - oNewGeorefExt.minx)/2);
        dfMiddleY = oNewGeorefExt.miny + 
            ((oNewGeorefExt.maxy - oNewGeorefExt.miny)/2);
        
        if (dfDeltaExt > 0.0)
        {
            oNewGeorefExt.minx = dfMiddleX - (dfDeltaExt/2);
            oNewGeorefExt.miny = dfMiddleY - (dfDeltaExt/2);
            oNewGeorefExt.maxx = dfMiddleX + (dfDeltaExt/2);
            oNewGeorefExt.maxy = dfMiddleY + (dfDeltaExt/2);
        }
        else
          RETURN_FALSE;
    }

/* -------------------------------------------------------------------- */
/*      If the buffer is set, make sure that the extents do not go      */
/*      beyond the buffer.                                              */
/* -------------------------------------------------------------------- */
    if (bMaxExtSet)
    {
        dfDeltaX = oNewGeorefExt.maxx - oNewGeorefExt.minx;
        dfDeltaY = oNewGeorefExt.maxy - oNewGeorefExt.miny;
        
        /* Make sure Current georef extents is not bigger than max extents */
        if (dfDeltaX > (poMaxGeorefExt->maxx-poMaxGeorefExt->minx))
            dfDeltaX = poMaxGeorefExt->maxx-poMaxGeorefExt->minx;
        if (dfDeltaY > (poMaxGeorefExt->maxy-poMaxGeorefExt->miny))
            dfDeltaY = poMaxGeorefExt->maxy-poMaxGeorefExt->miny;

        if (oNewGeorefExt.minx < poMaxGeorefExt->minx)
        {
            oNewGeorefExt.minx = poMaxGeorefExt->minx;
            oNewGeorefExt.maxx =  oNewGeorefExt.minx + dfDeltaX;
        }
        if (oNewGeorefExt.maxx > poMaxGeorefExt->maxx)
        {
            oNewGeorefExt.maxx = poMaxGeorefExt->maxx;
            oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
        }
        if (oNewGeorefExt.miny < poMaxGeorefExt->miny)
        {
            oNewGeorefExt.miny = poMaxGeorefExt->miny;
            oNewGeorefExt.maxy =  oNewGeorefExt.miny + dfDeltaY;
        }
        if (oNewGeorefExt.maxy > poMaxGeorefExt->maxy)
        {
            oNewGeorefExt.maxy = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }

    self->extent.minx = oNewGeorefExt.minx;
    self->extent.miny = oNewGeorefExt.miny;
    self->extent.maxx = oNewGeorefExt.maxx;
    self->extent.maxy = oNewGeorefExt.maxy;
    
    self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                    self->height);    
    dfDeltaX = self->extent.maxx - self->extent.minx;
    dfDeltaY = self->extent.maxy - self->extent.miny; 

    if (bMaxExtSet)
    {
        if (self->extent.minx < poMaxGeorefExt->minx)
        {
            self->extent.minx = poMaxGeorefExt->minx;
            self->extent.maxx = self->extent.minx + dfDeltaX;
        }
        if (self->extent.maxx > poMaxGeorefExt->maxx)
        {
            self->extent.maxx = poMaxGeorefExt->maxx;
            oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
        }
        if (self->extent.miny < poMaxGeorefExt->miny)
        {
            self->extent.miny = poMaxGeorefExt->miny;
            self->extent.maxy =  self->extent.miny + dfDeltaY;
        }
        if (self->extent.maxy > poMaxGeorefExt->maxy)
        {
            self->extent.miny = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }

    if (msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scale)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR); 
    _phpms_set_property_double(pThis,"scale", self->scale, E_ERROR); 

#ifdef PHP4
    if (zend_hash_find(pThis->value.obj.properties, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#else
    if (_php3_hash_find(pThis->value.ht, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double(pExtent,"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#endif

    RETURN_TRUE;
}

/************************************************************************/
/*                             map->zoomScale()                         */
/*                                                                      */
/*      Parmeters are :                                                 */
/*                                                                      */
/*       - Scale : Scale at which to see the map. Must be a positive    */
/*                 value. Ex : for a scale to 1/250000, the value of    */
/*                 this parameter is 250000.                            */
/*       - Pixel position (pointObj) : x, y coordinates of the click.   */
/*       - Width : width in pixel of the current image.                 */
/*       - Height : Height in pixel of the current image.               */
/*       - Georef extent (rectObj) : current georef extents.            */
/*       - MaxGeoref extent (rectObj) : (optional) maximum georef       */
/*                                      extents.                        */
/************************************************************************/
DLEXPORT void php3_ms_map_zoomScale(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj      *self;
    pval        *pThis;
#ifdef PHP4
    pval   **pExtent;
#else
    pval   *pExtent;
#endif
    pval        *pScale;
    pval        *pPixelPos;
    pval        *pWidth, *pHeight;
    pval        *pGeorefExt;
    pval        *pMaxGeorefExt;
    rectObj     *poGeorefExt = NULL;
    rectObj     *poMaxGeorefExt = NULL;
    pointObj    *poPixPos = NULL;
    double      dfGeoPosX, dfGeoPosY;
    double      dfDeltaX, dfDeltaY;
    rectObj     oNewGeorefExt;    
    double      dfNewScale = 0.0;
    int         bMaxExtSet = 0;
    int         nArgs = ARG_COUNT(ht);
    double      dfDeltaExt = -1.0;
    double      dfCurrentScale = 0.0;
    int         nTmp = 0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||   
        (nArgs != 5 && nArgs != 6))
    {
        WRONG_PARAM_COUNT;
    }
    
    if (getParameters(ht, nArgs, 
                      &pScale, &pPixelPos, &pWidth, &pHeight,
                      &pGeorefExt, &pMaxGeorefExt) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    if (nArgs == 6)
        bMaxExtSet =1;

    convert_to_long(pScale);
    convert_to_long(pWidth);
    convert_to_long(pHeight);
         
    poGeorefExt = (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                                  PHPMS_GLOBAL(le_msrect_ref),
                                                  PHPMS_GLOBAL(le_msrect_new),
                                                  list);
    poPixPos = (pointObj *)_phpms_fetch_handle2(pPixelPos, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list);
    
    if (bMaxExtSet)
        poMaxGeorefExt = 
            (rectObj *)_phpms_fetch_handle2(pMaxGeorefExt, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list);

/* -------------------------------------------------------------------- */
/*      check the validity of the parameters.                           */
/* -------------------------------------------------------------------- */
    if (pScale->value.lval <= 0.0 || 
        pWidth->value.lval <= 0 ||
        pHeight->value.lval <= 0 ||
        poGeorefExt == NULL ||
        poPixPos == NULL ||
        (bMaxExtSet && poMaxGeorefExt == NULL))
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomScale failed : incorrect parameters\n");
        RETURN_FALSE;
    }

/* -------------------------------------------------------------------- */
/*      check if the values passed are consistent min > max.             */
/* -------------------------------------------------------------------- */
    if (poGeorefExt->minx >= poGeorefExt->maxx)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomScale failed : Georeferenced coordinates minx >= maxx\n");
    }
    if (poGeorefExt->miny >= poGeorefExt->maxy)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomScale failed : Georeferenced coordinates miny >= maxy\n");
    }
    if (bMaxExtSet)
    {
        if (poMaxGeorefExt->minx >= poMaxGeorefExt->maxx)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "zoomScale failed : Max Georeferenced coordinates minx >= maxx\n");
        }
        if (poMaxGeorefExt->miny >= poMaxGeorefExt->maxy)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "zoomScale failed : Max Georeferenced coordinates miny >= maxy\n");
        }
    }
    

    dfGeoPosX = Pix2Georef((int)poPixPos->x, 0, (int)pWidth->value.lval, 
                           poGeorefExt->minx, poGeorefExt->maxx, 0); 
    dfGeoPosY = Pix2Georef((int)poPixPos->y, 0, (int)pHeight->value.lval, 
                           poGeorefExt->miny, poGeorefExt->maxy, 1); 
    dfDeltaX = poGeorefExt->maxx - poGeorefExt->minx;
    dfDeltaY = poGeorefExt->maxy - poGeorefExt->miny;

   
/* -------------------------------------------------------------------- */
/*      Calculate new extents based on the sacle.                       */
/* -------------------------------------------------------------------- */

/* ==================================================================== */
/*      make sure to take the smallest size because this is the one     */
/*      that will be used to ajust the scale.                           */
/* ==================================================================== */

    if (self->width <  self->height)
      nTmp = self->width;
    else
      nTmp = self->height;

    dfDeltaExt = 
        GetDeltaExtentsUsingScale(pScale->value.lval, self->units, nTmp,
                                  self->resolution);
                                  
    if (dfDeltaExt > 0.0)
    {
        oNewGeorefExt.minx = dfGeoPosX - (dfDeltaExt/2);
        oNewGeorefExt.miny = dfGeoPosY - (dfDeltaExt/2);
        oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaExt/2);
        oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaExt/2);
    }
    else
      RETURN_FALSE;

/* -------------------------------------------------------------------- */
/*      get current scale.                                              */
/* -------------------------------------------------------------------- */
    if (msCalculateScale(*poGeorefExt, self->units, 
                         self->width, self->height,
                         self->resolution, &dfCurrentScale) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

/* -------------------------------------------------------------------- */
/*      if the min and max scale are set in the map file, we will       */
/*      use them to test before zooming.                                */
/*                                                                      */
/*       This function has the same effect as zoomin or zoom out. If    */
/*      the current scale is > newscale we zoom in; else it is a        */
/*      zoom out.                                                       */
/* -------------------------------------------------------------------- */
    msAdjustExtent(&oNewGeorefExt, self->width, self->height);
    if (msCalculateScale(oNewGeorefExt, self->units, 
                         self->width, self->height,
                         self->resolution, &dfNewScale) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    if (self->web.maxscale > 0)
    {
        if (dfCurrentScale < dfNewScale && dfNewScale >  self->web.maxscale)
        {
            RETURN_FALSE;
        }
    }

/* ==================================================================== */
/*      we do a special case for zoom in : we try to zoom as much as    */
/*      possible using the mincale set in the .map.                     */
/* ==================================================================== */
    if (self->web.minscale > 0 && dfNewScale <  self->web.minscale &&
        dfCurrentScale > dfNewScale)
    {
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscale, self->units, 
                                      self->width, self->resolution);
        if (dfDeltaExt > 0.0)
        {
            oNewGeorefExt.minx = dfGeoPosX - (dfDeltaExt/2);
            oNewGeorefExt.miny = dfGeoPosY - (dfDeltaExt/2);
            oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaExt/2);
            oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaExt/2);
        }
        else
          RETURN_FALSE;
    }
/* -------------------------------------------------------------------- */
/*      If the buffer is set, make sure that the extents do not go      */
/*      beyond the buffer.                                              */
/* -------------------------------------------------------------------- */
    if (bMaxExtSet)
    {
        dfDeltaX = oNewGeorefExt.maxx - oNewGeorefExt.minx;
        dfDeltaY = oNewGeorefExt.maxy - oNewGeorefExt.miny;
        
        /* Make sure Current georef extents is not bigger than max extents */
        if (dfDeltaX > (poMaxGeorefExt->maxx-poMaxGeorefExt->minx))
            dfDeltaX = poMaxGeorefExt->maxx-poMaxGeorefExt->minx;
        if (dfDeltaY > (poMaxGeorefExt->maxy-poMaxGeorefExt->miny))
            dfDeltaY = poMaxGeorefExt->maxy-poMaxGeorefExt->miny;

        if (oNewGeorefExt.minx < poMaxGeorefExt->minx)
        {
            oNewGeorefExt.minx = poMaxGeorefExt->minx;
            oNewGeorefExt.maxx =  oNewGeorefExt.minx + dfDeltaX;
        }
        if (oNewGeorefExt.maxx > poMaxGeorefExt->maxx)
        {
            oNewGeorefExt.maxx = poMaxGeorefExt->maxx;
            oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
        }
        if (oNewGeorefExt.miny < poMaxGeorefExt->miny)
        {
            oNewGeorefExt.miny = poMaxGeorefExt->miny;
            oNewGeorefExt.maxy =  oNewGeorefExt.miny + dfDeltaY;
        }
        if (oNewGeorefExt.maxy > poMaxGeorefExt->maxy)
        {
            oNewGeorefExt.maxy = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }
    
/* -------------------------------------------------------------------- */
/*      set the map extents with new values.                            */
/* -------------------------------------------------------------------- */
    self->extent.minx = oNewGeorefExt.minx;
    self->extent.miny = oNewGeorefExt.miny;
    self->extent.maxx = oNewGeorefExt.maxx;
    self->extent.maxy = oNewGeorefExt.maxy;
    

    self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                    self->height);      
    dfDeltaX = self->extent.maxx - self->extent.minx;
    dfDeltaY = self->extent.maxy - self->extent.miny; 
    
    if (bMaxExtSet)
    {
        if (self->extent.minx < poMaxGeorefExt->minx)
        {
            self->extent.minx = poMaxGeorefExt->minx;
            self->extent.maxx = self->extent.minx + dfDeltaX;
        }
        if (self->extent.maxx > poMaxGeorefExt->maxx)
        {
            self->extent.maxx = poMaxGeorefExt->maxx;
            oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
        }
        if (self->extent.miny < poMaxGeorefExt->miny)
        {
            self->extent.miny = poMaxGeorefExt->miny;
            self->extent.maxy =  self->extent.miny + dfDeltaY;
        }
        if (self->extent.maxy > poMaxGeorefExt->maxy)
        {
            self->extent.miny = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }
    
    if (msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scale)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR); 
    _phpms_set_property_double(pThis,"scale", self->scale, E_ERROR); 

#ifdef PHP4
    if (zend_hash_find(pThis->value.obj.properties, "extent", sizeof("extent"), 
                       (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#else
    if (_php3_hash_find(pThis->value.ht, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double(pExtent,"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
#endif

     RETURN_TRUE;

}
    
/**********************************************************************
 *                        map->addColor()
 **********************************************************************/

/* {{{ proto int map.addColor(int r, int g, int b)
   Add a color to map's palette.  Returns color index.*/

DLEXPORT void php3_ms_map_addColor(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pR, *pG, *pB, *pThis;
    int     nColorId = 0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 3, &pR, &pG, &pB) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pR);
    convert_to_long(pG);
    convert_to_long(pB);
    
    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || 
        (nColorId = mapObj_addColor(self, pR->value.lval, 
                                    pG->value.lval, pB->value.lval)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(nColorId);
}
/* }}} */


/************************************************************************/
/*                              DLEXPORT void                           */
/*        php3_ms_map_getSymbolByName(INTERNAL_FUNCTION_PARAMETERS)     */
/*                                                                      */
/*       Get the symbol id using it's name. Parameters are :            */
/*                                                                      */
/*        - symbol name                                                 */
/************************************************************************/
DLEXPORT void php3_ms_map_getSymbolByName(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    pval        *pSymName;
    mapObj      *self=NULL;
    int         nSymbolId = -1;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pSymName) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pSymName);


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        nSymbolId = 
            mapObj_getSymbolByName(self, pSymName->value.str.val);
    }

    RETURN_LONG(nSymbolId);
}
/* }}} */


/**********************************************************************
 *                        map->prepareImage()
 **********************************************************************/

/* {{{ proto int map.prepareImage()
   Return handle on blank image object. */

DLEXPORT void php3_ms_map_prepareImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_prepareImage(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->prepareQuery()
 **********************************************************************/

/* {{{ proto int map.prepareQuery()
   Calculate the scale of the map and assign it to the map->scale */

DLEXPORT void php3_ms_map_prepareQuery(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self != NULL)
      mapObj_prepareQuery(self);
    
}
/**********************************************************************
 *                        map->draw()
 **********************************************************************/

/* {{{ proto int map.draw()
   Render map and return handle on image object. */

DLEXPORT void php3_ms_map_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_draw(self)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }
    else
    {
        /* Return an image object */
        _phpms_build_img_object(im, &(self->web), list, return_value);
    }
}
/* }}} */

/**********************************************************************
 *                        map->drawQuery()
 **********************************************************************/

/* {{{ proto int map.drawQuery()
   Renders a query map, returns an image.. */
DLEXPORT void php3_ms_map_drawQuery(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    mapObj      *self;
    gdImagePtr im = NULL;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im =  mapObj_drawQuery(self)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }
    else
    {
        /* Return an image object */
        _phpms_build_img_object(im, &(self->web), list, return_value);
    }
}
     
/* }}} */

/**********************************************************************
 *                        map->drawLegend()
 **********************************************************************/

/* {{{ proto int map.drawLegend()
   Render legend and return handle on image object. */

DLEXPORT void php3_ms_map_drawLegend(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_drawLegend(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->drawReferenceMap()
 **********************************************************************/

/* {{{ proto int map.drawReferenceMap()
   Render reference map and return handle on image object. */

DLEXPORT void php3_ms_map_drawReferenceMap(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_drawReferenceMap(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->drawScaleBar()
 **********************************************************************/

/* {{{ proto int map.drawScaleBar()
   Render scale bar and return handle on image object. */

DLEXPORT void php3_ms_map_drawScaleBar(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_drawScalebar(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->embedScalebar()
 **********************************************************************/

/* {{{ proto int map.embedScalebar(image img)
   Renders the scalebar within the map. Returns -1 on error. */

DLEXPORT void php3_ms_map_embedScalebar(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *imgObj, *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
    int    retVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &imgObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
        
    im = (gdImagePtr)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (retVal = mapObj_embedScalebar(self, im)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(retVal);
}
/* }}} */

/**********************************************************************
 *                        map->embedLegend()
 **********************************************************************/

/* {{{ proto int map.embedLegend(image img)
   Renders the legend within the map. Returns -1 on error. */

DLEXPORT void php3_ms_map_embedLegend(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *imgObj, *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
    int    retVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &imgObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
        
    im = (gdImagePtr)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (retVal = mapObj_embedLegend(self, im)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        map->drawLabelCache()
 **********************************************************************/

/* {{{ proto int map.drawLabelCache(image img)
   Renders the labels for a map. Returns -1 on error. */

DLEXPORT void php3_ms_map_drawLabelCache(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *imgObj, *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
    int    retVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &imgObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
        
    im = (gdImagePtr)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (retVal = mapObj_drawLabelCache(self, im)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        map->getLayer()
 *
 * Note: Multiple calls to getlayer() will return multiple instances
 * of PHP objects pointing to the same layerObj... this is safe but is a
 * waste of resources.
 **********************************************************************/

/* {{{ proto int map.getLayer(int i)
   Returns a layerObj from the map given an index value (0=first layer, etc.) */

DLEXPORT void php3_ms_map_getLayer(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pLyrIndex, *pThis;
    mapObj *self=NULL;
    layerObj *newLayer=NULL;
    int map_id;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pLyrIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    /* pLyrIndex is the 0-based index of the requested layer */
    convert_to_long(pLyrIndex);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self == NULL || 
        (newLayer = mapObj_getLayer(self, pLyrIndex->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* We will store a reference to the parent object id (this) inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_layer_object(newLayer, map_id, list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->getLayerByName()
 *
 * Note: Multiple calls to getlayer() will return multiple instances
 * of PHP objects pointing to the same layerObj... this is safe but is a
 * waste of resources.
 **********************************************************************/

/* {{{ proto int map.getLayerByName(string layer_name)
   Returns a layerObj from the map given a layer name */

DLEXPORT void php3_ms_map_getLayerByName(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pLyrName, *pThis;
    mapObj *self=NULL;
    layerObj *newLayer=NULL;
    int map_id;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pLyrName) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pLyrName);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        newLayer = mapObj_getLayerByName(self, pLyrName->value.str.val);
        if (newLayer == NULL)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "getLayerByName failed for : %s\n",
                       pLyrName->value.str.val);
        }
    }
    else
    {
        RETURN_FALSE;
    }

    /* We will store a reference to the parent object id (this) inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_layer_object(newLayer, map_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        map->getLayersIndexByGroup()
 *
 * Return an array of layer's index given a group name.
 **********************************************************************/

/* {{{ proto int map.getLayersIndexByGroup(string groupname)
   Return an array of layer's index given a group name. */

DLEXPORT void php3_ms_map_getLayersIndexByGroup(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pGrpName, *pThis;
    mapObj      *self=NULL;
    int         *aiIndex = NULL;
    int         nCount = 0;
    int         i = 0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pGrpName) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pGrpName);

    if (array_init(return_value) == FAILURE) 
    {
        RETURN_FALSE;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        aiIndex = 
            mapObj_getLayersIndexByGroup(self, pGrpName->value.str.val,
                                         &nCount);

        if (aiIndex && nCount > 0)
        {
            for (i=0; i<nCount; i++)
            {
                add_next_index_long(return_value, aiIndex[i]);
            }
            free (aiIndex);
        }
        else
            RETURN_FALSE; 
    }
    else
    {
        RETURN_FALSE;
    }
}
/* }}} */

/**********************************************************************
 *                        map->getAllLayerNames()
 *
 * Return an array conating all the layers name.
 **********************************************************************/

/* {{{ proto int map.getAllLayerNames()
   Return an array conating all the layers name.*/

DLEXPORT void php3_ms_map_getAllLayerNames(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    mapObj      *self=NULL;
    int         nCount = 0;
    int         i = 0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL)
    {
        WRONG_PARAM_COUNT;
    }

    if (array_init(return_value) == FAILURE) 
    {
        RETURN_FALSE;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        nCount = self->numlayers;
        for (i=0; i<nCount; i++)
        {
            add_next_index_string(return_value,  self->layers[i].name, 1);
        }
    }
    else
    {
        RETURN_FALSE;
    }
}
/* }}} */

/**********************************************************************
 *                        map->getAllGroupNames()
 *
 * Return an array containing all the group names
 **********************************************************************/

/* {{{ proto int map.getAllGroupNames()
   Return an array containing all the group names. */

DLEXPORT void php3_ms_map_getAllGroupNames(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    mapObj      *self=NULL;
    int         nCount = 0;
    int         i, j = 0;
    char        **papszGroups = NULL;
    int         nGroups = 0;
    int         nLength = 0;
    int         bFound = 0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL)
    {
        WRONG_PARAM_COUNT;
    }

    if (array_init(return_value) == FAILURE) 
    {
        RETURN_FALSE;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL && self->numlayers > 0)
    {
        nCount = self->numlayers;
        nGroups = 0;
        papszGroups = (char **)malloc(sizeof(char *)*nCount);
        for (i=0; i<nCount; i++)
        {
            bFound = 0;
            if (self->layers[i].group)
            {
                for (j=0; j<nGroups; j++)
                {
                    if (strcmp(self->layers[i].group, papszGroups[j]) == 0)
                    {
                        bFound = 1;
                        break;
                    }
                }
                if (!bFound)
                {
                    nLength = strlen(self->layers[i].group);
                    papszGroups[nGroups] = (char *)emalloc(nLength+1);
                    sprintf(papszGroups[nGroups], "%s", 
                            self->layers[i].group);
                    nGroups++;
                    add_next_index_string(return_value,  
                                          self->layers[i].group, 1);
                }
            }
        }
        for (j=0; j<nGroups; j++)
        {
            free(papszGroups[j]);
        }
        free(papszGroups[j]);
    }
    else
    {
        RETURN_FALSE;
    }
}
/* }}} */

/************************************************************************/
/*                         map->getColorByIndex                         */
/*                                                                      */
/*      Returns a color object (r,g,b values) using the color index     */
/*      given in argument.                                              */
/*      The color index corresponds to the index in the internal        */
/*      palette array used by the map to store the colors.              */
/************************************************************************/
/* {{{ proto int map.getColorByIndex(int nColorIndex)
   Returns a colorObj from the map given a color index */

DLEXPORT void php3_ms_map_getColorByIndex(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pColorIndex, *pThis;
    mapObj      *self=NULL;
    paletteObj  palette;
    colorObj    oColor;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pColorIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pColorIndex);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        palette = self->palette;
        
        if (pColorIndex->value.lval < palette.numcolors)
        {
            oColor.red = palette.colors[(int)pColorIndex->value.lval].red;
            oColor.green = palette.colors[(int)pColorIndex->value.lval].green;
            oColor.blue = palette.colors[(int)pColorIndex->value.lval].blue;

        }
        else
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "getColorByIndex failed"\
                       "Index shoud not be higher than %d\n",
                       palette.numcolors-1);
        }
    }
    else
    {
        RETURN_FALSE;
    }

    /* Return color object */
    _phpms_build_color_object(&oColor, list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->queryByPoint()
 *
 * Type is MS_SINGLE or MS_MULTIPLE depending on number of results
 * you want. Passing buffer <=0 defaults to tolerances set in the map 
 * file (in pixels) but you can use a constant buffer (specified in 
 * ground units) instead.
 **********************************************************************/

/* {{{ proto int map.queryByPoint(pointObj point, int type, double buffer)
   Query at point location. */

DLEXPORT void php3_ms_map_queryByPoint(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pPoint, *pType, *pBuffer;
    mapObj *self=NULL;
    pointObj *poPoint=NULL;
    int      nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 3, &pPoint, &pType, &pBuffer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);
    convert_to_double(pBuffer);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list);

    if (self && poPoint && 
        (nStatus=mapObj_queryByPoint(self, poPoint, pType->value.lval, 
                                      pBuffer->value.dval)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}
/* }}} */


/************************************************************************/
/*                          map->queryByRect()                          */
/*                                                                      */
/*      Parmeters :                                                     */
/*                                                                      */
/*       rectObj : poRect : extents used to make the query.             */
/************************************************************************/


/* {{{ proto int map.queryByRect(rectObj poRect) */
 

DLEXPORT void php3_ms_map_queryByRect(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pRect;
    mapObj *self=NULL;
    rectObj *poRect=NULL;
    int      nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pRect) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    poRect = (rectObj *)_phpms_fetch_handle2(pRect,
                                             PHPMS_GLOBAL(le_msrect_ref),
                                             PHPMS_GLOBAL(le_msrect_new),
                                             list);

    if (self && poRect && 
        (nStatus=mapObj_queryByRect(self, *poRect)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}
/* }}} */


/**********************************************************************
 *                    map->queryByFeatures(int slayer)
 **********************************************************************/
DLEXPORT void php3_ms_map_queryByFeatures(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pSLayer;
    mapObj *self=NULL;
    int      nStatus = MS_FAILURE;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pSLayer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pSLayer);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);

    if (self &&
        (nStatus=mapObj_queryByFeatures(self, 
                                        pSLayer->value.lval)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}

/**********************************************************************
 *                    map->queryByShape(shape)
 *
 * Generates a result set based on a single shape, the shape is
 * assumed to be a polygon at this point.
 **********************************************************************/

/* {{{ proto int map.queryByShape(shapeObj poShape) */
DLEXPORT void php3_ms_map_queryByShape(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pShape;
    mapObj      *self=NULL;
    shapeObj    *poShape=NULL;
    int         nStatus = MS_FAILURE;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);

    poShape = 
        (shapeObj *)_phpms_fetch_handle2(pShape, 
                                         PHPMS_GLOBAL(le_msshape_ref),
                                         PHPMS_GLOBAL(le_msshape_new),
                                         list);
    
    if (self && poShape && 
        (nStatus = mapObj_queryByShape(self, poShape))!= MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}

   
/**********************************************************************
 *                        map->save()
 **********************************************************************/

/* {{{ proto int map.save(string filename)
   Write map object to a file. */

DLEXPORT void php3_ms_map_save(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis, *pFname;
    mapObj *self;
    int    retVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pFname) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFname);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || 
        (retVal = mapObj_save(self, pFname->value.str.val)) != 0)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(retVal);
}
/* }}} */


/************************************************************************/
/*                    php3_ms_map_getLatLongExtent()                    */
/*                                                                      */
/*      Utility function (not documented) to get the lat/long           */
/*      extents of the current map. We assume here that the map has     */
/*      it's projection (ex LCC or UTM defined).                        */
/*                                                                      */
/*      Available only with PROJ support.                               */
/************************************************************************/
DLEXPORT void php3_ms_map_getLatLongExtent(INTERNAL_FUNCTION_PARAMETERS)
{ 
#ifdef USE_PROJ
    pval        *pThis;
    mapObj      *self=NULL;
    rectObj     oGeorefExt;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        oGeorefExt.minx = self->extent.minx;
        oGeorefExt.miny = self->extent.miny;
        oGeorefExt.maxx = self->extent.maxx;
        oGeorefExt.maxy = self->extent.maxy;
        
        if (self->projection.proj != NULL)
        {
            msProjectRect(&(self->projection), NULL, &oGeorefExt);
        }
        
    }
    /* Return rectObj */
    _phpms_build_rect_object(&oGeorefExt, PHPMS_GLOBAL(le_msrect_new), 
                             list, return_value);
#else
    php3_error(E_ERROR, 
               "getLatLongExtent() available only with PROJ.4 support.");
#endif
}

/**********************************************************************
 *                        map->getMetaData()
 **********************************************************************/

/* {{{ proto string map.getMetaData(string name)
   Return MetaData entry by name, or empty string if not found. */

DLEXPORT void php3_ms_map_getMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pThis, *pName;
    char   *pszValue = NULL;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pName) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pName);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap),
                                           list);
    if (self == NULL || 
        (pszValue = mapObj_getMetaData(self, pName->value.str.val)) == NULL)
    {
        pszValue = "";
    }

    RETURN_STRING(pszValue, 1);
}
/* }}} */

/**********************************************************************
 *                        map->setMetaData()
 **********************************************************************/

/* {{{ proto int map.setMetaData(string name, string value)
   Set MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_map_setMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pThis, *pName, *pValue;
    int    nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pName, &pValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pName);
    convert_to_string(pValue);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap),
                                           list);
    if (self == NULL || 
        (nStatus = mapObj_setMetaData(self, 
                                      pName->value.str.val,  
                                      pValue->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}
/* }}} */




/*=====================================================================
 *                 PHP function wrappers - image class
 *====================================================================*/

#define MS_IMAGE_FORMAT_EXT(type)  ((type)==MS_GIF?"gif": \
                                    (type)==MS_PNG?"png": \
                                    (type)==MS_JPEG?"jpg": \
                                    (type)==MS_WBMP?"wbmp":"???unsupported???")

/**********************************************************************
 *                     _phpms_build_img_object()
 **********************************************************************/
static long _phpms_build_img_object(gdImagePtr im, webObj *pweb,
                                    HashTable *list, pval *return_value)
{
    int img_id;

    if (im == NULL)
        return 0;

    img_id = php3_list_insert(im, PHPMS_GLOBAL(le_msimg));

    _phpms_object_init(return_value, img_id, php_img_class_functions,
                       PHP4_CLASS_ENTRY(img_class_entry_ptr));

    /* width/height params are read-only */
    add_property_long(return_value, "width", gdImageSX(im));
    add_property_long(return_value, "height", gdImageSY(im));
    
    if (pweb)
    {
        PHPMS_ADD_PROP_STR(return_value, "imagepath", pweb->imagepath);
        PHPMS_ADD_PROP_STR(return_value, "imageurl", pweb->imageurl);
    }

/* php3_printf("Create image: id=%d, ptr=0x%x<P>\n", img_id, im);*/

    return img_id;
}

/**********************************************************************
 *                        image->saveImage()
 *
 *       saveImage(string filename, int type, int transparent, 
 *                 int interlace, int quality)
 **********************************************************************/

/* {{{ proto int img.saveImage(string filename, int type, int transparent, int interlace, int quality)
   Writes image object to specifed filename.  If filename is empty then write to stdout.  Returns -1 on error. */

DLEXPORT void php3_ms_img_saveImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval   *pFname, *pTransparent, *pInterlace, *pThis;
    pval   *pType, *pQuality;
    gdImagePtr im = NULL;
    int retVal = 0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 5, &pFname, &pType, &pTransparent, 
                      &pInterlace, &pQuality) != SUCCESS  )
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFname);
    convert_to_long(pTransparent);
    convert_to_long(pInterlace);
    convert_to_long(pType);
    convert_to_long(pQuality);

    im = (gdImagePtr)_phpms_fetch_handle(pThis, le_msimg, list);

    if(pFname->value.str.val != NULL && strlen(pFname->value.str.val) > 0)
    {
        if (im == NULL ||
            (retVal = msSaveImage(im, pFname->value.str.val, 
                                  pType->value.lval, 
                                  pTransparent->value.lval, 
                                  pInterlace->value.lval, 
                                  pQuality->value.lval) ) != 0  )
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "Failed writing image to %s", 
                       pFname->value.str.val);
        }
    }
    else
    {           /* no filename - stdout */
        int size=0;
#if !defined(USE_GD_GIF) || defined(GD_HAS_GDIMAGEGIFPTR)
        void *iptr=NULL;
#else
        int   b;
        FILE *tmp;
        char  buf[4096];
#endif

        retVal = 0;

#ifdef PHP4
        php_header();
#else
        php3_header();
#endif

        if(pInterlace->value.lval)
            gdImageInterlace(im, 1);

        if(pTransparent->value.lval)
            gdImageColorTransparent(im, 0);

#if !defined(USE_GD_GIF) || defined(GD_HAS_GDIMAGEGIFPTR)

#ifdef USE_GD_GIF
        if(pType->value.lval == MS_GIF)
            iptr = gdImageGifPtr(im, &size);
        else
#endif
#ifdef USE_GD_PNG
        if(pType->value.lval == MS_PNG)
            iptr = gdImagePngPtr(im, &size);
        else
#endif
#ifdef USE_GD_JPEG
        if(pType->value.lval == MS_JPEG)
            iptr = gdImageJpegPtr(im, &size, pQuality->value.lval);
        else
#endif
#ifdef USE_GD_WBMP
        if(pType->value.lval == MS_WBMP)
            iptr = gdImageWBMPPtr(im, &size, 1);
        else
#endif
        {
            php3_error(E_ERROR, "Output to '%s' not available", 
                       MS_IMAGE_FORMAT_EXT(pType->value.lval) );
        }

        if (size == 0) {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "Failed writing image to stdout");
            retVal = -1;
        } 
        else
        {
#ifdef PHP4
                php_write(iptr, size);
#else
                php3_write(iptr, size);
#endif
            retVal = size;
            free(iptr);
        }

#else  /* No gdImageGifPtr(): GD 1.2/1.3 */

        /* there is no gdImageGifPtr function */

        tmp = tmpfile(); /* temporary file */
        if (tmp == NULL) 
        {
            php3_error(E_WARNING, "Unable to open temporary file");
            retVal = -1;
        } 
        else
        {
            gdImageGif (im, tmp);
            size = ftell(tmp);
            fseek(tmp, 0, SEEK_SET);

#if APACHE && defined(CHARSET_EBCDIC)
            /* This is a binary file already: avoid EBCDIC->ASCII conversion */
            ap_bsetflag(php3_rqst->connection->client, B_EBCDIC2ASCII, 0);
#endif

            while ((b = fread(buf, 1, sizeof(buf), tmp)) > 0) 
            {
#ifdef PHP4
                php_write(buf, b);
#else
                php3_write(buf, b);
#endif
            }

            fclose(tmp); /* the temporary file is automatically deleted */
            retVal = size;
        }
#endif

    }

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        image->saveWebImage()
 *       saveWebImage(int type, int transparent, int interlace, int quality)
 **********************************************************************/

/* {{{ proto int img.saveWebImage(int type, int transparent, int interlace, int quality)
   Writes image to temp directory.  Returns image URL. */

DLEXPORT void php3_ms_img_saveWebImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval   *pTransparent, *pInterlace, *pThis;
    pval   *pType, *pQuality;

    gdImagePtr im = NULL;
    char *pImagepath, *pImageurl, *pBuf;
    int nBufSize, nLen1, nLen2;
    const char *pszImageExt;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 4, &pType, &pTransparent, 
                      &pInterlace, &pQuality) != SUCCESS  )
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pTransparent);
    convert_to_long(pInterlace);
    convert_to_long(pType);
    convert_to_long(pQuality);

    pszImageExt = MS_IMAGE_FORMAT_EXT(pType->value.lval);

    im = (gdImagePtr)_phpms_fetch_handle(pThis, le_msimg, list);
    pImagepath = _phpms_fetch_property_string(pThis, "imagepath", E_ERROR);
    pImageurl = _phpms_fetch_property_string(pThis, "imageurl", E_ERROR);

    /* Build a unique filename in the IMAGEPATH directory 
     */
    nLen1 = strlen(pImagepath);
    nLen2 = strlen(pImageurl);
    nBufSize = (nLen1>nLen2 ? nLen1:nLen2) + strlen(tmpId) + 30;
    pBuf = (char*)emalloc(nBufSize);
    tmpCount++;
    sprintf(pBuf, "%s%s%d.%s", pImagepath, tmpId, tmpCount, pszImageExt);

    /* Save the image... 
     */
    if (im == NULL || 
        msSaveImage(im, pBuf, pType->value.lval, pTransparent->value.lval, 
                    pInterlace->value.lval, pQuality->value.lval) != 0 )
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed writing image to %s", 
                   pBuf);
    }

    /* ... and return the corresponding URL
     */
    sprintf(pBuf, "%s%s%d.%s", pImageurl, tmpId, tmpCount, pszImageExt);
    RETURN_STRING(pBuf, 0);
}
/* }}} */


/**********************************************************************
 *                        image->free()
 **********************************************************************/

/* {{{ proto int img.free()
   Destroys resources used by an image object */

DLEXPORT void php3_ms_img_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    gdImagePtr self;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (gdImagePtr)_phpms_fetch_handle(pThis, le_msimg, list);
    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
#ifdef PHP4
        pval **phandle;
        if (zend_hash_find(pThis->value.obj.properties, "_handle_", 
                           sizeof("_handle_"), 
                           (void **)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
#else
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)

        {
            php3_list_delete(phandle->value.lval);
        }
#endif
    }

}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - layerObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_layer_object()
 **********************************************************************/
static long _phpms_build_layer_object(layerObj *player, int parent_map_id,
                                      HashTable *list, pval *return_value)
{
    int layer_id;

    if (player == NULL)
    {
        return 0;
    }

    layer_id = php3_list_insert(player, PHPMS_GLOBAL(le_mslayer));

    _phpms_object_init(return_value, layer_id, php_layer_class_functions,
                       PHP4_CLASS_ENTRY(layer_class_entry_ptr));

#ifdef PHP4
    zend_list_addref(parent_map_id);
    add_property_resource(return_value, "_map_handle_", parent_map_id);
#else
    add_property_long(return_value, "_map_handle_", parent_map_id);
#endif

    /* read-only properties */
    add_property_long(return_value,   "numclasses", player->numclasses);
    add_property_long(return_value,   "index",      player->index);

    /* editable properties */
    add_property_long(return_value,   "status",     player->status);
    PHPMS_ADD_PROP_STR(return_value,  "classitem",  player->classitem);
    PHPMS_ADD_PROP_STR(return_value,  "name",       player->name);
    PHPMS_ADD_PROP_STR(return_value,  "group",      player->group);
    PHPMS_ADD_PROP_STR(return_value,  "data",       player->data);
    add_property_long(return_value,   "type",       player->type);
    add_property_double(return_value, "tolerance",  player->tolerance);
    add_property_long(return_value,   "toleranceunits",player->toleranceunits);
    add_property_double(return_value, "symbolscale",player->symbolscale);
    add_property_double(return_value, "minscale",   player->minscale);
    add_property_double(return_value, "maxscale",   player->maxscale);
    add_property_double(return_value, "labelminscale",player->labelminscale);
    add_property_double(return_value, "labelmaxscale",player->labelmaxscale);
    add_property_long(return_value,   "maxfeatures",player->maxfeatures);
    add_property_long(return_value,   "offsite",    player->offsite);
    add_property_long(return_value,   "transform",  player->transform);
    add_property_long(return_value,   "labelcache", player->labelcache);
    add_property_long(return_value,   "postlabelcache",player->postlabelcache);
    PHPMS_ADD_PROP_STR(return_value,  "labelitem",  player->labelitem);
    PHPMS_ADD_PROP_STR(return_value,  "labelsizeitem",player->labelsizeitem);
    PHPMS_ADD_PROP_STR(return_value,  "labelangleitem",player->labelangleitem);
    PHPMS_ADD_PROP_STR(return_value,  "tileitem",   player->tileitem);
    PHPMS_ADD_PROP_STR(return_value,  "tileindex",  player->tileindex);
    PHPMS_ADD_PROP_STR(return_value,  "header",     player->header);
    PHPMS_ADD_PROP_STR(return_value,  "footer",     player->footer);
    PHPMS_ADD_PROP_STR(return_value,  "connection", player->connection);
    add_property_long(return_value,   "connectiontype",player->connectiontype);
    PHPMS_ADD_PROP_STR(return_value,  "filteritem", player->filteritem);

    return layer_id;
}


/**********************************************************************
 *                        ms_newLayerObj()
 **********************************************************************/

/* {{{ proto layerObj ms_newLayerObj(mapObj map)
   Create a new layer in the specified map. */

DLEXPORT void php3_ms_lyr_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pMapObj;
    mapObj *parent_map;
    layerObj *pNewLayer;
    int map_id;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (getParameters(ht, 1, &pMapObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_map = (mapObj*)_phpms_fetch_handle(pMapObj, 
                                              PHPMS_GLOBAL(le_msmap),
                                              list);

    if (parent_map == NULL ||
        (pNewLayer = layerObj_new(parent_map)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Update mapObj members */
    _phpms_set_property_long(pMapObj, "numlayers",
                             parent_map->numlayers, E_ERROR); 

    /* We will store a reference to the parent_map object id inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_resource(pMapObj, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_layer_object(pNewLayer, map_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        layer->set()
 **********************************************************************/

/* {{{ proto int layer.set(string property_name, new_value)
   Set layer object property to new value. Returns -1 on error. */

DLEXPORT void php3_ms_lyr_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, le_mslayer, list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_LONG(       "status",     self->status)
    else IF_SET_STRING("classitem",  self->classitem)
    else IF_SET_STRING("name",       self->name)
    else IF_SET_STRING("group",      self->group)
    else IF_SET_STRING("data",       self->data)
    else IF_SET_LONG(  "type",       self->type)
    else IF_SET_DOUBLE("tolerance",  self->tolerance)
    else IF_SET_LONG(  "toleranceunits",self->toleranceunits)
    else IF_SET_DOUBLE("symbolscale",self->symbolscale)
    else IF_SET_DOUBLE("minscale",   self->minscale)
    else IF_SET_DOUBLE("maxscale",   self->maxscale)
    else IF_SET_DOUBLE("labelminscale",self->labelminscale)
    else IF_SET_DOUBLE("labelmaxscale",self->labelmaxscale)
    else IF_SET_LONG(  "maxfeatures",self->maxfeatures)
    else IF_SET_LONG(  "offsite",    self->offsite)
    else IF_SET_LONG(  "transform",  self->transform)
    else IF_SET_LONG(  "labelcache", self->labelcache)
    else IF_SET_LONG(  "postlabelcache", self->postlabelcache)
    else IF_SET_STRING("labelitem",  self->labelitem)
    else IF_SET_STRING("labelsizeitem",self->labelsizeitem)
    else IF_SET_STRING("labelangleitem",self->labelangleitem)
    else IF_SET_STRING("tileitem",   self->tileitem)
    else IF_SET_STRING("tileindex",  self->tileindex)
    else IF_SET_STRING("header",     self->header)
    else IF_SET_STRING("footer",     self->footer)
    else IF_SET_STRING("connection", self->connection)
    else IF_SET_LONG(  "connectiontype", self->connectiontype)
    else IF_SET_STRING("filteritem", self->filteritem)
    else if (strcmp( "numclasses", pPropertyName->value.str.val) == 0 ||
             strcmp( "index",      pPropertyName->value.str.val) == 0 )
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        layer->draw()
 **********************************************************************/

/* {{{ proto int layer.draw(image img)
   Draw a single layer, add labels to cache if required. Returns -1 on error. */

DLEXPORT void php3_ms_lyr_draw(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *imgObj, *pThis;
    mapObj *parent_map;
    layerObj *self;
    gdImagePtr im = NULL;
    int    retVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &imgObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    im = (gdImagePtr)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list);
   
    self = (layerObj *)_phpms_fetch_handle(pThis, 
                                           PHPMS_GLOBAL(le_mslayer),list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (im == NULL || self == NULL || parent_map == NULL ||
        (retVal = layerObj_draw(self, parent_map, im)) == -1)
        _phpms_report_mapserver_error(E_WARNING);

    RETURN_LONG(retVal);
}
/* }}} */

/**********************************************************************
 *                        layer->drawQuery()
 **********************************************************************/

/* {{{ proto int layer.drawQuery(image img)
   Draw query results for a layer. */

DLEXPORT void php3_ms_lyr_drawQuery(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *imgObj, *pThis;
    mapObj *parent_map;
    layerObj *self;
    gdImagePtr im = NULL;
    int    retVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &imgObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    im = (gdImagePtr)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list);
   
    self = (layerObj *)_phpms_fetch_handle(pThis, 
                                           PHPMS_GLOBAL(le_mslayer),list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (im == NULL || self == NULL || parent_map == NULL ||
        (retVal = layerObj_drawQuery(self, parent_map, im)) == -1)
        _phpms_report_mapserver_error(E_WARNING);

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        layer->getClass()
 *
 * Note: Multiple calls to getClass() will return multiple instances
 * of PHP objects pointing to the same classObj... this is safe but is a
 * waste of resources.
 **********************************************************************/

/* {{{ proto int layer.getClass(int i)
   Returns a classObj from the layer given an index value (0=first class) */

DLEXPORT void php3_ms_lyr_getClass(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pClassIndex, *pThis;
    layerObj *self=NULL;
    classObj *newClass=NULL;
    int layer_id;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pClassIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    /* pClassIndex is the 0-based index of the requested class */
    convert_to_long(pClassIndex);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self == NULL || 
        (newClass = layerObj_getClass(self, pClassIndex->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* We will store a reference to the parent object id (this) inside
     * the class obj.
     */
    layer_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_class_object(newClass, layer_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        layer->queryByAttributes()
 *
 * Mode is MS_SINGLE or MS_MULTIPLE depending on number of results
 * you want. 
 **********************************************************************/

/* {{{ proto int layer.queryAttributes(int mode)
   Query at point location. */

DLEXPORT void php3_ms_lyr_queryByAttributes(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pType;
    layerObj *self=NULL;
    mapObj   *parent_map;
    int      nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pType) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (self && parent_map &&
        (nStatus=layerObj_queryByAttributes(self, parent_map,
                                       pType->value.lval)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/**********************************************************************
 *                        layer->queryByPoint()
 *
 * Type is MS_SINGLE or MS_MULTIPLE depending on number of results
 * you want. Passing buffer <=0 defaults to tolerances set in the map 
 * file (in pixels) but you can use a constant buffer (specified in 
 * ground units) instead.
 **********************************************************************/

/* {{{ proto int layer.queryByPoint(pointObj point, int type, double buffer)
   Query at point location. */

DLEXPORT void php3_ms_lyr_queryByPoint(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pPoint, *pType, *pBuffer;
    layerObj *self=NULL;
    mapObj   *parent_map;
    pointObj *poPoint=NULL;
    int      nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 3, &pPoint, &pType, &pBuffer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);
    convert_to_double(pBuffer);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (self && poPoint && parent_map &&
        (nStatus=layerObj_queryByPoint(self, parent_map, poPoint, 
                                       pType->value.lval, 
                                       pBuffer->value.dval)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/************************************************************************/
/*                         layer->queryByRect()                         */
/*                                                                      */
/*      Query on a layer using rectangular extents.                     */
/************************************************************************/

/* {{{ proto int layer.queryByRect(rectObj poRect)
   Query using rectangular extent. */

DLEXPORT void php3_ms_lyr_queryByRect(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pRect;
    layerObj *self=NULL;
    mapObj   *parent_map;
    rectObj *poRect=NULL;
    int      nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pRect) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    poRect = (rectObj *)_phpms_fetch_handle2(pRect,
                                             PHPMS_GLOBAL(le_msrect_ref),
                                             PHPMS_GLOBAL(le_msrect_new),
                                             list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (self && poRect && parent_map &&
        (nStatus=layerObj_queryByRect(self, parent_map, 
                                      *poRect)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/************************************************************************/
/*                       layer->queryByFeatures()                       */
/*                                                                      */
/*      Query on a layer using query object.                            */
/************************************************************************/
DLEXPORT void php3_ms_lyr_queryByFeatures(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pSLayer;
    layerObj *self=NULL;
    mapObj   *parent_map;
    int      nStatus = MS_FAILURE;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pSLayer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pSLayer);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (self && parent_map &&
        (nStatus=layerObj_queryByFeatures(self, parent_map, 
                                          pSLayer->value.lval)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/************************************************************************/
/*                        layer->queryByShape()                         */
/*                                                                      */
/*      Query on a layer using a shape object.                          */
/************************************************************************/

/* {{{ proto int layer.queryByRect(ShapeObj poShape)
   Query using a shape */

DLEXPORT void php3_ms_lyr_queryByShape(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pShape;
    layerObj *self=NULL;
    mapObj   *parent_map;
    shapeObj *poShape=NULL;
    int      nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    poShape = 
        (shapeObj *)_phpms_fetch_handle2(pShape, 
                                         PHPMS_GLOBAL(le_msshape_ref),
                                         PHPMS_GLOBAL(le_msshape_new),
                                         list);

    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (self && poShape && parent_map &&
        (nStatus=layerObj_queryByShape(self, parent_map, 
                                       poShape)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}
/* }}} */


/**********************************************************************
 *                        layer->setFilter()
 **********************************************************************/

/* {{{ proto int layer.setFilter(string filter)
   Set layer filter expression.  Returns 0 on success, -1 in error. */

DLEXPORT void php3_ms_lyr_setFilter(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pFilterString;
    pval   *pThis;
    int     nStatus = -1;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pFilterString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFilterString);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self == NULL || 
        (nStatus = layerObj_setFilter(self, 
                                      pFilterString->value.str.val)) != 0)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/**********************************************************************
 *                        layer->setProjection()
 **********************************************************************/

/* {{{ proto int layer.setProjection(string projection)
   Set projection and coord. system for the layer. Returns -1 on error. */

DLEXPORT void php3_ms_lyr_setProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pProjString;
    pval   *pThis;
    int     nStatus = 0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pProjString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pProjString);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    if (self == NULL || 
        (nStatus = layerObj_setProjection(self, 
                                          pProjString->value.str.val)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(nStatus);
}
/* }}} */

/************************************************************************/
/*                        layer->addFeature                             */
/*                                                                      */
/*      Add a new feature in a layer.                                   */
/************************************************************************/

/* {{{ proto int layer.addFeature(ShapeObj poShape)
   Add a shape */
DLEXPORT void php3_ms_lyr_addFeature(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pShape;
    layerObj    *self=NULL;
    shapeObj    *poShape=NULL;
    int         nResult = -1;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    poShape = 
        (shapeObj *)_phpms_fetch_handle2(pShape, 
                                         PHPMS_GLOBAL(le_msshape_ref),
                                         PHPMS_GLOBAL(le_msshape_new),
                                         list);

    if (self && poShape)
    {
        nResult = layerObj_addFeature(self, poShape);
    }

    RETURN_LONG(nResult);
}

/* }}} */

/**********************************************************************
 *                        layer->getNumResults()
 **********************************************************************/

/* {{{ proto int layer.getNumResults()
   Returns the number of results from this layer in the last query. */

DLEXPORT void php3_ms_lyr_getNumResults(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
    layerObj *self=NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0) 
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self && self->resultcache) 
    {
        RETURN_LONG(self->resultcache->numresults);
    }

    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        layer->getResult(i)
 **********************************************************************/

/* {{{ proto int layer.getResult(int i)
   Returns a resultCacheMemberObj by index from a layer object.  Returns a valid object or FALSE(0) if index is invalid. */

DLEXPORT void php3_ms_lyr_getResult(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    layerObj *self;
    resultCacheMemberObj *poResult;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);

    if (self== NULL ||
        (poResult = layerObj_getResult(self, pIndex->value.lval)) == NULL)
    {
        /* Invalid result index. */
        RETURN_FALSE;
    }

    /* Return a resultCacheMemberObj object */
    _phpms_build_resultcachemember_object(&(self->resultcache->
                                            results[pIndex->value.lval]),
                                          list, return_value);
}
/* }}} */



/**********************************************************************
 *                        layer->open()
 **********************************************************************/

/* {{{ proto int layer.open(string shapepath)
   Open the layer for use with getShape().  Returns MS_SUCCESS/MS_FAILURE. */

DLEXPORT void php3_ms_lyr_open(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis, *pShapePath;
    layerObj *self=NULL;
    int         nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pShapePath) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pShapePath);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self == NULL || 
        (nStatus = layerObj_open(self, 
                                 pShapePath->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }
    else
    {
        // Until we implement selection of fields, default to selecting
        // all fields
        msLayerGetItems(self);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/**********************************************************************
 *                        layer->close()
 **********************************************************************/

/* {{{ proto int layer.close()
   Close layer previously opened with open(). */

DLEXPORT void php3_ms_lyr_close(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
    layerObj *self=NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self)
        layerObj_close(self);

}
/* }}} */

/**********************************************************************
 *                        layer->getShape()
 **********************************************************************/

/* {{{ proto shapeObj layer.getShape(tileindex, shapeindex)
   Retrieve shapeObj from a layer by index. */

DLEXPORT void php3_ms_lyr_getShape(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis, *pTileId, *pShapeId ;
    layerObj *self=NULL;
    shapeObj    *poShape;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pTileId, &pShapeId) != SUCCESS) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pTileId);
    convert_to_long(pShapeId);

    /* Create a new shapeObj to hold the result 
     * Note that the type used to create the shape (MS_NULL) does not matter
     * at this point since it will be set by SHPReadShape().
     */
    if ((poShape = shapeObj_new(MS_SHAPE_NULL)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new shape (out of memory?)");
        RETURN_FALSE;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);

    if (self == NULL || 
        layerObj_getShape(self, poShape, pTileId->value.lval, 
                          pShapeId->value.lval) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
        shapeObj_destroy(poShape);
        RETURN_FALSE; 
    }

    /* Return valid object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), self,
                              list, return_value);
}
/* }}} */


/**********************************************************************
 *                        layer->getMetaData()
 **********************************************************************/

/* {{{ proto string layer.getMetaData(string name)
   Return MetaData entry by name, or empty string if not found. */

DLEXPORT void php3_ms_lyr_getMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pThis, *pName;
    char   *pszValue = NULL;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pName) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pName);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self == NULL || 
        (pszValue = layerObj_getMetaData(self, pName->value.str.val)) == NULL)
    {
        pszValue = "";
    }

    RETURN_STRING(pszValue, 1);
}
/* }}} */

/**********************************************************************
 *                        layer->setMetaData()
 **********************************************************************/

/* {{{ proto int layer.setMetaData(string name, string value)
   Set MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_lyr_setMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pThis, *pName, *pValue;
    int    nStatus = MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pName, &pValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pName);
    convert_to_string(pValue);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self == NULL || 
        (nStatus = layerObj_setMetaData(self, 
                                        pName->value.str.val,  
                                        pValue->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - labelObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_label_object()
 **********************************************************************/
static long _phpms_build_label_object(labelObj *plabel,
                                      HashTable *list, pval *return_value)
{
    int label_id;

    if (plabel == NULL)
        return 0;

    label_id = php3_list_insert(plabel, PHPMS_GLOBAL(le_mslabel));

    _phpms_object_init(return_value, label_id, php_label_class_functions,
                       PHP4_CLASS_ENTRY(label_class_entry_ptr));

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value,  "font",       plabel->font);
    add_property_long(return_value,   "type",       plabel->type);
    add_property_long(return_value,   "color",      plabel->color);
    add_property_long(return_value,   "outlinecolor", plabel->outlinecolor);
    add_property_long(return_value,   "shadowcolor",plabel->shadowcolor);
    add_property_long(return_value,   "shadowsizex",plabel->shadowsizex);
    add_property_long(return_value,   "shadowsizey",plabel->shadowsizey);
    add_property_long(return_value,   "backgroundcolor",
                                                plabel->backgroundcolor);
    add_property_long(return_value,   "backgroundshadowcolor",
                                                plabel->backgroundshadowcolor);
    add_property_long(return_value,   "backgroundshadowsizex",
                                                plabel->backgroundshadowsizex);
    add_property_long(return_value,   "backgroundshadowsizey",
                                                plabel->backgroundshadowsizey);
    add_property_long(return_value,   "size",       plabel->size);
    add_property_long(return_value,   "minsize",    plabel->minsize);
    add_property_long(return_value,   "maxsize",    plabel->maxsize);
    add_property_long(return_value,   "position",   plabel->position);
    add_property_long(return_value,   "offsetx",    plabel->offsetx);
    add_property_long(return_value,   "offsety",    plabel->offsety);
    add_property_double(return_value, "angle",      plabel->angle);
    add_property_long(return_value,   "autoangle",  plabel->autoangle);
    add_property_long(return_value,   "buffer",     plabel->buffer);
    add_property_long(return_value,   "antialias",  plabel->antialias);
    add_property_long(return_value,   "wrap",       plabel->wrap);
    add_property_long(return_value,   "minfeaturesize",plabel->minfeaturesize);
    add_property_long(return_value,   "autominfeaturesize",plabel->autominfeaturesize);
    add_property_long(return_value,   "mindistance",plabel->mindistance);
    add_property_long(return_value,   "partials",   plabel->partials);
    add_property_long(return_value,   "force",      plabel->force);

    return label_id;
}


/**********************************************************************
 *                        label->set()
 **********************************************************************/

/* {{{ proto int label.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_label_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    labelObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (labelObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslabel),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "font",         self->font)
    else IF_SET_LONG(  "type",         self->type)
    else IF_SET_LONG(  "color",        self->color)
    else IF_SET_LONG(  "outlinecolor", self->outlinecolor)
    else IF_SET_LONG(  "shadowcolor",  self->shadowcolor)
    else IF_SET_LONG(  "shadowsizex",  self->shadowsizex)
    else IF_SET_LONG(  "shadowsizey",  self->shadowsizey)
    else IF_SET_LONG(  "backgroundcolor",       self->backgroundcolor)
    else IF_SET_LONG(  "backgroundshadowcolor", self->backgroundshadowcolor)
    else IF_SET_LONG(  "backgroundshadowsizex", self->backgroundshadowsizex)
    else IF_SET_LONG(  "backgroundshadowsizey", self->backgroundshadowsizey)
    else IF_SET_LONG(  "size",         self->size)
    else IF_SET_LONG(  "minsize",      self->minsize)
    else IF_SET_LONG(  "maxsize",      self->maxsize)
    else IF_SET_LONG(  "position",     self->position)
    else IF_SET_LONG(  "offsetx",      self->offsetx)
    else IF_SET_LONG(  "offsety",      self->offsety)
    else IF_SET_DOUBLE("angle",        self->angle)
    else IF_SET_LONG(  "autoangle",    self->autoangle)
    else IF_SET_LONG(  "buffer",       self->buffer)
    else IF_SET_LONG(  "antialias",    self->antialias)
    else IF_SET_BYTE(  "wrap",         self->wrap)
    else IF_SET_LONG(  "minfeaturesize", self->minfeaturesize)
    else IF_SET_LONG(  "autominfeaturesize", self->autominfeaturesize)
    else IF_SET_LONG(  "mindistance",  self->mindistance)
    else IF_SET_LONG(  "partials",     self->partials)
    else IF_SET_LONG(  "force",        self->force)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/*=====================================================================
 *                 PHP function wrappers - classObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_class_object()
 **********************************************************************/
static long _phpms_build_class_object(classObj *pclass, int parent_layer_id,
                                      HashTable *list, pval *return_value)
{
    int class_id;
#ifdef PHP4
    pval        *new_obj_ptr;
#else
    pval        new_obj_param;  /* No, it's not a pval * !!! */
    pval        *new_obj_ptr;
    new_obj_ptr = &new_obj_param;
#endif

    if (pclass == NULL)
        return 0;

    class_id = php3_list_insert(pclass, PHPMS_GLOBAL(le_msclass));

    _phpms_object_init(return_value, class_id, php_class_class_functions,
                       PHP4_CLASS_ENTRY(class_class_entry_ptr));

#ifdef PHP4
    add_property_resource(return_value, "_layer_handle_", parent_layer_id);
    zend_list_addref(parent_layer_id);
#else
    add_property_long(return_value, "_layer_handle_", parent_layer_id);
#endif

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value,  "name",       pclass->name);
    add_property_long(return_value,   "type",       pclass->type);
    add_property_long(return_value,   "color",      pclass->color);
    add_property_long(return_value, "backgroundcolor",pclass->backgroundcolor);
    add_property_long(return_value,   "outlinecolor", pclass->outlinecolor);
    add_property_long(return_value,   "overlaycolor", pclass->overlaycolor);
    add_property_long(return_value, "overlaybackgroundcolor",
                                               pclass->overlaybackgroundcolor);
    add_property_long(return_value,   "overlayoutlinecolor", 
                                               pclass->overlayoutlinecolor);
    add_property_long(return_value,   "symbol",     pclass->symbol);
    add_property_long(return_value,   "size",       pclass->size);
    add_property_long(return_value,   "minsize",    pclass->minsize);
    add_property_long(return_value,   "maxsize",    pclass->maxsize);
    PHPMS_ADD_PROP_STR(return_value,  "symbolname", pclass->symbolname);
    add_property_long(return_value,   "overlaysymbol", pclass->overlaysymbol);
    add_property_long(return_value,   "overlaysize", pclass->overlaysize);
    add_property_long(return_value,   "overlayminsize",pclass->overlayminsize);
    add_property_long(return_value,   "overlaymaxsize",pclass->overlaymaxsize);
    PHPMS_ADD_PROP_STR(return_value,  "overlaysymbolname", 
                                                pclass->overlaysymbolname);
    PHPMS_ADD_PROP_STR(return_value,  "template",   pclass->template);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_label_object(&(pclass->label), list, new_obj_ptr);
    _phpms_add_property_object(return_value, "label", new_obj_ptr,E_ERROR);

    return class_id;
}


/**********************************************************************
 *                        ms_newClassObj()
 **********************************************************************/

/* {{{ proto layerObj ms_newClassObj(layerObj layer)
   Create a new class in the specified layer. */

DLEXPORT void php3_ms_class_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pLayerObj;
    layerObj *parent_layer;
    classObj *pNewClass;
    int layer_id;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (getParameters(ht, 1, &pLayerObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_layer = (layerObj*)_phpms_fetch_handle(pLayerObj, 
                                                  PHPMS_GLOBAL(le_mslayer),
                                                  list);

    if (parent_layer == NULL ||
        (pNewClass = classObj_new(parent_layer)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* We will store a reference to the parent_layer object id inside
     * the class obj.
     */
    layer_id = _phpms_fetch_property_resource(pLayerObj, "_handle_", E_ERROR);

    /* Return class object */
    _phpms_build_class_object(pNewClass, layer_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        class->set()
 **********************************************************************/

/* {{{ proto int class.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_class_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    classObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "name",         self->name)
    else IF_SET_LONG(  "type",         self->type)
    else IF_SET_LONG(  "color",        self->color)
    else IF_SET_LONG(  "backgroundcolor",self->backgroundcolor)
    else IF_SET_LONG(  "outlinecolor", self->outlinecolor)
    else IF_SET_LONG(  "overlaycolor", self->overlaycolor)
    else IF_SET_LONG(  "overlaybackgroundcolor",self->overlaybackgroundcolor)
    else IF_SET_LONG(  "overlayoutlinecolor", self->overlayoutlinecolor)
    else IF_SET_LONG(  "symbol",       self->symbol)
    else IF_SET_LONG(  "size",         self->size)
    else IF_SET_LONG(  "minsize",      self->minsize)
    else IF_SET_LONG(  "maxsize",      self->maxsize)
    else IF_SET_STRING("symbolname",   self->symbolname)
    else IF_SET_LONG(  "overlaysymbol",self->overlaysymbol)
    else IF_SET_LONG(  "overlaysize",  self->overlaysize)
    else IF_SET_LONG(  "overlayminsize", self->overlayminsize)
    else IF_SET_LONG(  "overlaymaxsize", self->overlaymaxsize)
    else IF_SET_STRING("overlaysymbolname", self->overlaysymbolname)
    else IF_SET_STRING("template",      self->template)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/************************************************************************/
/*                          class->setExpression()                      */
/*                                                                      */
/*      Set the expression string for a class object.                   */
/*                                                                      */
/*      Returns 0 on success, -1 on error.                              */
/************************************************************************/

/* {{{ proto int class.setExpression(string exression)
   Set the expression string for a class object. */

DLEXPORT void php3_ms_class_setExpression(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pString;
    classObj *self=NULL;
    int     nStatus=-1;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pString) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pString);

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list);

    if (self == NULL || 
        (nStatus = classObj_setExpression(self, pString->value.str.val)) != 0)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_LONG(nStatus);
    }

    RETURN_LONG(0);
}
/* }}} */

/************************************************************************/
/*                          class->setText()                            */
/*                                                                      */
/*      Set the expression string for a class object.                   */
/*                                                                      */
/*      Returns 0 on success, -1 on error.                              */
/************************************************************************/

/* {{{ proto int class.setText(string text)
   Set the text string for a class object. */

DLEXPORT void php3_ms_class_setText(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pString, *pLayerObj;
    classObj    *self=NULL;
    layerObj    *parent_layer;
    int         nStatus=-1;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pLayerObj, &pString) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pString);

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list);

    parent_layer = (layerObj*)_phpms_fetch_handle(pLayerObj, 
                                                  PHPMS_GLOBAL(le_mslayer),
                                                  list);

    if (self == NULL || parent_layer == NULL ||
        (nStatus = classObj_setText(self, parent_layer, pString->value.str.val)) != 0)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_LONG(nStatus);
    }

    RETURN_LONG(0);
}
/* }}} */



/*=====================================================================
 *                 PHP function wrappers - colorObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_color_object()
 **********************************************************************/
static long _phpms_build_color_object(colorObj *pcolor,
                                      HashTable *list, pval *return_value)
{
    int color_id;

    if (pcolor == NULL)
      return 0;

    color_id = php3_list_insert(pcolor, PHPMS_GLOBAL(le_mscolor));

    _phpms_object_init(return_value, color_id, php_color_class_functions,
                       PHP4_CLASS_ENTRY(color_class_entry_ptr));

    /* editable properties */
    add_property_long(return_value,   "red",   pcolor->red);
    add_property_long(return_value,   "green", pcolor->green);
    add_property_long(return_value,   "blue",  pcolor->blue);

    return color_id;
}


/**********************************************************************
 *                        color->setRGB()
 **********************************************************************/

/* {{{ proto int color.setRGB(int R, int G, int B)
   Set new RGB color. Returns -1 on error. */

DLEXPORT void php3_ms_color_setRGB(INTERNAL_FUNCTION_PARAMETERS)
{
    colorObj *self;
    pval   *pR, *pG, *pB, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 3, &pR, &pG, &pB) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (colorObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mscolor),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_long(pR);
    convert_to_long(pG);
    convert_to_long(pB);

    self->red =   pR->value.lval;
    self->green = pG->value.lval;
    self->blue =  pB->value.lval;

    _phpms_set_property_long(pThis, "red",   self->red, E_ERROR);
    _phpms_set_property_long(pThis, "green", self->green, E_ERROR);
    _phpms_set_property_long(pThis, "blue",  self->blue, E_ERROR);

    RETURN_LONG(0);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - pointObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_point_object()
 *
 * handle_type is either le_mspoint_ref for an embedded object or
 * le_mspoint_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_point_object(pointObj *ppoint, int handle_type,
                                      HashTable *list, pval *return_value)
{
    int point_id;

    if (ppoint == NULL)
        return 0;

    point_id = php3_list_insert(ppoint, handle_type);

    _phpms_object_init(return_value, point_id, php_point_class_functions,
                       PHP4_CLASS_ENTRY(point_class_entry_ptr));

    /* editable properties */
    add_property_double(return_value,   "x",   ppoint->x);
    add_property_double(return_value,   "y",   ppoint->y);

    return point_id;
}


/**********************************************************************
 *                        ms_newPointObj()
 **********************************************************************/

/* {{{ proto pointObj ms_newPointObj()
   Create a new pointObj instance. */

DLEXPORT void php3_ms_point_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pointObj *pNewPoint;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pNewPoint = pointObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return point object */
    _phpms_build_point_object(pNewPoint, PHPMS_GLOBAL(le_mspoint_new), 
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        point->setXY()
 **********************************************************************/

/* {{{ proto int point.setXY(double x, double y)
   Set new RGB point. Returns -1 on error. */

DLEXPORT void php3_ms_point_setXY(INTERNAL_FUNCTION_PARAMETERS)
{
    pointObj *self;
    pval   *pX, *pY, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pX, &pY) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pX);
    convert_to_double(pY);

    self->x = pX->value.dval;
    self->y = pY->value.dval;

    _phpms_set_property_double(pThis, "x", self->x, E_ERROR);
    _phpms_set_property_double(pThis, "y", self->y, E_ERROR);

    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        point->project()
 **********************************************************************/

/* {{{ proto int point.project(projectionObj in, projectionObj out)
   Project the point. returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_point_project(INTERNAL_FUNCTION_PARAMETERS)
{
    pval                *pThis, *pIn, *pOut;
    pointObj            *self;
    projectionObj       *poInProj;
    projectionObj       *poOutProj;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pIn, &pOut) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);
    poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);

    if (self && poInProj && poOutProj &&
        (pointObj_project(self, poInProj, poOutProj) != MS_SUCCESS))
    {
        RETURN_FALSE;
    }
    /* Return point object */
    _phpms_build_point_object(self, PHPMS_GLOBAL(le_mspoint_ref),
                              list, return_value);

}
/* }}} */


/**********************************************************************
 *                        point->draw()
 **********************************************************************/

/* {{{ proto int point.draw(mapObj map, layerObj layer, imageObj img, string class_name, string text)
   Draws the individual point using layer. Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_point_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pMap, *pLayer, *pImg, *pClass, *pText;
    pointObj    *self;
    mapObj      *poMap;
    layerObj    *poLayer;
    gdImagePtr  im;
    int         nRetVal=MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 5, &pMap, &pLayer, &pImg, &pClass, &pText) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pClass);
    convert_to_string(pText);

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list);
    im = (gdImagePtr)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list);

    if (self &&
        (nRetVal = pointObj_draw(self, poMap, poLayer, im, 
                                 pClass->value.lval, 
                                 pText->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nRetVal)
}
/* }}} */

/**********************************************************************
 *                        point->distanceToPoint()
 **********************************************************************/

/* {{{ proto int point.distanceToPoint(pointObj)
   Calculates distance between two points. */

DLEXPORT void php3_ms_point_distanceToPoint(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pPoint;
    pointObj    *self;
    pointObj    *poPoint;
    double         dfDist=-1.0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pPoint) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);

    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint, 
                                               PHPMS_GLOBAL(le_mspoint_new), 
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               list);
    if (self != NULL && poPoint != NULL)
        dfDist = pointObj_distanceToPoint(self, poPoint);

    RETURN_DOUBLE(dfDist)
}
/* }}} */

/**********************************************************************
 *                        point->distanceToLine()
 **********************************************************************/

/* {{{ proto int point.distanceToLine(pointObj p1, (pointObj p2)
   Calculates distance between a point ad a lined defined by the
   two points passed in argument. */

DLEXPORT void php3_ms_point_distanceToLine(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pPoint1, *pPoint2;
    pointObj    *self;
    pointObj    *poPoint1;
    pointObj    *poPoint2;
    double         dfDist=-1.0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPoint1, &pPoint2) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);

    poPoint1 = (pointObj *)_phpms_fetch_handle2(pPoint1, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list);

    poPoint2 = (pointObj *)_phpms_fetch_handle2(pPoint2, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list);
    if (self != NULL && poPoint1 != NULL && poPoint2 != NULL)
        dfDist = pointObj_distanceToLine(self, poPoint1, poPoint2);

    RETURN_DOUBLE(dfDist);
}
/* }}} */

/**********************************************************************
 *                        point->distanceToPoint()
 **********************************************************************/

/* {{{ proto int point.distanceToShape(shapeObj shape)
   Calculates the minimum distance between a point and a shape. */
DLEXPORT void php3_ms_point_distanceToShape(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pShape;
    pointObj    *self;
    shapeObj    *poShape;
    double         dfDist=-1.0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);

    poShape = (shapeObj *)_phpms_fetch_handle2(pShape, 
                                               PHPMS_GLOBAL(le_msshape_ref),
                                               PHPMS_GLOBAL(le_msshape_new),
                                               list);
    if (self != NULL && poShape != NULL)
        dfDist = pointObj_distanceToShape(self, poShape);

    RETURN_DOUBLE(dfDist)
}
/* }}} */


/**********************************************************************
 *                        point->free()
 **********************************************************************/

/* {{{ proto int point.free()
   Destroys resources used by a point object */
DLEXPORT void php3_ms_point_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    pointObj *self;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle(pThis, le_mspoint_new, list);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
#ifdef PHP4
        pval **phandle;
        if (zend_hash_find(pThis->value.obj.properties, "_handle_", 
                           sizeof("_handle_"), 
                           (void **)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
#else
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)

        {
            php3_list_delete(phandle->value.lval);
        }
#endif
    }

}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - lineObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_line_object()
 *
 * handle_type is either le_msline_ref for an embedded object or
 * le_msline_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_line_object(lineObj *pline, int handle_type,
                                      HashTable *list, pval *return_value)
{
    int line_id;

    if (pline == NULL)
        return 0;

    line_id = php3_list_insert(pline, handle_type);

    _phpms_object_init(return_value, line_id, php_line_class_functions,
                       PHP4_CLASS_ENTRY(line_class_entry_ptr));

    /* read-only properties */
    add_property_long(return_value, "numpoints", pline->numpoints);

    return line_id;
}


/**********************************************************************
 *                        ms_newLineObj()
 **********************************************************************/

/* {{{ proto lineObj ms_newLineObj()
   Create a new lineObj instance. */

DLEXPORT void php3_ms_line_new(INTERNAL_FUNCTION_PARAMETERS)
{
    lineObj *pNewLine;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pNewLine = lineObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return line object */
    _phpms_build_line_object(pNewLine, PHPMS_GLOBAL(le_msline_new), 
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        line->project()
 **********************************************************************/

/* {{{ proto int line.project(projectionObj in, projectionObj out)
   Project the point. returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_line_project(INTERNAL_FUNCTION_PARAMETERS)
{
    pval                *pThis, *pIn, *pOut;
    lineObj             *self;
    projectionObj       *poInProj;
    projectionObj       *poOutProj;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

     if (pThis == NULL ||
        getParameters(ht, 2, &pIn, &pOut) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list);

    poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);
    if (self && poInProj && poOutProj &&
        (lineObj_project(self, poInProj, poOutProj) != MS_SUCCESS))
    {
         RETURN_FALSE;
    }

    /* Return line object */
    _phpms_build_line_object(self, PHPMS_GLOBAL(le_msline_ref),
                             list, return_value);
}
/* }}} */



/**********************************************************************
 *                        line->add()
 **********************************************************************/

/* {{{ proto int line.add(pointObj point)
   Adds a point to the end of a line */

DLEXPORT void php3_ms_line_add(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pPoint;
    lineObj     *self;
    pointObj    *poPoint;
    int         nRetVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pPoint) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list);

    poPoint = (pointObj*)_phpms_fetch_handle2(pPoint,
                                              PHPMS_GLOBAL(le_mspoint_ref),
                                              PHPMS_GLOBAL(le_mspoint_new),
                                              list);

    if (self && poPoint)
        nRetVal = lineObj_add(self, poPoint);

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        line->addXY()
 **********************************************************************/

/* {{{ proto int line.addXY(double x, double y)
   Adds a point to the end of a line */

DLEXPORT void php3_ms_line_addXY(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pX, *pY;
    lineObj     *self;
    pointObj    oPoint;
    int         nRetVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pX, &pY) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pX);
    convert_to_double(pY);

    oPoint.x = pX->value.dval;
    oPoint.y = pY->value.dval;

    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list);

    if (self)
        nRetVal = lineObj_add(self, &oPoint);

    RETURN_LONG(nRetVal)
}
/* }}} */



/**********************************************************************
 *                        line->point()
 **********************************************************************/

/* {{{ proto int line.point(int i)
   Returns point number i.  First point is number 0. */

DLEXPORT void php3_ms_line_point(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    lineObj     *self;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list);

    if (self==NULL || 
        pIndex->value.lval < 0 || pIndex->value.lval >= self->numpoints)
    {
        RETURN_FALSE;
    }

    /* Return reference to the specified point.  Reference is valid only
     * during the life of the lineObj that contains the point.
     */
    _phpms_build_point_object(&(self->point[pIndex->value.lval]), 
                                PHPMS_GLOBAL(le_mspoint_ref), 
                                list, return_value);
}
/* }}} */


/**********************************************************************
 *                        line->free()
 **********************************************************************/

/* {{{ proto int line.free()
   Destroys resources used by a line object */
DLEXPORT void php3_ms_line_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    lineObj *self;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (lineObj *)_phpms_fetch_handle(pThis, le_msline_new, list);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
#ifdef PHP4
        pval **phandle;
        if (zend_hash_find(pThis->value.obj.properties, "_handle_", 
                           sizeof("_handle_"), 
                           (void **)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
#else
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)

        {
            php3_list_delete(phandle->value.lval);
        }
#endif
    }

}
/* }}} */

/*=====================================================================
 *                 PHP function wrappers - shapeObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_shape_object()
 *
 * handle_type is either le_msshape_ref for an embedded object or
 * le_msshape_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_shape_object(shapeObj *pshape, int handle_type,
                                      layerObj *pLayer,
                                      HashTable *list, pval *return_value)
{
    int     shape_id;
#ifdef PHP4
    pval        *new_obj_ptr;
#else
    pval        new_obj_param;  /* No, it's not a pval * !!! */
    pval        *new_obj_ptr;
    new_obj_ptr = &new_obj_param;
#endif

    if (pshape == NULL)
        return 0;

    shape_id = php3_list_insert(pshape, handle_type);

    _phpms_object_init(return_value, shape_id, php_shape_class_functions,
                       PHP4_CLASS_ENTRY(shape_class_entry_ptr));

    /* read-only properties */
    add_property_long(return_value, "numlines", pshape->numlines);
    add_property_long(return_value, "type",     pshape->type);
    add_property_long(return_value, "index",    pshape->index);
    add_property_long(return_value, "tileindex", pshape->tileindex);
    add_property_long(return_value, "classindex", pshape->classindex);
    add_property_long(return_value, "numvalues", pshape->numvalues);
    PHPMS_ADD_PROP_STR(return_value,"text",     pshape->text);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_rect_object(&(pshape->bounds), PHPMS_GLOBAL(le_msrect_ref), 
                             list, new_obj_ptr);
    _phpms_add_property_object(return_value, "bounds", new_obj_ptr,E_ERROR);

    /* Package values as an associative array
     * For now we do this only for shapes returned from map layers, and not
     * for those from shapefileObj.
     */
    if (pLayer && pshape->numvalues && pshape->numvalues == pLayer->numitems)
    {
        int i;
#ifdef PHP4
        MAKE_STD_ZVAL(new_obj_ptr);
#endif
        array_init(new_obj_ptr);
        for(i=0; i<pshape->numvalues; i++)
        {
            add_assoc_string(new_obj_ptr, 
                             pLayer->items[i], pshape->values[i], 1);
        }
        _phpms_add_property_object(return_value, "values", 
                                   new_obj_ptr, E_ERROR);
    }
    else if (pLayer)
    {
        php3_error(E_ERROR,
                   "Assertion failed, Could not set shape values: %d, %d",
                   pshape->numvalues, pLayer->numitems);
    }

    return shape_id;
}


/**********************************************************************
 *                        ms_newShapeObj()
 **********************************************************************/

/* {{{ proto shapeObj ms_newShapeObj()
   Create a new shapeObj instance. */

DLEXPORT void php3_ms_shape_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pType;
    shapeObj *pNewShape;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (getParameters(ht, 1, &pType) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);

    if ((pNewShape = shapeObj_new(pType->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(pNewShape, PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value);
}
/* }}} */


/**********************************************************************
 *                        shape->set()
 **********************************************************************/

/* {{{ proto int shape.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_shape_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    shapeObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "text",         self->text)
    else IF_SET_LONG(  "classindex",   self->classindex)
    else if (strcmp( "numlines",  pPropertyName->value.str.val) == 0 ||
             strcmp( "type",      pPropertyName->value.str.val) == 0 ||
             strcmp( "index",     pPropertyName->value.str.val) == 0 ||
             strcmp( "tileindex", pPropertyName->value.str.val) == 0 ||
             strcmp( "numvalues", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }         
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */


/**********************************************************************
 *                        shape->project()
 **********************************************************************/

/* {{{ proto int shape.project(projectionObj in, projectionObj out)
   Project a Shape. Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_shape_project(INTERNAL_FUNCTION_PARAMETERS)
{
    pval                *pThis, *pIn, *pOut;
    shapeObj            *self;
    projectionObj       *poInProj;
    projectionObj       *poOutProj;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pIn, &pOut) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

     self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msshape_ref),
                                           PHPMS_GLOBAL(le_msshape_new),
                                           list);
     poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);

    if (self && poInProj && poOutProj &&
        (shapeObj_project(self, poInProj, poOutProj) != MS_SUCCESS))
    {
         RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(self, PHPMS_GLOBAL(le_msshape_ref), NULL,
                              list, return_value);

}
/* }}} */

/**********************************************************************
 *                        shape->add()
 **********************************************************************/

/* {{{ proto int shape.add(lineObj line)
   Adds a line (i.e. a part) to a shape */

DLEXPORT void php3_ms_shape_add(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pLine;
    shapeObj     *self;
    lineObj    *poLine;
    int         nRetVal=0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pLine) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msshape_ref),
                                           PHPMS_GLOBAL(le_msshape_new),
                                           list);

    poLine = (lineObj*)_phpms_fetch_handle2(pLine,
                                            PHPMS_GLOBAL(le_msline_ref),
                                            PHPMS_GLOBAL(le_msline_new),
                                            list);

    if (self && poLine)
        nRetVal = shapeObj_add(self, poLine);

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        shape->line()
 **********************************************************************/

/* {{{ proto int shape.line(int i)
   Returns line (part) number i.  First line is number 0. */

DLEXPORT void php3_ms_shape_line(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    shapeObj     *self;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msshape_ref),
                                           PHPMS_GLOBAL(le_msshape_new),
                                           list);

    if (self==NULL || 
        pIndex->value.lval < 0 || pIndex->value.lval >= self->numlines)
    {
        RETURN_FALSE;
    }

    /* Return reference to the specified line.  Reference is valid only
     * during the life of the shapeObj that contains the line.
     */
    _phpms_build_line_object(&(self->line[pIndex->value.lval]), 
                                PHPMS_GLOBAL(le_msline_ref), 
                                list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shape->draw()
 **********************************************************************/

/* {{{ proto int shape.draw(mapObj map, layerObj layer, imageObj img)
   Draws the individual shape using layer. Returns MS_SUCCESS/MS_FAILURE*/

DLEXPORT void php3_ms_shape_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pMap, *pLayer, *pImg;
    shapeObj    *self;
    mapObj      *poMap;
    layerObj    *poLayer;
    gdImagePtr  im;
    int         nRetVal=MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 3, &pMap, &pLayer, &pImg) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list);
    im = (gdImagePtr)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list);

    if (self && 
        (nRetVal = shapeObj_draw(self, poMap, poLayer, im)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nRetVal);
}
/* }}} */


/**********************************************************************
 *                        shape->contains()
 **********************************************************************/
/* {{{ proto int shape.contains(pointObj pPoint)
   Returns true or false if the the point is in a polygone shape.*/

DLEXPORT void php3_ms_shape_contains(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pPoint;
    shapeObj     *self;
    pointObj    *poPoint;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pPoint) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list);
     poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list);
    if (self==NULL || poPoint == NULL || !shapeObj_contains(self, poPoint))
      RETURN_FALSE;

    RETURN_TRUE;
}
/* }}} */

/**********************************************************************
 *                        shape->intersects()
 **********************************************************************/
/* {{{ proto int shape.intersects(shapeObj pShape)
   Returns true if the two shapes intersect. Else false.*/

DLEXPORT void php3_ms_shape_intersects(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pShape;
    shapeObj     *self;
    shapeObj    *poShape;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list);
    poShape = 
    (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list);
    
    if (self==NULL || poShape == NULL || !shapeObj_intersects(self, poShape))
      RETURN_FALSE;

    RETURN_TRUE;
}
/* }}} */

/**********************************************************************
 *                        shape->getValue()
 **********************************************************************/
/* {{{ proto string shape.getValue(layerObj layer, string fieldName)
   Returns value for specified field name. */

DLEXPORT void php3_ms_shape_getvalue(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pLayer, *pFieldName;
    shapeObj    *self;
    layerObj    *poLayer;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pLayer, &pFieldName) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list);

    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list);
    
    if (self && poLayer && self->numvalues == poLayer->numitems)
    {
        int i;
        for(i=0; i<poLayer->numitems; i++)
        {
            if (strcasecmp(poLayer->items[i], pFieldName->value.str.val)==0)
            {
                RETURN_STRING(self->values[i], 1);
            }
        }
    }

    RETURN_STRING("", 1);
}
/* }}} */

/**********************************************************************
 *                        shape->free()
 **********************************************************************/

/* {{{ proto int point.free()
   Destroys resources used by a point object */
DLEXPORT void php3_ms_shape_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    shapeObj *self;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle(pThis, le_msshape_new, list);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
#ifdef PHP4
        pval **phandle;
        if (zend_hash_find(pThis->value.obj.properties, "_handle_", 
                           sizeof("_handle_"), 
                           (void **)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
#else
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)

        {
            php3_list_delete(phandle->value.lval);
        }
#endif
    }

}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - webObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_web_object()
 **********************************************************************/
static long _phpms_build_web_object(webObj *pweb, 
                                    HashTable *list, pval *return_value)
{
    int         web_id;
#ifdef PHP4
    pval        *new_obj_ptr;
#else
    pval        new_obj_param;  /* No, it's not a pval * !!! */
    pval        *new_obj_ptr;
    new_obj_ptr = &new_obj_param;
#endif

    if (pweb == NULL)
        return 0;

    web_id = php3_list_insert(pweb, PHPMS_GLOBAL(le_msweb));

    _phpms_object_init(return_value, web_id, php_web_class_functions,
                       PHP4_CLASS_ENTRY(web_class_entry_ptr));

    PHPMS_ADD_PROP_STR(return_value,  "log",            pweb->log);
    PHPMS_ADD_PROP_STR(return_value,  "imagepath",      pweb->imagepath);
    PHPMS_ADD_PROP_STR(return_value,  "template",       pweb->template);
    PHPMS_ADD_PROP_STR(return_value,  "imageurl",       pweb->imageurl);
    PHPMS_ADD_PROP_STR(return_value,  "header",         pweb->header);
    PHPMS_ADD_PROP_STR(return_value,  "footer",         pweb->footer);
    PHPMS_ADD_PROP_STR(return_value,  "empty",          pweb->empty);
    PHPMS_ADD_PROP_STR(return_value,  "error",          pweb->error);
    PHPMS_ADD_PROP_STR(return_value,  "mintemplate",    pweb->mintemplate);
    PHPMS_ADD_PROP_STR(return_value,  "maxtemplate",    pweb->maxtemplate);
    add_property_double(return_value,   "minscale",       pweb->minscale);
    add_property_double(return_value,   "maxscale",       pweb->maxscale);
    
#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_rect_object(&(pweb->extent), PHPMS_GLOBAL(le_msrect_ref), 
                             list, new_obj_ptr);
    _phpms_add_property_object(return_value, "extent", new_obj_ptr,E_ERROR);

    return web_id;
}

/**********************************************************************
 *                        web->set()
 **********************************************************************/

/* {{{ proto int web.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */


DLEXPORT void php3_ms_web_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    webObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (webObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msweb),
                                         list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    
    IF_SET_STRING(       "log",         self->log)
    else IF_SET_STRING(  "imagepath",   self->imagepath)
    else IF_SET_STRING(  "template",   self->template) 
    else IF_SET_STRING(  "imageurl",   self->imageurl)
    else IF_SET_STRING(  "header",   self->header)
    else IF_SET_STRING(  "footer",   self->footer)
    else IF_SET_STRING(  "mintemplate",   self->mintemplate) 
    else IF_SET_STRING(  "maxtemplate",   self->maxtemplate) 
    else IF_SET_LONG(    "minscale",   self->minscale)
    else IF_SET_LONG(    "maxscale",   self->maxscale)  
    else if (strcmp( "empty", pPropertyName->value.str.val) == 0 ||
             strcmp( "error",  pPropertyName->value.str.val) == 0 ||
             strcmp( "extent", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }         
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}           



/*=====================================================================
 *                 PHP function wrappers - rectObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_rect_object()
 * handle_type is either le_msrect_ref for an embedded object or
 * le_msrect_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_rect_object(rectObj *prect, int handle_type, 
                                     HashTable *list, pval *return_value)
{
    int rect_id;

    if (prect == NULL)
      return 0;

    rect_id = php3_list_insert(prect, handle_type);

    _phpms_object_init(return_value, rect_id, php_rect_class_functions,
                       PHP4_CLASS_ENTRY(rect_class_entry_ptr));

    add_property_double(return_value,   "minx",       prect->minx);
    add_property_double(return_value,   "miny",       prect->miny);
    add_property_double(return_value,   "maxx",       prect->maxx);
    add_property_double(return_value,   "maxy",       prect->maxy);

    return rect_id;
}


/**********************************************************************
 *                        ms_newRectObj()
 **********************************************************************/

/* {{{ proto rectObj ms_newRectObj()
   Create a new rectObj instance. */

DLEXPORT void php3_ms_rect_new(INTERNAL_FUNCTION_PARAMETERS)
{
    rectObj *pNewRect;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pNewRect = rectObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return rect object */
    _phpms_build_rect_object(pNewRect, PHPMS_GLOBAL(le_msrect_new), 
                             list, return_value);
}
/* }}} */


/**********************************************************************
 *                        rect->project()
 **********************************************************************/

/* {{{ proto int rect.project(projectionObj in, projectionObj out)
   Project a Rect object Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_rect_project(INTERNAL_FUNCTION_PARAMETERS)
{
    pval                *pThis, *pIn, *pOut;
    rectObj             *self;
    projectionObj       *poInProj;
    projectionObj       *poOutProj;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pIn, &pOut) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (rectObj *)_phpms_fetch_handle2(pThis, PHPMS_GLOBAL(le_msrect_ref),
                                           PHPMS_GLOBAL(le_msrect_new), list);
    poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list);

    if (self && poInProj && poOutProj &&
        (rectObj_project(self, poInProj, poOutProj) != MS_SUCCESS))
    {
        RETURN_FALSE;
    }
    
    /* Return rect object */
    _phpms_build_rect_object(self, PHPMS_GLOBAL(le_msrect_ref),
                             list, return_value);

}
/* }}} */

/**********************************************************************
 *                        rect->set()
 **********************************************************************/

/* {{{ proto int rect.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_rect_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    rectObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (rectObj *)_phpms_fetch_handle2(pThis, PHPMS_GLOBAL(le_msrect_ref),
                                           PHPMS_GLOBAL(le_msrect_new), list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_DOUBLE(        "minx",   self->minx)
    else IF_SET_DOUBLE(   "miny",   self->miny)  
    else IF_SET_DOUBLE(   "maxx",   self->maxx) 
    else IF_SET_DOUBLE(   "maxy",   self->maxy)           
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }
         
    RETURN_LONG(0);
}           

/**********************************************************************
 *                        rect->setextent()
 **********************************************************************/

/* {{{ proto int rect.setextent(xmin, ymin, xmax, ymax)
   Set object property using four values.
*/
DLEXPORT void php3_ms_rect_setExtent(INTERNAL_FUNCTION_PARAMETERS)
{
    rectObj *self;
    pval   *pXMin, *pYMin, *pXMax, *pYMax, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 4, &pXMin, &pYMin, &pXMax, &pYMax) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (rectObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msrect_ref),
                                           PHPMS_GLOBAL(le_msrect_new),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pXMin);
    convert_to_double(pYMin);
    convert_to_double(pXMax);
    convert_to_double(pYMax);
    
    self->minx = pXMin->value.dval;
    self->miny = pYMin->value.dval;
    self->maxx = pXMax->value.dval;
    self->maxy = pYMax->value.dval;

    _phpms_set_property_double(pThis, "minx", self->minx, E_ERROR);
    _phpms_set_property_double(pThis, "miny", self->miny, E_ERROR);
    _phpms_set_property_double(pThis, "maxx", self->maxx, E_ERROR);
    _phpms_set_property_double(pThis, "maxy", self->maxy, E_ERROR);

    RETURN_LONG(0);
}
/* }}} */


/**********************************************************************
 *                        rect->draw()
 **********************************************************************/

/* {{{ proto int rect.draw(mapObj map, layerObj layer, imageObj img, string class_name, string text)
   Draws the individual rect using layer. Returns MS_SUCCESS/MS_FAILURE. */

DLEXPORT void php3_ms_rect_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pMap, *pLayer, *pImg, *pClass, *pText;
    rectObj    *self;
    mapObj      *poMap;
    layerObj    *poLayer;
    gdImagePtr  im;
    int         nRetVal=MS_FAILURE;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 5, &pMap, &pLayer, &pImg, &pClass, &pText) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pClass);
    convert_to_string(pText);

    self = (rectObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list);
    im = (gdImagePtr)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list);

    if (self &&
        (nRetVal = rectObj_draw(self, poMap, poLayer, im, 
                                pClass->value.lval, 
                                pText->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        rect->fit()
 **********************************************************************/

/* {{{ proto int rect.fit(int nWidth, int nHeight)
   Adjust extents of the rectangle to fit the width/height specified. */

DLEXPORT void php3_ms_rect_fit(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pWidth, *pHeight;
    rectObj     *self;
    double      dfRetVal=0.0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pWidth, &pHeight) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pWidth);
    convert_to_long(pHeight);

    self = (rectObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msrect_ref),
                                           PHPMS_GLOBAL(le_msrect_new),
                                           list);

    if (self != NULL)
      dfRetVal = rectObj_fit(self, pWidth->value.lval, pHeight->value.lval);

    RETURN_DOUBLE(dfRetVal)
}
/* }}} */

/**********************************************************************
 *                        rect->free()
 **********************************************************************/

/* {{{ proto int rect.free()
   Destroys resources used by a rect object */
DLEXPORT void php3_ms_rect_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    rectObj *self;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (rectObj *)_phpms_fetch_handle(pThis, le_msrect_new, list);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
#ifdef PHP4
        pval **phandle;
        if (zend_hash_find(pThis->value.obj.properties, "_handle_", 
                           sizeof("_handle_"), 
                           (void **)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
#else
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)

        {
            php3_list_delete(phandle->value.lval);
        }
#endif
    }

}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - referneceMapObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_referenceMap_object()
 **********************************************************************/
static long _phpms_build_referenceMap_object(referenceMapObj *preference, 
                                    HashTable *list, pval *return_value)
{
    int         reference_id;
#ifdef PHP4
    pval        *new_obj_ptr;
#else
    pval        new_obj_param;  /* No, it's not a pval * !!! */
    pval        *new_obj_ptr;
    new_obj_ptr = &new_obj_param;
#endif

    if (preference == NULL)
        return 0;

    reference_id = php3_list_insert(preference, PHPMS_GLOBAL(le_msrefmap));

    _phpms_object_init(return_value, reference_id, 
                       php_reference_class_functions,
                       PHP4_CLASS_ENTRY(reference_class_entry_ptr));

    PHPMS_ADD_PROP_STR(return_value,  "image",   preference->image);
    add_property_long(return_value,   "width",  preference->width);
    add_property_long(return_value,   "height",  preference->height);
    add_property_long(return_value,   "status",  preference->status);
    
#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_rect_object(&(preference->extent), 
                             PHPMS_GLOBAL(le_msrect_ref),list, new_obj_ptr);
    _phpms_add_property_object(return_value, "extent", new_obj_ptr, E_ERROR);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_color_object(&(preference->color),list, new_obj_ptr);
    _phpms_add_property_object(return_value, "color", new_obj_ptr, E_ERROR);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_color_object(&(preference->outlinecolor),list, new_obj_ptr);
    _phpms_add_property_object(return_value, "outlinecolor", 
                               new_obj_ptr, E_ERROR);

    return reference_id;
}


/**********************************************************************
 *                        referenceMap->set()
 **********************************************************************/

/* {{{ proto int web.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_referenceMap_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    referenceMapObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (referenceMapObj *)_phpms_fetch_handle(pThis, 
                                                  PHPMS_GLOBAL(le_msrefmap),
                                                  list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    
    IF_SET_STRING(       "image",    self->image)
    else IF_SET_LONG(    "width",    self->width)
    else IF_SET_LONG(    "height",   self->height)
    else IF_SET_LONG(    "status",   self->status) 
    else if (strcmp( "extent", pPropertyName->value.str.val) == 0 ||
             strcmp( "color",  pPropertyName->value.str.val) == 0 ||
             strcmp( "outlinecolor", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }                  
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}           
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - shapefileObj class
 *====================================================================*/


/**********************************************************************
 *                     _phpms_build_shapefile_object()
 **********************************************************************/
static long _phpms_build_shapefile_object(shapefileObj *pshapefile,
                                          HashTable *list, pval *return_value)
{
    int shapefile_id;
#ifdef PHP4
    pval        *new_obj_ptr;
#else
    pval        new_obj_param;  /* No, it's not a pval * !!! */
    pval        *new_obj_ptr;
    new_obj_ptr = &new_obj_param;
#endif

    if (pshapefile == NULL)
        return 0;

    shapefile_id = php3_list_insert(pshapefile, PHPMS_GLOBAL(le_msshapefile));

    _phpms_object_init(return_value, shapefile_id, 
                       php_shapefile_class_functions,
                       PHP4_CLASS_ENTRY(shapefile_class_entry_ptr));

    /* read-only properties */
    add_property_long(return_value, "numshapes",  pshapefile->numshapes);
    add_property_long(return_value, "type",       pshapefile->numshapes);
    PHPMS_ADD_PROP_STR(return_value,"source",     pshapefile->source);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_rect_object(&(pshapefile->bounds), 
                             PHPMS_GLOBAL(le_msrect_ref), list, new_obj_ptr);
    _phpms_add_property_object(return_value, "bounds", new_obj_ptr,E_ERROR);

    return shapefile_id;
}

/**********************************************************************
 *                        ms_newShapefileObj()
 **********************************************************************/

/* {{{ proto shapefileObj ms_newShapefileObj(string filename, int type)
   Opens a shapefile and returns a new object to deal with it. Filename should be passed with no extension. */

DLEXPORT void php3_ms_shapefile_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pFname, *pType;
    shapefileObj *pNewObj = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (getParameters(ht, 2, &pFname, &pType) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    /* Attempt to open the file 
     */
    convert_to_string(pFname);
    convert_to_long(pType);
    pNewObj = shapefileObj_new(pFname->value.str.val, pType->value.lval);
    if (pNewObj == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed to open shapefile %s", 
                            pFname->value.str.val);
        RETURN_FALSE;
    }

    /* Create a PHP object, add all shapefileObj methods, etc.
     */
    _phpms_build_shapefile_object(pNewObj, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        shapefile->addShape()
 **********************************************************************/

/* {{{ proto int shapefile.addShape(shapeObj shape)
   Appends a shape to an open shapefile. */

DLEXPORT void php3_ms_shapefile_addshape(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pShape;
    shapefileObj *self;
    shapeObj    *poShape;
    int         nRetVal=0;
    
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    poShape = (shapeObj*)_phpms_fetch_handle2(pShape,
                                              PHPMS_GLOBAL(le_msshape_ref),
                                              PHPMS_GLOBAL(le_msshape_new),
                                              list);

    if (self && poShape)
        nRetVal = shapefileObj_add(self, poShape);

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        shapefile->addPoint()
 **********************************************************************/

/* {{{ proto int shapefile.addPoint(pointObj point)
   Appends a point to a poin layer. */

DLEXPORT void php3_ms_shapefile_addpoint(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pPoint;
    shapefileObj *self;
    pointObj    *poPoint;
    int         nRetVal=0;
    
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pPoint) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    poPoint = (pointObj*)_phpms_fetch_handle2(pPoint,
                                              PHPMS_GLOBAL(le_mspoint_ref),
                                              PHPMS_GLOBAL(le_mspoint_new),
                                              list);

    if (self && poPoint)
        nRetVal = shapefileObj_addPoint(self, poPoint);

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        shapefile->getShape()
 **********************************************************************/

/* {{{ proto int shapefile.getShape(int i)
   Retrieve shape by index. */

DLEXPORT void php3_ms_shapefile_getshape(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    shapefileObj *self;
    shapeObj    *poShape;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    /* Create a new shapeObj to hold the result 
     * Note that the type used to create the shape (MS_NULL) does not matter
     * at this point since it will be set by SHPReadShape().
     */
    if ((poShape = shapeObj_new(MS_SHAPE_NULL)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new shape (out of memory?)");
        RETURN_FALSE;
    }

    /* Read from the file */
    if (self == NULL || 
        shapefileObj_get(self, pIndex->value.lval, poShape) != 0)
    {
        shapeObj_destroy(poShape);
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed reading shape %d.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value);
}
/* }}} */


/**********************************************************************
 *                        shapefile->getShape()
 **********************************************************************/

/* {{{ proto int shapefile.getPoint(int i)
   Retrieve a point by index. */

DLEXPORT void php3_ms_shapefile_getpoint(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    shapefileObj *self;
    pointObj    *poPoint;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    /* Create a new PointObj to hold the result */
    if ((poPoint = pointObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new point (out of memory?)");
        RETURN_FALSE;
    }

    /* Read from the file */
    if (self == NULL || 
        shapefileObj_getPoint(self, pIndex->value.lval, poPoint) != 0)
    {
        pointObj_destroy(poPoint);
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed reading point %d.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return point object */
    _phpms_build_point_object(poPoint, PHPMS_GLOBAL(le_mspoint_new),
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shapefile->gettransformed()
 **********************************************************************/

/* {{{ proto int shapefile.getTransformed(mapObj map, int i)
   Retrieve shape by index. */

DLEXPORT void php3_ms_shapefile_gettransformed(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex, *pMap;
    shapefileObj *self;
    shapeObj    *poShape;
    mapObj      *poMap;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pMap, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);

    /* Create a new shapeObj to hold the result 
     * Note that the type used to create the shape (MS_NULL) does not matter
     * at this point since it will be set by SHPReadShape().
     */
    if ((poShape = shapeObj_new(MS_SHAPE_NULL)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new shape (out of memory?)");
        RETURN_FALSE;
    }

    /* Read from the file */
    if (self == NULL || 
        shapefileObj_getTransformed(self, poMap, 
                                    pIndex->value.lval, poShape) != 0)
    {
        shapeObj_destroy(poShape);
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed reading shape %d.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shapefile->getExtent()
 **********************************************************************/

/* {{{ proto int shapefile.getExtent(int i)
   Retrieve a shape's bounding box by index. */

DLEXPORT void php3_ms_shapefile_getextent(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    shapefileObj *self;
    rectObj      *poRect;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    if (self == NULL)
    {
        RETURN_FALSE;
    }

    /* Create a new rectObj to hold the result */
    if ((poRect = rectObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new rectObj (out of memory?)");
        RETURN_FALSE;
    }

    /* Read from the file 
     * shapefileObj_getExtent() has no return value!  How do we catch errors?
     */
    shapefileObj_getExtent(self, pIndex->value.lval, poRect);

    /* Return rectObj */
    _phpms_build_rect_object(poRect, PHPMS_GLOBAL(le_msrect_new), 
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shapeFileObj->free()
 **********************************************************************/

/* {{{ proto int shapefile.free()
   Destroys resources used by a shapeFileObj object */

DLEXPORT void php3_ms_shapefile_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    shapefileObj *self;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapefileObj*)_phpms_fetch_handle(pThis, le_msshapefile, list);
    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
#ifdef PHP4
        pval **phandle;
        if (zend_hash_find(pThis->value.obj.properties, "_handle_", 
                           sizeof("_handle_"), 
                           (void **)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
#else
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)

        {
            php3_list_delete(phandle->value.lval);
        }
#endif
    }
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - resultCacheMemberObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_resultCacheMember_object()
 **********************************************************************/
static long _phpms_build_resultcachemember_object(resultCacheMemberObj *pRes,
                                                  HashTable *list, 
                                                  pval *return_value)
{
    if (pRes == NULL)
        return 0;

    object_init(return_value);

    /* Note: Contrary to most other object classes, this one does not
     *       need to keep a handle on the internal structure since all
     *       members are read-only and thus there is no set() method.
     */

    /* read-only properties */
    add_property_long(return_value,   "shapeindex", pRes->shapeindex);
    add_property_long(return_value,   "tileindex",  pRes->tileindex);
    add_property_long(return_value,   "classindex", pRes->classindex);

    return 0;
}


/*=====================================================================
 *                 PHP function wrappers - projection class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_projection_object()
**********************************************************************/
static long _phpms_build_projection_object(projectionObj *pproj, 
                                           int handle_type, 
                                           HashTable *list, pval *return_value)
{
    int projection_id;

    if (pproj == NULL)
      return 0;

    projection_id = php3_list_insert(pproj, handle_type);

    _phpms_object_init(return_value, projection_id, 
                       php_projection_class_functions,
                       PHP4_CLASS_ENTRY(projection_class_entry_ptr));

    return projection_id;
}
 
/**********************************************************************
 *                        ms_newProjectionObj()
 **********************************************************************/

/* {{{ proto projectionObj ms_newProjectionObj()
   Create a new projectionObj instance. */

DLEXPORT void php3_ms_projection_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pString;
    projectionObj *pNewProj = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (getParameters(ht, 1, &pString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pString);

    if ((pNewProj = projectionObj_new(pString->value.str.val)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return rect object */
    _phpms_build_projection_object(pNewProj, 
                                   PHPMS_GLOBAL(le_msprojection_new), 
                                   list, return_value);
}
/* }}} */

/**********************************************************************
 *                        projection->free()
 **********************************************************************/

/* {{{ proto int projection.free()
   Destroys resources used by a projection object */
DLEXPORT void php3_ms_projection_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    projectionObj *self;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (projectionObj *)_phpms_fetch_handle(pThis, 
                                                le_msprojection_new, list);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
#ifdef PHP4
        pval **phandle;
        if (zend_hash_find(pThis->value.obj.properties, "_handle_", 
                           sizeof("_handle_"), 
                           (void **)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
#else
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)

        {
            php3_list_delete(phandle->value.lval);
        }
#endif
    }

}
/* }}} */



/* ==================================================================== */
/*      utility functions                                               */
/* ==================================================================== */


/************************************************************************/
/*      DLEXPORT void php3_ms_getcwd(INTERNAL_FUNCTION_PARAMETERS)      */
/*                                                                      */
/*      This function is a copy of the                                  */
/*      vod php3_posix_getcwd(INTERNAL_FUNCTION_PARAMETERS)             */
/*      (./php/functions/posix.c).                                      */
/*                                                                      */
/*       Since posix functions seems to not be standard in php, It has been*/
/*      added here.                                                     */
/************************************************************************/
/* OS/2's gcc defines _POSIX_PATH_MAX but not PATH_MAX */
#if !defined(PATH_MAX) && defined(_POSIX_PATH_MAX)
#define PATH_MAX _POSIX_PATH_MAX
#endif

#if !defined(PATH_MAX) && defined(MAX_PATH)
#define PATH_MAX MAX_PATH
#endif

#if !defined(PATH_MAX)
#define PATH_MAX 512
#endif

DLEXPORT void php3_ms_getcwd(INTERNAL_FUNCTION_PARAMETERS)
{
    char  buffer[PATH_MAX];
    char *p;
   
    p = getcwd(buffer, PATH_MAX);
    if (!p) {
        php3_error(E_WARNING, "posix_getcwd() failed with '%s'",
                   strerror(errno));
        RETURN_FALSE;
    }

    RETURN_STRING(buffer, 1);
}

/************************************************************************/
/*                           php3_ms_getpid()                           */
/************************************************************************/
DLEXPORT void php3_ms_getpid(INTERNAL_FUNCTION_PARAMETERS)
{
    RETURN_LONG(getpid());
}


/************************************************************************/
/*       DLEXPORT void php3_ms_getscale(INTERNAL_FUNCTION_PARAMETERS)   */
/*                                                                      */
/*      Utility function to get the scale based on the width/height     */
/*      of the pixmap, the georeference and the units of the data.      */
/*                                                                      */
/*       Parameters are :                                               */
/*                                                                      */
/*            - Georefernce extents (rectObj)                           */
/*            - Width : width in pixel                                  */
/*            - Height : Height in pixel                                */
/*            - Units of the data (int)                                 */
/*                                                                      */
/*                                                                      */
/************************************************************************/
DLEXPORT void php3_ms_getscale(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pGeorefExt = NULL;
    pval        *pWidth = NULL;
    pval        *pHeight;
    pval        *pUnit, *pResolution;
    rectObj     *poGeorefExt = NULL;
    double      dfScale = 0.0;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    if (getParameters(ht, 5, 
                      &pGeorefExt, &pWidth, &pHeight, 
                      &pUnit, &pResolution) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }
    
    convert_to_long(pWidth);
    convert_to_long(pHeight);
    convert_to_long(pUnit);
    convert_to_long(pResolution);

    poGeorefExt = 
        (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                        PHPMS_GLOBAL(le_msrect_ref),
                                        PHPMS_GLOBAL(le_msrect_new),
                                        list);

    if (msCalculateScale(*poGeorefExt, pUnit->value.lval, 
                         pWidth->value.lval, pHeight->value.lval,
                         pResolution->value.lval, &dfScale) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }
    
    RETURN_DOUBLE(dfScale);
}

/************************************************************************/
/*  static double GetDeltaExtentsUsingScale(double dfMinscale, int nUnits,*/
/*                                              int nWidth)             */
/*                                                                      */
/*      Utility function to return the maximum extent using the         */
/*      scale and the width of the image.                               */
/*                                                                      */
/*      Base on the function msCalculateScale (mapscale.c)              */
/************************************************************************/
static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};
static double GetDeltaExtentsUsingScale(double dfScale, int nUnits, 
                                        int nWidth, int resolution)
{
    double md = 0.0;
    double dfDelta = -1.0;

    if (dfScale <= 0 || nWidth <=0)
      return -1;

    switch (nUnits) 
    {
      case(MS_DD):
      case(MS_METERS):    
      case(MS_KILOMETERS):
      case(MS_MILES):
      case(MS_INCHES):  
      case(MS_FEET):
        md = nWidth/(resolution*inchesPerUnit[nUnits]);
        dfDelta = md * dfScale;
        break;
          
      default:
        break;
    }

    return dfDelta;
}

