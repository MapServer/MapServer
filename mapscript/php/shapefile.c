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

zend_class_entry *mapscript_ce_shapefile;

ZEND_BEGIN_ARG_INFO_EX(shapefile___construct_args, 0, 0, 2)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile_getShape_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile_getPoint_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile_getExtent_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile_addShape_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile_addPoint_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shapefile_getTransformed_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

/* {{{ proto shapefile __construct(string filename, int type)
   Create a new shapeFileObj instance. */
PHP_METHOD(shapeFileObj, __construct)
{
  zval *zobj = getThis();
  php_shapefile_object *php_shapefile;
  char *filename;
  long filename_len = 0;
  long type;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                            &filename, &filename_len, &type) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *)zend_object_store_get_object(zobj TSRMLS_CC);

  php_shapefile->shapefile = shapefileObj_new(filename, type);

  if (php_shapefile->shapefile == NULL) {
    mapscript_throw_mapserver_exception("Failed to open shapefile %s" TSRMLS_CC, filename);
    return;
  }
}
/* }}} */

PHP_METHOD(shapeFileObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_shapefile_object *php_shapefile;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("numshapes", php_shapefile->shapefile->numshapes)
  else IF_GET_LONG("type", php_shapefile->shapefile->type)
    else IF_GET_LONG("isopen", php_shapefile->shapefile->isopen)
      else IF_GET_LONG("lastshape", php_shapefile->shapefile->lastshape)
        else IF_GET_STRING("source", php_shapefile->shapefile->source)
          else IF_GET_OBJECT("bounds", mapscript_ce_rect, php_shapefile->bounds, &php_shapefile->shapefile->bounds)
            else {
              mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
            }
}

PHP_METHOD(shapeFileObj, __set)
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

  if ( (STRING_EQUAL("numshapes", property)) ||
       (STRING_EQUAL("type", property)) ||
       (STRING_EQUAL("source", property)) ||
       (STRING_EQUAL("isopen", property)) ||
       (STRING_EQUAL("lastshape", property)) ||
       (STRING_EQUAL("bounds", property)) ) {
    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

/* {{{ proto int shapefile.getShape(int i)
   Retrieve shape by index. */
PHP_METHOD(shapeFileObj, getShape)
{
  zval *zobj =  getThis();
  long index;
  shapeObj *shape;
  php_shapefile_object *php_shapefile;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  /* Create a new shapeObj to hold the result
   * Note that the type used to create the shape (MS_NULL) does not matter
   * at this point since it will be set by SHPReadShape().
   */
  if ((shape = shapeObj_new(MS_SHAPE_NULL)) == NULL) {
    mapscript_throw_mapserver_exception("Failed creating new shape (out of memory?)" TSRMLS_CC);
    return;
  }

  if (shapefileObj_get(php_shapefile->shapefile, index, shape) != MS_SUCCESS) {
    shapeObj_destroy(shape);
    mapscript_throw_mapserver_exception("Failed reading shape %ld." TSRMLS_CC, index);
    return;
  }

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shapefile.getPoint(int i)
   Retrieve a point by index. */
PHP_METHOD(shapeFileObj, getPoint)
{
  zval *zobj =  getThis();
  long index;
  pointObj *point;
  php_shapefile_object *php_shapefile;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  /* Create a new shapeObj to hold the result
   * Note that the type used to create the shape (MS_NULL) does not matter
   * at this point since it will be set by SHPReadShape().
   */
  /* Create a new PointObj to hold the result */
  if ((point = pointObj_new()) == NULL) {
    mapscript_throw_mapserver_exception("Failed creating new point (out of memory?)" TSRMLS_CC);
    return;
  }

  /* Read from the file */
  if (shapefileObj_getPoint(php_shapefile->shapefile, index, point) != MS_SUCCESS) {
    pointObj_destroy(point);
    mapscript_throw_mapserver_exception("Failed reading point %ld." TSRMLS_CC, index);
    return;
  }

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_point(point, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shapefile.getExtent(int i)
   Retrieve a shape's bounding box by index. */

PHP_METHOD(shapeFileObj, getExtent)
{
  zval *zobj =  getThis();
  long index;
  rectObj *rect;
  php_shapefile_object *php_shapefile;
  parent_object p;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  /* Create a new rectObj to hold the result */
  if ((rect = rectObj_new()) == NULL) {
    mapscript_throw_mapserver_exception("Failed creating new rectObj (out of memory?)" TSRMLS_CC);
    return;
  }

  /* Read from the file
   * shapefileObj_getExtent() has no return value!  How do we catch errors?
   */
  shapefileObj_getExtent(php_shapefile->shapefile, index, rect);

  /* Return rectObj */
  MAPSCRIPT_INIT_PARENT(p);
  mapscript_create_rect(rect, p, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shapefile.addShape(shapeObj shape)
   Appends a shape to an open shapefile. */
PHP_METHOD(shapeFileObj, addShape)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shapefile_object *php_shapefile;
  php_shape_object *php_shape;
  int retval = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  retval = shapefileObj_add(php_shapefile->shapefile, php_shape->shape);

  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int shapefile.addPoint(pointObj point)
   Appends a point to a poin layer. */
PHP_METHOD(shapeFileObj, addPoint)
{
  zval *zobj =  getThis();
  zval *zpoint;
  php_shapefile_object *php_shapefile;
  php_point_object *php_point;
  int retval = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zpoint, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);

  retval = shapefileObj_addPoint(php_shapefile->shapefile, php_point->point);

  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int shapefile.getTransformed(mapObj map, int index)
   Retrieve shape by index. */
PHP_METHOD(shapeFileObj, getTransformed)
{
  zval *zobj =  getThis();
  zval *zmap;
  long index;
  php_shapefile_object *php_shapefile;
  php_map_object *php_map;
  shapeObj *shape = NULL;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ol",
                            &zmap, mapscript_ce_map, &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_map = (php_map_object *) zend_object_store_get_object(zmap TSRMLS_CC);

  /* Create a new shapeObj to hold the result
   * Note that the type used to create the shape (MS_NULL) does not matter
   * at this point since it will be set by SHPReadShape().
   */
  if ((shape = shapeObj_new(MS_SHAPE_NULL)) == NULL) {
    mapscript_throw_mapserver_exception("Failed creating new shape (out of memory?)" TSRMLS_CC);
    return;
  }

  /* Read from the file */
  if (shapefileObj_getTransformed(php_shapefile->shapefile, php_map->map,
                                  index, shape) != MS_SUCCESS) {
    shapeObj_destroy(shape);
    mapscript_throw_mapserver_exception("Failed reading shape %ld." TSRMLS_CC, index);
    return;
  }

  /* Return shape object */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto void shapefile.free()
   Free the object  */
PHP_METHOD(shapeFileObj, free)
{
  zval *zobj =  getThis();
  php_shapefile_object *php_shapefile;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shapefile = (php_shapefile_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_shapefile->bounds);
}
/* }}} */

zend_function_entry shapefile_functions[] = {
  PHP_ME(shapeFileObj, __construct, shapefile___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(shapeFileObj, __get, shapefile___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, __set, shapefile___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, getShape, shapefile_getShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, getPoint, shapefile_getPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, getExtent, shapefile_getExtent_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, addShape, shapefile_addShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, addPoint, shapefile_addPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, getTransformed, shapefile_getTransformed_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeFileObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_shapefile(shapefileObj *shapefile, zval *return_value TSRMLS_DC)
{
  php_shapefile_object * php_shapefile;

  object_init_ex(return_value, mapscript_ce_shapefile);
  php_shapefile = (php_shapefile_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_shapefile->shapefile = shapefile;
}

static void mapscript_shapefile_object_destroy(void *object TSRMLS_DC)
{
  php_shapefile_object *php_shapefile = (php_shapefile_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_shapefile);

  MAPSCRIPT_DELREF(php_shapefile->bounds);

  shapefileObj_destroy(php_shapefile->shapefile);

  efree(object);
}

static zend_object_value mapscript_shapefile_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_shapefile_object *php_shapefile;

  MAPSCRIPT_ALLOC_OBJECT(php_shapefile, php_shapefile_object);

  retval = mapscript_object_new(&php_shapefile->std, ce,
                                &mapscript_shapefile_object_destroy TSRMLS_CC);

  php_shapefile->bounds = NULL;

  return retval;
}

PHP_MINIT_FUNCTION(shapefile)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("shapeFileObj",
                           shapefile_functions,
                           mapscript_ce_shapefile,
                           mapscript_shapefile_object_new);

  mapscript_ce_shapefile->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
