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

zend_class_entry *mapscript_ce_class;
zend_object_handlers mapscript_class_object_handlers;

ZEND_BEGIN_ARG_INFO_EX(class___construct_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_ARG_OBJ_INFO(0, class, classObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_addLabel_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, label, labelObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_removeLabel_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_getLabel_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_updateFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, snippet)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_setExpression_args, 0, 0, 1)
ZEND_ARG_INFO(0, expression)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_setText_args, 0, 0, 1)
ZEND_ARG_INFO(0, text)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_getStyle_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_insertStyle_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, style, styleObj, 0)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_removeStyle_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_moveStyleUp_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_moveStyleDown_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_deleteStyle_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_getMetaData_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_setMetaData_args, 0, 0, 2)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_removeMetaData_args, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_createLegendIcon_args, 0, 0, 2)
ZEND_ARG_INFO(0, width)
ZEND_ARG_INFO(0, height)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(class_drawLegendIcon_args, 0, 0, 5)
ZEND_ARG_INFO(0, width)
ZEND_ARG_INFO(0, height)
ZEND_ARG_OBJ_INFO(0, image, imageObj, 0)
ZEND_ARG_INFO(0, dstX)
ZEND_ARG_INFO(0, dstY)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct(layerObj layer [, classObj class])
   Create a new class instance in the specified layer.. */
PHP_METHOD(classObj, __construct)
{
  zval *zobj = getThis();
  zval *zlayer, *zclass = NULL;
  classObj *class;
  php_layer_object *php_layer;
  php_class_object *php_class, *php_class2;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|O",
                            &zlayer, mapscript_ce_layer,
                            &zclass, mapscript_ce_class) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(zlayer TSRMLS_CC);
  if (zclass)
    php_class2 = (php_class_object *) zend_object_store_get_object(zclass TSRMLS_CC);


  if ((class = classObj_new(php_layer->layer, (zclass ? php_class2->class:NULL))) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  php_class->class = class;

  MAPSCRIPT_MAKE_PARENT(zlayer,NULL);
  php_class->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}
/* }}} */

PHP_METHOD(classObj, __get)
{
  char *property;
  long property_len;
  zval *zobj = getThis();
  php_class_object *php_class;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_STRING("name", php_class->class->name)
  else IF_GET_STRING("title", php_class->class->title)
    else IF_GET_LONG("type", php_class->class->type)
      else IF_GET_LONG("status", php_class->class->status)
        else IF_GET_DOUBLE("minscaledenom", php_class->class->minscaledenom)
          else IF_GET_DOUBLE("maxscaledenom", php_class->class->maxscaledenom)
            else IF_GET_LONG("minfeaturesize", php_class->class->minfeaturesize)
              else IF_GET_LONG("numlabels", php_class->class->numlabels)
                else IF_GET_STRING("template", php_class->class->template)
                  else IF_GET_STRING("keyimage", php_class->class->keyimage)
                    else IF_GET_STRING("group", php_class->class->group)
                      else IF_GET_LONG("numstyles", php_class->class->numstyles)
                        else IF_GET_OBJECT("metadata", mapscript_ce_hashtable, php_class->metadata, &php_class->class->metadata)
                          else IF_GET_OBJECT("leader", mapscript_ce_labelleader, php_class->leader, &php_class->class->leader)
                            else {
                              mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                            }
}

PHP_METHOD(classObj, __set)
{
  char *property;
  long property_len;
  zval *value;
  zval *zobj = getThis();
  php_class_object *php_class;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  /* special case for "template" which we want to set to NULL and not an empty string */
  if(Z_TYPE_P(value)==IS_NULL && !strcmp(property,"template")) {
    msFree(php_class->class->template);
    php_class->class->template = NULL;
  } else
  IF_SET_STRING("name", php_class->class->name, value)
  else IF_SET_STRING("title", php_class->class->title, value)
    else IF_SET_LONG("type", php_class->class->type, value)
      else IF_SET_LONG("status", php_class->class->status, value)
        else IF_SET_DOUBLE("minscaledenom", php_class->class->minscaledenom, value)
          else IF_SET_DOUBLE("maxscaledenom", php_class->class->maxscaledenom, value)
            else IF_SET_LONG("minfeaturesize", php_class->class->minfeaturesize, value)
              else IF_SET_STRING("template", php_class->class->template, value)
                else IF_SET_STRING("keyimage", php_class->class->keyimage, value)
                  else IF_SET_STRING("group", php_class->class->group, value)
                    else if ( (STRING_EQUAL("metadata", property)) ||
                              (STRING_EQUAL("leader", property)) ) {
                      mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                    } else if ( (STRING_EQUAL("numstyles", property)) ||
                                (STRING_EQUAL("numstyles", property)) ) {
                      mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
                    } else {
                      mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                    }
}

/* {{{ proto int addLabel(labelObj *label)
   Add a label to the class.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(classObj, addLabel)
{
  zval *zobj = getThis();
  zval *zlabel;
  php_class_object *php_class;
  php_label_object *php_label;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zlabel, mapscript_ce_label) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_label = (php_label_object *) zend_object_store_get_object(zlabel TSRMLS_CC);

  status = classObj_addLabel(php_class->class, php_label->label);
  php_label->is_ref = 1;

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int removeLabel(int index)
   Removes the label indicated and returns a copy, or NULL in the case of a
   failure. */
PHP_METHOD(classObj, removeLabel)
{
  zval *zobj = getThis();
  long index;
  labelObj *label;
  php_class_object *php_class;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((label = classObj_removeLabel(php_class->class, index)) == NULL) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_NULL();
  }

  /* Return a copy of the class object just removed */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_label(label, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int class.getLabel(int i)
   Returns a labelObj from the class given an index value (0=first label) */
PHP_METHOD(classObj, getLabel)
{
  zval *zobj = getThis();
  long index;
  labelObj *label = NULL;
  php_class_object *php_class;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if ((label = classObj_getLabel(php_class->class, index)) == NULL) {
    mapscript_throw_exception("Invalid label index." TSRMLS_CC);
    return;
  }

  /* Return class object */
  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_label(label, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int updateFromString(string snippet)
   Update a class from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(classObj, updateFromString)
{
  char *snippet;
  long snippet_len;
  zval *zobj = getThis();
  php_class_object *php_class;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = classObj_updateFromString(php_class->class, snippet);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int setExpression(string exression)
   Set the expression string for a class object. */
PHP_METHOD(classObj, setExpression)
{
  char *expression;
  long expression_len;
  zval *zobj = getThis();
  php_class_object *php_class;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &expression, &expression_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = classObj_setExpression(php_class->class, expression);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string getExpressionString()
   Get the expression string for a class object. */
PHP_METHOD(classObj, getExpressionString)
{
  zval *zobj = getThis();
  php_class_object *php_class;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = classObj_getExpressionString(php_class->class);

  if (value == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int setText(string text)
   Set the text string for a class object. */
PHP_METHOD(classObj, setText)
{
  char *text;
  long text_len;
  zval *zobj = getThis();
  php_class_object *php_class;
  php_layer_object *php_layer;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &text, &text_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(php_class->parent.val TSRMLS_CC);

  status = classObj_setText(php_class->class, php_layer->layer, text);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string getTextString()
   Get the text string for a class object. */
PHP_METHOD(classObj, getTextString)
{
  zval *zobj = getThis();
  php_class_object *php_class;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value =  classObj_getTextString(php_class->class);

  if (value == NULL)
    RETURN_STRING("", 1);

  RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto styleObj getstyle(int index)
   return the style object. */
PHP_METHOD(classObj, getStyle)
{
  long index;
  zval *zobj = getThis();
  php_class_object *php_class;
  styleObj *style = NULL;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (index < 0 || index >= php_class->class->numstyles) {
    mapscript_throw_exception("Invalid style index." TSRMLS_CC);
    return;
  }

  style = php_class->class->styles[index];

  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_style(style, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int insertStyle(styleObj style)
   return MS_SUCCESS or MS_FAILURE. */
PHP_METHOD(classObj, insertStyle)
{
  zval *zobj = getThis();
  zval *zstyle = NULL;
  long index = -1;
  php_class_object *php_class;
  php_style_object *php_style;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|l",
                            &zstyle, mapscript_ce_style, &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_style = (php_style_object *) zend_object_store_get_object(zstyle TSRMLS_CC);

  RETURN_LONG(msInsertStyle(php_class->class, php_style->style, index));
}
/* }}} */

/* {{{ proto styleObj removeStyle(int index)
   return the styleObj removed. */
PHP_METHOD(classObj, removeStyle)
{
  zval *zobj = getThis();
  long index;
  styleObj *style;
  php_class_object *php_class;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  style = msRemoveStyle(php_class->class, index);

  /* Return a copy of the class object just removed */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_style(style, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int moveStyleUp(int index) The style specified by the
   style index will be moved up into the array of classes. Returns MS_SUCCESS
   or MS_FAILURE. ex class->movestyleup(1) will have the effect of moving
   style 1 up to position 0, and the style at position 0 will be moved to
   position 1. */
PHP_METHOD(classObj, moveStyleUp)
{
  long index;
  zval *zobj = getThis();
  php_class_object *php_class;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = classObj_moveStyleUp(php_class->class, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int moveStyleDown(int index) The style specified by the
   style index will be moved down into the array of classes. Returns
   MS_SUCCESS or MS_FAILURE. ex class->movestyledown(0) will have the
   effect of moving style 0 up to position 1, and the style at position 1
   will be moved to position 0. */
PHP_METHOD(classObj, moveStyleDown)
{
  long index;
  zval *zobj = getThis();
  php_class_object *php_class;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = classObj_moveStyleDown(php_class->class, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int deleteStyle(int index) Delete the style specified
   by the style index. If there are any style that follow the deleted
 style, their index will decrease by 1. */
PHP_METHOD(classObj, deleteStyle)
{
  long index;
  zval *zobj = getThis();
  php_class_object *php_class;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status =  classObj_deleteStyle(php_class->class, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string getMetaData(string name)
   Return MetaData entry by name, or empty string if not found. */
PHP_METHOD(classObj, getMetaData)
{
  zval *zname;
  zval *zobj = getThis();
  php_class_object *php_class;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",
                            &zname) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_class->metadata, &php_class->class->metadata);

  MAPSCRIPT_CALL_METHOD_1(php_class->metadata, "get", retval, zname);

  RETURN_STRING(Z_STRVAL_P(retval),1);
}
/* }}} */

/* {{{ proto int setMetaData(string name, string value)
   Set MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(classObj, setMetaData)
{
  zval *zname, *zvalue;
  zval *zobj = getThis();
  php_class_object *php_class;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz",
                            &zname, &zvalue) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_class->metadata, &php_class->class->metadata);

  MAPSCRIPT_CALL_METHOD_2(php_class->metadata, "set", retval, zname, zvalue);

  RETURN_LONG(Z_LVAL_P(retval));
}
/* }}} */

/* {{{ proto int removeMetaData(string name)
   Remove MetaData entry by name.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(classObj, removeMetaData)
{
  zval *zname;
  zval *zobj = getThis();
  php_class_object *php_class;
  zval *retval;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",
                            &zname) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  CHECK_OBJECT(mapscript_ce_hashtable, php_class->metadata, &php_class->class->metadata);

  MAPSCRIPT_CALL_METHOD_1(php_class->metadata, "remove", retval, zname);

  RETURN_LONG(Z_LVAL_P(retval));
}
/* }}} */

/* {{{ proto imageObj createLegendIcon(int width, int height)
   Return the legend icon. */
PHP_METHOD(classObj, createLegendIcon)
{
  zval *zobj = getThis();
  long width, height;
  imageObj *image = NULL;
  php_class_object *php_class;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",
                            &width, &height) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(php_class->parent.val TSRMLS_CC);

  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this class object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if ((image = classObj_createLegendIcon(php_class->class,
                                         php_map->map,
                                         php_layer->layer,
                                         width, height)) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* Return an image object */
  mapscript_create_image(image, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int drawLegendIcon(int width, int height, imageObj image, int dstX, int dstY)
   set the lengend icon in img. */
PHP_METHOD(classObj, drawLegendIcon)
{
  zval *zobj = getThis();
  zval *zimage;
  long width, height, dstX, dstY;
  int status = MS_FAILURE;
  php_class_object *php_class;
  php_image_object *php_image;
  php_layer_object *php_layer;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llOll",
                            &width, &height,
                            &zimage, mapscript_ce_image,
                            &dstX, &dstY) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_image = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(php_class->parent.val TSRMLS_CC);


  if (!php_layer->parent.val) {
    mapscript_throw_exception("No map object associated with this class object." TSRMLS_CC);
    return;
  }

  php_map = (php_map_object *) zend_object_store_get_object(php_layer->parent.val TSRMLS_CC);

  if (!MS_RENDERER_PLUGIN(php_image->image->format)) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    mapscript_report_php_error(E_WARNING, "DrawLegendicon function is only available for renderer plugin drivers" TSRMLS_CC);
    RETURN_LONG(MS_FAILURE);
  }

  if ((status = classObj_drawLegendIcon(php_class->class,
                                        php_map->map,
                                        php_layer->layer,
                                        width, height,
                                        php_image->image,
                                        dstX, dstY)) != MS_SUCCESS) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string free()
   Free the object */
PHP_METHOD(classObj, free)
{
  zval *zobj = getThis();
  php_class_object *php_class;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  MAPSCRIPT_DELREF(php_class->metadata);
}
/* }}} */

zend_function_entry class_functions[] = {
  PHP_ME(classObj, __construct, class___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(classObj, __get, class___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, __set, class___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(classObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, addLabel, class_addLabel_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, removeLabel, class_removeLabel_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, getLabel, class_getLabel_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, updateFromString, class_updateFromString_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, setExpression, class_setExpression_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, getExpressionString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, setText, class_setText_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, getTextString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, getStyle, class_getStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, insertStyle, class_insertStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, removeStyle, class_removeStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, moveStyleUp, class_moveStyleUp_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, moveStyleDown, class_moveStyleDown_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, deleteStyle, class_deleteStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, getMetaData, class_getMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, setMetaData, class_setMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, removeMetaData, class_removeMetaData_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, createLegendIcon, class_createLegendIcon_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, drawLegendIcon, class_drawLegendIcon_args, ZEND_ACC_PUBLIC)
  PHP_ME(classObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_class(classObj *class, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_class_object * php_class;
  object_init_ex(return_value, mapscript_ce_class);
  php_class = (php_class_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_class->class = class;

  php_class->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_class_object_destroy(void *object TSRMLS_DC)
{
  php_class_object *php_class = (php_class_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_class);

  MAPSCRIPT_FREE_PARENT(php_class->parent);

  MAPSCRIPT_DELREF(php_class->metadata);
  MAPSCRIPT_DELREF(php_class->leader);

  /* We don't need to free the classObj */

  efree(object);
}

static zend_object_value mapscript_class_object_new_ex(zend_class_entry *ce, php_class_object **ptr TSRMLS_DC)
{
  zend_object_value retval;
  php_class_object *php_class;

  MAPSCRIPT_ALLOC_OBJECT(php_class, php_class_object);

  retval = mapscript_object_new_ex(&php_class->std, ce,
                                   &mapscript_class_object_destroy,
                                   &mapscript_class_object_handlers TSRMLS_CC);

  if (ptr)
    *ptr = php_class;

  MAPSCRIPT_INIT_PARENT(php_class->parent);

  php_class->metadata = NULL;
  php_class->leader = NULL;

  return retval;
}

static zend_object_value mapscript_class_object_new(zend_class_entry *ce TSRMLS_DC)
{
  return mapscript_class_object_new_ex(ce, NULL TSRMLS_CC);
}

static zend_object_value mapscript_class_object_clone(zval *zobj TSRMLS_DC)
{
  php_class_object *php_class_old, *php_class_new;
  php_layer_object *php_layer;
  zend_object_value new_ov;

  php_class_old = (php_class_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_layer = (php_layer_object *) zend_object_store_get_object(php_class_old->parent.val TSRMLS_CC);

  new_ov = mapscript_class_object_new_ex(mapscript_ce_class, &php_class_new TSRMLS_CC);
  zend_objects_clone_members(&php_class_new->std, new_ov, &php_class_old->std, Z_OBJ_HANDLE_P(zobj) TSRMLS_CC);

  php_class_new->class = classObj_clone(php_class_old->class, php_layer->layer);

  return new_ov;
}

PHP_MINIT_FUNCTION(class)
{
  zend_class_entry ce;

  memcpy(&mapscript_class_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mapscript_class_object_handlers.clone_obj = mapscript_class_object_clone;

  MAPSCRIPT_REGISTER_CLASS("classObj",
                           class_functions,
                           mapscript_ce_class,
                           mapscript_class_object_new);

  mapscript_ce_class->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
