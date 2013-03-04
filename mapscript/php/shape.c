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

zend_class_entry *mapscript_ce_shape;

ZEND_BEGIN_ARG_INFO_EX(shape___construct_args, 0, 0, 1)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_project_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, projIn, projectionObj, 0)
ZEND_ARG_OBJ_INFO(0, projOut, projectionObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_add_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, line, lineObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_line_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_contains_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_intersects_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_getPointUsingMeasure_args, 0, 0, 1)
ZEND_ARG_INFO(0, measure)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_getMeasureUsingPoint_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_getValue_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_ARG_INFO(0, fieldName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_buffer_args, 0, 0, 1)
ZEND_ARG_INFO(0, width)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_containsShape_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_union_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_intersection_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_difference_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_symdifference_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_overlaps_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_within_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_crosses_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_touches_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_equals_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_disjoint_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_simplify_args, 0, 0, 1)
ZEND_ARG_INFO(0, tolerance)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_topologyPreservingSimplify_args, 0, 0, 1)
ZEND_ARG_INFO(0, tolerance)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_draw_args, 0, 0, 3)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_distanceToPoint_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shape_distanceToShape_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

/* {{{ proto shape __construct(int type)
   Create a new shapeObj instance. */
PHP_METHOD(shapeObj, __construct)
{
  zval *zobj = getThis();
  php_shape_object *php_shape;
  long type;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &type) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *)zend_object_store_get_object(zobj TSRMLS_CC);

  if ((php_shape->shape = shapeObj_new(type)) == NULL) {
    mapscript_throw_exception("Unable to construct shapeObj." TSRMLS_CC);
    return;
  }

  MAKE_STD_ZVAL(php_shape->values);
  array_init(php_shape->values);
}
/* }}} */

PHP_METHOD(shapeObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_STRING("text", php_shape->shape->text)
  else IF_GET_LONG("classindex", php_shape->shape->classindex)
    else IF_GET_LONG("index", php_shape->shape->index)
      else IF_GET_LONG("tileindex", php_shape->shape->tileindex)
        else IF_GET_LONG("resultindex", php_shape->shape->resultindex)
          else IF_GET_LONG("numlines", php_shape->shape->numlines)
            else IF_GET_LONG("numvalues", php_shape->shape->numvalues)
              else IF_GET_LONG("type", php_shape->shape->type)
                else IF_GET_OBJECT("bounds", mapscript_ce_rect, php_shape->bounds, &php_shape->shape->bounds)
                  else IF_GET_OBJECT("values", NULL, php_shape->values, NULL)
                    else {
                      mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                    }
}

PHP_METHOD(shapeObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_STRING("text", php_shape->shape->text, value)
  else IF_SET_LONG("classindex", php_shape->shape->classindex, value)
    else IF_SET_LONG("index", php_shape->shape->index, value)
      else if ( (STRING_EQUAL("type", property)) ||
                (STRING_EQUAL("numlines", property)) ||
                (STRING_EQUAL("tileindex", property)) ||
                (STRING_EQUAL("resultindex", property)) ||
                (STRING_EQUAL("bounds", property)) ||
                (STRING_EQUAL("values", property)) ||
                (STRING_EQUAL("numvalues", property)) ) {
        mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
      } else {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
      }
}

/* {{{ proto int shape.add(lineObj line)
   Adds a line (i.e. a part) to a shape */
PHP_METHOD(shapeObj, add)
{
  zval *zobj =  getThis();
  zval *zline;
  php_shape_object *php_shape;
  php_line_object *php_line;
  int retval = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zline, mapscript_ce_line) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_line = (php_line_object *) zend_object_store_get_object(zline TSRMLS_CC);

  retval = shapeObj_add(php_shape->shape, php_line->line);

  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int shape.line(int i)
   Returns line (part) number i.  First line is number 0. */
PHP_METHOD(shapeObj, line)
{
  zval *zobj =  getThis();
  long index;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);


  if (index < 0 || index >= php_shape->shape->numlines) {
    mapscript_throw_exception("Line '%d' does not exist in this object." TSRMLS_CC, index);
    return;
  }

  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_line(&(php_shape->shape->line[index]), parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.contains(pointObj point)
   Returns true or false if the the point is in a polygone shape.*/
PHP_METHOD(shapeObj, contains)
{
  zval *zobj =  getThis();
  zval *zpoint;
  php_shape_object *php_shape;
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zpoint, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);

  if (!shapeObj_contains(php_shape->shape, php_point->point))
    RETURN_FALSE;

  RETURN_TRUE;
}
/* }}} */

/* {{{ proto int shape.intersects(shapeObj shape)
   Returns true if the two shapes intersect. Else false.*/
PHP_METHOD(shapeObj, intersects)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (!shapeObj_intersects(php_shape->shape, php_shape2->shape))
    RETURN_FALSE;

  RETURN_TRUE;
}
/* }}} */

/* {{{ proto int shape.project(projectionObj in, projectionObj out)
   Project a Shape. Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(shapeObj, project)
{
  zval *zobj_proj_in, *zobj_proj_out;
  zval *zobj =  getThis();
  php_shape_object *php_shape;
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

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_proj_in = (php_projection_object *) zend_object_store_get_object(zobj_proj_in TSRMLS_CC);
  php_proj_out = (php_projection_object *) zend_object_store_get_object(zobj_proj_out TSRMLS_CC);

  status = shapeObj_project(php_shape->shape, php_proj_in->projection, php_proj_out->projection);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int shape.getpointusingmeasure(double measure)
   Given a shape and a mesure, return a point object containing the XY
   location corresponding to the measure */
PHP_METHOD(shapeObj, getPointUsingMeasure)
{
  zval *zobj =  getThis();
  double measure;
  pointObj *point = NULL;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d",
                            &measure) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  point = shapeObj_getpointusingmeasure(php_shape->shape, measure);
  if (point == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_point(point, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.getpointusingmeasure(pointObj point)
   Given a shape and a point object, return a point object containing the XY
   location of the intersection between the point and the shape. The point
   return contains also the extrapolated M value at the intersection. */
PHP_METHOD(shapeObj, getMeasureUsingPoint)
{
  zval *zobj =  getThis();
  zval *zpoint;
  pointObj *intersection = NULL;
  php_shape_object *php_shape;
  php_point_object *php_point;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zpoint, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);

  intersection = shapeObj_getmeasureusingpoint(php_shape->shape, php_point->point);
  if (intersection == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_point(intersection, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto string shape.getValue(layerObj layer, string fieldName)
   Returns value for specified field name. */
PHP_METHOD(shapeObj, getValue)
{
  zval *zobj =  getThis();
  zval *zlayer;
  char *fieldName;
  long fieldName_len = 0;
  int i;
  php_layer_object *php_layer;
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os",
                            &zlayer, mapscript_ce_layer,
                            &fieldName, &fieldName_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(zlayer TSRMLS_CC);

  if (php_shape->shape->numvalues != php_layer->layer->numitems)
    RETURN_STRING("", 1);

  for(i=0; i<php_layer->layer->numitems; i++) {
    if (strcasecmp(php_layer->layer->items[i], fieldName)==0) {
      RETURN_STRING(php_shape->shape->values[i], 1);
    }
  }
}
/* }}} */

/* {{{ proto int shape.buffer(double width)
   Given a shape and a width, return a shape object with a buffer using
   underlying GEOS library*/
PHP_METHOD(shapeObj, buffer)
{
  zval *zobj =  getThis();
  double width;
  shapeObj *shape = NULL;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d",
                            &width) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  shape = shapeObj_buffer(php_shape->shape, width);
  if (shape == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.convexhull()
   Given a shape, return a shape representing the convex hull using
   underlying GEOS library */
PHP_METHOD(shapeObj, convexhull)
{
  zval *zobj =  getThis();
  shapeObj *shape = NULL;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  shape = shapeObj_convexHull(php_shape->shape);
  if (shape == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.boundary()
   Returns the boundary of the shape. Only available if php/mapscript
   is built with GEOS library.*/
PHP_METHOD(shapeObj, boundary)
{
  zval *zobj =  getThis();
  shapeObj *shape = NULL;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  shape = shapeObj_boundary(php_shape->shape);
  if (shape == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.contains(shapeobj shape)
   Return true or false if the given shape in argument 1 is contained
   in the shape. Use3d underlying msGEOSContains GEOS library*/
PHP_METHOD(shapeObj, containsShape)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (shapeObj_contains_geos(php_shape->shape, php_shape2->shape)) {
    RETURN_TRUE;
  } else
    RETURN_FALSE;

}
/* }}} */

/* {{{ proto int shape.Union(shapeobj shape)
   Return a shape which is the union of the current shape and the one
   given in argument 1 . Uses underlying GEOS library*/
PHP_METHOD(shapeObj, union)
{
  zval *zobj =  getThis();
  zval *zshape;
  shapeObj *shape;
  php_shape_object *php_shape, *php_shape2;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  shape = shapeObj_Union(php_shape->shape, php_shape2->shape);

  if (shape  == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.intersection(shapeobj shape)
   Return a shape which is the intersection of the current shape and the one
   given in argument 1 . Uses underlying GEOS library*/
PHP_METHOD(shapeObj, intersection)
{
  zval *zobj =  getThis();
  zval *zshape;
  shapeObj *shape;
  php_shape_object *php_shape, *php_shape2;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  shape = shapeObj_intersection(php_shape->shape, php_shape2->shape);

  if (shape  == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.difference(shapeobj shape)
   Return a shape which is the difference of the current shape and the one
   given in argument 1 . Uses underlying GEOS library*/
PHP_METHOD(shapeObj, difference)
{
  zval *zobj =  getThis();
  zval *zshape;
  shapeObj *shape;
  php_shape_object *php_shape, *php_shape2;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  shape = shapeObj_difference(php_shape->shape, php_shape2->shape);

  if (shape  == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.symdifference(shapeobj shape)
   Return a shape which is the symetrical difference of the current shape and
   the one given in argument 1 . Uses underlying GEOS library*/
PHP_METHOD(shapeObj, symdifference)
{
  zval *zobj =  getThis();
  zval *zshape;
  shapeObj *shape;
  php_shape_object *php_shape, *php_shape2;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  shape = shapeObj_symdifference(php_shape->shape, php_shape2->shape);

  if (shape  == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.overlaps(shapeobj shape)
   Return true if the given shape in argument 1 overlaps
   the shape. Else return false. */
PHP_METHOD(shapeObj, overlaps)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (shapeObj_overlaps(php_shape->shape, php_shape2->shape)) {
    RETURN_TRUE;
  } else
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto int shape.within(shapeobj shape)
   Return true if the given shape in argument 1 is within
   the shape. Else return false. */
PHP_METHOD(shapeObj, within)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (shapeObj_within(php_shape->shape, php_shape2->shape)) {
    RETURN_TRUE;
  } else
    RETURN_FALSE;

}
/* }}} */

/* {{{ proto int shape.crosses(shapeobj shape)
   Return true if the given shape in argument 1 crosses
   the shape. Else return false. */
PHP_METHOD(shapeObj, crosses)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (shapeObj_crosses(php_shape->shape, php_shape2->shape)) {
    RETURN_TRUE;
  } else
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto int shape.touches(shapeobj shape)
   Return true if the given shape in argument 1 touches
   the shape. Else return false. */
PHP_METHOD(shapeObj, touches)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (shapeObj_touches(php_shape->shape, php_shape2->shape)) {
    RETURN_TRUE;
  } else
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto int shape.equals(shapeobj shape)
   Return true if the given shape in argument 1 is equal to
   the shape. Else return false. */
PHP_METHOD(shapeObj, equals)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (shapeObj_equals(php_shape->shape, php_shape2->shape)) {
    RETURN_TRUE;
  } else
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto int shape.disjoint(shapeobj shape)
   Return true if the given shape in argument 1 is disjoint to
   the shape. Else return false. */
PHP_METHOD(shapeObj, disjoint)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape, *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  if (shapeObj_disjoint(php_shape->shape, php_shape2->shape)) {
    RETURN_TRUE;
  } else
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto int shape.getcentroid()
   Given a shape, return a point object representing the centroid */
PHP_METHOD(shapeObj, getCentroid)
{
  zval *zobj =  getThis();
  pointObj *point;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  point = shapeObj_getcentroid(php_shape->shape);

  if (point  == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_point(point, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.getarea()
   Returns the area  of the shape */
PHP_METHOD(shapeObj, getArea)
{
  zval *zobj =  getThis();
  double area = 0;
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  area = shapeObj_getarea(php_shape->shape);

  RETURN_DOUBLE(area);
}
/* }}} */

/* {{{ proto int shape.getlength()
   Returns the length  of the shape */
PHP_METHOD(shapeObj, getLength)
{
  zval *zobj =  getThis();
  double length = 0;
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  length = shapeObj_getlength(php_shape->shape);

  RETURN_DOUBLE(length);
}
/* }}} */

/* {{{ proto int shape.getlabelpoint()
   Given a shape, return a point object suitable for labelling it */
PHP_METHOD(shapeObj, getLabelPoint)
{
  zval *zobj =  getThis();
  pointObj *point;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  point =  shapeObj_getLabelPoint(php_shape->shape);

  if (point  == NULL)
    RETURN_FALSE;

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_point(point, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto string shape.toWkt()
   Returns WKT representation of geometry. */
PHP_METHOD(shapeObj, toWkt)
{
  zval *zobj =  getThis();
  char *wkt = NULL;
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  wkt = msShapeToWKT(php_shape->shape);
  if (wkt) {
    RETVAL_STRING(wkt, 1);
    msFree(wkt);
    return;
  }

  RETURN_STRING("", 1);
}
/* }}} */

/* {{{ proto int shape.setBounds()
   Updates the bounds property of the shape. Must be called to calculate new
   bounding box after new parts have been added. Returns true if
   successful, else return false.*/
PHP_METHOD(shapeObj, setBounds)
{
  zval *zobj =  getThis();
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  shapeObj_setBounds(php_shape->shape);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int shape.simplify(double tolerance)
   Given a shape and a tolerance, return a simplified shape object using
   underlying GEOS library */
PHP_METHOD(shapeObj, simplify)
{
  zval *zobj =  getThis();
  double tolerance;
  shapeObj *shape = NULL;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d",
                            &tolerance) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  shape = shapeObj_simplify(php_shape->shape, tolerance);
  if (shape  == NULL)
    RETURN_NULL();

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.topologypreservingsimplify(double tolerance)
   Given a shape and a tolerance, return a simplified shape object using
   underlying GEOS library */
PHP_METHOD(shapeObj, topologyPreservingSimplify)
{
  zval *zobj =  getThis();
  double tolerance;
  shapeObj *shape = NULL;
  php_shape_object *php_shape;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d",
                            &tolerance) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  shape = shapeObj_topologypreservingsimplify(php_shape->shape, tolerance);
  if (shape  == NULL)
    RETURN_NULL();

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_shape(shape, parent, NULL, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int shape.draw(mapObj map, layerObj layer, imageObj img)
   Draws the individual shape using layer. Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(shapeObj, draw)
{
  zval *zobj =  getThis();
  zval *zmap, *zlayer, *zimage;
  int status = MS_FAILURE;
  php_shape_object *php_shape;
  php_map_object *php_map;
  php_layer_object *php_layer;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "OOO",
                            &zmap, mapscript_ce_map,
                            &zlayer, mapscript_ce_layer,
                            &zimage, mapscript_ce_image) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_map = (php_map_object *) zend_object_store_get_object(zmap TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(zlayer TSRMLS_CC);
  php_image = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);

  if ((status = shapeObj_draw(php_shape->shape, php_map->map, php_layer->layer,
                              php_image->image)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto void shape.free()
   Free the object */
PHP_METHOD(shapeObj, free)
{
  zval *zobj =  getThis();
  php_shape_object *php_shape;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_shape->bounds);
}
/* }}} */

/* {{{ proto int shape.distanceToPoint(pointObj point)
   Returns the distance from the point to shape. */
PHP_METHOD(shapeObj, distanceToPoint)
{
  zval *zobj =  getThis();
  zval *zpoint;
  php_shape_object *php_shape;
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zpoint, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_point = (php_point_object *) zend_object_store_get_object(zpoint TSRMLS_CC);

  RETURN_DOUBLE(msDistancePointToShape(php_point->point, php_shape->shape));
}
/* }}} */

/* {{{ proto int shape.distanceToShape(shapeObj shape)
   Returns the distance from the shape to shape2. */
PHP_METHOD(shapeObj, distanceToShape)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_shape_object *php_shape;
  php_shape_object *php_shape2;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_shape = (php_shape_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_shape2 = (php_shape_object *) zend_object_store_get_object(zshape TSRMLS_CC);

  RETURN_DOUBLE(msGEOSDistance(php_shape->shape, php_shape2->shape));
}
/* }}} */

zend_function_entry shape_functions[] = {
  PHP_ME(shapeObj, __construct, shape___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(shapeObj, __get, shape___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, __set, shape___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(shapeObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, project, shape_project_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, add, shape_add_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, line, shape_line_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, contains, shape_contains_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, intersects, shape_intersects_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, getPointUsingMeasure, shape_getPointUsingMeasure_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, getMeasureUsingPoint, shape_getMeasureUsingPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, getValue, shape_getValue_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, buffer, shape_buffer_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, convexhull, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, boundary, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, containsShape, shape_containsShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, union, shape_union_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, intersection, shape_intersection_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, difference, shape_difference_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, symdifference, shape_symdifference_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, overlaps, shape_overlaps_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, within, shape_within_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, crosses, shape_crosses_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, touches, shape_touches_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, equals, shape_equals_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, disjoint, shape_disjoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, getCentroid, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, getArea, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, getLength, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, getLabelPoint, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, toWkt, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, setBounds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, simplify, shape_simplify_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, topologyPreservingSimplify, shape_topologyPreservingSimplify_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, draw, shape_draw_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, distanceToPoint, shape_distanceToPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, distanceToShape, shape_distanceToShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(shapeObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_shape(shapeObj *shape, parent_object parent, php_layer_object *php_layer, zval *return_value TSRMLS_DC)
{
  php_shape_object *php_shape;

  int i;

  object_init_ex(return_value, mapscript_ce_shape);
  php_shape = (php_shape_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_shape->shape = shape;

  MAKE_STD_ZVAL(php_shape->values);
  array_init(php_shape->values);

  if (php_layer) {
    if ((php_shape->shape->numvalues == php_layer->layer->numitems) ||
        (php_shape->shape->numvalues == 0 && php_layer->layer->numitems == -1)) {
      for(i=0; i<php_shape->shape->numvalues; i++) {
        add_assoc_string(php_shape->values, php_layer->layer->items[i], php_shape->shape->values[i], 1);
      }
    } else {
      mapscript_throw_exception("Assertion failed, Could not set shape values: %d, %d" TSRMLS_CC,
                                php_shape->shape->numvalues, php_layer->layer->numitems);
      return;
    }
  }

  if (parent.val)
    php_shape->is_ref = 1;

  php_shape->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_shape_object_destroy(void *object TSRMLS_DC)
{
  php_shape_object *php_shape = (php_shape_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_shape);

  MAPSCRIPT_FREE_PARENT(php_shape->parent);
  MAPSCRIPT_DELREF(php_shape->bounds);
  MAPSCRIPT_DELREF(php_shape->values);

  if (php_shape->shape && !php_shape->is_ref) {
    shapeObj_destroy(php_shape->shape);
  }

  efree(object);
}

static zend_object_value mapscript_shape_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_shape_object *php_shape;

  MAPSCRIPT_ALLOC_OBJECT(php_shape, php_shape_object);

  retval = mapscript_object_new(&php_shape->std, ce,
                                &mapscript_shape_object_destroy TSRMLS_CC);

  php_shape->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_shape->parent);
  php_shape->bounds = NULL;
  php_shape->values = NULL;

  return retval;
}

PHP_MINIT_FUNCTION(shape)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("shapeObj",
                           shape_functions,
                           mapscript_ce_shape,
                           mapscript_shape_object_new);

  mapscript_ce_shape->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
