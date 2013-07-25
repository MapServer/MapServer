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

zend_class_entry *mapscript_ce_symbol;

ZEND_BEGIN_ARG_INFO_EX(symbol___construct_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_ARG_INFO(0, symbolName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(symbol___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(symbol___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(symbol_setPoints_args, 0, 0, 1)
ZEND_ARG_INFO(0, points)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(symbol_setImagePath_args, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(symbol_setImage_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(symbol_getImage_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, outputformat, outputFormatObj, 0)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct(mapObj map, string symbolname)
   Create a new symbolObj instance. */
PHP_METHOD(symbolObj, __construct)
{
  zval *zmap;
  char *symbolName;
  long symbolName_len = 0;
  int symbolId = -1;
  php_symbol_object *php_symbol;
  php_map_object *php_map;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os",
                            &zmap, mapscript_ce_map,
                            &symbolName, &symbolName_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
  php_map = (php_map_object *)zend_object_store_get_object(zmap TSRMLS_CC);

  symbolId = msAddNewSymbol(php_map->map, symbolName);

  if (symbolId == -1) {
    mapscript_throw_mapserver_exception("Unable to construct symbolObj" TSRMLS_CC);
    return;
  }

  php_symbol->symbol = php_map->map->symbolset.symbol[symbolId];

  MAPSCRIPT_MAKE_PARENT(zmap, NULL);
  php_symbol->parent = parent;
  MAPSCRIPT_ADDREF(zmap);
}
/* }}} */

PHP_METHOD(symbolObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_symbol_object *php_symbol;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_STRING("name", php_symbol->symbol->name)
  else IF_GET_LONG("type", php_symbol->symbol->type)
    else IF_GET_LONG("inmapfile", php_symbol->symbol->inmapfile)
      else IF_GET_DOUBLE("sizex", php_symbol->symbol->sizex)
        else IF_GET_DOUBLE("sizey", php_symbol->symbol->sizey)
          else IF_GET_LONG("numpoints", php_symbol->symbol->numpoints)
            else IF_GET_LONG("filled", php_symbol->symbol->filled)
              else IF_GET_STRING("imagepath", php_symbol->symbol->imagepath)
                else IF_GET_LONG("transparent", php_symbol->symbol->transparent)
                  else IF_GET_LONG("transparentcolor", php_symbol->symbol->transparentcolor)
                    else IF_GET_STRING("character", php_symbol->symbol->character)
                        else IF_GET_LONG("antialias", php_symbol->symbol->antialias)
                          else IF_GET_DOUBLE("anchorpoint_y", php_symbol->symbol->anchorpoint_y)
                            else IF_GET_DOUBLE("anchorpoint_x", php_symbol->symbol->anchorpoint_x)
                              else IF_GET_DOUBLE("maxx", php_symbol->symbol->maxx)
                                else IF_GET_DOUBLE("minx", php_symbol->symbol->minx)
                                  else IF_GET_DOUBLE("miny", php_symbol->symbol->miny)
                                    else IF_GET_DOUBLE("maxy", php_symbol->symbol->maxy)
                                      else IF_GET_STRING("font", php_symbol->symbol->font)
                                        else {
                                          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                        }
}

PHP_METHOD(symbolObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_symbol_object *php_symbol;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_STRING("name", php_symbol->symbol->name, value)
  else IF_SET_LONG("type", php_symbol->symbol->type, value)
    else IF_SET_LONG("inmapfile", php_symbol->symbol->inmapfile, value)
      else IF_SET_DOUBLE("sizex", php_symbol->symbol->sizex, value)
        else IF_SET_DOUBLE("sizey", php_symbol->symbol->sizey, value)
          else IF_SET_LONG("filled", php_symbol->symbol->filled, value)
            else IF_SET_LONG("transparent", php_symbol->symbol->transparent, value)
              else IF_SET_LONG("transparentcolor", php_symbol->symbol->transparentcolor, value)
                else IF_SET_STRING("character", php_symbol->symbol->character, value)
                    else IF_SET_LONG("antialias", php_symbol->symbol->antialias, value)
                      else IF_SET_STRING("font", php_symbol->symbol->font, value)
                        else IF_SET_DOUBLE("anchorpoint_y", php_symbol->symbol->anchorpoint_y, value)
                          else IF_SET_DOUBLE("anchorpoint_x", php_symbol->symbol->anchorpoint_x, value)
                            else IF_SET_DOUBLE("maxx", php_symbol->symbol->maxx, value)
                              else IF_SET_DOUBLE("maxy", php_symbol->symbol->maxy, value)
                                else IF_SET_DOUBLE("minx", php_symbol->symbol->minx, value)
                                  else IF_SET_DOUBLE("miny", php_symbol->symbol->miny, value)
                                    else if ( (STRING_EQUAL("numpoints", property)) ||
                                              (STRING_EQUAL("imagepath", property))) {
                                      mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
                                    } else {
                                      mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                    }
}

/* {{{ proto int symbol.setpoints(array points)
   Set the points of the symbol ) */
PHP_METHOD(symbolObj, setPoints)
{
  zval *zpoints, **ppzval;
  HashTable *points_hash = NULL;
  zval *zobj = getThis();
  int index = 0, flag = 0, numelements = 0;
  php_symbol_object *php_symbol;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a",
                            &zpoints) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  points_hash = Z_ARRVAL_P(zpoints);

  numelements = zend_hash_num_elements(points_hash);
  if ((numelements == 0) || (numelements % 2 != 0)) {
    mapscript_report_php_error(E_WARNING,
                               "symbol->setpoints : invalid array of %d element(s) as parameter." TSRMLS_CC, numelements);
    RETURN_LONG(MS_FAILURE);

  }

  for(zend_hash_internal_pointer_reset(points_hash);
      zend_hash_has_more_elements(points_hash) == SUCCESS;
      zend_hash_move_forward(points_hash)) {

    zend_hash_get_current_data(points_hash, (void **)&ppzval);
    if (Z_TYPE_PP(ppzval) != IS_DOUBLE)
      convert_to_double(*ppzval);

    if (!flag) {
      php_symbol->symbol->points[index].x = Z_DVAL_PP(ppzval);
      php_symbol->symbol->sizex = MS_MAX(php_symbol->symbol->sizex, php_symbol->symbol->points[index].x);
    } else {
      php_symbol->symbol->points[index].y = Z_DVAL_PP(ppzval);
      php_symbol->symbol->sizey = MS_MAX(php_symbol->symbol->sizey, php_symbol->symbol->points[index].y);
      index++;
    }
    flag = !flag;
  }

  php_symbol->symbol->numpoints = (numelements/2);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int symbol.getPointsArray()
   Returns an array containing the points of the symbol.
   Refer to setpoints to see how the array should be interpreted. */
PHP_METHOD(symbolObj, getPointsArray)
{
  zval *zobj = getThis();
  php_symbol_object *php_symbol;
  int index;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  array_init(return_value);

  if (php_symbol->symbol->numpoints > 0) {
    for (index=0; index < php_symbol->symbol->numpoints; index++) {
      add_next_index_double(return_value, php_symbol->symbol->points[index].x);
      add_next_index_double(return_value, php_symbol->symbol->points[index].y);
    }
  }
}
/* }}} */



/* {{{ proto int symbol.setimagepath(char *imagefile)
   loads a new symbol image  ) */
PHP_METHOD(symbolObj, setImagePath)
{
  zval *zobj = getThis();
  php_symbol_object *php_symbol;
  char *filename;
  long filename_len = 0;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filename, &filename_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = msLoadImageSymbol(php_symbol->symbol, filename);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int setImage(imageObj image)
   Set image pixmap symbol */
PHP_METHOD(symbolObj, setImage)
{
  zval *zimage;
  php_symbol_object *php_symbol;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zimage, mapscript_ce_image) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
  php_image = (php_image_object *)zend_object_store_get_object(zimage TSRMLS_CC);

  RETURN_LONG(symbolObj_setImage(php_symbol->symbol, php_image->image));
}
/* }}} */

/* {{{ proto imageObj getImage(outputFormatObj outputformat)
   Get the symbol image */
PHP_METHOD(symbolObj, getImage)
{
  zval *zoutputformat;
  imageObj *image = NULL;
  php_map_object *php_map;
  php_symbol_object *php_symbol;
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zoutputformat, mapscript_ce_outputformat) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_symbol = (php_symbol_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
  php_map = (php_map_object *) zend_object_store_get_object(php_symbol->parent.val TSRMLS_CC);
  php_outputformat = (php_outputformat_object *)zend_object_store_get_object(zoutputformat TSRMLS_CC);

  image = symbolObj_getImage(php_symbol->symbol, php_outputformat->outputformat);
  if (image == NULL) {
    mapscript_throw_exception("Unable to get the symbol image" TSRMLS_CC);
    return;
  }

  /* the outputformat HAS to be added to the map, since the renderer is now used
     by the current symbol */
  if (msGetOutputFormatIndex(php_map->map, php_outputformat->outputformat->name) == -1)
    msAppendOutputFormat(php_map->map, php_outputformat->outputformat);

  mapscript_create_image(image, return_value TSRMLS_CC);
  } /* }}} */

zend_function_entry symbol_functions[] = {
  PHP_ME(symbolObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(symbolObj, __get, symbol___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(symbolObj, __set, symbol___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(symbolObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(symbolObj, setPoints, symbol_setPoints_args, ZEND_ACC_PUBLIC)
  PHP_ME(symbolObj, getPointsArray, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(symbolObj, setImage, symbol_setImage_args, ZEND_ACC_PUBLIC)
  PHP_ME(symbolObj, getImage, symbol_getImage_args, ZEND_ACC_PUBLIC)
  PHP_ME(symbolObj, setImagePath, symbol_setImagePath_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_symbol(symbolObj *symbol, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_symbol_object * php_symbol;
  object_init_ex(return_value, mapscript_ce_symbol);
  php_symbol = (php_symbol_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_symbol->symbol = symbol;

  php_symbol->parent = parent;

  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_symbol_object_destroy(void *object TSRMLS_DC)
{
  php_symbol_object *php_symbol = (php_symbol_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_symbol);

  MAPSCRIPT_FREE_PARENT(php_symbol->parent);

  /* We don't need to free the symbolObj */

  efree(object);
}

static zend_object_value mapscript_symbol_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_symbol_object *php_symbol;

  MAPSCRIPT_ALLOC_OBJECT(php_symbol, php_symbol_object);

  retval = mapscript_object_new(&php_symbol->std, ce,
                                &mapscript_symbol_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_symbol->parent);

  return retval;
}

PHP_MINIT_FUNCTION(symbol)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("symbolObj",
                           symbol_functions,
                           mapscript_ce_symbol,
                           mapscript_symbol_object_new);

  mapscript_ce_symbol->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
