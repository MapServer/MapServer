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

zend_class_entry *mapscript_ce_error;

ZEND_BEGIN_ARG_INFO_EX(error___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(error___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto error __construct()
   errorObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(errorObj, __construct)
{
  mapscript_throw_exception("errorObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(errorObj, __get)
{
  char *property;
  long property_len;
  zval *zobj = getThis();
  php_error_object *php_error;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_error = (php_error_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("code", php_error->error->code)
  else IF_GET_STRING("routine", php_error->error->routine)
    else IF_GET_STRING("message", php_error->error->message)
      else IF_GET_LONG("isreported", php_error->error->isreported)
        else {
          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
        }
}

PHP_METHOD(errorObj, __set)
{
  char *property;
  long property_len;
  zval *value;
  zval *zobj = getThis();
  php_error_object *php_error;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_error = (php_error_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ( (STRING_EQUAL("code", property)) ||
       (STRING_EQUAL("routine", property)) ||
       (STRING_EQUAL("isreported", property)) ||
       (STRING_EQUAL("message", property))) {
    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

/* {{{ proto int error.next()
   Returns a ref to the next errorObj in the list, or NULL if we reached the last one */
PHP_METHOD(errorObj, next)
{
  zval *zobj = getThis();
  php_error_object *php_error;
  errorObj *error = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_error = (php_error_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (php_error->error->next == NULL)
    RETURN_NULL();

  /* Make sure 'self' is still valid.  It may have been deleted by
   * msResetErrorList() */
  error = msGetErrorObj();
  while(error != php_error->error) {
    if (error->next == NULL) {
      mapscript_throw_exception("Trying to access an errorObj that has expired." TSRMLS_CC);
      return;
    }
    error = error->next;
  }

  php_error->error = php_error->error->next;
  *return_value = *zobj;
  zval_copy_ctor(return_value);
  INIT_PZVAL(return_value);
}
/* }}} */

zend_function_entry error_functions[] = {
  PHP_ME(errorObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(errorObj, __get, error___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(errorObj, __set, error___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(errorObj, next, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_error(errorObj *error, zval *return_value TSRMLS_DC)
{
  php_error_object * php_error;
  object_init_ex(return_value, mapscript_ce_error);
  php_error = (php_error_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_error->error = error;
}

static void mapscript_error_object_destroy(void *object TSRMLS_DC)
{
  php_error_object *php_error = (php_error_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_error);

  /* We don't need to free the errorObj */

  efree(object);
}

static zend_object_value mapscript_error_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_error_object *php_error;

  MAPSCRIPT_ALLOC_OBJECT(php_error, php_error_object);

  retval = mapscript_object_new(&php_error->std, ce,
                                &mapscript_error_object_destroy TSRMLS_CC);

  return retval;
}

PHP_MINIT_FUNCTION(error)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("errorObj",
                           error_functions,
                           mapscript_ce_error,
                           mapscript_error_object_new);

  mapscript_ce_error->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
