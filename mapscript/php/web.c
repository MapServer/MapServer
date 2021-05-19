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

zend_class_entry *mapscript_ce_web;
#if PHP_VERSION_ID >= 70000
zend_object_handlers mapscript_web_object_handlers;
#endif  

ZEND_BEGIN_ARG_INFO_EX(web___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(web___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(web_updateFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

/* {{{ proto web __construct()
   webObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(webObj, __construct)
{
  mapscript_throw_exception("webObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(webObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_web_object *php_web;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_web = MAPSCRIPT_OBJ_P(php_web_object, zobj);

  IF_GET_STRING("imagepath", php_web->web->imagepath)
    else IF_GET_STRING("template", php_web->web->template)
      else IF_GET_STRING("imageurl", php_web->web->imageurl)
        else IF_GET_STRING("temppath", php_web->web->temppath)
          else IF_GET_STRING("header", php_web->web->header)
            else IF_GET_STRING("footer", php_web->web->footer)
              else IF_GET_STRING("empty", php_web->web->empty)
                else IF_GET_STRING("error", php_web->web->error)
                  else IF_GET_STRING("mintemplate", php_web->web->mintemplate)
                    else IF_GET_STRING("maxtemplate", php_web->web->maxtemplate)
                      else IF_GET_DOUBLE("minscaledenom", php_web->web->minscaledenom)
                        else IF_GET_DOUBLE("maxscaledenom", php_web->web->maxscaledenom)
                          else IF_GET_STRING("queryformat", php_web->web->queryformat)
                            else IF_GET_STRING("legendformat", php_web->web->legendformat)
                              else IF_GET_STRING("browseformat", php_web->web->browseformat)
                                else IF_GET_OBJECT("extent", mapscript_ce_rect, php_web->extent, &php_web->web->extent)
                                  else IF_GET_OBJECT("metadata", mapscript_ce_hashtable, php_web->metadata, &php_web->web->metadata)
                                    else IF_GET_OBJECT("validation", mapscript_ce_hashtable, php_web->validation, &php_web->web->validation)
                                      else {
                                        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                      }
}

PHP_METHOD(webObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_web_object *php_web;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_web = MAPSCRIPT_OBJ_P(php_web_object, zobj);

  IF_SET_STRING("imagepath", php_web->web->imagepath, value)
    else IF_SET_STRING("template", php_web->web->template, value)
      else IF_SET_STRING("imageurl", php_web->web->imageurl, value)
        else IF_SET_STRING("temppath", php_web->web->temppath, value)
          else IF_SET_STRING("header", php_web->web->header, value)
            else IF_SET_STRING("footer", php_web->web->footer, value)
              else IF_SET_STRING("mintemplate", php_web->web->mintemplate, value)
                else IF_SET_STRING("maxtemplate", php_web->web->maxtemplate, value)
                  else IF_SET_DOUBLE("minscaledenom", php_web->web->minscaledenom, value)
                    else IF_SET_DOUBLE("maxscaledenom", php_web->web->maxscaledenom, value)
                      else IF_SET_STRING("queryformat", php_web->web->queryformat, value)
                        else IF_SET_STRING("legendformat", php_web->web->legendformat, value)
                          else IF_SET_STRING("browseformat", php_web->web->browseformat, value)
                            else if ( (STRING_EQUAL("empty", property)) ||
                                      (STRING_EQUAL("error", property)) ||
                                      (STRING_EQUAL("extent", property))) {
                              mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
                            } else if ( (STRING_EQUAL("metadata", property)) ||
                                        (STRING_EQUAL("validation", property)) ) {
                              mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                            } else {
                              mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                            }
}

/* {{{ proto int web.updateFromString(string snippet)
   Update a web from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(webObj, updateFromString)
{
  char *snippet;
  long snippet_len = 0;
  zval *zobj = getThis();
  php_web_object *php_web;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_web = MAPSCRIPT_OBJ_P(php_web_object, zobj);

  status =  webObj_updateFromString(php_web->web, snippet);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string convertToString()
   Convert the web object to string. */
PHP_METHOD(webObj, convertToString)
{
  zval *zobj = getThis();
  php_web_object *php_web;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_web = MAPSCRIPT_OBJ_P(php_web_object, zobj);

  value =  webObj_convertToString(php_web->web);

  if (value == NULL)
    MAPSCRIPT_RETURN_STRING("", 1);

  MAPSCRIPT_RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int web.free()
   Free the object. */
PHP_METHOD(webObj, free)
{
  zval *zobj = getThis();
  php_web_object *php_web;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_web = MAPSCRIPT_OBJ_P(php_web_object, zobj);

  MAPSCRIPT_DELREF(php_web->extent);
  MAPSCRIPT_DELREF(php_web->metadata);
  MAPSCRIPT_DELREF(php_web->validation);
}
/* }}} */

zend_function_entry web_functions[] = {
  PHP_ME(webObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(webObj, __get, web___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(webObj, __set, web___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(webObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(webObj, updateFromString, web_updateFromString_args, ZEND_ACC_PUBLIC)
  PHP_ME(webObj, convertToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(webObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_web(webObj *web, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_web_object * php_web;
  object_init_ex(return_value, mapscript_ce_web);
  php_web = MAPSCRIPT_OBJ_P(php_web_object, return_value);
  php_web->web = web;

  php_web->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

#if PHP_VERSION_ID >= 70000
/* PHP7 - Modification by Bjoern Boldt <mapscript@pixaweb.net> */
static zend_object *mapscript_web_create_object(zend_class_entry *ce TSRMLS_DC)
{
  php_web_object *php_web;

  php_web = ecalloc(1, sizeof(*php_web) + zend_object_properties_size(ce));

  zend_object_std_init(&php_web->zobj, ce TSRMLS_CC);
  object_properties_init(&php_web->zobj, ce);

  php_web->zobj.handlers = &mapscript_web_object_handlers;

  MAPSCRIPT_INIT_PARENT(php_web->parent);
  ZVAL_UNDEF(&php_web->extent);
  ZVAL_UNDEF(&php_web->metadata);
  ZVAL_UNDEF(&php_web->validation);

  return &php_web->zobj;
}

static void mapscript_web_free_object(zend_object *object)
{
  php_web_object *php_web;

  php_web = (php_web_object *)((char *)object - XtOffsetOf(php_web_object, zobj));

  MAPSCRIPT_FREE_PARENT(php_web->parent);
  MAPSCRIPT_DELREF(php_web->extent);
  MAPSCRIPT_DELREF(php_web->metadata);
  MAPSCRIPT_DELREF(php_web->validation);

  /* We don't need to free the webObj */

  zend_object_std_dtor(object);
}

PHP_MINIT_FUNCTION(web)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "webObj", web_functions);
  mapscript_ce_web = zend_register_internal_class(&ce TSRMLS_CC);

  mapscript_ce_web->create_object = mapscript_web_create_object;
  mapscript_ce_web->ce_flags |= ZEND_ACC_FINAL;

  memcpy(&mapscript_web_object_handlers, &mapscript_std_object_handlers, sizeof(mapscript_web_object_handlers));
  mapscript_web_object_handlers.free_obj = mapscript_web_free_object;
  mapscript_web_object_handlers.offset   = XtOffsetOf(php_web_object, zobj);

  return SUCCESS;
}
#else
/* PHP5 */
static void mapscript_web_object_destroy(void *object TSRMLS_DC)
{
  php_web_object *php_web = (php_web_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_web);

  MAPSCRIPT_FREE_PARENT(php_web->parent);
  MAPSCRIPT_DELREF(php_web->extent);
  MAPSCRIPT_DELREF(php_web->metadata);
  MAPSCRIPT_DELREF(php_web->validation);

  /* We don't need to free the webObj */

  efree(object);
}

static zend_object_value mapscript_web_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_web_object *php_web;

  MAPSCRIPT_ALLOC_OBJECT(php_web, php_web_object);

  retval = mapscript_object_new(&php_web->std, ce,
                                &mapscript_web_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_web->parent);
  php_web->extent = NULL;
  php_web->metadata = NULL;
  php_web->validation = NULL;

  return retval;
}

PHP_MINIT_FUNCTION(web)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("webObj",
                           web_functions,
                           mapscript_ce_web,
                           mapscript_web_object_new);

  mapscript_ce_web->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
#endif
