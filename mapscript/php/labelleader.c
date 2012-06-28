/**********************************************************************
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

zend_class_entry *mapscript_ce_labelleader;

ZEND_BEGIN_ARG_INFO_EX(labelleader___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(labelleader___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto labelleader __construct()
   labelLeaderObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(labelLeaderObj, __construct)
{
  mapscript_throw_exception("labelLeaderObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(labelLeaderObj, __get)
{
  char *property;
  long property_len;
  zval *zobj = getThis();
  php_labelleader_object *php_labelleader;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_labelleader = (php_labelleader_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("maxdistance", php_labelleader->labelleader->maxdistance)
  else IF_GET_LONG("gridstep", php_labelleader->labelleader->gridstep)
    else {
      mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
    }
}

PHP_METHOD(labelLeaderObj, __set)
{
  char *property;
  long property_len;
  zval *value;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ( (STRING_EQUAL("maxdistance", property)) ||
       (STRING_EQUAL("gridstep", property)) ) {
    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

zend_function_entry labelleader_functions[] = {
  PHP_ME(labelLeaderObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(labelLeaderObj, __get, labelleader___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelLeaderObj, __set, labelleader___set_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_labelleader(labelLeaderObj *labelleader, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_labelleader_object * php_labelleader;
  object_init_ex(return_value, mapscript_ce_labelleader);
  php_labelleader = (php_labelleader_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_labelleader->labelleader = labelleader;

  php_labelleader->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_labelleader_object_destroy(void *object TSRMLS_DC)
{
  php_labelleader_object *php_labelleader = (php_labelleader_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_labelleader);

  MAPSCRIPT_FREE_PARENT(php_labelleader->parent);

  /* We don't need to free the labelLeaderObj */

  efree(object);
}

static zend_object_value mapscript_labelleader_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_labelleader_object *php_labelleader;

  MAPSCRIPT_ALLOC_OBJECT(php_labelleader, php_labelleader_object);

  retval = mapscript_object_new(&php_labelleader->std, ce,
                                &mapscript_labelleader_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_labelleader->parent);

  return retval;
}

PHP_MINIT_FUNCTION(labelleader)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("labelLeaderObj",
                           labelleader_functions,
                           mapscript_ce_labelleader,
                           mapscript_labelleader_object_new);

  mapscript_ce_labelleader->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
