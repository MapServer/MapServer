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

zend_class_entry *mapscript_ce_cluster;

ZEND_BEGIN_ARG_INFO_EX(cluster___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(cluster___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(cluster_updateFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(cluster_setGroup_args, 0, 0, 1)
ZEND_ARG_INFO(0, group)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(cluster_setFilter_args, 0, 0, 1)
ZEND_ARG_INFO(0, filter)
ZEND_END_ARG_INFO()


/* {{{ proto outputformat __construct()
   clusterObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(clusterObj, __construct)
{
  mapscript_throw_exception("clusterObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(clusterObj, __get)
{
  char *property;
  long property_len;
  zval *zobj = getThis();
  php_cluster_object *php_cluster;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_cluster = (php_cluster_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_DOUBLE("maxdistance", php_cluster->cluster->maxdistance)
  else IF_GET_DOUBLE("buffer", php_cluster->cluster->buffer)
    else IF_GET_STRING("region", php_cluster->cluster->region)
      else {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
      }
}

PHP_METHOD(clusterObj, __set)
{
  char *property;
  long property_len;
  zval *value;
  zval *zobj = getThis();
  php_cluster_object *php_cluster;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_cluster = (php_cluster_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_DOUBLE("maxdistance", php_cluster->cluster->maxdistance, value)
  else IF_SET_DOUBLE("buffer", php_cluster->cluster->buffer, value)
    else IF_SET_STRING("region", php_cluster->cluster->region, value)
      else {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
      }
}

/* {{{ proto int updateFromString(string snippet)
   Update a cluster from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(clusterObj, updateFromString)
{
  char *snippet;
  long snippet_len;
  zval *zobj = getThis();
  php_cluster_object *php_cluster;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_cluster = (php_cluster_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = clusterObj_updateFromString(php_cluster->cluster, snippet);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int setGroup(string group)
   Set the group expression string.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(clusterObj, setGroup)
{
  char *group = NULL;
  long group_len;
  zval *zobj = getThis();
  php_cluster_object *php_cluster;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &group, &group_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_cluster = (php_cluster_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((status = clusterObj_setGroup(php_cluster->cluster, group)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string cluster.getGroupString()
   Return the cluster's group expression. Returns NULL on error. */
PHP_METHOD(clusterObj, getGroupString)
{
  zval *zobj = getThis();
  char *value = NULL;
  php_cluster_object *php_cluster;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_cluster = (php_cluster_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = clusterObj_getGroupString(php_cluster->cluster);
  if (value == NULL) {
    RETURN_NULL();
  }

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int setFilter(string filter)
   Set the filter expression string.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(clusterObj, setFilter)
{
  char *filter = NULL;
  long filter_len;
  zval *zobj = getThis();
  php_cluster_object *php_cluster;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filter, &filter_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_cluster = (php_cluster_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((status = clusterObj_setFilter(php_cluster->cluster, filter)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string cluster.getFilterString()
   Return the cluster's filter expression. Returns NULL on error. */
PHP_METHOD(clusterObj, getFilterString)
{
  zval *zobj = getThis();
  char *value = NULL;
  php_cluster_object *php_cluster;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_cluster = (php_cluster_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = clusterObj_getFilterString(php_cluster->cluster);
  if (value == NULL) {
    RETURN_NULL();
  }

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

zend_function_entry cluster_functions[] = {
  PHP_ME(clusterObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(clusterObj, __get, cluster___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(clusterObj, __set, cluster___set_args, ZEND_ACC_PUBLIC)
  PHP_ME(clusterObj, updateFromString, cluster_updateFromString_args, ZEND_ACC_PUBLIC)
  PHP_ME(clusterObj, setGroup, cluster_setGroup_args, ZEND_ACC_PUBLIC)
  PHP_ME(clusterObj, getGroupString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(clusterObj, setFilter, cluster_setFilter_args, ZEND_ACC_PUBLIC)
  PHP_ME(clusterObj, getFilterString, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_cluster(clusterObj *cluster, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_cluster_object * php_cluster;
  object_init_ex(return_value, mapscript_ce_cluster);
  php_cluster = (php_cluster_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_cluster->cluster = cluster;

  if (parent.val)
    php_cluster->is_ref = 1;

  php_cluster->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_cluster_object_destroy(void *object TSRMLS_DC)
{
  php_cluster_object *php_cluster = (php_cluster_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_cluster);

  MAPSCRIPT_FREE_PARENT(php_cluster->parent);

  /* We don't need to free the clusterObj */

  efree(object);
}

static zend_object_value mapscript_cluster_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_cluster_object *php_cluster;

  MAPSCRIPT_ALLOC_OBJECT(php_cluster, php_cluster_object);

  retval = mapscript_object_new(&php_cluster->std, ce,
                                &mapscript_cluster_object_destroy TSRMLS_CC);

  php_cluster->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_cluster->parent)

  return retval;
}

PHP_MINIT_FUNCTION(cluster)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("clusterObj",
                           cluster_functions,
                           mapscript_ce_cluster,
                           mapscript_cluster_object_new);

  mapscript_ce_cluster->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
