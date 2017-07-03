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

zend_class_entry *mapscript_ce_hashtable;
#if PHP_VERSION_ID >= 70000
zend_object_handlers mapscript_hashtable_object_handlers;
#endif  

ZEND_BEGIN_ARG_INFO_EX(hashtable___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(hashtable___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(hashtable_get_args, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(hashtable_set_args, 0, 0, 2)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(hashtable_remove_args, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(hashtable_nextkey_args, 0, 0, 1)
ZEND_ARG_INFO(0, previousKey)
ZEND_END_ARG_INFO()

/* {{{ proto hashtable __construct()
   Create a new hashtableObj instance. */
PHP_METHOD(hashtableObj, __construct)
{
  mapscript_throw_exception("hashTableObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(hashtableObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_hashtable_object *php_hashtable;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_hashtable = MAPSCRIPT_OBJ_P(php_hashtable_object, zobj);

  IF_GET_LONG("numitems", php_hashtable->hashtable->numitems)
  else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

PHP_METHOD(hashtableObj, __set)
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

  if ( (STRING_EQUAL("numitems", property))) {
    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

/* {{{ proto int hashtable.get(string key)
   Get a value from item by its key. Returns empty string if not found. */
PHP_METHOD(hashtableObj, get)
{
  char *key;
  long key_len = 0;
  zval *zobj = getThis();
  const char *value = NULL;
  php_hashtable_object *php_hashtable;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &key, &key_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_hashtable = MAPSCRIPT_OBJ_P(php_hashtable_object, zobj);

  value = hashTableObj_get(php_hashtable->hashtable, key);
  if (value == NULL) {
    MAPSCRIPT_RETURN_STRING("",1);
  }

  MAPSCRIPT_RETURN_STRING((char *)value, 1);
}
/* }}} */

/* {{{ proto int hashtable.set(string key, string value)
   Set a hash item given key and value. Returns MS_FAILURE on error. */
PHP_METHOD(hashtableObj, set)
{
  char *key, *value;
  long key_len, value_len = 0;
  zval *zobj = getThis();
  int status = MS_FAILURE;
  php_hashtable_object *php_hashtable;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                            &key, &key_len, &value, &value_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_hashtable = MAPSCRIPT_OBJ_P(php_hashtable_object, zobj);

  if ((status = hashTableObj_set(php_hashtable->hashtable, key, value)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int hashtable.remove(string key)
   Remove one item from hash table. Returns MS_FAILURE on error. */
PHP_METHOD(hashtableObj, remove)
{
  char *key;
  long key_len = 0;
  zval *zobj = getThis();
  int status = MS_FAILURE;
  php_hashtable_object *php_hashtable;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &key, &key_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_hashtable = MAPSCRIPT_OBJ_P(php_hashtable_object, zobj);

  if ((status = hashTableObj_remove(php_hashtable->hashtable, key)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int hashtable.clear()
   Clear all items in hash table (to NULL). */
PHP_METHOD(hashtableObj, clear)
{
  zval *zobj = getThis();
  php_hashtable_object *php_hashtable;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_hashtable = MAPSCRIPT_OBJ_P(php_hashtable_object, zobj);

  hashTableObj_clear(php_hashtable->hashtable);
}
/* }}} */

/* {{{ proto int hashtable.nextkey(string previousKey)
   Return the next key or first key if previousKey == NULL.
   Returns NULL if no item is in the hashTable or end of hashTable reached */
PHP_METHOD(hashtableObj, nextKey)
{
  char *key;
  long key_len = 0;
  zval *zobj = getThis();
  const char *value = NULL;
  php_hashtable_object *php_hashtable;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!",
                            &key, &key_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_hashtable = MAPSCRIPT_OBJ_P(php_hashtable_object, zobj);

  value = hashTableObj_nextKey(php_hashtable->hashtable, key);

  if (value == NULL)
    RETURN_NULL();

  MAPSCRIPT_RETURN_STRING(value, 1);
}
/* }}} */

zend_function_entry hashtable_functions[] = {
  PHP_ME(hashtableObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(hashtableObj, __get, hashtable___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(hashtableObj, __set, hashtable___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(hashtableObj, get, hashtable_get_args, ZEND_ACC_PUBLIC)
  PHP_ME(hashtableObj, set, hashtable_set_args, ZEND_ACC_PUBLIC)
  PHP_ME(hashtableObj, remove, hashtable_remove_args, ZEND_ACC_PUBLIC)
  PHP_ME(hashtableObj, clear, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(hashtableObj, nextKey, hashtable_nextkey_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_hashtable(hashTableObj *hashtable, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_hashtable_object * php_hashtable;
  object_init_ex(return_value, mapscript_ce_hashtable);
  php_hashtable = MAPSCRIPT_OBJ_P(php_hashtable_object, return_value);
  php_hashtable->hashtable = hashtable;

  php_hashtable->parent = parent;

  MAPSCRIPT_ADDREF(parent.val);
}

#if PHP_VERSION_ID >= 70000
/* PHP7 - Modification by Bjoern Boldt <mapscript@pixaweb.net> */
static zend_object *mapscript_hashtable_create_object(zend_class_entry *ce TSRMLS_DC)
{
  php_hashtable_object *php_hashtable;

  php_hashtable = ecalloc(1, sizeof(*php_hashtable) + zend_object_properties_size(ce));

  zend_object_std_init(&php_hashtable->zobj, ce TSRMLS_CC);
  object_properties_init(&php_hashtable->zobj, ce);

  php_hashtable->zobj.handlers = &mapscript_hashtable_object_handlers;

  MAPSCRIPT_INIT_PARENT(php_hashtable->parent);

  return &php_hashtable->zobj;
}

static void mapscript_hashtable_free_object(zend_object *object)
{
  php_hashtable_object *php_hashtable;

  php_hashtable = (php_hashtable_object *)((char *)object - XtOffsetOf(php_hashtable_object, zobj));

  MAPSCRIPT_FREE_PARENT(php_hashtable->parent);

  /* We don't need to free the hashTableObj */

  zend_object_std_dtor(object);
}

PHP_MINIT_FUNCTION(hashtable)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "hashTableObj", hashtable_functions);
  mapscript_ce_hashtable = zend_register_internal_class(&ce TSRMLS_CC);

  mapscript_ce_hashtable->create_object = mapscript_hashtable_create_object;
  mapscript_ce_hashtable->ce_flags |= ZEND_ACC_FINAL;

  memcpy(&mapscript_hashtable_object_handlers, &mapscript_std_object_handlers, sizeof(mapscript_hashtable_object_handlers));
  mapscript_hashtable_object_handlers.free_obj = mapscript_hashtable_free_object;
  mapscript_hashtable_object_handlers.offset   = XtOffsetOf(php_hashtable_object, zobj);

  return SUCCESS;
}
#else
/* PHP5 */
static void mapscript_hashtable_object_destroy(void *object TSRMLS_DC)
{
  php_hashtable_object *php_hashtable = (php_hashtable_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_hashtable);

  MAPSCRIPT_FREE_PARENT(php_hashtable->parent);

  /* We don't need to free the hashTableObj */

  efree(object);
}

static zend_object_value mapscript_hashtable_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_hashtable_object *php_hashtable;

  MAPSCRIPT_ALLOC_OBJECT(php_hashtable, php_hashtable_object);

  retval = mapscript_object_new(&php_hashtable->std, ce,
                                &mapscript_hashtable_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_hashtable->parent);

  return retval;
}

PHP_MINIT_FUNCTION(hashtable)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("hashTableObj",
                           hashtable_functions,
                           mapscript_ce_hashtable,
                           mapscript_hashtable_object_new);

  mapscript_ce_hashtable->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
#endif
