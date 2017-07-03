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

zend_class_entry *mapscript_ce_labelcachemember;
#if PHP_VERSION_ID >= 70000
zend_object_handlers mapscript_labelcachemember_object_handlers;
#endif  

ZEND_BEGIN_ARG_INFO_EX(labelcachemember___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(labelcachemember___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct()
   labelCacheMemberObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(labelCacheMemberObj, __construct)
{
  mapscript_throw_exception("labelCacheMemberObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(labelCacheMemberObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_labelcachemember_object *php_labelcachemember;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_labelcachemember = MAPSCRIPT_OBJ_P(php_labelcachemember_object, zobj);

  IF_GET_LONG("classindex", php_labelcachemember->labelcachemember->classindex)
  /* else IF_GET_LONG("featuresize", php_labelcachemember->labelcachemember->featuresize) */
    else IF_GET_LONG("layerindex", php_labelcachemember->labelcachemember->layerindex)
      /* else IF_GET_LONG("numstyles", php_labelcachemember->labelcachemember->numstyles) */
        /* else IF_GET_LONG("numlabels", php_labelcachemember->labelcachemember->numlabels) */
          /* else IF_GET_LONG("shapeindex", php_labelcachemember->labelcachemember->shapeindex) */
          else IF_GET_LONG("status", php_labelcachemember->labelcachemember->status)
            else IF_GET_LONG("markerid", php_labelcachemember->labelcachemember->markerid)
              /* else IF_GET_LONG("tileindex", php_labelcachemember->labelcachemember->tileindex) */
              else IF_GET_OBJECT("point", mapscript_ce_point, php_labelcachemember->point, &php_labelcachemember->labelcachemember->point)
                /* else IF_GET_OBJECT("labels", mapscript_ce_label, php_labelcachemember->labels, &php_labelcachemember->labelcachemember->labels) */
                  /* else IF_GET_OBJECT("styles", mapscript_ce_style, php_labelcachemember->styles, php_labelcachemember->labelcachemember->styles) */
                    /* else IF_GET_OBJECT("poly", mapscript_ce_shape, php_labelcachemember->poly, php_labelcachemember->labelcachemember->poly) */
                      else {
                        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                      }
}

PHP_METHOD(labelCacheMemberObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  /* zval *zobj = getThis(); */
  /* php_labelcachemember_object *php_labelcachemember; */

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  /* php_labelcachemember = MAPSCRIPT_OBJ_P(php_labelcachemember_object, zobj); */

  if ( (STRING_EQUAL("classindex", property)) ||
       /* (STRING_EQUAL("featuresize", property)) || */
       (STRING_EQUAL("layerindex", property)) ||
       /* (STRING_EQUAL("numstyles", property)) || */
       /* (STRING_EQUAL("numlabels", property)) || */
       /* (STRING_EQUAL("shapeindex", property)) || */
       (STRING_EQUAL("status", property)) ||
       (STRING_EQUAL("markerid", property)) ||
       /* (STRING_EQUAL("tileindex", property)) || */
       /* (STRING_EQUAL("labels", property)) || */
       /* (STRING_EQUAL("styles", property)) || */
       /* (STRING_EQUAL("poly", property)) || */
       (STRING_EQUAL("point", property))) {
    mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
  } else {
    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
  }
}

/* proto void free()
   Free the object */
PHP_METHOD(labelCacheMemberObj, free)
{
  zval *zobj = getThis();
  php_labelcachemember_object *php_labelcachemember;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_labelcachemember = MAPSCRIPT_OBJ_P(php_labelcachemember_object, zobj);

  MAPSCRIPT_DELREF(php_labelcachemember->point);
  MAPSCRIPT_DELREF(php_labelcachemember->labels);
  MAPSCRIPT_DELREF(php_labelcachemember->styles);
  MAPSCRIPT_DELREF(php_labelcachemember->poly);
}
/* }}} */

zend_function_entry labelcachemember_functions[] = {
  PHP_ME(labelCacheMemberObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(labelCacheMemberObj, __get, labelcachemember___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelCacheMemberObj, __set, labelcachemember___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(labelCacheMemberObj, setProperty, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(labelCacheMemberObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_labelcachemember(labelCacheMemberObj *labelcachemember,
                                       parent_object parent, zval *return_value TSRMLS_DC)
{
  php_labelcachemember_object * php_labelcachemember;
  object_init_ex(return_value, mapscript_ce_labelcachemember);
  php_labelcachemember = MAPSCRIPT_OBJ_P(php_labelcachemember_object, return_value);
  php_labelcachemember->labelcachemember = labelcachemember;

  php_labelcachemember->parent = parent;

  MAPSCRIPT_ADDREF(parent.val);
}

#if PHP_VERSION_ID >= 70000
/* PHP7 - Modification by Bjoern Boldt <mapscript@pixaweb.net> */
static zend_object *mapscript_labelcachemember_create_object(zend_class_entry *ce TSRMLS_DC)
{
  php_labelcachemember_object *php_labelcachemember;

  php_labelcachemember = ecalloc(1, sizeof(*php_labelcachemember) + zend_object_properties_size(ce));

  zend_object_std_init(&php_labelcachemember->zobj, ce TSRMLS_CC);
  object_properties_init(&php_labelcachemember->zobj, ce);

  php_labelcachemember->zobj.handlers = &mapscript_labelcachemember_object_handlers;

  MAPSCRIPT_INIT_PARENT(php_labelcachemember->parent);
  ZVAL_UNDEF(&php_labelcachemember->point);
  ZVAL_UNDEF(&php_labelcachemember->labels);
  ZVAL_UNDEF(&php_labelcachemember->styles);
  ZVAL_UNDEF(&php_labelcachemember->poly);


  return &php_labelcachemember->zobj;
}

static void mapscript_labelcachemember_free_object(zend_object *object)
{
  php_labelcachemember_object *php_labelcachemember;

  php_labelcachemember = (php_labelcachemember_object *)((char *)object - XtOffsetOf(php_labelcachemember_object, zobj));

  MAPSCRIPT_FREE_PARENT(php_labelcachemember->parent);
  MAPSCRIPT_DELREF(php_labelcachemember->point);
  MAPSCRIPT_DELREF(php_labelcachemember->labels);
  MAPSCRIPT_DELREF(php_labelcachemember->styles);
  MAPSCRIPT_DELREF(php_labelcachemember->poly);

  /* We don't need to free the labelCacheMemberObj, the mapObj will do it */

  zend_object_std_dtor(object);
}

PHP_MINIT_FUNCTION(labelcachemember)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "labelCacheMemberObj", labelcachemember_functions);
  mapscript_ce_labelcachemember = zend_register_internal_class(&ce TSRMLS_CC);

  mapscript_ce_labelcachemember->create_object = mapscript_labelcachemember_create_object;
  mapscript_ce_labelcachemember->ce_flags |= ZEND_ACC_FINAL;

  memcpy(&mapscript_labelcachemember_object_handlers, &mapscript_std_object_handlers, sizeof(mapscript_labelcachemember_object_handlers));
  mapscript_labelcachemember_object_handlers.free_obj = mapscript_labelcachemember_free_object;
  mapscript_labelcachemember_object_handlers.offset   = XtOffsetOf(php_labelcachemember_object, zobj);

  return SUCCESS;
}
#else
/* PHP5 */

static void mapscript_labelcachemember_object_destroy(void *object TSRMLS_DC)
{
  php_labelcachemember_object *php_labelcachemember = (php_labelcachemember_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_labelcachemember);

  MAPSCRIPT_FREE_PARENT(php_labelcachemember->parent);
  MAPSCRIPT_DELREF(php_labelcachemember->point);
  MAPSCRIPT_DELREF(php_labelcachemember->labels);
  MAPSCRIPT_DELREF(php_labelcachemember->styles);
  MAPSCRIPT_DELREF(php_labelcachemember->poly);

  /* We don't need to free the labelCacheMemberObj, the mapObj will do it */

  efree(object);
}

static zend_object_value mapscript_labelcachemember_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_labelcachemember_object *php_labelcachemember;

  MAPSCRIPT_ALLOC_OBJECT(php_labelcachemember, php_labelcachemember_object);

  retval = mapscript_object_new(&php_labelcachemember->std, ce,
                                &mapscript_labelcachemember_object_destroy TSRMLS_CC);

  MAPSCRIPT_INIT_PARENT(php_labelcachemember->parent);
  php_labelcachemember->point = NULL;
  php_labelcachemember->labels = NULL;
  php_labelcachemember->styles = NULL;
  php_labelcachemember->poly = NULL;

  return retval;
}

PHP_MINIT_FUNCTION(labelcachemember)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("labelCacheMemberObj",
                           labelcachemember_functions,
                           mapscript_ce_labelcachemember,
                           mapscript_labelcachemember_object_new);

  mapscript_ce_labelcachemember->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
#endif
