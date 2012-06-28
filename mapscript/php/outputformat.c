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

zend_class_entry *mapscript_ce_outputformat;

ZEND_BEGIN_ARG_INFO_EX(outputformat___construct_args, 0, 0, 1)
ZEND_ARG_INFO(0, driver)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(outputformat___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)

ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(outputformat___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(outputformat_setOption_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(outputformat_getOption_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(outputformat_getOptionByIndex_args, 0, 0, 1)
ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

/* {{{ proto outputformat __construct()
   instanciate outputFormatObj */
PHP_METHOD(outputFormatObj, __construct)
{
  zval *zobj = getThis();
  php_outputformat_object *php_outputformat;
  char *driver;
  long driver_len;
  char *name = NULL;
  long name_len;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                            &driver, &driver_len, &name, &name_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_outputformat = (php_outputformat_object *)zend_object_store_get_object(zobj TSRMLS_CC);

  if ((php_outputformat->outputformat = outputFormatObj_new(driver, name)) == NULL) {
    mapscript_throw_exception("Unable to construct outputFormatObj." TSRMLS_CC);
    return;
  }
}
/* }}} */

PHP_METHOD(outputFormatObj, __get)
{
  char *property;
  long property_len;
  zval *zobj = getThis();
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_outputformat = (php_outputformat_object *) zend_object_store_get_object(zobj TSRMLS_CC);


  IF_GET_STRING("name", php_outputformat->outputformat->name)
  else IF_GET_STRING("mimetype", php_outputformat->outputformat->mimetype)
    else IF_GET_STRING("driver", php_outputformat->outputformat->driver)
      else IF_GET_STRING("extension", php_outputformat->outputformat->extension)
        else IF_GET_LONG("renderer", php_outputformat->outputformat->renderer)
          else IF_GET_LONG("imagemode", php_outputformat->outputformat->imagemode)
            else IF_GET_LONG("transparent", php_outputformat->outputformat->transparent)
              else IF_GET_LONG("bands", php_outputformat->outputformat->bands)
                else IF_GET_LONG("numformatoptions", php_outputformat->outputformat->numformatoptions)
                  else {
                    mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
                  }
}

PHP_METHOD(outputFormatObj, __set)
{
  char *property;
  long property_len;
  zval *value;
  zval *zobj = getThis();
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_outputformat = (php_outputformat_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_STRING("name", php_outputformat->outputformat->name, value)
  else IF_SET_STRING("mimetype", php_outputformat->outputformat->mimetype, value)
    else IF_SET_STRING("driver", php_outputformat->outputformat->driver, value)
      else IF_SET_STRING("extension", php_outputformat->outputformat->extension, value)
        else IF_SET_LONG("renderer", php_outputformat->outputformat->renderer, value)
          else IF_SET_LONG("imagemode", php_outputformat->outputformat->imagemode, value)
            else IF_SET_LONG("transparent", php_outputformat->outputformat->transparent, value)
              else if ( (STRING_EQUAL("bands", property)) ||
                        (STRING_EQUAL("numformatoptions", property)) ) {
                mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
              }

              else {
                mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
              }
}

/* {{{ proto int outputFormat.setOption(string property_name, new_value)
   Add or Modify the format option list. return true on success.*/
PHP_METHOD(outputFormatObj, setOption)
{
  char *property;
  long property_len;
  char *value;
  long value_len;
  zval *zobj = getThis();
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                            &property, &property_len, &value, &value_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_outputformat = (php_outputformat_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  msSetOutputFormatOption(php_outputformat->outputformat, property, value);

  RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto string outputFormat.getOption(string property_name)
   Returns the associated value for the format option property passed as
   argument. Returns an empty string if property not found.*/
PHP_METHOD(outputFormatObj, getOption)
{
  char *property;
  long property_len;
  zval *zobj = getThis();
  const char *value = NULL;
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_outputformat = (php_outputformat_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  value = msGetOutputFormatOption(php_outputformat->outputformat, property, "");

  RETURN_STRING((char*)value,1);
}
/* }}} */

/* {{{ proto int outputFormat.validate()
   Checks some internal consistency issues, and returns MS_SUCCESS if things
   are OK and MS_FAILURE if there are problems. Some problems are fixed up
   internally. May produce debug output if issues encountered.*/
PHP_METHOD(outputFormatObj, validate)
{
  zval *zobj = getThis();
  int status = MS_FALSE;
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_outputformat = (php_outputformat_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  status = msOutputFormatValidate(php_outputformat->outputformat, MS_TRUE);
  if (status != MS_TRUE) {
    mapscript_report_mapserver_error(E_WARNING TSRMLS_CC);
    RETURN_LONG(MS_FAILURE);
  } else
    RETURN_LONG(MS_SUCCESS);
}
/* }}} */

/* {{{ proto int outputformat.getOptionByIndex(int index).
   Return the option at index position. */
PHP_METHOD(outputFormatObj, getOptionByIndex)
{
  zval *zobj = getThis();
  long index = -1;
  php_outputformat_object *php_outputformat;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &index) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_outputformat = (php_outputformat_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  if (index < 0 || index >= php_outputformat->outputformat->numformatoptions) {
    mapscript_throw_mapserver_exception("Invalid format option index." TSRMLS_CC);
    return;
  }

  RETURN_STRING(php_outputformat->outputformat->formatoptions[index],1);
}
/* }}} */

zend_function_entry outputformat_functions[] = {
  PHP_ME(outputFormatObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(outputFormatObj, __get, outputformat___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(outputFormatObj, __set, outputformat___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(outputFormatObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(outputFormatObj, setOption, outputformat_setOption_args, ZEND_ACC_PUBLIC)
  PHP_ME(outputFormatObj, getOption, outputformat_getOption_args, ZEND_ACC_PUBLIC)
  PHP_ME(outputFormatObj, getOptionByIndex, outputformat_getOptionByIndex_args, ZEND_ACC_PUBLIC)
  PHP_ME(outputFormatObj, validate, NULL, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};

void mapscript_create_outputformat(outputFormatObj *outputformat, parent_object parent, zval *return_value TSRMLS_DC)
{
  php_outputformat_object * php_outputformat;
  object_init_ex(return_value, mapscript_ce_outputformat);
  php_outputformat = (php_outputformat_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_outputformat->outputformat = outputformat;

  if (parent.val)
    php_outputformat->is_ref = 1;

  php_outputformat->parent = parent;
  MAPSCRIPT_ADDREF(parent.val);
}

static void mapscript_outputformat_object_destroy(void *object TSRMLS_DC)
{
  php_outputformat_object *php_outputformat = (php_outputformat_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_outputformat);

  MAPSCRIPT_FREE_PARENT(php_outputformat->parent);

  if (php_outputformat->outputformat && !php_outputformat->is_ref) {
    outputFormatObj_destroy(php_outputformat->outputformat);
  }

  efree(object);
}

static zend_object_value mapscript_outputformat_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_outputformat_object *php_outputformat;

  MAPSCRIPT_ALLOC_OBJECT(php_outputformat, php_outputformat_object);

  retval = mapscript_object_new(&php_outputformat->std, ce,
                                &mapscript_outputformat_object_destroy TSRMLS_CC);

  php_outputformat->is_ref = 0;
  MAPSCRIPT_INIT_PARENT(php_outputformat->parent);

  return retval;
}

PHP_MINIT_FUNCTION(outputformat)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("outputFormatObj",
                           outputformat_functions,
                           mapscript_ce_outputformat,
                           mapscript_outputformat_object_new);

  mapscript_ce_outputformat->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
