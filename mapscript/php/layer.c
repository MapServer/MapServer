/**********************************************************************
 * $Id: php_mapscript.c 9765 2010-01-28 15:32:10Z aboudreault $
 *
 * Project:  MapServer
 * Purpose:  PHP/MapScript extension for MapServer.  External interface
 *           functions
 * Author:   Daniel Morissette, DM Solutions Group (dmorissette@dmsolutions.ca)
 *           Alan Boudreault, Mapgears
 *
 **********************************************************************
 * Copyright (c) 2000-2010, Daniel Morissette, DM Solutions Group Inc.
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

zend_class_entry *mapscript_ce_layer;
zend_object_handlers mapscript_layer_object_handlers;

ZEND_BEGIN_ARG_INFO_EX(layer___construct_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_updateFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setExtent_args, 0, 0, 4)
ZEND_ARG_INFO(0, minx)
ZEND_ARG_INFO(0, miny)
ZEND_ARG_INFO(0, maxx)
ZEND_ARG_INFO(0, maxy)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_getClass_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_getClassIndex_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_ARG_INFO(0, classGroup)
ZEND_ARG_INFO(0, numClasses)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setFilter_args, 0, 0, 1)
ZEND_ARG_INFO(0, expression)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setProjection_args, 0, 0, 1)
ZEND_ARG_INFO(0, projectionParams)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setWKTProjection_args, 0, 0, 1)
ZEND_ARG_INFO(0, projectionParams)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_getResult_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_whichShapes_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, extent, rectObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_addFeature_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_getMetaData_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setMetaData_args, 0, 0, 2)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_removeMetaData_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_getWMSFeatureInfoURL_args, 0, 0, 4)
ZEND_ARG_INFO(0, clickX)
ZEND_ARG_INFO(0, clickY)
ZEND_ARG_INFO(0, featureCount)
ZEND_ARG_INFO(0, infoFormat)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setProcessing_args, 0, 0, 1)
ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setProcessingKey_args, 0, 0, 2)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_applySLD_args, 0, 0, 2)
ZEND_ARG_INFO(0, sldxml)
ZEND_ARG_INFO(0, namedLayer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_applySLDURL_args, 0, 0, 2)
ZEND_ARG_INFO(0, sldurl)
ZEND_ARG_INFO(0, namedLayer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_moveClassUp_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_moveClassDown_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_removeClass_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setConnectionType_args, 0, 0, 2)
ZEND_ARG_INFO(0, connectionType)
ZEND_ARG_INFO(0, pluginLibrary)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_draw_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_drawQuery_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_queryByPoint_args, 0, 0, 3)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_ARG_INFO(0, mode)
ZEND_ARG_INFO(0, buffer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_queryByRect_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, rect, rectObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_queryByShape_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_queryByFeatures_args, 0, 0, 1)
ZEND_ARG_INFO(0, slayer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_queryByAttributes_args, 0, 0, 3)
ZEND_ARG_INFO(0, item)
ZEND_ARG_INFO(0, string)
ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_queryByFilter_args, 0, 0, 1)
ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_queryByIndex_args, 0, 0, 2)
ZEND_ARG_INFO(0, tileindex)
ZEND_ARG_INFO(0, shapeindex)
ZEND_ARG_INFO(0, addtoquery)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_getShape_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, record, resultObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(layer_setGeomTransform_args, 0, 0, 1)
ZEND_ARG_INFO(0, transform)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct(mapObj map [, layerObj layer])
   Create a new layerObj instance. */
PHP_METHOD(layerObj, __construct)
{
  zval *zobj = getThis();
  zval *zmap, *zlayer = NULL;
  layerObj *layer;
  int index;
  php_map_object *php_map;
  php_layer_object *php_layer, *php_layer2=NULL;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|O",
                            &zmap, mapscript_ce_map,
                            &zlayer, mapscript_ce_layer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *)zend_object_store_get_object(zobj TSRMLS_CC);
  php_map = (php_map_object *)zend_object_store_get_object(zmap TSRMLS_CC);
  if (zlayer)
    php_layer2 = (php_layer_object *)zend_object_store_get_object(zlayer TSRMLS_CC);

  if ((layer = layerObj_new(php_map->map)) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  php_layer->layer = layer;
  php_layer->is_ref = 1;

  MAPSCRIPT_MAKE_PARENT(zmap, NULL);
  php_layer->parent = parent;
  MAPSCRIPT_ADDREF(zmap);

  /* if a layer is passed as argument, copy the layer into
     the new one */
  if (zlayer) {
    index = layer->index;
    msCopyLayer(layer, php_layer2->layer);
    layer->index = index;
  }


  if (layer->connectiontype != MS_GRATICULE || layer->layerinfo == NULL) {
    MAKE_STD_ZVAL(php_layer->grid);
    ZVAL_NULL(php_layer->grid);
  }
}
/* }}} */

PHP_METHOD(layerObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("numclasses",  php_layer->layer->numclasses)
  else IF_GET_STRING("encoding", php_layer->layer->encoding)
    else IF_GET_LONG("index", php_layer->layer->index)
      else IF_GET_LONG("status", php_layer->layer->status)
        else IF_GET_LONG("debug",  php_layer->layer->debug)
          else IF_GET_STRING("bandsitem", php_layer->layer->bandsitem)
            else IF_GET_STRING("classitem", php_layer->layer->classitem)
              else IF_GET_STRING("classgroup", php_layer->layer->classgroup)
                else IF_GET_STRING("name", php_layer->layer->name)
                  else IF_GET_STRING("group", php_layer->layer->group)
                    else IF_GET_STRING("data", php_layer->layer->data)
                      else IF_GET_LONG("type",  php_layer->layer->type)
                        else IF_GET_LONG("dump", php_layer->layer->dump)
                          else IF_GET_DOUBLE("tolerance", php_layer->layer->tolerance)
                            else IF_GET_LONG("toleranceunits", php_layer->layer->toleranceunits)
                              else IF_GET_LONG("sizeunits", php_layer->layer->sizeunits)
                                else IF_GET_DOUBLE("symbolscaledenom", php_layer->layer->symbolscaledenom)
                                  else IF_GET_LONG("maxclasses",  php_layer->layer->maxclasses)
                                    else IF_GET_DOUBLE("minscaledenom", php_layer->layer->minscaledenom)
                                      else IF_GET_DOUBLE("maxscaledenom", php_layer->layer->maxscaledenom)
                                        else IF_GET_DOUBLE("labelminscaledenom", php_layer->layer->labelminscaledenom)
                                          else IF_GET_DOUBLE("labelmaxscaledenom", php_layer->layer->labelmaxscaledenom)
                                            else IF_GET_DOUBLE("maxgeowidth", php_layer->layer->maxgeowidth)
                                              else IF_GET_DOUBLE("mingeowidth", php_layer->layer->mingeowidth)
                                                else IF_GET_STRING("mask", php_layer->layer->mask)
                                                  else IF_GET_LONG("minfeaturesize", php_layer->layer->minfeaturesize)
                                                    else IF_GET_LONG("maxfeatures", php_layer->layer->maxfeatures)
                                                      else IF_GET_LONG("startindex", php_layer->layer->startindex)
                                                        else IF_GET_LONG("transform", php_layer->layer->transform)
                                                          else IF_GET_LONG("labelcache", php_layer->layer->labelcache)
                                                            else IF_GET_LONG("postlabelcache", php_layer->layer->postlabelcache)
                                                              else IF_GET_STRING("labelitem", php_layer->layer->labelitem)
                                                                else IF_GET_STRING("tileitem", php_layer->layer->tileitem)
                                                                  else IF_GET_STRING("tileindex", php_layer->layer->tileindex)
                                                                    else IF_GET_STRING("header", php_layer->layer->header)
                                                                      else IF_GET_STRING("footer", php_layer->layer->footer)
                                                                        else IF_GET_STRING("connection", php_layer->layer->connection)
                                                                          else IF_GET_LONG("connectiontype", php_layer->layer->connectiontype)
                                                                            else IF_GET_STRING("filteritem", php_layer->layer->filteritem)
                                                                              else IF_GET_STRING("template", php_layer->layer->template)
                                                                                else IF_GET_LONG("opacity", (php_layer->layer->compositer?php_layer->layer->compositer->opacity:100))
                                                                                  else IF_GET_STRING("styleitem", php_layer->layer->styleitem)
                                                                                    else IF_GET_LONG("numitems", php_layer->layer->numitems)
                                                                                      else IF_GET_LONG("numjoins", php_layer->layer->numjoins)
                                                                                        else IF_GET_LONG("num_processing", php_layer->layer->numprocessing)
                                                                                          else IF_GET_STRING("requires", php_layer->layer->requires)
                                                                                            else IF_GET_STRING("labelrequires", php_layer->layer->labelrequires)
                                                                                              else IF_GET_OBJECT("offsite", mapscript_ce_color, php_layer->offsite, &php_layer->layer->offsite)
                                                                                                else IF_GET_OBJECT("extent", mapscript_ce_rect, php_layer->extent, &php_layer->layer->extent)
                                                                                                  else IF_GET_OBJECT("grid",  mapscript_ce_grid, php_layer->grid, (graticuleObj *)(php_layer->layer->layerinfo))
                                                                                                    else IF_GET_OBJECT("metadata", mapscript_ce_hashtable, php_layer->metadata, &php_layer->layer->metadata)
                                                                                                      else IF_GET_OBJECT("bindvals", mapscript_ce_hashtable, php_layer->bindvals, &php_layer->layer->bindvals)
                                                                                                        else IF_GET_OBJECT("cluster", mapscript_ce_cluster, php_layer->cluster, &php_layer->layer->cluster)
                                                                                                          else IF_GET_OBJECT("projection", mapscript_ce_projection, php_layer->projection, &php_layer->layer->projection)
                                                                                                            else {
                                                                                                              mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                                                                                            }
}

PHP_METHOD(layerObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  /* special case for "template" which we want to set to NULL and not an empty string */
  if(Z_TYPE_P(value)==IS_NULL && !strcmp(property,"template")) {
    msFree(php_layer->layer->template);
    php_layer->layer->template = NULL;
  } else if(!strcmp(property,"opacity")) {
    msSetLayerOpacity(php_layer->layer,Z_LVAL_P(value));
  } else
  IF_SET_LONG("status", php_layer->layer->status, value)
  else IF_SET_STRING("encoding",  php_layer->layer->encoding, value)
    else IF_SET_LONG("debug",  php_layer->layer->debug, value)
      else IF_SET_STRING("classitem", php_layer->layer->classitem, value)
        else IF_SET_STRING("bandsitem", php_layer->layer->bandsitem, value)
          else IF_SET_STRING("classgroup", php_layer->layer->classgroup, value)
            else IF_SET_STRING("name", php_layer->layer->name, value)
              else IF_SET_STRING("group", php_layer->layer->group, value)
                else IF_SET_STRING("data", php_layer->layer->data, value)
                  else IF_SET_LONG("type",  php_layer->layer->type, value)
                    else IF_SET_LONG("dump", php_layer->layer->dump, value)
                      else IF_SET_DOUBLE("tolerance", php_layer->layer->tolerance, value)
                        else IF_SET_LONG("toleranceunits", php_layer->layer->toleranceunits, value)
                          else IF_SET_LONG("sizeunits", php_layer->layer->sizeunits, value)
                            else IF_SET_DOUBLE("symbolscaledenom", php_layer->layer->symbolscaledenom, value)
                              else IF_SET_DOUBLE("minscaledenom", php_layer->layer->minscaledenom, value)
                                else IF_SET_DOUBLE("maxscaledenom", php_layer->layer->maxscaledenom, value)
                                  else IF_SET_LONG("minfeaturesize", php_layer->layer->minfeaturesize, value)
                                    else IF_SET_DOUBLE("labelminscaledenom", php_layer->layer->labelminscaledenom, value)
                                      else IF_SET_DOUBLE("labelmaxscaledenom", php_layer->layer->labelmaxscaledenom, value)
                                        else IF_SET_DOUBLE("maxgeowidth", php_layer->layer->maxgeowidth, value)
                                          else IF_SET_DOUBLE("mingeowidth", php_layer->layer->mingeowidth, value)
                                            else IF_SET_STRING("mask", php_layer->layer->mask, value)
                                              else IF_SET_LONG("maxfeatures", php_layer->layer->maxfeatures, value)
                                                else IF_SET_LONG("startindex", php_layer->layer->startindex, value)
                                                  else IF_SET_LONG("transform", php_layer->layer->transform, value)
                                                    else IF_SET_LONG("labelcache", php_layer->layer->labelcache, value)
                                                      else IF_SET_LONG("postlabelcache", php_layer->layer->postlabelcache, value)
                                                        else IF_SET_STRING("labelitem", php_layer->layer->labelitem, value)
                                                          else IF_SET_STRING("tileitem", php_layer->layer->tileitem, value)
                                                            else IF_SET_STRING("tileindex", php_layer->layer->tileindex, value)
                                                              else IF_SET_STRING("header", php_layer->layer->header, value)
                                                                else IF_SET_STRING("footer", php_layer->layer->footer, value)
                                                                  else IF_SET_STRING("connection", php_layer->layer->connection, value)
                                                                    else IF_SET_STRING("filteritem", php_layer->layer->filteritem, value)
                                                                      else IF_SET_STRING("template", php_layer->layer->template, value)
                                                                          else IF_SET_STRING("styleitem", php_layer->layer->styleitem, value)
                                                                            else IF_SET_LONG("num_processing", php_layer->layer->numprocessing, value)
                                                                              else IF_SET_STRING("requires", php_layer->layer->requires, value)
                                                                                else IF_SET_STRING("labelrequires", php_layer->layer->labelrequires, value)
                                                                                  else if ( (STRING_EQUAL("offsite", property)) ||
                                                                                            (STRING_EQUAL("grid", property)) ||
                                                                                            (STRING_EQUAL("metadata", property)) ||
                                                                                            (STRING_EQUAL("bindvals", property)) ||
                                                                                            (STRING_EQUAL("projection", property)) ||
                                                                                            (STRING_EQUAL("maxclasses", property)) ||
                                                                                            (STRING_EQUAL("numitems", property)) ||
                                                                                            (STRING_EQUAL("numjoins", property)) ||
                                                                                            (STRING_EQUAL("extent", property)) ||
                                                                                            (STRING_EQUAL("cluster", property)) ) {
                                                                                    mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                                                                                  } else if ( (STRING_EQUAL("numclasses", property)) ||
                                                                                              (STRING_EQUAL("index", property)) ||
                                                                                              (STRING_EQUAL("connectiontype", property)) ) {
                                                                                    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
                                                                                  } else {
                                                                                    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                                                                  }
}

/* {{{ proto int draw(imageObj image)
   Draw a single layer, add labels to cache if required. Returns MS_FAILURE on error. */
PHP_METHOD(layerObj, draw)
{
  zval *zobj = getThis();
  zval *zimage;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zimage, mapscript_ce_image) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_image = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_draw(php_layer->layer, php_map->map, php_image->image)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int drawQuery(image img)
   Draw query results for a layer. */
PHP_METHOD(layerObj, drawQuery)
{
  zval *zobj = getThis();
  zval *zimage;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zimage, mapscript_ce_image) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_image = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_drawQuery(php_layer->layer, php_map->map, php_image->image)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int queryByPoint(pointObj point, int type, double buffer)
   Query at point location. */
PHP_METHOD(layerObj, queryByPoint)
{
  zval *zobj = getThis();
  zval *zpoint;
  int status = MS_FAILURE;
  long mode;
  double buffer;
  php_layer_object *php_layer;
  php_map_object *php_map;
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Old",
                            &zpoint, mapscript_ce_point,
                            &mode, &buffer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_queryByPoint(php_layer->layer, php_map->map, php_point->point,
                                      mode, buffer)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int queryByRect(rectObj rect)
   Query using rectangular extent. */
PHP_METHOD(layerObj, queryByRect)
{
  zval *zobj = getThis();
  zval *zrect;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;
  php_rect_object *php_rect;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zrect, mapscript_ce_rect) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_rect = (php_rect_object *) zend_object_store_get_object(zrect TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_queryByRect(php_layer->layer, php_map->map, *(php_rect->rect))) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int queryByShape(shapeObj shape)
   Query using a shape */
PHP_METHOD(layerObj, queryByShape)
{
  zval *zobj = getThis();
  zval *zshape;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_queryByShape(php_layer->layer, php_map->map, php_shape->shape)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int queryByFeatures(int slayer)
   Query on a layer using query object. */
PHP_METHOD(layerObj, queryByFeatures)
{
  zval *zobj = getThis();
  long slayer;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &slayer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_queryByFeatures(php_layer->layer, php_map->map, slayer)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.queryAttributes(string item, string string, int mode)
   Query at point location. */
PHP_METHOD(layerObj, queryByAttributes)
{
  zval *zobj = getThis();
  char *item;
  long item_len = 0;
  char *string;
  long string_len = 0;
  long mode;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl",
                            &item, &item_len,
                            &string, &string_len,
                            &mode) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_queryByAttributes(php_layer->layer, php_map->map,
                item, string, mode)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.queryByFilter(string string)
   Query by filter. */
PHP_METHOD(layerObj, queryByFilter)
{
  zval *zobj = getThis();
  char *string;
  long string_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &string, &string_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_queryByFilter(php_layer->layer, php_map->map,
                                       string)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.queryByIndex(int tileindex, int shapeindex, int addtoquery)
   Query by index. */
PHP_METHOD(layerObj, queryByIndex)
{
  zval *zobj = getThis();
  long tileindex, shapeindex;
  long addtoquery=MS_FALSE;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|l",
                            &tileindex, &shapeindex, &addtoquery) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((status = layerObj_queryByIndex(php_layer->layer, php_map->map,
                                      tileindex, shapeindex, addtoquery)) != MS_SUCCESS)
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.updateFromString(string snippet)
   Update a layer from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(layerObj, updateFromString)
{
  zval *zobj = getThis();
  char *snippet;
  long snippet_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((status = layerObj_updateFromString(php_layer->layer, snippet)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string convertToString()
   Convert the layer object to string. */
PHP_METHOD(layerObj, convertToString)
{
  zval *zobj = getThis();
  php_layer_object *php_layer;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value =  layerObj_convertToString(php_layer->layer);

  if (value == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int layer.getClass(int i)
   Returns a classObj from the layer given an index value (0=first class) */
PHP_METHOD(layerObj, getClass)
{
  zval *zobj = getThis();
  long index;
  classObj *class = NULL;
  php_layer_object *php_layer;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((class = layerObj_getClass(php_layer->layer, index)) == NULL) {
    mapscript_throw_exception("Invalid class index." TSRMLS_CC);
    return;
  }

  /* Return class object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_class(class, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int layer.getClassIndex(shapeObj shape [, string classGroup, int numClasses])
   Returns the class index for the shape */
PHP_METHOD(layerObj, getClassIndex)
{
  zval *zobj = getThis();
  zval *zshape, **ppzval, *zclassgroup = NULL;
  int numElements, *classGroups = NULL;
  int retval = -1, i = 0;
  long numClasses = 0;
  HashTable *classgroup_hash = NULL;
  php_shape_object *php_shape;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|a!l",
                            &zshape, mapscript_ce_shape,
                            &zclassgroup,
                            &numClasses) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if (zclassgroup) {
    classgroup_hash = Z_ARRVAL_P(zclassgroup);
    numElements = zend_hash_num_elements(classgroup_hash);
    classGroups = (int*)malloc(sizeof(int)*numElements);

    for(zend_hash_internal_pointer_reset(classgroup_hash);
        zend_hash_has_more_elements(classgroup_hash) == SUCCESS;
        zend_hash_move_forward(classgroup_hash), ++i) {
      zend_hash_get_current_data(classgroup_hash, (void **)&ppzval);
      classGroups[i] = Z_LVAL_PP(ppzval);
    }
  }

  retval = layerObj_getClassIndex(php_layer->layer, php_map->map, php_shape->shape, classGroups, numClasses);

  if (zclassgroup)
    free(classGroups);

  RETURN_LONG(retval);
}
/* }}} */


/* {{{ proto int layer.setFilter(string filter)
   Set layer filter expression.  Returns 0 on success, -1 in error. */
PHP_METHOD(layerObj, setFilter)
{
  zval *zobj = getThis();
  char *expression;
  long expression_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &expression, &expression_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((status = layerObj_setFilter(php_layer->layer, expression)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string layer.getFilterString()
    Return the layer's filter expression. Returns NULL on error. */
PHP_METHOD(layerObj, getFilterString)
{
  zval *zobj = getThis();
  char *value = NULL;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = layerObj_getFilter(php_layer->layer);
  if (value == NULL) {
    RETURN_NULL();
  }

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int layer.setProjection(string projection_params)
   Set projection and coord. system for the layer. */
PHP_METHOD(layerObj, setProjection)
{
  zval *zobj = getThis();
  char *projection;
  long projection_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_projection_object *php_projection=NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &projection, &projection_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  if (php_layer->projection)
    php_projection = (php_projection_object *) zend_object_store_get_object(php_layer->projection TSRMLS_CC);

  if ((status = layerObj_setProjection(php_layer->layer, projection)) != MS_SUCCESS) {
    mapscript_report_php_error(E_WARNING, "setProjection failed" TSRMLS_CC);
    RETURN_LONG(status);
  }

  if (php_layer->projection)
    php_projection->projection = &(php_layer->layer->projection);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.getProjection()
    Return the projection string of the layer. Returns NULL on error. */
PHP_METHOD(layerObj, getProjection)
{
  zval *zobj = getThis();
  char *projection = NULL;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  projection = layerObj_getProjection(php_layer->layer);
  if (projection == NULL) {
    RETURN_NULL();
  }

  RETVAL_STRING(projection, 1);
  free(projection);
}
/* }}} */

/* {{{ proto int layer.setWKTProjection(string projection_params)
   Set projection and coord. system for the layer. */
PHP_METHOD(layerObj, setWKTProjection)
{
  zval *zobj = getThis();
  char *projection;
  long projection_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_projection_object *php_projection=NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &projection, &projection_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  if (php_layer->projection)
    php_projection = (php_projection_object *) zend_object_store_get_object(php_layer->projection TSRMLS_CC);

  if ((status = layerObj_setWKTProjection(php_layer->layer, projection)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  if (php_layer->projection)
    php_projection->projection = &(php_layer->layer->projection);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.getNumResults()
   Returns the number of results from this layer in the last query. */
PHP_METHOD(layerObj, getNumResults)
{
  zval *zobj = getThis();
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);


  if (!php_layer->layer->resultcache)
    RETURN_LONG(0);

  RETURN_LONG(php_layer->layer->resultcache->numresults);
}
/* }}} */

/* {{{ proto int layer.getResultsBounds()
   Returns the bounds of results from this layer in the last query. */
PHP_METHOD(layerObj, getResultsBounds)
{
  zval *zobj = getThis();
  php_layer_object *php_layer;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->layer->resultcache)
    RETURN_NULL();

  /* Return result object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_rect(&(php_layer->layer->resultcache->bounds),
                        parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int layer.getResult(int i)
   Returns a resultObj by index from a layer object.*/
PHP_METHOD(layerObj, getResult)
{
  zval *zobj = getThis();
  long index;
  resultObj *result = NULL;
  php_layer_object *php_layer;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((result = layerObj_getResult(php_layer->layer, index)) == NULL) {
    mapscript_throw_exception("Invalid result index." TSRMLS_CC);
    return;
  }

  /* Return result object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_result(&(php_layer->layer->resultcache->results[index]),
                          parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int layer.open()
   Open the layer for use with getShape(). Returns MS_SUCCESS/MS_FAILURE. */
PHP_METHOD(layerObj, open)
{
  zval *zobj = getThis();
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);


  status = layerObj_open(php_layer->layer);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_LONG(status);
  } else {
    msLayerGetItems(php_layer->layer);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.whichshapes(rectObj extent) Returns MS_SUCCESS/MS_FAILURE. */
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
PHP_METHOD(layerObj, whichShapes)
{
  zval *zobj = getThis();
  zval *zrect;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_rect_object *php_rect;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zrect, mapscript_ce_rect) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_rect = (php_rect_object *) zend_object_store_get_object(zrect TSRMLS_CC);


  status = layerObj_whichShapes(php_layer->layer, php_rect->rect);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int layer.nextshape() Returns a shape or NULL. */
PHP_METHOD(layerObj, nextShape)
{
  zval *zobj = getThis();
  shapeObj *shape = NULL;
  php_layer_object *php_layer;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  shape = layerObj_nextShape(php_layer->layer);

  if (!shape)
    RETURN_NULL();

  /* Return valid object */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, php_layer, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int layer.close()
   Close layer previously opened with open(). */
PHP_METHOD(layerObj, close)
{
  zval *zobj = getThis();
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  layerObj_close(php_layer->layer);
}
/* }}} */

/* {{{ proto int layer.getExtent()
   Retrieve or calculate a layer's extents. */
PHP_METHOD(layerObj, getExtent)
{
  zval *zobj = getThis();
  rectObj *rect = NULL;
  php_layer_object *php_layer;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  /* Create a new rectObj to hold the result */
  if ((rect = rectObj_new()) == NULL) {
    mapscript_throw_mapserver_exception("Failed creating new rectObj (out of memory?)" TSRMLS_CC);
    return;
  }

  if (msLayerGetExtent(php_layer->layer, rect) != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_NULL();
  }

  /* Return rectObj */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_rect(rect, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int layer.setExtent(int minx, int miny, int maxx, int maxy)
   Set the layer extent. */
PHP_METHOD(layerObj, setExtent)
{
  zval *zobj = getThis();
  long minx, miny, maxx, maxy;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llll",
                            &minx, &miny, &maxx, &maxy) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (minx > maxx || miny > maxy) {
    mapscript_throw_mapserver_exception("Invalid min/max values" TSRMLS_CC);
    return;
  }

  RETURN_LONG(msLayerSetExtent(php_layer->layer, minx, miny, maxx, maxy))
}
/* }}} */


/* {{{ proto int layer.addFeature(ShapeObj poShape)
   Add a shape */
PHP_METHOD(layerObj, addFeature)
{
  zval *zobj = getThis();
  zval *zshape;
  int status = MS_FAILURE;
  php_layer_object *php_layer;
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);


  status = layerObj_addFeature(php_layer->layer, php_shape->shape);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string layer.getMetaData(string name)
   Return MetaData entry by name, or empty string if not found. */
PHP_METHOD(layerObj, getMetaData)
{
  zval *zname;
  zval *zobj = getThis();
  php_layer_object *php_layer;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",
                            &zname) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_layer->metadata, &php_layer->layer->metadata);

  MAPSCRIPT_CALL_METHOD_1(php_layer->metadata, "get", retval, zname);

  RETURN_STRING(Z_STRVAL_P(retval),1);
}
/* }}} */

/* {{{ proto int layer.setMetaData(string name, string value)
   Set MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(layerObj, setMetaData)
{
  zval *zname, *zvalue;
  zval *zobj = getThis();
  php_layer_object *php_layer;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz",
                            &zname, &zvalue) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_layer->metadata, &php_layer->layer->metadata);

  MAPSCRIPT_CALL_METHOD_2(php_layer->metadata, "set", retval, zname, zvalue);

  RETURN_LONG(Z_LVAL_P(retval));
}
/* }}} */

/* {{{ proto int layer.removeMetaData(string name)
   Remove MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(layerObj, removeMetaData)
{
  zval *zname;
  zval *zobj = getThis();
  php_layer_object *php_layer;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",
                            &zname) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_layer->metadata, &php_layer->layer->metadata);

  MAPSCRIPT_CALL_METHOD_1(php_layer->metadata, "remove", retval, zname);

  RETURN_LONG(Z_LVAL_P(retval));
}
/* }}} */

/* {{{ proto string layer.getWMSFeatureInfoURL(int clickX, int clickY, int featureCount, string infoFormat)
   Return a WMS GetFeatureInfo URL (only for WMS layers). */
PHP_METHOD(layerObj, getWMSFeatureInfoURL)
{
  zval *zobj = getThis();
  long clickx, clicky, featureCount;
  char *infoFormat = NULL;
  long infoFormat_len = 0;
  char *value =  NULL;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llls",
                            &clickx, &clicky, &featureCount,
                            &infoFormat, &infoFormat_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((value = layerObj_getWMSFeatureInfoURL(php_layer->layer, php_map->map,
               clickx,
               clicky,
               featureCount,
               infoFormat)) == NULL) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_STRING("", 1);
  }

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto char** layer.getItems()
   Return an array containing all the layer items.*/
PHP_METHOD(layerObj, getItems)
{
  zval *zobj = getThis();
  int i, status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  array_init(return_value);
  status = msLayerGetItems(php_layer->layer);

  if (status != MS_FAILURE &&  php_layer->layer->numitems > 0) {
    for (i=0; i<php_layer->layer->numitems; i++) {
      /* add a copy of the group name to the PHP array */
      add_next_index_string(return_value, php_layer->layer->items[i], 1);
    }
  }
}
/* }}} */

/* {{{ boolean layer.setProcessing(string)
  set a processing string to the layer*/
PHP_METHOD(layerObj, setProcessing)
{
  zval *zobj = getThis();
  char *string = NULL;
  long string_len = 0;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &string, &string_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  php_layer->layer->numprocessing++;
  if(php_layer->layer->numprocessing == 1)
    php_layer->layer->processing = (char **) malloc(2*sizeof(char *));
  else
    php_layer->layer->processing = (char **) realloc(php_layer->layer->processing, sizeof(char*) * (php_layer->layer->numprocessing+1));

  php_layer->layer->processing[php_layer->layer->numprocessing-1] = strdup(string);
  php_layer->layer->processing[php_layer->layer->numprocessing] = NULL;

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ boolean layer.setProcessingKey(string key, string value)
  set a processing key string to the layer*/
PHP_METHOD(layerObj, setProcessingKey)
{
  zval *zobj = getThis();
  char *key = NULL;
  long key_len = 0;
  char *value = NULL;
  long value_len = 0;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                            &key, &key_len, &value, &value_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  msLayerSetProcessingKey( php_layer->layer, key, value );

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto char** layer.getProcessing()
   Return an array containing all the processing strings.*/
PHP_METHOD(layerObj, getProcessing)
{
  zval *zobj = getThis();
  int i;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  array_init(return_value);
  if (php_layer->layer->numprocessing > 0) {
    for (i=0; i<php_layer->layer->numprocessing; i++) {
      add_next_index_string(return_value, php_layer->layer->processing[i], 1);
    }
  }
}
/* }}} */

/* {{{ boolean layer.clearProcessing()
   clear the processing strings in the layer*/
PHP_METHOD(layerObj, clearProcessing)
{
  zval *zobj = getThis();
  int i;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (php_layer->layer->numprocessing > 0) {
    for(i=0; i<php_layer->layer->numprocessing; i++)
      free(php_layer->layer->processing[i]);

    php_layer->layer->numprocessing = 0;
    free(php_layer->layer->processing);
  }
}
/* }}} */

/* {{{ string layer.executewfsgetfeature()
   Executes a GetFeature request on a WFS layer and returns the name of the temporary GML file created. Returns an empty string on error.*/
PHP_METHOD(layerObj, executeWFSGetFeature)
{
  zval *zobj = getThis();
  char *value = NULL;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((value=layerObj_executeWFSGetFeature(php_layer->layer)) == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int applySLD(string sldxml, string namedlayer)
   Apply the SLD document to the layer object. The matching between the sld
   document and the layer will be done using the layer's name. If a
   namedlayer argument is passed (argument is optional), the NamedLayer in
   the sld that matchs it will be used to style the layer. */
PHP_METHOD(layerObj, applySLD)
{
  zval *zobj = getThis();
  char *sldxml;
  long sldxml_len = 0;
  char *namedLayer = NULL;
  long namedLayer_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                            &sldxml, &sldxml_len,
                            &namedLayer, &namedLayer_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = layerObj_applySLD(php_layer->layer, sldxml, namedLayer);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int applySLDURL(string sldurl, string namedlayer)
   Apply the SLD document pointed by the URL to the layer object. The
   matching between the sld document and the layer will be done using the
   layer's name. If a namedlayer argument is passed (argument is optional),
   the NamedLayer in the sld that matchs it will be used to style the
   layer. */
PHP_METHOD(layerObj, applySLDURL)
{
  zval *zobj = getThis();
  char *sldurl;
  long sldurl_len = 0;
  char *namedLayer = NULL;
  long namedLayer_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                            &sldurl, &sldurl_len,
                            &namedLayer, &namedLayer_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = layerObj_applySLDURL(php_layer->layer, sldurl, namedLayer);

  RETURN_LONG(status);
}
/* }}} */


/* {{{ proto string generateSLD()
   Returns an SLD XML string based on all the classes found in the layers.*/
PHP_METHOD(layerObj, generateSLD)
{
  zval *zobj = getThis();
  char *buffer = NULL;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  buffer = layerObj_generateSLD(php_layer->layer);

  if (buffer) {
    RETVAL_STRING(buffer, 1);
    free(buffer);
  } else {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_STRING("", 1);
  }
}
/* }}} */

/* {{{ proto int moveClassUp(int index)
    The class specified by the class index will be moved up into the array
    of layers. Returns MS_SUCCESS or MS_FAILURE. */
PHP_METHOD(layerObj, moveClassUp)
{
  zval *zobj = getThis();
  long index;
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = layerObj_moveClassUp(php_layer->layer, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int moveClassDown(int index)
   The class specified by the class index will be moved down into the array
   of layers. Returns MS_SUCCESS or MS_FAILURE.*/
PHP_METHOD(layerObj, moveClassDown)
{
  zval *zobj = getThis();
  long index;
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = layerObj_moveClassDown(php_layer->layer, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int removeClass(int index)
   Removes the class indicated and returns a copy, or NULL in the case of a
   failure.*/
PHP_METHOD(layerObj, removeClass)
{
  zval *zobj = getThis();
  long index;
  classObj *class;
  php_layer_object *php_layer;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((class = layerObj_removeClass(php_layer->layer, index)) == NULL) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_NULL();
  }

  /* Return a copy of the class object just removed */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_class(class, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int layer.isVisible()
   Returns MS_TRUE/MS_FALSE depending on whether the layer is currently
   visible in the map (i.e. turned on, in scale, etc.). */
PHP_METHOD(layerObj, isVisible)
{
  zval *zobj = getThis();
  int retval = MS_FALSE;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  retval = msLayerIsVisible(php_map->map, php_layer->layer);

  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int layer.setConnectionType(int connectionType, string pluginLibrary)
   Set layer connectiontype.  Returns MS_SUCCESS/MS_FAILURE. */
PHP_METHOD(layerObj, setConnectionType)
{
  zval *zobj = getThis();
  long type;
  char *plugin = "";
  long plugin_len = 0;
  int status = MS_FAILURE;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|s",
                            &type,
                            &plugin, &plugin_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((status = layerObj_setConnectionType(php_layer->layer,
                type,
                plugin)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  if (php_layer->layer->connectiontype != MS_GRATICULE || php_layer->layer->layerinfo == NULL) {
    if (php_layer->grid && Z_TYPE_P(php_layer->grid) == IS_OBJECT) {
      MAPSCRIPT_DELREF(php_layer->grid);
      MAKE_STD_ZVAL(php_layer->grid);
      ZVAL_NULL(php_layer->grid);
    }
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int getGridIntersctionCoordinates()
   Not sure if this function is often used, let it as it is. */
PHP_METHOD(layerObj, getGridIntersectionCoordinates)
{
  zval *zobj = getThis();
  graticuleIntersectionObj *values=NULL;
  zval *tmp_arr1;
  int i=0;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this layer object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);


  array_init(return_value);

  if (php_layer->layer->connectiontype != MS_GRATICULE) {
    mapscript_throw_exception("Layer is not of graticule type" TSRMLS_CC);
    return;
  }

  values = msGraticuleLayerGetIntersectionPoints(php_map->map, php_layer->layer);

  if (values == NULL)
    return; /* empty array is returned */

  /*TOP*/
  add_assoc_double(return_value, "topnumpoints", values->nTop);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nTop; i++) {
    add_next_index_string(tmp_arr1, values->papszTopLabels[i],1);
  }
  zend_hash_update(Z_ARRVAL_P(return_value), "toplabels", strlen("toplabels")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nTop; i++) {
    add_next_index_double(tmp_arr1, values->pasTop[i].x);
    add_next_index_double(tmp_arr1, values->pasTop[i].y);

  }

  zend_hash_update(Z_ARRVAL_P(return_value), "toppoints", strlen("toppoints")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);

  /*BOTTOM*/

  add_assoc_double(return_value, "bottomnumpoints", values->nBottom);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nBottom; i++) {
    add_next_index_string(tmp_arr1, values->papszBottomLabels[i],1);
  }
  zend_hash_update(Z_ARRVAL_P(return_value), "bottomlabels", strlen("bottomlabels")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nBottom; i++) {
    add_next_index_double(tmp_arr1, values->pasBottom[i].x);
    add_next_index_double(tmp_arr1, values->pasBottom[i].y);

  }

  zend_hash_update(Z_ARRVAL_P(return_value), "bottompoints", strlen("bottompoints")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);


  /*LEFT*/
  add_assoc_double(return_value, "leftnumpoints", values->nLeft);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nLeft; i++) {
    add_next_index_string(tmp_arr1, values->papszLeftLabels[i],1);
  }
  zend_hash_update(Z_ARRVAL_P(return_value), "leftlabels", strlen("leftlabels")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nLeft; i++) {
    add_next_index_double(tmp_arr1, values->pasLeft[i].x);
    add_next_index_double(tmp_arr1, values->pasLeft[i].y);

  }

  zend_hash_update(Z_ARRVAL_P(return_value), "leftpoints", strlen("leftpoints")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);


  /*RIGHT*/
  add_assoc_double(return_value, "rightnumpoints", values->nRight);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nRight; i++) {
    add_next_index_string(tmp_arr1, values->papszRightLabels[i],1);
  }
  zend_hash_update(Z_ARRVAL_P(return_value), "rightlabels", strlen("rightlabels")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);

  MAKE_STD_ZVAL(tmp_arr1);
  array_init(tmp_arr1);
  for (i=0; i<values->nRight; i++) {
    add_next_index_double(tmp_arr1, values->pasRight[i].x);
    add_next_index_double(tmp_arr1, values->pasRight[i].y);

  }

  zend_hash_update(Z_ARRVAL_P(return_value), "rightpoints", strlen("rightpoints")+1, &tmp_arr1,
                   sizeof(tmp_arr1), NULL);

  msGraticuleLayerFreeIntersectionPoints(values);
}
/* }}} */

/* {{{ proto shapeObj layer.getShape(record)
   Retrieve shapeObj from a resultset by index. */
PHP_METHOD(layerObj, getShape)
{
  zval *zobj = getThis();
  zval *zresult;
  shapeObj *shape = NULL;
  php_result_object *php_result;
  php_layer_object *php_layer;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zresult, mapscript_ce_result) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_result = (php_result_object *) zend_object_store_get_object(zresult TSRMLS_CC);

  /* Create a new shapeObj to hold the result
   * Note that the type used to create the shape (MS_NULL) does not matter
   * at this point since it will be set by SHPReadShape().
   */
  if ((shape = shapeObj_new(MS_SHAPE_NULL)) == NULL) {
    mapscript_throw_mapserver_exception("Failed creating new shape (out of memory?)" TSRMLS_CC);
    return;
  }

  if (msLayerGetShape(php_layer->layer, shape, php_result->result) != MS_SUCCESS) {
    shapeObj_destroy(shape);
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* Return valid object */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, php_layer, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int layer.getGeomTransform()
   return the geometry transform expression */
PHP_METHOD(layerObj, getGeomTransform)
{
  zval *zobj = getThis();
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (php_layer->layer->_geomtransform.type == MS_GEOMTRANSFORM_NONE ||
      !php_layer->layer->_geomtransform.string)
    RETURN_STRING("", 1);

  RETURN_STRING(php_layer->layer->_geomtransform.string, 1);
}
/* }}} */

/* {{{ proto int layer.setGeomTransform()
   set the geometry transform expression */
PHP_METHOD(layerObj, setGeomTransform)
{
  zval *zobj = getThis();
  char *transform;
  long transform_len = 0;
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &transform, &transform_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  msFree(php_layer->layer->_geomtransform.string);
  if (transform_len > 0) {
    php_layer->layer->_geomtransform.string = msStrdup(transform);
    php_layer->layer->_geomtransform.type = MS_GEOMTRANSFORM_EXPRESSION;
  }
  else {
    php_layer->layer->_geomtransform.type = MS_GEOMTRANSFORM_NONE;
    php_layer->layer->_geomtransform.string = NULL;    
  }

  RETURN_LONG(MS_SUCCESS);  
}

/* {{{ proto void layer.free()
   Free the object */
PHP_METHOD(layerObj, free)
{
  zval *zobj = getThis();
  php_layer_object *php_layer;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_layer->offsite);
  if (php_layer->grid && Z_TYPE_P(php_layer->grid) == IS_OBJECT)
    MAPSCRIPT_DELREF(php_layer->grid);
  MAPSCRIPT_DELREF(php_layer->metadata);
  MAPSCRIPT_DELREF(php_layer->bindvals);
  MAPSCRIPT_DELREF(php_layer->cluster);
  MAPSCRIPT_DELREF(php_layer->projection);
}
/* }}} */

zend_function_entry layer_functions[] = {
  PHP_ME(layerObj, __construct, layer___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(layerObj, __get, layer___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, __set, layer___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(layerObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, draw, layer_draw_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, drawQuery, layer_drawQuery_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, updateFromString, layer_updateFromString_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, convertToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getClass, layer_getClass_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getClassIndex, layer_getClassIndex_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, queryByPoint, layer_queryByPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, queryByRect, layer_queryByRect_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, queryByShape, layer_queryByShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, queryByFeatures, layer_queryByFeatures_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, queryByAttributes, layer_queryByAttributes_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, queryByFilter, layer_queryByFilter_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, queryByIndex, layer_queryByIndex_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setFilter, layer_setFilter_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getFilterString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setProjection, layer_setProjection_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getProjection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setWKTProjection, layer_setWKTProjection_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getNumResults, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getResultsBounds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getResult, layer_getResult_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, open, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, whichShapes, layer_whichShapes_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, nextShape, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, close, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getExtent, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setExtent, layer_setExtent_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, addFeature, layer_addFeature_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getMetaData, layer_getMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setMetaData, layer_setMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, removeMetaData, layer_removeMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getWMSFeatureInfoURL, layer_getWMSFeatureInfoURL_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getItems, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setProcessing, layer_setProcessing_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setProcessingKey, layer_setProcessingKey_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getProcessing, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, clearProcessing, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, executeWFSGetFeature, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, applySLD, layer_applySLD_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, applySLDURL, layer_applySLDURL_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, generateSLD, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, moveClassUp, layer_moveClassUp_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, moveClassDown, layer_moveClassDown_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, removeClass, layer_removeClass_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, isVisible, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setConnectionType, layer_setConnectionType_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getGridIntersectionCoordinates, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getShape, layer_getShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, getGeomTransform, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(layerObj, setGeomTransform, layer_setGeomTransform_args, ZEND_ACC_PUBLIC)  
  PHP_ME(layerObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_layer(layerObj *layer, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_layer_object * php_layer;
  object_init_ex(return_value, mapscript_ce_layer);
  php_layer = (php_layer_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_layer->layer = layer;

  if (layer->connectiontype != MS_GRATICULE || layer->layerinfo == NULL) {
    MAKE_STD_ZVAL(php_layer->grid);
    ZVAL_NULL(php_layer->grid);
  }

  if (parent.val)
    php_layer->is_ref = 1;

  php_layer->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_layer_object_destroy(void *object TSRMLS_DC)
{
  php_layer_object *php_layer = (php_layer_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_layer);

  MAPSCRIPT_FREE_PARENT(php_layer->parent);
  MAPSCRIPT_DELREF(php_layer->offsite);
  if (php_layer->grid && Z_TYPE_P(php_layer->grid) == IS_OBJECT)
    MAPSCRIPT_DELREF(php_layer->grid);
  MAPSCRIPT_DELREF(php_layer->metadata);
  MAPSCRIPT_DELREF(php_layer->bindvals);
  MAPSCRIPT_DELREF(php_layer->cluster);
  MAPSCRIPT_DELREF(php_layer->projection);
  MAPSCRIPT_DELREF(php_layer->extent);

  if (php_layer->layer && !php_layer->is_ref) {
    layerObj_destroy(php_layer->layer);
  }
  efree(object);
}

static zend_object_value mapscript_layer_object_new_ex(zend_class_entry *ce, php_layer_object **ptr TSRMLS_DC)
{
  zend_object_value retval;
  php_layer_object *php_layer;

  MAPSCRIPT_ALLOC_OBJECT(php_layer, php_layer_object);

  retval = mapscript_object_new_ex(&php_layer->std, ce,
                                   &mapscript_layer_object_destroy,
                                   &mapscript_layer_object_handlers TSRMLS_CC);

  if (ptr)
    *ptr = php_layer;

  php_layer->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_layer->parent);
  php_layer->offsite = NULL;
  php_layer->grid = NULL;
  php_layer->metadata = NULL;
  php_layer->bindvals = NULL;
  php_layer->cluster = NULL;
  php_layer->projection = NULL;
  php_layer->extent = NULL;

  return retval;
}

static zend_object_value mapscript_layer_object_new(zend_class_entry *ce TSRMLS_DC)
{
  return mapscript_layer_object_new_ex(ce, NULL TSRMLS_CC);
}

static zend_object_value mapscript_layer_object_clone(zval *zobj TSRMLS_DC)
{
  php_layer_object *php_layer_old, *php_layer_new;
  zend_object_value new_ov;

  php_layer_old = (php_layer_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  new_ov = mapscript_layer_object_new_ex(mapscript_ce_layer, &php_layer_new TSRMLS_CC);
  zend_objects_clone_members(&php_layer_new->std, new_ov, &php_layer_old->std, Z_OBJ_HANDLE_P(zobj) TSRMLS_CC);

  php_layer_new->layer = layerObj_clone(php_layer_old->layer);

  return new_ov;
}

PHP_MINIT_FUNCTION(layer)
{
  zend_class_entry ce;

  memcpy(&mapscript_layer_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mapscript_layer_object_handlers.clone_obj = mapscript_layer_object_clone;

  MAPSCRIPT_REGISTER_CLASS("layerObj",
                           layer_functions,
                           mapscript_ce_layer,
                           mapscript_layer_object_new);

  mapscript_ce_layer->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}

