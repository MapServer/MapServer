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

zend_class_entry *mapscript_ce_result;

ZEND_BEGIN_ARG_INFO_EX(result___construct_args, 0, 0, 1)
ZEND_ARG_INFO(0, shapeindex)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(result___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(result___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()


/* {{{ proto resultObj __construct()
   Create a new resultObj instance */
PHP_METHOD(resultObj, __construct)
{
  long shapeindex;
  php_result_object *php_result;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &shapeindex) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_result = (php_result_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

  if ((php_result->result = resultObj_new()) == NULL) {
    mapscript_throw_exception("Unable to construct resultObj." TSRMLS_CC);
    return;
  }

  php_result->result->shapeindex = shapeindex;
}
/* }}} */

PHP_METHOD(resultObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_result_object *php_result;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_result = (php_result_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("shapeindex", php_result->result->shapeindex)
  else IF_GET_LONG("tileindex", php_result->result->tileindex)
    else IF_GET_LONG("classindex", php_result->result->classindex)
      else IF_GET_LONG("resultindex", php_result->result->resultindex)
        else {
          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
        }
}

PHP_METHOD(resultObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_result_object *php_result;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_result = (php_result_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ( (STRING_EQUAL("shapeindex", property)) ||
       (STRING_EQUAL("tileindex", property)) ||
       (STRING_EQUAL("resultindex", property)) ||
       (STRING_EQUAL("classindex", property))) {
    mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

zend_function_entry result_functions[] = {
  PHP_ME(resultObj, __construct, result___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(resultObj, __get, result___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(resultObj, __set, result___set_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_result(resultObj *result, parent_object parent,
                             zval *return_value TSRMLS_DC)
{
  php_result_object * php_result;
  object_init_ex(return_value, mapscript_ce_result);
  php_result = (php_result_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_result->result = result;

  php_result->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_result_object_destroy(void *object TSRMLS_DC)
{
  php_result_object *php_result = (php_result_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_result);

  MAPSCRIPT_FREE_PARENT(php_result->parent);

  /* We don't need to free the resultObj */

  efree(object);
}

static zend_object_value mapscript_result_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_result_object *php_result;

  MAPSCRIPT_ALLOC_OBJECT(php_result, php_result_object);

  retval = mapscript_object_new(&php_result->std, ce,
                                &mapscript_result_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_result->parent);

  return retval;
}

PHP_MINIT_FUNCTION(result)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("resultObj",
                           result_functions,
                           mapscript_ce_result,
                           mapscript_result_object_new);

  mapscript_ce_result->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
