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

zend_class_entry *mapscript_ce_style;

ZEND_BEGIN_ARG_INFO_EX(style___construct_args, 0, 0, 1)
  ZEND_ARG_OBJ_INFO(0, class, classObj, 0)
  ZEND_ARG_OBJ_INFO(0, style, styleObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(style___get_args, 0, 0, 1)
  ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(style___set_args, 0, 0, 2)
  ZEND_ARG_INFO(0, property)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(style_updateFromString_args, 0, 0, 1)
  ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(style_setBinding_args, 0, 0, 2)
  ZEND_ARG_INFO(0, styleBinding)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(style_getBinding_args, 0, 0, 1)
  ZEND_ARG_INFO(0, styleBinding)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(style_removeBinding_args, 0, 0, 1)
  ZEND_ARG_INFO(0, styleBinding)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct(classObj class [, styleObj style]) 
   Create a new styleObj instance */
PHP_METHOD(styleObj, __construct)
{
    zval *zobj = getThis();
    zval *zclass, *zstyle = NULL;
    styleObj *style;
    php_class_object *php_class;
    php_style_object *php_style, *php_style2;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|O",
                              &zclass, mapscript_ce_class,
                              &zstyle, mapscript_ce_style) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

    php_style = (php_style_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    php_class = (php_class_object *)zend_object_store_get_object(zclass TSRMLS_CC);
    if (zstyle)
        php_style2 = (php_style_object *)zend_object_store_get_object(zstyle TSRMLS_CC);

    if ((style = styleObj_new(php_class->class, (zstyle ? php_style2->style : NULL))) == NULL)
    {
        mapscript_throw_mapserver_exception("" TSRMLS_CC);
        return;
    }

    php_style->style = style;
    
    MAPSCRIPT_MAKE_PARENT(zclass, NULL);
    php_style->parent = parent;
    MAPSCRIPT_ADDREF(zclass);  
}
/* }}} */

PHP_METHOD(styleObj, __get)
{
    char *property;
    long property_len;
    zval *zobj = getThis();
    php_style_object *php_style;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                              &property, &property_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    IF_GET_LONG("symbol", php_style->style->symbol)
    else IF_GET_STRING("symbolname", php_style->style->symbolname)
    else IF_GET_DOUBLE("size", php_style->style->size)
    else IF_GET_DOUBLE("minsize", php_style->style->minsize)
    else IF_GET_DOUBLE("maxsize", php_style->style->maxsize)
    else IF_GET_DOUBLE("width", php_style->style->width)
    else IF_GET_DOUBLE("minwidth", php_style->style->minwidth)
    else IF_GET_DOUBLE("maxwidth", php_style->style->maxwidth)
    else IF_GET_LONG("offsetx", php_style->style->offsetx)
    else IF_GET_LONG("offsety", php_style->style->offsety)
    else IF_GET_DOUBLE("angle", php_style->style->angle)
    else IF_GET_LONG("antialias", php_style->style->antialias)
    else IF_GET_DOUBLE("minvalue", php_style->style->minvalue)
    else IF_GET_DOUBLE("maxvalue", php_style->style->maxvalue)
    else IF_GET_STRING("rangeitem", php_style->style->rangeitem)
    else IF_GET_LONG("opacity", php_style->style->opacity)
    else IF_GET_OBJECT("color", mapscript_ce_color, php_style->color, &php_style->style->color) 
    else IF_GET_OBJECT("outlinecolor", mapscript_ce_color, php_style->outlinecolor, &php_style->style->outlinecolor) 
    else IF_GET_OBJECT("backgroundcolor", mapscript_ce_color, php_style->backgroundcolor, &php_style->style->backgroundcolor) 
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
    }
/*
    MAKE_STD_ZVAL(php_style->color);
    mapscript_create_color(&(style->color), return_value, php_style->color TSRMLS_CC);

    MAKE_STD_ZVAL(php_style->outlinecolor);
    mapscript_create_color(&(style->outlinecolor), return_value, php_style->outlinecolor TSRMLS_CC);

    MAKE_STD_ZVAL(php_style->backgroundcolor);
    mapscript_create_color(&(style->backgroundcolor), return_value, php_style->backgroundcolor TSRMLS_CC);

 */
}

PHP_METHOD(styleObj, __set)
{
    char *property;
    long property_len;
    zval *value;
    zval *zobj = getThis();
    php_style_object *php_style;
    php_map_object *php_map;
    php_class_object *php_class;
    php_layer_object *php_layer;
    php_labelcachemember_object *php_labelcachemember;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                              &property, &property_len, &value) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);
    
    IF_SET_LONG("symbol", php_style->style->symbol, value)
    else IF_SET_STRING("symbolname", php_style->style->symbolname, value)
    else IF_SET_DOUBLE("size", php_style->style->size, value)
    else IF_SET_DOUBLE("minsize", php_style->style->minsize, value)
    else IF_SET_DOUBLE("maxsize", php_style->style->maxsize, value)
    else IF_SET_DOUBLE("width", php_style->style->width, value)
    else IF_SET_DOUBLE("minwidth", php_style->style->minwidth, value)
    else IF_SET_DOUBLE("maxwidth", php_style->style->maxwidth, value)
    else IF_SET_LONG("offsetx", php_style->style->offsetx, value)
    else IF_SET_LONG("offsety", php_style->style->offsety, value)
    else IF_SET_DOUBLE("angle", php_style->style->angle, value)
    else IF_SET_LONG("antialias", php_style->style->antialias, value)
    else IF_SET_DOUBLE("minvalue", php_style->style->minvalue, value)
    else IF_SET_DOUBLE("maxvalue", php_style->style->maxvalue, value)
    else IF_SET_STRING("rangeitem", php_style->style->rangeitem, value)
    else IF_SET_LONG("opacity", php_style->style->opacity, value)
    else if (STRING_EQUAL("symbolname", property))
    {
        /* The parent can be a classObj or a labelCacheMemberObj */
        if (Z_OBJCE_P(php_style->parent.val) == mapscript_ce_class)
        {
            php_class = (php_class_object *) zend_object_store_get_object(php_style->parent.val TSRMLS_CC); 
            /* Can a class have no layer object ? */
            php_layer = (php_layer_object *) zend_object_store_get_object(php_class->parent.val TSRMLS_CC);
            if (!php_layer->parent.val)
            {
                mapscript_throw_exception("No map object associated with this style object." TSRMLS_CC);
                return;
            }
            php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);
        }
        else if (Z_OBJCE_P(php_style->parent.val) == mapscript_ce_labelcachemember)
        {
            /* The parent is always a map */
            php_labelcachemember = (php_labelcachemember_object *) zend_object_store_get_object(php_style->parent.val TSRMLS_CC);
            if (!php_labelcachemember->parent.val)
            {
                mapscript_throw_exception("No map object associated with this style object." TSRMLS_CC);
                return;
            }
            php_map = (php_map_object *) zend_object_store_get_object(php_labelcachemember->parent.val TSRMLS_CC);
        }

        styleObj_setSymbolByName(php_style->style,
                                 php_map->map,
                                 php_style->style->symbolname);
    }
    else if ( (STRING_EQUAL("color", property)) ||
              (STRING_EQUAL("outlinecolor", property)) ||
              (STRING_EQUAL("backgroundcolor", property)))
    {
        mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
    }
    else 
    {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
    }
         
}

PHP_METHOD(styleObj, __clone)
{
    zval *zobj = getThis();
    styleObj *style;
    php_style_object *php_style;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters_none() == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);
    
    if ((style = styleObj_clone(php_style->style)) == NULL)
    {
        mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
        RETURN_NULL();
    }
    
    /* setting the class as parent .... but should not */
    mapscript_create_style(style, php_style->parent, return_value TSRMLS_CC);
}

/* {{{ proto int style.updateFromString(string snippet)
   Update a style from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(styleObj, updateFromString)
{
    zval *zobj = getThis();
    char *snippet;
    long snippet_len;
    int status = MS_FAILURE;
    php_style_object *php_style;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                              &snippet, &snippet_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if ((status = styleObj_updateFromString(php_style->style, snippet)) != MS_SUCCESS)
    {
        mapscript_throw_mapserver_exception("" TSRMLS_CC);
        return;
    }

    RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int style.setbinding(const bindingid, string value)
   Set the attribute binding for a specfiled style property. Returns MS_SUCCESS on success. */
PHP_METHOD(styleObj, setBinding)
{
    zval *zobj = getThis();
    char *value;
    long value_len;
    long bindingId;
    php_style_object *php_style;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls",
                              &bindingId, &value, &value_len) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if (bindingId < 0 || bindingId > MS_STYLE_BINDING_LENGTH)
     {
        mapscript_throw_exception("Invalid binding id." TSRMLS_CC);
        return;
     }

    if (!value || strlen(value) <= 0)
    {
        mapscript_throw_exception("Invalid binding value." TSRMLS_CC);
        return;
    }

    if(php_style->style->bindings[bindingId].item) 
    {
        msFree(php_style->style->bindings[bindingId].item);
        php_style->style->bindings[bindingId].index = -1; 
        php_style->style->numbindings--;
    }

    php_style->style->bindings[bindingId].item = strdup(value); 
    php_style->style->numbindings++;

    RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int style.getbinding(const bindingid)
   Get the value of a attribute binding for a specfiled style property. 
   Returns the string value if exist, else null. */
PHP_METHOD(styleObj, getBinding)
{
    zval *zobj = getThis();
    long bindingId;
    char *value = NULL;
    php_style_object *php_style;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                              &bindingId) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if (bindingId < 0 || bindingId > MS_STYLE_BINDING_LENGTH)
    {
        mapscript_throw_exception("Invalid binding id." TSRMLS_CC);
        return;
    }

    if( (value = php_style->style->bindings[bindingId].item) != NULL)
    {
       RETURN_STRING(value, 1);                
    }

    RETURN_NULL();
    
}
/* }}} */

/* {{{ proto int style.removebinding(const bindingid)
   Remove attribute binding for a specfiled style property. Returns MS_SUCCESS on success. */
PHP_METHOD(styleObj, removeBinding)
{
    zval *zobj = getThis();
    long bindingId;
    php_style_object *php_style;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                              &bindingId) == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    if (bindingId < 0 || bindingId > MS_STYLE_BINDING_LENGTH)
    {
        mapscript_throw_exception("Invalid binding id." TSRMLS_CC);
        return;
    }


    if(php_style->style->bindings[bindingId].item) 
    {
        msFree(php_style->style->bindings[bindingId].item);
        php_style->style->bindings[bindingId].item =  NULL;
        php_style->style->bindings[bindingId].index = -1; 
        php_style->style->numbindings--;
    }

    RETURN_LONG(MS_SUCCESS);
    
}
/* }}} */

/* {{{ proto int style.free()
   Free the object */
PHP_METHOD(styleObj, free)
{
    zval *zobj = getThis();
    php_style_object *php_style;

    PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
    if (zend_parse_parameters_none() == FAILURE) {
        PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
        return;
    }
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    
    php_style = (php_style_object *) zend_object_store_get_object(zobj TSRMLS_CC);

    MAPSCRIPT_DELREF(php_style->color);
    MAPSCRIPT_DELREF(php_style->outlinecolor);
    MAPSCRIPT_DELREF(php_style->backgroundcolor);
}
/* }}} */

zend_function_entry style_functions[] = {
    PHP_ME(styleObj, __construct, style___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(styleObj, __get, style___get_args, ZEND_ACC_PUBLIC)
    PHP_ME(styleObj, __set, style___set_args, ZEND_ACC_PUBLIC)
    PHP_MALIAS(styleObj, set, __set, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(styleObj, __clone, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(styleObj, clone, __clone, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(styleObj, updateFromString, style_updateFromString_args, ZEND_ACC_PUBLIC)
    PHP_ME(styleObj, setBinding, style_setBinding_args, ZEND_ACC_PUBLIC)
    PHP_ME(styleObj, getBinding, style_getBinding_args, ZEND_ACC_PUBLIC)
    PHP_ME(styleObj, removeBinding, style_removeBinding_args, ZEND_ACC_PUBLIC)
    PHP_ME(styleObj, free, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


void mapscript_create_style(styleObj *style, parent_object parent, zval *return_value TSRMLS_DC)
{
    php_style_object * php_style;
    object_init_ex(return_value, mapscript_ce_style); 
    php_style = (php_style_object *)zend_object_store_get_object(return_value TSRMLS_CC);
    php_style->style = style;

    php_style->parent = parent;
    
    MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_style_object_destroy(void *object TSRMLS_DC)
{
    php_style_object *php_style = (php_style_object *)object;

    MAPSCRIPT_FREE_OBJECT(php_style);

    MAPSCRIPT_FREE_PARENT(php_style->parent);
    MAPSCRIPT_DELREF(php_style->color);
    MAPSCRIPT_DELREF(php_style->outlinecolor);
    MAPSCRIPT_DELREF(php_style->backgroundcolor);

    /* We don't need to free the styleObj, the mapObj will do it */ 
    efree(object);
}

static zend_object_value mapscript_style_object_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    php_style_object *php_style;

    MAPSCRIPT_ALLOC_OBJECT(php_style, php_style_object);

    retval = mapscript_object_new(&php_style->std, ce,
                                  &mapscript_style_object_destroy TSRMLS_CC);

    MAPSCRIPT_INIT_PARENT(php_style->parent);
    php_style->color = NULL;
    php_style->outlinecolor = NULL;
    php_style->backgroundcolor = NULL;

    return retval;
}

PHP_MINIT_FUNCTION(style)
{
    zend_class_entry ce;

    MAPSCRIPT_REGISTER_CLASS("styleObj", 
                             style_functions,
                             mapscript_ce_style,
                             mapscript_style_object_new);

    mapscript_ce_style->ce_flags |= ZEND_ACC_FINAL_CLASS; 
    
    return SUCCESS;
}

