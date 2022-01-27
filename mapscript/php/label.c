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
zend_object_handlers mapscript_label_object_handlers;

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

ZEND_BEGIN_ARG_INFO_EX(label_getStyle_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_insertStyle_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, style, styleObj, 0)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_removeStyle_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_moveStyleUp_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_moveStyleDown_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_deleteStyle_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_setExpression_args, 0, 0, 1)
ZEND_ARG_INFO(0, expression)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(label_setText_args, 0, 0, 1)
ZEND_ARG_INFO(0, text)
ZEND_END_ARG_INFO()

/* {{{ proto void __construct()
   Create a new label instance. */
PHP_METHOD(labelObj, __construct)
{
  php_label_object *php_label;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, getThis());

  if ((php_label->label = labelObj_new()) == NULL) {
    mapscript_throw_exception("Unable to construct labelObj." TSRMLS_CC);
    return;
  }
}
/* }}} */

PHP_METHOD(labelObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_label_object *php_label;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  IF_GET_STRING("font",  php_label->label->font)
  else IF_GET_STRING("encoding", php_label->label->encoding)
    else IF_GET_LONG("type", MS_TRUETYPE)
      else IF_GET_LONG("shadowsizex",  php_label->label->shadowsizex)
        else IF_GET_LONG("shadowsizey",  php_label->label->shadowsizey)
          else IF_GET_DOUBLE("size", php_label->label->size)
            else IF_GET_DOUBLE("minsize", php_label->label->minsize)
              else IF_GET_DOUBLE("maxsize", php_label->label->maxsize)
                else IF_GET_DOUBLE("minscaledenom", php_label->label->minscaledenom)
                  else IF_GET_DOUBLE("maxscaledenom", php_label->label->maxscaledenom)
                    else IF_GET_LONG("position", php_label->label->position)
                      else IF_GET_LONG("offsetx", php_label->label->offsetx)
                        else IF_GET_LONG("offsety", php_label->label->offsety)
                          else IF_GET_DOUBLE("angle", php_label->label->angle)
                            else IF_GET_LONG("anglemode", php_label->label->anglemode)
                              else IF_GET_LONG("buffer", php_label->label->buffer)
                                  else IF_GET_LONG("wrap", php_label->label->wrap)
                                    else IF_GET_LONG("minfeaturesize", php_label->label->minfeaturesize)
                                      else IF_GET_LONG("autominfeaturesize", php_label->label->autominfeaturesize)
                                        else IF_GET_LONG("repeatdistance", php_label->label->repeatdistance)
                                          else IF_GET_LONG("numstyles", php_label->label->numstyles)
                                            else IF_GET_LONG("mindistance", php_label->label->mindistance)
                                              else IF_GET_LONG("partials", php_label->label->partials)
                                                else IF_GET_LONG("force", php_label->label->force)
                                                  else IF_GET_LONG("outlinewidth", php_label->label->outlinewidth)
                                                    else IF_GET_LONG("align", php_label->label->align)
                                                      else IF_GET_LONG("maxlength", php_label->label->maxlength)
                                                        else IF_GET_LONG("maxoverlapangle", php_label->label->maxoverlapangle)
                                                          else IF_GET_LONG("priority", php_label->label->priority)
                                                            else IF_GET_OBJECT("color", mapscript_ce_color, php_label->color, &php_label->label->color)
                                                              else IF_GET_OBJECT("leader", mapscript_ce_labelleader, php_label->leader, php_label->label->leader)
                                                                else IF_GET_OBJECT("outlinecolor", mapscript_ce_color, php_label->outlinecolor, &php_label->label->outlinecolor)
                                                                  else IF_GET_OBJECT("shadowcolor", mapscript_ce_color, php_label->shadowcolor, &php_label->label->shadowcolor)
                                                                    else {
                                                                      mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                                                    }
}

PHP_METHOD(labelObj, __set)
{
  char *property;
  long property_len = 0;
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

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  IF_SET_STRING("font",  php_label->label->font, value)
  else IF_SET_STRING("encoding", php_label->label->encoding, value)
      else IF_SET_LONG("shadowsizex",  php_label->label->shadowsizex, value)
        else IF_SET_LONG("shadowsizey",  php_label->label->shadowsizey, value)
          else IF_SET_DOUBLE("size", php_label->label->size, value)
            else IF_SET_DOUBLE("minsize", php_label->label->minsize, value)
              else IF_SET_DOUBLE("maxsize", php_label->label->maxsize, value)
                else IF_SET_LONG("position", php_label->label->position, value)
                  else IF_SET_LONG("offsetx", php_label->label->offsetx, value)
                    else IF_SET_LONG("offsety", php_label->label->offsety, value)
                      else IF_SET_DOUBLE("angle", php_label->label->angle, value)
                        else IF_SET_LONG("anglemode", php_label->label->anglemode, value)
                          else IF_SET_LONG("buffer", php_label->label->buffer, value)
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
                                                  else IF_SET_LONG("maxoverlapangle", php_label->label->maxoverlapangle, value)
                                                    else IF_SET_LONG("priority", php_label->label->priority, value)
                                                      else IF_SET_DOUBLE("maxscaledenom", php_label->label->maxscaledenom, value)
                                                        else IF_SET_DOUBLE("minscaledenom", php_label->label->minscaledenom, value)
                                                          else if ( (STRING_EQUAL("color", property)) ||
                                                                    (STRING_EQUAL("outlinecolor", property)) ||
                                                                    (STRING_EQUAL("leader", property)) ||
                                                                    (STRING_EQUAL("shadowcolor", property)) ) {
                                                            mapscript_throw_exception("Property '%s' is an object and can only be modified through its accessors." TSRMLS_CC, property);
                                                          } else if (STRING_EQUAL("numstyles", property)) {
                                                            mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
                                                          } else {
                                                            mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                                                          }

}

/* {{{ proto int label.updateFromString(string snippet)
   Update a label from a string snippet.  Returns MS_SUCCESS/MS_FAILURE */
PHP_METHOD(labelObj, updateFromString)
{
  zval *zobj = getThis();
  char *snippet;
  long snippet_len = 0;
  int status = MS_FAILURE;
  php_label_object *php_label;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &snippet, &snippet_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  if ((status = labelObj_updateFromString(php_label->label, snippet)) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string convertToString()
   Convert the label object to string. */
PHP_METHOD(labelObj, convertToString)
{
  zval *zobj = getThis();
  php_label_object *php_label;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  value =  labelObj_convertToString(php_label->label);

  if (value == NULL)
    MAPSCRIPT_RETURN_STRING("", 1);

  MAPSCRIPT_RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int label.setbinding(const bindingid, string value)
   Set the attribute binding for a specfiled label property. Returns MS_SUCCESS on success. */
PHP_METHOD(labelObj, setBinding)
{
  zval *zobj = getThis();
  char *value;
  long value_len = 0;
  long bindingId;
  php_label_object *php_label;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls",
                            &bindingId, &value, &value_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  if (bindingId < 0 || bindingId >= MS_LABEL_BINDING_LENGTH) {
    mapscript_throw_exception("Invalid binding id." TSRMLS_CC);
    return;
  }

  if (!value || strlen(value) == 0) {
    mapscript_throw_exception("Invalid binding value." TSRMLS_CC);
    return;
  }

  if(php_label->label->bindings[bindingId].item) {
    msFree(php_label->label->bindings[bindingId].item);
    php_label->label->bindings[bindingId].index = -1;
    php_label->label->numbindings--;
  }

  php_label->label->bindings[bindingId].item = msStrdup(value);
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
  long bindingId;
  char *value = NULL;
  php_label_object *php_label;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &bindingId) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  if (bindingId < 0 || bindingId >= MS_LABEL_BINDING_LENGTH) {
    mapscript_throw_exception("Invalid binding id." TSRMLS_CC);
    return;
  }

  if( (value = php_label->label->bindings[bindingId].item) != NULL) {
    MAPSCRIPT_RETURN_STRING(value, 1);
  }

  RETURN_NULL();

}
/* }}} */

/* {{{ proto int label.removebinding(const bindingid)
   Remove attribute binding for a specfiled label property. Returns MS_SUCCESS on success. */
PHP_METHOD(labelObj, removeBinding)
{
  zval *zobj = getThis();
  long bindingId;
  php_label_object *php_label;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &bindingId) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  if (bindingId < 0 || bindingId >= MS_LABEL_BINDING_LENGTH) {
    mapscript_throw_exception("Invalid binding id." TSRMLS_CC);
    return;
  }


  if(php_label->label->bindings[bindingId].item) {
    msFree(php_label->label->bindings[bindingId].item);
    php_label->label->bindings[bindingId].item =  NULL;
    php_label->label->bindings[bindingId].index = -1;
    php_label->label->numbindings--;
  }

  RETURN_LONG(MS_SUCCESS);

}
/* }}} */

/* {{{ proto int getstyle(int index)
   return the style object. */
PHP_METHOD(labelObj, getStyle)
{
  long index;
  zval *zobj = getThis();
  php_label_object *php_label;
  styleObj *style = NULL;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  if (index < 0 || index >= php_label->label->numstyles) {
    mapscript_throw_exception("Invalid style index." TSRMLS_CC);
    return;
  }

  style = php_label->label->styles[index];

  MAPSCRIPT_MAKE_PARENT(zobj, NULL);
  mapscript_create_style(style, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int insertStyle(styleObj style)
   return MS_SUCCESS or MS_FAILURE. */
PHP_METHOD(labelObj, insertStyle)
{
  zval *zobj = getThis();
  zval *zstyle = NULL;
  long index = -1;
  php_label_object *php_label;
  php_style_object *php_style;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|l",
                            &zstyle, mapscript_ce_style, &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);
  php_style = MAPSCRIPT_OBJ_P(php_style_object, zstyle);

  RETURN_LONG(msInsertLabelStyle(php_label->label, php_style->style, index));
}
/* }}} */

/* {{{ proto styleObj removeStyle(int index)
   return the styleObj removed. */
PHP_METHOD(labelObj, removeStyle)
{
  zval *zobj = getThis();
  long index;
  styleObj *style;
  php_label_object *php_label;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  style = msRemoveLabelStyle(php_label->label, index);

  /* Return a copy of the class object just removed */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_style(style, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int moveStyleUp(int index)  */
PHP_METHOD(labelObj, moveStyleUp)
{
  long index;
  zval *zobj = getThis();
  php_label_object *php_label;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  status = labelObj_moveStyleUp(php_label->label, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int moveStyleDown(int index) */
PHP_METHOD(labelObj, moveStyleDown)
{
  long index;
  zval *zobj = getThis();
  php_label_object *php_label;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  status = labelObj_moveStyleDown(php_label->label, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int deleteStyle(int index) */
PHP_METHOD(labelObj, deleteStyle)
{
  long index;
  zval *zobj = getThis();
  php_label_object *php_label;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  status =  labelObj_deleteStyle(php_label->label, index);

  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto int label.free()
   Free the object */
PHP_METHOD(labelObj, free)
{
  zval *zobj = getThis();
  php_label_object *php_label;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  MAPSCRIPT_DELREF(php_label->color);
  MAPSCRIPT_DELREF(php_label->outlinecolor);
  MAPSCRIPT_DELREF(php_label->shadowcolor);
}
/* }}} */

/* {{{ proto int setExpression(string exression)
   Set the expression string for a label object. */
PHP_METHOD(labelObj, setExpression)
{
  char *expression;
  long expression_len;
  zval *zobj = getThis();
  php_label_object *php_label;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &expression, &expression_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  status = labelObj_setExpression(php_label->label, expression);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string getExpressionString()
   Get the expression string for a label object. */
PHP_METHOD(labelObj, getExpressionString)
{
  zval *zobj = getThis();
  php_label_object *php_label;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  value = labelObj_getExpressionString(php_label->label);

  if (value == NULL)
    MAPSCRIPT_RETURN_STRING("", 1);

  MAPSCRIPT_RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

/* {{{ proto int setText(string text)
   Set the text string for a label object. */
PHP_METHOD(labelObj, setText)
{
  char *text;
  long text_len;
  zval *zobj = getThis();
  php_label_object *php_label;
  php_layer_object *php_layer;
  int status = MS_FAILURE;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &text, &text_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);
  php_layer = MAPSCRIPT_OBJ(php_layer_object, php_label->parent.val);

  status = labelObj_setText(php_label->label, php_layer->layer, text);

  if (status != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }


  RETURN_LONG(status);
}
/* }}} */

/* {{{ proto string getTextString()
   Get the text string for a label object. */
PHP_METHOD(labelObj, getTextString)
{
  zval *zobj = getThis();
  php_label_object *php_label;
  char *value = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_label = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  value =  labelObj_getTextString(php_label->label);

  if (value == NULL)
    MAPSCRIPT_RETURN_STRING("", 1);

  MAPSCRIPT_RETVAL_STRING(value, 1);
  free(value);
}
/* }}} */

zend_function_entry label_functions[] = {
  PHP_ME(labelObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(labelObj, __get, label___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, __set, label___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(labelObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, updateFromString, label_updateFromString_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, convertToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, setBinding, label_setBinding_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, getBinding, label_getBinding_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, removeBinding, label_removeBinding_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, getStyle, label_getStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, insertStyle, label_insertStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, removeStyle, label_removeStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, moveStyleUp, label_moveStyleUp_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, moveStyleDown, label_moveStyleDown_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, deleteStyle, label_deleteStyle_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, setExpression, label_setExpression_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, getExpressionString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, setText, label_setText_args, ZEND_ACC_PUBLIC)
  PHP_ME(labelObj, getTextString, NULL, ZEND_ACC_PUBLIC)  
  PHP_ME(labelObj, free, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_label(labelObj *label, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_label_object * php_label;
  object_init_ex(return_value, mapscript_ce_label);
  php_label = MAPSCRIPT_OBJ_P(php_label_object, return_value);
  php_label->label = label;

  if(ZVAL_NOT_UNDEF(parent.val))
    php_label->is_ref = 1;

  php_label->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

#if PHP_VERSION_ID >= 70000
/* PHP7 - Modification by Bjoern Boldt <mapscript@pixaweb.net> */
static zend_object *mapscript_label_create_object(zend_class_entry *ce TSRMLS_DC)
{
  php_label_object *php_label;

  php_label = ecalloc(1, sizeof(*php_label) + zend_object_properties_size(ce));

  zend_object_std_init(&php_label->zobj, ce TSRMLS_CC);
  object_properties_init(&php_label->zobj, ce);

  php_label->zobj.handlers = &mapscript_label_object_handlers;

  MAPSCRIPT_INIT_PARENT(php_label->parent);
  php_label->is_ref = 0;
  ZVAL_UNDEF(&php_label->color);
  ZVAL_UNDEF(&php_label->outlinecolor);
  ZVAL_UNDEF(&php_label->shadowcolor);
  ZVAL_UNDEF(&php_label->leader);


  return &php_label->zobj;
}

static void mapscript_label_free_object(zend_object *object)
{
  php_label_object *php_label;

  php_label = (php_label_object *)((char *)object - XtOffsetOf(php_label_object, zobj));

  MAPSCRIPT_FREE_PARENT(php_label->parent);
  MAPSCRIPT_DELREF(php_label->color);
  MAPSCRIPT_DELREF(php_label->outlinecolor);
  MAPSCRIPT_DELREF(php_label->shadowcolor);
  MAPSCRIPT_DELREF(php_label->leader);

  if (php_label->label && !php_label->is_ref) {
    labelObj_destroy(php_label->label);
  }

  zend_object_std_dtor(object);
}

static zend_object* mapscript_label_clone_object(zval *zobj)
{
  php_label_object *php_label_old, *php_label_new;
  zend_object* zobj_new;

  php_label_old = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  zobj_new = mapscript_label_create_object(mapscript_ce_label);
  php_label_new = (php_label_object *)((char *)zobj_new - XtOffsetOf(php_label_object, zobj));

  php_label_new->label = labelObj_clone(php_label_old->label);

  return zobj_new;
}

PHP_MINIT_FUNCTION(label)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "labelObj", label_functions);
  mapscript_ce_label = zend_register_internal_class(&ce TSRMLS_CC);

  mapscript_ce_label->create_object = mapscript_label_create_object;
  mapscript_ce_label->ce_flags |= ZEND_ACC_FINAL;

  memcpy(&mapscript_label_object_handlers, &mapscript_std_object_handlers, sizeof(mapscript_label_object_handlers));
  mapscript_label_object_handlers.free_obj = mapscript_label_free_object;
  mapscript_label_object_handlers.clone_obj = mapscript_label_clone_object;
  mapscript_label_object_handlers.offset   = XtOffsetOf(php_label_object, zobj);

  return SUCCESS;
}
#else
/* PHP5 */
static void mapscript_label_object_destroy(void *object TSRMLS_DC)
{
  php_label_object *php_label = (php_label_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_label);

  MAPSCRIPT_FREE_PARENT(php_label->parent);
  MAPSCRIPT_DELREF(php_label->color);
  MAPSCRIPT_DELREF(php_label->outlinecolor);
  MAPSCRIPT_DELREF(php_label->shadowcolor);
  MAPSCRIPT_DELREF(php_label->leader);

  if (php_label->label && !php_label->is_ref) {
    labelObj_destroy(php_label->label);
  }

  efree(object);
}

static zend_object_value mapscript_label_object_new_ex(zend_class_entry *ce, php_label_object **ptr TSRMLS_DC)
{
  zend_object_value retval;
  php_label_object *php_label;

  MAPSCRIPT_ALLOC_OBJECT(php_label, php_label_object);

  retval = mapscript_object_new_ex(&php_label->std, ce,
                                   &mapscript_label_object_destroy,
                                   &mapscript_label_object_handlers TSRMLS_CC);

  if (ptr)
    *ptr = php_label;

  php_label->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_label->parent);
  php_label->color = NULL;
  php_label->outlinecolor = NULL;
  php_label->shadowcolor = NULL;
  php_label->leader = NULL;

  return retval;
}

static zend_object_value mapscript_label_object_new(zend_class_entry *ce TSRMLS_DC)
{
  return mapscript_label_object_new_ex(ce, NULL TSRMLS_CC);
}

static zend_object_value mapscript_label_object_clone(zval *zobj TSRMLS_DC)
{
  php_label_object *php_label_old, *php_label_new;
  zend_object_value new_ov;

  php_label_old = MAPSCRIPT_OBJ_P(php_label_object, zobj);

  new_ov = mapscript_label_object_new_ex(mapscript_ce_label, &php_label_new TSRMLS_CC);
  zend_objects_clone_members(&php_label_new->std, new_ov, &php_label_old->std, Z_OBJ_HANDLE_P(zobj) TSRMLS_CC);

  php_label_new->label = labelObj_clone(php_label_old->label);

  return new_ov;
}

PHP_MINIT_FUNCTION(label)
{
  zend_class_entry ce;

  memcpy(&mapscript_label_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mapscript_label_object_handlers.clone_obj = mapscript_label_object_clone;

  MAPSCRIPT_REGISTER_CLASS("labelObj",
                           label_functions,
                           mapscript_ce_label,
                           mapscript_label_object_new);

  mapscript_ce_label->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
#endif
