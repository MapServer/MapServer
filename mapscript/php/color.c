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

zend_class_entry *mapscript_ce_color;

ZEND_BEGIN_ARG_INFO_EX(color___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(color___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(color_setRGB_args, 0, 0, 3)
ZEND_ARG_INFO(0, red)
ZEND_ARG_INFO(0, green)
ZEND_ARG_INFO(0, blue)
ZEND_ARG_INFO(0, alpha)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(color_setHex_args, 0, 0, 1)
ZEND_ARG_INFO(0, rgba)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(color_toHex_args, 0, 0, 0)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct()
   colorObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(colorObj, __construct)
{
  mapscript_throw_exception("colorObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(colorObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_color_object *php_color;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_color = (php_color_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("red", php_color->color->red)
  else IF_GET_LONG("green", php_color->color->green)
    else IF_GET_LONG("blue", php_color->color->blue)
      else IF_GET_LONG("alpha", php_color->color->alpha)
        else {
          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
        }
}

PHP_METHOD(colorObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_color_object *php_color;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_color = (php_color_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_COLOR("red", php_color->color->red, value)
  else IF_SET_COLOR("green", php_color->color->green, value)
    else IF_SET_COLOR("blue", php_color->color->blue, value)
      else IF_SET_COLOR("alpha", php_color->color->alpha, value)
        else {
          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
        }

}

/* {{{ proto int color.setRGB(int R, int G, int B, int A = 255)
    Set new RGB color. */
PHP_METHOD(colorObj, setRGB)
{
  zval *zobj = getThis();
  long red, green, blue, alpha = 255;
  php_color_object *php_color;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lll|l",
                            &red, &green, &blue, &alpha) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_color = (php_color_object *) zend_object_store_get_object(zobj TSRMLS_CC);


  MS_INIT_COLOR(*(php_color->color), red, green, blue, alpha);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int color.setHex(string hex)
    Set new RGB(A) color from hex string. */
PHP_METHOD(colorObj, setHex)
{
  zval *zobj = getThis();
  char *hex;
  long hex_len = 0, red, green, blue, alpha = 255;
  php_color_object *php_color;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &hex, &hex_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ((hex_len == 7 || hex_len == 9) && hex[0] == '#') {
    red = msHexToInt(hex + 1);
    green = msHexToInt(hex + 3);
    blue = msHexToInt(hex + 5);
    if (hex_len == 9) {
      alpha = msHexToInt(hex + 7);
    }

    if (red > 255 || green > 255 || blue > 255 || alpha > 255) {
      mapscript_throw_exception("Invalid color index." TSRMLS_CC);
      RETURN_LONG(MS_FAILURE);
    }

    php_color = (php_color_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    MS_INIT_COLOR(*(php_color->color), red, green, blue, alpha);

    RETURN_LONG(MS_SUCCESS);
  } else {
    mapscript_throw_exception("Invalid hex color string." TSRMLS_CC);
    RETURN_LONG(MS_FAILURE);
  }
}
/* }}} */
 
/* {{{ proto string color.toHex()
    Get hex string #rrggbb[aa]. */
PHP_METHOD(colorObj, toHex)
{
  char *hex;
  zval *zobj = getThis();
  php_color_object *php_color;
  colorObj *color;

  php_color = (php_color_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  color = php_color->color;

  if (color->red < 0 || color->green < 0 || color->blue < 0) {
    mapscript_throw_exception("Can't express invalid color as hex." TSRMLS_CC);
    return;
  }

  if (color->alpha == 255) {
    hex = msSmallMalloc(8);
    snprintf(hex, 8, "#%02x%02x%02x",
             color->red, color->green, color->blue);
  } else if (color->alpha >= 0) {
    hex = msSmallMalloc(10);
    snprintf(hex, 10, "#%02x%02x%02x%02x",
             color->red, color->green, color->blue, color->alpha);
  } else {
    mapscript_throw_exception("Can't express color with invalid alpha as hex." TSRMLS_CC);
    return;
  }
  
  RETURN_STRINGL(hex, strlen(hex), 0);
}
/* }}} */

zend_function_entry color_functions[] = {
  PHP_ME(colorObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(colorObj, __get, color___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(colorObj, __set, color___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(colorObj, setRGB, color_setRGB_args, ZEND_ACC_PUBLIC)
  PHP_ME(colorObj, setHex, color_setHex_args, ZEND_ACC_PUBLIC)
  PHP_ME(colorObj, toHex, color_toHex_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_color(colorObj *color, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_color_object * php_color;
  object_init_ex(return_value, mapscript_ce_color);
  php_color = (php_color_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_color->color = color;

  php_color->parent = parent;

  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_color_object_destroy(void *object TSRMLS_DC)
{
  php_color_object *php_color = (php_color_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_color);

  MAPSCRIPT_FREE_PARENT(php_color->parent);

  /* We don't need to free the colorObj, the mapObj will do it */

  efree(object);
}

static zend_object_value mapscript_color_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_color_object *php_color;

  MAPSCRIPT_ALLOC_OBJECT(php_color, php_color_object);

  retval = mapscript_object_new(&php_color->std, ce,
                                &mapscript_color_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_color->parent);

  return retval;
}

PHP_MINIT_FUNCTION(color)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("colorObj",
                           color_functions,
                           mapscript_ce_color,
                           mapscript_color_object_new);

  mapscript_ce_color->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}

