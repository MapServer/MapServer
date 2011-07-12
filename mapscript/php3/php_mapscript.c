/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PHP/MapScript extension for MapServer.  External interface 
 *           functions
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
 
#include "php_mapscript.h"
#include "php_mapscript_util.h"

#include "php.h"
#include "php_globals.h"
#include "SAPI.h"
#include "ext/standard/info.h"
#include "ext/standard/head.h"
#include "main/php_output.h" 

#include "maperror.h"

#include <time.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#else
#include <errno.h>
#endif

/* All gd functions used in this extension (if they are renamed in the
   "main/php_compat.h" file of php source) should be added here too for
   compatibility reasons: when php compiles gd as a shared extention */
#if defined(HAVE_GD_BUNDLED)
#undef gdImageColorExact                                                                    
#undef gdImageColorTransparent
#undef gdImageCopy
#endif

#define PHPMS_VERSION "($Revision$ $Date$)"

#ifndef DLEXPORT 
#define DLEXPORT ZEND_DLEXPORT
#endif

//#if defined(_WIN32) && !defined(__CYGWIN__)
//void ***tsrm_ls;
//#endif

/*=====================================================================
 *                         Prototypes
 *====================================================================*/

PHP_MINFO_FUNCTION(mapscript);
PHP_MINIT_FUNCTION(phpms);
PHP_MSHUTDOWN_FUNCTION(phpms);
PHP_RINIT_FUNCTION(phpms);
PHP_RSHUTDOWN_FUNCTION(phpms);


static void php_ms_free_map(zend_rsrc_list_entry *rsrc TSRMLS_DC);

static void php_ms_free_image(zend_rsrc_list_entry *rsrc TSRMLS_DC);
DLEXPORT void php3_ms_free_layer(layerObj *pLayer);
DLEXPORT void php3_ms_free_point(pointObj *pPoint);
DLEXPORT void php3_ms_free_line(lineObj *pLine);
DLEXPORT void php3_ms_free_shape(shapeObj *pShape);
DLEXPORT void php3_ms_free_shapefile(shapefileObj *pShapefile);
DLEXPORT void php3_ms_free_rect(rectObj *pRect);
DLEXPORT void php3_ms_free_stub(void *ptr) ;
DLEXPORT void php3_ms_free_projection(projectionObj *pProj);
DLEXPORT void php_ms_free_cgirequest(cgiRequestObj *request);


DLEXPORT void php3_ms_getversion(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_getversionint(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_tokenizeMap(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_new_from_string(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_clone(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setProjection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getProjection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setWKTProjection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getSymbolByName(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getSymbolObjectById(INTERNAL_FUNCTION_PARAMETERS);
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
DLEXPORT void php3_ms_map_queryByIndex(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_savequery(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_loadquery(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_freequery(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_save(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_removeMetaData(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_setExtent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setRotation(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setSize(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomRectangle(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomScale(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_getLatLongExtent(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_moveLayerUp(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_moveLayerDown(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getLayersDrawingOrder(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setLayersDrawingOrder(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_processTemplate(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_processLegendTemplate(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_processQueryTemplate(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_setSymbolSet(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getNumSymbols(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setFontSet(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_saveMapContext(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_loadMapContext(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void  php3_ms_map_selectOutputFormat(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void  php3_ms_map_applySLD(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void  php3_ms_map_applySLDURL(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void  php3_ms_map_generateSLD(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_getConfigOption(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setConfigOption(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_applyConfigOptions(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_loadOWSParameters(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_OWSDispatch(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_insertLayer(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_removeLayer(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_setcenter(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_offsetextent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_scaleextent(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_img_saveImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_img_saveWebImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_img_pasteImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_img_free(INTERNAL_FUNCTION_PARAMETERS);


DLEXPORT void php3_ms_lyr_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
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
DLEXPORT void php3_ms_lyr_getProjection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setWKTProjection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_addFeature(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getNumResults(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getResult(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_open(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_whichShapes(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_nextShape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_close(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getShape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getFeature(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_resultsGetShape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getExtent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_removeMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setFilter(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getFilterString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getWMSFeatureInfoURL(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getItems(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setProcessing(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getProcessing(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_clearProcessing(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_executeWFSGetfeature(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_applySLD(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_applySLDURL(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_generateSLD(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_moveClassUp(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_moveClassDown(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_removeClass(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_isVisible(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setConnectionType(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getGridIntersectionCoordinates(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_class_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setExpression(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_getExpressionString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setText(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_getTextString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_drawLegendIcon(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_createLegendIcon(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_getStyle(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_clone(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_moveStyleUp(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_moveStyleDown(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_deleteStyle(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_class_getMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setMetaData(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_removeMetaData(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_label_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_label_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_label_setBinding(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_label_getBinding(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_label_removeBinding(INTERNAL_FUNCTION_PARAMETERS);

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
DLEXPORT void php3_ms_point_setXYZ(INTERNAL_FUNCTION_PARAMETERS);
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
DLEXPORT void php3_ms_line_addXYZ(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_point(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_shape_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_fromwkt(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_project(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_add(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_line(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_contains(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_intersects(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_getvalue(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_getpointusingmeasure(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_getmeasureusingpoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_setbounds(INTERNAL_FUNCTION_PARAMETERS);

/*geos related functions*/
DLEXPORT void php3_ms_shape_buffer(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_topologypreservingsimplify(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_simplify(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_convexhull(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_boundary(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_Union(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_intersection(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_difference(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_symdifference(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_contains_geos(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_overlaps(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_within(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_crosses(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_touches(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_equals(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_disjoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_getcentroid(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_getarea(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_getlength(INTERNAL_FUNCTION_PARAMETERS);
/*end of geos related functions*/
DLEXPORT void php3_ms_shape_getlabelpoint(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_shape_towkt(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_shapefile_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_addshape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_addpoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getshape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getpoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_gettransformed(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getextent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_web_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_web_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_referenceMap_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_referenceMap_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_projection_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_projection_getunits(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_projection_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_scalebar_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_scalebar_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_scalebar_setImageColor(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_legend_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_legend_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_style_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_style_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_style_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_style_clone(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_style_setBinding(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_style_getBinding(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_style_removeBinding(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_grid_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_grid_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_get_error_obj(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_error_next(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_reset_error_list(INTERNAL_FUNCTION_PARAMETERS);


//DLEXPORT void php_ms_outputformat_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_outputformat_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_outputformat_setOption(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_outputformat_getOption(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_outputformat_validate(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_symbol_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_symbol_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_symbol_setPoints(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_symbol_getPoints(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_symbol_setPattern(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_symbol_getPattern(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_symbol_setImagepath(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_querymap_updateFromString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_querymap_setProperty(INTERNAL_FUNCTION_PARAMETERS);


DLEXPORT void php_ms_cgirequest_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_cgirequest_loadParams(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_cgirequest_setParameter(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_cgirequest_getName(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_cgirequest_getValue(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_cgirequest_getValueByName(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_hashtable_get(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_hashtable_set(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_hashtable_remove(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_hashtable_clear(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_hashtable_nextkey(INTERNAL_FUNCTION_PARAMETERS);

static long _phpms_build_img_object(imageObj *im, webObj *pweb,
                                    HashTable *list, pval *return_value  TSRMLS_DC);
static long _phpms_build_layer_object(layerObj *player, int parent_map_id,
                                      HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_class_object(classObj *pclass, int parent_map_id, 
                                      int parent_layer_id, HashTable *list, 
                                      pval *return_value TSRMLS_DC);
static long _phpms_build_label_object(labelObj *plabel,
                                      HashTable *list, pval *return_value TSRMLS_DC);

static long _phpms_build_color_object(colorObj *pcolor,
                                      HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_point_object(pointObj *ppoint, int handle_type,
                                      HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_shape_object(shapeObj *pshape, int handle_type,
                                      layerObj *pLayer,
                                      HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_web_object(webObj *pweb,
                                    HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_rect_object(rectObj *prect, int handle_type,
                                     HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_referenceMap_object(referenceMapObj *preferenceMap,
                                          HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_resultcachemember_object(resultCacheMemberObj *pRes,
                                                  HashTable *list TSRMLS_DC, 
                                                  pval *return_value);

static long _phpms_build_projection_object(projectionObj *pproj, 
                                           int handle_type, HashTable *list, 
                                           pval *return_value TSRMLS_DC);
static long _phpms_build_scalebar_object(scalebarObj *pscalebar,
                                         HashTable *list, pval *return_value TSRMLS_DC);
static long _phpms_build_legend_object(legendObj *plegend,
                                       HashTable *list, pval *return_value TSRMLS_DC);

static long _phpms_build_style_object(styleObj *pstyle, int parent_map_id, 
                                      int parent_layer_id, 
                                      int parent_class_id,
                                      HashTable *list, 
                                      pval *return_value TSRMLS_DC);

static long _phpms_build_outputformat_object(outputFormatObj *poutputformat,
                                             HashTable *list, 
                                             pval *return_value TSRMLS_DC);

static long _phpms_build_grid_object(graticuleObj *pgrid, 
                                     int parent_layer_id,
                                     HashTable *list, 
                                     pval *return_value TSRMLS_DC);

static long _phpms_build_labelcache_object(labelCacheObj *plabelcache,
                                           HashTable *list, 
                                           pval *return_value TSRMLS_DC);

static long _phpms_build_symbol_object(symbolObj *psSymbol, 
                                       int parent_map_id, 
                                       HashTable *list, 
                                       pval *return_value TSRMLS_DC);

DLEXPORT void php_ms_labelcache_free(INTERNAL_FUNCTION_PARAMETERS);

static long _phpms_build_querymap_object(queryMapObj *pquerymap,
                                      HashTable *list, pval *return_value TSRMLS_DC);

static long _phpms_build_hashtable_object(hashTableObj *hashtable,
                                          HashTable *list, pval *return_value TSRMLS_DC);


/* ==================================================================== */
/*      utility functions prototypes.                                   */
/* ==================================================================== */
DLEXPORT void php3_ms_getcwd(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_getpid(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_getscale(INTERNAL_FUNCTION_PARAMETERS);


/* ==================================================================== */
/*      msio functions prototypes.                                      */
/* ==================================================================== */
DLEXPORT void php_ms_IO_installStdoutToBuffer(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_IO_installStdinFromBuffer(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_IO_getStdoutBufferString(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_IO_resetHandlers(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_IO_stripStdoutBufferContentType(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_ms_IO_getStdoutBufferBytes(INTERNAL_FUNCTION_PARAMETERS);


/*=====================================================================
 *               PHP Dynamically Loadable Library stuff
 *====================================================================*/

/* True global resources - no need for thread safety here */
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
static int le_msprojection_ref;
static int le_msscalebar;
static int le_mslegend;
static int le_msstyle;
static int le_msoutputformat;
static int le_msgrid;
static int le_mserror_ref;
static int le_mslabelcache;
static int le_mssymbol;
static int le_msquerymap;
static int le_mscgirequest;
static int le_mshashtable;

/* 
 * Declare any global variables you may need between the BEGIN
 * and END macros here after uncommenting the following lines and the 
 * ZEND_INIT_MODULE_GLOBALS() call in PHP_MINIT_FUNCTION() )
 */

/*
ZEND_BEGIN_MODULE_GLOBALS(phpms)
    int   global_value;
    char *global_string;
ZEND_END_MODULE_GLOBALS(phpms)

ZEND_DECLARE_MODULE_GLOBALS(phpms)

static void phpms_init_globals(zend_phpms_globals *phpms_globals)
{
    phpms_globals->global_value = 0;
    phpms_globals->global_string = NULL;
}
*/


/* In every utility function you add that needs to use variables 
   in phpms_globals, call TSRM_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as PHPMS_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define PHPMS_G(v) TSRMG(phpms_globals_id, zend_phpms_globals *, v)
#else
#define PHPMS_G(v) (phpms_globals.v)
#endif


/* -------------------------------------------------------------------- */
/*      class entries.                                                  */
/* -------------------------------------------------------------------- */
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
static zend_class_entry *scalebar_class_entry_ptr;
static zend_class_entry *legend_class_entry_ptr;
static zend_class_entry *style_class_entry_ptr;
static zend_class_entry *outputformat_class_entry_ptr;
static zend_class_entry *grid_class_entry_ptr;
static zend_class_entry *error_class_entry_ptr;
static zend_class_entry *labelcache_class_entry_ptr;
static zend_class_entry *symbol_class_entry_ptr;
static zend_class_entry *querymap_class_entry_ptr;
static zend_class_entry *cgirequest_class_entry_ptr;
static zend_class_entry *hashtable_class_entry_ptr;

#ifdef ZEND_ENGINE_2  // PHP5
ZEND_BEGIN_ARG_INFO(one_arg_force_ref, 0)
    ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(two_args_first_arg_force_ref, 0)
    ZEND_ARG_PASS_INFO(1)
    ZEND_ARG_PASS_INFO(0)
ZEND_END_ARG_INFO()
#else   // PHP4
static unsigned char one_arg_force_ref[] = 
  { 1, BYREF_FORCE};
static unsigned char two_args_first_arg_force_ref[] = 
    { 2, BYREF_FORCE, BYREF_NONE };
#endif

function_entry phpms_functions[] = {
    {"ms_getversion",   php3_ms_getversion,     NULL},
    {"ms_getversionint",php3_ms_getversionint,  NULL},
    {"ms_newmapobj",    php3_ms_map_new,        NULL},
    {"ms_newmapobjfromstring",    php3_ms_map_new_from_string,        NULL},
    {"ms_newlayerobj",  php3_ms_lyr_new,        two_args_first_arg_force_ref},
    {"ms_newclassobj",  php3_ms_class_new,      one_arg_force_ref},
    {"ms_newpointobj",  php3_ms_point_new,      NULL},
    {"ms_newlineobj",   php3_ms_line_new,       NULL},
    {"ms_newshapeobj",  php3_ms_shape_new,      NULL},
    {"ms_shapeobjfromwkt", php3_ms_shape_fromwkt,  NULL},
    {"ms_newshapefileobj", php3_ms_shapefile_new,  NULL},
    {"ms_newrectobj",   php3_ms_rect_new,       NULL},
    {"ms_getcwd",       php3_ms_getcwd,         NULL},
    {"ms_getpid",       php3_ms_getpid,         NULL},
    {"ms_getscale",     php3_ms_getscale,       NULL},
    {"ms_newprojectionobj", php3_ms_projection_new, NULL},
    {"ms_tokenizemap",  php3_ms_tokenizeMap,    NULL},
    {"ms_newstyleobj",  php3_ms_style_new,      one_arg_force_ref},
    {"ms_newgridobj",   php3_ms_grid_new,       one_arg_force_ref},
    {"ms_geterrorobj",  php3_ms_get_error_obj,  NULL},
    {"ms_reseterrorlist", php3_ms_reset_error_list, NULL},
    {"ms_newsymbolobj", php3_ms_symbol_new, NULL},
    {"ms_newowsrequestobj",    php_ms_cgirequest_new,        NULL},
    {"ms_ioinstallstdouttobuffer",    php_ms_IO_installStdoutToBuffer,   NULL},
    {"ms_ioinstallstdinfrombuffer",    php_ms_IO_installStdinFromBuffer,   NULL},
    {"ms_iogetstdoutbufferstring",    php_ms_IO_getStdoutBufferString,   NULL},
    {"ms_ioresethandlers",    php_ms_IO_resetHandlers,   NULL},
    {"ms_iostripstdoutbuffercontenttype",    php_ms_IO_stripStdoutBufferContentType,  NULL},
    {"ms_iogetstdoutbufferbytes",    php_ms_IO_getStdoutBufferBytes,  NULL},
    {NULL, NULL, NULL}
};

zend_module_entry php_ms_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "MapScript", 
    phpms_functions,
    /* MINIT()/MSHUTDOWN() are called once only when PHP starts up and 
     * shutdowns.  They're really called only once and *not* when a new Apache
     * child process is created.
     */
    PHP_MINIT(phpms),
    PHP_MSHUTDOWN(phpms),
    /* RINIT()/RSHUTDOWN() are called once per request 
     * We shouldn't really be using them since they are run on every request
     * and can hit performance.
     */
    NULL /* PHP_RINIT(phpms)     */,
    NULL /* PHP_RSHUTDOWN(phpms) */,
    PHP_MINFO(mapscript),
#if ZEND_MODULE_API_NO >= 20010901
    PHPMS_VERSION,          /* extension version number (string) */
#endif
    STANDARD_MODULE_PROPERTIES 
};


#if COMPILE_DL
DLEXPORT zend_module_entry *get_module(void) { return &php_ms_module_entry; }
#endif

/* -------------------------------------------------------------------- */
/*      Class method definitions.                                       */
/*      We use those to initialize objects in both PHP3 and PHP4        */
/*      through _phpms_object_init()                                    */
/* -------------------------------------------------------------------- */
function_entry php_map_class_functions[] = {
    {"clone",           php3_ms_map_clone,              NULL},
    {"set",             php3_ms_map_setProperty,        NULL},
    {"setprojection",   php3_ms_map_setProjection,      NULL},
    {"getprojection",   php3_ms_map_getProjection,      NULL},
    {"setwktprojection",php3_ms_map_setWKTProjection,   NULL},
    {"getsymbolbyname", php3_ms_map_getSymbolByName,    NULL},
    {"getsymbolobjectbyid", php3_ms_map_getSymbolObjectById,    NULL},
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
    {"getalllayernames",php3_ms_map_getAllLayerNames,   NULL},
    {"getallgroupnames",php3_ms_map_getAllGroupNames,   NULL},
    {"getcolorbyindex", php3_ms_map_getColorByIndex,    NULL},
    {"setextent",       php3_ms_map_setExtent,          NULL},
    {"setrotation",     php3_ms_map_setRotation,        NULL},
    {"setsize",         php3_ms_map_setSize,            NULL},
    {"zoompoint",       php3_ms_map_zoomPoint,          NULL},
    {"zoomrectangle",   php3_ms_map_zoomRectangle,      NULL},
    {"zoomscale",       php3_ms_map_zoomScale,          NULL},
    {"querybypoint",    php3_ms_map_queryByPoint,       NULL},
    {"querybyrect",     php3_ms_map_queryByRect,        NULL},
    {"querybyfeatures", php3_ms_map_queryByFeatures,    NULL},
    {"querybyshape",    php3_ms_map_queryByShape,       NULL},
    {"querybyindex",    php3_ms_map_queryByIndex,       NULL},
    {"savequery",       php3_ms_map_savequery,          NULL},
    {"loadquery",       php3_ms_map_loadquery,          NULL},
    {"freequery",       php3_ms_map_freequery,          NULL},
    {"save",            php3_ms_map_save,               NULL},
    {"getlatlongextent", php3_ms_map_getLatLongExtent,  NULL},
    {"getmetadata",     php3_ms_map_getMetaData,        NULL},
    {"setmetadata",     php3_ms_map_setMetaData,        NULL},
    {"removemetadata",  php3_ms_map_removeMetaData,     NULL},
    {"prepareimage",    php3_ms_map_prepareImage,       NULL},
    {"preparequery",    php3_ms_map_prepareQuery,       NULL},
    {"movelayerup",     php3_ms_map_moveLayerUp,        NULL},
    {"movelayerdown",   php3_ms_map_moveLayerDown,      NULL},
    {"getlayersdrawingorder", php3_ms_map_getLayersDrawingOrder, NULL},
    {"setlayersdrawingorder", php3_ms_map_setLayersDrawingOrder, NULL},
    {"processtemplate", php3_ms_map_processTemplate,    NULL},
    {"processlegendtemplate", php3_ms_map_processLegendTemplate, NULL},
    {"processquerytemplate",  php3_ms_map_processQueryTemplate,  NULL},
    {"setsymbolset",    php3_ms_map_setSymbolSet,       NULL},
    {"getnumsymbols",   php3_ms_map_getNumSymbols,      NULL},
    {"setfontset",      php3_ms_map_setFontSet,         NULL},
    {"savemapcontext",  php3_ms_map_saveMapContext,     NULL},
    {"loadmapcontext",  php3_ms_map_loadMapContext,     NULL},
    {"selectoutputformat", php3_ms_map_selectOutputFormat,      NULL},
    {"applysld",        php3_ms_map_applySLD,           NULL},
    {"applysldurl",     php3_ms_map_applySLDURL,        NULL},
    {"generatesld",     php3_ms_map_generateSLD,        NULL},
    {"getconfigoption", php3_ms_map_getConfigOption,    NULL},
    {"setconfigoption", php3_ms_map_setConfigOption,    NULL},
    {"applyconfigoptions", php3_ms_map_applyConfigOptions,      NULL},
    {"loadowsparameters",  php3_ms_map_loadOWSParameters,       NULL},
    {"owsdispatch",     php3_ms_map_OWSDispatch,        NULL},
    {"insertlayer",     php3_ms_map_insertLayer,        NULL},
    {"removelayer",     php3_ms_map_removeLayer,        NULL},
    {"setcenter",       php3_ms_map_setcenter,          NULL},
    {"offsetextent",    php3_ms_map_offsetextent,       NULL},
    {"scaleextent",     php3_ms_map_scaleextent,       NULL},
    {NULL, NULL, NULL}
};

function_entry php_img_class_functions[] = {
    {"saveimage",       php3_ms_img_saveImage,          NULL},
    {"savewebimage",    php3_ms_img_saveWebImage,       NULL},
    {"pasteimage",      php3_ms_img_pasteImage,         NULL},
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
    {"updatefromstring", php3_ms_web_updateFromString,NULL },
    {"set",             php3_ms_web_setProperty,        NULL},    
    {NULL, NULL, NULL}
};

function_entry php_reference_class_functions[] = {
    {"updatefromstring", php3_ms_referenceMap_updateFromString,NULL },
    {"set",             php3_ms_referenceMap_setProperty,NULL},    
    {NULL, NULL, NULL}
};

function_entry php_scalebar_class_functions[] = {
    {"updatefromstring", php3_ms_scalebar_updateFromString,NULL },
    {"set",             php3_ms_scalebar_setProperty,   NULL},
    {"setimagecolor",   php3_ms_scalebar_setImageColor, NULL},    
    {NULL, NULL, NULL}
};

function_entry php_legend_class_functions[] = {
    {"updatefromstring",  php3_ms_legend_updateFromString,     NULL},
    {"set",             php3_ms_legend_setProperty,     NULL},    
    {NULL, NULL, NULL}
};

function_entry php_layer_class_functions[] = {
    {"updatefromstring",  php3_ms_lyr_updateFromString,     NULL},
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
    {"getprojection",   php3_ms_lyr_getProjection,      NULL},
    {"setwktprojection",php3_ms_lyr_setWKTProjection,   NULL},
    {"setconnectiontype",php3_ms_lyr_setConnectionType, NULL},
    {"addfeature",      php3_ms_lyr_addFeature,         NULL},
    {"getnumresults",   php3_ms_lyr_getNumResults,      NULL},
    {"getresult",       php3_ms_lyr_getResult,          NULL},
    {"open",            php3_ms_lyr_open,               NULL},
    {"whichshapes",     php3_ms_lyr_whichShapes,        NULL},
    {"nextshape",       php3_ms_lyr_nextShape,          NULL},
    {"close",           php3_ms_lyr_close,              NULL},
    {"getshape",        php3_ms_lyr_getShape,           NULL},
    {"getfeature",      php3_ms_lyr_getFeature,         NULL},
    {"resultsgetshape", php3_ms_lyr_resultsGetShape,    NULL},
    {"getextent",       php3_ms_lyr_getExtent,          NULL},
    {"getmetadata",     php3_ms_lyr_getMetaData,        NULL},
    {"setmetadata",     php3_ms_lyr_setMetaData,        NULL},
    {"removemetadata",  php3_ms_lyr_removeMetaData,     NULL},
    {"setfilter",       php3_ms_lyr_setFilter,          NULL},
    {"getfilterstring", php3_ms_lyr_getFilterString,    NULL},
    {"getfilter",       php3_ms_lyr_getFilterString,    NULL},
    {"getwmsfeatureinfourl", php3_ms_lyr_getWMSFeatureInfoURL, NULL},
    {"getitems",        php3_ms_lyr_getItems,           NULL},
    {"setprocessing",   php3_ms_lyr_setProcessing,      NULL},
    {"getprocessing",   php3_ms_lyr_getProcessing,      NULL},
    {"clearprocessing", php3_ms_lyr_clearProcessing,    NULL},
    {"executewfsgetfeature", php3_ms_lyr_executeWFSGetfeature, NULL},
    {"applysld",        php3_ms_lyr_applySLD,           NULL},
    {"applysldurl",     php3_ms_lyr_applySLDURL,        NULL},
    {"generatesld",     php3_ms_lyr_generateSLD,        NULL},
    {"moveclassup",     php3_ms_lyr_moveClassUp,        NULL},
    {"moveclassdown",   php3_ms_lyr_moveClassDown,      NULL},
    {"removeclass",     php3_ms_lyr_removeClass,        NULL},
    {"isvisible",       php3_ms_lyr_isVisible,          NULL},
    {"getgridintersectioncoordinates",       php3_ms_lyr_getGridIntersectionCoordinates,          NULL},
    {NULL, NULL, NULL}
};

function_entry php_label_class_functions[] = {
    {"updatefromstring",  php3_ms_label_updateFromString,     NULL},
    {"set",             php3_ms_label_setProperty,      NULL},    
    {"setbinding",      php3_ms_label_setBinding,      NULL},    
    {"getbinding",      php3_ms_label_getBinding,      NULL},    
    {"removebinding",   php3_ms_label_removeBinding,      NULL},    
    {NULL, NULL, NULL}
};

function_entry php_class_class_functions[] = {
    {"updatefromstring",  php3_ms_class_updateFromString,     NULL},
    {"set",             php3_ms_class_setProperty,      NULL},    
    {"setexpression",   php3_ms_class_setExpression,    NULL},    
    {"getexpressionstring", php3_ms_class_getExpressionString, NULL},    
    {"getexpression",       php3_ms_class_getExpressionString, NULL},    
    {"settext",         php3_ms_class_setText,          NULL},
    {"gettextstring",   php3_ms_class_getTextString,    NULL},    
    {"drawlegendicon",  php3_ms_class_drawLegendIcon,   NULL},
    {"createlegendicon",php3_ms_class_createLegendIcon, NULL},   
    {"getstyle",        php3_ms_class_getStyle,         NULL},   
    {"clone",           php3_ms_class_clone,            NULL},   
    {"movestyleup",     php3_ms_class_moveStyleUp,      NULL},   
    {"movestyledown",   php3_ms_class_moveStyleDown,    NULL},   
    {"deletestyle",     php3_ms_class_deleteStyle,      NULL},   
    {"getmetadata",     php3_ms_class_getMetaData,        NULL},
    {"setmetadata",     php3_ms_class_setMetaData,        NULL},
    {"removemetadata",  php3_ms_class_removeMetaData,     NULL},
    {NULL, NULL, NULL}
};

function_entry php_point_class_functions[] = {
    {"setxy",           php3_ms_point_setXY,            NULL},    
    {"setxyz",          php3_ms_point_setXYZ,           NULL},    
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
    {"addxyz",          php3_ms_line_addXYZ,            NULL},    
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
    {"getpointusingmeasure", php3_ms_shape_getpointusingmeasure, NULL},
    {"getmeasureusingpoint", php3_ms_shape_getmeasureusingpoint, NULL},
    {"buffer",          php3_ms_shape_buffer,           NULL},
    {"topologypreservingsimplify", php3_ms_shape_topologypreservingsimplify, NULL},
    {"simplify",        php3_ms_shape_simplify,         NULL},
    {"convexhull",      php3_ms_shape_convexhull,       NULL},
    {"boundary",        php3_ms_shape_boundary,         NULL},
    {"containsshape",   php3_ms_shape_contains_geos,    NULL},
    {"union_geos",      php3_ms_shape_Union,            NULL},
    {"union",           php3_ms_shape_Union,            NULL},
    {"intersection",    php3_ms_shape_intersection,     NULL},
    {"difference",      php3_ms_shape_difference,       NULL},
    {"symdifference",   php3_ms_shape_symdifference,    NULL},
    {"overlaps",        php3_ms_shape_overlaps,         NULL},
    {"within",          php3_ms_shape_within,           NULL},
    {"crosses",         php3_ms_shape_crosses,          NULL},
    {"touches",         php3_ms_shape_touches,          NULL},
    {"equals",          php3_ms_shape_equals,           NULL},
    {"disjoint",        php3_ms_shape_disjoint,         NULL},
    {"getcentroid",     php3_ms_shape_getcentroid,      NULL},
    {"getarea",         php3_ms_shape_getarea,          NULL},
    {"getlength",       php3_ms_shape_getlength,        NULL},
    {"towkt",           php3_ms_shape_towkt,            NULL},
    {"free",            php3_ms_shape_free,             NULL},
    {"getlabelpoint",   php3_ms_shape_getlabelpoint,    NULL},
    {"setbounds",       php3_ms_shape_setbounds,    NULL},
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
    {"getunits",        php3_ms_projection_getunits,    NULL},
    {"free",            php3_ms_projection_free,        NULL},    
    {NULL, NULL, NULL}
};

function_entry php_style_class_functions[] = {
    {"updatefromstring",  php3_ms_style_updateFromString,     NULL},
    {"set",             php3_ms_style_setProperty,      NULL},    
    {"clone",           php3_ms_style_clone,            NULL},    
    {"setbinding",      php3_ms_style_setBinding,       NULL},    
    {"getbinding",      php3_ms_style_getBinding,       NULL},    
    {"removebinding",      php3_ms_style_removeBinding,       NULL},    
    {NULL, NULL, NULL}
};

function_entry php_outputformat_class_functions[] = {
    {"set",             php_ms_outputformat_setProperty,NULL},
    {"setoption",       php_ms_outputformat_setOption,  NULL},
    {"getoption",       php_ms_outputformat_getOption,  NULL},
    {"validate",        php_ms_outputformat_validate,   NULL},
/* The following are deprecated since 4.3. */
    {"setformatoption", php_ms_outputformat_setOption,  NULL},
    {"getformatoption", php_ms_outputformat_getOption,  NULL},
    {NULL, NULL, NULL}
};

function_entry php_grid_class_functions[] = {
    {"set",             php3_ms_grid_setProperty,       NULL},
    {NULL, NULL, NULL}
};

function_entry php_error_class_functions[] = {
    {"next",            php3_ms_error_next,             NULL},
    {NULL, NULL, NULL}
};


function_entry php_labelcache_class_functions[] = {
    {"free",            php_ms_labelcache_free,         NULL},
    {NULL, NULL, NULL}
};

function_entry php_symbol_class_functions[] = {
    {"set",             php3_ms_symbol_setProperty,     NULL},    
    {"setpoints",       php3_ms_symbol_setPoints,       NULL},    
    {"getpointsarray",  php3_ms_symbol_getPoints,       NULL},    
    {"setpattern",      php3_ms_symbol_setPattern,      NULL},    
    {"getpatternarray", php3_ms_symbol_getPattern,      NULL},    
    {"setimagepath",    php3_ms_symbol_setImagepath,    NULL},    
/* TODO: setstyle and getstylearray deprecated in v5.0. To be removed in a later release */
    {"setstyle",        php3_ms_symbol_setPattern,      NULL},    
    {"getstylearray",   php3_ms_symbol_getPattern,      NULL},    
    {NULL, NULL, NULL}
};


function_entry php_querymap_class_functions[] = {
    {"updatefromstring",  php3_ms_querymap_updateFromString,     NULL},
    {"set",             php3_ms_querymap_setProperty,   NULL},
    {NULL, NULL, NULL}
};

function_entry php_hashtable_class_functions[] = {
    {"get",             php3_ms_hashtable_get,        NULL},    
    {"set",             php3_ms_hashtable_set,        NULL},    
    {"remove",          php3_ms_hashtable_remove,     NULL},    
    {"clear",           php3_ms_hashtable_clear,      NULL},
    {"nextkey",         php3_ms_hashtable_nextkey,    NULL},  
    {NULL, NULL, NULL}
};

function_entry php_cgirequest_class_functions[] = {
    {"loadparams",             php_ms_cgirequest_loadParams,   NULL},
    {"setparameter",             php_ms_cgirequest_setParameter,   NULL},
    {"getname",             php_ms_cgirequest_getName,   NULL},
    {"getvalue",             php_ms_cgirequest_getValue,   NULL},
    {"getvaluebyname",             php_ms_cgirequest_getValueByName,   NULL},
    {NULL, NULL, NULL}
};


PHP_MINFO_FUNCTION(mapscript)
{
  php_info_print_table_start();
  php_info_print_table_row(2, "MapServer Version", msGetVersion());
  php_info_print_table_row(2, "PHP MapScript Version", PHPMS_VERSION);
  php_info_print_table_end();

  /* Remove comments if you have entries in php.ini
  DISPLAY_INI_ENTRIES();
  */

}


PHP_MINIT_FUNCTION(phpms)
{
    zend_class_entry tmp_class_entry;

    int const_flag = CONST_CS|CONST_PERSISTENT;

    /* Init MapServer resources */
    if (msSetup() != MS_SUCCESS)
    {
        php_error(E_ERROR, "msSetup(): MapScript initialization failed.");
        return FAILURE;
    }

    /* If you have defined globals, uncomment this line
    ZEND_INIT_MODULE_GLOBALS(phpms, phpms_init_globals, NULL);
    */

    /* If you have INI entries, uncomment this line
    REGISTER_INI_ENTRIES();
    */

    PHPMS_GLOBAL(le_msmap)  = 
        zend_register_list_destructors_ex(php_ms_free_map, NULL, 
                                          "mapObj", module_number);
    PHPMS_GLOBAL(le_msimg)  = 
        zend_register_list_destructors_ex(php_ms_free_image, NULL,
                                          "imageObj", module_number);

    PHPMS_GLOBAL(le_mslayer)= register_list_destructors(php3_ms_free_layer,
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

    PHPMS_GLOBAL(le_msprojection_ref)= 
      register_list_destructors(php3_ms_free_stub,
                                NULL);

    PHPMS_GLOBAL(le_msscalebar)= 
        zend_register_list_destructors_ex(NULL, NULL,
                                          "scalebarObj", module_number);

    PHPMS_GLOBAL(le_mslegend)= 
        zend_register_list_destructors_ex(NULL, NULL,
                                          "legendObj", module_number);

    PHPMS_GLOBAL(le_msstyle)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);

    PHPMS_GLOBAL(le_msoutputformat)= register_list_destructors(php3_ms_free_stub,
                                                               NULL);

    PHPMS_GLOBAL(le_msgrid)= register_list_destructors(php3_ms_free_stub,
                                                       NULL);

    PHPMS_GLOBAL(le_mserror_ref)= register_list_destructors(php3_ms_free_stub,
                                                            NULL);

    PHPMS_GLOBAL(le_mssymbol)= register_list_destructors(php3_ms_free_stub,
                                                         NULL);

    PHPMS_GLOBAL(le_mslabelcache)= register_list_destructors(php3_ms_free_stub,
                                                             NULL);

    PHPMS_GLOBAL(le_msquerymap)= register_list_destructors(php3_ms_free_stub, 
                                                           NULL);

    PHPMS_GLOBAL(le_mscgirequest)= 
        register_list_destructors(php_ms_free_cgirequest, NULL);
    
    PHPMS_GLOBAL(le_mshashtable)= register_list_destructors(php3_ms_free_stub, 
                                                            NULL);

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
    REGISTER_LONG_CONSTANT("MS_NAUTICALMILES",MS_NAUTICALMILES,const_flag);
    REGISTER_LONG_CONSTANT("MS_KILOMETERS", MS_KILOMETERS,  const_flag);
    REGISTER_LONG_CONSTANT("MS_DD",         MS_DD,          const_flag);
    REGISTER_LONG_CONSTANT("MS_PIXELS",     MS_PIXELS,      const_flag);

    /* layer type constants*/
    REGISTER_LONG_CONSTANT("MS_LAYER_POINT",MS_LAYER_POINT, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_LINE", MS_LAYER_LINE,  const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_POLYGON",MS_LAYER_POLYGON, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_RASTER",MS_LAYER_RASTER, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_ANNOTATION",MS_LAYER_ANNOTATION,const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_QUERY",MS_LAYER_QUERY, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_CIRCLE",MS_LAYER_CIRCLE, const_flag);
    REGISTER_LONG_CONSTANT("MS_LAYER_TILEINDEX",MS_LAYER_TILEINDEX, const_flag);    REGISTER_LONG_CONSTANT("MS_LAYER_CHART",MS_LAYER_CHART, const_flag);


    /* layer status constants (see also MS_ON, MS_OFF) */
    REGISTER_LONG_CONSTANT("MS_DEFAULT",    MS_DEFAULT,     const_flag);
    REGISTER_LONG_CONSTANT("MS_EMBED",      MS_EMBED,       const_flag);
    REGISTER_LONG_CONSTANT("MS_DELETE",     MS_DELETE,       const_flag);

    //For layer transparency, allows alpha transparent pixmaps to be used
    // with RGB map images
    //#define MS_GD_ALPHA 1000
    REGISTER_LONG_CONSTANT("MS_GD_ALPHA",     MS_GD_ALPHA,       const_flag);

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
    REGISTER_LONG_CONSTANT("MS_CC",         MS_CC,          const_flag);
    REGISTER_LONG_CONSTANT("MS_AUTO",       MS_AUTO,        const_flag);
    REGISTER_LONG_CONSTANT("MS_XY",         MS_XY,          const_flag);
    REGISTER_LONG_CONSTANT("MS_FOLLOW",     MS_FOLLOW,      const_flag);

    /* alignment constants*/
    REGISTER_LONG_CONSTANT("MS_ALIGN_LEFT",  MS_ALIGN_LEFT,  const_flag);
    REGISTER_LONG_CONSTANT("MS_ALIGN_CENTER",MS_ALIGN_CENTER,const_flag);
    REGISTER_LONG_CONSTANT("MS_ALIGN_RIGHT", MS_ALIGN_RIGHT, const_flag);

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
    REGISTER_LONG_CONSTANT("MS_SHP_POINTM",  SHP_POINTM,      const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_ARCM",    SHP_ARCM,        const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_POLYGONM",SHP_POLYGONM,    const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_MULTIPOINTM",SHP_MULTIPOINTM, const_flag);
    /* new names??? */
    REGISTER_LONG_CONSTANT("SHP_POINT",     SHP_POINT,      const_flag);
    REGISTER_LONG_CONSTANT("SHP_ARC",       SHP_ARC,        const_flag);
    REGISTER_LONG_CONSTANT("SHP_POLYGON",   SHP_POLYGON,    const_flag);
    REGISTER_LONG_CONSTANT("SHP_MULTIPOINT",SHP_MULTIPOINT, const_flag);
    REGISTER_LONG_CONSTANT("SHP_POINTM",     SHP_POINTM,      const_flag);
    REGISTER_LONG_CONSTANT("SHP_ARCM",       SHP_ARCM,        const_flag);
    REGISTER_LONG_CONSTANT("SHP_POLYGONM",   SHP_POLYGONM,    const_flag);
    REGISTER_LONG_CONSTANT("SHP_MULTIPOINTM",SHP_MULTIPOINTM, const_flag);

    /* query/join type constants*/
    REGISTER_LONG_CONSTANT("MS_SINGLE",     MS_SINGLE,      const_flag);
    REGISTER_LONG_CONSTANT("MS_MULTIPLE",   MS_MULTIPLE,    const_flag);

    /* connection type constants*/
    REGISTER_LONG_CONSTANT("MS_INLINE",     MS_INLINE,      const_flag);
    REGISTER_LONG_CONSTANT("MS_SHAPEFILE",  MS_SHAPEFILE,   const_flag);
    REGISTER_LONG_CONSTANT("MS_TILED_SHAPEFILE",MS_TILED_SHAPEFILE,const_flag);
    REGISTER_LONG_CONSTANT("MS_SDE",        MS_SDE,         const_flag);
    REGISTER_LONG_CONSTANT("MS_OGR",        MS_OGR,         const_flag);
    REGISTER_LONG_CONSTANT("MS_POSTGIS",    MS_POSTGIS,     const_flag);
    REGISTER_LONG_CONSTANT("MS_WMS",        MS_WMS,         const_flag);
    REGISTER_LONG_CONSTANT("MS_ORACLESPATIAL", MS_ORACLESPATIAL,const_flag);
    REGISTER_LONG_CONSTANT("MS_WFS",        MS_WFS,         const_flag);
    REGISTER_LONG_CONSTANT("MS_GRATICULE",  MS_GRATICULE,   const_flag);
    REGISTER_LONG_CONSTANT("MS_MYGIS",      MS_MYGIS,       const_flag);
    REGISTER_LONG_CONSTANT("MS_RASTER",     MS_RASTER,      const_flag);
    REGISTER_LONG_CONSTANT("MS_PLUGIN",     MS_PLUGIN,      const_flag);
 
    /* output image type constants*/
    /*
    REGISTER_LONG_CONSTANT("MS_GIF",        MS_GIF,         const_flag);
    REGISTER_LONG_CONSTANT("MS_PNG",        MS_PNG,         const_flag);
    REGISTER_LONG_CONSTANT("MS_JPEG",       MS_JPEG,        const_flag);
    REGISTER_LONG_CONSTANT("MS_WBMP",       MS_WBMP,        const_flag);
    REGISTER_LONG_CONSTANT("MS_SWF",        MS_SWF,        const_flag);
    */

    /* querymap style constants */
    REGISTER_LONG_CONSTANT("MS_NORMAL",     MS_NORMAL,      const_flag);
    REGISTER_LONG_CONSTANT("MS_HILITE",     MS_HILITE,      const_flag);
    REGISTER_LONG_CONSTANT("MS_SELECTED",   MS_SELECTED,    const_flag);

    /* return value constants */
    REGISTER_LONG_CONSTANT("MS_SUCCESS",    MS_SUCCESS,     const_flag);
    REGISTER_LONG_CONSTANT("MS_FAILURE",    MS_FAILURE,     const_flag);
    REGISTER_LONG_CONSTANT("MS_DONE",       MS_DONE,        const_flag);
   
    /* error code constants */
    REGISTER_LONG_CONSTANT("MS_NOERR",      MS_NOERR,       const_flag);
    REGISTER_LONG_CONSTANT("MS_IOERR",      MS_IOERR,       const_flag);
    REGISTER_LONG_CONSTANT("MS_MEMERR",     MS_MEMERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_TYPEERR",    MS_TYPEERR,     const_flag);
    REGISTER_LONG_CONSTANT("MS_SYMERR",     MS_SYMERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_REGEXERR",   MS_REGEXERR,    const_flag);
    REGISTER_LONG_CONSTANT("MS_TTFERR",     MS_TTFERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_DBFERR",     MS_DBFERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_GDERR",      MS_GDERR,       const_flag);
    REGISTER_LONG_CONSTANT("MS_IDENTERR",   MS_IDENTERR,    const_flag);
    REGISTER_LONG_CONSTANT("MS_EOFERR",     MS_EOFERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_PROJERR",    MS_PROJERR,     const_flag);
    REGISTER_LONG_CONSTANT("MS_MISCERR",    MS_MISCERR,     const_flag);
    REGISTER_LONG_CONSTANT("MS_CGIERR",     MS_CGIERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_WEBERR",     MS_WEBERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_IMGERR",     MS_IMGERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_HASHERR",    MS_HASHERR,     const_flag);
    REGISTER_LONG_CONSTANT("MS_JOINERR",    MS_JOINERR,     const_flag);
    REGISTER_LONG_CONSTANT("MS_NOTFOUND",   MS_NOTFOUND,    const_flag);
    REGISTER_LONG_CONSTANT("MS_SHPERR",     MS_SHPERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_PARSEERR",   MS_PARSEERR,    const_flag);
    REGISTER_LONG_CONSTANT("MS_SDEERR",     MS_SDEERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_OGRERR",     MS_OGRERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_QUERYERR",   MS_QUERYERR,    const_flag);
    REGISTER_LONG_CONSTANT("MS_WMSERR",     MS_WMSERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_WMSCONNERR", MS_WMSCONNERR,  const_flag);
    REGISTER_LONG_CONSTANT("MS_ORACLESPATIALERR", MS_ORACLESPATIALERR, const_flag);
    REGISTER_LONG_CONSTANT("MS_WFSERR",     MS_WFSERR,      const_flag);
    REGISTER_LONG_CONSTANT("MS_WFSCONNERR", MS_WFSCONNERR,  const_flag);
    REGISTER_LONG_CONSTANT("MS_MAPCONTEXTERR", MS_MAPCONTEXTERR, const_flag);
    REGISTER_LONG_CONSTANT("MS_HTTPERR",    MS_HTTPERR,     const_flag);
    REGISTER_LONG_CONSTANT("MS_WCSERR",    MS_WCSERR,     const_flag);
 
    /*symbol types */
    REGISTER_LONG_CONSTANT("MS_SYMBOL_SIMPLE", MS_SYMBOL_SIMPLE, const_flag);
    REGISTER_LONG_CONSTANT("MS_SYMBOL_VECTOR", MS_SYMBOL_VECTOR, const_flag);
    REGISTER_LONG_CONSTANT("MS_SYMBOL_ELLIPSE", MS_SYMBOL_ELLIPSE, const_flag);
    REGISTER_LONG_CONSTANT("MS_SYMBOL_PIXMAP", MS_SYMBOL_PIXMAP, const_flag);
    REGISTER_LONG_CONSTANT("MS_SYMBOL_TRUETYPE", MS_SYMBOL_TRUETYPE, const_flag);
    REGISTER_LONG_CONSTANT("MS_SYMBOL_CARTOLINE", MS_SYMBOL_CARTOLINE, const_flag);
    /* MS_IMAGEMODE types for use with outputFormatObj */
    REGISTER_LONG_CONSTANT("MS_IMAGEMODE_PC256", MS_IMAGEMODE_PC256, const_flag);
    REGISTER_LONG_CONSTANT("MS_IMAGEMODE_RGB",  MS_IMAGEMODE_RGB, const_flag);
    REGISTER_LONG_CONSTANT("MS_IMAGEMODE_RGBA", MS_IMAGEMODE_RGBA, const_flag);
    REGISTER_LONG_CONSTANT("MS_IMAGEMODE_INT16", MS_IMAGEMODE_INT16, const_flag);
    REGISTER_LONG_CONSTANT("MS_IMAGEMODE_FLOAT32", MS_IMAGEMODE_FLOAT32, const_flag);
    REGISTER_LONG_CONSTANT("MS_IMAGEMODE_BYTE", MS_IMAGEMODE_BYTE, const_flag);
    REGISTER_LONG_CONSTANT("MS_IMAGEMODE_NULL", MS_IMAGEMODE_NULL, const_flag);

    /*binding types*/
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_SIZE",  MS_STYLE_BINDING_SIZE, const_flag);
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_WIDTH",  MS_STYLE_BINDING_WIDTH, const_flag);
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_ANGLE", MS_STYLE_BINDING_ANGLE, const_flag);
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_COLOR", MS_STYLE_BINDING_COLOR, const_flag);
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OUTLINECOLOR", MS_STYLE_BINDING_OUTLINECOLOR, const_flag);
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_SYMBOL", MS_STYLE_BINDING_SYMBOL, const_flag);
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OUTLINEWIDTH", MS_STYLE_BINDING_OUTLINEWIDTH, const_flag);
    REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OPACITY", MS_STYLE_BINDING_OPACITY, const_flag);
      
    REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_SIZE",  MS_LABEL_BINDING_SIZE, const_flag);
    REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_ANGLE", MS_LABEL_BINDING_ANGLE, const_flag);
    REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_COLOR", MS_LABEL_BINDING_COLOR, const_flag);
    REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_OUTLINECOLOR", MS_LABEL_BINDING_OUTLINECOLOR, const_flag);
    REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_FONT",  MS_LABEL_BINDING_FONT, const_flag);
    REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_PRIORITY", MS_LABEL_BINDING_PRIORITY, const_flag);
    
    /*cgi request types*/
    REGISTER_LONG_CONSTANT("MS_GET_REQUEST", MS_GET_REQUEST, const_flag);
    REGISTER_LONG_CONSTANT("MS_POST_REQUEST", MS_POST_REQUEST, const_flag);
       
    INIT_CLASS_ENTRY(tmp_class_entry, "ms_map_obj", php_map_class_functions);
    map_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);
    
    INIT_CLASS_ENTRY(tmp_class_entry, "ms_img_obj", php_img_class_functions);
    img_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_rect_obj", php_rect_class_functions);
    rect_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_color_obj", 
                     php_color_class_functions);
    color_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_web_obj", php_web_class_functions);
    web_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_reference_obj", 
                     php_reference_class_functions);
    reference_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);
    
    INIT_CLASS_ENTRY(tmp_class_entry, "ms_scalebar_obj", 
                     php_scalebar_class_functions);
    scalebar_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_legend_obj", 
                     php_legend_class_functions);
    legend_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_layer_obj", 
                     php_layer_class_functions);
    layer_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_label_obj", 
                     php_label_class_functions);
    label_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_class_obj", 
                     php_class_class_functions);
    class_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_point_obj", 
                     php_point_class_functions);
    point_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_line_obj", 
                     php_line_class_functions);
    line_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_shape_obj", 
                     php_shape_class_functions);
    shape_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);
    
    INIT_CLASS_ENTRY(tmp_class_entry, "ms_shapefile_obj", 
                     php_shapefile_class_functions);
    shapefile_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(tmp_class_entry, "ms_projection_obj", 
                     php_projection_class_functions);
    projection_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_style_obj", 
                      php_style_class_functions);
    style_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_outputformat_obj", 
                      php_outputformat_class_functions);
     outputformat_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_grid_obj", 
                      php_grid_class_functions);
     grid_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_error_obj", 
                      php_error_class_functions);
     error_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_labelcache_obj", 
                      php_labelcache_class_functions);
     labelcache_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_symbol_obj", 
                      php_symbol_class_functions);
     symbol_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_querymap_obj", 
                      php_querymap_class_functions);
     querymap_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_cgirequest_obj", php_cgirequest_class_functions);
     cgirequest_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

     INIT_CLASS_ENTRY(tmp_class_entry, "ms_hashtable_obj", php_hashtable_class_functions);
     hashtable_class_entry_ptr = zend_register_internal_class(&tmp_class_entry TSRMLS_CC);

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(phpms)
{
    /* Cleanup MapServer resources */
    msCleanup();

    return SUCCESS;
}

PHP_RINIT_FUNCTION(phpms)
{
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(phpms)
{
    return SUCCESS;
}



/**********************************************************************
 *                     resource list destructors
 **********************************************************************/
static void php_ms_free_map(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    mapObj_destroy((mapObj*)rsrc->ptr);
}

static void php_ms_free_image(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    imageObj *image = (imageObj *)rsrc->ptr;
    
    msFreeImage(image);
}

DLEXPORT void php3_ms_free_layer(layerObj *pLayer) 
{
    layerObj_destroy(pLayer);
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

DLEXPORT void php_ms_free_cgirequest(cgiRequestObj *request) 
{
    cgirequestObj_destroy(request);
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

/************************************************************************/
/*                         php3_ms_getversionint()                      */
/*                                                                      */
/*      Returns the MapServer version in int format.                    */
/*      Given version x.y.z, returns x*0x10000+y*0x100+z                */
/************************************************************************/
DLEXPORT void php3_ms_getversionint(INTERNAL_FUNCTION_PARAMETERS)
{
    RETURN_LONG(msGetVersionInt());
}




/*=====================================================================
 *                 PHP function wrappers - mapObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_map_object()
 **********************************************************************/
static long _phpms_build_map_object(mapObj *pMap, HashTable *list, 
                                    pval *return_value TSRMLS_DC)
{
    int         map_id;
    pval        *new_obj_ptr;

    if (pMap == NULL)
    {
        return 0;
    }

    map_id = php3_list_insert(pMap, PHPMS_GLOBAL(le_msmap));

    _phpms_object_init(return_value, map_id, php_map_class_functions,
                       PHP4_CLASS_ENTRY(map_class_entry_ptr) TSRMLS_CC);

    /* read-only properties */
    add_property_long(return_value, "numlayers", pMap->numlayers);

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value, "name",      pMap->name);
    add_property_long(return_value,  "status",    pMap->status);
    add_property_long(return_value,  "debug",     pMap->debug);
    add_property_long(return_value,  "width",     pMap->width);
    add_property_long(return_value,  "height",    pMap->height);
    add_property_long(return_value,  "maxsize",   pMap->maxsize);
    add_property_long(return_value,  "transparent", pMap->transparent);
    add_property_long(return_value,  "interlace", pMap->interlace);
    PHPMS_ADD_PROP_STR(return_value,  "imagetype", pMap->imagetype);
    add_property_long(return_value,  "imagequality", pMap->imagequality);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_rect_object(&(pMap->extent), PHPMS_GLOBAL(le_msrect_ref),
                             list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "extent", new_obj_ptr, E_ERROR TSRMLS_CC);

    add_property_double(return_value,"cellsize",  pMap->cellsize);
    add_property_long(return_value,  "units",     pMap->units);

    add_property_double(return_value,"scaledenom",pMap->scaledenom);
    /* TODO: scale deprecated in v5.0 remove in future release */
    add_property_double(return_value,"scale",     pMap->scaledenom);

    add_property_double(return_value,"resolution",pMap->resolution);
    add_property_double(return_value,"defresolution",pMap->defresolution);
    PHPMS_ADD_PROP_STR(return_value, "shapepath", pMap->shapepath);
    add_property_long(return_value,  "keysizex",  pMap->legend.keysizex);
    add_property_long(return_value,  "keysizey",  pMap->legend.keysizey);
    add_property_long(return_value, "keyspacingx",pMap->legend.keyspacingx);
    add_property_long(return_value, "keyspacingy",pMap->legend.keyspacingy);
    
    PHPMS_ADD_PROP_STR(return_value, "symbolsetfilename", 
                                                  pMap->symbolset.filename);
    PHPMS_ADD_PROP_STR(return_value, "fontsetfilename", 
                                                  pMap->fontset.filename);
    PHPMS_ADD_PROP_STR(return_value, "mappath",      pMap->mappath);
    
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pMap->imagecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "imagecolor",new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_web_object(&(pMap->web), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "web", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_referenceMap_object(&(pMap->reference), list, 
                                     new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "reference", new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_scalebar_object(&(pMap->scalebar), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "scalebar", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_legend_object(&(pMap->legend), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "legend", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_projection_object(&(pMap->latlon), PHPMS_GLOBAL(le_msprojection_ref),
                                   list,  new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "latlon", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_projection_object(&(pMap->projection), PHPMS_GLOBAL(le_msprojection_ref),
                                   list,  new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "projection", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_outputformat_object(pMap->outputformat, list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "outputformat", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_labelcache_object(&(pMap->labelcache), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "labelcache", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_querymap_object(&(pMap->querymap), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "querymap", new_obj_ptr, E_ERROR TSRMLS_CC);	

    return map_id;
}



/**********************************************************************
 *                        ms_newMapObj()
 **********************************************************************/

/* {{{ proto mapObj ms_newMapObj(string filename)
   Returns a new object to deal with a MapServer map file. */

DLEXPORT void php3_ms_map_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pFname, *pNewPath;
    mapObj      *pNewMap = NULL;
    int         nArgs;
    char        *pszNewPath = NULL;
    HashTable   *list=NULL;

#if defined(WIN32)
    char        szPath[MS_MAXPATHLEN], szFname[MS_MAXPATHLEN];
    char        szNewPath[MS_MAXPATHLEN];
#endif

    nArgs = ARG_COUNT(ht);
    if ((nArgs != 1 && nArgs != 2) ||
        getParameters(ht, nArgs, &pFname, &pNewPath) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFname);

    if (nArgs >= 2)
    {
        convert_to_string(pNewPath);
        pszNewPath = pNewPath->value.str.val;
    }

    /* Attempt to open the MAP file 
     */

#if defined(WIN32)
    /* With PHP4, we have to use the virtual_cwd API... for now we'll
     * just make sure that the .map file path is absolute, but what we
     * should really do is make all of MapServer use the V_* macros and
     * avoid calling setcwd() from anywhere.
     */

    virtual_getcwd(szFname, MS_MAXPATHLEN TSRMLS_CC);
    msBuildPath(szPath, szFname, pFname->value.str.val);
    if (strlen(pFname->value.str.val) == 0)
      szPath[0] = '\0';

    if (pszNewPath)
    {
        msBuildPath(szNewPath, szFname, pszNewPath);
        pNewMap = mapObj_new(szPath, szNewPath);
    }
    else
       pNewMap = mapObj_new(szPath, pszNewPath);
   
#else
    pNewMap = mapObj_new(pFname->value.str.val, pszNewPath);
#endif
    if (pNewMap == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_WARNING, "Failed to open map file %s", 
                            pFname->value.str.val);
        RETURN_FALSE;
    }

    /* Return map object */
    _phpms_build_map_object(pNewMap, list, return_value TSRMLS_CC);

}
/* }}} */


/**********************************************************************
 *                        ms_newMapObjFromString()
 **********************************************************************/

/* {{{ proto mapObj ms_newMapObjFromString(string mapfileString)
   Returns a new object to deal with a MapServer map file. */

DLEXPORT void php3_ms_map_new_from_string(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pMapText, *pNewPath;
    mapObj      *pNewMap = NULL;
    int         nArgs;
    char        *pszNewPath = NULL;
    HashTable   *list=NULL;

#if defined(WIN32)
    char        szMapText[MS_MAXPATHLEN];
    char        szNewPath[MS_MAXPATHLEN];
#endif

    nArgs = ARG_COUNT(ht);
    if ((nArgs != 1 && nArgs != 2) ||
        getParameters(ht, nArgs, &pMapText, &pNewPath) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pMapText);

    if (nArgs >= 2)
    {
        convert_to_string(pNewPath);
        pszNewPath = pNewPath->value.str.val;
    }

    /* Attempt to open the MAP file 
     */

#if defined(WIN32)

    if (pszNewPath)
    {
        virtual_getcwd(szMapText, MS_MAXPATHLEN TSRMLS_CC);
        msBuildPath(szNewPath, szMapText, pszNewPath);
        pNewMap = mapObj_newFromString(pMapText->value.str.val, szNewPath);
    }
    else
       pNewMap = mapObj_newFromString(pMapText->value.str.val, pszNewPath);
   
#else
    pNewMap = mapObj_newFromString(pMapText->value.str.val, pszNewPath);
#endif
    if (pNewMap == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_WARNING, "Failed to open map file %s", 
                            pMapText->value.str.val);
        RETURN_FALSE;
    }

    /* Return map object */
    _phpms_build_map_object(pNewMap, list, return_value TSRMLS_CC);

}
/* }}} */


/**********************************************************************
 *                        map->clone()
 **********************************************************************/

/* {{{ proto int map.clone()
   Make a copy of this mapObj and returne a refrence to it. Returns NULL(0) on error. */

DLEXPORT void php3_ms_map_clone(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self, *pNewMap;
    pval   *pThis;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) != 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    /* Make a copy of current map object */
    if ((pNewMap = mapObj_clone(self)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }

    /* Return new map object */
    _phpms_build_map_object(pNewMap, list, return_value TSRMLS_CC);

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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);


    IF_SET_STRING(     "name",        self->name)
    else IF_SET_LONG(  "status",      self->status)
    else IF_SET_LONG(  "debug",       self->debug)
    else IF_SET_LONG(  "width",       self->width)
    else IF_SET_LONG(  "height",      self->height)
    else IF_SET_LONG(  "maxsize",     self->maxsize)
    else IF_SET_LONG(  "transparent", self->transparent)
    else IF_SET_LONG(  "interlace",   self->interlace)
    else IF_SET_LONG(  "imagequality",self->imagequality)
    else IF_SET_DOUBLE("cellsize",    self->cellsize)
    else IF_SET_LONG(  "units",       self->units)
    else IF_SET_DOUBLE("scaledenom",  self->scaledenom)
    /* TODO: scale deprecated in v5.0 remove in future release */
    else IF_SET_DOUBLE("scale",       self->scaledenom)
    else IF_SET_DOUBLE("resolution",  self->resolution)
    else IF_SET_DOUBLE("defresolution",  self->defresolution)
    else IF_SET_STRING("shapepath",   self->shapepath)
    else IF_SET_LONG(  "keysizex",    self->legend.keysizex)
    else IF_SET_LONG(  "keysizey",    self->legend.keysizey)
    else IF_SET_LONG(  "keyspacingx", self->legend.keyspacingx)
    else IF_SET_LONG(  "keyspacingy", self->legend.keyspacingy)
    else if (strcmp( "numlayers", pPropertyName->value.str.val) == 0 ||
             strcmp( "extent", pPropertyName->value.str.val) == 0 ||
             strcmp( "symbolsetfilename", pPropertyName->value.str.val) == 0 ||
             strcmp( "fontsetfilename", pPropertyName->value.str.val) == 0 ||
             strcmp( "imagetype", pPropertyName->value.str.val) == 0)
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
    pval   **pExtent;
    pval   *pMinX, *pMinY, *pMaxX, *pMaxY;
    pval *pThis;
    int nStatus;

    HashTable   *list=NULL;

    pThis = getThis();    

    if (pThis == NULL ||
        getParameters(ht, 4, &pMinX, &pMinY, &pMaxX, &pMaxY) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    convert_to_double(pMinX);
    convert_to_double(pMinY);
    convert_to_double(pMaxX);
    convert_to_double(pMaxY);

    nStatus = msMapSetExtent( self, 
                              pMinX->value.dval, pMinY->value.dval,
                              pMaxX->value.dval, pMaxY->value.dval);
    
    if (nStatus != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* We still sync the class members even if the call failed */

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR TSRMLS_CC); 
    _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
    /* TODO: scale deprecated in v5.0 remove in future release */
    _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 

    if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", sizeof("extent"),
                       (void *)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR TSRMLS_CC);
    }

    RETURN_LONG(nStatus);
}

/**********************************************************************
 *                        map->setRotation()
 **********************************************************************/

/* {{{ proto int map.setrotation(double rotation_angle)
   Set map rotation angle. The map view rectangle (specified in EXTENTS) will be rotated by the indicated angle in the counter-clockwise direction. Note that this implies the rendered map will be rotated by the angle in the clockwise direction. */

DLEXPORT void php3_ms_map_setRotation(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pAngle;
    pval   *pThis;
    HashTable   *list=NULL;
    int nStatus;

    pThis = getThis();
    
    if (pThis == NULL ||
        getParameters(ht, 1, &pAngle) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FALSE);
    }

    convert_to_double(pAngle);

    if ((nStatus = mapObj_setRotation(self, pAngle->value.dval)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}

/**********************************************************************
 *                        map->setSize()
 **********************************************************************/

/* {{{ proto int map.setextent(string property_name, new_value)
   Set map size (width, height) properties to new values and upddate internal geotransform. Returns MS_SUCCESS/MS_FAILURE. */

DLEXPORT void php3_ms_map_setSize(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pWidth, *pHeight;
    pval *pThis;
    int nStatus;

    HashTable   *list=NULL;

    pThis = getThis();    

    if (pThis == NULL ||
        getParameters(ht, 2, &pWidth, &pHeight) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    convert_to_long(pWidth);
    convert_to_long(pHeight);

    if ((nStatus = msMapSetSize( self, pWidth->value.lval, 
                                 pHeight->value.lval) ) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    /* We still sync the class members even if the call failed
     * Note that as I'm implementing this, msMapSetSize() doesn't
     * update scale and cellsize but I think it should, so we'll sync
     * them as well, and if/when they are updated by the low-level function
     * then they'll be sync'd 
     */
    _phpms_set_property_double(pThis, "cellsize", self->cellsize, E_ERROR TSRMLS_CC); 
    _phpms_set_property_double(pThis, "scaledenom",self->scaledenom,E_ERROR TSRMLS_CC); 
    /* TODO: scale deprecated in v5.0 remove in future release */
    _phpms_set_property_double(pThis, "scale",    self->scaledenom,E_ERROR TSRMLS_CC); 
    _phpms_set_property_double(pThis, "width",    self->width,    E_ERROR TSRMLS_CC); 
    _phpms_set_property_double(pThis, "height",   self->height,   E_ERROR TSRMLS_CC); 


    RETURN_LONG(nStatus);
}

/**********************************************************************
 *        map->setProjection() and map->setWKTProjection()
 **********************************************************************/

static int _php3_ms_map_setProjection(int bWKTProj, mapObj *self, pval *pThis,
                                      int nArgs, pval *pProjString, 
                                      pval *pSetUnitsAndExtents TSRMLS_DC)
{
#ifdef USE_PROJ
    int                 nStatus = 0;
    int                 nUnits =   MS_METERS;  
    projectionObj       in;
    projectionObj       out;
    rectObj             sRect;
    int                 bSetNewExtents = 0; 
    int                 bSetUnitsAndExtents = 0;
    pval                **pExtent, *new_obj_ptr;
    HashTable           *list=NULL;
    

    convert_to_string(pProjString);
    if (nArgs == 2)
    {
        convert_to_long(pSetUnitsAndExtents);
        bSetUnitsAndExtents = pSetUnitsAndExtents->value.lval;
    }

    in = self->projection;
    msInitProjection(&out);
    if (bWKTProj)
        msOGCWKT2ProjectionObj(pProjString->value.str.val, &(out),self->debug);
    else
        msLoadProjectionString(&(out),  pProjString->value.str.val);
    sRect = self->extent;
 
    if (in.proj!= NULL && out.proj!=NULL)
    {
        if (msProjectionsDiffer(&in, &out))
        {
            if (msProjectRect(&in, &out, &sRect) == MS_SUCCESS)
              bSetNewExtents =1;
        }
    }
    /* Free the temporary projection object */
    msFreeProjection(&out);

    if (bWKTProj) 
        nStatus = mapObj_setWKTProjection(self, pProjString->value.str.val);
    else
        nStatus = mapObj_setProjection(self, pProjString->value.str.val);

    if (nStatus == -1)
        _phpms_report_mapserver_error(E_ERROR);
    else
    { /* update the php projection object */
       zend_hash_del(Z_OBJPROP_P(pThis), "projection", 
                      sizeof("projection"));
      
       MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
       _phpms_build_projection_object(&(self->projection), PHPMS_GLOBAL(le_msprojection_ref),
                                      list,  new_obj_ptr TSRMLS_CC);
                                      _phpms_add_property_object(pThis, "projection", new_obj_ptr, E_ERROR TSRMLS_CC);
    }
    
    nUnits = GetMapserverUnitUsingProj(&(self->projection));
    if (nUnits != -1 && bSetUnitsAndExtents)
    {
/* -------------------------------------------------------------------- 
      set the units and map extents.                                  
 -------------------------------------------------------------------- */
        self->units = nUnits;

        if (bSetNewExtents)
        {
            self->extent = sRect;

            self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                            self->height); 
            msCalculateScale(self->extent, self->units, self->width, self->height, 
                             self->resolution, &(self->scaledenom));

            _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR TSRMLS_CC); 
            _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
            /* TODO: scale deprecated in v5.0 remove in future release */
            _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 
            _phpms_set_property_long(pThis,"units", self->units, E_ERROR TSRMLS_CC); 

            if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", 
                               sizeof("extent"), (void *)&pExtent) == SUCCESS)
            {
                _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                           E_ERROR TSRMLS_CC);
                _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                           E_ERROR TSRMLS_CC);
                _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                           E_ERROR TSRMLS_CC);
                _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                           E_ERROR TSRMLS_CC);
            }
        }
    }

    return nStatus;
#else
    php3_error(E_ERROR, 
               "setProjection() available only with PROJ.4 support.");
    return -1;
#endif
}

/**********************************************************************
 *                        map->setProjection()
 **********************************************************************/
/* {{{ proto int map.setProjection(string projection)
   Set projection and coord. system for the map. Returns -1 on error. */

DLEXPORT void php3_ms_map_setProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj              *self;
    pval                *pProjString, *pSetUnitsAndExtents;
    pval                *pThis;
    int                 nStatus = 0;
    int                 nArgs = ARG_COUNT(ht);

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

 
    if (pThis == NULL ||
        (nArgs != 1 && nArgs != 2))
    {
        WRONG_PARAM_COUNT;
    }
        
    if (getParameters(ht, nArgs, &pProjString, &pSetUnitsAndExtents) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    nStatus = _php3_ms_map_setProjection(MS_FALSE, self, pThis, 
                                         nArgs, pProjString, 
                                         pSetUnitsAndExtents TSRMLS_CC);

    RETURN_LONG(nStatus);
}

/* }}} */

/**********************************************************************
 *                        map->setWKTProjection()
 **********************************************************************/
/* {{{ proto int map.setWKTProjection(string projection)
   Set projection and coord. system for the map. Returns -1 on error. */

DLEXPORT void php3_ms_map_setWKTProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj              *self;
    pval                *pProjString, *pSetUnitsAndExtents;
    pval                *pThis;
    int                 nStatus = 0;
    int                 nArgs = ARG_COUNT(ht);
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

 
    if (pThis == NULL ||
        (nArgs != 1 && nArgs != 2))
    {
        WRONG_PARAM_COUNT;
    }
        
    if (getParameters(ht, nArgs, &pProjString, &pSetUnitsAndExtents) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    nStatus = _php3_ms_map_setProjection(MS_TRUE, self, pThis, 
                                         nArgs, pProjString, 
                                         pSetUnitsAndExtents TSRMLS_CC);

    RETURN_LONG(nStatus);
}

/* }}} */


/**********************************************************************
 *                        map->getProjection()
 **********************************************************************/

/* {{{ proto int map.getProjection()
   Return the projection string of the map. Returns FALSE on error. */
DLEXPORT void php3_ms_map_getProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj      *self;
    pval        *pThis = NULL;
    char        *pszPojString = NULL;

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
        RETURN_FALSE;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    pszPojString = mapObj_getProjection(self);
    if (pszPojString == NULL)
    {
        RETURN_FALSE;
    }
    else
    {
        RETVAL_STRING(pszPojString, 1);
        free(pszPojString);
    }
}
    
/* ==================================================================== */
/*      Zoom related functions.                                         */
/* ==================================================================== */

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
    HashTable   *list=NULL;

    pThis = getThis();

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



    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
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
                                                  list TSRMLS_CC);
    poPixPos = (pointObj *)_phpms_fetch_handle2(pPixelPos, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list TSRMLS_CC);
    
    if (bMaxExtSet)
        poMaxGeorefExt = 
            (rectObj *)_phpms_fetch_handle2(pMaxGeorefExt, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list TSRMLS_CC);

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

    if (self->web.maxscaledenom > 0)
    {
        if (pZoomFactor->value.lval < 0 && dfNewScale >  self->web.maxscaledenom)
        {
            RETURN_FALSE;
        }
    }

/* ==================================================================== */
/*      we do a spcial case for zoom in : we try to zoom as much as     */
/*      possible using the mincale set in the .map.                     */
/* ==================================================================== */
    if (self->web.minscaledenom > 0 && dfNewScale <  self->web.minscaledenom &&
        pZoomFactor->value.lval > 1)
    {
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscaledenom, self->units,
                                      dfGeoPosY, self->width, 
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
            self->extent.maxy = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }
    
    if (msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scaledenom)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR TSRMLS_CC); 
    _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
    /* TODO: scale deprecated in v5.0 remove in future release */
    _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 

    if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", sizeof("extent"), 
                       (void *)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR TSRMLS_CC);
    }

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

    HashTable   *list=NULL;

    pThis = getThis();

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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
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
                                                  list TSRMLS_CC);
    poPixExt = (rectObj *)_phpms_fetch_handle2(pPixelExt, 
                                               PHPMS_GLOBAL(le_msrect_ref), 
                                               PHPMS_GLOBAL(le_msrect_new),
                                               list TSRMLS_CC);

    if (bMaxExtSet)
        poMaxGeorefExt = 
            (rectObj *)_phpms_fetch_handle2(pMaxGeorefExt, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list TSRMLS_CC);
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
/*      use them to test before setting extents.                        */
/* -------------------------------------------------------------------- */
    if (msCalculateScale(oNewGeorefExt, self->units, 
                         self->width, self->height, 
                         self->resolution, &dfNewScale) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    if (self->web.maxscaledenom > 0 &&  dfNewScale > self->web.maxscaledenom)
    {
        RETURN_FALSE;
    }

    if (self->web.minscaledenom > 0 && dfNewScale <  self->web.minscaledenom)
    {
        dfMiddleX = oNewGeorefExt.minx + 
            ((oNewGeorefExt.maxx - oNewGeorefExt.minx)/2);
        dfMiddleY = oNewGeorefExt.miny + 
            ((oNewGeorefExt.maxy - oNewGeorefExt.miny)/2);
        
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscaledenom, self->units, 
                                      dfMiddleY, self->width, 
                                      self->resolution);

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
            self->extent.maxy = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }

    if (msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scaledenom)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR TSRMLS_CC); 
    _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
    /* TODO: scale deprecated in v5.0 remove in future release */
    _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 

    if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", sizeof("extent"), 
                        (void *)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR TSRMLS_CC);
    }

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
    pval       **pExtent;
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
    HashTable   *list=NULL;

    pThis = getThis();

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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    if (nArgs == 6)
        bMaxExtSet =1;

    convert_to_double(pScale);
    convert_to_long(pWidth);
    convert_to_long(pHeight);
         
    poGeorefExt = (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                                  PHPMS_GLOBAL(le_msrect_ref),
                                                  PHPMS_GLOBAL(le_msrect_new),
                                                  list TSRMLS_CC);
    poPixPos = (pointObj *)_phpms_fetch_handle2(pPixelPos, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list TSRMLS_CC);
    
    if (bMaxExtSet)
        poMaxGeorefExt = 
            (rectObj *)_phpms_fetch_handle2(pMaxGeorefExt, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list TSRMLS_CC);

/* -------------------------------------------------------------------- */
/*      check the validity of the parameters.                           */
/* -------------------------------------------------------------------- */
    if (pScale->value.dval <= 0.0 || 
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
        GetDeltaExtentsUsingScale(pScale->value.dval, self->units, dfGeoPosY,
                                  nTmp, self->resolution);
                                  
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

    if (self->web.maxscaledenom > 0)
    {
        if (dfCurrentScale < dfNewScale && dfNewScale >  self->web.maxscaledenom)
        {
            RETURN_FALSE;
        }
    }

/* ==================================================================== */
/*      we do a special case for zoom in : we try to zoom as much as    */
/*      possible using the mincale set in the .map.                     */
/* ==================================================================== */
    if (self->web.minscaledenom > 0 && dfNewScale <  self->web.minscaledenom &&
        dfCurrentScale > dfNewScale)
    {
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscaledenom, self->units, 
                                      dfGeoPosY, self->width, 
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
            self->extent.maxy = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }
    
    if (msCalculateScale(self->extent, self->units, self->width, self->height, 
                         self->resolution, &(self->scaledenom)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR TSRMLS_CC); 
    _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
    /* TODO: scale deprecated in v5.0 remove in future release */
    _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 

    if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", sizeof("extent"), 
                       (void *)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR TSRMLS_CC);
    }

     RETURN_TRUE;

}
    


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


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self != NULL)
    {
        nSymbolId = 
            mapObj_getSymbolByName(self, pSymName->value.str.val);
    }

    RETURN_LONG(nSymbolId);
}


/************************************************************************/
/*                              DLEXPORT void                           */
/*        php3_ms_map_getSymbolByName(INTERNAL_FUNCTION_PARAMETERS)     */
/*                                                                      */
/*       Get the symbol id using it's name. Parameters are :            */
/*                                                                      */
/*        - symbol name                                                 */
/************************************************************************/
DLEXPORT void php3_ms_map_getSymbolObjectById(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    pval        *pSymId;
    mapObj      *self=NULL;
    symbolObj *psSymbol = NULL;
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
        getParameters(ht, 1, &pSymId) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pSymId);


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    if (self == NULL)
      php3_error(E_ERROR, "Invalid map object.");

    if ( pSymId->value.lval < 0 ||  
         pSymId->value.lval >= self->symbolset.numsymbols)
      php3_error(E_ERROR, "Invalid symbol index.");

    map_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR TSRMLS_CC);

    psSymbol = self->symbolset.symbol[pSymId->value.lval];
    /* Return style object */
    _phpms_build_symbol_object(psSymbol, map_id,  list, return_value TSRMLS_CC);
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
    imageObj *im = NULL;
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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL || (im = mapObj_prepareImage(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value TSRMLS_CC);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self != NULL)
    {
        mapObj_prepareQuery(self);

        _phpms_set_property_double(pThis,"cellsize", self->cellsize, 
                                 E_ERROR TSRMLS_CC); 
        _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
        /* TODO: scale deprecated in v5.0 remove in future release */
        _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 
    }
    
}
/**********************************************************************
 *                        map->draw()
 **********************************************************************/

/* {{{ proto int map.draw()
   Render map and return handle on image object. */

DLEXPORT void php3_ms_map_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    mapObj      *self;
    imageObj    *im = NULL;
    pval        **pExtent;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL || (im = mapObj_draw(self)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }
    else
    {
        /* Return an image object */
/* -------------------------------------------------------------------- */
/*      since the darwmap forces the scale and cellsize calculation,    */
/*      we also update the php object with the latest values.           */
/* -------------------------------------------------------------------- */
         _phpms_set_property_double(pThis,"cellsize", self->cellsize, 
                                    E_ERROR TSRMLS_CC); 
         _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
         /* TODO: scale deprecated in v5.0 remove in future release */
         _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 

         if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", 
                            sizeof("extent"), (void *)&pExtent) == SUCCESS)
         {
             _phpms_set_property_double((*pExtent),"minx", 
                                        self->extent.minx, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pExtent),"miny", 
                                        self->extent.miny, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pExtent),"maxx", 
                                        self->extent.maxx, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pExtent),"maxy", 
                                        self->extent.maxy, 
                                        E_ERROR TSRMLS_CC);
         }
         
         _phpms_build_img_object(im, &(self->web), list, return_value TSRMLS_CC);
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
    imageObj    *im = NULL;
    pval        **pExtent;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL || (im =  mapObj_drawQuery(self)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }
    else
    {
/* -------------------------------------------------------------------- */
/*      since the darwmap forces the scale and cellsize calculation,    */
/*      we also update the php object with the latest values.           */
/* -------------------------------------------------------------------- */
         _phpms_set_property_double(pThis,"cellsize", self->cellsize, 
                                    E_ERROR TSRMLS_CC); 
         _phpms_set_property_double(pThis,"scaledenom", self->scaledenom, E_ERROR TSRMLS_CC); 
         /* TODO: scale deprecated in v5.0 remove in future release */
         _phpms_set_property_double(pThis,"scale", self->scaledenom, E_ERROR TSRMLS_CC); 

         if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", 
                            sizeof("extent"), (void *)&pExtent) == SUCCESS)
         {
             _phpms_set_property_double((*pExtent),"minx", 
                                        self->extent.minx, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pExtent),"miny", 
                                        self->extent.miny, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pExtent),"maxx", 
                                        self->extent.maxx, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pExtent),"maxy", 
                                        self->extent.maxy, 
                                        E_ERROR TSRMLS_CC);
         }

        /* Return an image object */
         _phpms_build_img_object(im, &(self->web), list, return_value TSRMLS_CC);
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
    imageObj *im = NULL;
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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL || (im = mapObj_drawLegend(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value TSRMLS_CC);
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
    imageObj *im = NULL;
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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL || (im = mapObj_drawReferenceMap(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value TSRMLS_CC);
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
    imageObj *im = NULL;

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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
    if (self == NULL || (im = mapObj_drawScalebar(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value TSRMLS_CC);
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
    imageObj *im = NULL;
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
        
    im = (imageObj *)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
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
    imageObj *im = NULL;
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
        
    im = (imageObj *)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
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
    imageObj *im = NULL;
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
        
    im = (imageObj *)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL || 
        (newLayer = mapObj_getLayer(self, pLyrIndex->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* We will store a reference to the parent object id (this) inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR TSRMLS_CC);

    /* Return layer object */
    _phpms_build_layer_object(newLayer, map_id, list, return_value TSRMLS_CC);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL ||
        (newLayer=mapObj_getLayerByName(self, pLyrName->value.str.val))==NULL )
    {
        // No MapServer error to report.  Just produce a PHP warning.
        php3_error(E_WARNING, "getLayerByName failed for : %s\n",
                   pLyrName->value.str.val);
        RETURN_FALSE;
    }

    /* We will store a reference to the parent object id (this) inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR TSRMLS_CC);

    /* Return layer object */
    _phpms_build_layer_object(newLayer, map_id, list, return_value TSRMLS_CC);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self != NULL)
    {
        nCount = self->numlayers;
        for (i=0; i<nCount; i++)
        {
            add_next_index_string(return_value,  self->layers[i]->name, 1);
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
   Return an array containing all the group names.*/

DLEXPORT void php3_ms_map_getAllGroupNames(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    mapObj      *self=NULL;
    int         i = 0;
    char        **papszGroups = NULL;

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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self != NULL && self->numlayers > 0)
    {
       int numTok;
       papszGroups = msGetAllGroupNames(self, &numTok);
       
       for (i=0; i<numTok; i++)
       {
         /* add a copy of the group name to the PHP array */
          add_next_index_string(return_value, papszGroups[i], 1);
          
          free(papszGroups[i]);
       }
       free(papszGroups);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
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
            php3_error(E_ERROR, "getColorByIndex failed"
                       "Index shoud not be higher than %d\n",
                       palette.numcolors-1);
        }
    }
    else
    {
        RETURN_FALSE;
    }

    /* Return color object */
    _phpms_build_color_object(&oColor, list, return_value TSRMLS_CC);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list TSRMLS_CC);

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


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    poRect = (rectObj *)_phpms_fetch_handle2(pRect,
                                             PHPMS_GLOBAL(le_msrect_ref),
                                             PHPMS_GLOBAL(le_msrect_new),
                                             list TSRMLS_CC);

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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

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


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    poShape = 
        (shapeObj *)_phpms_fetch_handle2(pShape, 
                                         PHPMS_GLOBAL(le_msshape_ref),
                                         PHPMS_GLOBAL(le_msshape_new),
                                         list TSRMLS_CC);
    
    if (self && poShape && 
        (nStatus = mapObj_queryByShape(self, poShape))!= MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}

 
/**********************************************************************
 *     map->queryByIndex(qlayer, tileindex, shapeindex, bAddtoQuery)
 *
 *
 * Add a shape into the query result on a specific layer.
 * if bAddtoQuery (not mandatory) isset to true, the sahpe will be added to 
 * the existing query result. Else the query result is cleared before adding 
 * the sahpe (which is the default behavior).
 **********************************************************************/
DLEXPORT void php3_ms_map_queryByIndex(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pQlayer, *pTileIndex, *pShapeIndex, *pAddToQuery;
    mapObj      *self=NULL;
    int         nStatus = MS_FAILURE;
    int         bAddToQuery = -1;
    int         nArgs = ARG_COUNT(ht);

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        (nArgs != 3 && nArgs != 4))
    {
        WRONG_PARAM_COUNT;
    }

    if (pThis == NULL ||
        getParameters(ht, nArgs, &pQlayer, &pTileIndex, &pShapeIndex,
                      &pAddToQuery) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pQlayer);
    convert_to_long(pTileIndex);
    convert_to_long(pShapeIndex);
    
    if (nArgs == 4)
    {
      convert_to_long(pAddToQuery);
      bAddToQuery = pAddToQuery->value.lval;
    }


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    
    if (self && 
        (nStatus = mapObj_queryByIndex(self, pQlayer->value.lval,
                                       pTileIndex->value.lval, 
                                       pShapeIndex->value.lval,
                                       bAddToQuery))!= MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
}

 

/**********************************************************************
 *                    map->savequery(filename)
 *
 * Save the current query to a specfied file. Cal be used with loadquery
 **********************************************************************/

/* {{{ proto int map.savequery(filename) */
DLEXPORT void php3_ms_map_savequery(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pFileName;
    mapObj      *self=NULL;
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
        getParameters(ht, 1, &pFileName) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFileName);
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    
    nStatus = mapObj_saveQuery(self, pFileName->value.str.val);

    RETURN_LONG(nStatus);
}


/**********************************************************************
 *                    map->loadquery(filename)
 *
 * Load the query from a specfied file. Used with savequery
 **********************************************************************/

/* {{{ proto int map.loadquery(filename) */
DLEXPORT void php3_ms_map_loadquery(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pFileName;
    mapObj      *self=NULL;
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
        getParameters(ht, 1, &pFileName) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFileName);
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    
    nStatus = mapObj_loadQuery(self, pFileName->value.str.val);

    RETURN_LONG(nStatus);
}


/**********************************************************************
 *                    map->freequery(qlayer)
 *
 * Free the query on a specified layer. If qlayer is set to -1
 * all queries on all layers will be freed.
 **********************************************************************/

/* {{{ proto int map->freequery(qlayer) */
DLEXPORT void php3_ms_map_freequery(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pQlayer;
    mapObj      *self=NULL;
    int         nStatus = MS_SUCCESS;


#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pQlayer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pQlayer);
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    
    mapObj_freeQuery(self, pQlayer->value.lval);

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

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list TSRMLS_CC);
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
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
                             list, return_value TSRMLS_CC);
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
                                           list TSRMLS_CC);
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
                                           list TSRMLS_CC);
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



/**********************************************************************
 *                        map->removeMetaData()
 **********************************************************************/

/* {{{ proto int map.removeMetaData(string name)
   Remove MetaData entry using by name.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_map_removeMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pThis, *pName;
    int    nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pName) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pName);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = mapObj_removeMetaData(self, 
                                         pName->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}
/* }}} */


/**********************************************************************
 *                        map->movelayerup()
 *
 * Move layer up in the list of layers (for drawing purposes).
 * The layer in this case is drawn earlier.
 **********************************************************************/

/* {{{ proto int map.movelayerup(int layer_index)
   Returns true on sucess, else false. */

DLEXPORT void php3_ms_map_moveLayerUp(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pLyrIdx, *pThis;
    mapObj *self=NULL;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pLyrIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pLyrIdx);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self != NULL)
    {
        if (mapObj_moveLayerup(self, pLyrIdx->value.lval) == 0)
            RETURN_TRUE;       
    }

    RETURN_FALSE;
}
/* }}} */

/**********************************************************************
 *                        map->movelayerdown()
 *
 * Move layer down in the list of layers (for drawing purposes).
 * The layer in this case is drawn later.
 **********************************************************************/

/* {{{ proto int map.movelayerdown(int layer_index)
   Returns true on sucess, else false. */

DLEXPORT void php3_ms_map_moveLayerDown(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pLyrIdx, *pThis;
    mapObj *self=NULL;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pLyrIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pLyrIdx);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self != NULL)
    {
        if (mapObj_moveLayerdown(self, pLyrIdx->value.lval) == 0)
            RETURN_TRUE;       
    }

    RETURN_FALSE;
}
/* }}} */


/**********************************************************************
 *                        map->getLayersDrawingOrder()
 *
 * Return an array conating all the layers name.
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_getLayersDrawingOrder()
   Return an array conating all the layers index sorted by drawing order.
   Note : the first element in the array is the one drawn first.*/
DLEXPORT void php3_ms_map_getLayersDrawingOrder(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    mapObj      *self=NULL;
    int         nCount = 0;
    int         i = 0;
    int         *panLayers = NULL;

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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    panLayers = mapObj_getLayersdrawingOrder(self);
    if (self != NULL)
    {
        nCount = self->numlayers;

/* -------------------------------------------------------------------- */
/*      Go through the prioriy list and return the layers index. If     */
/*      the priority list is not set, it will return the layer          */
/*      indexs as they were at the load time.                           */
/* -------------------------------------------------------------------- */
        for (i=0; i<nCount; i++)
        {
            if (panLayers)
            {
                add_next_index_long(return_value,  panLayers[i]);
            }
            else
                add_next_index_long(return_value, i);
        }
    }
    else
    {
        RETURN_FALSE;
    }
}

/**********************************************************************
 *                        map->setLayersDrawingOrder()
 *
 * Set the array used for the drawing order. 
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_getLayersDrawingOrder(array_layer_index)
   array_layers_index : an array containing all the layer's index ordered
                        by the drawing priority.
                        Ex : for 3 layers in the map file, if 
                            array[0] = 2
                            array[1] = 0
                            array[2] = 1
                            will set the darwing order to layer 2, layer 0,
                            and then layer 1.
   Return TRUE on success or FALSE.
   Note : the first element in the array is the one drawn first.*/
DLEXPORT void php3_ms_map_setLayersDrawingOrder(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pArrayIndexes;
    mapObj      *self=NULL;
    int         nElements = 0;
    int         i = 0;

    int         *panIndexes = NULL;

    HashTable   *list=NULL;
    pval        **pValue = NULL;

    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }


    if (ZEND_NUM_ARGS() != 1 || getParameters(ht,1,&pArrayIndexes)==FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
    
        
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    if (pArrayIndexes->type != IS_ARRAY) 
    {
        php_error(E_WARNING, 
                  "setLayersDrawingOrder : expected array as parameter");
        RETURN_FALSE;
    }

    nElements = zend_hash_num_elements(pArrayIndexes->value.ht);

/* -------------------------------------------------------------------- */
/*      validate that the array of index given has the same size as     */
/*      the number of layers and also the the indexs are valid.         */
/* -------------------------------------------------------------------- */
    if (self->numlayers != nElements)
    {
        RETURN_FALSE;
    }
    panIndexes = (int *)malloc(sizeof(int)*nElements);
    for (i=0; i<nElements; i++)
    {
        if (zend_hash_index_find(pArrayIndexes->value.ht, i, 
                                 (void *)&pValue) == FAILURE)
        {
            RETURN_FALSE;
        }
        convert_to_long((*pValue));
        panIndexes[i] = (*pValue)->value.lval;
    }
    
    if (!mapObj_setLayersdrawingOrder(self, panIndexes))
    {
        free(panIndexes);
        RETURN_FALSE;
    }
    free(panIndexes);

    RETURN_TRUE;
}       



/**********************************************************************
 *                        map->processTemplate()
 *
 * Process a template  
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_processTemplate(array_layer_index)*/

DLEXPORT void php3_ms_map_processTemplate(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    pval        *pParamValue, *pGenerateImage;
    mapObj      *self=NULL;
    char        *pszBuffer = NULL;
    int         i, iIndice = 0;
    HashTable   *ar;
    int         numelems=0,  size;
    char        **papszNameValue = NULL;
    char        **papszName = NULL;
    char        **papszValue = NULL;

    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }

    if (ZEND_NUM_ARGS() != 2 || 
        getParameters(ht,2,&pParamValue, &pGenerateImage)==FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
    
    convert_to_long(pGenerateImage);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
        RETURN_FALSE;

/* -------------------------------------------------------------------- */
/*      build the parm and value arrays.                                */
/*      Note : code extacted from sablot.c (functions related to        */
/*      xslt)                                                           */
/* -------------------------------------------------------------------- */
    ar = HASH_OF(pParamValue);
    if (ar)
    {
        /**
         * Allocate 2 times the number of elements in
         * the array, since with associative arrays in PHP
         * keys are not counted.
         */

        numelems = zend_hash_num_elements(ar);
        size = (numelems * 2 + 1) * sizeof(char *);
            
        papszNameValue = (char **)emalloc(size+1);
        memset((char *)papszNameValue, 0, size);
        
        /**
         * Translate a PHP array (HashTable *) into a 
         * Sablotron array (char **).
         */
        if (_php_extract_associative_array(ar, papszNameValue))
        {
            papszName = (char **)malloc(sizeof(char *)*numelems);
            papszValue = (char **)malloc(sizeof(char *)*numelems);
            
            
            for (i=0; i<numelems; i++)
            {
                iIndice = i*2;
                papszName[i] = papszNameValue[iIndice];
                papszValue[i] = papszNameValue[iIndice+1];
            }

        }
        else
        {
            // Failed for some reason
            php_error(E_WARNING, 
                      "processLegendTemplate: failed reading array");
            RETURN_FALSE;
        }
        efree(papszNameValue);
    }

    pszBuffer = mapObj_processTemplate(self, pGenerateImage->value.lval,
                                       papszName, papszValue, numelems);
            
    msFree(papszName);  // The strings inside the array are just refs
    msFree(papszValue);

    if (pszBuffer)
    {
        RETVAL_STRING(pszBuffer, 1);
        free(pszBuffer);
    }
    else
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_STRING("", 0);
    }

}

/**********************************************************************
 *                        map->processLegendTemplate()
 *
 * Process the legend template.
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_processLegendTemplate(array_layer_index)*/

DLEXPORT void php3_ms_map_processLegendTemplate(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    pval        *pParamValue;
    mapObj      *self=NULL;
    char        *pszBuffer = NULL;
    int         i, iIndice = 0;
    HashTable   *ar;
    int         numelems=0,  size;
    char        **papszNameValue = NULL;
    char        **papszName = NULL;
    char        **papszValue = NULL;

    
    HashTable   *list=NULL;


    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }

    if (ZEND_NUM_ARGS() != 1 || 
        getParameters(ht,1,&pParamValue)==FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
    
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
        RETURN_FALSE;

/* -------------------------------------------------------------------- */
/*      build the parm and value arrays.                                */
/*      Note : code extacted from sablot.c (functions related to        */
/*      xslt)                                                           */
/* -------------------------------------------------------------------- */
    ar = HASH_OF(pParamValue);
    if (ar)
    {
        /**
         * Allocate 2 times the number of elements in
         * the array, since with associative arrays in PHP
         * keys are not counted.
         */

        numelems = zend_hash_num_elements(ar);
        size = (numelems * 2 + 1) * sizeof(char *);
            
        papszNameValue = (char **)emalloc(size+1);
        memset((char *)papszNameValue, 0, size);
        
        /**
         * Translate a PHP array (HashTable *) into a 
         * Sablotron array (char **).
         */
        if (_php_extract_associative_array(ar, papszNameValue))
        {
            papszName = (char **)malloc(sizeof(char *)*numelems);
            papszValue = (char **)malloc(sizeof(char *)*numelems);

            for (i=0; i<numelems; i++)
            {
                iIndice = i*2;
                papszName[i] = papszNameValue[iIndice];
                papszValue[i] = papszNameValue[iIndice+1];
            }
        }
        else
        {
            // Failed for some reason
            php_error(E_WARNING, 
                      "processLegendTemplate: failed reading array");
            RETURN_FALSE;
        }
        efree(papszNameValue);
    }

    pszBuffer = mapObj_processLegendTemplate(self, papszName, 
                                             papszValue, numelems);

    msFree(papszName);  // The strings inside the array are just refs
    msFree(papszValue);

    if (pszBuffer)
    {
        RETVAL_STRING(pszBuffer, 1);
        free(pszBuffer);
    }
    else
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_STRING("", 0);
    }
}


/**********************************************************************
 *                        map->processQueryTemplate()
 *
 * Process a template  
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_processQueryTemplate(array_layer_index)*/

DLEXPORT void php3_ms_map_processQueryTemplate(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    pval        *pParamValue, *pGenerateImage;
    mapObj      *self=NULL;
    char        *pszBuffer = NULL;
    int         i, iIndice = 0;
    HashTable   *ar;
    int         numelems=0,  size;
    char        **papszNameValue = NULL;
    char        **papszName = NULL;
    char        **papszValue = NULL;
    int         nGenerateImage = 1;
    int         nArgs = ARG_COUNT(ht);
          

    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }

    if (pThis == NULL ||
        (nArgs != 1 && nArgs != 2))
    {
        WRONG_PARAM_COUNT;
    }

    if (getParameters(ht,nArgs,&pParamValue, &pGenerateImage)==FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
    
    if (nArgs == 2)
    {
        convert_to_long(pGenerateImage);
        nGenerateImage = pGenerateImage->value.lval;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
        RETURN_FALSE;

/* -------------------------------------------------------------------- */
/*      build the parm and value arrays.                                */
/*      Note : code extacted from sablot.c (functions related to        */
/*      xslt)                                                           */
/* -------------------------------------------------------------------- */
    ar = HASH_OF(pParamValue);
    if (ar)
    {
        /**
         * Allocate 2 times the number of elements in
         * the array, since with associative arrays in PHP
         * keys are not counted.
         */

        numelems = zend_hash_num_elements(ar);
        size = (numelems * 2 + 1) * sizeof(char *);
            
        papszNameValue = (char **)emalloc(size+1);
        memset((char *)papszNameValue, 0, size);
        
        /**
         * Translate a PHP array (HashTable *) into a 
         * Sablotron array (char **).
         */
        if (_php_extract_associative_array(ar, papszNameValue))
        {
            papszName = (char **)malloc(sizeof(char *)*numelems);
            papszValue = (char **)malloc(sizeof(char *)*numelems);
            
            
            for (i=0; i<numelems; i++)
            {
                iIndice = i*2;
                papszName[i] = papszNameValue[iIndice];
                papszValue[i] = papszNameValue[iIndice+1];
            }

        }
        else
        {
            // Failed for some reason
            php_error(E_WARNING, 
                      "processLegendTemplate: failed reading array");
            RETURN_FALSE;
        }
        efree(papszNameValue);
    }
    pszBuffer = mapObj_processQueryTemplate(self, nGenerateImage,
                                            papszName, 
                                            papszValue, numelems);

    msFree(papszName);  // The strings inside the array are just refs
    msFree(papszValue);

    if (pszBuffer)
    {
        RETVAL_STRING(pszBuffer, 1);
        free(pszBuffer);
    }
    else
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_STRING("", 0);
    }


}

/**********************************************************************
 *                        map->setSymbolSet(szFileName)
 *
 * Load a symbol file.
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_setSymbolSet(fileName)*/

DLEXPORT void php3_ms_map_setSymbolSet(INTERNAL_FUNCTION_PARAMETERS)
{
#ifdef PHP4
    pval        *pThis;
    pval        *pParamFileName;
    mapObj      *self=NULL;
    int         retVal=0;

#ifdef PHP4
    HashTable   *list=NULL;

#else
    pval        *pValue = NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }

    if (getParameters(ht,1,&pParamFileName) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pParamFileName);
   
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
        RETURN_FALSE;

    if(pParamFileName->value.str.val != NULL && 
       strlen(pParamFileName->value.str.val) > 0)
    {
        if ((retVal = mapObj_setSymbolSet(self, 
                                          pParamFileName->value.str.val)) != 0)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "Failed loading symbolset from %s",
                       pParamFileName->value.str.val);
        }
    }

    if (self->symbolset.filename)
        _phpms_set_property_string(pThis, "symbolsetfilename", 
                                   self->symbolset.filename?
                                       self->symbolset.filename:"", E_ERROR TSRMLS_CC); 

    RETURN_LONG(retVal);
#endif
}

/**********************************************************************
 *                        map->getNumSymbols()
 **********************************************************************/

/* {{{ proto int layer.getNumSymbols()
   Returns the number of sumbols from this map. */

DLEXPORT void php3_ms_map_getNumSymbols(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
    mapObj *self=NULL;
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

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list TSRMLS_CC);
    if (self == NULL)
        RETURN_FALSE;
   
    RETURN_LONG(self->symbolset.numsymbols);
}
/* }}} */

/**********************************************************************
 *                        map->setFontSet(szFileName)
 *
 * Load a new fontset
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_setFontSet(fileName)*/

DLEXPORT void php3_ms_map_setFontSet(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    pval        *pParamFileName;
    mapObj      *self=NULL;
    int         retVal=0;

#ifdef PHP4
    HashTable   *list=NULL;

#else
    pval        *pValue = NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }

    if (getParameters(ht,1,&pParamFileName) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pParamFileName);
   
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
        RETURN_FALSE;

    if(pParamFileName->value.str.val != NULL && 
       strlen(pParamFileName->value.str.val) > 0)
    {
        if ((retVal = mapObj_setFontSet(self, 
                                        pParamFileName->value.str.val)) != 0)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "Failed loading fontset from %s",
                       pParamFileName->value.str.val);
        }
    }


    if (self->fontset.filename)
        _phpms_set_property_string(pThis, "fontsetfilename", 
                                   self->fontset.filename?
                                       self->fontset.filename:"", E_ERROR TSRMLS_CC);

    RETURN_LONG(retVal);
}
/* }}} */

/**********************************************************************
 *                        map->saveMapContext(szFileName)
 *
 * Save mapfile under the Map Context format
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_saveMapContext(fileName)*/

DLEXPORT void php3_ms_map_saveMapContext(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    pval        *pParamFileName;
    mapObj      *self=NULL;
    int         retVal=0;

#ifdef PHP4
    HashTable   *list=NULL;

#else
    pval        *pValue = NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if (getParameters(ht,1,&pParamFileName) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pParamFileName);
   
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if(pParamFileName->value.str.val != NULL && 
       strlen(pParamFileName->value.str.val) > 0)
    {
        if ((retVal = mapObj_saveMapContext(self, 
                                pParamFileName->value.str.val)) != MS_SUCCESS)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_WARNING, "Failed saving map context from %s",
                       pParamFileName->value.str.val);
            RETURN_LONG(MS_FAILURE);
        }
    }

    RETURN_LONG(retVal);
}
/* }}} */

/**********************************************************************
 *                        map->loadMapContext(szFileName)
 *
 * Save mapfile under the Map Context format
 **********************************************************************/

/* {{{ proto int map.php3_ms_map_loadMapContext(fileName)*/

DLEXPORT void php3_ms_map_loadMapContext(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    pval        *pParamFileName;
    pval        *pUniqueLayerName;
    mapObj      *self=NULL;
    int         retVal=0;
    int         nArgs;
    int         bUniqueLayerName = MS_FALSE;

    HashTable   *list=NULL;
    pval        **pExtent;

    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    nArgs = ARG_COUNT(ht);
    if((nArgs != 1 && nArgs != 2) ||
       getParameters(ht,nArgs,&pParamFileName, &pUniqueLayerName) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pParamFileName);
   
    if (nArgs == 2)
    {
        convert_to_long(pUniqueLayerName);
        bUniqueLayerName = pUniqueLayerName->value.lval;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if(pParamFileName->value.str.val != NULL && 
       strlen(pParamFileName->value.str.val) > 0)
    {
        if ((retVal = mapObj_loadMapContext(self, pParamFileName->value.str.val,
                                            bUniqueLayerName)) != MS_SUCCESS)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_WARNING, "Failed loading map context from %s",
                       pParamFileName->value.str.val);
            RETURN_LONG(MS_FAILURE);
        }
    }

    /* read-only properties */
    _phpms_set_property_long(pThis, "numlayers", self->numlayers, E_ERROR TSRMLS_CC);

    /* editable properties */
    if(self->name)
        _phpms_set_property_string(pThis, "name",      self->name, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis,  "status",    self->status, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis,  "width",     self->width, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis,  "height",    self->height, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis,  "transparent", self->transparent, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis,  "interlace", self->interlace, E_ERROR TSRMLS_CC);
    if(self->imagetype)
        _phpms_set_property_string(pThis,"imagetype", self->imagetype,E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis,"imagequality", self->imagequality, E_ERROR TSRMLS_CC);

    if (zend_hash_find(Z_OBJPROP_P(pThis), "extent", sizeof("extent"), 
                       (void *)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double((*pExtent),"minx", self->extent.minx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"miny", self->extent.miny, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxx", self->extent.maxx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pExtent),"maxy", self->extent.maxy, 
                                   E_ERROR TSRMLS_CC);
    }

    _phpms_set_property_double(pThis,"cellsize",  self->cellsize, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis,  "units",     self->units, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis,"scaledenom",self->scaledenom, E_ERROR TSRMLS_CC);
    /* TODO: scale deprecated in v5.0 remove in future release */
    _phpms_set_property_double(pThis,"scale",     self->scaledenom, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis,  "resolution",self->resolution, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis,  "defresolution",self->defresolution, E_ERROR TSRMLS_CC);
    if(self->shapepath)
        _phpms_set_property_string(pThis, "shapepath",self->shapepath,E_ERROR TSRMLS_CC);


    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        map->selectoutputformat(type)
 *
 * Selects the output format (to be used in the map given) by giving
 * the type.
 **********************************************************************/
DLEXPORT void php3_ms_map_selectOutputFormat(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    pval        *pImageType;
    mapObj      *self=NULL;
    HashTable   *list=NULL;
    int         nStatus = MS_SUCCESS;
    pval        *new_obj_ptr;

    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if (getParameters(ht,1,&pImageType) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }
    
    convert_to_string(pImageType);
   
    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if ((nStatus = mapObj_selectOutputFormat(self, 
                                 pImageType->value.str.val)) != MS_SUCCESS)

    {
        php3_error(E_WARNING, "Unable to set output format to '%s'", 
                   pImageType->value.str.val);
    }
    else
    {
        if(self->imagetype)
          _phpms_set_property_string(pThis,"imagetype", self->imagetype,E_ERROR TSRMLS_CC);
  

        zend_hash_del(Z_OBJPROP_P(pThis), "outputformat", 
                      sizeof("outputformat"));
      
        MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
        _phpms_build_outputformat_object(self->outputformat, list, new_obj_ptr TSRMLS_CC);
        _phpms_add_property_object(pThis, "outputformat", new_obj_ptr, E_ERROR TSRMLS_CC);
        
    }

    RETURN_LONG(nStatus);
}

/**********************************************************************
 *                        map->applySLD(szSLDString)
 *
 * Apply an XML SLD string to the map file
 **********************************************************************/
DLEXPORT void php3_ms_map_applySLD(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     pval        *pSLDString;
     mapObj      *self=NULL;
      HashTable   *list=NULL;
     int         nStatus = MS_SUCCESS;

     pThis = getThis();

     if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if (getParameters(ht,1,&pSLDString) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pSLDString);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    nStatus = mapObj_applySLD(self, pSLDString->value.str.val);

    RETURN_LONG(nStatus);
}


/**********************************************************************
 *                        map->applySLD(szSLDURL)
 *
 * Apply an SLD to map. The SLD is given as an URL.
 **********************************************************************/
DLEXPORT void php3_ms_map_applySLDURL(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     pval        *pSLDString;
     mapObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_SUCCESS;

     pThis = getThis();

     if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if (getParameters(ht,1,&pSLDString) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pSLDString);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    nStatus = mapObj_applySLDURL(self, pSLDString->value.str.val);

    RETURN_LONG(nStatus);
}


/**********************************************************************
 *                        map->generateSLD()
 *
 * Generates an sld based on the map layers/class
 **********************************************************************/
DLEXPORT void php3_ms_map_generateSLD(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     mapObj      *self=NULL;
     HashTable   *list=NULL;
     char       *pszBuffer = NULL;

     pThis = getThis();

     if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    pszBuffer = mapObj_generateSLD(self);

    if (pszBuffer)
    {
        RETVAL_STRING(pszBuffer, 1);
        free(pszBuffer);
    }
    else
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_STRING("", 0);
    }
}





/************************************************************************/
/*                       php3_ms_map_getConfigOption                    */
/*                                                                      */
/*      returns the configuation value associated with the key          */
/*      passed as argument. Returns an empty string on error.           */
/*      prototype : value = $map->getconfigoption(key)                  */
/************************************************************************/
DLEXPORT void php3_ms_map_getConfigOption(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     pval        *pKey;
     mapObj      *self=NULL;
     HashTable   *list=NULL;
     char       *pszValue = NULL;
 
     pThis = getThis();

     if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if (getParameters(ht,1,&pKey) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pKey);


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    
    if (self == NULL || 
        (pszValue = (char *)msGetConfigOption(self,pKey->value.str.val )) == NULL)
    {
        RETURN_STRING("", 1);
    }
    else
    {
        RETURN_STRING(pszValue, 1);
    }
}



DLEXPORT void php3_ms_map_setConfigOption(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     pval        *pKey;
     pval        *pConfig;
     mapObj      *self=NULL;
     HashTable   *list=NULL;
 
     pThis = getThis();

     if (pThis == NULL)
     {
         RETURN_LONG(MS_FAILURE);
     }

     if (getParameters(ht,2,&pKey, &pConfig) == FAILURE)
     {
         WRONG_PARAM_COUNT;
     }

     convert_to_string(pKey);
     convert_to_string(pConfig);


     self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                          list TSRMLS_CC);
     if (self != NULL)
     {
         msSetConfigOption(self,pKey->value.str.val,pConfig->value.str.val);
    
         RETURN_LONG(MS_SUCCESS);
     }
   
     RETURN_LONG(MS_FAILURE);
}



DLEXPORT void php3_ms_map_applyConfigOptions(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     mapObj      *self=NULL;
     HashTable   *list=NULL;
 
     pThis = getThis();

     if (pThis == NULL)
     {
         RETURN_LONG(MS_FAILURE);
     }

     self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

     if (self != NULL)
     {
         msApplyMapConfigOptions(self);
         RETURN_LONG(MS_SUCCESS);
     }
   
     RETURN_LONG(MS_FAILURE);
}



/************************************************************************/
/*                         php3_ms_map_OWSDispatch                      */
/*                                                                      */
/*      Dispatch a owsrequest through the wxs services.                 */
/************************************************************************/
DLEXPORT void php3_ms_map_OWSDispatch(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pRequest;
     mapObj      *self=NULL;
     HashTable   *list=NULL;
     cgiRequestObj *pCgiRequest = NULL;
     int nReturn = 0;

     pThis = getThis();

     if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }


     if (getParameters(ht,1,&pRequest) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    pCgiRequest = (cgiRequestObj *)_phpms_fetch_handle(pRequest, PHPMS_GLOBAL(le_mscgirequest), 
                                                       list TSRMLS_CC);
    if (pCgiRequest == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    nReturn = mapObj_OWSDispatch(self, pCgiRequest);

    RETURN_LONG(nReturn);
}


DLEXPORT void php3_ms_map_loadOWSParameters(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pRequest, *pVersion;
     mapObj      *self=NULL;
     HashTable   *list=NULL;
     cgiRequestObj *pCgiRequest = NULL;
     int nArgs;
     char *pszVersion = NULL;
      int nReturn = 0;

     pThis = getThis();

     if (pThis == NULL)
     {
       RETURN_LONG(MS_FAILURE);
     }


     nArgs = ARG_COUNT(ht);
     if ((nArgs != 1 && nArgs != 2) ||
         getParameters(ht,nArgs,&pRequest, &pVersion) != SUCCESS)
     {
       WRONG_PARAM_COUNT;
     }

     if (nArgs >= 2)
     {
       convert_to_string(pVersion);
       pszVersion =  strdup(pVersion->value.str.val);
     }
     else
       pszVersion = strdup("1.1.1");
     
     self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
     if (self == NULL)
     {
        RETURN_LONG(MS_FAILURE);
    }

     pCgiRequest = (cgiRequestObj *)_phpms_fetch_handle(pRequest, 
                                                        PHPMS_GLOBAL(le_mscgirequest), 
                                                       list TSRMLS_CC);
     if (pCgiRequest == NULL)
     {
       RETURN_LONG(MS_FAILURE);
     }

     nReturn = mapObj_loadOWSParameters(self, pCgiRequest, pszVersion);

     msFree(pszVersion);

     RETURN_LONG(nReturn);
}




/**********************************************************************
 *                        map->insertLayer(layerobj)
 * 
 **********************************************************************/

/* {{{ proto int map.insertLayer(layerObj layer, index)
   Returns the index where the layer had been inserted*/

DLEXPORT void php3_ms_map_insertLayer(INTERNAL_FUNCTION_PARAMETERS)
{ 
  pval  *pLyrIndex, *pLyr, *pThis;
  mapObj *self=NULL;
  layerObj *poLayer=NULL;
  int nLyrIndex = -1, iReturn=-1;
  int nArgs;

#ifdef PHP4
    HashTable   *list=NULL;
#endif
    nArgs = ARG_COUNT(ht);

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        (nArgs != 1 && nArgs != 2) ||
        getParameters(ht, nArgs, &pLyr, &pLyrIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
    

    if (nArgs == 2)
    {
        convert_to_long(pLyrIndex);
        nLyrIndex = pLyrIndex->value.lval;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    poLayer = (layerObj *)_phpms_fetch_handle(pLyr, 
                                              PHPMS_GLOBAL(le_mslayer),
                                              list TSRMLS_CC); 
    if (self == NULL || poLayer == NULL ||
        (iReturn =  mapObj_insertLayer(self, poLayer, nLyrIndex) ) < 0)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* Update mapObj members */
    _phpms_set_property_long(pThis, "numlayers",
                             self->numlayers, E_ERROR TSRMLS_CC); 

    
    /* Return layer object */
    RETURN_LONG(iReturn);
}
/* }}} */

/**********************************************************************
 *                        map->removelayer()
 *
 * Remove layer from map object
 **********************************************************************/

/* {{{ proto int map.removeLayer(int layer_index)
   Returns layerObj removed on sucess, else null. */
 
DLEXPORT void php3_ms_map_removeLayer(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
    long  layerIndex = 0;
    mapObj *self=NULL;
    layerObj *poLayer=NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &layerIndex) 
         == FAILURE))
    {
       return;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);
    
    if (self == NULL ||
        (poLayer = mapObj_removeLayer(self, layerIndex)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

     /* Update mapObj members */
    _phpms_set_property_long(pThis, "numlayers",
                             self->numlayers, E_ERROR TSRMLS_CC); 

     /* Return layer object */
    _phpms_build_layer_object(poLayer, (int)NULL, list, return_value TSRMLS_CC);
    
}
/* }}} */

/**********************************************************************
 *                        map->setCenter()
 *
 * Set the map center to the given map point. 
 **********************************************************************/

/* {{{ proto int map.setCenter(pointObj center)
   Returns MS_SUCCESS or MS_FAILURE */
 
DLEXPORT void php3_ms_map_setcenter(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis, *pPoint;
    mapObj *self=NULL;
    pointObj *poPoint;
    HashTable   *list=NULL;
    int nStatus = MS_FAILURE;

    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &pPoint) 
         == FAILURE))
    {
       return;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list TSRMLS_CC);

    if (self && poPoint &&
        (nStatus=mapObj_setCenter(self, poPoint) != MS_SUCCESS))
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
    
}
/* }}} */

/**********************************************************************
 *                        map->offsetExtent()
 *
 * Offset the map extent based on the given distances in map coordinates.
 **********************************************************************/

/* {{{ proto int map.offsetExtend(float x, float y)
   Returns MS_SUCCESS or MS_FAILURE */
 
DLEXPORT void php3_ms_map_offsetextent(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
    mapObj *self=NULL;
    HashTable   *list=NULL;
    int nStatus = MS_FAILURE;
    double x, y;

    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "dd", &x, &y) 
         == FAILURE))
    {
       return;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    if (self &&
        (nStatus=mapObj_offsetExtent(self, x, y) != MS_SUCCESS))
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
    
}
/* }}} */

/**********************************************************************
 *                        map->scaleExtent()
 *
 * Scale the map extent using the zoomfactor and ensure the extent within
 * the minscaledenom and maxscaledenom domain. If minscaledenom and/or
 * maxscaledenom is 0 then the parameter is not taken into account.
 **********************************************************************/

/* {{{ proto int map.scaleExtend(double zoomfactor, double minscaledenom,
                                 double maxscaledenom) 
   Returns MS_SUCCESS or MS_FAILURE */
 
DLEXPORT void php3_ms_map_scaleextent(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
    mapObj *self=NULL;
    HashTable   *list=NULL;
    int nStatus = MS_FAILURE;
    double zoomfactor, minscaledenom, maxscaledenom;

    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ddd", &zoomfactor, &minscaledenom, &maxscaledenom) 
         == FAILURE))
    {
       return;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), 
                                         list TSRMLS_CC);

    if (self &&
        (nStatus=mapObj_scaleExtent(self, zoomfactor, minscaledenom, maxscaledenom) != MS_SUCCESS))
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(nStatus);
    
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - image class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_img_object()
 **********************************************************************/
static long _phpms_build_img_object(imageObj *im, webObj *pweb,
                                    HashTable *list, pval *return_value TSRMLS_DC)
{
    int img_id;

    if (im == NULL)
        return 0;

    img_id = php3_list_insert(im, PHPMS_GLOBAL(le_msimg));

    _phpms_object_init(return_value, img_id, php_img_class_functions,
                       PHP4_CLASS_ENTRY(img_class_entry_ptr) TSRMLS_CC);

    /* width/height params are read-only */
    add_property_long(return_value, "width", im->width);
    add_property_long(return_value, "height", im->height);
    
    PHPMS_ADD_PROP_STR(return_value, "imagepath", im->imagepath);
    PHPMS_ADD_PROP_STR(return_value, "imageurl", im->imageurl);
    

    PHPMS_ADD_PROP_STR(return_value, "imagetype", im->format->name);

    return img_id;
}

/**********************************************************************
 *                        image->saveImage()
 *
 *       saveImage(string filename)
 **********************************************************************/

/* {{{ proto int img.saveImage(string filename, Map $oMap)
   Writes image object to specifed filename.  If filename is empty then write to stdout.  Returns -1 on error. Second aregument oMap is not manadatory. It 
 is usful when saving to other formats like GTIFF to get georeference infos.*/

DLEXPORT void php3_ms_img_saveImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pFname, *pThis, *pMapObj;
    imageObj    *im = NULL;
    int         retVal = 0;
    int         nArgs;
    mapObj      *poMap = NULL;
    char        *pImagepath = NULL;
    char        *pBuf = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    nArgs = ARG_COUNT(ht);
    if (pThis == NULL || (nArgs != 1 && nArgs != 2) ||
        getParameters(ht, nArgs, &pFname, &pMapObj) != SUCCESS  )
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFname);
    if (nArgs == 2)
    {
        poMap = (mapObj*)_phpms_fetch_handle(pMapObj, 
                                             PHPMS_GLOBAL(le_msmap),
                                             list TSRMLS_CC);
    }


    im = (imageObj *)_phpms_fetch_handle(pThis, le_msimg, list TSRMLS_CC);
    pImagepath = _phpms_fetch_property_string(pThis, "imagepath", E_ERROR TSRMLS_CC);
    
    if(pFname->value.str.val != NULL && strlen(pFname->value.str.val) > 0)
    {
        if (im == NULL ||
            (retVal = msSaveImage(poMap, im, pFname->value.str.val) != 0))
        {
          _phpms_report_mapserver_error(E_WARNING);
          php3_error(E_ERROR, "Failed writing image to %s", 
                     pFname->value.str.val);
        }
    }
    else
    {           /* no filename - stdout */
        int size=0;
        int   b;
        FILE *tmp = NULL;
        char  buf[4096];
        void *iptr=NULL;

        retVal = 0;


        /* if there is no output buffer active, set the header */ 
        if (OG(ob_nesting_level)<=0) 
        { 
            php_header(TSRMLS_C); 
        } 

        if( MS_DRIVER_GD(im->format) || MS_DRIVER_AGG(im->format))
          iptr = (void *)msSaveImageBuffer(im, &size, im->format);
        else if (im->format->name && strcasecmp(im->format->name, "imagemap")==0)
        {
            iptr = im->img.imagemap;
	    size = strlen(im->img.imagemap);
        }
        else if (MS_DRIVER_SVG(im->format))
        {
            retVal = -1;
            
            if (pImagepath)
            {
                pBuf = msTmpFile(NULL, pImagepath, "svg");
                
                tmp = fopen(pBuf, "w");
            }
            //tmp = tmpfile(); 
            if (tmp == NULL) 
            {
                 _phpms_report_mapserver_error(E_WARNING);
                php3_error(E_ERROR, "Unable to open temporary file for SVG output.");
                retVal = -1;
            }
            if (msSaveImagetoFpSVG(im, tmp) == MS_SUCCESS)
            {
                fclose(tmp);
                tmp = fopen(pBuf, "r");
                //fseek(tmp, 0, SEEK_SET);
                while ((b = fread(buf, 1, sizeof(buf), tmp)) > 0) 
                {
                    php_write(buf, b TSRMLS_CC);
                }
                fclose(tmp);
                retVal = 1;
            }
            else
            {
                _phpms_report_mapserver_error(E_WARNING);
                php3_error(E_ERROR, "Unable to open temporary file for SVG output.");
                retVal = -1;
            } 

            RETURN_LONG(retVal);
        }   
        if (size == 0) {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "Failed writing image to stdout");
            retVal = -1;
        } 
        else
        {
            php_write(iptr, size TSRMLS_CC);
            retVal = size;
            gdFree(iptr);
        }


    }

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        image->saveWebImage()
 *       saveWebImage()
 **********************************************************************/

/* {{{ proto int img.saveWebImage(int type, int transparent, int interlace, int quality)
   Writes image to temp directory.  Returns image URL. */

DLEXPORT void php3_ms_img_saveWebImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval   *pThis;

    imageObj *im = NULL;

    char *pImagepath, *pImageurl, *pTmpfname, *pImagefile,*pImageurlfull;
    const char *pszImageExt;
    char szPath[MS_MAXPATHLEN];

    HashTable   *list=NULL;
    pThis = getThis();

    if (pThis == NULL)
    {
        WRONG_PARAM_COUNT;
    }

    im = (imageObj *)_phpms_fetch_handle(pThis, le_msimg, list TSRMLS_CC);
    pImagepath = _phpms_fetch_property_string(pThis, "imagepath", E_ERROR TSRMLS_CC);
    pImageurl = _phpms_fetch_property_string(pThis, "imageurl", E_ERROR TSRMLS_CC);

    pszImageExt = im->format->extension;

    pTmpfname = msTmpFile(NULL,NULL,pszImageExt);

    /* Save the image... 
     */
    pImagefile = msBuildPath(szPath,pImagepath,pTmpfname);
    if (im == NULL || 
        msSaveImage(NULL, im, pImagefile) != 0 )
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed writing image to %s", 
                   pImagefile);
    }

    /* ... and return the corresponding URL
     */
    pImageurlfull = msBuildPath(szPath,pImageurl,pTmpfname);
    msFree(pTmpfname);
    
    RETURN_STRING(pImageurlfull, 1);
}
/* }}} */


/**********************************************************************
 *                        image->pasteImage()
 **********************************************************************/

/* {{{ proto void img.pasteImage(imageObj Src, int transparentColor [[,int dstx, int dsty], int angle])
   Pastes another imageObj on top of this imageObj. transparentColor is
   the color (0xrrggbb) from srcImg that should be considered transparent.
   Pass transparentColor=-1 if you don't want any transparent color.
   If optional dstx,dsty are provided then they define the position where the
   image should be copied (dstx,dsty = top-left corner position).
   The optional angle is a value between 0 and 360 degrees to rotate the
   source image counterclockwise.  Note that if a rotation is requested then
   the dstx and dsty coordinates specify the CENTER of the destination area.
   NOTE : this function only works for 8 bits GD images.
*/

DLEXPORT void php3_ms_img_pasteImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval   *pSrcImg, *pTransparent, *pThis, *pDstX, *pDstY, *pAngle;
    imageObj *imgDst = NULL, *imgSrc = NULL;
    int         nDstX=0, nDstY=0, nAngle=0, bAngleSet=MS_FALSE;
    int         nArgs = ARG_COUNT(ht);
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        (nArgs != 2 && nArgs != 4 && nArgs != 5))
    {
        WRONG_PARAM_COUNT;
    }
    if (pThis == NULL ||
        getParameters(ht, nArgs, &pSrcImg, &pTransparent, 
                      &pDstX, &pDstY, &pAngle) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    imgDst = (imageObj *)_phpms_fetch_handle(pThis, le_msimg, list TSRMLS_CC);

    imgSrc = (imageObj *)_phpms_fetch_handle(pSrcImg, PHPMS_GLOBAL(le_msimg), 
                                             list TSRMLS_CC);
    
    if( (!MS_DRIVER_GD(imgSrc->format) && !MS_DRIVER_AGG(imgSrc->format)) ||  
 	(!MS_DRIVER_GD(imgDst->format) && !MS_DRIVER_AGG(imgDst->format))) 
      {
        php3_error(E_ERROR, "PasteImage function should only be used with GD or AGG images.");
        RETURN_LONG(-1);
    }
      
#ifdef USE_AGG
    if( MS_RENDERER_AGG(imgSrc->format)) 
      msAlphaAGG2GD(imgSrc); 
    if( MS_RENDERER_AGG(imgDst->format)) 
      msAlphaAGG2GD(imgDst); 
#endif

    convert_to_long(pTransparent);

    if (nArgs >= 4)
    {
        convert_to_long(pDstX);
        convert_to_long(pDstY);
        nDstX = pDstX->value.lval;
        nDstY = pDstY->value.lval;
    }

    if (nArgs == 5)
    {
        convert_to_long(pAngle);
        nAngle = pAngle->value.lval;
        bAngleSet = MS_TRUE;
    }

    if (imgSrc != NULL && imgDst != NULL)
    {
        int    nOldTransparentColor, nNewTransparentColor=-1, nR, nG, nB;

        /* Look for r,g,b in color table and make it transparent.
         * will return -1 if there is no exact match which will result in
         * no transparent color in the call to gdImageColorTransparent().
         */
        if (pTransparent->value.lval != -1)
        {
            nR = (pTransparent->value.lval / 0x010000) & 0xff;
            nG = (pTransparent->value.lval / 0x0100) & 0xff;
            nB = pTransparent->value.lval & 0xff;
            nNewTransparentColor = gdImageColorExact(imgSrc->img.gd, nR,nG,nB);
        }

        nOldTransparentColor = gdImageGetTransparent(imgSrc->img.gd);
        gdImageColorTransparent(imgSrc->img.gd, nNewTransparentColor);

        if (!bAngleSet)
            gdImageCopy(imgDst->img.gd, imgSrc->img.gd, nDstX, nDstY, 
                        0, 0, imgSrc->img.gd->sx, imgSrc->img.gd->sy);
        else
            gdImageCopyRotated(imgDst->img.gd, imgSrc->img.gd, nDstX, nDstY, 
                               0, 0, imgSrc->img.gd->sx, imgSrc->img.gd->sy,
                               nAngle);

        gdImageColorTransparent(imgSrc->img.gd, nOldTransparentColor);
    }
    else
    {
        php3_error(E_ERROR, "Source or destination image is invalid.");
    }

    RETURN_LONG(0);
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
    imageObj *self;

    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (imageObj*)_phpms_fetch_handle(pThis, le_msimg, list TSRMLS_CC);
    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval **phandle;
        if (zend_hash_find(Z_OBJPROP_P(pThis), "_handle_", 
                           sizeof("_handle_"), 
                           (void *)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
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
                                      HashTable *list, pval *return_value TSRMLS_DC)
{
    int         layer_id;
    pval        *new_obj_ptr;

    if (player == NULL)
    {
        return 0;
    }

    layer_id = php3_list_insert(player, PHPMS_GLOBAL(le_mslayer));

    _phpms_object_init(return_value, layer_id, php_layer_class_functions,
                       PHP4_CLASS_ENTRY(layer_class_entry_ptr) TSRMLS_CC);

    if (parent_map_id != (int)NULL)
       zend_list_addref(parent_map_id);
    add_property_resource(return_value, "_map_handle_", parent_map_id);
    MS_REFCNT_INCR(player);

    /* read-only properties */
    add_property_long(return_value,   "numclasses", player->numclasses);
    add_property_long(return_value,   "index",      player->index);

    /* editable properties */
    add_property_long(return_value,   "status",     player->status);
    add_property_long(return_value,   "debug",      player->debug);
    PHPMS_ADD_PROP_STR(return_value,  "classitem",  player->classitem);
    PHPMS_ADD_PROP_STR(return_value,  "classgroup",  player->classgroup);
    PHPMS_ADD_PROP_STR(return_value,  "name",       player->name);
    PHPMS_ADD_PROP_STR(return_value,  "group",      player->group);
    PHPMS_ADD_PROP_STR(return_value,  "data",       player->data);
    add_property_long(return_value,   "type",       player->type);
    add_property_long(return_value,   "dump",       player->dump);
    add_property_double(return_value, "tolerance",  player->tolerance);
    add_property_long(return_value,   "toleranceunits",player->toleranceunits);
    add_property_long(return_value,   "sizeunits",  player->sizeunits);

    add_property_double(return_value, "symbolscaledenom",player->symbolscaledenom);
    add_property_double(return_value, "minscaledenom",   player->minscaledenom);
    add_property_double(return_value, "maxscaledenom",   player->maxscaledenom);
    add_property_double(return_value, "labelminscaledenom",player->labelminscaledenom);
    add_property_double(return_value, "labelmaxscaledenom",player->labelmaxscaledenom);

    /* TODO: *scale deprecated in v5.0. Remove in future release */
    add_property_double(return_value, "symbolscale",player->symbolscaledenom);
    add_property_double(return_value, "minscale",   player->minscaledenom);
    add_property_double(return_value, "maxscale",   player->maxscaledenom);
    add_property_double(return_value, "labelminscale",player->labelminscaledenom);
    add_property_double(return_value, "labelmaxscale",player->labelmaxscaledenom);

    add_property_long(return_value,   "maxfeatures",player->maxfeatures);
    add_property_long(return_value,   "transform",  player->transform);
    add_property_long(return_value,   "labelcache", player->labelcache);
    add_property_long(return_value,   "postlabelcache",player->postlabelcache);
    PHPMS_ADD_PROP_STR(return_value,  "labelitem",  player->labelitem);
    PHPMS_ADD_PROP_STR(return_value,  "tileitem",   player->tileitem);
    PHPMS_ADD_PROP_STR(return_value,  "tileindex",  player->tileindex);
    PHPMS_ADD_PROP_STR(return_value,  "header",     player->header);
    PHPMS_ADD_PROP_STR(return_value,  "footer",     player->footer);
    PHPMS_ADD_PROP_STR(return_value,  "connection", player->connection);
    add_property_long(return_value,   "connectiontype",player->connectiontype);
    PHPMS_ADD_PROP_STR(return_value,  "filteritem", player->filteritem);
    PHPMS_ADD_PROP_STR(return_value,  "template", player->template);
    add_property_long(return_value,   "opacity", player->opacity);
    /* TODO: transparency deprecated in v5.0. Remove in future release */
    add_property_long(return_value,   "transparency", player->opacity);
    PHPMS_ADD_PROP_STR(return_value,  "styleitem",  player->styleitem);
    add_property_long(return_value,   "num_processing",player->numprocessing);
    PHPMS_ADD_PROP_STR(return_value,  "requires",   player->requires);
    PHPMS_ADD_PROP_STR(return_value,  "labelrequires", player->labelrequires);

    MAKE_STD_ZVAL(new_obj_ptr); 
    _phpms_build_projection_object(&(player->projection), PHPMS_GLOBAL(le_msprojection_ref),
                                   list,  new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "projection", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);
    _phpms_build_color_object(&(player->offsite),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "offsite", new_obj_ptr, E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);
    _phpms_build_hashtable_object(&(player->metadata),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "metadata",new_obj_ptr,E_ERROR TSRMLS_CC);

    if (player->connectiontype == MS_GRATICULE && 
        player->layerinfo != NULL)
    {
        MAKE_STD_ZVAL(new_obj_ptr);
         _phpms_build_grid_object((graticuleObj *)(player->layerinfo),
                                  layer_id,
                                  list, new_obj_ptr TSRMLS_CC);
         _phpms_add_property_object(return_value, "grid", new_obj_ptr, E_ERROR TSRMLS_CC);

    }
    return layer_id;
}


/**********************************************************************
 *                        ms_newLayerObj()
 **********************************************************************/

/* {{{ proto layerObj ms_newLayerObj(mapObj map)
   Create a new layer in the specified map. */

DLEXPORT void php3_ms_lyr_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pMapObj, *pSrcLayer;
    mapObj      *parent_map;
    layerObj    *pNewLayer;
    layerObj    *poSrcLayer = NULL;
    int         map_id;
    int         nArgs;
    int         nOrigIndex = 0;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

    nArgs = ARG_COUNT(ht);
    if ((nArgs != 1 && nArgs != 2) ||
        getParameters(ht, nArgs, &pMapObj, &pSrcLayer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_map = (mapObj*)_phpms_fetch_handle(pMapObj, 
                                              PHPMS_GLOBAL(le_msmap),
                                              list TSRMLS_CC);

    if (nArgs == 2)
    {
        poSrcLayer = (layerObj *)_phpms_fetch_handle(pSrcLayer, 
                                                     PHPMS_GLOBAL(le_mslayer),
                                                     list TSRMLS_CC);
    }
        
    if (parent_map == NULL ||
        (pNewLayer = layerObj_new(parent_map)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }
    /* if a layer is passed as argument, copy the layer into
       the new one */
    if (poSrcLayer)
    {
        nOrigIndex = pNewLayer->index;
        msCopyLayer(pNewLayer, poSrcLayer);
        pNewLayer->index = nOrigIndex;
    }

    /* Update mapObj members */
    _phpms_set_property_long(pMapObj, "numlayers",
                             parent_map->numlayers, E_ERROR TSRMLS_CC); 

    /* We will store a reference to the parent_map object id inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_resource(pMapObj, "_handle_", E_ERROR TSRMLS_CC);

    /* Return layer object */
    _phpms_build_layer_object(pNewLayer, map_id, list, return_value TSRMLS_CC);
}
/* }}} */

/**********************************************************************
 *                        layer->updateFromString()
 **********************************************************************/

/* {{{ proto int layer.updateFromString(string snippet)
   Update a layer from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_lyr_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = layerObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
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

    self = (layerObj *)_phpms_fetch_handle(pThis, le_mslayer, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_LONG(       "status",     self->status)
    else IF_SET_LONG(  "debug",      self->debug)
    else IF_SET_STRING("classitem",  self->classitem)
    else IF_SET_STRING("name",       self->name)
    else IF_SET_STRING("group",      self->group)
    else IF_SET_STRING("data",       self->data)
    else IF_SET_LONG(  "type",       self->type)
    else IF_SET_LONG(  "dump",       self->dump)
    else IF_SET_DOUBLE("tolerance",  self->tolerance)
    else IF_SET_LONG(  "toleranceunits",self->toleranceunits)
    else IF_SET_LONG(  "sizeunits",  self->sizeunits)
    else IF_SET_DOUBLE("symbolscaledenom",self->symbolscaledenom)
    else IF_SET_DOUBLE("minscaledenom",   self->minscaledenom)
    else IF_SET_DOUBLE("maxscaledenom",   self->maxscaledenom)
    else IF_SET_DOUBLE("labelminscaledenom",self->labelminscaledenom)
    else IF_SET_DOUBLE("labelmaxscaledenom",self->labelmaxscaledenom)
    /* TODO: *scale deprecated in v5.0. Remove in future release */
    else IF_SET_DOUBLE("symbolscale",self->symbolscaledenom)
    else IF_SET_DOUBLE("minscale",   self->minscaledenom)
    else IF_SET_DOUBLE("maxscale",   self->maxscaledenom)
    else IF_SET_DOUBLE("labelminscale",self->labelminscaledenom)
    else IF_SET_DOUBLE("labelmaxscale",self->labelmaxscaledenom)

    else IF_SET_LONG(  "maxfeatures",self->maxfeatures)
    else IF_SET_LONG(  "transform",  self->transform)
    else IF_SET_LONG(  "labelcache", self->labelcache)
    else IF_SET_LONG(  "postlabelcache", self->postlabelcache)
    else IF_SET_STRING("labelitem",  self->labelitem)
    else IF_SET_STRING("tileitem",   self->tileitem)
    else IF_SET_STRING("tileindex",  self->tileindex)
    else IF_SET_STRING("header",     self->header)
    else IF_SET_STRING("footer",     self->footer)
    else IF_SET_STRING("connection", self->connection)
    else IF_SET_STRING("filteritem", self->filteritem)
    else IF_SET_STRING("template",   self->template)
    else IF_SET_LONG(  "opacity", self->opacity)
    /* TODO: transparency deprecated in v5.0. Remove in future release */
    else IF_SET_LONG(  "transparency",self->opacity)
    else IF_SET_STRING("styleitem",  self->styleitem)
    else IF_SET_STRING("requires",   self->requires)
    else IF_SET_STRING("labelrequires",   self->labelrequires)
    else IF_SET_STRING("classgroup",   self->classgroup)
    else if (strcmp( "connectiontype", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR, "Property 'connectiontype' must be set "
                            "using setConnectionType().");
        RETURN_LONG(-1);
    }
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
    imageObj *im = NULL;
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

    im = (imageObj *)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);
   
    self = (layerObj *)_phpms_fetch_handle(pThis, 
                                           PHPMS_GLOBAL(le_mslayer),list TSRMLS_CC);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

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
    imageObj *im = NULL;
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

    im = (imageObj *)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);
   
    self = (layerObj *)_phpms_fetch_handle(pThis, 
                                           PHPMS_GLOBAL(le_mslayer),list TSRMLS_CC);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

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
    int layer_id, map_id;
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
                                           list TSRMLS_CC);
    if (self == NULL || 
        (newClass = layerObj_getClass(self, pClassIndex->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* We will store a reference to the parent object id (this) inside
     * the class obj.
     */
    layer_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR TSRMLS_CC);

    /* We will store a reference to the map parent object id (this) inside
     * the class obj.
     */
    map_id = _phpms_fetch_property_resource(pThis, "_map_handle_", E_ERROR TSRMLS_CC);

    /* Return layer object */
    _phpms_build_class_object(newClass, map_id, layer_id, list, 
                              return_value TSRMLS_CC);
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
    pval   *pThis, *pType, *pQItem, *pQString;
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
        getParameters(ht, 3, &pQItem, &pQString, &pType) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);
    convert_to_string(pQItem);
    convert_to_string(pQString);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC,E_ERROR);

    if (self && parent_map &&
        (nStatus=layerObj_queryByAttributes(self, parent_map,
                                            pQItem->value.str.val,
                                            pQString->value.str.val,
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
                                           list TSRMLS_CC);
    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list TSRMLS_CC);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

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
                                           list TSRMLS_CC);
    poRect = (rectObj *)_phpms_fetch_handle2(pRect,
                                             PHPMS_GLOBAL(le_msrect_ref),
                                             PHPMS_GLOBAL(le_msrect_new),
                                             list TSRMLS_CC);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

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
                                           list TSRMLS_CC);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

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
                                           list TSRMLS_CC);
    poShape = 
        (shapeObj *)_phpms_fetch_handle2(pShape, 
                                         PHPMS_GLOBAL(le_msshape_ref),
                                         PHPMS_GLOBAL(le_msshape_new),
                                         list TSRMLS_CC);

    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

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

    HashTable   *list=NULL;
    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pFilterString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }
    convert_to_string(pFilterString);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
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
 *                        layer->getFilterString()
 **********************************************************************/

/* {{{ proto string layer.getFilterString()
    Return the layer's filter expression. Returns FALSE on error. */

DLEXPORT void php3_ms_lyr_getFilterString(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj    *self;
    pval        *pThis = NULL;
    char        *pszFilterString = NULL;

    HashTable   *list=NULL;
    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    ;
    if ((pszFilterString = layerObj_getFilter(self)) == NULL)
    {
        RETURN_FALSE;
    }
    else
    {
        RETVAL_STRING(pszFilterString, 1);
        free(pszFilterString);
    }
}

/**********************************************************************
 *                        layer->setProjection()
 **********************************************************************/

/* {{{ proto int layer.setProjection(string projection)
   Set projection and coord. system for the layer. Returns -1 on error. */

DLEXPORT void php3_ms_lyr_setProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pProjString;
    pval   *pThis, *new_obj_ptr;
    int     nStatus = 0;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pProjString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pProjString);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    if (self == NULL || 
        (nStatus = layerObj_setProjection(self, 
                                          pProjString->value.str.val)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    /* update the php projection object */
    zend_hash_del(Z_OBJPROP_P(pThis), "projection", 
                  sizeof("projection"));
    
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_projection_object(&(self->projection), PHPMS_GLOBAL(le_msprojection_ref),
                                   list,  new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(pThis, "projection", new_obj_ptr, E_ERROR TSRMLS_CC);
    

    RETURN_LONG(nStatus);
}
/* }}} */


/**********************************************************************
 *                        layer->setWKTProjection()
 **********************************************************************/

/* {{{ proto int layer.setWKTProjection(string projection)
   Set projection and coord. system for the layer. Returns -1 on error. */

DLEXPORT void php3_ms_lyr_setWKTProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pProjString;
    pval   *pThis, *new_obj_ptr;
    int     nStatus = 0;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pProjString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pProjString);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    if (self == NULL || 
        (nStatus = layerObj_setWKTProjection(self, 
                                          pProjString->value.str.val)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    /* update the php projection object */
    zend_hash_del(Z_OBJPROP_P(pThis), "projection", 
                  sizeof("projection"));
    
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_projection_object(&(self->projection), PHPMS_GLOBAL(le_msprojection_ref),
                                   list,  new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(pThis, "projection", new_obj_ptr, E_ERROR TSRMLS_CC);

    RETURN_LONG(nStatus);
}
/* }}} */


/**********************************************************************
 *                        layer->getProjection()
 **********************************************************************/

/* {{{ proto int layer.getProjection()
    Return the projection string of the layer. Returns FALSE on error. */

DLEXPORT void php3_ms_lyr_getProjection(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj    *self;
    pval        *pThis = NULL;
    char        *pszPojString = NULL;

    HashTable   *list=NULL;
    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    if (self == NULL)
    {
        RETURN_FALSE;
    }

    pszPojString = layerObj_getProjection(self);
    if (pszPojString == NULL)
    {
        RETURN_FALSE;
    }
    else
    {
        RETVAL_STRING(pszPojString, 1);
        free(pszPojString);
    }
}


/**********************************************************************
 *                        layer->setConnectionType()
 **********************************************************************/

/* {{{ proto int layer.setConnectionType(int connectiontype, string library_str)
   Set layer connectiontype.  Returns 0 on success, -1 in error. */

DLEXPORT void php3_ms_lyr_setConnectionType(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pConnectionType, *pLibrary;
    pval   *pThis;
    int     numArgs, nStatus = -1;
    const char *pszLibrary = "";

    HashTable   *list=NULL;
    pThis = getThis();
    numArgs = ARG_COUNT(ht);

    if (pThis == NULL || (numArgs != 1 && numArgs != 2) ||
        getParameters(ht, numArgs, &pConnectionType, &pLibrary) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }
    convert_to_long(pConnectionType);

    if (numArgs >= 2)
    {
        convert_to_string(pLibrary);
        pszLibrary = pLibrary->value.str.val;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = layerObj_setConnectionType(self, 
                                              pConnectionType->value.lval,
                                              pszLibrary)) != 0)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }
    else
    {
        _phpms_set_property_long(pThis, "connectiontype", 
                                 self->connectiontype, E_ERROR TSRMLS_CC);
    }

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
                                           list TSRMLS_CC);
    poShape = 
        (shapeObj *)_phpms_fetch_handle2(pShape, 
                                         PHPMS_GLOBAL(le_msshape_ref),
                                         PHPMS_GLOBAL(le_msshape_new),
                                         list TSRMLS_CC);

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
                                           list TSRMLS_CC);
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
                                           list TSRMLS_CC);

    if (self== NULL ||
        (poResult = layerObj_getResult(self, pIndex->value.lval)) == NULL)
    {
        /* Invalid result index. */
        RETURN_FALSE;
    }

    /* Return a resultCacheMemberObj object */
    _phpms_build_resultcachemember_object(&(self->resultcache->
                                            results[pIndex->value.lval]),
                                          list TSRMLS_CC, return_value);
}
/* }}} */



/**********************************************************************
 *                        layer->open()
 **********************************************************************/

/* {{{ proto int layer.open()
   Open the layer for use with getFeature().  Returns MS_SUCCESS/MS_FAILURE. */

DLEXPORT void php3_ms_lyr_open(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
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

    if (pThis == NULL || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }


    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = layerObj_open(self)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_LONG(nStatus);
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
 *                        layer->whichshapes()
 **********************************************************************/

/* {{{ proto int layer.whichshapes() Returns MS_SUCCESS/MS_FAILURE. */
/*
** Performs a spatial, and optionally an attribute based feature search. The function basically
** prepares things so that candidate features can be accessed by query or drawing functions. For
** OGR and shapefiles this sets an internal bit vector that indicates whether a particular feature
** is to processed. For SDE it executes an SQL statement on the SDE server. Once run the msLayerNextShape
** function should be called to actually access the shapes.
**
** Note that for shapefiles we apply any maxfeatures constraint at this point. That may be the only
** connection type where this is feasible.
*/

DLEXPORT void php3_ms_lyr_whichShapes(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pExtent;
    layerObj    *self=NULL;
    HashTable   *list=NULL;
    int         nArgs = ARG_COUNT(ht);
    rectObj     *poGeorefExt = NULL;
    int         nStatus = MS_FAILURE;

    pThis = getThis();

    if (pThis == NULL || nArgs != 1)
    {
        WRONG_PARAM_COUNT;
    }

    if (getParameters(ht, nArgs, &pExtent) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);

    poGeorefExt = (rectObj *)_phpms_fetch_handle2(pExtent, 
                                                  PHPMS_GLOBAL(le_msrect_ref),
                                                  PHPMS_GLOBAL(le_msrect_new),
                                                  list TSRMLS_CC);
    
    if (self && poGeorefExt)
      nStatus = layerObj_whichShapes(self, poGeorefExt);
    

    RETURN_LONG(nStatus);

}


/**********************************************************************
 *                        layer->nextshape()
 **********************************************************************/

/* {{{ proto int layer.nextshape() Returns a shape or MS_FAILURE. */
DLEXPORT void php3_ms_lyr_nextShape(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis;
    layerObj *self=NULL;
    HashTable   *list=NULL;
    shapeObj    *poShape = NULL;

    pThis = getThis();

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self)
      poShape = layerObj_nextShape(self);

    if (!poShape)
       RETURN_FALSE; 

    /* Return valid object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), self,
                              list, return_value TSRMLS_CC);

}


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
                                           list TSRMLS_CC);
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
    HashTable   *list=NULL;

    pThis = getThis();

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
                                           list TSRMLS_CC);

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
                              list, return_value TSRMLS_CC);
}
/* }}} */


/**********************************************************************
 *                        layer->getFeature()
 **********************************************************************/

/* {{{ proto shapeObj layer.getFeature(shapeindex, [tileindex])
   Retrieve shapeObj from a layer by index. */

DLEXPORT void php3_ms_lyr_getFeature(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis, *pTileId=NULL, *pShapeId ;
    layerObj *self=NULL;
    shapeObj    *poShape;
    int      numArgs, nTileId = -1;
    HashTable   *list=NULL;

    pThis = getThis();
    numArgs = ARG_COUNT(ht);
    if (pThis == NULL || (numArgs != 1 && numArgs != 2) ||
        getParameters(ht, numArgs, &pShapeId, &pTileId) != SUCCESS) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pShapeId);

    if (numArgs >= 2)
    {
        convert_to_long(pTileId);
        nTileId = pTileId->value.lval;
    }
        
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
                                           list TSRMLS_CC);

    if (self == NULL || 
        layerObj_getShape(self, poShape, nTileId, 
                          pShapeId->value.lval) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
        shapeObj_destroy(poShape);
        RETURN_FALSE; 
    }

    /* Return valid object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), self,
                              list, return_value TSRMLS_CC);
}
/* }}} */

/**********************************************************************
 *                        layer->resultsGetShape()
 **********************************************************************/

/* {{{ proto shapeObj layer.resultsGetShape(shapeindex, [tileindex])
   Retrieve shapeObj from a resultset by index. */

DLEXPORT void php3_ms_lyr_resultsGetShape(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis, *pTileId=NULL, *pShapeId ;
    layerObj *self=NULL;
    shapeObj    *poShape;
    int      numArgs, nTileId = -1;
    HashTable   *list=NULL;

    pThis = getThis();
    numArgs = ARG_COUNT(ht);
    if (pThis == NULL || (numArgs != 1 && numArgs != 2) ||
        getParameters(ht, numArgs, &pShapeId, &pTileId) != SUCCESS) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pShapeId);

    if (numArgs >= 2)
    {
        convert_to_long(pTileId);
        nTileId = pTileId->value.lval;
    }
        
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
                                           list TSRMLS_CC);

    if (self == NULL || 
        msLayerResultsGetShape(self, poShape, nTileId, 
                               pShapeId->value.lval) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
        shapeObj_destroy(poShape);
        RETURN_FALSE; 
    }

    /* Return valid object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), self,
                              list, return_value TSRMLS_CC);
}
/* }}} */


/**********************************************************************
 *                        layer->getExtent()
 **********************************************************************/

/* {{{ proto int layer.getExtent()
   Retrieve or calculate a layer's extents. */

DLEXPORT void php3_ms_lyr_getExtent(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis;
    layerObj *self=NULL;
    rectObj  *poRect=NULL;
    HashTable   *list=NULL;
    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) > 0) 
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);

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

    if (msLayerGetExtent(self, poRect) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }

    /* Return rectObj */
    _phpms_build_rect_object(poRect, PHPMS_GLOBAL(le_msrect_new), 
                              list, return_value TSRMLS_CC);
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
                                           list TSRMLS_CC);
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
                                           list TSRMLS_CC);
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


/**********************************************************************
 *                        layer->removeMetaData()
 **********************************************************************/

/* {{{ proto int layer.removeMetaData(string name)
   Remove MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_lyr_removeMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pThis, *pName;
    int    nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    pThis = getThis();


    if (pThis == NULL ||
        getParameters(ht, 1, &pName) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pName);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = layerObj_removeMetaData(self, 
                                        pName->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/**********************************************************************
 *                        layer->getWMSFeatureInfoURL()
 **********************************************************************/

/* {{{ proto string layer.getWMSFeatureInfoURL(int clickX, int clickY, int featureCount, string infoFormat)
   Return a WMS GetFeatureInfo URL (only for WMS layers). */

DLEXPORT void php3_ms_lyr_getWMSFeatureInfoURL(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pThis, *pX, *pY, *pCount, *pFormat;
    char   *pszValue = NULL;
    mapObj *parent_map;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 4, &pX, &pY, &pCount, &pFormat) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pX);
    convert_to_long(pY);
    convert_to_long(pCount);
    convert_to_string(pFormat);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);

    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);


    if (self == NULL || parent_map == NULL ||
        (pszValue = layerObj_getWMSFeatureInfoURL(self, parent_map, 
                                                  pX->value.lval,
                                                  pY->value.lval,
                                                  pCount->value.lval,
                                                  pFormat->value.str.val)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_STRING("", 1);
    }

    RETVAL_STRING(pszValue, 1);
    free(pszValue);
}
/* }}} */

/**********************************************************************
 *                        layer->getItems()
 *
 * Return an array of string containing all the layer items
 **********************************************************************/

/* {{{ proto char** layer.getItems()
   Return an array containing all the layer items.*/

DLEXPORT void php3_ms_lyr_getItems(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    layerObj    *self=NULL;
    int         i = 0;
    int         res = 0;

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

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                         list TSRMLS_CC);
   
    if (self != NULL)
     res = msLayerGetItems(self);
   
    if (res != MS_FAILURE &&  self->numitems > 0)
    {
       for (i=0; i<self->numitems; i++)
       {
         /* add a copy of the group name to the PHP array */
          add_next_index_string(return_value, self->items[i], 1);
       }
    }
    else
    {
        RETURN_FALSE;
    }
}

/**********************************************************************
 *                        layer->setProcessing()
 *
 * set a processing string to the layer
 **********************************************************************/

/* {{{ boolean layer.setProcessing(string)
  set a processing string to the layer*/

DLEXPORT void php3_ms_lyr_setProcessing(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis, *pString;
    layerObj    *layer=NULL;

    HashTable   *list=NULL;
    pThis = getThis();
    

    if (pThis == NULL ||
        getParameters(ht, 1, &pString) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pString);

    layer = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
    
    if (!layer)
      RETURN_FALSE;

    layer->numprocessing++;
    if(layer->numprocessing == 1)
      layer->processing = (char **) malloc(2*sizeof(char *));
    else
      layer->processing = (char **) realloc(layer->processing, sizeof(char*) * (layer->numprocessing+1));
    layer->processing[layer->numprocessing-1] = strdup(pString->value.str.val);
    layer->processing[layer->numprocessing] = NULL;
    
    _phpms_set_property_long(pThis, "num_processing", layer->numprocessing, E_ERROR TSRMLS_CC);

    RETURN_TRUE;
}    

/**********************************************************************
 *                        layer->getProcessing()
 *
 * Return an array of string containing all the layer items
 **********************************************************************/

/* {{{ proto char** layer.getProcessing()
   Return an array containing all the processing strings.*/
DLEXPORT void php3_ms_lyr_getProcessing(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    layerObj    *self=NULL;
    int         i = 0;

    HashTable   *list=NULL;
    pThis = getThis();


    if (pThis == NULL)
    {
        WRONG_PARAM_COUNT;
    }

    if (array_init(return_value) == FAILURE)
      RETURN_FALSE;
    

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);
   
    if (self == NULL || self->numprocessing <= 0)
      RETURN_FALSE;

   
    for (i=0; i<self->numprocessing; i++)
    {
          add_next_index_string(return_value, self->processing[i], 1);
    }
}

/**********************************************************************
 *                        layer->clearProcessing()
 *
 * clear the processing strings in the layer
 **********************************************************************/

/* {{{ boolean layer.clearProcessing()
  clear the processing strings in the layer*/

DLEXPORT void php3_ms_lyr_clearProcessing(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    layerObj    *layer=NULL;
    int         i = 0;

    HashTable   *list=NULL;
    pThis = getThis();
    

    if (pThis == NULL)
        WRONG_PARAM_COUNT;

    layer = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                            list TSRMLS_CC);
    
    if (!layer)
      RETURN_FALSE;
    
    if (layer->numprocessing > 0)
    {
        for(i=0; i<layer->numprocessing; i++)
          free(layer->processing[i]);

        layer->numprocessing = 0;
        free(layer->processing);

        _phpms_set_property_long(pThis, "num_processing", layer->numprocessing, E_ERROR TSRMLS_CC);

    }
}        


/**********************************************************************
 *                        layer->executewfsgetfeature()
 *
 * Execute a GetFeature request on a wfs layer and return the name
 * of the temporary GML file created.
 **********************************************************************/

/* {{{ string layer.executewfsgetfeature()*/

DLEXPORT void php3_ms_lyr_executeWFSGetfeature(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pThis;
    layerObj    *layer=NULL;
    char        *pszValue = NULL;

    HashTable   *list=NULL;
    pThis = getThis();
    

    if (pThis == NULL)
        WRONG_PARAM_COUNT;

    layer = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                            list TSRMLS_CC);
    
    if (layer == NULL || (pszValue=layerObj_executeWFSGetFeature(layer)) == NULL)
       RETURN_STRING("", 1);

    RETVAL_STRING(pszValue, 1);
    free(pszValue);
}
 
/**********************************************************************
 *                        layer->applySLD(szSLDString)
 *
 * Apply an XML SLD string to the layer
 **********************************************************************/
DLEXPORT void php3_ms_lyr_applySLD(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     pval        *pSLDString=NULL, *pStyleLayer=NULL;
     layerObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_SUCCESS;
     int         nArgs = ARG_COUNT(ht);

     pThis = getThis();

     if (pThis == NULL)
     {
         RETURN_LONG(MS_FAILURE);
    }

     if (nArgs != 1 && nArgs != 2)
     {
        WRONG_PARAM_COUNT;
     }
     if (getParameters(ht,nArgs,&pSLDString, &pStyleLayer) == FAILURE)
     {
        WRONG_PARAM_COUNT;
     }

    convert_to_string(pSLDString);
    if (nArgs == 2)
      convert_to_string(pStyleLayer);
    
    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if (nArgs == 2)
      nStatus = layerObj_applySLD(self, pSLDString->value.str.val,
                                  pStyleLayer->value.str.val);
    else
      nStatus = layerObj_applySLD(self, pSLDString->value.str.val, NULL);

    RETURN_LONG(nStatus);
}


/**********************************************************************
 *                        layer->applySLD(szSLDURL)
 *
 * Apply an SLD to a layer. The SLD is given as an URL.
 **********************************************************************/
DLEXPORT void php3_ms_lyr_applySLDURL(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     pval        *pSLDString=NULL, *pStyleLayer=NULL;
     layerObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_SUCCESS;
     int         nArgs = ARG_COUNT(ht);

     pThis = getThis();

     if (pThis == NULL)
     {
         RETURN_LONG(MS_FAILURE);
     }
     if (nArgs != 1 && nArgs != 2)
     {
        WRONG_PARAM_COUNT;
     }
     
    if (getParameters(ht,nArgs,&pSLDString, &pStyleLayer) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pSLDString);
    if (nArgs == 2)
      convert_to_string(pStyleLayer);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    if (nArgs == 2)
      nStatus = layerObj_applySLDURL(self, pSLDString->value.str.val,
                                      pStyleLayer->value.str.val);
    else
      nStatus = layerObj_applySLDURL(self, pSLDString->value.str.val, NULL);

    RETURN_LONG(nStatus);
}


DLEXPORT void php3_ms_lyr_generateSLD(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis;
     layerObj      *self=NULL;
     HashTable   *list=NULL;
     char       *pszBuffer = NULL;

     pThis = getThis();

     if (pThis == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }


    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer), 
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(MS_FAILURE);
    }

    pszBuffer = layerObj_generateSLD(self);

    if (pszBuffer)
    {
        RETVAL_STRING(pszBuffer, 1);
        free(pszBuffer);
    }
    else
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_STRING("", 0);
    }
}


DLEXPORT void  php3_ms_lyr_moveClassUp(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pClassIdx;
     layerObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_FAILURE;

     pThis = getThis();

     if (pThis == NULL ||
        getParameters(ht, 1, &pClassIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

     convert_to_long(pClassIdx);

     self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer), 
                                            list TSRMLS_CC);

     if (self != NULL)
     {
         nStatus = layerObj_moveClassUp(self, pClassIdx->value.lval);
     }

      RETURN_LONG(nStatus);
}

DLEXPORT void  php3_ms_lyr_moveClassDown(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pClassIdx;
     layerObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_FAILURE;


     pThis = getThis();

     if (pThis == NULL ||
        getParameters(ht, 1, &pClassIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

     convert_to_long(pClassIdx);

     self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer), 
                                            list TSRMLS_CC);

     if (self != NULL)
     {
         nStatus = layerObj_moveClassDown(self, pClassIdx->value.lval);
     }

     RETURN_LONG(nStatus);
}

DLEXPORT void  php3_ms_lyr_removeClass(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pClassIdx;
     layerObj      *self=NULL;
     HashTable   *list=NULL;
     classObj    *pOldClassObj = NULL;
     int layer_id, map_id;


     pThis = getThis();

     if (pThis == NULL ||
        getParameters(ht, 1, &pClassIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

     convert_to_long(pClassIdx);

     self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer), 
                                            list TSRMLS_CC);

    /* We will store a reference to the parent_layer object id inside
     * the class obj.
     */
    layer_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR TSRMLS_CC);

    /* We will store a reference to the parent_map object id inside
     * the class obj.
     */
    map_id = _phpms_fetch_property_resource(pThis, "_map_handle_", E_ERROR TSRMLS_CC);


     if (self != NULL)
     {
         pOldClassObj = layerObj_removeClass(self, pClassIdx->value.lval);

        /* Update number of classes within the layer object */
        _phpms_set_property_long(pThis, "numclasses",
                             self->numclasses, E_ERROR TSRMLS_CC);
     }

     /* Return a copy of the class object just removed */
     _phpms_build_class_object(pOldClassObj, map_id, layer_id, list, 
                              return_value TSRMLS_CC);
}

/* }}} */


/**********************************************************************
 *                        layer->isVisible()
 **********************************************************************/

/* {{{ proto int layer.isVisible()
   Returns MS_TRUE/MS_FALSE depending on whether the layer is currently visible in the map (i.e. turned on, in scale, etc.). */

DLEXPORT void php3_ms_lyr_isVisible(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    layerObj    *self=NULL;
    mapObj      *parent_map = NULL;
    int         bVisible = MS_FALSE;
    HashTable   *list=NULL;
    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) > 0) 
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);

    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);
    if (self != NULL && parent_map != NULL)
    {
        bVisible = msLayerIsVisible(parent_map, self);
    }

    RETURN_LONG(bVisible);
}
/* }}} */


/**********************************************************************
 *                        layer->getGridIntersctionCoordinates()
 **********************************************************************/

/* {{{ proto int layer.getGridIntersctionCoordinates()

*/
DLEXPORT void php3_ms_lyr_getGridIntersectionCoordinates(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    layerObj    *self=NULL;
    mapObj      *parent_map = NULL;
    graticuleIntersectionObj *pasValues=NULL;
    HashTable   *list=NULL;
    pval *tmp_arr1;
    int i=0;


    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) > 0) 
    {
        WRONG_PARAM_COUNT;
    }

    if (array_init(return_value) == FAILURE) 
      RETURN_FALSE;

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list TSRMLS_CC);

    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);
    if (self->connectiontype != MS_GRATICULE)
    {
        php3_error(E_ERROR, "GetGridIntersectionCoordinates failed: Layer is not of graticule type"); 
        RETURN_FALSE;
    }
    if (self != NULL && parent_map != NULL)
    {
        pasValues = msGraticuleLayerGetIntersectionPoints(parent_map, self);
        if (pasValues == NULL)
          RETURN_FALSE;

        /*TOP*/
        add_assoc_double(return_value, "topnumpoints", pasValues->nTop);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nTop; i++)
        {
            add_next_index_string(tmp_arr1, pasValues->papszTopLabels[i],1);
        }
        zend_hash_update(Z_ARRVAL_P(return_value), "toplabels", strlen("toplabels")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nTop; i++)
        {
            add_next_index_double(tmp_arr1, pasValues->pasTop[i].x);
            add_next_index_double(tmp_arr1, pasValues->pasTop[i].y);
            
        }

        zend_hash_update(Z_ARRVAL_P(return_value), "toppoints", strlen("toppoints")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);

        /*BOTTOM*/

        add_assoc_double(return_value, "bottomnumpoints", pasValues->nBottom);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nBottom; i++)
        {
            add_next_index_string(tmp_arr1, pasValues->papszBottomLabels[i],1);
        }
        zend_hash_update(Z_ARRVAL_P(return_value), "bottomlabels", strlen("bottomlabels")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nBottom; i++)
        {
            add_next_index_double(tmp_arr1, pasValues->pasBottom[i].x);
            add_next_index_double(tmp_arr1, pasValues->pasBottom[i].y);
            
        }

        zend_hash_update(Z_ARRVAL_P(return_value), "bottompoints", strlen("bottompoints")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);


        /*LEFT*/
        add_assoc_double(return_value, "leftnumpoints", pasValues->nLeft);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nLeft; i++)
        {
            add_next_index_string(tmp_arr1, pasValues->papszLeftLabels[i],1);
        }
        zend_hash_update(Z_ARRVAL_P(return_value), "leftlabels", strlen("leftlabels")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nLeft; i++)
        {
            add_next_index_double(tmp_arr1, pasValues->pasLeft[i].x);
            add_next_index_double(tmp_arr1, pasValues->pasLeft[i].y);
            
        }

        zend_hash_update(Z_ARRVAL_P(return_value), "leftpoints", strlen("leftpoints")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);


        /*RIGHT*/
        add_assoc_double(return_value, "rightnumpoints", pasValues->nRight);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nRight; i++)
        {
            add_next_index_string(tmp_arr1, pasValues->papszRightLabels[i],1);
        }
        zend_hash_update(Z_ARRVAL_P(return_value), "rightlabels", strlen("rightlabels")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);

        MAKE_STD_ZVAL(tmp_arr1);
        array_init(tmp_arr1);
        for (i=0; i<pasValues->nRight; i++)
        {
            add_next_index_double(tmp_arr1, pasValues->pasRight[i].x);
            add_next_index_double(tmp_arr1, pasValues->pasRight[i].y);
            
        }

        zend_hash_update(Z_ARRVAL_P(return_value), "rightpoints", strlen("rightpoints")+1, &tmp_arr1, 
                         sizeof(tmp_arr1), NULL);
        
        msGraticuleLayerFreeIntersectionPoints(pasValues);

    }
    else
      RETURN_FALSE;
}
/* }}} */

/*=====================================================================
 *                 PHP function wrappers - labelObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_label_object()
 **********************************************************************/
static long _phpms_build_label_object(labelObj *plabel,
                                      HashTable *list, pval *return_value TSRMLS_DC)
{
    int         label_id;
    pval        *new_obj_ptr;

    if (plabel == NULL)
        return 0;

    label_id = php3_list_insert(plabel, PHPMS_GLOBAL(le_mslabel));

    _phpms_object_init(return_value, label_id, php_label_class_functions,
                       PHP4_CLASS_ENTRY(label_class_entry_ptr) TSRMLS_CC);

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value,  "font",       plabel->font);
    PHPMS_ADD_PROP_STR(return_value,  "encoding",   plabel->encoding);
    add_property_long(return_value,   "type",       plabel->type);
    add_property_long(return_value,   "shadowsizex",plabel->shadowsizex);
    add_property_long(return_value,   "shadowsizey",plabel->shadowsizey);
    add_property_long(return_value,   "backgroundshadowsizex",
                                                plabel->backgroundshadowsizex);
    add_property_long(return_value,   "backgroundshadowsizey",
                                                plabel->backgroundshadowsizey);
    add_property_double(return_value,   "size",       plabel->size);
    add_property_double(return_value,   "minsize",    plabel->minsize);
    add_property_double(return_value,   "maxsize",    plabel->maxsize);
    add_property_long(return_value,   "position",   plabel->position);
    add_property_long(return_value,   "offsetx",    plabel->offsetx);
    add_property_long(return_value,   "offsety",    plabel->offsety);
    add_property_double(return_value, "angle",      plabel->angle);
    add_property_long(return_value,   "autoangle",  plabel->autoangle);
    add_property_long(return_value,   "autofollow",  plabel->autofollow);
    add_property_long(return_value,   "buffer",     plabel->buffer);
    add_property_long(return_value,   "antialias",  plabel->antialias);
    add_property_long(return_value,   "wrap",       plabel->wrap);
    add_property_long(return_value,   "minfeaturesize",plabel->minfeaturesize);
    add_property_long(return_value,   "autominfeaturesize",plabel->autominfeaturesize);
    add_property_long(return_value,   "repeatdistance",plabel->repeatdistance);
    add_property_long(return_value,   "mindistance",plabel->mindistance);
    add_property_long(return_value,   "partials",   plabel->partials);
    add_property_long(return_value,   "force",      plabel->force);
    add_property_long(return_value,   "outlinewidth", plabel->outlinewidth);
    add_property_long(return_value,   "align", plabel->align);
    add_property_long(return_value,   "maxlength", plabel->maxlength);
    add_property_long(return_value,   "minlength", plabel->minlength);
    add_property_long(return_value ,  "priority",   plabel->priority);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(plabel->color),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "color",new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(plabel->outlinecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "outlinecolor",new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(plabel->shadowcolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "shadowcolor",new_obj_ptr,E_ERROR TSRMLS_CC);
 

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(plabel->backgroundcolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "backgroundcolor",new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(plabel->backgroundshadowcolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "backgroundshadowcolor",new_obj_ptr,E_ERROR TSRMLS_CC);


    return label_id;
}

/**********************************************************************
 *                        label->updateFromString()
 **********************************************************************/

/* {{{ proto int label.updateFromString(string snippet)
   Update a label from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_label_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    labelObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (labelObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslabel),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = labelObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}


/* }}} */


/**********************************************************************
 *                        label->set()
 **********************************************************************/

/* {{{ proto int label.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_label_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    labelObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (labelObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslabel),
                                           list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "font",         self->font)
    else IF_SET_STRING("encoding",     self->encoding)
    else IF_SET_LONG(  "type",         self->type)
    else IF_SET_LONG(  "shadowsizex",  self->shadowsizex)
    else IF_SET_LONG(  "shadowsizey",  self->shadowsizey)
    else IF_SET_LONG(  "backgroundshadowsizex", self->backgroundshadowsizex)
    else IF_SET_LONG(  "backgroundshadowsizey", self->backgroundshadowsizey)
    else IF_SET_DOUBLE(  "size",         self->size)
    else IF_SET_DOUBLE(  "minsize",      self->minsize)
    else IF_SET_DOUBLE(  "maxsize",      self->maxsize)
    else IF_SET_LONG(  "position",     self->position)
    else IF_SET_LONG(  "offsetx",      self->offsetx)
    else IF_SET_LONG(  "offsety",      self->offsety)
    else IF_SET_DOUBLE("angle",        self->angle)
    else IF_SET_LONG(  "autoangle",    self->autoangle)
    else IF_SET_LONG(  "autofollow",  self->autofollow)
    else IF_SET_LONG(  "buffer",       self->buffer)
    else IF_SET_LONG(  "antialias",    self->antialias)
    else IF_SET_BYTE(  "wrap",         self->wrap)
    else IF_SET_LONG(  "minfeaturesize", self->minfeaturesize)
    else IF_SET_LONG(  "autominfeaturesize", self->autominfeaturesize)
    else IF_SET_LONG(  "repeatdistance",  self->repeatdistance)
    else IF_SET_LONG(  "mindistance",  self->mindistance)
    else IF_SET_LONG(  "partials",     self->partials)
    else IF_SET_LONG(  "force",        self->force)
    else IF_SET_LONG(  "outlinewidth", self->outlinewidth)
    else IF_SET_LONG(  "align", self->align)
    else IF_SET_LONG(  "maxlength", self->maxlength)
    else IF_SET_LONG(  "minlength", self->minlength)
    else IF_SET_LONG(  "priority",     self->priority)       
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    if (self->size == -1)
    {
        _phpms_set_property_long(pThis,"size", MS_MEDIUM, E_ERROR TSRMLS_CC);
        self->size =  MS_MEDIUM;
    }

    RETURN_LONG(0);
}



/* {{{ proto int label.setbinding(const bindingid, string value)
   Set the attribute binding for a specfiled label property. Returns true on success. */

DLEXPORT void php3_ms_label_setBinding(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    labelObj *self = NULL;
    HashTable   *list=NULL;
    pval   *pBindingId, *pValue;

    pThis = getThis();

    if (pThis == NULL || 
        getParameters(ht, 2, &pBindingId, &pValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (labelObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_mslabel),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid label object.");

    convert_to_string(pValue);
    convert_to_long(pBindingId);

    if (pBindingId->value.lval < 0 || pBindingId->value.lval > MS_LABEL_BINDING_LENGTH)
       php3_error(E_ERROR, "Invalid binding id given for setbinding function.");

    if (!pValue->value.str.val || strlen(pValue->value.str.val) <= 0)
       php3_error(E_ERROR, "Invalid binding value given for setbinding function.");

    if(self->bindings[pBindingId->value.lval].item) 
    {
        msFree(self->bindings[pBindingId->value.lval].item);
        self->bindings[pBindingId->value.lval].index = -1; 
        self->numbindings--;
    }
    self->bindings[pBindingId->value.lval].item = strdup(pValue->value.str.val); 
    self->numbindings++;

    RETURN_TRUE;
}

/* {{{ proto int label.getbinding(const bindingid)
   Get the value of a attribute binding for a specfiled label property. 
   Returns the string value if exist, else null. */

DLEXPORT void php3_ms_label_getBinding(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    labelObj *self = NULL;
    HashTable   *list=NULL;
    pval   *pBindingId;
    char   *pszValue=NULL;

    pThis = getThis();

    if (pThis == NULL || 
        getParameters(ht, 1, &pBindingId) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (labelObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_mslabel),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid label object.");

    convert_to_long(pBindingId);

    if (pBindingId->value.lval < 0 || pBindingId->value.lval > MS_LABEL_BINDING_LENGTH)
       php3_error(E_ERROR, "Invalid binding id given for getbinding function.");


    if( (pszValue = self->bindings[pBindingId->value.lval].item) != NULL) 
    {
       RETURN_STRING(pszValue, 1);                
    }

    return;
}


/* {{{ proto int label.removebinding(const bindingid)
   Remove attribute binding for a specfiled label property. Returns true on success. */

DLEXPORT void php3_ms_label_removeBinding(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    labelObj *self = NULL;
    HashTable   *list=NULL;
    pval   *pBindingId;

    pThis = getThis();

    if (pThis == NULL || 
        getParameters(ht, 1, &pBindingId) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (labelObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_mslabel),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid label object.");

    convert_to_long(pBindingId);

    if (pBindingId->value.lval < 0 || pBindingId->value.lval > MS_LABEL_BINDING_LENGTH)
       php3_error(E_ERROR, "Invalid binding id given for removebinding function.");


    if(self->bindings[pBindingId->value.lval].item) 
    {
        msFree(self->bindings[pBindingId->value.lval].item);
        self->bindings[pBindingId->value.lval].index = -1; 
        self->numbindings--;
    }

    RETURN_TRUE;
}


/*=====================================================================
 *                 PHP function wrappers - classObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_class_object()
 **********************************************************************/
static long _phpms_build_class_object(classObj *pclass, int parent_map_id, 
                                      int parent_layer_id,
                                      HashTable *list, 
                                      pval *return_value TSRMLS_DC)
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
                       PHP4_CLASS_ENTRY(class_class_entry_ptr) TSRMLS_CC);

#ifdef PHP4
    add_property_resource(return_value, "_layer_handle_", parent_layer_id);
    zend_list_addref(parent_layer_id);
#else
    add_property_long(return_value, "_layer_handle_", parent_layer_id);
#endif
   
#ifdef PHP4
    add_property_resource(return_value, "_map_handle_", parent_map_id);
    zend_list_addref(parent_map_id);
#else
    add_property_long(return_value, "_map_handle_", parent_map_id);
#endif   

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value,  "name",       pclass->name);
    PHPMS_ADD_PROP_STR(return_value,  "title",       pclass->title);
    add_property_long(return_value,   "type",       pclass->type);
    add_property_long(return_value,   "status",     pclass->status);
    PHPMS_ADD_PROP_STR(return_value,  "template",   pclass->template);
    add_property_long(return_value,   "numstyles",  pclass->numstyles);
    
#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_label_object(&(pclass->label), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "label", new_obj_ptr,E_ERROR TSRMLS_CC);

    add_property_double(return_value,  "minscaledenom", pclass->minscaledenom);
    add_property_double(return_value,  "maxscaledenom", pclass->maxscaledenom);
    /* TODO: *scale deprecated in v5.0. Remove in future release */
    add_property_double(return_value,  "minscale", pclass->minscaledenom);
    add_property_double(return_value,  "maxscale", pclass->maxscaledenom);

    PHPMS_ADD_PROP_STR(return_value,   "keyimage",  pclass->keyimage);

    PHPMS_ADD_PROP_STR(return_value,   "group",  pclass->group);

    MAKE_STD_ZVAL(new_obj_ptr);
    _phpms_build_hashtable_object(&(pclass->metadata),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "metadata",new_obj_ptr,E_ERROR TSRMLS_CC);
    
    return class_id;
}


/**********************************************************************
 *                        ms_newClassObj()
 **********************************************************************/

/* {{{ proto layerObj ms_newClassObj(layerObj layer)
   Create a new class in the specified layer. */

DLEXPORT void php3_ms_class_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pLayerObj,*pClassObj;
    layerObj *parent_layer;
    classObj *pNewClass;
    classObj *class_obj = NULL;
    int layer_id, map_id;
    int         nArgs = ARG_COUNT(ht);

#ifdef PHP4
    HashTable   *list=NULL;
#endif


    if (nArgs != 1 && nArgs != 2)
    {
        WRONG_PARAM_COUNT;
    }


    if (getParameters(ht, nArgs, &pLayerObj, &pClassObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_layer = (layerObj*)_phpms_fetch_handle(pLayerObj, 
                                                  PHPMS_GLOBAL(le_mslayer),
                                                  list TSRMLS_CC);

    if (nArgs == 2)
    {
        class_obj = (classObj*)_phpms_fetch_handle(pClassObj, 
                                                   PHPMS_GLOBAL(le_msclass),
                                                   list TSRMLS_CC);
    }

    if (parent_layer == NULL ||
        (pNewClass = classObj_new(parent_layer, class_obj)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

     _phpms_set_property_long(pLayerObj,"numclasses", parent_layer->numclasses, 
                              E_ERROR TSRMLS_CC); 

    /* We will store a reference to the parent_layer object id inside
     * the class obj.
     */
    layer_id = _phpms_fetch_property_resource(pLayerObj, "_handle_", E_ERROR TSRMLS_CC);

    /* We will store a reference to the parent_map object id inside
     * the class obj.
     */
    map_id = _phpms_fetch_property_resource(pLayerObj, "_map_handle_", E_ERROR TSRMLS_CC);
   
    /* Return class object */
    _phpms_build_class_object(pNewClass, map_id, layer_id, list, 
                              return_value TSRMLS_CC);
}
/* }}} */


/**********************************************************************
 *                        class->updateFromString()
 **********************************************************************/

/* {{{ proto int class.updateFromString(string snippet)
   Update a class from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_class_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    classObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = classObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
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
    mapObj *parent_map;
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
                                           list TSRMLS_CC);
   
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_",
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

    if (self == NULL || parent_map == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "name",          self->name)
    else IF_SET_STRING("title",         self->title)
    else IF_SET_LONG(  "type",          self->type)
    else IF_SET_LONG(  "status",        self->status)
    else IF_SET_DOUBLE("minscaledenom", self->minscaledenom)
    else IF_SET_DOUBLE("maxscaledenom", self->maxscaledenom)
    /* TODO: *scale deprecated in v5.0. Remove in future release */
    else IF_SET_DOUBLE("minscale",      self->minscaledenom)
    else IF_SET_DOUBLE("maxscale",      self->maxscaledenom)

    else IF_SET_STRING("template",      self->template)
    else IF_SET_STRING("keyimage",      self->keyimage)       
    else if (strcmp( "numstyles", pPropertyName->value.str.val) == 0)
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
                                           list TSRMLS_CC);

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
/*                          class->getExpressionString()                */
/*                                                                      */
/*      Returns the expression string for a class object.               */
/*                                                                      */
/************************************************************************/

/* {{{ proto string class.getExpressionString()
   Get the expression string for a class object. */

DLEXPORT void php3_ms_class_getExpressionString(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis;
    classObj *self=NULL;
    HashTable   *list=NULL;
     char   *pszValue = NULL;

    pThis = getThis();

    if (pThis == NULL)
    {
        WRONG_PARAM_COUNT;
    }

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (pszValue = classObj_getExpressionString(self)) == NULL)
    {
      RETURN_STRING("", 1);
    }
    else
    {
      RETURN_STRING(pszValue, 1);
      free(pszValue);
    }
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
    int         nArgs;

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    nArgs = ARG_COUNT(ht);
    if ((nArgs != 1 && nArgs != 2) || pThis == NULL)
      WRONG_PARAM_COUNT;

    if (nArgs == 1)
      getParameters(ht, 1, &pString);
    else
      getParameters(ht, 2, &pLayerObj, &pString);


    convert_to_string(pString);

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);

    parent_layer = (layerObj*)_phpms_fetch_property_handle(pThis, "_layer_handle_",
                                                           PHPMS_GLOBAL(le_mslayer),
                                                           list TSRMLS_CC, E_ERROR);
    //parent_layer = (layerObj*)_phpms_fetch_handle(pLayerObj, 
    //                                             PHPMS_GLOBAL(le_mslayer),
    //                                              list TSRMLS_CC);

    if (self == NULL || parent_layer == NULL ||
        (nStatus = classObj_setText(self, parent_layer, pString->value.str.val)) != 0)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_LONG(nStatus);
    }

    RETURN_LONG(0);
}
/* }}} */


/************************************************************************/
/*                          class->getTextString()                      */
/*                                                                      */
/*      Returns the text string for a class object.                     */
/*                                                                      */
/************************************************************************/

/* {{{ proto string class.getTextString()
   Get the text string for a class object. */

DLEXPORT void php3_ms_class_getTextString(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis;
    classObj *self=NULL;
    HashTable   *list=NULL;
     char   *pszValue = NULL;

    pThis = getThis();

    if (pThis == NULL)
    {
        WRONG_PARAM_COUNT;
    }

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (pszValue = classObj_getTextString(self)) == NULL)
    {
      RETURN_STRING("", 1);
    }
    else
    {
      RETURN_STRING(pszValue, 1);
      free(pszValue);
    }
}
/* }}} */

/************************************************************************/
/*                          class->drawLegendIcon()                     */
/*                                                                      */
/*      return the legend icon related to this class.                   */
/*                                                                      */
/*      Returns 0 on success, -1 on error.                              */
/************************************************************************/

/* {{{ proto int class.drawLegendIcon(int width, int height, image img, int dstX, int dstY)
   set the lengend icon in img. */

/* SFO */

DLEXPORT void php3_ms_class_drawLegendIcon(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pWidth, *pHeight, *imgObj, *pDstX, *pDstY, *pThis;
    mapObj *parent_map;
    classObj *self;
    layerObj *parent_layer;
    imageObj *im = NULL;
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
        getParameters(ht, 5, &pWidth, &pHeight, &imgObj, &pDstX, &pDstY) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    im = (imageObj *)_phpms_fetch_handle(imgObj,
                                         PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);

    convert_to_long(pWidth);
    convert_to_long(pHeight);
    convert_to_long(pDstX);
    convert_to_long(pDstY);   

    self = (classObj *)_phpms_fetch_handle(pThis, 
                                           PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);

    parent_layer = (layerObj*)_phpms_fetch_property_handle(pThis, "_layer_handle_",
                                                           PHPMS_GLOBAL(le_mslayer),
                                                           list TSRMLS_CC, E_ERROR);

    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_",
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);
    if (im != NULL && !(MS_DRIVER_GD(im->format)||MS_DRIVER_AGG(im->format)))
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_WARNING, "DrawLegendicon function is only available for GD and AGG drivers");
        RETURN_FALSE;
    }
    if (self == NULL || parent_map == NULL || parent_layer == NULL ||
        (retVal = classObj_drawLegendIcon(self, 
                                          parent_map, 
                                          parent_layer, 
                                          pWidth->value.lval, pHeight->value.lval, 
                                          im, 
                                          pDstX->value.lval, pDstY->value.lval)) == -1)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }
   
    RETURN_LONG(retVal);   
}

/************************************************************************/
/*                          class->createLegendIcon()                   */
/*                                                                      */
/*      return the legend icon related to this class.                   */
/*                                                                      */
/*      Returns image object.                                           */
/************************************************************************/

/* {{{ proto int class.createLegendIcon(int width, int height)
   return the lengend icon. */

DLEXPORT void php3_ms_class_createLegendIcon(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pWidth, *pHeight, *pThis;
    mapObj *parent_map;
    classObj *self;
    layerObj *parent_layer;
    imageObj *im = NULL;
   
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 2, &pWidth, &pHeight) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pWidth);
    convert_to_long(pHeight);

    self = (classObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);

    parent_layer = (layerObj*)_phpms_fetch_property_handle(pThis, "_layer_handle_",
                                                           PHPMS_GLOBAL(le_mslayer),
                                                           list TSRMLS_CC, E_ERROR);

    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_",
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

    if (self == NULL || parent_map == NULL || parent_layer == NULL ||
        (im = classObj_createLegendIcon(self, 
                                        parent_map, 
                                        parent_layer, 
                                        pWidth->value.lval, pHeight->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
       
        RETURN_FALSE;
    }

    /* Return an image object */
    _phpms_build_img_object(im, &(parent_map->web), list, return_value TSRMLS_CC);
}


/************************************************************************/
/*                      class->getstyle(index)                          */
/*                                                                      */
/*      return the style object referneced by the index. The index      */
/*      should be from 0 to class->numstyles.                           */
/*                                                                      */
/*                                                                      */
/*      Returns a style object.                                         */
/************************************************************************/

/* {{{ proto int class.getstyle(int index)
   return the style object. */

DLEXPORT void php3_ms_class_getStyle(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pIndex,  *pThis;
    classObj *self;
    int layer_id, map_id, class_id;
    styleObj *psStyle;

    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        getParameters(ht, 1, &pIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (classObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid class object.");

    if (pIndex->value.lval < 0 || pIndex->value.lval >= self->numstyles)
    {
        php3_error(E_ERROR, "Invalid style index.");
    }

    psStyle = self->styles[ pIndex->value.lval ];
    
    class_id = _phpms_fetch_property_resource(pThis, "_handle_", E_ERROR TSRMLS_CC);
    layer_id = _phpms_fetch_property_resource(pThis, "_layer_handle_", 
                                               E_ERROR TSRMLS_CC);
    map_id = _phpms_fetch_property_resource(pThis, "_map_handle_", E_ERROR TSRMLS_CC);
      
    /* Return style object */
    _phpms_build_style_object(psStyle, map_id, layer_id, class_id, list, 
                              return_value TSRMLS_CC);
}
    

DLEXPORT void php3_ms_class_clone(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    classObj *self = NULL, *pNewClass = NULL;
    layerObj *parent_layer = NULL;
    int layer_id, map_id;
    HashTable   *list=NULL;
    pThis = getThis();



    if (pThis == NULL)
      php3_error(E_ERROR, "Invalid class object.");

    self = (classObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid class object.");

    parent_layer = (layerObj*)_phpms_fetch_property_handle(pThis, "_layer_handle_",
                                                           PHPMS_GLOBAL(le_mslayer),
                                                           list TSRMLS_CC, E_ERROR);

    if ((pNewClass = classObj_clone(self, parent_layer)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }

    layer_id = _phpms_fetch_property_resource(pThis, "_layer_handle_", E_ERROR TSRMLS_CC);
    map_id = _phpms_fetch_property_resource(pThis, "_map_handle_", E_ERROR TSRMLS_CC);

     /* Return class object */
    _phpms_build_class_object(pNewClass, map_id, layer_id, list, 
                              return_value TSRMLS_CC);
}

DLEXPORT void  php3_ms_class_moveStyleUp(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pIdx;
     classObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_FAILURE;

     pThis = getThis();

     if (pThis == NULL ||
        getParameters(ht, 1, &pIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

     convert_to_long(pIdx);

     self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass), 
                                            list TSRMLS_CC);

     if (self != NULL)
     {
         nStatus = classObj_moveStyleUp(self, pIdx->value.lval);
     }

      RETURN_LONG(nStatus);
}

DLEXPORT void  php3_ms_class_moveStyleDown(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pIdx;
     classObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_FAILURE;

     pThis = getThis();

     if (pThis == NULL ||
        getParameters(ht, 1, &pIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

     convert_to_long(pIdx);

     self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass), 
                                            list TSRMLS_CC);

     if (self != NULL)
     {
         nStatus = classObj_moveStyleDown(self, pIdx->value.lval);
     }

      RETURN_LONG(nStatus);
}

DLEXPORT void  php3_ms_class_deleteStyle(INTERNAL_FUNCTION_PARAMETERS)
{
     pval        *pThis, *pIdx;
     classObj      *self=NULL;
     HashTable   *list=NULL;
     int         nStatus = MS_FAILURE;

     pThis = getThis();

     if (pThis == NULL ||
        getParameters(ht, 1, &pIdx) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

     convert_to_long(pIdx);

     self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass), 
                                            list TSRMLS_CC);

     if (self != NULL)
     {
         nStatus = classObj_deleteStyle(self, pIdx->value.lval);

         if (nStatus == MS_TRUE)
           _phpms_set_property_long(pThis,"numstyles", self->numstyles, E_ERROR TSRMLS_CC); 
     }

     RETURN_LONG(nStatus);
}



/**********************************************************************
 *                        class->getMetaData()
 **********************************************************************/

/* {{{ proto string class.getMetaData(string name)
   Return MetaData entry by name, or empty string if not found. */

DLEXPORT void php3_ms_class_getMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    classObj *self;
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

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (pszValue = classObj_getMetaData(self, pName->value.str.val)) == NULL)
    {
        pszValue = "";
    }

    RETURN_STRING(pszValue, 1);
}
/* }}} */

/**********************************************************************
 *                        class->setMetaData()
 **********************************************************************/

/* {{{ proto int class.setMetaData(string name, string value)
   Set MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_class_setMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    classObj *self;
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

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = classObj_setMetaData(self, 
                                        pName->value.str.val,  
                                        pValue->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}
/* }}} */


/**********************************************************************
 *                        class->removeMetaData()
 **********************************************************************/

/* {{{ proto int class.removeMetaData(string name)
   Remove MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_class_removeMetaData(INTERNAL_FUNCTION_PARAMETERS)
{
    classObj *self;
    pval   *pThis, *pName;
    int    nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    pThis = getThis();


    if (pThis == NULL ||
        getParameters(ht, 1, &pName) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pName);

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = classObj_removeMetaData(self, 
                                        pName->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}


/* }}} */

/*=====================================================================
 *                 PHP function wrappers - colorObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_color_object()
 **********************************************************************/
static long _phpms_build_color_object(colorObj *pcolor,
                                      HashTable *list, pval *return_value TSRMLS_DC)
{
    int color_id;

    if (pcolor == NULL)
      return 0;

    color_id = php3_list_insert(pcolor, PHPMS_GLOBAL(le_mscolor));

    _phpms_object_init(return_value, color_id, php_color_class_functions,
                       PHP4_CLASS_ENTRY(color_class_entry_ptr) TSRMLS_CC);

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
                                           list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_long(pR);
    convert_to_long(pG);
    convert_to_long(pB);

    MS_INIT_COLOR(*self, pR->value.lval, pG->value.lval, pB->value.lval);

    _phpms_set_property_long(pThis, "red",   self->red, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis, "green", self->green, E_ERROR TSRMLS_CC);
    _phpms_set_property_long(pThis, "blue",  self->blue, E_ERROR TSRMLS_CC);

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
                                      HashTable *list, pval *return_value TSRMLS_DC)
{
    int point_id;

    if (ppoint == NULL)
        return 0;

    point_id = php3_list_insert(ppoint, handle_type);

    _phpms_object_init(return_value, point_id, php_point_class_functions,
                       PHP4_CLASS_ENTRY(point_class_entry_ptr) TSRMLS_CC);

    /* editable properties */
    add_property_double(return_value,   "x",   ppoint->x);
    add_property_double(return_value,   "y",   ppoint->y);
#ifdef USE_POINT_Z_M
    add_property_double(return_value,   "z",   ppoint->z);
    add_property_double(return_value,   "m",   ppoint->m);
#endif

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
                              list, return_value TSRMLS_CC);
}
/* }}} */

/**********************************************************************
 *                        point->setXY()
 **********************************************************************/

/* {{{ proto int point.setXY(double x, double y, double m)
 3rd argument m is used for Measured shape files. It is not mandatory.
   Set new point. Returns -1 on error. */

DLEXPORT void php3_ms_point_setXY(INTERNAL_FUNCTION_PARAMETERS)
{
    pointObj    *self;
    pval        *pX, *pY, *pM, *pThis;
    int         nArgs = ARG_COUNT(ht);

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        (nArgs != 2 && nArgs != 3))
    {
        WRONG_PARAM_COUNT;
    }

    if (getParameters(ht, nArgs, &pX, &pY, &pM) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pX);
    convert_to_double(pY);

    

    self->x = pX->value.dval;
    self->y = pY->value.dval;

#ifdef USE_POINT_Z_M
    if (nArgs == 3)
    {
        convert_to_double(pM); 
        self->m = pM->value.dval;
    }
    else
      self->m = 0.0; 
#endif

    _phpms_set_property_double(pThis, "x", self->x, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis, "y", self->y, E_ERROR TSRMLS_CC);
#ifdef USE_POINT_Z_M
    _phpms_set_property_double(pThis, "m", self->y, E_ERROR TSRMLS_CC);
#endif


    RETURN_LONG(0);
}
/* }}} */


/**********************************************************************
 *                        point->setXYZ()
 **********************************************************************/

/* {{{ proto int point.setXYZ(double x, double y, double z, double m)
 4th argument m is used for Measured shape files. It is not mandatory.
   Set new point. Returns -1 on error. */

DLEXPORT void php3_ms_point_setXYZ(INTERNAL_FUNCTION_PARAMETERS)
{
    pointObj    *self;
    pval        *pX, *pY, *pZ,*pM, *pThis;
    int         nArgs = ARG_COUNT(ht);

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        (nArgs != 3 && nArgs != 4))
    {
        WRONG_PARAM_COUNT;
    }

    if (getParameters(ht, nArgs, &pX, &pY, &pZ, &pM) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pX);
    convert_to_double(pY);
    convert_to_double(pZ);

    

    self->x = pX->value.dval;
    self->y = pY->value.dval;
#ifdef USE_POINT_Z_M
    self->z = pZ->value.dval;

    if (nArgs == 4)
    {
        convert_to_double(pM); 
        self->m = pM->value.dval;
    }
    else
      self->m = 0.0; 
#endif

    _phpms_set_property_double(pThis, "x", self->x, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis, "y", self->y, E_ERROR TSRMLS_CC);
#ifdef USE_POINT_Z_M
    _phpms_set_property_double(pThis, "z", self->z, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis, "m", self->m, E_ERROR TSRMLS_CC);
#endif


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
    int                 status=MS_FAILURE;


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
                                            list TSRMLS_CC);
    poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);

    if (self && poInProj && poOutProj &&
        (status = pointObj_project(self, poInProj, poOutProj)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }
    else
    {
        // Update the members of the PHP wrapper object.
        _phpms_set_property_double(pThis, "x", self->x, E_ERROR TSRMLS_CC);
        _phpms_set_property_double(pThis, "y", self->y, E_ERROR TSRMLS_CC);
    }

    RETURN_LONG(status);
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
    imageObj    *im;
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
                                            list TSRMLS_CC);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list TSRMLS_CC);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list TSRMLS_CC);
    im = (imageObj *)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);

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
                                            list TSRMLS_CC);

    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint, 
                                               PHPMS_GLOBAL(le_mspoint_new), 
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               list TSRMLS_CC);
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
                                            list TSRMLS_CC);

    poPoint1 = (pointObj *)_phpms_fetch_handle2(pPoint1, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list TSRMLS_CC);

    poPoint2 = (pointObj *)_phpms_fetch_handle2(pPoint2, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list TSRMLS_CC);
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
                                            list TSRMLS_CC);

    poShape = (shapeObj *)_phpms_fetch_handle2(pShape, 
                                               PHPMS_GLOBAL(le_msshape_ref),
                                               PHPMS_GLOBAL(le_msshape_new),
                                               list TSRMLS_CC);
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

    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle(pThis, le_mspoint_new, list TSRMLS_CC);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval **phandle;
        if (zend_hash_find(Z_OBJPROP_P(pThis), "_handle_", 
                           sizeof("_handle_"), 
                           (void *)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
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
                                      HashTable *list, pval *return_value TSRMLS_DC)
{
    int line_id;

    if (pline == NULL)
        return 0;

    line_id = php3_list_insert(pline, handle_type);

    _phpms_object_init(return_value, line_id, php_line_class_functions,
                       PHP4_CLASS_ENTRY(line_class_entry_ptr) TSRMLS_CC);

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
                              list, return_value TSRMLS_CC);
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
    int                 status=MS_FAILURE;

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
                                           list TSRMLS_CC);

    poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);
    if (self && poInProj && poOutProj &&
        (status = lineObj_project(self, poInProj, poOutProj)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(status);
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
                                           list TSRMLS_CC);

    poPoint = (pointObj*)_phpms_fetch_handle2(pPoint,
                                              PHPMS_GLOBAL(le_mspoint_ref),
                                              PHPMS_GLOBAL(le_mspoint_new),
                                              list TSRMLS_CC);

    if (self && poPoint)
    {
        nRetVal = lineObj_add(self, poPoint);
        _phpms_set_property_long(pThis, "numpoints", self->numpoints, E_ERROR TSRMLS_CC);
    }

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        line->addXY()
 **********************************************************************/

/* {{{ proto int line.addXY(double x, double y, double m)
3rd argument m is used for Measured shape files. It is not mandatory.
   Adds a point to the end of a line */

DLEXPORT void php3_ms_line_addXY(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pX, *pY, *pM;
    lineObj     *self;
    pointObj    oPoint;
    int         nRetVal=0;
    int         nArgs = ARG_COUNT(ht);

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

     if (pThis == NULL ||
         (nArgs != 2 && nArgs != 3))
     {
         WRONG_PARAM_COUNT;
     }

    if (pThis == NULL ||
        getParameters(ht, nArgs, &pX, &pY, &pM) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pX);
    convert_to_double(pY);

    oPoint.x = pX->value.dval;
    oPoint.y = pY->value.dval;

#ifdef USE_POINT_Z_M
    oPoint.z = 0.0;

    if (nArgs == 3)
    {
        convert_to_double(pM); 
        oPoint.m = pM->value.dval;
    }
    else
      oPoint.m = 0.0;
#endif
 
    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list TSRMLS_CC);

    if (self)
    {
        nRetVal = lineObj_add(self, &oPoint);
        _phpms_set_property_long(pThis, "numpoints", self->numpoints, E_ERROR TSRMLS_CC);
    }

    RETURN_LONG(nRetVal)
}
/* }}} */



/**********************************************************************
 *                        line->addXYZ()
 **********************************************************************/

/* {{{ proto int line.addXY(double x, double y, double z, double m)
4th argument m is used for Measured shape files. It is not mandatory.
   Adds a point to the end of a line */

DLEXPORT void php3_ms_line_addXYZ(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pX, *pY, *pZ, *pM;
    lineObj     *self;
    pointObj    oPoint;
    int         nRetVal=0;
    int         nArgs = ARG_COUNT(ht);

#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

     if (pThis == NULL ||
         (nArgs != 3 && nArgs != 4))
     {
         WRONG_PARAM_COUNT;
     }

    if (pThis == NULL ||
        getParameters(ht, nArgs, &pX, &pY, &pZ,&pM) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pX);
    convert_to_double(pY);
    convert_to_double(pZ);

    oPoint.x = pX->value.dval;
    oPoint.y = pY->value.dval;
#ifdef USE_POINT_Z_M
    oPoint.z = pY->value.dval;

    if (nArgs == 4)
    {
        convert_to_double(pM); 
        oPoint.m = pM->value.dval;
    }
    else
      oPoint.m = 0.0;
#endif
 
    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list TSRMLS_CC);

    if (self)
    {
        nRetVal = lineObj_add(self, &oPoint);
        _phpms_set_property_long(pThis, "numpoints", self->numpoints, E_ERROR TSRMLS_CC);
    }

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
                                           list TSRMLS_CC);

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
                                list, return_value TSRMLS_CC);
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

    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (lineObj *)_phpms_fetch_handle(pThis, le_msline_new, list TSRMLS_CC);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval **phandle;
        if (zend_hash_find(Z_OBJPROP_P(pThis), "_handle_", 
                           sizeof("_handle_"), 
                           (void *)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
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
                                      HashTable *list, pval *return_value TSRMLS_DC)
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
                       PHP4_CLASS_ENTRY(shape_class_entry_ptr) TSRMLS_CC);

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
                             list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "bounds", new_obj_ptr,E_ERROR TSRMLS_CC);

    /* Package values as an associative array
     * For now we do this only for shapes returned from map layers, and not
     * for those from shapefileObj.
     */
    if (pLayer && 
        ((pshape->numvalues == pLayer->numitems) ||
         (pshape->numvalues == 0 && pLayer->numitems == -1)))
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
                                   new_obj_ptr, E_ERROR TSRMLS_CC);
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
                              list, return_value TSRMLS_CC);
}
/* }}} */


/**********************************************************************
 *                        ms_newShapeObj()
 **********************************************************************/

/* {{{ proto shapeObj ms_shapeObjFromWkt()
   Create a new shapeObj instance from a WKT string. */

DLEXPORT void php3_ms_shape_fromwkt(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pWkt;
    shapeObj *pNewShape;
    HashTable   *list=NULL;

    if (getParameters(ht, 1, &pWkt) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pWkt);

    if ((pNewShape = msShapeFromWKT(pWkt->value.str.val)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(pNewShape, PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value TSRMLS_CC);
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
                                            list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "text",         self->text)
    else IF_SET_LONG(  "classindex",   self->classindex)
    else IF_SET_LONG(  "index",        self->index)
    else if (strcmp( "numlines",  pPropertyName->value.str.val) == 0 ||
             strcmp( "type",      pPropertyName->value.str.val) == 0 ||
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
    int                 status=MS_FAILURE;

    HashTable   *list=NULL;
    pval   **pBounds;


    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 2, &pIn, &pOut) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

     self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msshape_ref),
                                           PHPMS_GLOBAL(le_msshape_new),
                                           list TSRMLS_CC);
     poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);

    if (self && poInProj && poOutProj &&
        (status = shapeObj_project(self, poInProj, poOutProj)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }
    else
    {
         if (zend_hash_find(Z_OBJPROP_P(pThis), "bounds", 
                            sizeof("bounds"), (void *)&pBounds) == SUCCESS)
         {
             _phpms_set_property_double((*pBounds),"minx", self->bounds.minx, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pBounds),"miny", self->bounds.miny, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pBounds),"maxx", self->bounds.maxx, 
                                        E_ERROR TSRMLS_CC);
             _phpms_set_property_double((*pBounds),"maxy", self->bounds.maxy, 
                                        E_ERROR TSRMLS_CC);
         }
    }

    RETURN_LONG(status);
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
                                           list TSRMLS_CC);

    poLine = (lineObj*)_phpms_fetch_handle2(pLine,
                                            PHPMS_GLOBAL(le_msline_ref),
                                            PHPMS_GLOBAL(le_msline_new),
                                            list TSRMLS_CC);

    if (self && poLine)
    {
        nRetVal = shapeObj_add(self, poLine);
        _phpms_set_property_long(pThis, "numlines", self->numlines, E_ERROR TSRMLS_CC);
    }

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
                                           list TSRMLS_CC);

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
                                list, return_value TSRMLS_CC);
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
    imageObj    *im;
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
                                            list TSRMLS_CC);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list TSRMLS_CC);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list TSRMLS_CC);
    im = (imageObj *)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);

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
                                            list TSRMLS_CC);
     poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list TSRMLS_CC);
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
                                            list TSRMLS_CC);
    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
    
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
                                            list TSRMLS_CC);

    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list TSRMLS_CC);
    
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
 *                        shape->getpointusingmeasure()
 **********************************************************************/
/* {{{ proto int shape.getpointusingmeasure(double m)
   Given a shape and a nmesure, return a point object containing the XY
   location corresponding to the measure */

DLEXPORT void php3_ms_shape_getpointusingmeasure(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pMeasure;
    shapeObj     *self = NULL;
    pointObj    *point = NULL;
#ifdef PHP4
    HashTable   *list=NULL;
#endif

#ifdef PHP4
    pThis = getThis();
#else
    getThis(&pThis);
#endif

    if (pThis == NULL ||
        getParameters(ht, 1, &pMeasure) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pMeasure);

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    point = shapeObj_getpointusingmeasure(self, pMeasure->value.dval);
    if (point == NULL)
       RETURN_FALSE;
        
    _phpms_build_point_object(point, 
                              PHPMS_GLOBAL(le_mspoint_ref), 
                              list, return_value TSRMLS_CC);
}


/**********************************************************************
 *                        shape->getmeasureusingpoint()
 **********************************************************************/
/* {{{ proto int shape.getpointusingmeasure(pointObject point)
   Given a shape and a point object, return a point object containing the XY
   location of the intersection between the point and the shape. The point
   return contains also the extrapolated M value at the intersection. */

DLEXPORT void php3_ms_shape_getmeasureusingpoint(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pPoint;
    shapeObj     *self = NULL;
    pointObj    *point = NULL;
    pointObj    *intersection = NULL;
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

    point = (pointObj *)_phpms_fetch_handle2(pPoint,
                                             PHPMS_GLOBAL(le_mspoint_ref),
                                             PHPMS_GLOBAL(le_mspoint_new),
                                             list TSRMLS_CC);
    if (point == NULL)
      RETURN_FALSE;


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    intersection = shapeObj_getmeasureusingpoint(self, point);
    if (intersection == NULL)
      RETURN_FALSE;

     _phpms_build_point_object(intersection, 
                              PHPMS_GLOBAL(le_mspoint_ref), 
                              list, return_value TSRMLS_CC);
}
 


/**********************************************************************
 *                        shape->buffer(width)
 **********************************************************************/
/* {{{ proto int shape.buffer(double width)
   Given a shape and a width, return a shape object with a buffer using
   underlying GEOS library*/

DLEXPORT void php3_ms_shape_buffer(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pWidth;
    shapeObj     *self = NULL;
    shapeObj    *return_shape = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pWidth) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pWidth);

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    return_shape = shapeObj_buffer(self, pWidth->value.dval);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value TSRMLS_CC);
}
 


/**********************************************************************
 *                        shape->topologypreservingsimplify(tolerance)
 **********************************************************************/
/* {{{ proto int shape.topologypreservingsimplify(double tolerance)
   Given a shape and a tolerance, return a simplified shape object using 
   underlying GEOS library*/

DLEXPORT void php3_ms_shape_topologypreservingsimplify(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pTolerance;
    shapeObj     *self = NULL;
    shapeObj    *return_shape = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pTolerance) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pTolerance);

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    return_shape = shapeObj_topologypreservingsimplify(self, 
                                                     pTolerance->value.dval);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value TSRMLS_CC);
}

/**********************************************************************
 *                        shape->simplify(tolerance)
 **********************************************************************/
/* {{{ proto int shape.simplify(double tolerance)
   Given a shape and a tolerance, return a simplified shape object using 
   underlying GEOS library*/

DLEXPORT void php3_ms_shape_simplify(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pTolerance;
    shapeObj     *self = NULL;
    shapeObj    *return_shape = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pTolerance) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pTolerance);

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    return_shape = shapeObj_simplify(self, pTolerance->value.dval);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value TSRMLS_CC);
}
 

/**********************************************************************
 *                        shape->convexhull()
 **********************************************************************/
/* {{{ proto int shape.convexhull()
   Given a shape, return a shape representing the convex hull using
   underlying GEOS library*/

DLEXPORT void php3_ms_shape_convexhull(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    shapeObj     *self = NULL;
    shapeObj    *return_shape = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL)
      WRONG_PARAM_COUNT;
    


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    return_shape = shapeObj_convexHull(self);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new),  NULL,
                              list, return_value TSRMLS_CC);
}
   


DLEXPORT void php3_ms_shape_boundary(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    shapeObj     *self = NULL;
    shapeObj    *return_shape = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL)
      WRONG_PARAM_COUNT;
    


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    return_shape = shapeObj_boundary(self);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new),  NULL,
                              list, return_value TSRMLS_CC);
}
   

/**********************************************************************
 *                        shape->contains_geos()
 **********************************************************************/
/* {{{ proto int shape.contains(shapeobj shape)
   Return true or false if the given shape in argument 1 is contained
   in the shape. Use3d underlying msGEOSContains GEOS library*/

DLEXPORT void php3_ms_shape_contains_geos(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;


    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    if (shapeObj_contains_geos(self, poShape))
    {
      RETURN_TRUE;
    }
    else
      RETURN_FALSE; 
}
 
 

/**********************************************************************
 *                        shape->Union()
 **********************************************************************/
/* {{{ proto int shape.Union(shapeobj shape)
   Return a shape which is the union of the current shape and the one
   given in argument 1 . Uses underlying GEOS library*/

DLEXPORT void php3_ms_shape_Union(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;
    shapeObj    *return_shape = NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    return_shape = shapeObj_Union(self, poShape);
    
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new),  NULL,
                              list, return_value TSRMLS_CC);
}
 


/**********************************************************************
 *                        shape->intersection()
 **********************************************************************/
/* {{{ proto int shape.intersection(shapeobj shape)
   Return a shape which is the intersection of the current shape and the one
   given in argument 1 . Uses underlying GEOS library*/

DLEXPORT void php3_ms_shape_intersection(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;
    shapeObj    *return_shape = NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    return_shape = shapeObj_intersection(self, poShape);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new),  NULL,
                              list, return_value TSRMLS_CC);

}



/**********************************************************************
 *                        shape->difference()
 **********************************************************************/
/* {{{ proto int shape.difference(shapeobj shape)
   Return a shape which is the difference of the current shape and the one
   given in argument 1 . Uses underlying GEOS library*/

DLEXPORT void php3_ms_shape_difference(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;
    shapeObj    *return_shape = NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    return_shape = shapeObj_difference(self, poShape);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new),  NULL,
                              list, return_value TSRMLS_CC);
}

/**********************************************************************
 *                        shape->symdifference()
 **********************************************************************/
/* {{{ proto int shape.symdifference(shapeobj shape)
   Return a shape which is the symetrical difference of the current shape and 
   the one given in argument 1 . Uses underlying GEOS library*/

DLEXPORT void php3_ms_shape_symdifference(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;
    shapeObj    *return_shape = NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    return_shape = shapeObj_symdifference(self, poShape);
    if (return_shape  == NULL)
       RETURN_FALSE;
        
    _phpms_build_shape_object(return_shape, 
                              PHPMS_GLOBAL(le_msshape_new),  NULL,
                              list, return_value TSRMLS_CC);
}



/**********************************************************************
 *                        shape->overlaps()
 **********************************************************************/
/* {{{ proto int shape.overlaps(shapeobj shape)
   Return true if the given shape in argument 1 overlaps
   the shape. Else return false. */

DLEXPORT void php3_ms_shape_overlaps(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;


    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    if (shapeObj_overlaps(self, poShape))
    {
      RETURN_TRUE;
    }
    else
      RETURN_FALSE; 
}


/**********************************************************************
 *                        shape->within()
 **********************************************************************/
/* {{{ proto int shape.within(shapeobj shape)
   Return true if the given shape in argument 1 is within
   the shape. Else return false. */

DLEXPORT void php3_ms_shape_within(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    if (shapeObj_within(self, poShape))
    {
      RETURN_TRUE;
    }
    else
      RETURN_FALSE; 
}


/**********************************************************************
 *                        shape->crosses()
 **********************************************************************/
/* {{{ proto int shape.crosses(shapeobj shape)
   Return true if the given shape in argument 1 crosses
   the shape. Else return false. */

DLEXPORT void php3_ms_shape_crosses(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;


    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    if (shapeObj_crosses(self, poShape))
    {
      RETURN_TRUE;
    }
    else
      RETURN_FALSE; 
}


/**********************************************************************
 *                        shape->touches()
 **********************************************************************/
/* {{{ proto int shape.touches(shapeobj shape)
   Return true if the given shape in argument 1 touches
   the shape. Else return false. */
DLEXPORT void php3_ms_shape_touches(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;


    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    if (shapeObj_touches(self, poShape))
    {
      RETURN_TRUE;
    }
    else
      RETURN_FALSE; 
}


/**********************************************************************
 *                        shape->equals()
 **********************************************************************/
/* {{{ proto int shape.equals(shapeobj shape)
   Return true if the given shape in argument 1 is equal to
   the shape. Else return false. */
DLEXPORT void php3_ms_shape_equals(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;


    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    if (shapeObj_equals(self, poShape))
    {
      RETURN_TRUE;
    }
    else
      RETURN_FALSE; 
}

/**********************************************************************
 *                        shape->disjoint()
 **********************************************************************/
/* {{{ proto int shape.disjoint(shapeobj shape)
   Return true if the given shape in argument 1 is disjoint to
   the shape. Else return false. */
DLEXPORT void php3_ms_shape_disjoint(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis, *pShape;
    shapeObj     *self = NULL;
    shapeObj    *poShape;
    HashTable   *list=NULL;


    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    poShape = 
      (shapeObj *)_phpms_fetch_handle2(pShape, 
                                     PHPMS_GLOBAL(le_msshape_ref),
                                     PHPMS_GLOBAL(le_msshape_new),
                                     list TSRMLS_CC);
       
    if (poShape  == NULL)
       RETURN_FALSE;
        
    if (shapeObj_disjoint(self, poShape))
    {
      RETURN_TRUE;
    }
    else
      RETURN_FALSE; 
}


/**********************************************************************
 *                        shape->getcentroid()
 **********************************************************************/
/* {{{ proto int shape.getcentroid()
   Given a shape, return a point pbject representing the centroid */

DLEXPORT void php3_ms_shape_getcentroid(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    shapeObj     *self = NULL;
    pointObj    *return_point = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL)
      WRONG_PARAM_COUNT;
    


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    return_point = shapeObj_getcentroid(self);
    if (return_point  == NULL)
       RETURN_FALSE;
        
    _phpms_build_point_object(return_point, 
                              PHPMS_GLOBAL(le_mspoint_new),
                              list, return_value TSRMLS_CC);
}
 

/**********************************************************************
 *                        shape->getarea()
 **********************************************************************/
/* {{{ proto int shape.getarea()
   Returns the area  of the shape */
DLEXPORT void php3_ms_shape_getarea(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    shapeObj     *self = NULL;
    HashTable   *list=NULL;
    double      dfArea = 0;

    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    dfArea = shapeObj_getarea(self);

    RETURN_DOUBLE(dfArea);
}
   
/**********************************************************************
 *                        shape->getlength()
 **********************************************************************/
/* {{{ proto int shape.getlength()
   Returns the length  of the shape */
DLEXPORT void php3_ms_shape_getlength(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    shapeObj     *self = NULL;
    HashTable   *list=NULL;
    double      dfLength = 0;

    pThis = getThis();

    if (pThis == NULL)
    {
        RETURN_FALSE;;
    }


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    dfLength = shapeObj_getlength(self);

    RETURN_DOUBLE(dfLength);
}
   

/**********************************************************************
 *                        shape->getLabelPoint()
 **********************************************************************/
/* {{{ proto int shape.getlabelpoint()
   Given a shape, return a point object suitable for labelling it */

DLEXPORT void php3_ms_shape_getlabelpoint(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    shapeObj     *self = NULL;
    pointObj    *return_point = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL)
      WRONG_PARAM_COUNT;
    


    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    return_point = shapeObj_getLabelPoint(self);
    if (return_point  == NULL)
       RETURN_FALSE;
        
    _phpms_build_point_object(return_point, 
                              PHPMS_GLOBAL(le_mspoint_new),
                              list, return_value TSRMLS_CC);
}
 

/**********************************************************************
 *                        shape->toWkt()
 **********************************************************************/
/* {{{ proto string shape.toWkt()
   Returns WKT representation of geometry. */

DLEXPORT void php3_ms_shape_towkt(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    shapeObj    *self;
    HashTable   *list=NULL;
    char        *pszWKT = NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);

    if (self && (pszWKT = msShapeToWKT(self)) )
    {
        RETVAL_STRING(pszWKT, 1);
        msFree(pszWKT);
        return;
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
    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle(pThis, le_msshape_new, list TSRMLS_CC);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval **phandle;
        if (zend_hash_find(Z_OBJPROP_P(pThis), "_handle_", 
                           sizeof("_handle_"), 
                           (void *)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
    }

}
/* }}} */

/**********************************************************************
 *                        shape->setbounds()
 **********************************************************************/

/* {{{ proto int shape.project(projectionObj in, projectionObj out)
   calculate new bounding . Returns MS_SUCCESS/MS_FAILURE*/

DLEXPORT void php3_ms_shape_setbounds(INTERNAL_FUNCTION_PARAMETERS)
{
  pval                *pThis;
    shapeObj            *self;

    HashTable   *list=NULL;
    pval   **pBounds;


    pThis = getThis();

    if (pThis == NULL)
      RETURN_FALSE;

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    shapeObj_setBounds(self);
   

    if (zend_hash_find(Z_OBJPROP_P(pThis), "bounds", 
                       sizeof("bounds"), (void *)&pBounds) == SUCCESS)
    {
        _phpms_set_property_double((*pBounds),"minx", self->bounds.minx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pBounds),"miny", self->bounds.miny, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pBounds),"maxx", self->bounds.maxx, 
                                   E_ERROR TSRMLS_CC);
        _phpms_set_property_double((*pBounds),"maxy", self->bounds.maxy, 
                                   E_ERROR TSRMLS_CC);
    }
    
    RETURN_TRUE;
}
/* }}} */

/*=====================================================================
 *                 PHP function wrappers - webObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_web_object()
 **********************************************************************/
static long _phpms_build_web_object(webObj *pweb, 
                                    HashTable *list, pval *return_value TSRMLS_DC)
{
    int         web_id;
    pval        *new_obj_ptr;

    if (pweb == NULL)
        return 0;

    web_id = php3_list_insert(pweb, PHPMS_GLOBAL(le_msweb));

    _phpms_object_init(return_value, web_id, php_web_class_functions,
                       PHP4_CLASS_ENTRY(web_class_entry_ptr) TSRMLS_CC);

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

    add_property_double(return_value, "minscaledenom",  pweb->minscaledenom);
    add_property_double(return_value, "maxscaledenom",  pweb->maxscaledenom);
    /* TODO: *scale deprecated in v5.0. Remove in future release */
    add_property_double(return_value, "minscale",       pweb->minscaledenom);
    add_property_double(return_value, "maxscale",       pweb->maxscaledenom);

    PHPMS_ADD_PROP_STR(return_value,  "queryformat",    pweb->queryformat);
    PHPMS_ADD_PROP_STR(return_value,  "legendformat",   pweb->legendformat);
    PHPMS_ADD_PROP_STR(return_value,  "browseformat",   pweb->browseformat);
    
    
    MAKE_STD_ZVAL(new_obj_ptr);
    _phpms_build_rect_object(&(pweb->extent), PHPMS_GLOBAL(le_msrect_ref), 
                             list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "extent", new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);
    _phpms_build_hashtable_object(&(pweb->metadata),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "metadata",new_obj_ptr,E_ERROR TSRMLS_CC);

    return web_id;
}

/**********************************************************************
 *                        web->updateFromString()
 **********************************************************************/

/* {{{ proto int web.updateFromString(string snippet)
   Update a web from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_web_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    webObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (webObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msweb),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = webObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
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
                                         list TSRMLS_CC);
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

    
    IF_SET_STRING(       "log",           self->log)
    else IF_SET_STRING(  "imagepath",     self->imagepath)
    else IF_SET_STRING(  "template",      self->template) 
    else IF_SET_STRING(  "imageurl",      self->imageurl)
    else IF_SET_STRING(  "header",        self->header)
    else IF_SET_STRING(  "footer",        self->footer)
    else IF_SET_STRING(  "mintemplate",   self->mintemplate) 
    else IF_SET_STRING(  "maxtemplate",   self->maxtemplate) 
    else IF_SET_STRING(  "queryformat",   self->queryformat) 
    else IF_SET_STRING(  "legendformat",  self->legendformat) 
    else IF_SET_STRING(  "browseformat",  self->browseformat) 
    else IF_SET_LONG(    "minscaledenom", self->minscaledenom)
    else IF_SET_LONG(    "maxscaledenom", self->maxscaledenom)  
    /* TODO: *scale deprecated in v5.0. Remove in future release */
    else IF_SET_LONG(    "minscale",      self->minscaledenom)
    else IF_SET_LONG(    "maxscale",      self->maxscaledenom)  
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
                                     HashTable *list, pval *return_value TSRMLS_DC)
{
    int rect_id;

    if (prect == NULL)
      return 0;

    rect_id = php3_list_insert(prect, handle_type);

    _phpms_object_init(return_value, rect_id, php_rect_class_functions,
                       PHP4_CLASS_ENTRY(rect_class_entry_ptr) TSRMLS_CC);

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
                             list, return_value TSRMLS_CC);
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
    int                 status=MS_FAILURE;

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
                                           PHPMS_GLOBAL(le_msrect_new), list TSRMLS_CC);
    poInProj = 
        (projectionObj*)_phpms_fetch_handle(pIn, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);
    poOutProj = 
        (projectionObj*)_phpms_fetch_handle(pOut, 
                                            PHPMS_GLOBAL(le_msprojection_new), 
                                            list TSRMLS_CC);

    if (self && poInProj && poOutProj &&
        (status = rectObj_project(self, poInProj, poOutProj)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_WARNING);
    }
    else
    {
        // Update the members of the PHP wrapper object.
        _phpms_set_property_double(pThis, "minx", self->minx, E_ERROR TSRMLS_CC);
        _phpms_set_property_double(pThis, "miny", self->miny, E_ERROR TSRMLS_CC);
        _phpms_set_property_double(pThis, "maxx", self->maxx, E_ERROR TSRMLS_CC);
        _phpms_set_property_double(pThis, "maxy", self->maxy, E_ERROR TSRMLS_CC);
    }

    RETURN_LONG(status);
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
                                           PHPMS_GLOBAL(le_msrect_new), list TSRMLS_CC);
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
                                           list TSRMLS_CC);
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

    _phpms_set_property_double(pThis, "minx", self->minx, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis, "miny", self->miny, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis, "maxx", self->maxx, E_ERROR TSRMLS_CC);
    _phpms_set_property_double(pThis, "maxy", self->maxy, E_ERROR TSRMLS_CC);

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
    imageObj    *im;
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
                                            list TSRMLS_CC);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list TSRMLS_CC);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list TSRMLS_CC);
    im = (imageObj *)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list TSRMLS_CC);

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
                                           list TSRMLS_CC);

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
    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (rectObj *)_phpms_fetch_handle(pThis, le_msrect_new, list TSRMLS_CC);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval **phandle;
        if (zend_hash_find(Z_OBJPROP_P(pThis), "_handle_", 
                           sizeof("_handle_"), 
                           (void *)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
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
                                    HashTable *list, pval *return_value TSRMLS_DC)
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
                       PHP4_CLASS_ENTRY(reference_class_entry_ptr) TSRMLS_CC);

    PHPMS_ADD_PROP_STR(return_value,  "image",   preference->image);
    add_property_long(return_value,   "width",  preference->width);
    add_property_long(return_value,   "height",  preference->height);
    add_property_long(return_value,   "status",  preference->status);
    
#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_rect_object(&(preference->extent), 
                             PHPMS_GLOBAL(le_msrect_ref),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "extent", new_obj_ptr, E_ERROR TSRMLS_CC);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_color_object(&(preference->color),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "color", new_obj_ptr, E_ERROR TSRMLS_CC);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_color_object(&(preference->outlinecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "outlinecolor", 
                               new_obj_ptr, E_ERROR TSRMLS_CC);

    return reference_id;
}

/**********************************************************************
 *                        referenceMap->updateFromString()
 **********************************************************************/

/* {{{ proto int referenceMap.updateFromString(string snippet)
   Update a referenceMap from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_referenceMap_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    referenceMapObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (referenceMapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msrefmap),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = referenceMapObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
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
                                                  list TSRMLS_CC);
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
        php3_error(E_ERROR,"Property '%s' is an object and cannot be set using set().  Use the %s object's methods instead.", 
                            pPropertyName->value.str.val, 
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
                                          HashTable *list, pval *return_value TSRMLS_DC)
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
                       PHP4_CLASS_ENTRY(shapefile_class_entry_ptr) TSRMLS_CC);

    /* read-only properties */
    add_property_long(return_value, "numshapes",  pshapefile->numshapes);
    add_property_long(return_value, "type",       pshapefile->type);
    PHPMS_ADD_PROP_STR(return_value,"source",     pshapefile->source);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_rect_object(&(pshapefile->bounds), 
                             PHPMS_GLOBAL(le_msrect_ref), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "bounds", new_obj_ptr,E_ERROR TSRMLS_CC);

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
    _phpms_build_shapefile_object(pNewObj, list, return_value TSRMLS_CC);
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
                                               list TSRMLS_CC);

    poShape = (shapeObj*)_phpms_fetch_handle2(pShape,
                                              PHPMS_GLOBAL(le_msshape_ref),
                                              PHPMS_GLOBAL(le_msshape_new),
                                              list TSRMLS_CC);

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
                                               list TSRMLS_CC);

    poPoint = (pointObj*)_phpms_fetch_handle2(pPoint,
                                              PHPMS_GLOBAL(le_mspoint_ref),
                                              PHPMS_GLOBAL(le_mspoint_new),
                                              list TSRMLS_CC);

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
                                               list TSRMLS_CC);

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
        php3_error(E_ERROR, "Failed reading shape %ld.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value TSRMLS_CC);
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
                                               list TSRMLS_CC);

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
        php3_error(E_ERROR, "Failed reading point %ld.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return point object */
    _phpms_build_point_object(poPoint, PHPMS_GLOBAL(le_mspoint_new),
                              list, return_value TSRMLS_CC);
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
                                               list TSRMLS_CC);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list TSRMLS_CC);

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
        php3_error(E_ERROR, "Failed reading shape %ld.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), NULL,
                              list, return_value TSRMLS_CC);
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
                                               list TSRMLS_CC);

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
                              list, return_value TSRMLS_CC);
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
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapefileObj*)_phpms_fetch_handle(pThis, le_msshapefile, list TSRMLS_CC);
    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval **phandle;

        if (zend_hash_find(Z_OBJPROP_P(pThis), "_handle_", 
                           sizeof("_handle_"), 
                           (void *)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }

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
                                                  HashTable *list TSRMLS_DC, 
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
                                           HashTable *list, pval *return_value TSRMLS_DC)
{
    int projection_id;

    if (pproj == NULL)
      return 0;

    projection_id = php3_list_insert(pproj, handle_type);

    _phpms_object_init(return_value, projection_id, 
                       php_projection_class_functions,
                       PHP4_CLASS_ENTRY(projection_class_entry_ptr) TSRMLS_CC);

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
                                   list, return_value TSRMLS_CC);
}
/* }}} */

/**********************************************************************
 *                        projection->getunits()
 **********************************************************************/

/* {{{ proto int projection.getunits()
   Returns the units of a projection object */
DLEXPORT void php3_ms_projection_getunits(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    projectionObj *self;
    long lValue = -1;

    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (projectionObj *)_phpms_fetch_handle2(pThis, 
                                                 PHPMS_GLOBAL(le_msprojection_new), 
                                                 PHPMS_GLOBAL(le_msprojection_ref),
                                                 list TSRMLS_CC);

    if (self)
    {
       lValue = projectionObj_getUnits(self);
    } else {
       php3_error(E_ERROR, "Invalid projection object.");
    }

    RETURN_LONG(lValue);
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

    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (projectionObj *)_phpms_fetch_handle(pThis, 
                                                le_msprojection_new, list TSRMLS_CC);

    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval **phandle;
        if (zend_hash_find(Z_OBJPROP_P(pThis), "_handle_", 
                           sizeof("_handle_"), 
                           (void *)&phandle) == SUCCESS)
        {
            php3_list_delete((*phandle)->value.lval);
        }
    }

}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - scalebarObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_scalebar_object()
 **********************************************************************/
static long _phpms_build_scalebar_object(scalebarObj *pscalebar, 
                                         HashTable *list, pval *return_value TSRMLS_DC)
{
    int         scalebar_id;
    pval        *new_obj_ptr;

    if (pscalebar == NULL)
        return 0;

    scalebar_id = php3_list_insert(pscalebar, PHPMS_GLOBAL(le_msscalebar));

    _phpms_object_init(return_value, scalebar_id, php_scalebar_class_functions,
                       PHP4_CLASS_ENTRY(scalebar_class_entry_ptr) TSRMLS_CC);

    add_property_long(return_value,  "height",          pscalebar->height);
    add_property_long(return_value,  "width",           pscalebar->width);
    add_property_long(return_value,  "style",           pscalebar->style);
    add_property_long(return_value,  "intervals",       pscalebar->intervals);

    add_property_long(return_value,  "units",           pscalebar->units);
    add_property_long(return_value,  "status",          pscalebar->status);
    add_property_long(return_value,  "position",        pscalebar->position);
    add_property_long(return_value,  "transparent",     pscalebar->transparent);
    add_property_long(return_value,  "interlace",       pscalebar->interlace);
    add_property_long(return_value,  "postlabelcache",  
                      pscalebar->postlabelcache);
    add_property_long(return_value,  "align",           pscalebar->align);

    
    MAKE_STD_ZVAL(new_obj_ptr);
    _phpms_build_label_object(&(pscalebar->label), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "label", new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pscalebar->imagecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "imagecolor",new_obj_ptr,E_ERROR TSRMLS_CC);


    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pscalebar->color),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "color",new_obj_ptr,E_ERROR TSRMLS_CC);


    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pscalebar->backgroundcolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "backgroundcolor",new_obj_ptr,E_ERROR TSRMLS_CC);

  
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pscalebar->outlinecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "outlinecolor",new_obj_ptr,E_ERROR TSRMLS_CC);
    
    return scalebar_id;
}

/**********************************************************************
 *                        scalebar->updateFromString()
 **********************************************************************/

/* {{{ proto int scalebar.updateFromString(string snippet)
   Update a scalebar from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_scalebar_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    scalebarObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (scalebarObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msscalebar),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = scalebarObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}

/**********************************************************************
 *                        scalebar->set()
 **********************************************************************/

/* {{{ proto int scalebar.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */


DLEXPORT void php3_ms_scalebar_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    scalebarObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = 
        (scalebarObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msscalebar),
                                         list TSRMLS_CC);
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

    
    IF_SET_LONG(       "height",        self->height)
    else IF_SET_LONG(  "width",         self->width)
    else IF_SET_LONG(  "style",         self->style)
    else IF_SET_LONG(  "intervals",     self->intervals)
    else IF_SET_LONG(  "units",         self->units)
    else IF_SET_LONG(  "status",        self->status)
    else IF_SET_LONG(  "position",      self->position)
    else IF_SET_LONG(  "transparent",   self->transparent)
    else IF_SET_LONG(  "interlace",     self->interlace)
    else IF_SET_LONG(  "postlabelcache",self->postlabelcache)
    else IF_SET_LONG(  "align",         self->align)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}           


/**********************************************************************
 *                        scalebar->setimagecolor()
 **********************************************************************/

/* {{{ proto int scalebar.set(int red, int green, int blue)
   Set the imagecolor property of the scalebar. Returns -1 on error. */
DLEXPORT void php3_ms_scalebar_setImageColor(INTERNAL_FUNCTION_PARAMETERS)
{
    scalebarObj *self;
    pval        *pThis, *pR, *pG, *pB;
    int         r, g, b = 0;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 3, &pR, &pG, &pB) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = 
        (scalebarObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msscalebar),
                                         list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_FALSE;
    }
    
    convert_to_long(pR);
    convert_to_long(pG);
    convert_to_long(pB);

    r = pR->value.lval;
    g = pG->value.lval;
    b = pB->value.lval;

/* -------------------------------------------------------------------- */
/*      validate values                                                 */
/* -------------------------------------------------------------------- */
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
    {
        RETURN_FALSE;
    }
    
    self->imagecolor.red = r;
    self->imagecolor.green = g;
    self->imagecolor.blue = b;

    RETURN_TRUE;
}           

/*=====================================================================
 *                 PHP function wrappers - legendObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_legend_object()
 **********************************************************************/
static long _phpms_build_legend_object(legendObj *plegend, 
                                       HashTable *list, pval *return_value TSRMLS_DC)
{
    int         legend_id;
#ifdef PHP4
    pval        *new_obj_ptr;
#else
    pval        new_obj_param;  /* No, it's not a pval * !!! */
    pval        *new_obj_ptr;
    new_obj_ptr = &new_obj_param;
#endif

    if (plegend == NULL)
        return 0;

    legend_id = php3_list_insert(plegend, PHPMS_GLOBAL(le_mslegend));

    _phpms_object_init(return_value, legend_id, php_legend_class_functions,
                       PHP4_CLASS_ENTRY(legend_class_entry_ptr) TSRMLS_CC);

    add_property_long(return_value,  "height",          plegend->height);
    add_property_long(return_value,  "width",           plegend->width);
    add_property_long(return_value,  "keysizex",        plegend->keysizex);
    add_property_long(return_value,  "keysizey",        plegend->keysizey);
    add_property_long(return_value,  "keyspacingx",     plegend->keyspacingx);
    add_property_long(return_value,  "keyspacingy",     plegend->keyspacingy);
    add_property_long(return_value,  "status",          plegend->status);
    add_property_long(return_value,  "position",        plegend->position);
    add_property_long(return_value,  "transparent",     plegend->transparent);
    add_property_long(return_value,  "interlace",       plegend->interlace);
    add_property_long(return_value,  "postlabelcache",  
                      plegend->postlabelcache);
    PHPMS_ADD_PROP_STR(return_value, "template", plegend->template);
    
    
#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);
#endif
    _phpms_build_label_object(&(plegend->label), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "label", new_obj_ptr,E_ERROR TSRMLS_CC);

#ifdef PHP4
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
#endif
    _phpms_build_color_object(&(plegend->imagecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "imagecolor",new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(plegend->outlinecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "outlinecolor",new_obj_ptr,E_ERROR TSRMLS_CC);

    return legend_id;
}

/**********************************************************************
 *                        legend->updateFromString()
 **********************************************************************/

/* {{{ proto int legend.updateFromString(string snippet)
   Update a legend from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_legend_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    legendObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (legendObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslegend),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = legendObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}


/* }}} */


/**********************************************************************
 *                        legend->set()
 **********************************************************************/

/* {{{ proto int legend.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */


DLEXPORT void php3_ms_legend_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    legendObj *self;
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

    self = 
        (legendObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslegend),
                                         list TSRMLS_CC);
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

    
    IF_SET_LONG(       "height",        self->height)
    else IF_SET_LONG(  "width",         self->width)
    else IF_SET_LONG(  "keysizex",      self->keysizex)
    else IF_SET_LONG(  "keysizey",      self->keysizey)
    else IF_SET_LONG(  "keyspacingx",   self->keyspacingx)
    else IF_SET_LONG(  "keyspacingy",   self->keyspacingy)
    else IF_SET_LONG(  "status",        self->status)
    else IF_SET_LONG(  "position",      self->position)
    else IF_SET_LONG(  "transparent",   self->transparent)
    else IF_SET_LONG(  "interlace",     self->interlace)
    else IF_SET_LONG(  "postlabelcache",self->postlabelcache)
    else IF_SET_STRING( "template",     self->template)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in legend object.", 
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}           

/*=====================================================================
 *                 PHP function wrappers - styleObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_style_object()
 **********************************************************************/
static long _phpms_build_style_object(styleObj *pstyle, int parent_map_id, 
                                      int parent_layer_id, 
                                      int parent_class_id,
                                      HashTable *list, 
                                      pval *return_value TSRMLS_DC)
{
    int style_id;

    pval        *new_obj_ptr;


    if (pstyle == NULL)
        return 0;

    style_id = php3_list_insert(pstyle, PHPMS_GLOBAL(le_msstyle));

    _phpms_object_init(return_value, style_id, php_style_class_functions,
                       PHP4_CLASS_ENTRY(style_class_entry_ptr) TSRMLS_CC);

    add_property_resource(return_value, "_class_handle_", parent_class_id);
    zend_list_addref(parent_class_id);

    add_property_resource(return_value, "_layer_handle_", parent_layer_id);
    zend_list_addref(parent_layer_id);

    add_property_resource(return_value, "_map_handle_", parent_map_id);
    zend_list_addref(parent_map_id);


    /* editable properties */
    add_property_long(return_value,   "symbol",         pstyle->symbol);
    PHPMS_ADD_PROP_STR(return_value,  "symbolname",     pstyle->symbolname);
    add_property_double(return_value, "size",           pstyle->size);
    add_property_double(return_value, "minsize",        pstyle->minsize);
    add_property_double(return_value, "maxsize",        pstyle->maxsize);
    add_property_double(return_value, "width",          pstyle->width);
    add_property_double(return_value, "minwidth",       pstyle->minwidth);
    add_property_double(return_value, "maxwidth",       pstyle->maxwidth);
    add_property_double(return_value,   "offsetx",        pstyle->offsetx);
    add_property_double(return_value,   "offsety",        pstyle->offsety);
    add_property_double(return_value, "angle",          pstyle->angle);
    add_property_long(return_value,   "antialias",      pstyle->antialias);
    add_property_double(return_value, "minvalue",       pstyle->minvalue);
    add_property_double(return_value, "maxvalue",       pstyle->maxvalue);
    PHPMS_ADD_PROP_STR(return_value,  "rangeitem",      pstyle->rangeitem);
    add_property_long(return_value,   "opacity",        pstyle->opacity);
    
    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pstyle->color),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "color",new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pstyle->backgroundcolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "backgroundcolor",new_obj_ptr,E_ERROR TSRMLS_CC);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pstyle->outlinecolor),list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "outlinecolor",new_obj_ptr,E_ERROR TSRMLS_CC);

    return style_id;
}


/**********************************************************************
 *                        ms_newStyleObj()
 **********************************************************************/

/* {{{ proto ms_newStyleObj(classObj class)
   Create a new style in the specified class. */

DLEXPORT void php3_ms_style_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pClassObj, *pStyleObj=NULL;
    classObj *parent_class;
    styleObj *pNewStyle, *style_obj=NULL;
    int layer_id, map_id, class_id;
    HashTable   *list=NULL;
    int         nArgs = ARG_COUNT(ht);

    if (nArgs != 1 && nArgs != 2)
    {
        WRONG_PARAM_COUNT;
    }
    if (getParameters(ht, nArgs, &pClassObj, &pStyleObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_class = (classObj*)_phpms_fetch_handle(pClassObj, 
                                                  PHPMS_GLOBAL(le_msclass),
                                                  list TSRMLS_CC);

    if (nArgs == 2)
    {
        style_obj = (styleObj*)_phpms_fetch_handle(pStyleObj, 
                                                   PHPMS_GLOBAL(le_msstyle),
                                                   list TSRMLS_CC);
    }

    if (parent_class == NULL ||
        (pNewStyle = styleObj_new(parent_class, style_obj)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

     _phpms_set_property_long(pClassObj,"numstyles", parent_class->numstyles, 
                              E_ERROR TSRMLS_CC); 

     
    /* We will store a reference to the parent_class object id inside
     * the obj.
     */
    class_id = _phpms_fetch_property_resource(pClassObj, "_handle_", E_ERROR TSRMLS_CC);

    /* We will store a reference to the parent_layer object id inside
     * the obj.
     */
    layer_id = _phpms_fetch_property_resource(pClassObj, "_layer_handle_", E_ERROR TSRMLS_CC);

    /* We will store a reference to the parent_map object id inside
     * the obj.
     */
    map_id = _phpms_fetch_property_resource(pClassObj, "_map_handle_", E_ERROR TSRMLS_CC);
   
    /* Return style object */
    _phpms_build_style_object(pNewStyle, map_id, layer_id, class_id, list, 
                              return_value TSRMLS_CC);
}
/* }}} */

/**********************************************************************
 *                        style->updateFromString()
 **********************************************************************/

/* {{{ proto int style.updateFromString(string snippet)
   Update a style from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_style_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    styleObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (styleObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msstyle),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = styleObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}


/**********************************************************************
 *                        style->set()
 **********************************************************************/

/* {{{ proto int style.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_style_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    styleObj *self;
    mapObj *parent_map;
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

    self = (styleObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msstyle),
                                           list TSRMLS_CC);
   
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_",
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list TSRMLS_CC, E_ERROR);

    if (self == NULL || parent_map == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_LONG(  "symbol",             self->symbol)
    else IF_SET_STRING( "symbolname",   self->symbolname)
    else IF_SET_DOUBLE( "size",         self->size)
    else IF_SET_DOUBLE( "minsize",      self->minsize)
    else IF_SET_DOUBLE( "maxsize",      self->maxsize)
    else IF_SET_DOUBLE( "width",        self->width)
    else IF_SET_DOUBLE( "minwidth",     self->minwidth)
    else IF_SET_DOUBLE( "maxwidth",     self->maxwidth)
    else IF_SET_LONG( "offsetx",        self->offsetx)
    else IF_SET_LONG( "offsety",        self->offsety)
    else IF_SET_DOUBLE("angle",         self->angle)
    else IF_SET_LONG(  "antialias",     self->antialias)
    else IF_SET_DOUBLE("minvalue",      self->minvalue)
    else IF_SET_DOUBLE("maxvalue",      self->maxvalue)
    else IF_SET_STRING( "rangeitem",    self->rangeitem)
    else IF_SET_LONG( "opacity",        self->opacity)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.",
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    if (strcmp(pPropertyName->value.str.val, "symbolname") == 0)
    {
        if (styleObj_setSymbolByName(self,
                                     parent_map,
                                     self->symbolname) == -1)
        {
            RETURN_LONG(-1);
        }
        _phpms_set_property_long(pThis,"symbol", self->symbol, E_ERROR TSRMLS_CC); 
    }

    RETURN_LONG(0);
}

DLEXPORT void php3_ms_style_clone(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    styleObj *self = NULL, *pNewStyle = NULL;
    HashTable   *list=NULL;
    int map_id, layer_id, class_id;
    pThis = getThis();

    if (pThis == NULL)
      php3_error(E_ERROR, "Invalid style object.");

    self = (styleObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_msstyle),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid style object.");

    if ((pNewStyle = styleObj_clone(self)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        RETURN_FALSE;
    }

    class_id = _phpms_fetch_property_resource(pThis, "_class_handle_", E_ERROR TSRMLS_CC);
    layer_id = _phpms_fetch_property_resource(pThis, "_layer_handle_", E_ERROR TSRMLS_CC);
    map_id = _phpms_fetch_property_resource(pThis, "_map_handle_", E_ERROR TSRMLS_CC);

     /* Return style object */
    _phpms_build_style_object(pNewStyle, map_id, layer_id, class_id, list, 
                              return_value TSRMLS_CC);
}



/* {{{ proto int style.setbinding(const bindingid, string value)
   Set the attribute binding for a specfiled style property. Returns true on success. */

DLEXPORT void php3_ms_style_setBinding(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    styleObj *self = NULL;
    HashTable   *list=NULL;
    pval   *pBindingId, *pValue;

    pThis = getThis();

    if (pThis == NULL || 
        getParameters(ht, 2, &pBindingId, &pValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (styleObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_msstyle),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid style object.");

    convert_to_string(pValue);
    convert_to_long(pBindingId);

    if (pBindingId->value.lval < 0 || pBindingId->value.lval > MS_STYLE_BINDING_LENGTH)
       php3_error(E_ERROR, "Invalid binding id given for setbinding function.");

    if (!pValue->value.str.val || strlen(pValue->value.str.val) <= 0)
       php3_error(E_ERROR, "Invalid binding value given for setbinding function.");

    if(self->bindings[pBindingId->value.lval].item) 
    {
        msFree(self->bindings[pBindingId->value.lval].item);
        self->bindings[pBindingId->value.lval].index = -1; 
        self->numbindings--;
    }
    self->bindings[pBindingId->value.lval].item = strdup(pValue->value.str.val); 
    self->numbindings++;

    RETURN_TRUE;
}

/* {{{ proto int style.getbinding(const bindingid)
   Get the value of a attribute binding for a specfiled style property. 
   Returns the string value if exist, else null. */

DLEXPORT void php3_ms_style_getBinding(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    styleObj *self = NULL;
    HashTable   *list=NULL;
    pval   *pBindingId;
    char   *pszValue=NULL;

    pThis = getThis();

    if (pThis == NULL || 
        getParameters(ht, 1, &pBindingId) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (styleObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_msstyle),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid style object.");

    convert_to_long(pBindingId);

    if (pBindingId->value.lval < 0 || pBindingId->value.lval > MS_STYLE_BINDING_LENGTH)
       php3_error(E_ERROR, "Invalid binding id given for getbinding function.");


    if( (pszValue = self->bindings[pBindingId->value.lval].item) != NULL) 
    {
       RETURN_STRING(pszValue, 1);                
    }

    return;
}


/* {{{ proto int style.removebinding(const bindingid)
   Remove attribute binding for a specfiled style property. Returns true on success. */

DLEXPORT void php3_ms_style_removeBinding(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pThis = NULL;
    styleObj *self = NULL;
    HashTable   *list=NULL;
    pval   *pBindingId;

    pThis = getThis();

    if (pThis == NULL || 
        getParameters(ht, 1, &pBindingId) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (styleObj *)_phpms_fetch_handle(pThis,
                                           PHPMS_GLOBAL(le_msstyle),
                                           list TSRMLS_CC);
    if (self == NULL)
       php3_error(E_ERROR, "Invalid style object.");

    convert_to_long(pBindingId);

    if (pBindingId->value.lval < 0 || pBindingId->value.lval > MS_STYLE_BINDING_LENGTH)
       php3_error(E_ERROR, "Invalid binding id given for setbinding function.");


    if(self->bindings[pBindingId->value.lval].item) 
    {
        msFree(self->bindings[pBindingId->value.lval].item);
        self->bindings[pBindingId->value.lval].index = -1; 
        self->numbindings--;
    }

    RETURN_TRUE;
}

/* }}} */



/*=====================================================================
 *                 PHP function wrappers - outputformat class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_outputformat_object()
 **********************************************************************/
static long _phpms_build_outputformat_object(outputFormatObj *poutputformat, 
                                             HashTable *list, 
                                             pval *return_value TSRMLS_DC)
{
    int         outputformat_id;

    if (poutputformat == NULL)
        return 0;

    outputformat_id = 
      php3_list_insert(poutputformat, PHPMS_GLOBAL(le_msoutputformat));

    _phpms_object_init(return_value, outputformat_id, 
                       php_outputformat_class_functions,
                       PHP4_CLASS_ENTRY(outputformat_class_entry_ptr) TSRMLS_CC);

    PHPMS_ADD_PROP_STR(return_value, "name", poutputformat->name);
    PHPMS_ADD_PROP_STR(return_value, "mimetype", poutputformat->mimetype);
    PHPMS_ADD_PROP_STR(return_value, "driver", poutputformat->driver);
    PHPMS_ADD_PROP_STR(return_value, "extension", poutputformat->extension);
    add_property_long(return_value,  "renderer", poutputformat->renderer);
    add_property_long(return_value,  "imagemode", poutputformat->imagemode);
    add_property_long(return_value,  "transparent", poutputformat->transparent);

    return outputformat_id;
}


/**********************************************************************
 *                        outputFormat->set()
 **********************************************************************/

/* {{{ proto int outputFormat.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php_ms_outputformat_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    outputFormatObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (outputFormatObj *)_phpms_fetch_handle(pThis, 
                                                  PHPMS_GLOBAL(le_msoutputformat),
                                                  list TSRMLS_CC);
   
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(    "name",           self->name)
    else IF_SET_STRING( "mimetype",     self->mimetype)
    else IF_SET_STRING( "driver",       self->driver)
    else IF_SET_STRING( "extension",    self->extension)
    else IF_SET_LONG( "renderer",       self->renderer)
    else IF_SET_LONG( "imagemode",      self->imagemode)
    else IF_SET_LONG( "transparent",    self->transparent)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.",
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}



DLEXPORT void php_ms_outputformat_setOption(INTERNAL_FUNCTION_PARAMETERS)
{
    outputFormatObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pPropertyName);
    convert_to_string(pNewValue);
     
    self = (outputFormatObj *)
      _phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msoutputformat),
                          list TSRMLS_CC);

    if (self == NULL)
      RETURN_FALSE;

    msSetOutputFormatOption(self, pPropertyName->value.str.val,
                            pNewValue->value.str.val);

    RETURN_TRUE;
}

DLEXPORT void php_ms_outputformat_getOption(INTERNAL_FUNCTION_PARAMETERS)
{
    outputFormatObj *self;
    const char *szRetrun = NULL;
    pval   *pPropertyName, *pThis;
    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        getParameters(ht, 1, &pPropertyName) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pPropertyName);
     
    self = (outputFormatObj *)
      _phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msoutputformat),
                          list TSRMLS_CC);

    if (self == NULL)
      RETURN_FALSE;

    szRetrun = msGetOutputFormatOption(self, pPropertyName->value.str.val, "");

    RETVAL_STRING((char*)szRetrun, 1);
}

DLEXPORT void php_ms_outputformat_validate(INTERNAL_FUNCTION_PARAMETERS)
{
    outputFormatObj *self;
    pval   *pThis;
    int retVal = MS_FALSE;
    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (outputFormatObj *)_phpms_fetch_handle(pThis, 
                                                  PHPMS_GLOBAL(le_msoutputformat),
                                                  list TSRMLS_CC);

    if (self == NULL)
      RETURN_LONG(MS_FALSE);

    if ( (retVal = msOutputFormatValidate( self )) != MS_SUCCESS )
    {
        _phpms_report_mapserver_error(E_WARNING);
    }

    RETURN_LONG(retVal);
}


/*=====================================================================
 *                 PHP function wrappers - grid class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_grid_object()
 **********************************************************************/
static long _phpms_build_grid_object(graticuleObj *pgrid, 
                                     int parent_layer_id, 
                                     HashTable *list, 
                                     pval *return_value TSRMLS_DC)
{
    int         grid_id;

    if (pgrid == NULL)
        return 0;

    grid_id = 
      php3_list_insert(pgrid, PHPMS_GLOBAL(le_msgrid));

    _phpms_object_init(return_value, grid_id, 
                       php_grid_class_functions,
                       PHP4_CLASS_ENTRY(grid_class_entry_ptr) TSRMLS_CC);

    add_property_resource(return_value, "_layer_handle_", parent_layer_id);
    zend_list_addref(parent_layer_id);

    add_property_double(return_value,  "minsubdivide", pgrid->minsubdivides);
    add_property_double(return_value,  "maxsubdivide", pgrid->maxsubdivides);
    add_property_double(return_value,  "minarcs", pgrid->minarcs);
    add_property_double(return_value,  "maxarcs", pgrid->maxarcs);
    add_property_double(return_value,  "mininterval", pgrid->minincrement);
    add_property_double(return_value,  "maxinterval", pgrid->maxincrement);
    PHPMS_ADD_PROP_STR(return_value, "labelformat", pgrid->labelformat);
    
    return grid_id;
}


DLEXPORT void php3_ms_grid_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pLayerObj;
    layerObj *parent_class;
    int layer_id;
    HashTable   *list=NULL;
    pval        *new_obj_ptr;

    if (getParameters(ht, 1, &pLayerObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_class = (layerObj*)_phpms_fetch_handle(pLayerObj, 
                                                  PHPMS_GLOBAL(le_mslayer),
                                                  list TSRMLS_CC);

    layer_id = _phpms_fetch_property_resource(pLayerObj, "_handle_", E_ERROR TSRMLS_CC);

    if (parent_class == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    parent_class->connectiontype = MS_GRATICULE;

    /* Update layerObj members */
    _phpms_set_property_long(pLayerObj, "connectiontype",
                             parent_class->connectiontype, E_ERROR TSRMLS_CC); 

    if (parent_class->layerinfo != NULL)
      free(parent_class->layerinfo);

    parent_class->layerinfo = (graticuleObj *)malloc( sizeof( graticuleObj ) );
    initGrid((graticuleObj *)parent_class->layerinfo );

    MAKE_STD_ZVAL(new_obj_ptr);
    _phpms_build_grid_object((graticuleObj *)(parent_class->layerinfo),
                             layer_id,
                             list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(pLayerObj, "grid", new_obj_ptr, E_ERROR TSRMLS_CC);
    
}
/* }}} */


/**********************************************************************
 *                        grid->set()
 **********************************************************************/

/* {{{ proto int grid.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_grid_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    graticuleObj *self;
    layerObj *parent_layer;
    pval   *pPropertyName, *pNewValue, *pThis;
    HashTable   *list=NULL;

    pThis = getThis();


    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (graticuleObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msgrid),
                                           list TSRMLS_CC);
   
    parent_layer = (layerObj*)_phpms_fetch_property_handle(pThis, "_layer_handle_",
                                                       PHPMS_GLOBAL(le_mslayer),
                                                       list TSRMLS_CC, E_ERROR);

    if (self == NULL || parent_layer == NULL)
    {
        RETURN_LONG(-1);
    }

    if (parent_layer->connectiontype == MS_GRATICULE &&
        parent_layer->layerinfo != NULL)
    {
        convert_to_string(pPropertyName);

        IF_SET_DOUBLE(  "minsubdivide",             self->minsubdivides)
        else IF_SET_DOUBLE(  "maxsubdivide",             self->maxsubdivides)
        else IF_SET_DOUBLE(  "minarcs",             self->minarcs)
        else IF_SET_DOUBLE(  "maxarcs",             self->maxarcs)
        else IF_SET_DOUBLE(  "mininterval",             self->minincrement)
        else IF_SET_DOUBLE(  "maxinterval",             self->maxincrement)
        else IF_SET_STRING( "labelformat",   self->labelformat)
        else
        {
            php3_error(E_ERROR,"Property '%s' does not exist in this object.",
                   pPropertyName->value.str.val);
            RETURN_LONG(-1);
        }

    }

    RETURN_LONG(0);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - errorObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_error_object()
 **********************************************************************/
static long _phpms_build_error_object(errorObj *perror, 
                                     HashTable *list, pval *return_value TSRMLS_DC)
{
    int error_id;

    if (perror == NULL)
      return 0;

    error_id = php3_list_insert(perror, PHPMS_GLOBAL(le_mserror_ref) );

    _phpms_object_init(return_value, error_id, php_error_class_functions,
                       PHP4_CLASS_ENTRY(error_class_entry_ptr) TSRMLS_CC);

    add_property_long(return_value,     "code",         perror->code);
    PHPMS_ADD_PROP_STR(return_value,    "routine",      perror->routine);
    PHPMS_ADD_PROP_STR(return_value,    "message",      perror->message);

    return error_id;
}


/**********************************************************************
 *                        ms_GetErrorObj()
 **********************************************************************/

/* {{{ proto errorObj ms_GetErrorObj()
   Return the head of the MapServer errorObj list. */

DLEXPORT void php3_ms_get_error_obj(INTERNAL_FUNCTION_PARAMETERS)
{
    errorObj *pError;
    HashTable   *list=NULL;

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pError = msGetErrorObj()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return error object */
    _phpms_build_error_object(pError, list, return_value TSRMLS_CC);
}
/* }}} */


/**********************************************************************
 *                        error->next()
 **********************************************************************/

/* {{{ proto int error.next()
   Returns a ref to the next errorObj in the list, or NULL if we reached the last one */

DLEXPORT void php3_ms_error_next(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pThis;
    errorObj    *self, *error_ptr;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (errorObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mserror_ref),
                                           list TSRMLS_CC);

    if (self == NULL || self->next == NULL)
    {
        RETURN_NULL();
    }

    /* Make sure 'self' is still valid.  It may have been deleted by 
     * msResetErrorList() 
     */
    error_ptr = msGetErrorObj();
    while(error_ptr != self)
    {
        if (error_ptr->next == NULL)
        {
            /* We reached end of list of active errorObj and didn't find the
             * errorObj... thisis bad!
             */
            php_error(E_WARNING, 
                      "ERROR: Trying to access an errorObj that has expired.");
            RETURN_NULL();
        }
        error_ptr = error_ptr->next;
    }

    /* Return error object */
    _phpms_build_error_object(self->next, list, return_value TSRMLS_CC);
}
/* }}} */


/**********************************************************************
 *                        ms_ResetErrorList()
 **********************************************************************/

/* {{{ proto errorObj ms_ResetErrorList()
   Clear the MapServer errorObj list. */

DLEXPORT void php3_ms_reset_error_list(INTERNAL_FUNCTION_PARAMETERS)
{
    msResetErrorList();
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - labelcache class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_labelcache_object()
 **********************************************************************/
static long _phpms_build_labelcache_object(labelCacheObj *plabelcache, 
                                           HashTable *list, 
                                           pval *return_value TSRMLS_DC)
{
    int         labelcache_id;

    if (plabelcache == NULL)
        return 0;

    labelcache_id = 
      php3_list_insert(plabelcache, PHPMS_GLOBAL(le_mslabelcache));

    _phpms_object_init(return_value, labelcache_id, 
                       php_labelcache_class_functions,
                       PHP4_CLASS_ENTRY(labelcache_class_entry_ptr) TSRMLS_CC);

    return labelcache_id;
}

/**********************************************************************
 *                        labelcache->free()
 **********************************************************************/

/* {{{ proto int labelcache->free(()
   Free the labelcache object in the map. Returns true on success or false if
   an error occurs. */

DLEXPORT void php_ms_labelcache_free(INTERNAL_FUNCTION_PARAMETERS)
{
    labelCacheObj *self;
    HashTable   *list=NULL;
    pval  *pThis;

    pThis = getThis();


    if (pThis == NULL)
      RETURN_FALSE;

     
    self = (labelCacheObj *)
      _phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslabelcache),
                          list TSRMLS_CC);

    if (self == NULL)
      RETURN_FALSE;

    labelCacheObj_freeCache(self);

     RETURN_TRUE;
}


/*=====================================================================
 *                 PHP function wrappers - symbol object
 *====================================================================*/

DLEXPORT void php3_ms_symbol_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pMapObj, *pName;
    mapObj      *parent_map;
    HashTable   *list=NULL;
    int         nReturn = -1;

    if ( getParameters(ht, 2, &pMapObj, &pName) == FAILURE)
    {
        WRONG_PARAM_COUNT;
    }

    parent_map = (mapObj*)_phpms_fetch_handle(pMapObj, 
                                              PHPMS_GLOBAL(le_msmap),
                                              list TSRMLS_CC);
    if (parent_map == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pName);

    nReturn = msAddNewSymbol(parent_map, pName->value.str.val);

    RETURN_LONG(nReturn);
}
 

/**********************************************************************
 *                     _phpms_build_symbol_object()
 **********************************************************************/
static long _phpms_build_symbol_object(symbolObj *psSymbol, 
                                       int parent_map_id, 
                                       HashTable *list, 
                                       pval *return_value TSRMLS_DC)
{
    int symbol_id;



    if (psSymbol == NULL)
        return 0;

    symbol_id = php3_list_insert(psSymbol, PHPMS_GLOBAL(le_mssymbol));

    _phpms_object_init(return_value, symbol_id, php_symbol_class_functions,
                       PHP4_CLASS_ENTRY(symbol_class_entry_ptr) TSRMLS_CC);

    add_property_resource(return_value, "_map_handle_", parent_map_id);
    zend_list_addref(parent_map_id);

    PHPMS_ADD_PROP_STR(return_value,  "name",       psSymbol->name);
    add_property_long(return_value,   "type",       psSymbol->type);
    add_property_long(return_value,   "inmapfile",  psSymbol->inmapfile);
    add_property_double(return_value, "sizex",      psSymbol->sizex);
    add_property_double(return_value, "sizey",      psSymbol->sizey);
    add_property_long(return_value,   "numpoints",  psSymbol->numpoints);
    add_property_long(return_value,   "filled",     psSymbol->filled);
    add_property_long(return_value,   "patternlength", psSymbol->patternlength);

    /* TODO: stylelength deprecated in v5.0. To be removed in a later release*/
    add_property_long(return_value,   "stylelength", psSymbol->patternlength);

    /* MS_SYMBOL_PIXMAP */ 
    PHPMS_ADD_PROP_STR(return_value,  "imagepath", psSymbol->imagepath);
    add_property_long(return_value,   "transparent",     psSymbol->transparent);
    add_property_long(return_value,   "transparentcolor", psSymbol->transparentcolor);

    /* MS_SYMBOL_TRUETYPE */             
    PHPMS_ADD_PROP_STR(return_value,  "character",  psSymbol->character); 
    add_property_long(return_value,   "antialias",  psSymbol->antialias); 
    PHPMS_ADD_PROP_STR(return_value,  "font",       psSymbol->font); 
    add_property_long(return_value,   "gap",        psSymbol->gap); 
    add_property_long(return_value,   "position",   psSymbol->position); 
    
    //TODO : true type and cartoline parameters to add.

    
    return symbol_id;
}
    
/**********************************************************************
 *                        symbol->set()
 **********************************************************************/

/* {{{ proto int symbol.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_symbol_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    symbolObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (symbolObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mssymbol),
                                            list TSRMLS_CC);
    
    if (self == NULL)
      RETURN_FALSE;

    convert_to_string(pPropertyName);

    IF_SET_STRING( "name",          self->name)
    else IF_SET_LONG(  "type",      self->type)
    else IF_SET_LONG(  "inmapfile", self->inmapfile)
    else IF_SET_DOUBLE(  "sizex",   self->sizex)
    else IF_SET_DOUBLE(  "sizey",   self->sizey)
    else IF_SET_LONG(  "filled",    self->filled)
    else IF_SET_LONG(  "transparent", self->transparent) 
    else IF_SET_LONG(  "transparentcolor", self->transparentcolor) 
    else IF_SET_STRING("character",   self->character) 
    else IF_SET_LONG(  "antialias",   self->antialias) 
    else IF_SET_STRING("font",        self->font) 
    else IF_SET_LONG(  "gap",         self->gap) 
    else IF_SET_LONG(  "position",    self->position)        
    else if (strcmp( "numpoints", pPropertyName->value.str.val) == 0 ||
             strcmp( "patternlength", pPropertyName->value.str.val) == 0 ||
             strcmp( "stylelength", pPropertyName->value.str.val) == 0 ||
             strcmp( "imagepath", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
         RETURN_FALSE;
    }
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.",
                   pPropertyName->value.str.val);
         RETURN_FALSE;
    }

    RETURN_TRUE;
}


/**********************************************************************
 *                        symbol->setpoints()
 **********************************************************************/

/* {{{ proto int symbol.setpoints(array points, int numpoints)
   Set the points of the symbol ) */ 

DLEXPORT void php3_ms_symbol_setPoints(INTERNAL_FUNCTION_PARAMETERS)
{
    symbolObj *self;
    pval   *pPoints, *pThis;
    HashTable   *list=NULL;
    pval        **pValue = NULL;
    int i=0, nElements = 0, iSymbol=0;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pPoints) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (symbolObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mssymbol),
                                            list TSRMLS_CC);
    
    if (self == NULL)
      RETURN_FALSE;

    if (pPoints->type != IS_ARRAY) 
    {
        php_error(E_WARNING, 
                  "symbol->setpoints : expected array as parameter");
        RETURN_FALSE;
    }

    nElements = zend_hash_num_elements(pPoints->value.ht);

    if (nElements <=0)
    {
        php_error(E_WARNING, 
                  "symbol->setpoints : invalid array as parameter. Array sould have at least two points.");
        RETURN_FALSE;
    }
    
    i= 0;
    iSymbol = 0;
    while (i<nElements)
    {
        if (zend_hash_index_find(pPoints->value.ht, i, 
                                 (void *)&pValue) == FAILURE)
        {
            RETURN_FALSE;
        }
        convert_to_double((*pValue));
        self->points[iSymbol].x = (*pValue)->value.dval;
        self->sizex = MS_MAX(self->sizex, self->points[iSymbol].x);
        i++;

         if (zend_hash_index_find(pPoints->value.ht, i, 
                                 (void *)&pValue) == FAILURE)
        {
            RETURN_FALSE;
        }
        convert_to_double((*pValue));
        self->points[iSymbol].y = (*pValue)->value.dval;
        self->sizey = MS_MAX(self->sizey, self->points[iSymbol].y);
        i++;

        iSymbol++;
    }
    
    self->numpoints = (nElements/2);

    _phpms_set_property_long(pThis,"numpoints", self->numpoints , E_ERROR TSRMLS_CC); 
    RETURN_TRUE;
}

/**********************************************************************
 *                        symbol->getpointsarray()
 **********************************************************************/
DLEXPORT void php3_ms_symbol_getPoints(INTERNAL_FUNCTION_PARAMETERS)
{
    symbolObj *self;
    pval        *pThis;
    HashTable   *list=NULL;
    int i=0;

     pThis = getThis();

     if (pThis == NULL)
        RETURN_FALSE;

     if (array_init(return_value) == FAILURE) 
     {
         RETURN_FALSE;
     }

     self = (symbolObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mssymbol), 
                                             list TSRMLS_CC);

     if (self == NULL)
       RETURN_FALSE;

     if (self->numpoints > 0)
     {
         for (i=0; i<self->numpoints; i++)
         {
             add_next_index_double(return_value, self->points[i].x);
             add_next_index_double(return_value, self->points[i].y);
         }
     }
     else
       RETURN_FALSE;
}


/**********************************************************************
 *                        symbol->getpatternarray()
 **********************************************************************/
DLEXPORT void php3_ms_symbol_getPattern(INTERNAL_FUNCTION_PARAMETERS)
{
    symbolObj *self;
    pval        *pThis;
    HashTable   *list=NULL;
    int i=0;

     pThis = getThis();

     if (pThis == NULL)
        RETURN_FALSE;

     if (array_init(return_value) == FAILURE) 
     {
         RETURN_FALSE;
     }

     self = (symbolObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mssymbol), 
                                             list TSRMLS_CC);

     if (self == NULL)
       RETURN_FALSE;

     if (self->patternlength > 0)
     {
         for (i=0; i<self->patternlength; i++)
         {
             add_next_index_double(return_value, self->pattern[i]);
         }
     }
     else
       RETURN_FALSE;
}
         
         

/**********************************************************************
 *                        symbol->setpattern()
 **********************************************************************/

/* {{{ proto int symbol.setpattern(array points, int numpoints)
   Set the pattern of the symbol ) */ 

DLEXPORT void php3_ms_symbol_setPattern(INTERNAL_FUNCTION_PARAMETERS)
{
    symbolObj *self;
    pval   *pPoints, *pThis;
    HashTable   *list=NULL;
    pval        **pValue = NULL;
    int i=0, nElements = 0;
 
    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pPoints) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (symbolObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mssymbol),
                                            list TSRMLS_CC);
    
    if (self == NULL)
      RETURN_FALSE;

    if (pPoints->type != IS_ARRAY) 
    {
        php_error(E_WARNING, 
                  "symbol->setpattern : expected array as parameter");
        RETURN_FALSE;
    }

    nElements = zend_hash_num_elements(pPoints->value.ht);

    if (nElements <=0)
    {
        php_error(E_WARNING, 
                  "symbol->setpattern : invalid array as parameter. Array sould have at least two entries.");
        RETURN_FALSE;
    }
    
    i= 0;
    while (i<nElements)
    {
        if (zend_hash_index_find(pPoints->value.ht, i, 
                                 (void *)&pValue) == FAILURE)
        {
            RETURN_FALSE;
        }
        convert_to_long((*pValue));
        self->pattern[i] = (*pValue)->value.lval;
        i++;
    }
    
    self->patternlength = nElements;

    _phpms_set_property_long(pThis,"patternlength", self->patternlength , E_ERROR TSRMLS_CC); 

    RETURN_TRUE;
}


/**********************************************************************
 *                        symbol->setimagepath()
 **********************************************************************/

/* {{{ proto int symbol.setimagepath(char *imagefile)
   loads a new symbol image  ) */ 

DLEXPORT void php3_ms_symbol_setImagepath(INTERNAL_FUNCTION_PARAMETERS)
{
    symbolObj *self;
    pval   *pFile, *pThis;
    HashTable   *list=NULL;
 

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pFile) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (symbolObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mssymbol),
                                            list TSRMLS_CC);
    
    if (self == NULL)
      RETURN_FALSE;

    convert_to_string(pFile);
    

    if (msLoadImageSymbol(self, pFile->value.str.val) == MS_SUCCESS)
    {
        _phpms_set_property_string(pThis,"imagepath", self->imagepath , E_ERROR TSRMLS_CC); 
        RETURN_TRUE;
    }
    else
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }
}




/*=====================================================================
 *                 PHP function wrappers - queryMapObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_querymap_object()
 **********************************************************************/
static long _phpms_build_querymap_object(queryMapObj *pquerymap,
                                      HashTable *list, pval *return_value TSRMLS_DC)
{
    pval *new_obj_ptr;
    int querymap_id;

    if (pquerymap == NULL)
      return 0;

    querymap_id = php3_list_insert(pquerymap, PHPMS_GLOBAL(le_msquerymap));

    _phpms_object_init(return_value, querymap_id, php_querymap_class_functions,
                       PHP4_CLASS_ENTRY(querymap_class_entry_ptr) TSRMLS_CC);

    /* editable properties */
    add_property_long(return_value,   "width",    pquerymap->width);
    add_property_long(return_value,   "height",   pquerymap->height);
    add_property_long(return_value,   "style",	  pquerymap->style);

    MAKE_STD_ZVAL(new_obj_ptr);  /* Alloc and Init a ZVAL for new object */
    _phpms_build_color_object(&(pquerymap->color), list, new_obj_ptr TSRMLS_CC);
    _phpms_add_property_object(return_value, "color", new_obj_ptr, E_ERROR TSRMLS_CC);

    return querymap_id;
}

/**********************************************************************
 *                        querymap->updateFromString()
 **********************************************************************/

/* {{{ proto int querymap.updateFromString(string snippet)
   Update a querymap from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */

DLEXPORT void php3_ms_querymap_updateFromString(INTERNAL_FUNCTION_PARAMETERS)
{
    queryMapObj *self;
    pval   *pThis;
    char   *pSnippet;
    int    s_len, nStatus = MS_FAILURE;
    HashTable   *list=NULL;
    
    pThis = getThis();

    if (pThis == NULL ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pSnippet, &s_len) 
         == FAILURE))
    {
       return;
    }

    self = (queryMapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msquerymap),
                                           list TSRMLS_CC);
    if (self == NULL || 
        (nStatus = queryMapObj_updateFromString(self, 
                                        pSnippet)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}


/* }}} */

/**********************************************************************
 *                        querymap->set()
 **********************************************************************/

/* {{{ proto int querymap.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_querymap_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    queryMapObj *self;
    pval *pPropertyName, *pNewValue;
    pval *pThis;

    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) != 2)
    {
        WRONG_PARAM_COUNT;
    }else
    {
        if (getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
            WRONG_PARAM_COUNT;
    }


    self = (queryMapObj *)_phpms_fetch_handle(pThis, le_msquerymap, list TSRMLS_CC);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_LONG(      "width",      self->width)
    else IF_SET_LONG( "height",     self->height)
    else IF_SET_LONG( "style",      self->style)
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
 *         PHP function wrappers - cgirequest object (used for ows)
 *====================================================================*/

static long _phpms_build_cgirequest_object(cgiRequestObj *prequest, int handle_type, 
                                           HashTable *list, pval *return_value TSRMLS_DC)
{
    int id;

    if (prequest == NULL)
      return 0;

    id = php3_list_insert(prequest, handle_type);

    _phpms_object_init(return_value, id, php_cgirequest_class_functions,
                       PHP4_CLASS_ENTRY(cgirequest_class_entry_ptr) TSRMLS_CC);

    add_property_long(return_value,   "numparams",     prequest->NumParams);
    add_property_long(return_value,   "type",     prequest->type);

    return id;
}


DLEXPORT void php_ms_cgirequest_new(INTERNAL_FUNCTION_PARAMETERS)
{
    int          nArgs;
    cgiRequestObj *pRequest;
     HashTable   *list=NULL;

    nArgs = ARG_COUNT(ht);

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pRequest = cgirequestObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return cgirequest object */
    _phpms_build_cgirequest_object(pRequest, PHPMS_GLOBAL(le_mscgirequest), 
                                   list, return_value TSRMLS_CC);
}


DLEXPORT void php_ms_cgirequest_loadParams(INTERNAL_FUNCTION_PARAMETERS)
{
     cgiRequestObj *self;
     pval *pThis;
     HashTable   *list=NULL;

     pThis = getThis();

     if (pThis == NULL)
       RETURN_FALSE;

      self = (cgiRequestObj *)_phpms_fetch_handle(pThis, le_mscgirequest, list TSRMLS_CC);
      if (self == NULL)
         RETURN_FALSE;

      cgirequestObj_loadParams(self);
     /* sync the class member*/ 
     _phpms_set_property_long(pThis,"numparams", self->NumParams, E_ERROR TSRMLS_CC); 
     _phpms_set_property_long(pThis,"type", self->type, E_ERROR TSRMLS_CC); 

     RETURN_LONG(self->NumParams);
}


DLEXPORT void php_ms_cgirequest_setParameter(INTERNAL_FUNCTION_PARAMETERS)
{
    cgiRequestObj *self;
    pval *pThis;
    HashTable   *list=NULL;
    pval *pName, *pValue;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) != 2)
    {
      WRONG_PARAM_COUNT;
    }
     else
     {
       if (getParameters(ht, 2, &pName, &pValue) != SUCCESS)
         WRONG_PARAM_COUNT;
     }

    self = (cgiRequestObj *)_phpms_fetch_handle(pThis, le_mscgirequest, list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

     convert_to_string(pName);
     convert_to_string(pValue);

     cgirequestObj_setParameter(self, pName->value.str.val, 
                                pValue->value.str.val);

     /* sync the class member*/ 
     _phpms_set_property_long(pThis,"numparams", self->NumParams, E_ERROR TSRMLS_CC); 

     RETURN_LONG(0);
}


DLEXPORT void php_ms_cgirequest_getName(INTERNAL_FUNCTION_PARAMETERS)
{
    cgiRequestObj *self;
    pval *pThis;
    HashTable   *list=NULL;
    pval *pIndex;
    char   *pszValue = NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) != 1)
    {
      WRONG_PARAM_COUNT;
    }
     else
     {
       if (getParameters(ht, 1, &pIndex) != SUCCESS)
         WRONG_PARAM_COUNT;
     }

    self = (cgiRequestObj *)_phpms_fetch_handle(pThis, le_mscgirequest, list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    convert_to_long(pIndex);

    pszValue = cgirequestObj_getName(self, pIndex->value.lval);

    if (pszValue)
    {
      RETURN_STRING(pszValue, 1);
    }
    else
    {
      RETURN_STRING("", 1);
    }
}



DLEXPORT void php_ms_cgirequest_getValue(INTERNAL_FUNCTION_PARAMETERS)
{
    cgiRequestObj *self;
    pval *pThis;
    HashTable   *list=NULL;
    pval *pIndex;
    char   *pszValue = NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) != 1)
    {
      WRONG_PARAM_COUNT;
    }
     else
     {
       if (getParameters(ht, 1, &pIndex) != SUCCESS)
         WRONG_PARAM_COUNT;
     }

    self = (cgiRequestObj *)_phpms_fetch_handle(pThis, le_mscgirequest, list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    convert_to_long(pIndex);

    pszValue = cgirequestObj_getValue(self, pIndex->value.lval);

    if (pszValue)
    {
      RETURN_STRING(pszValue, 1);
    }
    else
    {
      RETURN_STRING("", 1);
    }
}


DLEXPORT void php_ms_cgirequest_getValueByName(INTERNAL_FUNCTION_PARAMETERS)
{
    cgiRequestObj *self;
    pval *pThis;
    HashTable   *list=NULL;
    pval *pName;
    char   *pszValue = NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) != 1)
    {
      WRONG_PARAM_COUNT;
    }
    else
    {
      if (getParameters(ht, 1, &pName) != SUCCESS)
        WRONG_PARAM_COUNT;
    }

    self = (cgiRequestObj *)_phpms_fetch_handle(pThis, le_mscgirequest, list TSRMLS_CC);
    if (self == NULL)
      RETURN_FALSE;

    convert_to_string(pName);

    pszValue = cgirequestObj_getValueByName(self, pName->value.str.val);

    if (pszValue)
    {
      RETURN_STRING(pszValue, 1);
    }
    else
    {
      RETURN_STRING("", 1);
    }
}
/* }}} */

/*=====================================================================
 *         PHP function wrappers - hashtable object
 *====================================================================*/

static long _phpms_build_hashtable_object(hashTableObj *hashtable,
                                          HashTable *list, pval *return_value TSRMLS_DC)
{
    int hashtable_id;

    if (hashtable == NULL)
      return 0;

    hashtable_id = php3_list_insert(hashtable, PHPMS_GLOBAL(le_mshashtable));

    _phpms_object_init(return_value, hashtable_id, php_hashtable_class_functions,
                       PHP4_CLASS_ENTRY(hashtable_class_entry_ptr) TSRMLS_CC);

    return hashtable_id;
}


/**********************************************************************
 *                        hashtable->get()
 **********************************************************************/

/* {{{ proto int hashtable.get(string key)
   Get a value from item by its key. Returns empty string if not found. */

DLEXPORT void php3_ms_hashtable_get(INTERNAL_FUNCTION_PARAMETERS)
{
    hashTableObj *self;
    pval         *pKey, *pThis;  
    const char   *pszValue=NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pKey) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (hashTableObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mshashtable),
                                               list TSRMLS_CC);

    convert_to_string(pKey);

    if ((self == NULL) ||
        ((pszValue = hashTableObj_get(self, pKey->value.str.val)) == NULL))
    {
       pszValue = "";
    }

    RETURN_STRING((char *)pszValue, 1);
}
/* }}} */

/**********************************************************************
 *                        hashtable->set()
 **********************************************************************/

/* {{{ proto int hashtable.set(string key, string value)
   Set a hash item given key and value. Returns MS_FAILURE on error. */

DLEXPORT void php3_ms_hashtable_set(INTERNAL_FUNCTION_PARAMETERS)
{
    hashTableObj *self;
    pval         *pKey, *pValue, *pThis;
    int          nStatus = MS_FAILURE;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 2, &pKey, &pValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (hashTableObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mshashtable),
                                               list TSRMLS_CC);

    if (self == NULL)
       RETURN_LONG(nStatus);
          
    convert_to_string(pKey);
    convert_to_string(pValue);      
    
    if ((nStatus = hashTableObj_set(self, pKey->value.str.val, 
                                    pValue->value.str.val)) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    RETURN_LONG(nStatus);
}
/* }}} */

/**********************************************************************
 *                        hashtable->remove()
 **********************************************************************/

/* {{{ proto int hashtable.remove(string key)
   Remove one item from hash table. Returns MS_FAILURE on error. */

DLEXPORT void php3_ms_hashtable_remove(INTERNAL_FUNCTION_PARAMETERS)
{
    hashTableObj *self;
    pval         *pKey, *pThis;  
    int           nStatus = MS_FAILURE;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pKey) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (hashTableObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mshashtable),
                                               list TSRMLS_CC);
    if (self == NULL)
       RETURN_LONG(nStatus);

    convert_to_string(pKey);

    if ((nStatus = hashTableObj_remove(self, pKey->value.str.val)) != MS_SUCCESS)
    {
       _phpms_report_mapserver_error(E_ERROR);
    }
    
    RETURN_LONG(nStatus);
}
/* }}} */

/**********************************************************************
 *                        hashtable->clear()
 **********************************************************************/

/* {{{ proto int hashtable.clear()
   Clear all items in hash table (to NULL). */

DLEXPORT void php3_ms_hashtable_clear(INTERNAL_FUNCTION_PARAMETERS)
{
    hashTableObj *self;
    pval         *pThis;  
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (hashTableObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mshashtable),
                                               list TSRMLS_CC);

    if (self == NULL)
       return;
    
    hashTableObj_clear(self);
}
/* }}} */

/**********************************************************************
 *                        hashtable->nextkey()
 **********************************************************************/

/* {{{ proto int hashtable.nextkey(string previousKey)
   Return the next key or first key if previousKey == NULL. 
   Returns NULL if no item is in the hashTable or end of hashTable reached */

DLEXPORT void php3_ms_hashtable_nextkey(INTERNAL_FUNCTION_PARAMETERS)
{
    hashTableObj *self;
    pval         *pPreviousKey, *pThis;
    char         *pszKey = NULL, *pszValue = NULL;
    HashTable   *list=NULL;

    pThis = getThis();

    if (pThis == NULL ||
        getParameters(ht, 1, &pPreviousKey) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (hashTableObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mshashtable),
                                               list TSRMLS_CC);

    convert_to_string(pPreviousKey);

    /* pszKey=NULL is used for first call */
    if (strcmp(pPreviousKey->value.str.val,"") != 0)
    {
       pszKey = pPreviousKey->value.str.val;
    }

    if ((self == NULL) || 
        (pszValue = hashTableObj_nextKey(self, pszKey)) == NULL)
       return;

    RETURN_STRING(pszValue, 1);
}
/* }}} */

/* ==================================================================== */
/*      utility functions related to msio                               */
/* ==================================================================== */
DLEXPORT void php_ms_IO_installStdoutToBuffer(INTERNAL_FUNCTION_PARAMETERS)
{
    msIO_installStdoutToBuffer();

    RETURN_TRUE;
}


DLEXPORT void php_ms_IO_resetHandlers(INTERNAL_FUNCTION_PARAMETERS)
{
    msIO_resetHandlers();
    RETURN_TRUE;
}


DLEXPORT void php_ms_IO_installStdinFromBuffer(INTERNAL_FUNCTION_PARAMETERS)
{
    msIO_installStdinFromBuffer();

    RETURN_TRUE;
}

DLEXPORT void php_ms_IO_getStdoutBufferString(INTERNAL_FUNCTION_PARAMETERS)
{
    char *buffer;

    msIOContext *ctx = msIO_getHandler( (FILE *) "stdout" );
    msIOBuffer  *buf;

    if(ctx == NULL ||  ctx->write_channel == MS_FALSE
        || strcmp(ctx->label,"buffer") != 0 )
    {
        php_error(E_ERROR, "Can't identify msIO buffer");
        RETURN_FALSE;
    }
    
    buf = (msIOBuffer *) ctx->cbData;

    /* write one zero byte and backtrack if it isn't already there */
    if( buf->data_len == 0 || buf->data[buf->data_offset] != '\0' ) {
        msIO_bufferWrite( buf, "", 1 );
	buf->data_offset--;
    }

    buffer = (char *) (buf->data);

    RETURN_STRING(buffer, 1);
}
  

typedef struct {
    unsigned char *data;
    int size;
    int owns_data;
} gdBuffer;


DLEXPORT void php_ms_IO_getStdoutBufferBytes(INTERNAL_FUNCTION_PARAMETERS)
{
    msIOContext *ctx = msIO_getHandler( (FILE *) "stdout" );
    msIOBuffer  *buf;
    gdBuffer     gdBuf;

    if( ctx == NULL || ctx->write_channel == MS_FALSE 
        || strcmp(ctx->label,"buffer") != 0 )
    {
        php_error(E_ERROR, "Can't identify msIO buffer");
        RETURN_FALSE;
    }

    buf = (msIOBuffer *) ctx->cbData;

    gdBuf.data = buf->data;
    gdBuf.size = buf->data_offset;
    gdBuf.owns_data = MS_FALSE;

    /* we are seizing ownership of the buffer contents */
    buf->data_offset = 0;
    buf->data_len = 0;
    buf->data = NULL;

    php_write(gdBuf.data, gdBuf.size TSRMLS_CC);

    /* return the gdBuf.size, which is the "really used length" of the msIOBuffer */ 
    RETURN_LONG(gdBuf.size); 
}

DLEXPORT void php_ms_IO_stripStdoutBufferContentType(INTERNAL_FUNCTION_PARAMETERS)
{
    const char *buf = NULL;

    buf = msIO_stripStdoutBufferContentType();

    if (buf)
    {
        RETURN_STRING((char *)buf, 1);
    }
    else
    {
        RETURN_FALSE;
    }
}

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
      //php3_error(E_WARNING, "posix_getcwd() failed with '%s'",
      //            strerror(errno));
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
    convert_to_double(pResolution);

    poGeorefExt = 
        (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                        PHPMS_GLOBAL(le_msrect_ref),
                                        PHPMS_GLOBAL(le_msrect_new),
                                        list TSRMLS_CC);

    if (msCalculateScale(*poGeorefExt, pUnit->value.lval, 
                         pWidth->value.lval, pHeight->value.lval,
                         pResolution->value.dval, &dfScale) != MS_SUCCESS)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }
    
    RETURN_DOUBLE(dfScale);
}

/**********************************************************************
 *                        ms_tokenizeMap()
 *
 * Preparse mapfile and return an array containg one item for each 
 * token in the map.
 **********************************************************************/

/* {{{ proto array ms_tokenizeMap(string filename)
   Preparse mapfile and return an array containg one item for each token in the map.*/

DLEXPORT void php3_ms_tokenizeMap(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pFname;
    char        **tokens;
    int         i, numtokens=0;

    if (getParameters(ht, 1, &pFname) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFname);

    if ((tokens = msTokenizeMap(pFname->value.str.val, &numtokens)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed tokenizing map file %s", 
                            pFname->value.str.val);
        RETURN_FALSE;
    }
    else
    {
        if (array_init(return_value) == FAILURE) 
        {
            RETURN_FALSE;
        }

        for (i=0; i<numtokens; i++)
        {
            add_next_index_string(return_value,  tokens[i], 1);
        }

        msFreeCharArray(tokens, numtokens);
    }


}
