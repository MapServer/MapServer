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

zend_class_entry *mapscript_ce_projection;
zend_object_handlers mapscript_projection_object_handlers;

ZEND_BEGIN_ARG_INFO_EX(projection___construct_args, 0, 0, 1)
ZEND_ARG_INFO(0, projString)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(projection_setWKTProjection_args, 0, 0, 1)
ZEND_ARG_INFO(0, wkt)
ZEND_END_ARG_INFO()

/* {{{ proto projectionObj __construct(string projString)
   Create a new projectionObj instance. */
PHP_METHOD(projectionObj, __construct)
{
  char *projString;
  long projString_len = 0;
  php_projection_object *php_projection;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &projString, &projString_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_projection = (php_projection_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

  if ((php_projection->projection = projectionObj_new(projString)) == NULL) {
    mapscript_throw_mapserver_exception("Unable to construct projectionObj." TSRMLS_CC);
    return;
  }
}
/* }}} */

/* {{{ proto projectionObj setWKTProjection(string wkt)
   Set the wkt projection. Return MS_SUCCESS or MS_FAILURE .*/
PHP_METHOD(projectionObj, setWKTProjection)
{
  char *wkt;
  long wkt_len = 0;
  php_projection_object *php_projection;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &wkt, &wkt_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_projection = (php_projection_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

  RETURN_LONG(msOGCWKT2ProjectionObj(wkt, php_projection->projection, MS_FALSE));
}
/* }}} */

/* {{{ proto int projectionObj.getunits()
   Returns the units of a projection object */
PHP_METHOD(projectionObj, getUnits)
{
  php_projection_object *php_projection;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_projection = (php_projection_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

  RETURN_LONG(projectionObj_getUnits(php_projection->projection));
}
/* }}} */

zend_function_entry projection_functions[] = {
  PHP_ME(projectionObj, __construct, projection___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(projectionObj, setWKTProjection, projection_setWKTProjection_args, ZEND_ACC_PUBLIC)
  PHP_ME(projectionObj, getUnits, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_projection(projectionObj *projection, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_projection_object * php_projection;
  object_init_ex(return_value, mapscript_ce_projection);
  php_projection = (php_projection_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_projection->projection = projection;

  if (parent.val)
    php_projection->is_ref = 1;

  php_projection->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_projection_object_destroy(void *object TSRMLS_DC)
{
  php_projection_object *php_projection = (php_projection_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_projection);

  MAPSCRIPT_FREE_PARENT(php_projection->parent);

  if (php_projection->projection && !php_projection->is_ref) {
    projectionObj_destroy(php_projection->projection);
  }

  efree(object);
}

static zend_object_value mapscript_projection_object_new_ex(zend_class_entry *ce, php_projection_object **ptr TSRMLS_DC)
{
  zend_object_value retval;
  php_projection_object *php_projection;

  MAPSCRIPT_ALLOC_OBJECT(php_projection, php_projection_object);

  retval = mapscript_object_new_ex(&php_projection->std, ce,
                                   &mapscript_projection_object_destroy,
                                   &mapscript_projection_object_handlers TSRMLS_CC);

  if (ptr)
    *ptr = php_projection;

  php_projection->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_projection->parent);

  return retval;
}

static zend_object_value mapscript_projection_object_new(zend_class_entry *ce TSRMLS_DC)
{
  return mapscript_projection_object_new_ex(ce, NULL TSRMLS_CC);
}

static zend_object_value mapscript_projection_object_clone(zval *zobj TSRMLS_DC)
{
  php_projection_object *php_projection_old, *php_projection_new;
  zend_object_value new_ov;

  php_projection_old = (php_projection_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  new_ov = mapscript_projection_object_new_ex(mapscript_ce_projection, &php_projection_new TSRMLS_CC);
  zend_objects_clone_members(&php_projection_new->std, new_ov, &php_projection_old->std, Z_OBJ_HANDLE_P(zobj) TSRMLS_CC);

  php_projection_new->projection = projectionObj_clone(php_projection_old->projection);

  return new_ov;
}

PHP_MINIT_FUNCTION(projection)
{
  zend_class_entry ce;

  memcpy(&mapscript_projection_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mapscript_projection_object_handlers.clone_obj = mapscript_projection_object_clone;

  MAPSCRIPT_REGISTER_CLASS("projectionObj",
                           projection_functions,
                           mapscript_ce_projection,
                           mapscript_projection_object_new);

  mapscript_ce_projection->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
