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

zend_class_entry *mapscript_ce_referencemap;

ZEND_BEGIN_ARG_INFO_EX(referenceMap___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(referenceMap___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(referenceMap_updateFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

/* {{{ proto referenceMapObj __construct()
   referenceMapObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(referenceMapObj, __construct)
{
  mapscript_throw_exception("referenceMapObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(referenceMapObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_referencemap_object *php_referencemap;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_referencemap = (php_referencemap_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_STRING("image", php_referencemap->referencemap->image)
  else IF_GET_LONG("width", php_referencemap->referencemap->width)
    else IF_GET_LONG("height", php_referencemap->referencemap->height)
      else IF_GET_LONG("status", php_referencemap->referencemap->status)
        else IF_GET_LONG("marker", php_referencemap->referencemap->marker)
          else IF_GET_STRING("markername", php_referencemap->referencemap->markername)
            else IF_GET_LONG("markersize", php_referencemap->referencemap->markersize)
              else IF_GET_LONG("maxboxsize", php_referencemap->referencemap->maxboxsize)
                else IF_GET_LONG("minboxsize", php_referencemap->referencemap->minboxsize)
                  else IF_GET_OBJECT("extent", mapscript_ce_rect, php_referencemap->extent, &php_referencemap->referencemap->extent)
                    else IF_GET_OBJECT("color", mapscript_ce_color, php_referencemap->color, &php_referencemap->referencemap->color)
                      else IF_GET_OBJECT("outlinecolor", mapscript_ce_color, php_referencemap->outlinecolor, &php_referencemap->referencemap->outlinecolor)
                        else {
                          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                        }
}

PHP_METHOD(referenceMapObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_referencemap_object *php_referencemap;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_referencemap = (php_referencemap_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_STRING("image", php_referencemap->referencemap->image, value)
  else IF_SET_LONG("width", php_referencemap->referencemap->width, value)
    else IF_SET_LONG("height", php_referencemap->referencemap->height, value)
      else IF_SET_LONG("status", php_referencemap->referencemap->status, value)
        else IF_SET_LONG("marker", php_referencemap->referencemap->marker, value)
          else IF_SET_STRING("markername", php_referencemap->referencemap->markername, value)
            else IF_SET_LONG("markersize", php_referencemap->referencemap->markersize, value)
              else IF_SET_LONG("maxboxsize", php_referencemap->referencemap->maxboxsize, value)
                else IF_SET_LONG("minboxsize", php_referencemap->referencemap->minboxsize, value)
                  else if ( (STRING_EQUAL("extent", property)) ||
                            (STRING_EQUAL("color", property)) ||
                            (STRING_EQUAL("outlinecolor", property))) {
                    mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                  } else {
                    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                  }
}

/* {{{ proto int referencemap.updateFromString(string snippet)
   Update a referencemap from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(referenceMapObj, updateFromString)
{
  char *snippet;
  long snippet_len = 0;
  zval *zobj = getThis();
  php_referencemap_object *php_referencemap;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_referencemap = (php_referencemap_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status =  referenceMapObj_updateFromString(php_referencemap->referencemap, snippet);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string convertToString()
   Convert the referencemap object to string. */
PHP_METHOD(referenceMapObj, convertToString)
{
  zval *zobj = getThis();
  php_referencemap_object *php_referencemap;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_referencemap = (php_referencemap_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value =  referenceMapObj_convertToString(php_referencemap->referencemap);

  if (value == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int referencemap.free()
   Free the object. */
PHP_METHOD(referenceMapObj, free)
{
  zval *zobj = getThis();
  php_referencemap_object *php_referencemap;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_referencemap = (php_referencemap_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_referencemap->extent);
  MAPSCRIPT_DELREF(php_referencemap->color);
  MAPSCRIPT_DELREF(php_referencemap->outlinecolor);
}
/* }}} */

zend_function_entry referencemap_functions[] = {
  PHP_ME(referenceMapObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(referenceMapObj, __get, referenceMap___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(referenceMapObj, __set, referenceMap___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(referenceMapObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(referenceMapObj, updateFromString, referenceMap_updateFromString_args, ZEND_ACC_PUBLIC)
  PHP_ME(referenceMapObj, convertToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(referenceMapObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_referencemap(referenceMapObj *referencemap, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_referencemap_object * php_referencemap;
  object_init_ex(return_value, mapscript_ce_referencemap);
  php_referencemap = (php_referencemap_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_referencemap->referencemap = referencemap;

  php_referencemap->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_referencemap_object_destroy(void *object TSRMLS_DC)
{
  php_referencemap_object *php_referencemap = (php_referencemap_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_referencemap);

  MAPSCRIPT_FREE_PARENT(php_referencemap->parent);
  MAPSCRIPT_DELREF(php_referencemap->extent);
  MAPSCRIPT_DELREF(php_referencemap->color);
  MAPSCRIPT_DELREF(php_referencemap->outlinecolor);

  /* We don't need to free the referenceMapObj */

  efree(object);
}

static zend_object_value mapscript_referencemap_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_referencemap_object *php_referencemap;

  MAPSCRIPT_ALLOC_OBJECT(php_referencemap, php_referencemap_object);

  retval = mapscript_object_new(&php_referencemap->std, ce,
                                &mapscript_referencemap_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_referencemap->parent);
  php_referencemap->extent = NULL;
  php_referencemap->color = NULL;
  php_referencemap->outlinecolor = NULL;

  return retval;
}

PHP_MINIT_FUNCTION(referencemap)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("referenceMapObj",
                           referencemap_functions,
                           mapscript_ce_referencemap,
                           mapscript_referencemap_object_new);

  mapscript_ce_referencemap->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
