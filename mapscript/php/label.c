
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

zend_class_entry *mapscript_ce_label;

ZEND_BEGIN_ARG_INFO_EX(label___get_args, 0, 0, 1)
  ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label___set_args, 0, 0, 2)
  ZEND_ARG_INFO(0, property)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_updateFromString_args, 0, 0, 1)
  ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_setBinding_args, 0, 0, 2)
  ZEND_ARG_INFO(0, labelBinding)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_getBinding_args, 0, 0, 1)
  ZEND_ARG_INFO(0, labelBinding)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_removeBinding_args, 0, 0, 1)
  ZEND_ARG_INFO(0, labelBinding)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct() 
   labelObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(labelObj, __construct)
{
    mapscript_throw_exception("labelObj cannot be constructed");
}
/* }}} */

PHP_METHOD(labelObj, __get)
{
    char *property;
    int property_len;
    zval *zobj = getThis();
    php_label_object *php_label;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                              &property, &property_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_label = (php_label_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    IF_GET_STRING("font",  php_label->label->font)
    else IF_GET_STRING("encoding", php_label->label->encoding)
    else IF_GET_LONG("type", php_label->label->type)
    else IF_GET_LONG("shadowsizex",  php_label->label->shadowsizex)
    else IF_GET_LONG("shadowsizey",  php_label->label->shadowsizey)
    else IF_GET_LONG("backgroundshadowsizex", php_label->label->backgroundshadowsizex)
    else IF_GET_LONG("backgroundshadowsizey", php_label->label->backgroundshadowsizey)
    else IF_GET_DOUBLE("size", php_label->label->size)
    else IF_GET_DOUBLE("minsize", php_label->label->minsize)
    else IF_GET_DOUBLE("maxsize", php_label->label->maxsize)
    else IF_GET_LONG("position", php_label->label->position)
    else IF_GET_LONG("offsetx", php_label->label->offsetx)
    else IF_GET_LONG("offsety", php_label->label->offsety)
    else IF_GET_DOUBLE("angle", php_label->label->angle)
    else IF_GET_LONG("autoangle", php_label->label->autoangle)
    else IF_GET_LONG("autofollow", php_label->label->autofollow)
    else IF_GET_LONG("buffer", php_label->label->buffer)
    else IF_GET_LONG("antialias", php_label->label->antialias)
    else IF_GET_LONG("wrap", php_label->label->wrap)
    else IF_GET_LONG("minfeaturesize", php_label->label->minfeaturesize)
    else IF_GET_LONG("autominfeaturesize", php_label->label->autominfeaturesize)
    else IF_GET_LONG("repeatdistance", php_label->label->repeatdistance)
    else IF_GET_LONG("mindistance", php_label->label->mindistance)
    else IF_GET_LONG("partials", php_label->label->partials)
    else IF_GET_LONG("force", php_label->label->force)
    else IF_GET_LONG("outlinewidth", php_label->label->outlinewidth)
    else IF_GET_LONG("align", php_label->label->align)
    else IF_GET_LONG("maxlength", php_label->label->maxlength)
    else IF_GET_LONG("minlength", php_label->label->minlength)
    else IF_GET_LONG("priority", php_label->label->priority)
    else IF_GET_OBJECT("color", php_label->color) 
    else IF_GET_OBJECT("outlinecolor", php_label->outlinecolor) 
    else IF_GET_OBJECT("shadowcolor", php_label->shadowcolor) 
    else IF_GET_OBJECT("backgroundcolor", php_label->backgroundcolor) 
    else IF_GET_OBJECT("backgroundshadowcolor", php_label->backgroundshadowcolor) 
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object.", property);
    }
}

PHP_METHOD(labelObj, __set)
{
    char *property;
    int property_len;
    zval *value;
    zval *zobj = getThis();
    php_label_object *php_label;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                              &property, &property_len, &value) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_label = (php_label_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    IF_SET_STRING("font",  php_label->label->font, value)
    else IF_SET_STRING("encoding", php_label->label->encoding, value)
    else IF_SET_LONG("type", php_label->label->type, value)
    else IF_SET_LONG("shadowsizex",  php_label->label->shadowsizex, value)
    else IF_SET_LONG("shadowsizey",  php_label->label->shadowsizey, value)
    else IF_SET_LONG("backgroundshadowsizex", php_label->label->backgroundshadowsizex, value)
    else IF_SET_LONG("backgroundshadowsizey", php_label->label->backgroundshadowsizey, value)
    else IF_SET_DOUBLE("size", php_label->label->size, value)
    else IF_SET_DOUBLE("minsize", php_label->label->minsize, value)
    else IF_SET_DOUBLE("maxsize", php_label->label->maxsize, value)
    else IF_SET_LONG("position", php_label->label->position, value)
    else IF_SET_LONG("offsetx", php_label->label->offsetx, value)
    else IF_SET_LONG("offsety", php_label->label->offsety, value)
    else IF_SET_DOUBLE("angle", php_label->label->angle, value)
    else IF_SET_LONG("autoangle", php_label->label->autoangle, value)
    else IF_SET_LONG("autofollow", php_label->label->autofollow, value)
    else IF_SET_LONG("buffer", php_label->label->buffer, value)
    else IF_SET_LONG("antialias", php_label->label->antialias, value)
    else IF_SET_BYTE("wrap", php_label->label->wrap, value)
    else IF_SET_LONG("minfeaturesize", php_label->label->minfeaturesize, value)
    else IF_SET_LONG("autominfeaturesize", php_label->label->autominfeaturesize, value)
    else IF_SET_LONG("repeatdistance", php_label->label->repeatdistance, value)
    else IF_SET_LONG("mindistance", php_label->label->mindistance, value)
    else IF_SET_LONG("partials", php_label->label->partials, value)
    else IF_SET_LONG("force", php_label->label->force, value)
    else IF_SET_LONG("outlinewidth", php_label->label->outlinewidth, value)
    else IF_SET_LONG("align", php_label->label->align, value)
    else IF_SET_LONG("maxlength", php_label->label->maxlength, value)
    else IF_SET_LONG("minlength", php_label->label->minlength, value)
    else IF_SET_LONG("priority", php_label->label->priority, value)
    else if ( (STRING_EQUAL("color", property)) ||
              (STRING_EQUAL("outlinecolor", property)) ||
              (STRING_EQUAL("shadowcolor", property)) ||
              (STRING_EQUAL("backgroundcolor", property)) ||
              (STRING_EQUAL("backgroundshadowcolor", property)) )
    {
        mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors.", property);
    }
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object.", property);
    }
         
}

/* {{{ proto int label.updateFromString(string snippet)
   Update a label from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(labelObj, updateFromString)
{
    zval *zobj = getThis();
    char *snippet;
    int snippet_len;
    int status = MS_FAILURE;
    php_label_object *php_label;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                              &snippet, &snippet_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_label = (php_label_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if ((status = labelObj_updateFromString(php_label->label, snippet)) != MS_SUCCESS)
    {
        mapscript_throw_mapserver_exception("");
        return;
    }

    RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int label.setbinding(const bindingid, string value)
   Set the attribute binding for a specfiled label property. Returns MS_SUCCESS on success. */
PHP_METHOD(labelObj, setBinding)
{
    zval *zobj = getThis();
    char *value;
    int value_len;
    int bindingId;
    php_label_object *php_label;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls",
                              &bindingId, &value, &value_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

    php_label = (php_label_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if (bindingId < 0 || bindingId > MS_LABEL_BINDING_LENGTH)
    {
        mapscript_throw_exception("Invalid binding id.");
        return;
    }

    if (!value || strlen(value) <= 0)
    {
        mapscript_throw_exception("Invalid binding value.");
        return;
    }

    if(php_label->label->bindings[bindingId].item) 
    {
        msFree(php_label->label->bindings[bindingId].item);
        php_label->label->bindings[bindingId].index = -1; 
        php_label->label->numbindings--;
    }

    php_label->label->bindings[bindingId].item = strdup(value); 
    php_label->label->numbindings++;

    RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int label.getbinding(const bindingid)
   Get the value of a attribute binding for a specfiled label property. 
   Returns the string value if exist, else null. */
PHP_METHOD(labelObj, getBinding)
{
    zval *zobj = getThis();
    int bindingId;
    char *value = NULL;
    php_label_object *php_label;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                              &bindingId) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_label = (php_label_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if (bindingId < 0 || bindingId > MS_LABEL_BINDING_LENGTH)
    {
        mapscript_throw_exception("Invalid binding id.");
        return;
    }

    if( (value = php_label->label->bindings[bindingId].item) != NULL)
    {
       RETURN_STRING(value, 1);                
    }

    RETURN_NULL();
    
}
/* }}} */

/* {{{ proto int label.removebinding(const bindingid)
   Remove attribute binding for a specfiled label property. Returns MS_SUCCESS on success. */
PHP_METHOD(labelObj, removeBinding)
{
    zval *zobj = getThis();
    int bindingId;
    php_label_object *php_label;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                              &bindingId) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_label = (php_label_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if (bindingId < 0 || bindingId > MS_LABEL_BINDING_LENGTH)
    {
        mapscript_throw_exception("Invalid binding id.");
        return;
    }


    if(php_label->label->bindings[bindingId].item) 
    {
        msFree(php_label->label->bindings[bindingId].item);
        php_label->label->bindings[bindingId].item =  NULL;
        php_label->label->bindings[bindingId].index = -1; 
        php_label->label->numbindings--;
    }

    RETURN_LONG(MS_SUCCESS);
    
}
/* }}} */

zend_function_entry label_functions[] = {
    PHP_ME(labelObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(labelObj, __get, label___get_args, ZEND_ACC_PUBLIC)
    PHP_ME(labelObj, __set, label___set_args, ZEND_ACC_PUBLIC)
    PHP_MALIAS(labelObj, set, __set, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(labelObj, updateFromString, label_updateFromString_args, ZEND_ACC_PUBLIC)
    PHP_ME(labelObj, setBinding, label_setBinding_args, ZEND_ACC_PUBLIC)
    PHP_ME(labelObj, getBinding, label_getBinding_args, ZEND_ACC_PUBLIC)
    PHP_ME(labelObj, removeBinding, label_removeBinding_args, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


void mapscript_create_label(labelObj *label, zval *php_parent, zval *return_value TSRMLS_DC)
{
    php_label_object * php_label;
    object_init_ex(return_value, mapscript_ce_label); 
    php_label = (php_label_object *)zend_object_store_get_object(return_value TSRMLS_CC);
    php_label->label = label;

    MAKE_STD_ZVAL(php_label->color);
    mapscript_create_color(&(label->color), return_value, php_label->color TSRMLS_CC);

    MAKE_STD_ZVAL(php_label->outlinecolor);
    mapscript_create_color(&(label->outlinecolor), return_value, php_label->outlinecolor TSRMLS_CC);

    MAKE_STD_ZVAL(php_label->shadowcolor);
    mapscript_create_color(&(label->shadowcolor), return_value, php_label->shadowcolor TSRMLS_CC);

    MAKE_STD_ZVAL(php_label->backgroundcolor);
    mapscript_create_color(&(label->backgroundcolor), return_value, php_label->backgroundcolor TSRMLS_CC);

    MAKE_STD_ZVAL(php_label->backgroundshadowcolor);
    mapscript_create_color(&(label->backgroundshadowcolor), return_value, php_label->backgroundshadowcolor TSRMLS_CC);

    php_label->parent = php_parent;
    
    MAPSCRIPT_ADDREF(php_parent);
}

static void mapscript_label_object_destroy(void *object TSRMLS_DC)
{
    php_label_object *php_label = (php_label_object *)object;

    MAPSCRIPT_FREE_OBJECT(php_label);

    MAPSCRIPT_DELREF(php_label->parent);
    MAPSCRIPT_DELREF(php_label->color);
    MAPSCRIPT_DELREF(php_label->outlinecolor);
    MAPSCRIPT_DELREF(php_label->shadowcolor);
    MAPSCRIPT_DELREF(php_label->backgroundcolor);
    MAPSCRIPT_DELREF(php_label->backgroundshadowcolor);
    
    /* We don't need to free the labelObj, the mapObj will do it */ 

    efree(object);
}

static zend_object_value mapscript_label_object_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    php_label_object *php_label;

    MAPSCRIPT_ALLOC_OBJECT(php_label, php_label_object);

    retval = mapscript_object_new(&php_label->std, ce,
                                  &mapscript_label_object_destroy);

    php_label->parent = NULL;
    php_label->color = NULL;
    php_label->outlinecolor = NULL;
    php_label->shadowcolor = NULL;
    php_label->backgroundcolor = NULL;
    php_label->backgroundshadowcolor = NULL;

    return retval;
}

PHP_MINIT_FUNCTION(label)
{
    zend_class_entry ce;

    MAPSCRIPT_REGISTER_CLASS("labelObj", 
                             label_functions,
                             mapscript_ce_label,
                             mapscript_label_object_new);

    mapscript_ce_label->ce_flags |= ZEND_ACC_FINAL_CLASS; 
    
    return SUCCESS;
}

