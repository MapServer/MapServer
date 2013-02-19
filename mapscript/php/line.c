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

zend_class_entry *mapscript_ce_line;
zend_object_handlers mapscript_line_object_handlers;

ZEND_BEGIN_ARG_INFO_EX(line___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(line___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(line_add_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(line_set_args, 0, 0, 2)
ZEND_ARG_INFO(0, index)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(line_addXY_args, 0, 0, 2)
ZEND_ARG_INFO(0, x)
ZEND_ARG_INFO(0, y)
ZEND_ARG_INFO(0, m)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(line_addXYZ_args, 0, 0, 3)
ZEND_ARG_INFO(0, x)
ZEND_ARG_INFO(0, y)
ZEND_ARG_INFO(0, z)
ZEND_ARG_INFO(0, m)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(line_project_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, projIn, projectionObj, 0)
ZEND_ARG_OBJ_INFO(0, projOut, projectionObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(line_point_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

/* {{{ proto line __construct()
   Create a new lineObj instance. */
PHP_METHOD(lineObj, __construct)
{
  php_line_object *php_line;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

  if ((php_line->line = lineObj_new()) == NULL) {
    mapscript_throw_exception("Unable to construct lineObj." TSRMLS_CC);
    return;
  }
}
/* }}} */

PHP_METHOD(lineObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_line_object *php_line;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("numpoints", php_line->line->numpoints)
  else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

PHP_METHOD(lineObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_line_object *php_line;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (STRING_EQUAL("numpoints", property)) {
    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

/* {{{ proto int line.add(pointObj point)
   Adds a point to the end of a line */
PHP_METHOD(lineObj, add)
{
  zval *zobj =  getThis();
  zval *zobj_point;
  php_line_object *php_line;
  php_point_object *php_point;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zobj_point, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zobj_point TSRMLS_CC);

  status = lineObj_add(php_line->line, php_point->point);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int line.addXY(double x, double y, double m)
   3rd argument m is used for Measured shape files. It is not mandatory.
   Adds a point to the end of a line */
PHP_METHOD(lineObj, addXY)
{
  zval *zobj = getThis();
  pointObj point;
  double x, y, m = 0;
  int status = MS_FAILURE;
  php_line_object *php_line;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "dd|d", &x, &y, &m) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  point.x = x;
  point.y = y;

#ifdef USE_LINE_Z_M
  point.z = 0;
  point.m = m;
#endif

  status = lineObj_add(php_line->line, &point);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int line.addXYZ(double x, double y, double z, double m)
   4th argument m is used for Measured shape files. It is not mandatory.
   Adds a point to the end of a line */
PHP_METHOD(lineObj, addXYZ)
{
  zval *zobj = getThis();
  pointObj point;
  double x, y, z, m = 0;
  int status = MS_FAILURE;
  php_line_object *php_line;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ddd|d",
                            &x, &y, &z, &m) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  point.x = x;
  point.y = y;

#ifdef USE_LINE_Z_M
  point.z = z;
  point.m = m;
#endif

  status = lineObj_add(php_line->line, &point);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int line.project(projectionObj in, projectionObj out)
   Project the point. returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(lineObj, project)
{
  zval *zobj_proj_in, *zobj_proj_out;
  zval *zobj =  getThis();
  php_line_object *php_line;
  php_projection_object *php_proj_in, *php_proj_out;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "OO",
                            &zobj_proj_in, mapscript_ce_projection,
                            &zobj_proj_out, mapscript_ce_projection) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_proj_in = (php_projection_object *) zend_object_store_get_object(zobj_proj_in TSRMLS_CC);
  php_proj_out = (php_projection_object *) zend_object_store_get_object(zobj_proj_out TSRMLS_CC);

  status = lineObj_project(php_line->line, php_proj_in->projection, php_proj_out->projection);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int line.point(int i)
   Returns point number i.  First point is number 0. */
PHP_METHOD(lineObj, point)
{
  zval *zobj = getThis();
  php_line_object *php_line;
  long index;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ( (index < 0) || (index >= php_line->line->numpoints)) {
    mapscript_throw_exception("Point '%d' does not exist in this object." TSRMLS_CC, index);
    return;
  }

  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_point(&(php_line->line->point[index]), parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int line.set(int, index, pointObj point)
   Set the point values at the specified index */
PHP_METHOD(lineObj, set)
{
  zval *zobj =  getThis();
  zval *zobj_point;
  long index;
  php_line_object *php_line;
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lO",
                            &index, &zobj_point, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_line = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ( (index < 0) || (index >= php_line->line->numpoints)) {
    mapscript_throw_exception("Point '%d' does not exist in this object." TSRMLS_CC, index);
    return;
  }

  php_point = (php_point_object *) zend_object_store_get_object(zobj_point TSRMLS_CC);

  php_line->line->point[index].x = php_point->point->x;
  php_line->line->point[index].y = php_point->point->y;

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

zend_function_entry line_functions[] = {
  PHP_ME(lineObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(lineObj, __get, line___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(lineObj, __set, line___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(lineObj, add, line_add_args, ZEND_ACC_PUBLIC)
  PHP_ME(lineObj, addXY, line_addXY_args, ZEND_ACC_PUBLIC)
  PHP_ME(lineObj, addXYZ, line_addXYZ_args, ZEND_ACC_PUBLIC)
  PHP_ME(lineObj, project, line_project_args, ZEND_ACC_PUBLIC)
  PHP_ME(lineObj, point, line_point_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(lineObj, get, point, line_point_args, ZEND_ACC_PUBLIC)
  PHP_ME(lineObj, set, line_set_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_line(lineObj *line, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_line_object * php_line;
  object_init_ex(return_value, mapscript_ce_line);
  php_line = (php_line_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_line->line = line;

  if (parent.val)
    php_line->is_ref = 1;

  php_line->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_line_object_destroy(void *object TSRMLS_DC)
{
  php_line_object *php_line = (php_line_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_line);

  MAPSCRIPT_FREE_PARENT(php_line->parent);

  if (php_line->line && !php_line->is_ref) {
    lineObj_destroy(php_line->line);
  }

  efree(object);
}

static zend_object_value mapscript_line_object_new_ex(zend_class_entry *ce, php_line_object **ptr TSRMLS_DC)
{
  zend_object_value retval;
  php_line_object *php_line;

  MAPSCRIPT_ALLOC_OBJECT(php_line, php_line_object);

  retval = mapscript_object_new_ex(&php_line->std, ce,
                                   &mapscript_line_object_destroy,
                                   &mapscript_line_object_handlers TSRMLS_CC);

  if (ptr)
    *ptr = php_line;

  php_line->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_line->parent);

  return retval;
}

static zend_object_value mapscript_line_object_new(zend_class_entry *ce TSRMLS_DC)
{
  return mapscript_line_object_new_ex(ce, NULL TSRMLS_CC);
}

static zend_object_value mapscript_line_object_clone(zval *zobj TSRMLS_DC)
{
  php_line_object *php_line_old, *php_line_new;
  zend_object_value new_ov;

  php_line_old = (php_line_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  new_ov = mapscript_line_object_new_ex(mapscript_ce_line, &php_line_new TSRMLS_CC);
  zend_objects_clone_members(&php_line_new->std, new_ov, &php_line_old->std, Z_OBJ_HANDLE_P(zobj) TSRMLS_CC);

  php_line_new->line = lineObj_clone(php_line_old->line);

  return new_ov;
}

PHP_MINIT_FUNCTION(line)
{
  zend_class_entry ce;

  memcpy(&mapscript_line_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mapscript_line_object_handlers.clone_obj = mapscript_line_object_clone;


  MAPSCRIPT_REGISTER_CLASS("lineObj",
                           line_functions,
                           mapscript_ce_line,
                           mapscript_line_object_new);

  mapscript_ce_line->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
