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
    mapscript_throw_exception("labelCacheMemberObj cannot be constructed");
}
/* }}} */

PHP_METHOD(labelCacheMemberObj, __get)
{
    char *property;
    int property_len;
    zval *zobj = getThis();
    php_labelcachemember_object *php_labelcachemember;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                              &property, &property_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_labelcachemember = (php_labelcachemember_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    IF_GET_STRING("text", php_labelcachemember->labelcachemember->text)
    else IF_GET_LONG("classindex", php_labelcachemember->labelcachemember->classindex)
    else IF_GET_LONG("featuresize", php_labelcachemember->labelcachemember->featuresize) 
    else IF_GET_LONG("layerindex", php_labelcachemember->labelcachemember->layerindex) 
    else IF_GET_LONG("numstyles", php_labelcachemember->labelcachemember->numstyles) 
    else IF_GET_LONG("shapeindex", php_labelcachemember->labelcachemember->shapeindex) 
    else IF_GET_LONG("status", php_labelcachemember->labelcachemember->status) 
    else IF_GET_LONG("tileindex", php_labelcachemember->labelcachemember->tileindex) 
    else IF_GET_OBJECT("point", php_labelcachemember->point) 
    else IF_GET_OBJECT("label", php_labelcachemember->label) 
    else IF_GET_OBJECT("styles", php_labelcachemember->styles) 
    else IF_GET_OBJECT("poly", php_labelcachemember->poly) 
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object.", property);
    }
}

PHP_METHOD(labelCacheMemberObj, __set)
{
    char *property;
    int property_len;
    zval *value;
    zval *zobj = getThis();
    php_labelcachemember_object *php_labelcachemember;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                              &property, &property_len, &value) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_labelcachemember = (php_labelcachemember_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if ( (STRING_EQUAL("text", property)) ||
         (STRING_EQUAL("classindex", property)) ||
         (STRING_EQUAL("featuresize", property)) ||
         (STRING_EQUAL("layerindex", property)) ||
         (STRING_EQUAL("numstyles", property)) ||
         (STRING_EQUAL("shapeindex", property)) ||
         (STRING_EQUAL("status", property)) ||
         (STRING_EQUAL("tileindex", property)) ||
         (STRING_EQUAL("label", property)) ||
         (STRING_EQUAL("styles", property)) ||
         (STRING_EQUAL("poly", property)) ||
         (STRING_EQUAL("point", property)))
    {
        mapscript_throw_exception("Property '%s' is read-only and cannot be set.", property);
    }
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object.", property);
    }
         
}

zend_function_entry labelcachemember_functions[] = {
    PHP_ME(labelCacheMemberObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(labelCacheMemberObj, __get, labelcachemember___get_args, ZEND_ACC_PUBLIC)
    PHP_ME(labelCacheMemberObj, __set, labelcachemember___set_args, ZEND_ACC_PUBLIC)
    PHP_MALIAS(labelCacheMemberObj, setProperty, __set, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


void mapscript_create_labelcachemember(labelCacheMemberObj *labelcachemember, 
                                        zval *php_parent, zval *return_value TSRMLS_DC)
{
    php_labelcachemember_object * php_labelcachemember;
    object_init_ex(return_value, mapscript_ce_labelcachemember); 
    php_labelcachemember = (php_labelcachemember_object *)zend_object_store_get_object(return_value TSRMLS_CC);
    php_labelcachemember->labelcachemember = labelcachemember;

    MAKE_STD_ZVAL(php_labelcachemember->label);
    mapscript_create_label(&(labelcachemember->label), return_value, php_labelcachemember->label TSRMLS_CC);

    MAKE_STD_ZVAL(php_labelcachemember->point);
    mapscript_create_point(&(labelcachemember->point), return_value, php_labelcachemember->point TSRMLS_CC);

    if (labelcachemember->styles)
    {
        MAKE_STD_ZVAL(php_labelcachemember->styles);
        mapscript_create_style(labelcachemember->styles, return_value, php_labelcachemember->styles TSRMLS_CC);
    }

    if (labelcachemember->poly)
    {
        MAKE_STD_ZVAL(php_labelcachemember->poly);
        mapscript_create_shape(labelcachemember->poly, return_value, NULL, php_labelcachemember->poly TSRMLS_CC);
    }

    php_labelcachemember->parent = php_parent;

    MAPSCRIPT_ADDREF(php_parent);
}

static void mapscript_labelcachemember_object_destroy(void *object TSRMLS_DC)
{
    php_labelcachemember_object *php_labelcachemember = (php_labelcachemember_object *)object;

    MAPSCRIPT_FREE_OBJECT(php_labelcachemember);

    MAPSCRIPT_DELREF(php_labelcachemember->parent);
    MAPSCRIPT_DELREF(php_labelcachemember->point);
    MAPSCRIPT_DELREF(php_labelcachemember->label);
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
                                  &mapscript_labelcachemember_object_destroy);

    php_labelcachemember->parent = NULL;
    php_labelcachemember->point = NULL;
    php_labelcachemember->label = NULL;
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

