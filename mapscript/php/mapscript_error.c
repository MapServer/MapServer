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
#include "zend_exceptions.h"
#include "php_mapscript_util.h"
#include <stdarg.h>
#include "../../maperror.h"

zend_class_entry *mapscript_ce_mapscriptexception;

#if  PHP_VERSION_ID >= 70000
zend_object* mapscript_throw_exception(char *format TSRMLS_DC, ...)
#else
zval* mapscript_throw_exception(char *format TSRMLS_DC, ...)
#endif
{
  va_list args;
  char message[MESSAGELENGTH];
  va_start(args, format);
  //prevent buffer overflow
  vsnprintf(message, MESSAGELENGTH, format, args);
  va_end(args);
  return zend_throw_exception(mapscript_ce_mapscriptexception, message, 0 TSRMLS_CC);
}

#if  PHP_VERSION_ID >= 70000
zend_object* mapscript_throw_mapserver_exception(char *format TSRMLS_DC, ...)
#else
zval* mapscript_throw_mapserver_exception(char *format TSRMLS_DC, ...)
#endif
{
  va_list args;
  char message[MESSAGELENGTH];
  errorObj *ms_error;

  ms_error = msGetErrorObj();

  while (ms_error && ms_error->code != MS_NOERR) {
    php_error_docref(NULL TSRMLS_CC, E_WARNING,
                     "[MapServer Error]: %s: %s\n",
                     ms_error->routine, ms_error->message);
    ms_error = ms_error->next;
  }

  va_start(args, format);
  //prevent buffer overflow
  vsnprintf(message, MESSAGELENGTH, format, args);
  va_end(args);
  //prevent format string attack
  return mapscript_throw_exception("%s", message TSRMLS_CC);
}

void mapscript_report_php_error(int error_type, char *format TSRMLS_DC, ...)
{
  va_list args;
  char message[MESSAGELENGTH];
  va_start(args, format);
  //prevent buffer overflow
  vsnprintf(message, MESSAGELENGTH, format, args); 
  va_end(args);
  php_error_docref(NULL TSRMLS_CC, error_type, "%s,", message);
}

void mapscript_report_mapserver_error(int error_type TSRMLS_DC)
{
  errorObj *ms_error;

  ms_error = msGetErrorObj();

  while (ms_error && ms_error->code != MS_NOERR) {
    php_error_docref(NULL TSRMLS_CC, error_type,
                     "[MapServer Error]: %s: %s\n",
                     ms_error->routine, ms_error->message);
    ms_error = ms_error->next;
  }
}

PHP_MINIT_FUNCTION(mapscript_error)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "MapScriptException", NULL);
  
#if PHP_VERSION_ID >= 70000
  mapscript_ce_mapscriptexception = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C));
#else
  mapscript_ce_mapscriptexception = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), "Exception" TSRMLS_CC);
#endif

  return SUCCESS;
}
