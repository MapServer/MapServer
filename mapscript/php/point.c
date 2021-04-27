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

zend_class_entry *mapscript_ce_point;
#if PHP_VERSION_ID >= 70000
zend_object_handlers mapscript_point_object_handlers;
#endif  

ZEND_BEGIN_ARG_INFO_EX(point___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point_setXY_args, 0, 0, 2)
ZEND_ARG_INFO(0, x)
ZEND_ARG_INFO(0, y)
ZEND_ARG_INFO(0, m)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point_setXYZ_args, 0, 0, 3)
ZEND_ARG_INFO(0, x)
ZEND_ARG_INFO(0, y)
ZEND_ARG_INFO(0, z)
ZEND_ARG_INFO(0, m)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point_project_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, projIn, projectionObj, 0)
ZEND_ARG_OBJ_INFO(0, projOut, projectionObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point_distanceToPoint_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point_distanceToLine_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, point, pointObj, 0)
ZEND_ARG_OBJ_INFO(0, point2, pointObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point_distanceToShape_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, shape, shapeObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(point_draw_args, 0, 0, 5)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_ARG_INFO(0, classIndex)
ZEND_ARG_INFO(0, text)
ZEND_END_ARG_INFO()

/* {{{ proto point __construct()
   Create a new pointObj instance. */
PHP_METHOD(pointObj, __construct)
{
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, getThis());

  if ((php_point->point = pointObj_new()) == NULL) {
    mapscript_throw_exception("Unable to construct pointObj." TSRMLS_CC);
    return;
  }

  php_point->point->x = 0;
  php_point->point->y = 0;
  php_point->point->z = 0;
  php_point->point->m = 0;
}
/* }}} */

PHP_METHOD(pointObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);

  IF_GET_DOUBLE("x", php_point->point->x)
  else IF_GET_DOUBLE("y", php_point->point->y)
    else IF_GET_DOUBLE("z", php_point->point->z)
      else IF_GET_DOUBLE("m", php_point->point->m)
        else {
          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
        }
}

PHP_METHOD(pointObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);

  IF_SET_DOUBLE("x", php_point->point->x, value)
  else IF_SET_DOUBLE("y", php_point->point->y, value)
    else IF_SET_DOUBLE("z", php_point->point->z, value)
      else IF_SET_DOUBLE("m", php_point->point->m, value)
        else {
          mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
        }
}

/* {{{ proto int point.setXY(double x, double y, double m)
   3rd argument m is used for Measured shape files. It is not mandatory.
   Set new point. Returns MS_FAILURE on error. */
PHP_METHOD(pointObj, setXY)
{
  double x, y, m;
  zval *zobj = getThis();
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "dd|d",
                            &x, &y, &m) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);

  php_point->point->x = x;
  php_point->point->y = y;

  if (ZEND_NUM_ARGS() == 3) {
    php_point->point->m = m;
  }

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int point.setXYZ(double x, double y, double z, double m)
   4th argument m is used for Measured shape files. It is not mandatory.
   Set new point. Returns MS_FAILURE on error. */
PHP_METHOD(pointObj, setXYZ)
{
  double x, y, z, m;
  zval *zobj = getThis();
  php_point_object *php_point;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ddd|d",
                            &x, &y, &z, &m) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);

  php_point->point->x = x;
  php_point->point->y = y;

  php_point->point->z = z;

  if (ZEND_NUM_ARGS() == 4) {
    php_point->point->m = m;
  }

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */


/* {{{ proto int point.project(projectionObj in, projectionObj out)
   Project the point. returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(pointObj, project)
{
  zval *zobj_proj_in, *zobj_proj_out;
  zval *zobj =  getThis();
  php_point_object *php_point;
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

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);
  php_proj_in = MAPSCRIPT_OBJ_P(php_projection_object, zobj_proj_in);
  php_proj_out = MAPSCRIPT_OBJ_P(php_projection_object, zobj_proj_out);

  status = pointObj_project(php_point->point, php_proj_in->projection, php_proj_out->projection);
  if (status != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int point.distanceToPoint(pointObj)
   Calculates distance between two points. */
PHP_METHOD(pointObj, distanceToPoint)
{
  zval *zobj_point2;
  zval *zobj =  getThis();
  php_point_object *php_point, *php_point2;
  double distance = -1;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zobj_point2, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);
  php_point2 = MAPSCRIPT_OBJ_P(php_point_object, zobj_point2);

  distance = pointObj_distanceToPoint(php_point->point, php_point2->point);

  RETURN_DOUBLE(distance);
}
/* }}} */

/* {{{ proto int point.distanceToLine(pointObj line_point, pointObj line_point2)
   Calculates distance between a point and a lined defined by the
   two points passed in argument. */
PHP_METHOD(pointObj, distanceToLine)
{
  zval *zobj =  getThis();
  zval *zobj_line_point, *zobj_line_point2;
  php_point_object *php_point, *php_line_point, *php_line_point2;
  double distance = -1;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "OO",
                            &zobj_line_point, mapscript_ce_point,
                            &zobj_line_point2, mapscript_ce_point) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);
  php_line_point = MAPSCRIPT_OBJ_P(php_point_object, zobj_line_point);
  php_line_point2 = MAPSCRIPT_OBJ_P(php_point_object, zobj_line_point2);

  distance = pointObj_distanceToLine(php_point->point, php_line_point->point, php_line_point2->point);

  RETURN_DOUBLE(distance);
}
/* }}} */

/* {{{ proto int distanceToShape(shapeObj shape)
   Calculates the minimum distance between a point and a shape. */
PHP_METHOD(pointObj, distanceToShape)
{
  zval *zobj =  getThis();
  zval *zshape;
  php_point_object *php_point;
  php_shape_object *php_shape;
  double distance = -1;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zshape, mapscript_ce_shape) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);
  php_shape = MAPSCRIPT_OBJ_P(php_shape_object, zshape);

  distance = pointObj_distanceToShape(php_point->point, php_shape->shape);

  RETURN_DOUBLE(distance);
}
/* }}} */

/* {{{ proto int point.draw(mapObj map, layerObj layer, imageObj img, string class_name, string text)
   Draws the individual point using layer. Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(pointObj, draw)
{
  zval *zobj =  getThis();
  zval *zmap, *zlayer, *zimage;
  char *text = NULL;
  long text_len = 0;
  long classIndex;
  int status = MS_FAILURE;
  php_point_object *php_point;
  php_map_object *php_map;
  php_layer_object *php_layer;
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "OOOl|s",
                            &zmap, mapscript_ce_map,
                            &zlayer, mapscript_ce_layer,
                            &zimage, mapscript_ce_image,
                            &classIndex, &text, &text_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_point = MAPSCRIPT_OBJ_P(php_point_object, zobj);
  php_map = MAPSCRIPT_OBJ_P(php_map_object, zmap);
  php_layer = MAPSCRIPT_OBJ_P(php_layer_object, zlayer);
  php_image = MAPSCRIPT_OBJ_P(php_image_object, zimage);

  if ((status = pointObj_draw(php_point->point, php_map->map, php_layer->layer, php_image->image,
                              classIndex, text)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

zend_function_entry point_functions[] = {
  PHP_ME(pointObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(pointObj, __get, point___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, __set, point___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, setXY, point_setXY_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, setXYZ, point_setXYZ_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, project, point_project_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, distanceToPoint, point_distanceToPoint_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, distanceToLine, point_distanceToLine_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(pointObj, distanceToSegment, distanceToLine, point_distanceToLine_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, distanceToShape, point_distanceToShape_args, ZEND_ACC_PUBLIC)
  PHP_ME(pointObj, draw, point_draw_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_point(pointObj *point, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_point_object * php_point;
  object_init_ex(return_value, mapscript_ce_point);
  php_point = MAPSCRIPT_OBJ_P(php_point_object, return_value);
  php_point->point = point;

  if (ZVAL_NOT_UNDEF(parent.val))
    php_point->is_ref = 1;

  php_point->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

#if PHP_VERSION_ID >= 70000
/* PHP7 - Modification by Bjoern Boldt <mapscript@pixaweb.net> */
static zend_object *mapscript_point_create_object(zend_class_entry *ce TSRMLS_DC)
{
  php_point_object *php_point;

  php_point = ecalloc(1, sizeof(*php_point) + zend_object_properties_size(ce));

  zend_object_std_init(&php_point->zobj, ce TSRMLS_CC);
  object_properties_init(&php_point->zobj, ce);

  php_point->zobj.handlers = &mapscript_point_object_handlers;

  php_point->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_point->parent);

  return &php_point->zobj;
}

static void mapscript_point_free_object(zend_object *object)
{
  php_point_object *php_point;

  php_point = (php_point_object *)((char *)object - XtOffsetOf(php_point_object, zobj));

  MAPSCRIPT_FREE_PARENT(php_point->parent);

  if (php_point->point && !php_point->is_ref) {
    pointObj_destroy(php_point->point);
  }

  zend_object_std_dtor(object);
}

static zend_object* mapscript_point_clone_object(zval *zobj)
{
  php_point_object *php_point_old, *php_point_new;
  zend_object* zobj_new;

  php_point_old = MAPSCRIPT_OBJ_P(php_point_object, zobj);

  zobj_new = mapscript_point_create_object(mapscript_ce_point);
  php_point_new = (php_point_object *)((char *)zobj_new - XtOffsetOf(php_point_object, zobj));

  zend_objects_clone_members(&php_point_new->zobj, &php_point_old->zobj);

  if ((php_point_new->point = pointObj_new()) == NULL) {
    mapscript_throw_exception("Unable to construct pointObj." TSRMLS_CC);
    return NULL;
  }
  memcpy(php_point_new->point, php_point_old->point, sizeof(pointObj));

  return zobj_new;
}

PHP_MINIT_FUNCTION(point)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "pointObj", point_functions);
  mapscript_ce_point = zend_register_internal_class(&ce TSRMLS_CC);

  mapscript_ce_point->create_object = mapscript_point_create_object;
  mapscript_ce_point->ce_flags |= ZEND_ACC_FINAL;

  memcpy(&mapscript_point_object_handlers, &mapscript_std_object_handlers, sizeof(mapscript_point_object_handlers));
  mapscript_point_object_handlers.free_obj = mapscript_point_free_object;
  mapscript_point_object_handlers.clone_obj = mapscript_point_clone_object;
  mapscript_point_object_handlers.offset   = XtOffsetOf(php_point_object, zobj);

  return SUCCESS;
}
#else
/* PHP5 */

static void mapscript_point_object_destroy(void *object TSRMLS_DC)
{
  php_point_object *php_point = (php_point_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_point);

  MAPSCRIPT_FREE_PARENT(php_point->parent);

  if (php_point->point && !php_point->is_ref) {
    pointObj_destroy(php_point->point);
  }

  efree(object);
}

static zend_object_value mapscript_point_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_point_object *php_point;

  MAPSCRIPT_ALLOC_OBJECT(php_point, php_point_object);

  retval = mapscript_object_new(&php_point->std, ce,
                                &mapscript_point_object_destroy TSRMLS_CC);

  php_point->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_point->parent);

  return retval;
}

PHP_MINIT_FUNCTION(point)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("pointObj",
                           point_functions,
                           mapscript_ce_point,
                           mapscript_point_object_new);

  mapscript_ce_point->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
#endif
