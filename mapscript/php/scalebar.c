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

zend_class_entry *mapscript_ce_scalebar;

ZEND_BEGIN_ARG_INFO_EX(scalebar___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(scalebar___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(scalebar_updateFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(scalebar_setImageColor_args, 0, 0, 3)
ZEND_ARG_INFO(0, red)
ZEND_ARG_INFO(0, green)
ZEND_ARG_INFO(0, blue)
ZEND_END_ARG_INFO()

/* {{{ proto scalebar __construct()
   scalebarObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(scalebarObj, __construct)
{
  mapscript_throw_exception("scalebarObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(scalebarObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_scalebar_object *php_scalebar;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_scalebar = (php_scalebar_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("height", php_scalebar->scalebar->height)
  else IF_GET_LONG("width", php_scalebar->scalebar->width)
    else IF_GET_LONG("style", php_scalebar->scalebar->style)
      else IF_GET_LONG("intervals", php_scalebar->scalebar->intervals)
        else IF_GET_LONG("units", php_scalebar->scalebar->units)
          else IF_GET_LONG("status", php_scalebar->scalebar->status)
            else IF_GET_LONG("position", php_scalebar->scalebar->position)
              else IF_GET_LONG("postlabelcache", php_scalebar->scalebar->postlabelcache)
                else IF_GET_LONG("align", php_scalebar->scalebar->align)
                  else IF_GET_OBJECT("color", mapscript_ce_color, php_scalebar->color, &php_scalebar->scalebar->color)
                    else IF_GET_OBJECT("backgroundcolor", mapscript_ce_color, php_scalebar->backgroundcolor, &php_scalebar->scalebar->backgroundcolor)
                      else IF_GET_OBJECT("outlinecolor", mapscript_ce_color, php_scalebar->outlinecolor, &php_scalebar->scalebar->outlinecolor)
                        else IF_GET_OBJECT("label", mapscript_ce_label, php_scalebar->label, &php_scalebar->scalebar->label)
                          else IF_GET_OBJECT("imagecolor", mapscript_ce_color, php_scalebar->imagecolor, &php_scalebar->scalebar->imagecolor)
                            else {
                              mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                            }
}

PHP_METHOD(scalebarObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_scalebar_object *php_scalebar;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_scalebar = (php_scalebar_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_LONG("height", php_scalebar->scalebar->height, value)
  else IF_SET_LONG("width", php_scalebar->scalebar->width, value)
    else IF_SET_LONG("style", php_scalebar->scalebar->style, value)
      else IF_SET_LONG("intervals", php_scalebar->scalebar->intervals, value)
        else IF_SET_LONG("units", php_scalebar->scalebar->units, value)
          else IF_SET_LONG("status", php_scalebar->scalebar->status, value)
            else IF_SET_LONG("position", php_scalebar->scalebar->position, value)
              else IF_SET_LONG("postlabelcache", php_scalebar->scalebar->postlabelcache, value)
                else IF_SET_LONG("align", php_scalebar->scalebar->align, value)
                  else if ( (STRING_EQUAL("color", property)) ||
                            (STRING_EQUAL("backgroundcolor", property)) ||
                            (STRING_EQUAL("outlinecolor", property)) ||
                            (STRING_EQUAL("label", property)) ||
                            (STRING_EQUAL("imagecolor", property))) {
                    mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                  } else {
                    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                  }
}

/* {{{ proto int scalebar.updateFromString(string snippet)
   Update a scalebar from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(scalebarObj, updateFromString)
{
  char *snippet;
  long snippet_len = 0;
  zval *zobj = getThis();
  php_scalebar_object *php_scalebar;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_scalebar = (php_scalebar_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status =  scalebarObj_updateFromString(php_scalebar->scalebar, snippet);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string convertToString()
   Convert the scalebar object to string. */
PHP_METHOD(scalebarObj, convertToString)
{
  zval *zobj = getThis();
  php_scalebar_object *php_scalebar;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_scalebar = (php_scalebar_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value =  scalebarObj_convertToString(php_scalebar->scalebar);

  if (value == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int scalebar.setImageColor(int red, int green, int blue)
   Set the imagecolor property of the scalebar. Returns -1 on error. */
PHP_METHOD(scalebarObj, setImageColor)
{
  zval *zobj = getThis();
  long red, green, blue;
  php_scalebar_object *php_scalebar;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lll",
                            &red, &green, &blue) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_scalebar = (php_scalebar_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
    RETURN_LONG(MS_FAILURE);

  php_scalebar->scalebar->imagecolor.red = red;
  php_scalebar->scalebar->imagecolor.green = green;
  php_scalebar->scalebar->imagecolor.blue = blue;

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int scalebar.free()
   Free the object */
PHP_METHOD(scalebarObj, free)
{
  zval *zobj = getThis();
  php_scalebar_object *php_scalebar;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_scalebar = (php_scalebar_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_scalebar->color);
  MAPSCRIPT_DELREF(php_scalebar->backgroundcolor);
  MAPSCRIPT_DELREF(php_scalebar->outlinecolor);
  MAPSCRIPT_DELREF(php_scalebar->imagecolor);
  MAPSCRIPT_DELREF(php_scalebar->label);
}
/* }}} */

zend_function_entry scalebar_functions[] = {
  PHP_ME(scalebarObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(scalebarObj, __get, scalebar___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(scalebarObj, __set, scalebar___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(scalebarObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(scalebarObj, updateFromString, scalebar_updateFromString_args, ZEND_ACC_PUBLIC)
  PHP_ME(scalebarObj, convertToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(scalebarObj, setImageColor, scalebar_setImageColor_args, ZEND_ACC_PUBLIC)
  PHP_ME(scalebarObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_scalebar(scalebarObj *scalebar, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_scalebar_object * php_scalebar;
  object_init_ex(return_value, mapscript_ce_scalebar);
  php_scalebar = (php_scalebar_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_scalebar->scalebar = scalebar;

  php_scalebar->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_scalebar_object_destroy(void *object TSRMLS_DC)
{
  php_scalebar_object *php_scalebar = (php_scalebar_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_scalebar);

  MAPSCRIPT_FREE_PARENT(php_scalebar->parent);
  MAPSCRIPT_DELREF(php_scalebar->color);
  MAPSCRIPT_DELREF(php_scalebar->backgroundcolor);
  MAPSCRIPT_DELREF(php_scalebar->outlinecolor);
  MAPSCRIPT_DELREF(php_scalebar->imagecolor);
  MAPSCRIPT_DELREF(php_scalebar->label);

  /* We don't need to free the scalebarObj */

  efree(object);
}

static zend_object_value mapscript_scalebar_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_scalebar_object *php_scalebar;

  MAPSCRIPT_ALLOC_OBJECT(php_scalebar, php_scalebar_object);

  retval = mapscript_object_new(&php_scalebar->std, ce,
                                &mapscript_scalebar_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_scalebar->parent);
  php_scalebar->color = NULL;
  php_scalebar->backgroundcolor = NULL;
  php_scalebar->outlinecolor = NULL;
  php_scalebar->imagecolor = NULL;
  php_scalebar->label = NULL;

  return retval;
}

PHP_MINIT_FUNCTION(scalebar)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("scalebarObj",
                           scalebar_functions,
                           mapscript_ce_scalebar,
                           mapscript_scalebar_object_new);

  mapscript_ce_scalebar->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
