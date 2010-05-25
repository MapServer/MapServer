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

zend_class_entry *mapscript_ce_resultcachemember;

ZEND_BEGIN_ARG_INFO_EX(resultcachemember___get_args, 0, 0, 1)
  ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(resultcachemember___set_args, 0, 0, 2)
  ZEND_ARG_INFO(0, property)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto resultCacheMemberObj __construct()
   resultCacheMemberObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(resultCacheMemberObj, __construct)
{
    mapscript_throw_exception("resultCacheMemberObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(resultCacheMemberObj, __get)
{
    char *property;
    long property_len;
    zval *zobj = getThis();
    php_resultcachemember_object *php_resultcachemember;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                              &property, &property_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_resultcachemember = (php_resultcachemember_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    IF_GET_LONG("shapeindex", php_resultcachemember->resultcachemember->shapeindex)
    else IF_GET_LONG("tileindex", php_resultcachemember->resultcachemember->tileindex)
    else IF_GET_LONG("classindex", php_resultcachemember->resultcachemember->classindex)
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
    }
}

PHP_METHOD(resultCacheMemberObj, __set)
{
    char *property;
    long property_len;
    zval *value;
    zval *zobj = getThis();
    php_resultcachemember_object *php_resultcachemember;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                              &property, &property_len, &value) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_resultcachemember = (php_resultcachemember_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if ( (STRING_EQUAL("shapeindex", property)) ||
         (STRING_EQUAL("tileindex", property)) ||
         (STRING_EQUAL("classindex", property)))
    {
        mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
    }
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
    }
}

zend_function_entry resultcachemember_functions[] = {
    PHP_ME(resultCacheMemberObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(resultCacheMemberObj, __get, resultcachemember___get_args, ZEND_ACC_PUBLIC)
    PHP_ME(resultCacheMemberObj, __set, resultcachemember___set_args, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

void mapscript_create_resultcachemember(resultCacheMemberObj *resultcachemember, parent_object parent,
                                        zval *return_value TSRMLS_DC)
{
    php_resultcachemember_object * php_resultcachemember;
    object_init_ex(return_value, mapscript_ce_resultcachemember); 
    php_resultcachemember = (php_resultcachemember_object *)zend_object_store_get_object(return_value TSRMLS_CC);
    php_resultcachemember->resultcachemember = resultcachemember;

    php_resultcachemember->parent = parent;
    MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_resultcachemember_object_destroy(void *object TSRMLS_DC)
{
    php_resultcachemember_object *php_resultcachemember = (php_resultcachemember_object *)object;

    MAPSCRIPT_FREE_OBJECT(php_resultcachemember);

    MAPSCRIPT_FREE_PARENT(php_resultcachemember->parent);

    /* We don't need to free the resultCacheMemberObj */ 
    
    efree(object);
}

static zend_object_value mapscript_resultcachemember_object_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    php_resultcachemember_object *php_resultcachemember;

    MAPSCRIPT_ALLOC_OBJECT(php_resultcachemember, php_resultcachemember_object);

    retval = mapscript_object_new(&php_resultcachemember->std, ce,
                                  &mapscript_resultcachemember_object_destroy TSRMLS_CC);

    MAPSCRIPT_INIT_PARENT(php_resultcachemember->parent);

    return retval;
}

PHP_MINIT_FUNCTION(resultcachemember)
{
    zend_class_entry ce;

    MAPSCRIPT_REGISTER_CLASS("resultCacheMemberObj", 
                             resultcachemember_functions,
                             mapscript_ce_resultcachemember,
                             mapscript_resultcachemember_object_new);

    mapscript_ce_resultcachemember->ce_flags |= ZEND_ACC_FINAL_CLASS; 
    
    return SUCCESS;
}
