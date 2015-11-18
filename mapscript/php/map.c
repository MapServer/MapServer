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

zend_class_entry *mapscript_ce_map;
zend_object_handlers mapscript_map_object_handlers;

static int mapscript_map_setProjection(int isWKTProj, php_map_object *php_map,
                                       char *projString, int setUnitsAndExtents TSRMLS_DC);

ZEND_BEGIN_ARG_INFO_EX(map___construct_args, 0, 0, 1)
ZEND_ARG_INFO(0, mapFileName)
ZEND_ARG_INFO(0, newMapPath)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getSymbolByName_args, 0, 0, 1)
ZEND_ARG_INFO(0, symbolName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getSymbolObjectById_args, 0, 0, 1)
ZEND_ARG_INFO(0, symbolId)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_embedLegend_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_embedScaleBar_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_drawLabelCache_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getLayer_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getLayerByName_args, 0, 0, 1)
ZEND_ARG_INFO(0, layerName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getColorByIndex_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setExtent_args, 0, 0, 4)
ZEND_ARG_INFO(0, minx)
ZEND_ARG_INFO(0, miny)
ZEND_ARG_INFO(0, maxx)
ZEND_ARG_INFO(0, maxy)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setCenter_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_offsetExtent_args, 0, 0, 2)
ZEND_ARG_INFO(0, x)
ZEND_ARG_INFO(0, y)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_scaleExtent_args, 0, 0, 3)
ZEND_ARG_INFO(0, zoomFactor)
ZEND_ARG_INFO(0, minScaleDenom)
ZEND_ARG_INFO(0, maxScaleDenom)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setRotation_args, 0, 0, 1)
ZEND_ARG_INFO(0, angle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setSize_args, 0, 0, 2)
ZEND_ARG_INFO(0, width)
ZEND_ARG_INFO(0, height)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_zoomPoint_args, 0, 0, 5)
ZEND_ARG_INFO(0, zoomFactor)
ZEND_ARG_OBJ_INFO(0, pixelPosition, pointObj, 0)
ZEND_ARG_INFO(0, width)
ZEND_ARG_INFO(0, height)
ZEND_ARG_OBJ_INFO(0, geoRefExtent, rectObj, 0)
ZEND_ARG_OBJ_INFO(0, maxGeoRefExtent, rectObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_zoomRectangle_args, 0, 0, 4)
ZEND_ARG_OBJ_INFO(0, pixelExtent, rectObj, 0)
ZEND_ARG_INFO(0, width)
ZEND_ARG_INFO(0, height)
ZEND_ARG_OBJ_INFO(0, geoRefExtent, rectObj, 0)
ZEND_ARG_OBJ_INFO(0, maxGeoRefExtent, rectObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_zoomScale_args, 0, 0, 5)
ZEND_ARG_INFO(0, scaleDenom)
ZEND_ARG_OBJ_INFO(0, pixelPosition, pointObj, 0)
ZEND_ARG_INFO(0, width)
ZEND_ARG_INFO(0, height)
ZEND_ARG_OBJ_INFO(0, geoRefExtent, rectObj, 0)
ZEND_ARG_OBJ_INFO(0, maxGeoRefExtent, rectObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_queryByPoint_args, 0, 0, 3)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_ARG_INFO(0, mode)
ZEND_ARG_INFO(0, buffer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_queryByRect_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, rect, rectObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_queryByShape_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_queryByFeatures_args, 0, 0, 1)
ZEND_ARG_INFO(0, slayer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_queryByIndex_args, 0, 0, 3)
ZEND_ARG_INFO(0, layerIndex)
ZEND_ARG_INFO(0, tileIndex)
ZEND_ARG_INFO(0, shapeIndex)
ZEND_ARG_INFO(0, addToQuery)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_queryByFilter_args, 0, 0, 1)
ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_saveQuery_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_INFO(0, results)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_saveQueryAsGML_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_INFO(0, namespace)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_loadQuery_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_freeQuery_args, 0, 0, 1)
ZEND_ARG_INFO(0, layerIndex)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_save_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setProjection_args, 0, 0, 1)
ZEND_ARG_INFO(0, projectionParams)
ZEND_ARG_INFO(0, setUnitsAndExtents)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setWKTProjection_args, 0, 0, 1)
ZEND_ARG_INFO(0, projectionParams)
ZEND_ARG_INFO(0, setUnitsAndExtents)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getMetaData_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setMetaData_args, 0, 0, 2)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_removeMetaData_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getLayersIndexByGroup_args, 0, 0, 1)
ZEND_ARG_INFO(0, groupName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_moveLayerUp_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_moveLayerDown_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setLayersDrawingOrder_args, 0, 0, 1)
ZEND_ARG_ARRAY_INFO(0, layerIndexes, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_processTemplate_args, 0, 0, 2)
ZEND_ARG_ARRAY_INFO(0, params, 0)
ZEND_ARG_INFO(0, generateImages)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_processQueryTemplate_args, 0, 0, 1)
ZEND_ARG_ARRAY_INFO(0, params, 0)
ZEND_ARG_INFO(0, generateImages)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_processLegendTemplate_args, 0, 0, 1)
ZEND_ARG_ARRAY_INFO(0, params, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setSymbolSet_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setFontSet_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_selectOutputFormat_args, 0, 0, 1)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_appendOutputFormat_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, outputformat, outputFormatObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_removeOutputFormat_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getOutputFormat_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_saveMapContext_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_loadMapContext_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_INFO(0, uniqueLayerName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_applySLD_args, 0, 0, 1)
ZEND_ARG_INFO(0, sldxml)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_applySLDURL_args, 0, 0, 1)
ZEND_ARG_INFO(0, sldurl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getConfigOption_args, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_setConfigOption_args, 0, 0, 2)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_loadOwsParameters_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, request, owsRequestObj, 0)
ZEND_ARG_INFO(0, version)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_owsDispatch_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, request, owsRequestObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_insertLayer_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_removeLayer_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(map_getLabel_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct(string mapFileName, newMapPath)
   Create a new mapObj instance. */
PHP_METHOD(mapObj, __construct)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  char *path = NULL;
  long path_len = 0;
  mapObj *map;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                            &filename, &filename_len,
                            &path, &path_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *)zend_object_store_get_object(zobj TSRMLS_CC);

  map = mapObj_new(filename, path);

  if (map == NULL) {
    mapscript_throw_mapserver_exception("Failed to open map file %s" TSRMLS_CC,  filename);
    return;
  }

  php_map->map = map;
}
/* }}} */

PHP_METHOD(mapObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("numlayers",  php_map->map->numlayers)
  else IF_GET_STRING("name", php_map->map->name)
    else IF_GET_STRING("mappath", php_map->map->mappath)
      else IF_GET_STRING("imagetype", php_map->map->imagetype)
        else IF_GET_LONG("status",  php_map->map->status)
          else IF_GET_LONG("debug", php_map->map->debug)
            else IF_GET_LONG("width", php_map->map->width)
              else IF_GET_LONG("height", php_map->map->height)
                else IF_GET_LONG("maxsize", php_map->map->maxsize)
                  else IF_GET_DOUBLE("resolution", php_map->map->resolution)
                    else IF_GET_DOUBLE("defresolution", php_map->map->defresolution)
                      else IF_GET_DOUBLE("cellsize", php_map->map->cellsize)
                        else IF_GET_LONG("units", php_map->map->units)
                          else IF_GET_DOUBLE("scaledenom", php_map->map->scaledenom)
                            else IF_GET_STRING("shapepath", php_map->map->shapepath)
                              else IF_GET_LONG("keysizex", php_map->map->legend.keysizex)
                                else IF_GET_LONG("keysizey", php_map->map->legend.keysizey)
                                  else IF_GET_LONG("keyspacingx", php_map->map->legend.keyspacingx)
                                    else IF_GET_LONG("keyspacingy", php_map->map->legend.keyspacingy)
                                      else IF_GET_STRING("symbolsetfilename", php_map->map->symbolset.filename)
                                        else IF_GET_LONG("numoutputformats", php_map->map->numoutputformats)
                                          else IF_GET_STRING("fontsetfilename", php_map->map->fontset.filename)
                                            else IF_GET_OBJECT("outputformat", mapscript_ce_outputformat, php_map->outputformat, php_map->map->outputformat)
                                              else IF_GET_OBJECT("configoptions", mapscript_ce_hashtable, php_map->configoptions, &php_map->map->configoptions)
                                                else IF_GET_OBJECT("extent", mapscript_ce_rect, php_map->extent, &php_map->map->extent)
                                                  else IF_GET_OBJECT("web", mapscript_ce_web, php_map->web, &php_map->map->web)
                                                    else IF_GET_OBJECT("reference", mapscript_ce_referencemap, php_map->reference, &php_map->map->reference)
                                                      else IF_GET_OBJECT("imagecolor", mapscript_ce_color, php_map->imagecolor, &php_map->map->imagecolor)
                                                        else IF_GET_OBJECT("scalebar", mapscript_ce_scalebar, php_map->scalebar, &php_map->map->scalebar)
                                                          else IF_GET_OBJECT("legend", mapscript_ce_legend, php_map->legend, &php_map->map->legend)
                                                            else IF_GET_OBJECT("querymap", mapscript_ce_querymap, php_map->querymap, &php_map->map->querymap)
#ifdef disabled
                                                              else IF_GET_OBJECT("labelcache", mapscript_ce_labelcache, php_map->labelcache, &php_map->map->labelcache)
#endif
                                                                else IF_GET_OBJECT("projection", mapscript_ce_projection, php_map->projection, &php_map->map->projection)
                                                                  else IF_GET_OBJECT("metadata", mapscript_ce_hashtable, php_map->metadata, &php_map->map->web.metadata)
                                                                    else {
                                                                      mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                                                    }
}

PHP_METHOD(mapObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_STRING("name", php_map->map->name, value)
  else IF_SET_LONG("status",  php_map->map->status, value)
    else IF_SET_LONG("debug", php_map->map->debug, value)
      else IF_SET_LONG("width", php_map->map->width, value)
        else IF_SET_LONG("height", php_map->map->height, value)
          else IF_SET_LONG("maxsize", php_map->map->maxsize, value)
            else IF_SET_DOUBLE("resolution", php_map->map->resolution, value)
              else IF_SET_DOUBLE("defresolution", php_map->map->defresolution, value)
                else IF_SET_DOUBLE("cellsize", php_map->map->cellsize, value)
                  else IF_SET_LONG("units", php_map->map->units, value)
                    else IF_SET_DOUBLE("scaledenom", php_map->map->scaledenom, value)
                      else IF_SET_STRING("shapepath", php_map->map->shapepath, value)
                        else IF_SET_LONG("keysizex", php_map->map->legend.keysizex, value)
                          else IF_SET_LONG("keysizey", php_map->map->legend.keysizey, value)
                            else IF_SET_LONG("keyspacingx", php_map->map->legend.keyspacingx, value)
                              else IF_SET_LONG("keyspacingy", php_map->map->legend.keyspacingy, value)
                                else if ( (STRING_EQUAL("outputformat", property)) ||
                                          (STRING_EQUAL("extent", property)) ||
                                          (STRING_EQUAL("web", property)) ||
                                          (STRING_EQUAL("reference", property)) ||
                                          (STRING_EQUAL("scalebar", property)) ||
                                          (STRING_EQUAL("legend", property)) ||
                                          (STRING_EQUAL("querymap", property)) ||
#ifdef disabled
                                          (STRING_EQUAL("labelcache", property)) ||
#endif
                                          (STRING_EQUAL("projection", property)) ||
                                          (STRING_EQUAL("metadata", property)) ||
                                          (STRING_EQUAL("configoptions", property)) ||
                                          (STRING_EQUAL("imagecolor", property)) ) {
                                  mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                                } else if ( (STRING_EQUAL("numlayers", property)) ||
                                            (STRING_EQUAL("symbolsetfilename", property)) ||
                                            (STRING_EQUAL("imagetype", property)) ||
                                            (STRING_EQUAL("numoutputformats", property)) ||
                                            (STRING_EQUAL("mappath", property)) ||
                                            (STRING_EQUAL("fontsetfilename", property)) ) {
                                  mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
                                } else {
                                  mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                }
}

/* {{{ proto int map.getSymbolByName(string symbolName)
   Returns the symbol index using the name. */
PHP_METHOD(mapObj, getSymbolByName)
{
  zval *zobj = getThis();
  char *symbolName;
  long symbolName_len = 0;
  int symbolId = -1;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &symbolName, &symbolName_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  symbolId = mapObj_getSymbolByName(php_map->map, symbolName);

  RETURN_LONG(symbolId);
}
/* }}} */

/* {{{ proto symbolObj map.getSymbolObjectById(int symbolId)
   Returns the symbol object using a symbol id. Refer to the symbol object
   reference section for more details. */
PHP_METHOD(mapObj, getSymbolObjectById)
{
  zval *zobj = getThis();
  long symbolId;
  symbolObj *symbol = NULL;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &symbolId) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ( symbolId < 0 || symbolId >= php_map->map->symbolset.numsymbols) {
    mapscript_throw_exception("Invalid symbol index." TSRMLS_CC);
    return;
  }

  symbol = php_map->map->symbolset.symbol[symbolId];

  /* Return style object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_symbol(symbol, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto void map.prepareQuery()
   Calculate the scale of the map and assign it to the map->scaledenom */
PHP_METHOD(mapObj, prepareQuery)
{
  zval *zobj = getThis();
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  mapObj_prepareQuery(php_map->map);
}
/* }}} */

/* {{{ proto imageObj map.prepareImage()
   Return handle on blank image object. */
PHP_METHOD(mapObj, prepareImage)
{
  zval *zobj = getThis();
  imageObj *image = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  image = mapObj_prepareImage(php_map->map);
  if (image == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  /* Return an image object */
  mapscript_create_image(image, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto imageObj map.draw()
   Render map and return handle on image object. */
PHP_METHOD(mapObj, draw)
{
  zval *zobj = getThis();
  imageObj *image = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  image = mapObj_draw(php_map->map);
  if (image == NULL) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_NULL();
  }

  mapscript_create_image(image, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int map.drawQuery()
   Renders a query map, returns an image.. */
PHP_METHOD(mapObj, drawQuery)
{
  zval *zobj = getThis();
  imageObj *image = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  image = mapObj_drawQuery(php_map->map);
  if (image == NULL) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_NULL();
  }

  mapscript_create_image(image, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int map.drawLegend()
   Render legend and return handle on image object. */
PHP_METHOD(mapObj, drawLegend)
{
  zval *zobj = getThis();
  imageObj *image = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  image = mapObj_drawLegend(php_map->map);
  if (image == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  mapscript_create_image(image, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int map.drawReferenceMap()
   Render reference map and return handle on image object. */
PHP_METHOD(mapObj, drawReferenceMap)
{
  zval *zobj = getThis();
  imageObj *image = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  image = mapObj_drawReferenceMap(php_map->map);
  if (image == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  mapscript_create_image(image, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int map.drawScaleBar()
   Render scale bar and return handle on image object. */
PHP_METHOD(mapObj, drawScaleBar)
{
  zval *zobj = getThis();
  imageObj *image = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  image = mapObj_drawScalebar(php_map->map);
  if (image == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  mapscript_create_image(image, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int map.embedLegend(image img)
   Renders the legend within the map. Returns MS_FAILURE on error. */
PHP_METHOD(mapObj, embedLegend)
{
  zval *zobj = getThis();
  zval *zimage;
  int retval = MS_FAILURE;
  php_map_object *php_map;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zimage, mapscript_ce_image) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_image = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);

  retval = mapObj_embedLegend(php_map->map, php_image->image);
  if ( (retval == MS_FAILURE) || (retval == -1) ) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int map.embedScalebar(image img)
   Renders the scalebar within the map. Returns MS_FAILURE on error. */
PHP_METHOD(mapObj, embedScaleBar)
{
  zval *zobj = getThis();
  zval *zimage;
  int retval = MS_FAILURE;
  php_map_object *php_map;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zimage, mapscript_ce_image) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_image = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);

  retval = mapObj_embedScalebar(php_map->map, php_image->image);
  if ( (retval == MS_FAILURE) || (retval == -1) ) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto layerObj map.drawLabelCache(image img)
   Renders the labels for a map. Returns MS_FAILURE on error. */
PHP_METHOD(mapObj, drawLabelCache)
{
  zval *zobj = getThis();
  zval *zimage;
  int retval = MS_FAILURE;
  php_map_object *php_map;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zimage, mapscript_ce_image) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_image = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);

  retval = mapObj_drawLabelCache(php_map->map, php_image->image);
  if ( (retval == MS_FAILURE) || (retval == -1) ) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int map.getLayer(int index)
   Returns a layerObj from the map given an index value (0=first layer, etc.) */
PHP_METHOD(mapObj, getLayer)
{
  zval *zobj = getThis();
  long index;
  layerObj *layer = NULL;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  layer = mapObj_getLayer(php_map->map, index);
  if  (layer == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  /* Return layer object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_layer(layer, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto layerObj map.getLayerByName(string layerName)
   Returns a layerObj from the map given a layer name */
PHP_METHOD(mapObj, getLayerByName)
{
  zval *zobj = getThis();
  char *layerName;
  long layerName_len = 0;
  layerObj *layer = NULL;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &layerName, &layerName_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  layer = mapObj_getLayerByName(php_map->map, layerName);
  if  (layer == NULL) {
    mapscript_report_php_error(E_WARNING, "getLayerByName failed for : %s\n" TSRMLS_CC, layerName);
    RETURN_NULL();
  }

  /* Return layer object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_layer(layer, parent, return_value TSRMLS_CC);
}
/* }}} */


/* {{{ proto colorObj map.getColorByIndex(int index)
   Returns a colorObj from the map given a color index */
PHP_METHOD(mapObj, getColorByIndex)
{
  zval *zobj = getThis();
  long index;
  paletteObj palette;
  colorObj color;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  palette = php_map->map->palette;

  if (index < palette.numcolors) {
    color.red = palette.colors[index].red;
    color.green = palette.colors[index].green;
    color.blue = palette.colors[index].blue;
  } else {
    mapscript_throw_mapserver_exception("Index shoud not be higher than %d\n" TSRMLS_CC, palette.numcolors-1);
    return;
  }

  /* Return color object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_color(&color, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int map.setextent(double minx, double miny, double maxx, double maxy)
   Set map extent property to new value. Returns MS_FAILURE on error. */
PHP_METHOD(mapObj, setExtent)
{
  zval *zobj = getThis();
  double minx, miny, maxx, maxy;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "dddd",
                            &minx, &miny, &maxx, &maxy) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = msMapSetExtent( php_map->map,
                           minx, miny,
                           maxx, maxy);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.setCenter(pointObj center)
   Returns MS_SUCCESS or MS_FAILURE */
PHP_METHOD(mapObj, setCenter)
{
  zval *zobj = getThis();
  zval *zpoint;
  int status = MS_FAILURE;
  php_map_object *php_map;
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zpoint, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);

  status = mapObj_setCenter(php_map->map, php_point->point);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.offsetExtent(double x, double y)
   Returns MS_SUCCESS or MS_FAILURE */
PHP_METHOD(mapObj, offsetExtent)
{
  zval *zobj = getThis();
  double x, y;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "dd",
                            &x, &y) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_offsetExtent(php_map->map, x, y);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.scaleExtent(double zoomfactor, double minscaledenom,
                                 double maxscaledenom)
   Returns MS_SUCCESS or MS_FAILURE */
PHP_METHOD(mapObj, scaleExtent)
{
  zval *zobj = getThis();
  double zoomFactor, minScaleDenom, maxScaleDenom;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ddd",
                            &zoomFactor, &minScaleDenom, &maxScaleDenom) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_scaleExtent(php_map->map, zoomFactor, minScaleDenom, maxScaleDenom);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.setRotation(double angle)
   Set map rotation angle. The map view rectangle (specified in EXTENTS) will
   be rotated by the indicated angle in the counter-clockwise direction. Note
   that this implies the rendered map will be rotated by the angle in the
   clockwise direction. */
PHP_METHOD(mapObj, setRotation)
{
  zval *zobj = getThis();
  double angle;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d",
                            &angle) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_setRotation(php_map->map, angle);
  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.setSize(int width, int height)
   Set map size (width, height) properties to new values and upddate internal geotransform.
   Returns MS_SUCCESS/MS_FAILURE. */
PHP_METHOD(mapObj, setSize)
{
  zval *zobj = getThis();
  long width, height;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",
                            &width, &height) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = msMapSetSize(php_map->map, width, height);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int zoompoint(int zoomFactor, pointObj pixelPosition, int width, int height,
                           rectObj geoRefExtent, [rectObj mapGeoRefExtent])
   Zoom to a given XY postion. */
PHP_METHOD(mapObj, zoomPoint)
{
  zval *zobj = getThis();
  zval *zpoint, *zgeoRefExtent, *zmaxGeoRefExtent = NULL;
  long zoomFactor, width, height;
  double      dfGeoPosX, dfGeoPosY;
  double      dfDeltaX, dfDeltaY;
  rectObj     newGeoRefExtent;
  double      dfNewScale = 0.0;
  double      dfDeltaExt = -1.0;
  php_point_object *php_pixelPosition;
  php_rect_object *php_geoRefExtent=NULL, *php_maxGeoRefExtent=NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lOllO|O",
                            &zoomFactor,
                            &zpoint, mapscript_ce_point,
                            &width, &height,
                            &zgeoRefExtent , mapscript_ce_rect,
                            &zmaxGeoRefExtent , mapscript_ce_rect) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_pixelPosition = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);
  php_geoRefExtent = (php_rect_object *) zend_object_store_get_object(zgeoRefExtent TSRMLS_CC);
  if (zmaxGeoRefExtent)
    php_maxGeoRefExtent = (php_rect_object *) zend_object_store_get_object(zmaxGeoRefExtent TSRMLS_CC);

  /* -------------------------------------------------------------------- */
  /*      check the validity of the parameters.                           */
  /* -------------------------------------------------------------------- */
  if (zoomFactor == 0 ||
      width <= 0 ||
      height <= 0 ||
      php_geoRefExtent->rect == NULL ||
      php_pixelPosition->point == NULL ||
      (zmaxGeoRefExtent && php_maxGeoRefExtent->rect == NULL)) {
    mapscript_throw_mapserver_exception("Incorrect parameters\n" TSRMLS_CC);
    return;
  }

  /* -------------------------------------------------------------------- */
  /*      check if the values passed are consistent min > max.             */
  /* -------------------------------------------------------------------- */
  if (php_geoRefExtent->rect->minx >= php_geoRefExtent->rect->maxx) {
    mapscript_throw_mapserver_exception("Georeferenced coordinates minx >= maxx\n" TSRMLS_CC);
    return;
  }
  if (php_geoRefExtent->rect->miny >= php_geoRefExtent->rect->maxy) {
    mapscript_throw_mapserver_exception("Georeferenced coordinates miny >= maxy\n" TSRMLS_CC);
    return;
  }

  if (zmaxGeoRefExtent) {
    if (php_maxGeoRefExtent->rect->minx >= php_maxGeoRefExtent->rect->maxx) {
      mapscript_throw_mapserver_exception("Max Georeferenced coordinates minx >= maxx\n" TSRMLS_CC);
      return;
    }
    if (php_maxGeoRefExtent->rect->miny >= php_maxGeoRefExtent->rect->maxy) {
      mapscript_throw_mapserver_exception("Max Georeferenced coordinates miny >= maxy\n" TSRMLS_CC);
      return;
    }
  }


  dfGeoPosX = Pix2Georef((int)php_pixelPosition->point->x, 0, width,
                         php_geoRefExtent->rect->minx, php_geoRefExtent->rect->maxx, 0);
  dfGeoPosY = Pix2Georef((int)php_pixelPosition->point->y, 0, height,
                         php_geoRefExtent->rect->miny, php_geoRefExtent->rect->maxy, 1);
  dfDeltaX = php_geoRefExtent->rect->maxx - php_geoRefExtent->rect->minx;
  dfDeltaY = php_geoRefExtent->rect->maxy - php_geoRefExtent->rect->miny;


  /* -------------------------------------------------------------------- */
  /*      zoom in                                                         */
  /* -------------------------------------------------------------------- */
  if (zoomFactor > 1) {
    newGeoRefExtent.minx =
      dfGeoPosX - (dfDeltaX/(2*zoomFactor));
    newGeoRefExtent.miny =
      dfGeoPosY - (dfDeltaY/(2*zoomFactor));
    newGeoRefExtent.maxx =
      dfGeoPosX + (dfDeltaX/(2*zoomFactor));
    newGeoRefExtent.maxy =
      dfGeoPosY + (dfDeltaY/(2*zoomFactor));
  }

  if (zoomFactor < 0) {
    newGeoRefExtent.minx =
      dfGeoPosX - (dfDeltaX/2)*(abs(zoomFactor));
    newGeoRefExtent.miny =
      dfGeoPosY - (dfDeltaY/2)*(abs(zoomFactor));
    newGeoRefExtent.maxx =
      dfGeoPosX + (dfDeltaX/2)*(abs(zoomFactor));
    newGeoRefExtent.maxy =
      dfGeoPosY + (dfDeltaY/2)*(abs(zoomFactor));
  }
  if (zoomFactor == 1) {
    newGeoRefExtent.minx = dfGeoPosX - (dfDeltaX/2);
    newGeoRefExtent.miny = dfGeoPosY - (dfDeltaY/2);
    newGeoRefExtent.maxx = dfGeoPosX + (dfDeltaX/2);
    newGeoRefExtent.maxy = dfGeoPosY + (dfDeltaY/2);
  }

  /* -------------------------------------------------------------------- */
  /*      if the min and max scale are set in the map file, we will       */
  /*      use them to test before zooming.                                */
  /* -------------------------------------------------------------------- */
  msAdjustExtent(&newGeoRefExtent, php_map->map->width, php_map->map->height);
  if (msCalculateScale(newGeoRefExtent, php_map->map->units,
                       php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &dfNewScale) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  if (php_map->map->web.maxscaledenom > 0) {
    if (zoomFactor < 0 && dfNewScale >  php_map->map->web.maxscaledenom) {
      RETURN_LONG(MS_FAILURE);
    }
  }

  /* ==================================================================== */
  /*      we do a spcial case for zoom in : we try to zoom as much as     */
  /*      possible using the mincale set in the .map.                     */
  /* ==================================================================== */
  if (php_map->map->web.minscaledenom > 0 && dfNewScale <  php_map->map->web.minscaledenom &&
      zoomFactor > 1) {
    dfDeltaExt =
      GetDeltaExtentsUsingScale(php_map->map->web.minscaledenom, php_map->map->units,
                                dfGeoPosY, php_map->map->width,
                                php_map->map->resolution);
    if (dfDeltaExt > 0.0) {
      newGeoRefExtent.minx = dfGeoPosX - (dfDeltaExt/2);
      newGeoRefExtent.miny = dfGeoPosY - (dfDeltaExt/2);
      newGeoRefExtent.maxx = dfGeoPosX + (dfDeltaExt/2);
      newGeoRefExtent.maxy = dfGeoPosY + (dfDeltaExt/2);
    } else
      RETURN_LONG(MS_FAILURE);
  }
  /* -------------------------------------------------------------------- */
  /*      If the buffer is set, make sure that the extents do not go      */
  /*      beyond the buffer.                                              */
  /* -------------------------------------------------------------------- */
  if (zmaxGeoRefExtent) {
    dfDeltaX = newGeoRefExtent.maxx - newGeoRefExtent.minx;
    dfDeltaY = newGeoRefExtent.maxy - newGeoRefExtent.miny;

    /* Make sure Current georef extents is not bigger than max extents */
    if (dfDeltaX > (php_maxGeoRefExtent->rect->maxx-php_maxGeoRefExtent->rect->minx))
      dfDeltaX = php_maxGeoRefExtent->rect->maxx-php_maxGeoRefExtent->rect->minx;
    if (dfDeltaY > (php_maxGeoRefExtent->rect->maxy-php_maxGeoRefExtent->rect->miny))
      dfDeltaY = php_maxGeoRefExtent->rect->maxy-php_maxGeoRefExtent->rect->miny;

    if (newGeoRefExtent.minx < php_maxGeoRefExtent->rect->minx) {
      newGeoRefExtent.minx = php_maxGeoRefExtent->rect->minx;
      newGeoRefExtent.maxx =  newGeoRefExtent.minx + dfDeltaX;
    }
    if (newGeoRefExtent.maxx > php_maxGeoRefExtent->rect->maxx) {
      newGeoRefExtent.maxx = php_maxGeoRefExtent->rect->maxx;
      newGeoRefExtent.minx = newGeoRefExtent.maxx - dfDeltaX;
    }
    if (newGeoRefExtent.miny < php_maxGeoRefExtent->rect->miny) {
      newGeoRefExtent.miny = php_maxGeoRefExtent->rect->miny;
      newGeoRefExtent.maxy =  newGeoRefExtent.miny + dfDeltaY;
    }
    if (newGeoRefExtent.maxy > php_maxGeoRefExtent->rect->maxy) {
      newGeoRefExtent.maxy = php_maxGeoRefExtent->rect->maxy;
      newGeoRefExtent.miny = newGeoRefExtent.maxy - dfDeltaY;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      set the map extents with new values.                            */
  /* -------------------------------------------------------------------- */
  php_map->map->extent.minx = newGeoRefExtent.minx;
  php_map->map->extent.miny = newGeoRefExtent.miny;
  php_map->map->extent.maxx = newGeoRefExtent.maxx;
  php_map->map->extent.maxy = newGeoRefExtent.maxy;

  php_map->map->cellsize = msAdjustExtent(&(php_map->map->extent), php_map->map->width,
                                          php_map->map->height);
  dfDeltaX = php_map->map->extent.maxx - php_map->map->extent.minx;
  dfDeltaY = php_map->map->extent.maxy - php_map->map->extent.miny;

  if (zmaxGeoRefExtent) {
    if (php_map->map->extent.minx < php_maxGeoRefExtent->rect->minx) {
      php_map->map->extent.minx = php_maxGeoRefExtent->rect->minx;
      php_map->map->extent.maxx = php_map->map->extent.minx + dfDeltaX;
    }
    if (php_map->map->extent.maxx > php_maxGeoRefExtent->rect->maxx) {
      php_map->map->extent.maxx = php_maxGeoRefExtent->rect->maxx;
      newGeoRefExtent.minx = newGeoRefExtent.maxx - dfDeltaX;
    }
    if (php_map->map->extent.miny < php_maxGeoRefExtent->rect->miny) {
      php_map->map->extent.miny = php_maxGeoRefExtent->rect->miny;
      php_map->map->extent.maxy =  php_map->map->extent.miny + dfDeltaY;
    }
    if (php_map->map->extent.maxy > php_maxGeoRefExtent->rect->maxy) {
      php_map->map->extent.maxy = php_maxGeoRefExtent->rect->maxy;
      newGeoRefExtent.miny = newGeoRefExtent.maxy - dfDeltaY;
    }
  }

  if (msCalculateScale(php_map->map->extent, php_map->map->units, php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &(php_map->map->scaledenom)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int zoomRectangle(rectObj pixelExtent, int width, int height, rectObj geoRefExtent [, rectObj geoRefExtent])
   Set the map extents to a given extents.*/
PHP_METHOD(mapObj, zoomRectangle)
{
  zval *zobj = getThis();
  zval *zpixelExtent, *zgeoRefExtent, *zmaxGeoRefExtent = NULL;
  long width, height;
  double      dfDeltaX, dfDeltaY;
  rectObj     newGeoRefExtent;
  double      dfNewScale = 0.0;
  double      dfDeltaExt = -1.0;
  double      dfMiddleX =0.0;
  double      dfMiddleY =0.0;
  php_rect_object *php_geoRefExtent=NULL, *php_maxGeoRefExtent=NULL, *php_pixelExtent=NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "OllO|O",
                            &zpixelExtent , mapscript_ce_rect,
                            &width, &height,
                            &zgeoRefExtent , mapscript_ce_rect,
                            &zmaxGeoRefExtent , mapscript_ce_rect) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_pixelExtent = (php_rect_object *) zend_object_store_get_object(zpixelExtent TSRMLS_CC);
  php_geoRefExtent = (php_rect_object *) zend_object_store_get_object(zgeoRefExtent TSRMLS_CC);
  if (zmaxGeoRefExtent)
    php_maxGeoRefExtent = (php_rect_object *) zend_object_store_get_object(zmaxGeoRefExtent TSRMLS_CC);


  /* -------------------------------------------------------------------- */
  /*      check the validity of the parameters.                           */
  /* -------------------------------------------------------------------- */
  if (width <= 0 ||
      height <= 0 ||
      php_geoRefExtent->rect == NULL ||
      php_pixelExtent->rect == NULL ||
      (zmaxGeoRefExtent && php_maxGeoRefExtent->rect == NULL)) {
    mapscript_throw_mapserver_exception("Incorrect parameters\n" TSRMLS_CC);
    return;
  }

  /* -------------------------------------------------------------------- */
  /*      check if the values passed are consistent min > max.            */
  /* -------------------------------------------------------------------- */
  if (php_geoRefExtent->rect->minx >= php_geoRefExtent->rect->maxx) {
    mapscript_throw_mapserver_exception("Georeferenced coordinates minx >= maxx\n" TSRMLS_CC);
    return;
  }
  if (php_geoRefExtent->rect->miny >= php_geoRefExtent->rect->maxy) {
    mapscript_throw_mapserver_exception("Georeferenced coordinates miny >= maxy\n" TSRMLS_CC);
    return;
  }
  if (zmaxGeoRefExtent) {
    if (php_maxGeoRefExtent->rect->minx >= php_maxGeoRefExtent->rect->maxx) {
      mapscript_throw_mapserver_exception("Max Georeferenced coordinates minx >= maxx\n" TSRMLS_CC);
      return;
    }
    if (php_maxGeoRefExtent->rect->miny >= php_maxGeoRefExtent->rect->maxy) {
      mapscript_throw_mapserver_exception("Max Georeferenced coordinates miny >= maxy\n" TSRMLS_CC);
      return;
    }
  }


  newGeoRefExtent.minx = Pix2Georef((int)php_pixelExtent->rect->minx, 0,
                                    width,
                                    php_geoRefExtent->rect->minx, php_geoRefExtent->rect->maxx, 0);
  newGeoRefExtent.maxx = Pix2Georef((int)php_pixelExtent->rect->maxx, 0,
                                    width,
                                    php_geoRefExtent->rect->minx, php_geoRefExtent->rect->maxx, 0);
  newGeoRefExtent.miny = Pix2Georef((int)php_pixelExtent->rect->miny, 0,
                                    height,
                                    php_geoRefExtent->rect->miny, php_geoRefExtent->rect->maxy, 1);
  newGeoRefExtent.maxy = Pix2Georef((int)php_pixelExtent->rect->maxy, 0,
                                    height,
                                    php_geoRefExtent->rect->miny, php_geoRefExtent->rect->maxy, 1);

  msAdjustExtent(&newGeoRefExtent, php_map->map->width, php_map->map->height);

  /* -------------------------------------------------------------------- */
  /*      if the min and max scale are set in the map file, we will       */
  /*      use them to test before setting extents.                        */
  /* -------------------------------------------------------------------- */
  if (msCalculateScale(newGeoRefExtent, php_map->map->units,
                       php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &dfNewScale) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  if (php_map->map->web.maxscaledenom > 0 &&  dfNewScale > php_map->map->web.maxscaledenom) {
    RETURN_LONG(MS_FAILURE);
  }

  if (php_map->map->web.minscaledenom > 0 && dfNewScale <  php_map->map->web.minscaledenom) {
    dfMiddleX = newGeoRefExtent.minx +
                ((newGeoRefExtent.maxx - newGeoRefExtent.minx)/2);
    dfMiddleY = newGeoRefExtent.miny +
                ((newGeoRefExtent.maxy - newGeoRefExtent.miny)/2);

    dfDeltaExt =
      GetDeltaExtentsUsingScale(php_map->map->web.minscaledenom, php_map->map->units,
                                dfMiddleY, php_map->map->width,
                                php_map->map->resolution);

    if (dfDeltaExt > 0.0) {
      newGeoRefExtent.minx = dfMiddleX - (dfDeltaExt/2);
      newGeoRefExtent.miny = dfMiddleY - (dfDeltaExt/2);
      newGeoRefExtent.maxx = dfMiddleX + (dfDeltaExt/2);
      newGeoRefExtent.maxy = dfMiddleY + (dfDeltaExt/2);
    } else
      RETURN_LONG(MS_FAILURE);
  }

  /* -------------------------------------------------------------------- */
  /*      If the buffer is set, make sure that the extents do not go      */
  /*      beyond the buffer.                                              */
  /* -------------------------------------------------------------------- */
  if (zmaxGeoRefExtent) {
    dfDeltaX = newGeoRefExtent.maxx - newGeoRefExtent.minx;
    dfDeltaY = newGeoRefExtent.maxy - newGeoRefExtent.miny;

    /* Make sure Current georef extents is not bigger than max extents */
    if (dfDeltaX > (php_maxGeoRefExtent->rect->maxx-php_maxGeoRefExtent->rect->minx))
      dfDeltaX = php_maxGeoRefExtent->rect->maxx-php_maxGeoRefExtent->rect->minx;
    if (dfDeltaY > (php_maxGeoRefExtent->rect->maxy-php_maxGeoRefExtent->rect->miny))
      dfDeltaY = php_maxGeoRefExtent->rect->maxy-php_maxGeoRefExtent->rect->miny;

    if (newGeoRefExtent.minx < php_maxGeoRefExtent->rect->minx) {
      newGeoRefExtent.minx = php_maxGeoRefExtent->rect->minx;
      newGeoRefExtent.maxx =  newGeoRefExtent.minx + dfDeltaX;
    }
    if (newGeoRefExtent.maxx > php_maxGeoRefExtent->rect->maxx) {
      newGeoRefExtent.maxx = php_maxGeoRefExtent->rect->maxx;
      newGeoRefExtent.minx = newGeoRefExtent.maxx - dfDeltaX;
    }
    if (newGeoRefExtent.miny < php_maxGeoRefExtent->rect->miny) {
      newGeoRefExtent.miny = php_maxGeoRefExtent->rect->miny;
      newGeoRefExtent.maxy =  newGeoRefExtent.miny + dfDeltaY;
    }
    if (newGeoRefExtent.maxy > php_maxGeoRefExtent->rect->maxy) {
      newGeoRefExtent.maxy = php_maxGeoRefExtent->rect->maxy;
      newGeoRefExtent.miny = newGeoRefExtent.maxy - dfDeltaY;
    }
  }

  php_map->map->extent.minx = newGeoRefExtent.minx;
  php_map->map->extent.miny = newGeoRefExtent.miny;
  php_map->map->extent.maxx = newGeoRefExtent.maxx;
  php_map->map->extent.maxy = newGeoRefExtent.maxy;

  php_map->map->cellsize = msAdjustExtent(&(php_map->map->extent), php_map->map->width,
                                          php_map->map->height);
  dfDeltaX = php_map->map->extent.maxx - php_map->map->extent.minx;
  dfDeltaY = php_map->map->extent.maxy - php_map->map->extent.miny;

  if (zmaxGeoRefExtent) {
    if (php_map->map->extent.minx < php_maxGeoRefExtent->rect->minx) {
      php_map->map->extent.minx = php_maxGeoRefExtent->rect->minx;
      php_map->map->extent.maxx = php_map->map->extent.minx + dfDeltaX;
    }
    if (php_map->map->extent.maxx > php_maxGeoRefExtent->rect->maxx) {
      php_map->map->extent.maxx = php_maxGeoRefExtent->rect->maxx;
      newGeoRefExtent.minx = newGeoRefExtent.maxx - dfDeltaX;
    }
    if (php_map->map->extent.miny < php_maxGeoRefExtent->rect->miny) {
      php_map->map->extent.miny = php_maxGeoRefExtent->rect->miny;
      php_map->map->extent.maxy =  php_map->map->extent.miny + dfDeltaY;
    }
    if (php_map->map->extent.maxy > php_maxGeoRefExtent->rect->maxy) {
      php_map->map->extent.maxy = php_maxGeoRefExtent->rect->maxy;
      newGeoRefExtent.miny = newGeoRefExtent.maxy - dfDeltaY;
    }
  }

  if (msCalculateScale(php_map->map->extent, php_map->map->units, php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &(php_map->map->scaledenom)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int zoomScale(int scaleDenom, pointObj pixelPosition, int width, int height,
                           rectObj geoRefExtent, [rectObj mapGeoRefExtent])
   Zoom to a given XY postion. */
PHP_METHOD(mapObj, zoomScale)
{
  zval *zobj = getThis();
  zval *zpoint, *zgeoRefExtent, *zmaxGeoRefExtent = NULL;
  long width, height;
  double scaleDenom;
  double      dfGeoPosX, dfGeoPosY;
  double      dfDeltaX, dfDeltaY;
  double      dfCurrentScale = 0.0;
  rectObj     newGeoRefExtent;
  double      dfNewScale = 0.0;
  double      dfDeltaExt = -1.0;
  int         tmp = 0;
  php_point_object *php_pixelPosition;
  php_rect_object *php_geoRefExtent=NULL, *php_maxGeoRefExtent=NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "dOllO|O",
                            &scaleDenom,
                            &zpoint, mapscript_ce_point,
                            &width, &height,
                            &zgeoRefExtent , mapscript_ce_rect,
                            &zmaxGeoRefExtent , mapscript_ce_rect) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_pixelPosition = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);
  php_geoRefExtent = (php_rect_object *) zend_object_store_get_object(zgeoRefExtent TSRMLS_CC);
  if (zmaxGeoRefExtent)
    php_maxGeoRefExtent = (php_rect_object *) zend_object_store_get_object(zmaxGeoRefExtent TSRMLS_CC);

  /* -------------------------------------------------------------------- */
  /*      check the validity of the parameters.                           */
  /* -------------------------------------------------------------------- */
  if (scaleDenom <= 0.0 ||
      width <= 0 ||
      height <= 0 ||
      php_geoRefExtent->rect == NULL ||
      php_pixelPosition->point == NULL ||
      (zmaxGeoRefExtent && php_maxGeoRefExtent->rect == NULL)) {
    mapscript_throw_mapserver_exception("Incorrect parameters\n" TSRMLS_CC);
    return;
  }

  /* -------------------------------------------------------------------- */
  /*      check if the values passed are consistent min > max.             */
  /* -------------------------------------------------------------------- */
  if (php_geoRefExtent->rect->minx >= php_geoRefExtent->rect->maxx) {
    mapscript_throw_mapserver_exception("Georeferenced coordinates minx >= maxx\n" TSRMLS_CC);
    return;
  }
  if (php_geoRefExtent->rect->miny >= php_geoRefExtent->rect->maxy) {
    mapscript_throw_mapserver_exception("Georeferenced coordinates miny >= maxy\n" TSRMLS_CC);
    return;
  }
  if (zmaxGeoRefExtent) {
    if (php_maxGeoRefExtent->rect->minx >= php_maxGeoRefExtent->rect->maxx) {
      mapscript_throw_mapserver_exception("Max Georeferenced coordinates minx >= maxx\n" TSRMLS_CC);
      return;
    }
    if (php_maxGeoRefExtent->rect->miny >= php_maxGeoRefExtent->rect->maxy) {
      mapscript_throw_mapserver_exception("Max Georeferenced coordinates miny >= maxy\n" TSRMLS_CC);
      return;
    }
  }


  dfGeoPosX = Pix2Georef((int)php_pixelPosition->point->x, 0, width,
                         php_geoRefExtent->rect->minx, php_geoRefExtent->rect->maxx, 0);
  dfGeoPosY = Pix2Georef((int)php_pixelPosition->point->y, 0, height,
                         php_geoRefExtent->rect->miny, php_geoRefExtent->rect->maxy, 1);
  dfDeltaX = php_geoRefExtent->rect->maxx - php_geoRefExtent->rect->minx;
  dfDeltaY = php_geoRefExtent->rect->maxy - php_geoRefExtent->rect->miny;


  /* -------------------------------------------------------------------- */
  /*      Calculate new extents based on the sacle.                       */
  /* -------------------------------------------------------------------- */

  /* ==================================================================== */
  /*      make sure to take the smallest size because this is the one     */
  /*      that will be used to ajust the scale.                           */
  /* ==================================================================== */

  if (php_map->map->width <  php_map->map->height)
    tmp = php_map->map->width;
  else
    tmp = php_map->map->height;

  dfDeltaExt =
    GetDeltaExtentsUsingScale(scaleDenom, php_map->map->units, dfGeoPosY,
                              tmp, php_map->map->resolution);

  if (dfDeltaExt > 0.0) {
    newGeoRefExtent.minx = dfGeoPosX - (dfDeltaExt/2);
    newGeoRefExtent.miny = dfGeoPosY - (dfDeltaExt/2);
    newGeoRefExtent.maxx = dfGeoPosX + (dfDeltaExt/2);
    newGeoRefExtent.maxy = dfGeoPosY + (dfDeltaExt/2);
  } else
    RETURN_LONG(MS_FAILURE);

  /* -------------------------------------------------------------------- */
  /*      get current scale.                                              */
  /* -------------------------------------------------------------------- */
  if (msCalculateScale(*php_geoRefExtent->rect, php_map->map->units,
                       php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &dfCurrentScale) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* -------------------------------------------------------------------- */
  /*      if the min and max scale are set in the map file, we will       */
  /*      use them to test before zooming.                                */
  /*                                                                      */
  /*       This function has the same effect as zoomin or zoom out. If    */
  /*      the current scale is > newscale we zoom in; else it is a        */
  /*      zoom out.                                                       */
  /* -------------------------------------------------------------------- */
  msAdjustExtent(&newGeoRefExtent, php_map->map->width, php_map->map->height);
  if (msCalculateScale(newGeoRefExtent, php_map->map->units,
                       php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &dfNewScale) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  if (php_map->map->web.maxscaledenom > 0) {
    if (dfCurrentScale < dfNewScale && dfNewScale >  php_map->map->web.maxscaledenom) {
      RETURN_LONG(MS_FAILURE);
    }
  }

  /* ==================================================================== */
  /*      we do a special case for zoom in : we try to zoom as much as    */
  /*      possible using the mincale set in the .map.                     */
  /* ==================================================================== */
  if (php_map->map->web.minscaledenom > 0 && dfNewScale <  php_map->map->web.minscaledenom &&
      dfCurrentScale > dfNewScale) {
    dfDeltaExt =
      GetDeltaExtentsUsingScale(php_map->map->web.minscaledenom, php_map->map->units,
                                dfGeoPosY, php_map->map->width,
                                php_map->map->resolution);
    if (dfDeltaExt > 0.0) {
      newGeoRefExtent.minx = dfGeoPosX - (dfDeltaExt/2);
      newGeoRefExtent.miny = dfGeoPosY - (dfDeltaExt/2);
      newGeoRefExtent.maxx = dfGeoPosX + (dfDeltaExt/2);
      newGeoRefExtent.maxy = dfGeoPosY + (dfDeltaExt/2);
    } else
      RETURN_LONG(MS_FAILURE);
  }
  /* -------------------------------------------------------------------- */
  /*      If the buffer is set, make sure that the extents do not go      */
  /*      beyond the buffer.                                              */
  /* -------------------------------------------------------------------- */
  if (zmaxGeoRefExtent) {
    dfDeltaX = newGeoRefExtent.maxx - newGeoRefExtent.minx;
    dfDeltaY = newGeoRefExtent.maxy - newGeoRefExtent.miny;

    /* Make sure Current georef extents is not bigger than max extents */
    if (dfDeltaX > (php_maxGeoRefExtent->rect->maxx-php_maxGeoRefExtent->rect->minx))
      dfDeltaX = php_maxGeoRefExtent->rect->maxx-php_maxGeoRefExtent->rect->minx;
    if (dfDeltaY > (php_maxGeoRefExtent->rect->maxy-php_maxGeoRefExtent->rect->miny))
      dfDeltaY = php_maxGeoRefExtent->rect->maxy-php_maxGeoRefExtent->rect->miny;

    if (newGeoRefExtent.minx < php_maxGeoRefExtent->rect->minx) {
      newGeoRefExtent.minx = php_maxGeoRefExtent->rect->minx;
      newGeoRefExtent.maxx =  newGeoRefExtent.minx + dfDeltaX;
    }
    if (newGeoRefExtent.maxx > php_maxGeoRefExtent->rect->maxx) {
      newGeoRefExtent.maxx = php_maxGeoRefExtent->rect->maxx;
      newGeoRefExtent.minx = newGeoRefExtent.maxx - dfDeltaX;
    }
    if (newGeoRefExtent.miny < php_maxGeoRefExtent->rect->miny) {
      newGeoRefExtent.miny = php_maxGeoRefExtent->rect->miny;
      newGeoRefExtent.maxy =  newGeoRefExtent.miny + dfDeltaY;
    }
    if (newGeoRefExtent.maxy > php_maxGeoRefExtent->rect->maxy) {
      newGeoRefExtent.maxy = php_maxGeoRefExtent->rect->maxy;
      newGeoRefExtent.miny = newGeoRefExtent.maxy - dfDeltaY;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      set the map extents with new values.                            */
  /* -------------------------------------------------------------------- */
  php_map->map->extent.minx = newGeoRefExtent.minx;
  php_map->map->extent.miny = newGeoRefExtent.miny;
  php_map->map->extent.maxx = newGeoRefExtent.maxx;
  php_map->map->extent.maxy = newGeoRefExtent.maxy;


  php_map->map->cellsize = msAdjustExtent(&(php_map->map->extent), php_map->map->width,
                                          php_map->map->height);
  dfDeltaX = php_map->map->extent.maxx - php_map->map->extent.minx;
  dfDeltaY = php_map->map->extent.maxy - php_map->map->extent.miny;

  if (zmaxGeoRefExtent) {
    if (php_map->map->extent.minx < php_maxGeoRefExtent->rect->minx) {
      php_map->map->extent.minx = php_maxGeoRefExtent->rect->minx;
      php_map->map->extent.maxx = php_map->map->extent.minx + dfDeltaX;
    }
    if (php_map->map->extent.maxx > php_maxGeoRefExtent->rect->maxx) {
      php_map->map->extent.maxx = php_maxGeoRefExtent->rect->maxx;
      newGeoRefExtent.minx = newGeoRefExtent.maxx - dfDeltaX;
    }
    if (php_map->map->extent.miny < php_maxGeoRefExtent->rect->miny) {
      php_map->map->extent.miny = php_maxGeoRefExtent->rect->miny;
      php_map->map->extent.maxy =  php_map->map->extent.miny + dfDeltaY;
    }
    if (php_map->map->extent.maxy > php_maxGeoRefExtent->rect->maxy) {
      php_map->map->extent.maxy = php_maxGeoRefExtent->rect->maxy;
      newGeoRefExtent.miny = newGeoRefExtent.maxy - dfDeltaY;
    }
  }

  if (msCalculateScale(php_map->map->extent, php_map->map->units, php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &(php_map->map->scaledenom)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(MS_SUCCESS);
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

/* {{{ proto int map.queryByPoint(pointObj point, int mode, double buffer)
   Query at point location. */
PHP_METHOD(mapObj, queryByPoint)
{
  zval *zobj = getThis();
  zval *zpoint;
  long mode;
  double buffer;
  int status = MS_FAILURE;
  php_point_object *php_point;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Old",
                            &zpoint, mapscript_ce_point,
                            &mode, &buffer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);

  status = mapObj_queryByPoint(php_map->map, php_point->point, mode, buffer);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.queryByRect(rectObj rect) */
PHP_METHOD(mapObj, queryByRect)
{
  zval *zobj = getThis();
  zval *zrect;
  int status = MS_FAILURE;
  php_rect_object *php_rect;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zrect, mapscript_ce_rect) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_rect = (php_rect_object *) zend_object_store_get_object(zrect TSRMLS_CC);

  status = mapObj_queryByRect(php_map->map, *(php_rect->rect));
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/**********************************************************************
 *                    map->queryByShape(shape)
 *
 * Generates a result set based on a single shape, the shape is
 * assumed to be a polygon at this point.
 **********************************************************************/

/* {{{ proto int map.queryByShape(shapeObj poShape) */
PHP_METHOD(mapObj, queryByShape)
{
  zval *zobj = getThis();
  zval *zshape;
  int status = MS_FAILURE;
  php_shape_object *php_shape;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  status = mapObj_queryByShape(php_map->map, php_shape->shape);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.queryByFeatures(int slayer) */
PHP_METHOD(mapObj, queryByFeatures)
{
  zval *zobj = getThis();
  long slayer;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &slayer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_queryByFeatures(php_map->map, slayer);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/**********************************************************************
 *     map->queryByIndex(qlayer, tileindex, shapeindex, bAddtoQuery)
 *
 *
 * Add a shape into the query result on a specific layer.
 * if bAddtoQuery (not mandatory) isset to true, the sahpe will be added to
 * the existing query result. Else the query result is cleared before adding
 * the sahpe (which is the default behavior).
 **********************************************************************/
/* {{{ proto int map.queryByIndex(int layerIndex, int tileIndex,
                                  int shapeIndex, int addToQuery) */
PHP_METHOD(mapObj, queryByIndex)
{
  zval *zobj = getThis();
  long layerIndex, tileIndex, shapeIndex, addToQuery = MS_FALSE;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lll|l",
                            &layerIndex, &tileIndex,
                            &shapeIndex, &addToQuery) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_queryByIndex(php_map->map, layerIndex,
                               tileIndex,
                               shapeIndex,
                               addToQuery);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.queryByFilter(char string) */
PHP_METHOD(mapObj, queryByFilter)
{
  zval *zobj = getThis();
  char *string;
  long string_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &string, &string_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_queryByFilter(php_map->map, string);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.savequery(string filename, int results)
   Save the current query to a specfied file. Can be used with loadquery */
PHP_METHOD(mapObj, saveQuery)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  long results = MS_FALSE;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",
                            &filename, &filename_len, &results) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_saveQuery(php_map->map, filename, results);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.savequeryasgml(string filename, string namespace)
   Save the current query to a specfied file as GML. */
PHP_METHOD(mapObj, saveQueryAsGML)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  char *namespace = "GOMF";
  long namespace_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                            &filename, &filename_len, &namespace, &namespace_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = msGMLWriteQuery(php_map->map, filename, namespace);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.loadquery(filename)
   Load the query from a specfied file. Used with savequery */
PHP_METHOD(mapObj, loadQuery)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filename, &filename_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_loadQuery(php_map->map, filename);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map->freeQuery(int qlayer)
   Free the query on a specified layer. If qlayer is set to -1
   all queries on all layers will be freed. */
PHP_METHOD(mapObj, freeQuery)
{
  zval *zobj = getThis();
  long qlayer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &qlayer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  mapObj_freeQuery(php_map->map, qlayer);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int map.save(string filename)
   Write map object to a file. */
PHP_METHOD(mapObj, save)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filename, &filename_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_save(php_map->map, filename);
  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */


/* {{{ proto int map.setProjection(string projection_params[, int setUnitsAndExtents])
   Set projection and coord. system for the map. */
PHP_METHOD(mapObj, setProjection)
{
  zval *zobj = getThis();
  char *projection;
  long projection_len = 0;
  int status = MS_FAILURE;
  long setUnitsAndExtents = MS_FALSE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",
                            &projection, &projection_len, &setUnitsAndExtents) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapscript_map_setProjection(MS_FALSE, php_map,
                                       projection, setUnitsAndExtents TSRMLS_CC);
  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.getProjection()
    Return the projection string of the layer. Returns NULL on error. */
PHP_METHOD(mapObj, getProjection)
{
  zval *zobj = getThis();
  char *projection = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  projection = mapObj_getProjection(php_map->map);
  if (projection == NULL) {
    RETURN_NULL();
  }

  RETVAL_STRING(projection, 1);
  free(projection);
}
/* }}} */

/* {{{ proto int map.setWKTProjection(string projection)
   Set projection and coord. system for the map. Returns MS_FAILURE on error. */
PHP_METHOD(mapObj, setWKTProjection)
{
  zval *zobj = getThis();
  char *projection;
  long projection_len = 0;
  int status = MS_FAILURE;
  long setUnitsAndExtents = MS_FALSE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",
                            &projection, &projection_len, &setUnitsAndExtents) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapscript_map_setProjection(MS_TRUE, php_map,
                                       projection, setUnitsAndExtents TSRMLS_CC);
  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string map.getMetaData(string name)
   Return MetaData entry by name, or empty string if not found. */
PHP_METHOD(mapObj, getMetaData)
{
  zval *zname;
  zval *zobj = getThis();
  php_map_object *php_map;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",
                            &zname) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_map->metadata, &php_map->map->web.metadata);

  MAPSCRIPT_CALL_METHOD_1(php_map->metadata, "get", retval, zname);

  RETURN_STRING(Z_STRVAL_P(retval),1);
}
/* }}} */

/* {{{ proto int map.setMetaData(string name, string value)
   Set MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(mapObj, setMetaData)
{
  zval *zname, *zvalue;
  zval *zobj = getThis();
  php_map_object *php_map;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz",
                            &zname, &zvalue) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_map->metadata, &php_map->map->web.metadata);

  MAPSCRIPT_CALL_METHOD_2(php_map->metadata, "set", retval, zname, zvalue);

  RETURN_LONG(Z_LVAL_P(retval));
}
/* }}} */

/* {{{ proto int map.removeMetaData(string name)
   Remove MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(mapObj, removeMetaData)
{
  zval *zname;
  zval *zobj = getThis();
  php_map_object *php_map;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",
                            &zname) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_map->metadata, &php_map->map->web.metadata);

  MAPSCRIPT_CALL_METHOD_1(php_map->metadata, "remove", retval, zname);

  RETURN_LONG(Z_LVAL_P(retval));
}
/* }}} */

/* {{{ proto int map.getLayersIndexByGroup(string groupname)
   Return an array of layer's index given a group name. */
PHP_METHOD(mapObj, getLayersIndexByGroup)
{
  zval *zobj = getThis();
  char *groupName;
  long groupName_len = 0;
  int *indexes = NULL;
  int count = 0;
  int i;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &groupName, &groupName_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  array_init(return_value);
  indexes = mapObj_getLayersIndexByGroup(php_map->map, groupName, &count);

  if (indexes && count > 0) {
    for (i=0; i<count; i++) {
      add_next_index_long(return_value, indexes[i]);
    }
    free (indexes);
  }
}
/* }}} */

/* {{{ proto int map.getAllGroupNames()
   Return an array containing all the group names.*/
PHP_METHOD(mapObj, getAllGroupNames)
{
  zval *zobj = getThis();
  int i, numTok;
  char **groups = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  array_init(return_value);
  if (php_map->map->numlayers > 0) {
    groups = msGetAllGroupNames(php_map->map, &numTok);

    for (i=0; i<numTok; i++) {
      /* add a copy of the group name to the PHP array */
      add_next_index_string(return_value, groups[i], 1);
      free(groups[i]);
    }
    free(groups);
  }
}
/* }}} */

/* {{{ proto int map.getAllLayerNames()
   Return an array conating all the layers name.*/
PHP_METHOD(mapObj, getAllLayerNames)
{
  zval *zobj = getThis();
  int count = 0;
  int i;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  array_init(return_value);
  count = php_map->map->numlayers;
  for (i=0; i<count; i++) {
    add_next_index_string(return_value,  php_map->map->layers[i]->name, 1);
  }
}
/* }}} */

/* {{{ proto int map.movelayerup(int index)
   Returns MS_SUCCESS/MS_FAILURE. */
PHP_METHOD(mapObj, moveLayerUp)
{
  zval *zobj = getThis();
  long index;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_moveLayerup(php_map->map, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.movelayerdown(int index)
   Returns MS_SUCCESS/MS_FAILURE. */
PHP_METHOD(mapObj, moveLayerDown)
{
  zval *zobj = getThis();
  long index;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_moveLayerdown(php_map->map, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.getLayersDrawingOrder()
   Return an array containing all the layers index sorted by drawing order.
   Note : the first element in the array is the one drawn first.*/
PHP_METHOD(mapObj, getLayersDrawingOrder)
{
  zval *zobj = getThis();
  int count = 0;
  int i;
  int *layerIndexes = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  array_init(return_value);

  layerIndexes = mapObj_getLayersdrawingOrder(php_map->map);
  count = php_map->map->numlayers;

  /* -------------------------------------------------------------------- */
  /*      Go through the prioriy list and return the layers index. If     */
  /*      the priority list is not set, it will return the layer          */
  /*      indexs as they were at the load time.                           */
  /* -------------------------------------------------------------------- */
  for (i=0; i<count; i++) {
    if (layerIndexes) {
      add_next_index_long(return_value,  layerIndexes[i]);
    } else
      add_next_index_long(return_value, i);
  }
}
/* }}} */

/* {{{ proto int map.setLayersDrawingOrder(array_layer_index)
   Set the array used for the drawing order.
   array_layers_index : an array containing all the layer's index ordered
                        by the drawing priority.
                        Ex : for 3 layers in the map file, if
                            array[0] = 2
                            array[1] = 0
                            array[2] = 1
                            will set the darwing order to layer 2, layer 0,
                            and then layer 1.
   Return MS_SUCCESS on success or MS_FAILURE.
   Note : the first element in the array is the one drawn first.*/
PHP_METHOD(mapObj, setLayersDrawingOrder)
{
  zval *zobj = getThis();
  zval *zindexes, **ppzval;
  HashTable *indexes_hash = NULL;
  int    *indexes = NULL;
  int numElements = 0;
  int i = 0;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a",
                            &zindexes) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  indexes_hash = Z_ARRVAL_P(zindexes);

  numElements = zend_hash_num_elements(indexes_hash);

  /* -------------------------------------------------------------------- */
  /*      validate that the array of index given has the same size as     */
  /*      the number of layers and also the the indexs are valid.         */
  /* -------------------------------------------------------------------- */
  if (php_map->map->numlayers != numElements) {
    RETURN_LONG(MS_FAILURE);
  }
  indexes = (int*)malloc(sizeof(int)*numElements);

  for(zend_hash_internal_pointer_reset(indexes_hash);
      zend_hash_has_more_elements(indexes_hash) == SUCCESS;
      zend_hash_move_forward(indexes_hash), ++i) {
    zend_hash_get_current_data(indexes_hash, (void **)&ppzval);
    indexes[i] = Z_LVAL_PP(ppzval);
  }

  if (!mapObj_setLayersdrawingOrder(php_map->map, indexes)) {
    free(indexes);
    RETURN_LONG(MS_FAILURE);
  }
  free(indexes);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int map.processTemplate(array params, int generateImages)*/
PHP_METHOD(mapObj, processTemplate)
{
  zval *zobj = getThis();
  zval *zindexes;
  HashTable *indexes_hash = NULL;
  long generateImages;
  char *buffer = NULL;
  int index = 0;
  int numElements = 0;
  int i, size;
  char        **papszNameValue = NULL;
  char        **papszName = NULL;
  char        **papszValue = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "al",
                            &zindexes, &generateImages) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  indexes_hash = Z_ARRVAL_P(zindexes);

  /**
   * Allocate 2 times the number of elements in
   * the array, since with associative arrays in PHP
   * keys are not counted.
   */

  numElements = zend_hash_num_elements(indexes_hash);
  size = (numElements * 2 + 1) * sizeof(char *);

  papszNameValue = (char **)emalloc(size+1);
  memset((char *)papszNameValue, 0, size);

  if (mapscript_extract_associative_array(indexes_hash, papszNameValue)) {
    papszName = (char **)malloc(sizeof(char *)*numElements);
    papszValue = (char **)malloc(sizeof(char *)*numElements);

    for (i=0; i<numElements; i++) {
      index = i*2;
      papszName[i] = papszNameValue[index];
      papszValue[i] = papszNameValue[index+1];
    }

  } else {
    // Failed for some reason
    mapscript_report_php_error(E_WARNING, "processTemplate: failed reading array" TSRMLS_CC);
    RETURN_STRING("", 1);
  }
  efree(papszNameValue);

  buffer = mapObj_processTemplate(php_map->map, generateImages,
                                  papszName, papszValue, numElements);

  msFree(papszName);  // The strings inside the array are just refs
  msFree(papszValue);

  if (buffer) {
    RETVAL_STRING(buffer, 1);
    free(buffer);
  } else {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_STRING("", 1);
  }
}
/* }}} */

/* {{{ proto int map.processQueryTemplate(array params, int generateImages)*/
PHP_METHOD(mapObj, processQueryTemplate)
{
  zval *zobj = getThis();
  zval *zindexes;
  HashTable *indexes_hash = NULL;
  long generateImages = MS_TRUE;
  char *buffer = NULL;
  int index = 0;
  int numElements = 0;
  int i, size;
  char        **papszNameValue = NULL;
  char        **papszName = NULL;
  char        **papszValue = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|l",
                            &zindexes, &generateImages) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  indexes_hash = Z_ARRVAL_P(zindexes);

  /**
   * Allocate 2 times the number of elements in
   * the array, since with associative arrays in PHP
   * keys are not counted.
   */

  numElements = zend_hash_num_elements(indexes_hash);
  size = (numElements * 2 + 1) * sizeof(char *);

  papszNameValue = (char **)emalloc(size+1);
  memset((char *)papszNameValue, 0, size);

  if (mapscript_extract_associative_array(indexes_hash, papszNameValue)) {
    papszName = (char **)malloc(sizeof(char *)*numElements);
    papszValue = (char **)malloc(sizeof(char *)*numElements);


    for (i=0; i<numElements; i++) {
      index = i*2;
      papszName[i] = papszNameValue[index];
      papszValue[i] = papszNameValue[index+1];
    }

  } else {
    // Failed for some reason
    mapscript_report_php_error(E_WARNING, "processQueryTemplate: failed reading array" TSRMLS_CC);
    RETURN_STRING("", 1);
  }
  efree(papszNameValue);


  buffer = mapObj_processQueryTemplate(php_map->map, generateImages,
                                       papszName, papszValue, numElements);

  msFree(papszName);  // The strings inside the array are just refs
  msFree(papszValue);

  if (buffer) {
    RETVAL_STRING(buffer, 1);
    free(buffer);
  } else {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_STRING("", 1);
  }
}
/* }}} */

/* {{{ proto int map.processLegendTemplate(array params)*/
PHP_METHOD(mapObj, processLegendTemplate)
{
  zval *zobj = getThis();
  zval *zindexes;
  HashTable *indexes_hash = NULL;
  char *buffer = NULL;
  int index = 0;
  int numElements = 0;
  int i, size;
  char        **papszNameValue = NULL;
  char        **papszName = NULL;
  char        **papszValue = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a",
                            &zindexes) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  indexes_hash = Z_ARRVAL_P(zindexes);

  /**
   * Allocate 2 times the number of elements in
   * the array, since with associative arrays in PHP
   * keys are not counted.
   */

  numElements = zend_hash_num_elements(indexes_hash);
  size = (numElements * 2 + 1) * sizeof(char *);

  papszNameValue = (char **)emalloc(size+1);
  memset((char *)papszNameValue, 0, size);

  if (mapscript_extract_associative_array(indexes_hash, papszNameValue)) {
    papszName = (char **)malloc(sizeof(char *)*numElements);
    papszValue = (char **)malloc(sizeof(char *)*numElements);


    for (i=0; i<numElements; i++) {
      index = i*2;
      papszName[i] = papszNameValue[index];
      papszValue[i] = papszNameValue[index+1];
    }

  } else {
    // Failed for some reason
    mapscript_report_php_error(E_WARNING, "processLegendTemplate: failed reading array" TSRMLS_CC);
    RETURN_STRING("", 1);
  }
  efree(papszNameValue);


  buffer = mapObj_processLegendTemplate(php_map->map, papszName,
                                        papszValue, numElements);

  msFree(papszName);  // The strings inside the array are just refs
  msFree(papszValue);

  if (buffer) {
    RETVAL_STRING(buffer, 1);
    free(buffer);
  } else {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_STRING("", 1);
  }
}
/* }}} */

/* {{{ proto int map.setSymbolSet(fileName)*/
PHP_METHOD(mapObj, setSymbolSet)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filename, &filename_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if(filename_len > 0) {
    if ((status = mapObj_setSymbolSet(php_map->map,
                                      filename)) != 0) {
      mapscript_throw_mapserver_exception("Failed loading symbolset from %s" TSRMLS_CC,
                                          filename);
      return;
    }
  }

  RETURN_LONG(status);
}
/* }}} */


/* {{{ proto int map.getNumSymbols()
   Returns the number of sumbols from this map. */
PHP_METHOD(mapObj, getNumSymbols)
{
  zval *zobj = getThis();
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  RETURN_LONG(php_map->map->symbolset.numsymbols);
}
/* }}} */

/* {{{ proto int map.setFontName(fileName)*/
PHP_METHOD(mapObj, setFontSet)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filename, &filename_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if(filename_len > 0) {
    if ((status = mapObj_setFontSet(php_map->map,
                                    filename)) != 0) {
      mapscript_throw_mapserver_exception("Failed loading fontset from %s" TSRMLS_CC,
                                          filename);
      return;
    }
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int selectOutputFormat(string type)
   Selects the output format to be used in the map. Returns MS_SUCCESS/MS_FAILURE. */
PHP_METHOD(mapObj, selectOutputFormat)
{
  zval *zobj = getThis();
  char *type;
  long type_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;
  php_outputformat_object *php_outputformat = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &type, &type_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  if (php_map->outputformat)
    php_outputformat = (php_outputformat_object *) zend_object_store_get_object(php_map->outputformat TSRMLS_CC);

  if ((status = mapObj_selectOutputFormat(php_map->map,
                                          type)) != MS_SUCCESS)

  {
    mapscript_report_php_error(E_WARNING, "Unable to set output format to '%s'" TSRMLS_CC,
                               type);
  } else if (php_map->outputformat) {
    php_outputformat->outputformat = php_map->map->outputformat;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int appendOutputFormat(outputFormatObj outputformat)
   Appends outputformat object in the map object. Returns the new numoutputformats value */
PHP_METHOD(mapObj, appendOutputFormat)
{
  zval *zobj = getThis();
  zval *zoutputformat = NULL;
  int retval = 0;
  php_map_object *php_map;
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zoutputformat, mapscript_ce_outputformat) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_outputformat = (php_outputformat_object *) zend_object_store_get_object(zoutputformat TSRMLS_CC);

  retval = msAppendOutputFormat(php_map->map, php_outputformat->outputformat);

  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int removeOutputFormat(string name)
   Remove outputformat from the map. Returns MS_SUCCESS/MS_FAILURE. */
PHP_METHOD(mapObj, removeOutputFormat)
{
  zval *zobj = getThis();
  char *name;
  long name_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &name, &name_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((status = msRemoveOutputFormat(php_map->map, name)) != MS_SUCCESS)

  {
    mapscript_report_php_error(E_WARNING, "Unable to remove output format to '%s'" TSRMLS_CC,
                               name);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.getOutputFormat(int index).
   Return the outputformat at index position. */
PHP_METHOD(mapObj, getOutputFormat)
{
  zval *zobj = getThis();
  long index = -1;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (index < 0 || index >= php_map->map->numoutputformats) {
    mapscript_throw_mapserver_exception("Invalid outputformat index." TSRMLS_CC);
    return;
  }

  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_outputformat(php_map->map->outputformatlist[index],
                                parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int map.saveMapContext(fileName) */
PHP_METHOD(mapObj, saveMapContext)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filename, &filename_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if(filename_len > 0) {
    if ((status = mapObj_saveMapContext(php_map->map, filename)) != MS_SUCCESS) {
      mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
      mapscript_report_php_error(E_WARNING, "Failed saving map context from %s" TSRMLS_CC,
                                 filename);
      RETURN_LONG(MS_FAILURE);
    }
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int map.loadMapContext(fileName) */
PHP_METHOD(mapObj, loadMapContext)
{
  zval *zobj = getThis();
  char *filename;
  long filename_len = 0;
  long unique = MS_FALSE;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",
                            &filename, &filename_len, &unique) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if(filename_len > 0) {
    if ((status = mapObj_loadMapContext(php_map->map, filename, unique)) != MS_SUCCESS) {
      mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
      mapscript_report_php_error(E_WARNING, "Failed loading map context from %s" TSRMLS_CC,
                                 filename);
      RETURN_LONG(MS_FAILURE);
    }
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int applySLD(string sldxml)
   Apply the SLD document to the map file. */
PHP_METHOD(mapObj, applySLD)
{
  zval *zobj = getThis();
  char *sldxml;
  long sldxml_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &sldxml, &sldxml_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_applySLD(php_map->map, sldxml);

  RETURN_LONG(status);
}
/* }}} */


/* {{{ proto int applySLDURL(string sldurl)
   Apply the SLD document pointed by the URL to the map file. */
PHP_METHOD(mapObj, applySLDURL)
{
  zval *zobj = getThis();
  char *sldurl;
  long sldurl_len = 0;
  int status = MS_FAILURE;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &sldurl, &sldurl_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = mapObj_applySLDURL(php_map->map, sldurl);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string generateSLD()
   Generates an sld based on the map layers/class. */
PHP_METHOD(mapObj, generateSLD)
{
  zval *zobj = getThis();
  char *buffer = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  buffer = mapObj_generateSLD(php_map->map);

  if (buffer) {
    RETVAL_STRING(buffer, 1);
    free(buffer);
  } else {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_STRING("", 1);
  }
}
/* }}} */

/* {{{ proto string map.getConfigOption(string key)
      Returns the configuation value associated with the key
      passed as argument. Returns an empty string on error.
      prototype : value = $map->getconfigoption(key) */
PHP_METHOD(mapObj, getConfigOption)
{
  zval *zobj = getThis();
  char *key;
  long key_len = 0;
  char *value = NULL;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &key, &key_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((value = (char *)msGetConfigOption(php_map->map, key)) == NULL) {
    RETURN_STRING("", 1);
  } else {
    RETURN_STRING(value, 1);
  }
}
/* }}} */

/* {{{ proto int map.setConfigOption(string key, string value)
   Sets a config parameter using the key and the value passed. */
PHP_METHOD(mapObj, setConfigOption)
{
  zval *zobj = getThis();
  char *key;
  long key_len = 0;
  char *value;
  long value_len = 0;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                            &key, &key_len, &value, &value_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  msSetConfigOption(php_map->map, key,value);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int map.applyConfigOptions()
   Applies the config options set in the map file. */
PHP_METHOD(mapObj, applyConfigOptions)
{
  zval *zobj = getThis();
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  msApplyMapConfigOptions(php_map->map);
  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int loadowsparameters(OWSRequestObj request, string version)
   Load OWS request parameters (BBOX, LAYERS, &c.) into map. Returns MS_SUCCESS or MS_FAILURE */
PHP_METHOD(mapObj, loadOwsParameters)
{
  zval *zobj = getThis();
  zval *zrequest;
  char *version = NULL;
  long version_len = 0;
  int isZval = 1;
  int status = MS_FAILURE;
  php_owsrequest_object *php_request;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|s",
                            &zrequest, mapscript_ce_owsrequest,
                            &version, &version_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_request = (php_owsrequest_object *) zend_object_store_get_object(zrequest TSRMLS_CC);

  if (!version) {
    version = strdup("1.1.1");
    isZval = 0;
  }

  status = mapObj_loadOWSParameters(php_map->map, php_request->cgirequest, version);

  if (!isZval)
    free(version);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int owsDispatch(owsrequest request)
   Processes and executes the passed OpenGIS Web Services request on the map. */
PHP_METHOD(mapObj, owsDispatch)
{
  zval *zobj = getThis();
  zval *zrequest;
  int status = MS_FAILURE;
  php_owsrequest_object *php_request;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zrequest, mapscript_ce_owsrequest) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_request = (php_owsrequest_object *) zend_object_store_get_object(zrequest TSRMLS_CC);

  status = mapObj_OWSDispatch(php_map->map, php_request->cgirequest);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int insertLayer(layerObj layer, index)
   Returns the index where the layer had been inserted*/
PHP_METHOD(mapObj, insertLayer)
{
  zval *zobj = getThis();
  zval *zlayer;
  long index = -1;
  int retval = -1;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|l",
                            &zlayer, mapscript_ce_layer, &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(zlayer TSRMLS_CC);


  if ((retval = mapObj_insertLayer(php_map->map, php_layer->layer, index)) < 0) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* return layer index */
  RETURN_LONG(retval);

}
/* }}} */

/* {{{ proto int removeLayer(int layer_index)
   Returns layerObj removed on sucesss, else null. */
PHP_METHOD(mapObj, removeLayer)
{
  zval *zobj = getThis();
  long index = -1;
  layerObj *layer = NULL;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((layer = mapObj_removeLayer(php_map->map, index)) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* return layer object */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_layer(layer, parent, return_value TSRMLS_CC);
}
/* }}} */

#ifdef disabled
/* {{{ proto int map.getLabel().
   Return the next label from the map���s labelcache, allowing iteration
   over labels. Return NULL when the labelcache is empty. */
PHP_METHOD(mapObj, getLabel)
{
  zval *zobj = getThis();
  long index = -1;
  labelCacheMemberObj *labelCacheMember = NULL;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  labelCacheMember=mapObj_getLabel(php_map->map, index);

  if (labelCacheMember == NULL)
    RETURN_NULL();

  /* Return labelCacheMember object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_labelcachemember(labelCacheMember, parent, return_value TSRMLS_CC);
}
/* }}} */
#endif

/* {{{ proto string convertToString()
   Convert the map object to string. */
PHP_METHOD(mapObj, convertToString)
{
  zval *zobj = getThis();
  php_map_object *php_map;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value =  mapObj_convertToString(php_map->map);

  if (value == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
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
PHP_METHOD(mapObj, getLatLongExtent)
{
#ifdef USE_PROJ
  zval *zobj = getThis();
  rectObj     geoRefExt;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  geoRefExt.minx = php_map->map->extent.minx;
  geoRefExt.miny = php_map->map->extent.miny;
  geoRefExt.maxx = php_map->map->extent.maxx;
  geoRefExt.maxy = php_map->map->extent.maxy;

  if (php_map->map->projection.proj != NULL) {
    msProjectRect(&(php_map->map->projection), NULL, &geoRefExt);
  }

  /* Return rectObj */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_rect(&geoRefExt, parent, return_value TSRMLS_CC);

#else
  mapscript_throw_exception("Available only with PROJ.4 support." TSRMLS_CC);
  return;
#endif
}

/* {{{ proto int map.free().
   Free explicitly the map object.
   Breaks the internal circular references between the map object and its children.
*/
PHP_METHOD(mapObj, free)
{
  zval *zobj = getThis();
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_map->outputformat);
  MAPSCRIPT_DELREF(php_map->extent);
  MAPSCRIPT_DELREF(php_map->web);
  MAPSCRIPT_DELREF(php_map->reference);
  MAPSCRIPT_DELREF(php_map->imagecolor);
  MAPSCRIPT_DELREF(php_map->scalebar);
  MAPSCRIPT_DELREF(php_map->legend);
  MAPSCRIPT_DELREF(php_map->querymap);
#ifdef disabled
  MAPSCRIPT_DELREF(php_map->labelcache);
#endif
  MAPSCRIPT_DELREF(php_map->projection);
  MAPSCRIPT_DELREF(php_map->metadata);
}
/* }}} */

zend_function_entry map_functions[] = {
  PHP_ME(mapObj, __construct, map___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(mapObj, __get, map___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, __set, map___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(mapObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getSymbolByName, map_getSymbolByName_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getSymbolObjectById, map_getSymbolObjectById_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, prepareQuery, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, prepareImage, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, draw, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, drawQuery, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, drawLegend, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, drawReferenceMap, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, drawScaleBar, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, embedLegend, map_embedLegend_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, embedScaleBar, map_embedScaleBar_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, drawLabelCache, map_drawLabelCache_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getLayer, map_getLayer_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getLayerByName, map_getLayerByName_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getColorByIndex, map_getColorByIndex_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setExtent, map_setExtent_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setCenter, map_setCenter_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, offsetExtent, map_offsetExtent_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, scaleExtent, map_scaleExtent_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setRotation, map_setRotation_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setSize, map_setSize_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, zoomPoint, map_zoomPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, zoomRectangle, map_zoomRectangle_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, zoomScale, map_zoomScale_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, queryByPoint, map_queryByPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, queryByRect, map_queryByRect_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, queryByShape, map_queryByShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, queryByFeatures, map_queryByFeatures_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, queryByFilter, map_queryByFilter_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, queryByIndex, map_queryByIndex_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, saveQuery, map_saveQuery_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, saveQueryAsGML, map_saveQueryAsGML_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, loadQuery, map_loadQuery_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, freeQuery, map_freeQuery_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, save, map_save_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getProjection, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setProjection, map_setProjection_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setWKTProjection, map_setWKTProjection_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getMetaData, map_getMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setMetaData, map_setMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, removeMetaData, map_removeMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getLayersIndexByGroup, map_getLayersIndexByGroup_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getAllGroupNames, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getAllLayerNames, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, moveLayerUp, map_moveLayerUp_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, moveLayerDown, map_moveLayerDown_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getLayersDrawingOrder, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setLayersDrawingOrder, map_setLayersDrawingOrder_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, processTemplate, map_processTemplate_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, processQueryTemplate, map_processQueryTemplate_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, processLegendTemplate, map_processLegendTemplate_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setSymbolSet, map_setSymbolSet_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getNumSymbols, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setFontSet, map_setFontSet_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, selectOutputFormat, map_selectOutputFormat_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, appendOutputFormat, map_appendOutputFormat_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, removeOutputFormat, map_removeOutputFormat_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getOutputFormat, map_getOutputFormat_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, saveMapContext, map_saveMapContext_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, loadMapContext, map_loadMapContext_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, applySLD, map_applySLD_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, applySLDURL, map_applySLDURL_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, generateSLD, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getConfigOption, map_getConfigOption_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, setConfigOption, map_setConfigOption_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, applyConfigOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, loadOwsParameters, map_loadOwsParameters_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, owsDispatch, map_owsDispatch_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, insertLayer, map_insertLayer_args, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, removeLayer, map_removeLayer_args, ZEND_ACC_PUBLIC)
#ifdef disabled
  PHP_ME(mapObj, getLabel, map_getLabel_args, ZEND_ACC_PUBLIC)
#endif
  PHP_ME(mapObj, convertToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, getLatLongExtent, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(mapObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

static int mapscript_map_setProjection(int isWKTProj, php_map_object *php_map,
                                       char *projString, int setUnitsAndExtents TSRMLS_DC)
{
#ifdef USE_PROJ
  int                 status = MS_SUCCESS;
  int                 units =   MS_METERS;
  projectionObj       in;
  projectionObj       out;
  rectObj             rect;
  int                 setNewExtents = 0;
  php_projection_object *php_projection = NULL;
  php_rect_object *php_extent = NULL;

  if (php_map->projection)
    php_projection = (php_projection_object *) zend_object_store_get_object(php_map->projection TSRMLS_CC);
  if (php_map->extent)
    php_extent = (php_rect_object *) zend_object_store_get_object(php_map->extent TSRMLS_CC);

  in = php_map->map->projection;
  msInitProjection(&out);
  if (isWKTProj)
    msOGCWKT2ProjectionObj(projString, &(out),php_map->map->debug);
  else
    msLoadProjectionString(&(out),  projString);
  rect = php_map->map->extent;

  if (in.proj!= NULL && out.proj!=NULL) {
    if (msProjectionsDiffer(&in, &out)) {
      if (msProjectRect(&in, &out, &rect) == MS_SUCCESS)
        setNewExtents =1;
    }
  }
  /* Free the temporary projection object */
  msFreeProjection(&out);

  if (isWKTProj)
    status = mapObj_setWKTProjection(php_map->map, projString);
  else
    status = mapObj_setProjection(php_map->map, projString);

  if (status == -1) {
    mapscript_report_php_error(E_WARNING, "setProjection failed" TSRMLS_CC);
    return MS_FAILURE;
  } else if (php_map->projection) {
    php_projection->projection = &(php_map->map->projection);
  }

  units = GetMapserverUnitUsingProj(&(php_map->map->projection));
  if (units != -1 && setUnitsAndExtents) {
    /* --------------------------------------------------------------------
          set the units and php_map->map extents.
     -------------------------------------------------------------------- */
    php_map->map->units = units;

    if (setNewExtents) {
      php_map->map->extent = rect;

      php_map->map->cellsize = msAdjustExtent(&(php_map->map->extent), php_map->map->width,
                                              php_map->map->height);
      msCalculateScale(php_map->map->extent, php_map->map->units, php_map->map->width, php_map->map->height,
                       php_map->map->resolution, &(php_map->map->scaledenom));

      if (php_map->extent)
        php_extent->rect = &(php_map->map->extent);
    }
  }

  return MS_SUCCESS;
#else
  mapscript_throw_exception("Available only with PROJ.4 support." TSRMLS_CC);
  return MS_FAILURE;
#endif
}

void mapscript_create_map(mapObj *map, zval *return_value TSRMLS_DC)
{
  php_map_object * php_map;
  object_init_ex(return_value, mapscript_ce_map);
  php_map = (php_map_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_map->map = map;
}

static void mapscript_map_object_destroy(void *object TSRMLS_DC)
{
  php_map_object *php_map = (php_map_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_map);

  MAPSCRIPT_DELREF(php_map->outputformat);
  MAPSCRIPT_DELREF(php_map->extent);
  MAPSCRIPT_DELREF(php_map->web);
  MAPSCRIPT_DELREF(php_map->reference);
  MAPSCRIPT_DELREF(php_map->imagecolor);
  MAPSCRIPT_DELREF(php_map->scalebar);
  MAPSCRIPT_DELREF(php_map->legend);
  MAPSCRIPT_DELREF(php_map->querymap);
#ifdef disabled
  MAPSCRIPT_DELREF(php_map->labelcache);
#endif
  MAPSCRIPT_DELREF(php_map->projection);
  MAPSCRIPT_DELREF(php_map->metadata);
  MAPSCRIPT_DELREF(php_map->configoptions);

  mapObj_destroy(php_map->map);
  efree(object);
}

static zend_object_value mapscript_map_object_new_ex(zend_class_entry *ce, php_map_object **ptr TSRMLS_DC)
{
  zend_object_value retval;
  php_map_object *php_map;

  MAPSCRIPT_ALLOC_OBJECT(php_map, php_map_object);

  retval = mapscript_object_new_ex(&php_map->std, ce,
                                   &mapscript_map_object_destroy,
                                   &mapscript_map_object_handlers TSRMLS_CC);

  if (ptr)
    *ptr = php_map;

  php_map->outputformat = NULL;
  php_map->extent = NULL;
  php_map->web = NULL;
  php_map->reference = NULL;
  php_map->imagecolor = NULL;
  php_map->scalebar = NULL;
  php_map->legend = NULL;
  php_map->querymap = NULL;
#ifdef disabled
  php_map->labelcache = NULL;
#endif
  php_map->projection = NULL;
  php_map->metadata = NULL;
  php_map->configoptions = NULL;

  return retval;
}

static zend_object_value mapscript_map_object_new(zend_class_entry *ce TSRMLS_DC)
{
  return mapscript_map_object_new_ex(ce, NULL TSRMLS_CC);
}

static zend_object_value mapscript_map_object_clone(zval *zobj TSRMLS_DC)
{
  php_map_object *php_map_old, *php_map_new;
  zend_object_value new_ov;

  php_map_old = (php_map_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  new_ov = mapscript_map_object_new_ex(mapscript_ce_map, &php_map_new TSRMLS_CC);
  zend_objects_clone_members(&php_map_new->std, new_ov, &php_map_old->std, Z_OBJ_HANDLE_P(zobj) TSRMLS_CC);

  php_map_new->map = mapObj_clone(php_map_old->map);

  return new_ov;
}

PHP_MINIT_FUNCTION(map)
{
  zend_class_entry ce;

  memcpy(&mapscript_map_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mapscript_map_object_handlers.clone_obj = mapscript_map_object_clone;

  MAPSCRIPT_REGISTER_CLASS("mapObj",
                           map_functions,
                           mapscript_ce_map,
                           mapscript_map_object_new);

  mapscript_ce_map->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}

