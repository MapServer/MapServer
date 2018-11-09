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

zend_class_entry *mapscript_ce_labelcache;
#if PHP_VERSION_ID >= 70000
zend_object_handlers mapscript_labelcache_object_handlers;
#endif  

ZEND_BEGIN_ARG_INFO_EX(labelcache___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(labelcache___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto labelcache __construct()
   labelCacheObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(labelCacheObj, __construct)
{
  mapscript_throw_exception("labelCacheObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(labelCacheObj, __get)
{
  mapscript_throw_exception("labelCacheObj has no property." TSRMLS_CC);
}

PHP_METHOD(labelCacheObj, __set)
{
  mapscript_throw_exception("labelCacheObj has no property." TSRMLS_CC);
}

/* {{{ proto int labelcache->freeCache(()
   Free the labelcache object in the map. Returns true on success or false if
   an error occurs. */
PHP_METHOD(labelCacheObj, freeCache)
{
  zval *zobj = getThis();
  php_labelcache_object *php_labelcache;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_labelcache = MAPSCRIPT_OBJ_P(php_labelcache_object, zobj);

  labelCacheObj_freeCache(php_labelcache->labelcache);

  RETURN_LONG(MS_SUCCESS);
}
/* }}}} */

zend_function_entry labelcache_functions[] = {
  PHP_ME(labelCacheObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(labelCacheObj, __get, labelcache___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelCacheObj, __set, labelcache___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelCacheObj, freeCache, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_labelcache(labelCacheObj *labelcache, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_labelcache_object * php_labelcache;
  object_init_ex(return_value, mapscript_ce_labelcache);
  php_labelcache = MAPSCRIPT_OBJ_P(php_labelcache_object, return_value);
  php_labelcache->labelcache = labelcache;

  php_labelcache->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}


#if PHP_VERSION_ID >= 70000
/* PHP7 - Modification by Bjoern Boldt <mapscript@pixaweb.net> */
static zend_object *mapscript_labelcache_create_object(zend_class_entry *ce TSRMLS_DC)
{
  php_labelcache_object *php_labelcache;

  php_labelcache = ecalloc(1, sizeof(*php_labelcache) + zend_object_properties_size(ce));

  zend_object_std_init(&php_labelcache->zobj, ce TSRMLS_CC);
  object_properties_init(&php_labelcache->zobj, ce);

  php_labelcache->zobj.handlers = &mapscript_labelcache_object_handlers;

  MAPSCRIPT_INIT_PARENT(php_labelcache->parent);

  return &php_labelcache->zobj;
}

static void mapscript_labelcache_free_object(zend_object *object)
{
  php_labelcache_object *php_labelcache;

  php_labelcache = (php_labelcache_object *)((char *)object - XtOffsetOf(php_labelcache_object, zobj));

  MAPSCRIPT_FREE_PARENT(php_labelcache->parent);

  zend_object_std_dtor(object);
}

PHP_MINIT_FUNCTION(labelcache)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "labelcacheObj", labelcache_functions);
  mapscript_ce_labelcache = zend_register_internal_class(&ce TSRMLS_CC);

  mapscript_ce_labelcache->create_object = mapscript_labelcache_create_object;
  mapscript_ce_labelcache->ce_flags |= ZEND_ACC_FINAL;

  memcpy(&mapscript_labelcache_object_handlers, &mapscript_std_object_handlers, sizeof(mapscript_labelcache_object_handlers));
  mapscript_labelcache_object_handlers.free_obj = mapscript_labelcache_free_object;
  mapscript_labelcache_object_handlers.offset   = XtOffsetOf(php_labelcache_object, zobj);

  return SUCCESS;
}
#else
/* PHP5 */
static void mapscript_labelcache_object_destroy(void *object TSRMLS_DC)
{
  php_labelcache_object *php_labelcache = (php_labelcache_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_labelcache);

  MAPSCRIPT_FREE_PARENT(php_labelcache->parent);

  /* We don't need to free the labelCacheObj */

  efree(object);
}

static zend_object_value mapscript_labelcache_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_labelcache_object *php_labelcache;

  MAPSCRIPT_ALLOC_OBJECT(php_labelcache, php_labelcache_object);

  retval = mapscript_object_new(&php_labelcache->std, ce,
                                &mapscript_labelcache_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_labelcache->parent);

  return retval;
}

PHP_MINIT_FUNCTION(labelcache)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("labelCacheObj",
                           labelcache_functions,
                           mapscript_ce_labelcache,
                           mapscript_labelcache_object_new);

  mapscript_ce_labelcache->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
#endif
