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

zend_class_entry *mapscript_ce_legend;

ZEND_BEGIN_ARG_INFO_EX(legend___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(legend___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(legend_updateFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

/* {{{ proto legend __construct()
   legendObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(legendObj, __construct)
{
  mapscript_throw_exception("legendObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(legendObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_legend_object *php_legend;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_legend = (php_legend_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("height", php_legend->legend->height)
  else IF_GET_LONG("width", php_legend->legend->width)
    else IF_GET_LONG("keysizex", php_legend->legend->keysizex)
      else IF_GET_LONG("keysizey", php_legend->legend->keysizey)
        else IF_GET_LONG("keyspacingx", php_legend->legend->keyspacingx)
          else IF_GET_LONG("keyspacingy", php_legend->legend->keyspacingy)
            else IF_GET_LONG("status", php_legend->legend->status)
              else IF_GET_LONG("position", php_legend->legend->position)
                else IF_GET_LONG("postlabelcache", php_legend->legend->postlabelcache)
                  else IF_GET_STRING("template", php_legend->legend->template)
                    else IF_GET_OBJECT("outlinecolor", mapscript_ce_color, php_legend->outlinecolor, &php_legend->legend->outlinecolor)
                      else IF_GET_OBJECT("label", mapscript_ce_label, php_legend->label, &php_legend->legend->label)
                        else IF_GET_OBJECT("imagecolor", mapscript_ce_color, php_legend->imagecolor, &php_legend->legend->imagecolor)
                          else {
                            mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                          }
}

PHP_METHOD(legendObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_legend_object *php_legend;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_legend = (php_legend_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_LONG("height", php_legend->legend->height, value)
  else IF_SET_LONG("width", php_legend->legend->width, value)
    else IF_SET_LONG("keysizex", php_legend->legend->keysizex, value)
      else IF_SET_LONG("keysizey", php_legend->legend->keysizey, value)
        else IF_SET_LONG("keyspacingx", php_legend->legend->keyspacingx, value)
          else IF_SET_LONG("keyspacingy", php_legend->legend->keyspacingy, value)
            else IF_SET_LONG("status", php_legend->legend->status, value)
              else IF_SET_LONG("position", php_legend->legend->position, value)
                else IF_SET_LONG("postlabelcache", php_legend->legend->postlabelcache, value)
                  else IF_SET_STRING("template", php_legend->legend->template, value)
                    else if ( (STRING_EQUAL("outlinecolor", property)) ||
                              (STRING_EQUAL("imagecolor", property)) ||
                              (STRING_EQUAL("label", property))) {
                      mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                    } else {
                      mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                    }
}

/* {{{ proto int legend.updateFromString(string snippet)
   Update a legend from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(legendObj, updateFromString)
{
  char *snippet;
  long snippet_len = 0;
  zval *zobj = getThis();
  php_legend_object *php_legend;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_legend = (php_legend_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status =  legendObj_updateFromString(php_legend->legend, snippet);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string convertToString()
   Convert the legend object to string. */
PHP_METHOD(legendObj, convertToString)
{
  zval *zobj = getThis();
  php_legend_object *php_legend;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_legend = (php_legend_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value =  legendObj_convertToString(php_legend->legend);

  if (value == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int legend.free()
   Free the object */
PHP_METHOD(legendObj, free)
{
  zval *zobj = getThis();
  php_legend_object *php_legend;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_legend = (php_legend_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_legend->outlinecolor);
  MAPSCRIPT_DELREF(php_legend->imagecolor);
  MAPSCRIPT_DELREF(php_legend->label);
}
/* }}} */

zend_function_entry legend_functions[] = {
  PHP_ME(legendObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(legendObj, __get, legend___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(legendObj, __set, legend___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(legendObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(legendObj, convertToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(legendObj, updateFromString, legend_updateFromString_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_legend(legendObj *legend, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_legend_object * php_legend;
  object_init_ex(return_value, mapscript_ce_legend);
  php_legend = (php_legend_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_legend->legend = legend;

  php_legend->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_legend_object_destroy(void *object TSRMLS_DC)
{
  php_legend_object *php_legend = (php_legend_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_legend);

  MAPSCRIPT_FREE_PARENT(php_legend->parent);
  MAPSCRIPT_DELREF(php_legend->outlinecolor);
  MAPSCRIPT_DELREF(php_legend->imagecolor);
  MAPSCRIPT_DELREF(php_legend->label);

  /* We don't need to free the legendObj */

  efree(object);
}

static zend_object_value mapscript_legend_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_legend_object *php_legend;

  MAPSCRIPT_ALLOC_OBJECT(php_legend, php_legend_object);

  retval = mapscript_object_new(&php_legend->std, ce,
                                &mapscript_legend_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_legend->parent);
  php_legend->outlinecolor = NULL;
  php_legend->imagecolor = NULL;
  php_legend->label = NULL;

  return retval;
}

PHP_MINIT_FUNCTION(legend)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("legendObj",
                           legend_functions,
                           mapscript_ce_legend,
                           mapscript_legend_object_new);

  mapscript_ce_legend->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
