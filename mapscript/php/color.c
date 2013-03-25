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

/* {{{ proto int color.setRGB(int R, int G, int B)
    Set new RGB color. */
PHP_METHOD(colorObj, setRGB)
{
  zval *zobj = getThis();
  long red, green, blue;
  php_color_object *php_color;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lll",
                            &red, &green, &blue) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_color = (php_color_object *) zend_object_store_get_object(zobj TSRMLS_CC);


  MS_INIT_COLOR(*(php_color->color), red, green, blue,255);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

zend_function_entry color_functions[] = {
  PHP_ME(colorObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(colorObj, __get, color___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(colorObj, __set, color___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(colorObj, setRGB, color_setRGB_args, ZEND_ACC_PUBLIC) {
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

