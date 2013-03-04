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

zend_class_entry *mapscript_ce_grid;

ZEND_BEGIN_ARG_INFO_EX(grid___construct_args, 0, 1, 1)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(grid___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(grid___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct(layerObj layer)
   Create a new intance of gridObj. */
PHP_METHOD(gridObj, __construct)
{
  zval *zlayer;
  php_layer_object *php_layer;
  php_grid_object *php_grid, *php_old_grid;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zlayer, mapscript_ce_layer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_grid = (php_grid_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(zlayer TSRMLS_CC);

  php_layer->layer->connectiontype = MS_GRATICULE;


  if (php_layer->layer->layerinfo != NULL)
    free(php_layer->layer->layerinfo);


  php_layer->layer->layerinfo = (graticuleObj *)malloc( sizeof( graticuleObj ) );
  initGrid((graticuleObj *)php_layer->layer->layerinfo);

  php_grid->grid = (graticuleObj *)php_layer->layer->layerinfo;

  if (php_layer->grid && (Z_TYPE_P(php_layer->grid) == IS_OBJECT)) {
    php_old_grid = (php_grid_object *) zend_object_store_get_object(php_layer->grid TSRMLS_CC);
    php_old_grid->parent.child_ptr = NULL;
    zend_objects_store_del_ref(php_layer->grid TSRMLS_CC);
  }

  MAKE_STD_ZVAL(php_layer->grid);
  MAPSCRIPT_MAKE_PARENT(zlayer, &php_layer->grid);
  mapscript_create_grid((graticuleObj *)(php_layer->layer->layerinfo), parent, php_layer->grid TSRMLS_CC);

  return_value_ptr = &php_layer->grid;
}
/* }}} */

PHP_METHOD(gridObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_grid_object *php_grid;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_grid = (php_grid_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_DOUBLE("minsubdivide", php_grid->grid->minsubdivides)
  else IF_GET_DOUBLE("maxsubdivide", php_grid->grid->maxsubdivides)
    else IF_GET_DOUBLE("minarcs", php_grid->grid->minarcs)
      else IF_GET_DOUBLE("maxarcs", php_grid->grid->maxarcs)
        else IF_GET_DOUBLE("mininterval", php_grid->grid->minincrement)
          else IF_GET_DOUBLE("maxinterval", php_grid->grid->maxincrement)
            else IF_GET_STRING("labelformat", php_grid->grid->labelformat)
              else {
                mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
              }
}

PHP_METHOD(gridObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_grid_object *php_grid;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_grid = (php_grid_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_DOUBLE("minsubdivide", php_grid->grid->minsubdivides, value)
  else IF_SET_DOUBLE("maxsubdivide", php_grid->grid->maxsubdivides, value)
    else IF_SET_DOUBLE("minarcs", php_grid->grid->minarcs, value)
      else IF_SET_DOUBLE("maxarcs", php_grid->grid->maxarcs, value)
        else IF_SET_DOUBLE("mininterval", php_grid->grid->minincrement, value)
          else IF_SET_DOUBLE("maxinterval", php_grid->grid->maxincrement, value)
            else IF_SET_STRING("labelformat", php_grid->grid->labelformat, value)
              else {
                mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
              }
}

zend_function_entry grid_functions[] = {
  PHP_ME(gridObj, __construct, grid___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(gridObj, __get, grid___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(gridObj, __set, grid___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(gridObj, set, __set, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_grid(graticuleObj *grid, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_grid_object * php_grid;
  object_init_ex(return_value, mapscript_ce_grid);
  php_grid = (php_grid_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_grid->grid = grid;

  php_grid->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_grid_object_destroy(void *object TSRMLS_DC)
{
  php_grid_object *php_grid = (php_grid_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_grid);

  MAPSCRIPT_FREE_PARENT(php_grid->parent);

  /* We don't need to free the gridObj */

  efree(object);
}

static zend_object_value mapscript_grid_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_grid_object *php_grid;

  MAPSCRIPT_ALLOC_OBJECT(php_grid, php_grid_object);

  retval = mapscript_object_new(&php_grid->std, ce,
                                &mapscript_grid_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_grid->parent);

  return retval;
}

PHP_MINIT_FUNCTION(grid)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("gridObj",
                           grid_functions,
                           mapscript_ce_grid,
                           mapscript_grid_object_new);

  mapscript_ce_grid->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
