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
#include "SAPI.h"
#include "php_variables.h"
#if PHP_VERSION_ID >= 50600
#include "php_streams.h"
#endif

char *owsrequest_getenv(const char *name, void *thread_context);

zend_class_entry *mapscript_ce_owsrequest;

ZEND_BEGIN_ARG_INFO_EX(owsrequest___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(owsrequest___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(owsrequest_setParameter_args, 0, 0, 2)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(owsrequest_addParameter_args, 0, 0, 2)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(owsrequest_getName_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(owsrequest_getValue_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(owsrequest_getValueByName_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

/* {{{ proto owsrequest __construct()
   Create a new OWSRequestObj instance. */
PHP_METHOD(OWSRequestObj, __construct)
{
  zval *zobj = getThis();
  php_owsrequest_object *php_owsrequest;
  cgiRequestObj *request;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((request = cgirequestObj_new()) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  php_owsrequest->cgirequest = request;
}
/* }}} */

PHP_METHOD(OWSRequestObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_owsrequest_object *php_owsrequest;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);


  IF_GET_LONG("numparams", php_owsrequest->cgirequest->NumParams)
  else IF_GET_STRING("contenttype", php_owsrequest->cgirequest->contenttype)
    else IF_GET_STRING("postrequest", php_owsrequest->cgirequest->postrequest)
      else IF_GET_STRING("httpcookiedata", php_owsrequest->cgirequest->httpcookiedata)
        else IF_GET_LONG("type", php_owsrequest->cgirequest->type)
          else {
            mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
          }
}

PHP_METHOD(OWSRequestObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ( (STRING_EQUAL("numparams", property)) ||
       (STRING_EQUAL("type", property)) ||
       (STRING_EQUAL("contenttype", property)) ||
       (STRING_EQUAL("postrequest", property)) ||
       (STRING_EQUAL("httpcookiedata", property))) {
    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

/* {{{ proto int owsrequest.loadParams()
   Initializes the OWSRequest object from the cgi environment variables
   REQUEST_METHOD, QUERY_STRING and HTTP_COOKIE. Returns the number of
   name/value pairs collected. */
PHP_METHOD(OWSRequestObj, loadParams)
{
  zval *zobj = getThis();
  zval **val;
  php_owsrequest_object *php_owsrequest;
  void *thread_context;

#ifdef ZTS
  thread_context = (void*)TSRMLS_C;
#else
  thread_context = NULL;
#endif

  //PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    //PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  //PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ( (STRING_EQUAL(sapi_module.name,"cli")) ||
       (STRING_EQUAL(sapi_module.name,"cgi")) ||
       (STRING_EQUAL(sapi_module.name,"cgi-fcgi")) ) {
    cgirequestObj_loadParams(php_owsrequest->cgirequest, NULL, NULL, 0, thread_context);
  } else {
    // check if we have input data for GET method
    if (SG(request_info).request_method &&
        STRING_EQUAL(SG(request_info).request_method, "GET")) {
      zend_is_auto_global("_SERVER", sizeof("_SERVER")-1 TSRMLS_CC);
      if ( PG(http_globals)[TRACK_VARS_SERVER] &&
           (zend_hash_find(PG(http_globals)[TRACK_VARS_SERVER]->value.ht, "QUERY_STRING", sizeof("QUERY_STRING"), (void **) &val) == SUCCESS) &&
           (Z_TYPE_PP(val) == IS_STRING) &&
           (Z_STRLEN_PP(val) > 0) ) {
        cgirequestObj_loadParams(php_owsrequest->cgirequest, owsrequest_getenv, NULL, 0, thread_context);
      }
    } else {
#if PHP_VERSION_ID >= 50600
      php_stream *s = php_stream_temp_new();
      php_stream *input = php_stream_open_wrapper("php://input", "r", 0, NULL);

      /* php://input does not support stat */
      php_stream_copy_to_stream_ex(input, s, -1, NULL);
      php_stream_close(input);

      php_stream_rewind(s);
      
      char *raw_post_data = NULL;
      long raw_post_data_length = 0;

      raw_post_data_length = php_stream_copy_to_mem(s, raw_post_data, -1, 0);

      cgirequestObj_loadParams(php_owsrequest->cgirequest, owsrequest_getenv,
                               raw_post_data,
                               raw_post_data_length, thread_context);
#else
      cgirequestObj_loadParams(php_owsrequest->cgirequest, owsrequest_getenv,
                               SG(request_info).raw_post_data,
                               SG(request_info).raw_post_data_length, thread_context);
#endif
    }
  }

  RETURN_LONG(php_owsrequest->cgirequest->NumParams);
}
/* }}} */

/* {{{ proto int owsrequest.setParameter(string name, string value)
   Set a request parameter. */
PHP_METHOD(OWSRequestObj, setParameter)
{
  char *name;
  long name_len = 0;
  char *value;
  long value_len = 0;
  zval *zobj = getThis();
  php_owsrequest_object *php_owsrequest;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                            &name, &name_len, &value, &value_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  cgirequestObj_setParameter(php_owsrequest->cgirequest, name, value);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int owsrequest.addParameter(string name, string value)
   Add a request parameter. */
PHP_METHOD(OWSRequestObj, addParameter)
{
  char *name;
  long name_len = 0;
  char *value;
  long value_len = 0;
  zval *zobj = getThis();
  php_owsrequest_object *php_owsrequest;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                            &name, &name_len, &value, &value_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  cgirequestObj_addParameter(php_owsrequest->cgirequest, name, value);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto string owsrequest.getName(int index)
   Return the name of the parameter at index in the request’s array of parameter names. */
PHP_METHOD(OWSRequestObj, getName)
{
  long index;
  zval *zobj = getThis();
  char *value = NULL;
  php_owsrequest_object *php_owsrequest;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = cgirequestObj_getName(php_owsrequest->cgirequest, index);
  if (!value)
    RETURN_STRING("", 1);

  RETURN_STRING(value,1);
}
/* }}} */

/* {{{ proto string owsrequest.getValue(int index)
   Return the value of the parameter at index in the request’s array of parameter values.*/
PHP_METHOD(OWSRequestObj, getValue)
{
  long index;
  zval *zobj = getThis();
  char *value = NULL;
  php_owsrequest_object *php_owsrequest;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = cgirequestObj_getValue(php_owsrequest->cgirequest, index);
  if (!value)
    RETURN_STRING("", 1);

  RETURN_STRING(value,1);
}
/* }}} */

/* {{{ proto string owsrequest.getValueByName(string name)
   Return the value associated with the parameter name.*/
PHP_METHOD(OWSRequestObj, getValueByName)
{
  char *name;
  long name_len = 0;
  zval *zobj = getThis();
  char *value = NULL;
  php_owsrequest_object *php_owsrequest;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &name, &name_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_owsrequest = (php_owsrequest_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = cgirequestObj_getValueByName(php_owsrequest->cgirequest, name);
  if (!value)
    RETURN_STRING("", 1);

  RETURN_STRING(value,1);
}
/* }}} */

zend_function_entry owsrequest_functions[] = {
  PHP_ME(OWSRequestObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(OWSRequestObj, __get, owsrequest___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(OWSRequestObj, __set, owsrequest___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(OWSRequestObj, loadParams, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(OWSRequestObj, setParameter, owsrequest_setParameter_args, ZEND_ACC_PUBLIC)
  PHP_ME(OWSRequestObj, addParameter, owsrequest_addParameter_args, ZEND_ACC_PUBLIC)
  PHP_ME(OWSRequestObj, getName, owsrequest_getName_args, ZEND_ACC_PUBLIC)
  PHP_ME(OWSRequestObj, getValue, owsrequest_getValue_args, ZEND_ACC_PUBLIC)
  PHP_ME(OWSRequestObj, getValueByName, owsrequest_getValueByName_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

char *owsrequest_getenv(const char *name, void *thread_context)
{
  zval **val, **ppzval;
  zval *cookie_result, *key;
  HashTable *cookies;
  char *string_key = NULL, *cookie_tmp;
  ulong num_key;
  int i = 0;
  TSRMLS_FETCH_FROM_CTX(thread_context);

  if  (STRING_EQUAL(name, "HTTP_COOKIE")) {
    cookies = PG(http_globals)[TRACK_VARS_COOKIE]->value.ht;
    MAKE_STD_ZVAL(cookie_result);
    ZVAL_STRING(cookie_result, "",1);
    for(zend_hash_internal_pointer_reset(cookies);
        zend_hash_has_more_elements(cookies) == SUCCESS;
        zend_hash_move_forward(cookies), ++i) {
      zend_hash_get_current_data(cookies, (void **)&ppzval);
      zend_hash_get_current_key(cookies, &string_key, &num_key, 1);
      cookie_tmp = malloc((strlen(string_key)+Z_STRLEN_PP(ppzval)+3) * sizeof(char));
      sprintf(cookie_tmp, "%s=%s;",string_key,Z_STRVAL_PP(ppzval));
      MAKE_STD_ZVAL(key);
      ZVAL_STRING(key, cookie_tmp,1);
      add_string_to_string(cookie_result,cookie_result, key);
      zval_dtor(key);
      free(cookie_tmp);
    }
    return Z_STRVAL_P(cookie_result);
  } else {
    zend_is_auto_global("_SERVER", sizeof("_SERVER")-1 TSRMLS_CC);
    if ( PG(http_globals)[TRACK_VARS_SERVER] &&
         (zend_hash_find(PG(http_globals)[TRACK_VARS_SERVER]->value.ht, name, strlen(name)+1, (void **) &val) == SUCCESS) &&
         (Z_TYPE_PP(val) == IS_STRING)) {
      return Z_STRVAL_PP(val);
    }
  }

  return NULL;
}

void mapscript_create_owsrequest(cgiRequestObj *cgirequest, zval *return_value TSRMLS_DC)
{
  php_owsrequest_object * php_owsrequest;
  object_init_ex(return_value, mapscript_ce_owsrequest);
  php_owsrequest = (php_owsrequest_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_owsrequest->cgirequest = cgirequest;
}

static void mapscript_owsrequest_object_destroy(void *object TSRMLS_DC)
{
  php_owsrequest_object *php_owsrequest = (php_owsrequest_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_owsrequest);

  cgirequestObj_destroy(php_owsrequest->cgirequest);

  efree(object);
}

static zend_object_value mapscript_owsrequest_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_owsrequest_object *php_owsrequest;

  MAPSCRIPT_ALLOC_OBJECT(php_owsrequest, php_owsrequest_object);

  retval = mapscript_object_new(&php_owsrequest->std, ce,
                                &mapscript_owsrequest_object_destroy TSRMLS_CC);
  return retval;
}

PHP_MINIT_FUNCTION(owsrequest)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("OWSRequestObj",
                           owsrequest_functions,
                           mapscript_ce_owsrequest,
                           mapscript_owsrequest_object_new);

  mapscript_ce_owsrequest->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}

