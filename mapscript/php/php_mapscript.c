/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PHP/MapScript extension for MapServer.  External interface
 *           functions
 * Author:   Daniel Morissette, DM Solutions Group (dmorissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2000-2005, Daniel Morissette, DM Solutions Group Inc.
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
#include "php_mapscript_util.h"
#include "php.h"
#include "php_globals.h"
#include "SAPI.h"
#include "ext/standard/info.h"
#include "ext/standard/head.h"

#include "../../maperror.h"

#include <time.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#else
#include <errno.h>
#endif

/* All gd functions used in this extension (if they are renamed in the
   "main/php_compat.h" file of php source) should be added here too for
   compatibility reasons: when php compiles gd as a shared extention */
#if defined(HAVE_GD_BUNDLED)
#undef gdImageColorExact
#undef gdImageColorTransparent
#undef gdImageCopy
#endif

#ifndef DLEXPORT
#define DLEXPORT ZEND_DLEXPORT
#endif

//#if defined(_WIN32) && !defined(__CYGWIN__)
//void ***tsrm_ls;
//#endif

zend_object_handlers mapscript_std_object_handlers;

ZEND_BEGIN_ARG_INFO_EX(ms_newShapeObj_args, 0, 0, 1)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_shapeObjFromWkt_args, 0, 0, 1)
ZEND_ARG_INFO(0, wkt)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newShapeFileObj_args, 0, 0, 2)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newMapObj_args, 0, 0, 1)
ZEND_ARG_INFO(0, mapFileName)
ZEND_ARG_INFO(0, newMapPath)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newMapObjFromString_args, 0, 0, 1)
ZEND_ARG_INFO(0, mapFileString)
ZEND_ARG_INFO(0, newMapPath)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newLayerObj_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newProjectionObj_args, 0, 0, 1)
ZEND_ARG_INFO(0, projString)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newStyleObj_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, class, classObj, 0)
ZEND_ARG_OBJ_INFO(0, style, styleObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newClassObj_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_ARG_OBJ_INFO(0, class, classObj, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newSymbolObj_args, 0, 0, 2)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_ARG_INFO(0, symbolName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ms_newGridObj_args, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, layer, layerObj, 0)
ZEND_END_ARG_INFO()

/* {{{ proto mapObj ms_newMapObj(string mapFileName, newMapPath)
   Create a new mapObj instance. */
PHP_FUNCTION(ms_newMapObj)
{
  char *filename;
  long filename_len = 0;
  char *path = NULL;
  long path_len = 0;
  mapObj *map = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                            &filename, &filename_len,
                            &path, &path_len) == FAILURE) {
    return;
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  map = mapObj_new(filename, path);

  if (map == NULL) {
    mapscript_throw_mapserver_exception("Failed to open map file \"%s\", or map file error." TSRMLS_CC,  filename);
    return;
  }

  mapscript_create_map(map, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto mapObj ms_newMapObj(string mapFileString, newMapPath)
   Create a new mapObj instance. */
PHP_FUNCTION(ms_newMapObjFromString)
{
  char *string;
  long string_len = 0;
  char *path = NULL;
  long path_len = 0;
  mapObj *map = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                            &string, &string_len,
                            &path, &path_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  map = mapObj_newFromString(string, path);

  if (map == NULL) {
    mapscript_throw_mapserver_exception("Error while loading map file from string." TSRMLS_CC);
    return;
  }

  mapscript_create_map(map, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto layerObj ms_newLayerObj(mapObj map [, layerObj layer])
   Create a new layerObj instance. */
PHP_FUNCTION(ms_newLayerObj)
{
  zval *zmap, *zlayer = NULL;
  layerObj *layer;
  int index;
  php_map_object *php_map;
  php_layer_object *php_layer = NULL;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|O",
                            &zmap, mapscript_ce_map,
                            &zlayer, mapscript_ce_layer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = MAPSCRIPT_OBJ_P(php_map_object, zmap);
  if (zlayer)
    php_layer = MAPSCRIPT_OBJ_P(php_layer_object, zlayer);

  if ((layer = layerObj_new(php_map->map)) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* if a layer is passed as argument, copy the layer into
     the new one */
  if (zlayer) {
    index = layer->index;
    msCopyLayer(layer, php_layer->layer);
    layer->index = index;
  }

  MAPSCRIPT_MAKE_PARENT(zmap, NULL);
  mapscript_create_layer(layer, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto projectionObj ms_newProjectionObj(string projString)
   Create a new projectionObj instance. */
PHP_FUNCTION(ms_newProjectionObj)
{
  char *projString;
  long projString_len = 0;
  projectionObj *projection = NULL;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &projString, &projString_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ((projection = projectionObj_new(projString)) == NULL) {
    mapscript_throw_mapserver_exception("Unable to construct projectionObj." TSRMLS_CC);
    return;
  }

  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_projection(projection, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto rectObj ms_newRectObj()
   Create a new rectObj instance. */
PHP_FUNCTION(ms_newRectObj)
{
  php_rect_object * php_rect;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  object_init_ex(return_value, mapscript_ce_rect);
  php_rect = MAPSCRIPT_OBJ_P(php_rect_object, return_value);

  if ((php_rect->rect = rectObj_new()) == NULL) {
    mapscript_throw_exception("Unable to construct rectObj." TSRMLS_CC);
    return;
  }
}
/* }}} */

/* {{{ proto pointObj ms_newPointObj()
   Create a new pointObj instance. */
PHP_FUNCTION(ms_newPointObj)
{
  pointObj *point = NULL;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ((point = pointObj_new()) == NULL) {
    mapscript_throw_mapserver_exception("Unable to construct pointObj." TSRMLS_CC);
    return;
  }

  point->x = 0;
  point->y = 0;
  point->z = 0;
  point->m = 0;

  /* Return point object */
  MAPSCRIPT_MAKE_PARENT(NULL, NULL);
  mapscript_create_point(point, parent, return_value TSRMLS_CC);
}

/* {{{ proto lineObj ms_newLineObj()
   Create a new lineObj instance. */
PHP_FUNCTION(ms_newLineObj)
{
  php_line_object * php_line;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  object_init_ex(return_value, mapscript_ce_line);
  php_line = MAPSCRIPT_OBJ_P(php_line_object, return_value);

  if ((php_line->line = lineObj_new()) == NULL) {
    mapscript_throw_exception("Unable to construct lineObj." TSRMLS_CC);
    return;
  }
}
/* }}} */

/* {{{ proto styleObj __construct(classObj class [, styleObj style])
   Create a new styleObj instance */
PHP_FUNCTION(ms_newStyleObj)
{
  zval *zclass, *zstyle = NULL;
  styleObj *style;
  php_class_object *php_class;
  php_style_object *php_style;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|O",
                            &zclass, mapscript_ce_class,
                            &zstyle, mapscript_ce_style) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_class = MAPSCRIPT_OBJ_P(php_class_object, zclass);
  if (zstyle)
    php_style = MAPSCRIPT_OBJ_P(php_style_object, zstyle);

  if ((style = styleObj_new(php_class->class, (zstyle ? php_style->style : NULL))) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* Return point object */
  MAPSCRIPT_MAKE_PARENT(zclass, NULL);
  mapscript_create_style(style, parent, return_value TSRMLS_CC);
}

/* {{{ proto classObj ms_newClassObj(layerObj layer [, classObj class])
   Create a new class instance in the specified layer.. */
PHP_FUNCTION(ms_newClassObj)
{
  zval *zlayer, *zclass = NULL;
  classObj *class;
  php_layer_object *php_layer;
  php_class_object *php_class;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|O",
                            &zlayer, mapscript_ce_layer,
                            &zclass, mapscript_ce_class) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = MAPSCRIPT_OBJ_P(php_layer_object, zlayer);
  if (zclass)
    php_class = MAPSCRIPT_OBJ_P(php_class_object, zclass);


  if ((class = classObj_new(php_layer->layer, (zclass ? php_class->class:NULL))) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  MAPSCRIPT_MAKE_PARENT(zlayer, NULL);
  mapscript_create_class(class, parent, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int ms_newSymbolObj(mapObj map, string symbolName)
   Create a new symbolObj instance and return its index. */
PHP_FUNCTION(ms_newSymbolObj)
{
  zval *zmap;
  char *symbolName;
  long symbolName_len = 0;
  int retval = 0;
  php_map_object *php_map;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os",
                            &zmap, mapscript_ce_map,
                            &symbolName, &symbolName_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_map = MAPSCRIPT_OBJ_P(php_map_object, zmap);

  retval = msAddNewSymbol(php_map->map, symbolName);

  RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto shapeObj ms_newShapeObj(int type)
   Create a new shapeObj instance. */
PHP_FUNCTION(ms_newShapeObj)
{
  php_shape_object * php_shape;
  long type;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                            &type) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  object_init_ex(return_value, mapscript_ce_shape);
  php_shape = MAPSCRIPT_OBJ_P(php_shape_object, return_value);

  if ((php_shape->shape = shapeObj_new(type)) == NULL) {
    mapscript_throw_exception("Unable to construct shapeObj." TSRMLS_CC);
    return;
  }

  MAKE_STD_ZVAL(php_shape->values);
  mapscript_array_init(php_shape->values);
}
/* }}} */

/* {{{ proto shapeObj shapeObjFromWkt(string type)
   Creates new shape object from WKT string. */
PHP_FUNCTION(ms_shapeObjFromWkt)
{
  php_shape_object * php_shape;
  char *wkt;
  long str_len = 0;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &wkt, &str_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  object_init_ex(return_value, mapscript_ce_shape);
  php_shape = MAPSCRIPT_OBJ_P(php_shape_object, return_value);

  if ((php_shape->shape = msShapeFromWKT(wkt)) == NULL) {
    mapscript_throw_exception("Unable to construct shapeObj." TSRMLS_CC);
    return;
  }

  MAKE_STD_ZVAL(php_shape->values);
  mapscript_array_init(php_shape->values);
}
/* }}} */

/* {{{ proto shapefile ms_newShapeFileObj(string filename, int type)
   Create a new shapeFileObj instance. */
PHP_FUNCTION(ms_newShapeFileObj)
{
  char *filename;
  long filename_len = 0;
  long type;
  shapefileObj *shapefile;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                            &filename, &filename_len, &type) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  shapefile = shapefileObj_new(filename, type);
  if (shapefile == NULL) {
    mapscript_throw_mapserver_exception("Failed to open shapefile %s" TSRMLS_CC, filename);
    return;
  }

  mapscript_create_shapefile(shapefile, return_value TSRMLS_CC);
}
/* }}} */

PHP_FUNCTION(ms_newOWSRequestObj)
{
  cgiRequestObj *request;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ((request = cgirequestObj_new()) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* Return owsrequest object */
  mapscript_create_owsrequest(request, return_value TSRMLS_CC);
}

/* {{{ proto gridObj ms_newGridObj(layerObj layer)
   Create a new intance of gridObj. */
PHP_FUNCTION(ms_newGridObj)
{
  zval *zlayer;
  php_layer_object *php_layer;
  php_grid_object *php_grid;
  parent_object parent;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O",
                            &zlayer, mapscript_ce_layer) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_layer = MAPSCRIPT_OBJ_P(php_layer_object, zlayer);

  php_layer->layer->connectiontype = MS_GRATICULE;

  if (php_layer->layer->grid != NULL) {
    freeGrid(php_layer->layer->grid);
    free(php_layer->layer->grid);
  }

  php_layer->layer->grid = (graticuleObj *)malloc( sizeof( graticuleObj ) );
  initGrid(php_layer->layer->grid);

  if (ZVAL_NOT_UNDEF(php_layer->grid) && (MAPSCRIPT_TYPE(php_layer->grid) == IS_OBJECT)) {
    php_grid = MAPSCRIPT_OBJ(php_grid_object, php_layer->grid);
    php_grid->parent.child_ptr = NULL;
#if PHP_VERSION_ID < 70000
    zend_objects_store_del_ref(php_layer->grid TSRMLS_CC);
#else
	MAPSCRIPT_DELREF(php_layer->grid);
#endif
  }

  MAKE_STD_ZVAL(php_layer->grid);

  MAPSCRIPT_MAKE_PARENT(zlayer, &php_layer->grid);
#if PHP_VERSION_ID < 70000
  mapscript_create_grid((graticuleObj *)(php_layer->layer->grid), parent, php_layer->grid TSRMLS_CC);
  zend_objects_store_add_ref(php_layer->grid TSRMLS_CC);

  *return_value = *(php_layer->grid);
#else
  mapscript_create_grid((graticuleObj *)(php_layer->layer->grid), parent, &php_layer->grid TSRMLS_CC);

  ZVAL_COPY_VALUE(return_value, &php_layer->grid);
#endif
}
/* }}} */

/* {{{ proto errorObj ms_GetErrorObj()
   Return the head of the MapServer errorObj list. */
PHP_FUNCTION(ms_GetErrorObj)
{
  errorObj *error;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ((error = msGetErrorObj()) == NULL) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  /* Return error object */
  mapscript_create_error(error, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto errorObj ms_ResetErrorList()
   Clear the MapServer errorObj list. */
PHP_FUNCTION(ms_ResetErrorList)
{
  msResetErrorList();
}
/* }} */

/*=====================================================================
 *                 PHP function wrappers
 *====================================================================*/

/************************************************************************/
/*                         ms_VetVersion()                              */
/*                                                                      */
/*      Returns a string with MapServer version and configuration       */
/*      params.                                                         */
/************************************************************************/
PHP_FUNCTION(ms_GetVersion)
{
  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  MAPSCRIPT_RETURN_STRING(msGetVersion(), 1);
}

/************************************************************************/
/*                         ms_GetVersionInt()                           */
/*                                                                      */
/*      Returns the MapServer version in int format.                    */
/*      Given version x.y.z, returns x*0x10000+y*0x100+z                */
/************************************************************************/
PHP_FUNCTION(ms_GetVersionInt)
{
  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  RETURN_LONG(msGetVersionInt());
}

/* ==================================================================== */
/*      utility functions related to msio                               */
/* ==================================================================== */
PHP_FUNCTION(ms_ioInstallStdoutToBuffer)
{
  msIO_installStdoutToBuffer();

  RETURN_TRUE;
}


PHP_FUNCTION(ms_ioResetHandlers)
{
  msIO_resetHandlers();
  RETURN_TRUE;
}


PHP_FUNCTION(ms_ioInstallStdinFromBuffer)
{
  msIO_installStdinFromBuffer();

  RETURN_TRUE;
}

PHP_FUNCTION(ms_ioGetStdoutBufferString)
{
  char *buffer;

  msIOContext *ctx = msIO_getHandler( (FILE *) "stdout" );
  msIOBuffer  *buf;

  if(ctx == NULL ||  ctx->write_channel == MS_FALSE
      || strcmp(ctx->label,"buffer") != 0 ) {
    php_error(E_ERROR, "Can't identify msIO buffer");
    RETURN_FALSE;
  }

  buf = (msIOBuffer *) ctx->cbData;

  /* write one zero byte and backtrack if it isn't already there */
  if( buf->data_len == 0 || buf->data[buf->data_offset] != '\0' ) {
    msIO_bufferWrite( buf, "", 1 );
    buf->data_offset--;
  }

  buffer = (char *) (buf->data);

  MAPSCRIPT_RETURN_STRINGL(buffer, buf->data_offset, 1);
}


typedef struct {
  unsigned char *data;
  int size;
  int owns_data;
} gdBuffer;


PHP_FUNCTION(ms_ioGetStdoutBufferBytes)
{
  msIOContext *ctx = msIO_getHandler( (FILE *) "stdout" );
  msIOBuffer  *buf;
  gdBuffer     gdBuf;

  if( ctx == NULL || ctx->write_channel == MS_FALSE
      || strcmp(ctx->label,"buffer") != 0 ) {
    php_error(E_ERROR, "Can't identify msIO buffer");
    RETURN_FALSE;
  }

  buf = (msIOBuffer *) ctx->cbData;

  gdBuf.data = buf->data;
  gdBuf.size = buf->data_offset;
  gdBuf.owns_data = MS_FALSE;
  (void)gdBuf.owns_data;

  /* we are seizing ownership of the buffer contents */
  buf->data_offset = 0;
  buf->data_len = 0;
  buf->data = NULL;

  php_write(gdBuf.data, gdBuf.size TSRMLS_CC);

  /* return the gdBuf.size, which is the "really used length" of the msIOBuffer */
  RETURN_LONG(gdBuf.size);
}

PHP_FUNCTION(ms_ioStripStdoutBufferContentType)
{
  const char *buf = NULL;

  buf = msIO_stripStdoutBufferContentType();

  if (buf) {
    MAPSCRIPT_RETURN_STRING((char *)buf, 1);
  } else {
    RETURN_FALSE;
  }
}

PHP_FUNCTION(ms_ioGetAndStripStdoutBufferMimeHeaders)
{
  hashTableObj *hashtable;
  char *value, *key = NULL;

  if((hashtable = msIO_getAndStripStdoutBufferMimeHeaders())) {

    array_init(return_value);

    while((key = hashTableObj_nextKey(hashtable, key))) {
      value = (char *) hashTableObj_get(hashtable, key); 
#if PHP_VERSION_ID < 70000
      add_assoc_string(return_value, key, value, 1);
#else
      add_assoc_string(return_value, key, value);
#endif
    }
    free(hashtable);
  }
  else
	RETURN_FALSE;
}

PHP_FUNCTION(ms_ioStripStdoutBufferContentHeaders)
{
  msIO_stripStdoutBufferContentHeaders();
}

/* ==================================================================== */
/*      utility functions                                               */
/* ==================================================================== */


/************************************************************************/
/*      void ms_getcwd  */
/*                                                                      */
/*      This function is a copy of the                                  */
/*      vod php3_posix_getcwd(INTERNAL_FUNCTION_PARAMETERS)             */
/*      (./php/functions/posix.c).                                      */
/*                                                                      */
/*       Since posix functions seems to not be standard in php, It has been*/
/*      added here.                                                     */
/************************************************************************/
/* OS/2's gcc defines _POSIX_PATH_MAX but not PATH_MAX */
#if !defined(PATH_MAX) && defined(_POSIX_PATH_MAX)
#define PATH_MAX _POSIX_PATH_MAX
#endif

#if !defined(PATH_MAX) && defined(MAX_PATH)
#define PATH_MAX MAX_PATH
#endif

#if !defined(PATH_MAX)
#define PATH_MAX 512
#endif

PHP_FUNCTION(ms_getCwd)
{
  char  buffer[PATH_MAX];
  char *p;

  p = getcwd(buffer, PATH_MAX);
  if (!p) {
    //php3_error(E_WARNING, "posix_getcwd() failed with '%s'",
    //            strerror(errno));
    RETURN_FALSE;
  }

  MAPSCRIPT_RETURN_STRING(buffer, 1);
}

PHP_FUNCTION(ms_getPid)
{
  RETURN_LONG(getpid());
}


/************************************************************************/
/*       DLEXPORT void php3_ms_getscale(INTERNAL_FUNCTION_PARAMETERS)   */
/*                                                                      */
/*      Utility function to get the scale based on the width/height     */
/*      of the pixmap, the georeference and the units of the data.      */
/*                                                                      */
/*       Parameters are :                                               */
/*                                                                      */
/*            - Georefernce extents (rectObj)                           */
/*            - Width : width in pixel                                  */
/*            - Height : Height in pixel                                */
/*            - Units of the data (int)                                 */
/*                                                                      */
/*                                                                      */
/************************************************************************/
PHP_FUNCTION(ms_getScale)
{
  zval *zgeoRefExt = NULL;
  long width, height,unit;
  double resolution;
  php_rect_object *php_geoRefExt;
  double dfScale = 0.0;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ollld",
                            &zgeoRefExt, mapscript_ce_rect,
                            &width, &height, &unit, &resolution) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_geoRefExt = MAPSCRIPT_OBJ_P(php_rect_object, zgeoRefExt);

  if (msCalculateScale(*(php_geoRefExt->rect), unit, width, height, resolution, &dfScale) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("" TSRMLS_CC);
    return;
  }

  RETURN_DOUBLE(dfScale);
}

/**********************************************************************
 *                        ms_tokenizeMap()
 *
 * Preparse mapfile and return an array containg one item for each
 * token in the map.
 **********************************************************************/

/* {{{ proto array ms_tokenizeMap(string filename)
   Preparse mapfile and return an array containg one item for each token in the map.*/

PHP_FUNCTION(ms_tokenizeMap)
{
  char *filename;
  long filename_len = 0;
  char  **tokens;
  int i, numtokens=0;


  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &filename, &filename_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if ((tokens = msTokenizeMap(filename, &numtokens)) == NULL) {
    mapscript_throw_mapserver_exception("Failed tokenizing map file %s" TSRMLS_CC,
                                        filename);
    return;
  } else {
    array_init(return_value);

    for (i=0; i<numtokens; i++) {
#if PHP_VERSION_ID < 70000
      add_next_index_string(return_value,  tokens[i], 1);
#else
      add_next_index_string(return_value,  tokens[i]);
#endif
    }

    msFreeCharArray(tokens, numtokens);
  }


}

zend_function_entry mapscript_functions[] = {
  PHP_FE(ms_GetVersion, NULL)
  PHP_FE(ms_GetVersionInt,  NULL)
  PHP_FE(ms_newLineObj, NULL)
  PHP_FE(ms_newRectObj, NULL)
  PHP_FE(ms_newShapeObj, ms_newShapeObj_args)
  PHP_FE(ms_shapeObjFromWkt, ms_shapeObjFromWkt_args)
  PHP_FE(ms_GetErrorObj, NULL)
  PHP_FE(ms_ResetErrorList, NULL)
  PHP_FE(ms_newOWSRequestObj, NULL)
  PHP_FE(ms_newShapeFileObj, ms_newShapeFileObj_args)
  PHP_FE(ms_newMapObj, ms_newMapObj_args)
  PHP_FE(ms_newMapObjFromString, ms_newMapObjFromString_args)
  PHP_FE(ms_newLayerObj, ms_newLayerObj_args)
  PHP_FE(ms_newPointObj, NULL)
  PHP_FE(ms_newProjectionObj, ms_newProjectionObj_args)
  PHP_FE(ms_newStyleObj, ms_newStyleObj_args)
  PHP_FE(ms_newSymbolObj, ms_newSymbolObj_args)
  PHP_FE(ms_newClassObj, ms_newClassObj_args)
  PHP_FE(ms_newGridObj, ms_newGridObj_args)
  PHP_FE(ms_getCwd, NULL)
  PHP_FE(ms_getPid, NULL)
  PHP_FE(ms_getScale, NULL)
  PHP_FE(ms_tokenizeMap, NULL)
  PHP_FE(ms_ioInstallStdoutToBuffer, NULL)
  PHP_FE(ms_ioInstallStdinFromBuffer, NULL)
  PHP_FE(ms_ioGetStdoutBufferString, NULL)
  PHP_FE(ms_ioResetHandlers, NULL)
  PHP_FE(ms_ioStripStdoutBufferContentType, NULL)
  PHP_FE(ms_ioStripStdoutBufferContentHeaders, NULL)
  PHP_FE(ms_ioGetAndStripStdoutBufferMimeHeaders, NULL)
  PHP_FE(ms_ioGetStdoutBufferBytes, NULL) {
    NULL, NULL, NULL
  }
};

zend_module_entry mapscript_module_entry = {
  STANDARD_MODULE_HEADER,
  "MapScript",
  mapscript_functions,
  /* MINIT()/MSHUTDOWN() are called once only when PHP starts up and
   * shutdowns.  They're really called only once and *not* when a new Apache
   * child process is created.
   */
  PHP_MINIT(mapscript),
  PHP_MSHUTDOWN(mapscript),
  /* RINIT()/RSHUTDOWN() are called once per request
   * We shouldn't really be using them since they are run on every request
   * and can hit performance.
   */
  NULL /* PHP_RINIT(mapscript)     */,
  NULL /* PHP_RSHUTDOWN(mapscript) */,
  PHP_MINFO(mapscript),
  MAPSCRIPT_VERSION,          /* extension version number (string) */
  STANDARD_MODULE_PROPERTIES
};

#if COMPILE_DL
ZEND_GET_MODULE(mapscript)
#endif


PHP_MINFO_FUNCTION(mapscript)
{
  php_info_print_table_start();
  php_info_print_table_row(2, "MapServer Version", msGetVersion());
  php_info_print_table_row(2, "PHP MapScript Version", MAPSCRIPT_VERSION);
  php_info_print_table_end();
}

PHP_MINIT_FUNCTION(mapscript)
{
  int const_flag = CONST_CS|CONST_PERSISTENT;

  /* Init MapServer resources */
  if (msSetup() != MS_SUCCESS) {
    mapscript_report_php_error(E_ERROR, "msSetup(): MapScript initialization failed." TSRMLS_CC);
    return FAILURE;
  }

  memcpy(&mapscript_std_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  mapscript_std_object_handlers.clone_obj = NULL;

  /* If you have INI entries, uncomment this line
  REGISTER_INI_ENTRIES();
  */

  /* boolean constants*/
  REGISTER_LONG_CONSTANT("MS_TRUE",       MS_TRUE,        const_flag);
  REGISTER_LONG_CONSTANT("MS_FALSE",      MS_FALSE,       const_flag);
  REGISTER_LONG_CONSTANT("MS_ON",         MS_ON,          const_flag);
  REGISTER_LONG_CONSTANT("MS_OFF",        MS_OFF,         const_flag);
  REGISTER_LONG_CONSTANT("MS_YES",        MS_YES,         const_flag);
  REGISTER_LONG_CONSTANT("MS_NO",         MS_NO,          const_flag);

  /* map units constants*/
  REGISTER_LONG_CONSTANT("MS_INCHES",     MS_INCHES,      const_flag);
  REGISTER_LONG_CONSTANT("MS_FEET",       MS_FEET,        const_flag);
  REGISTER_LONG_CONSTANT("MS_MILES",      MS_MILES,       const_flag);
  REGISTER_LONG_CONSTANT("MS_METERS",     MS_METERS,      const_flag);
  REGISTER_LONG_CONSTANT("MS_NAUTICALMILES",MS_NAUTICALMILES,const_flag);
  REGISTER_LONG_CONSTANT("MS_KILOMETERS", MS_KILOMETERS,  const_flag);
  REGISTER_LONG_CONSTANT("MS_DD",         MS_DD,          const_flag);
  REGISTER_LONG_CONSTANT("MS_PIXELS",     MS_PIXELS,      const_flag);

  /* layer type constants*/
  REGISTER_LONG_CONSTANT("MS_LAYER_POINT",MS_LAYER_POINT, const_flag);
  REGISTER_LONG_CONSTANT("MS_LAYER_LINE", MS_LAYER_LINE,  const_flag);
  REGISTER_LONG_CONSTANT("MS_LAYER_POLYGON",MS_LAYER_POLYGON, const_flag);
  REGISTER_LONG_CONSTANT("MS_LAYER_RASTER",MS_LAYER_RASTER, const_flag);
  REGISTER_LONG_CONSTANT("MS_LAYER_QUERY",MS_LAYER_QUERY, const_flag);
  REGISTER_LONG_CONSTANT("MS_LAYER_CIRCLE",MS_LAYER_CIRCLE, const_flag);
  REGISTER_LONG_CONSTANT("MS_LAYER_TILEINDEX",MS_LAYER_TILEINDEX, const_flag);
  REGISTER_LONG_CONSTANT("MS_LAYER_CHART",MS_LAYER_CHART, const_flag);


  /* layer status constants (see also MS_ON, MS_OFF) */
  REGISTER_LONG_CONSTANT("MS_DEFAULT",    MS_DEFAULT,     const_flag);
  REGISTER_LONG_CONSTANT("MS_EMBED",      MS_EMBED,       const_flag);
  REGISTER_LONG_CONSTANT("MS_DELETE",     MS_DELETE,       const_flag);

  /* font type constants*/
  REGISTER_LONG_CONSTANT("MS_TRUETYPE",   MS_TRUETYPE,    const_flag);
  REGISTER_LONG_CONSTANT("MS_BITMAP",     MS_BITMAP,      const_flag);

  /* bitmap font style constants */
  REGISTER_LONG_CONSTANT("MS_TINY",       MS_TINY,        const_flag);
  REGISTER_LONG_CONSTANT("MS_SMALL",      MS_SMALL,       const_flag);
  REGISTER_LONG_CONSTANT("MS_MEDIUM",     MS_MEDIUM,      const_flag);
  REGISTER_LONG_CONSTANT("MS_LARGE",      MS_LARGE,       const_flag);
  REGISTER_LONG_CONSTANT("MS_GIANT",      MS_GIANT,       const_flag);

  /* label position constants*/
  REGISTER_LONG_CONSTANT("MS_UL",         MS_UL,          const_flag);
  REGISTER_LONG_CONSTANT("MS_LR",         MS_LR,          const_flag);
  REGISTER_LONG_CONSTANT("MS_UR",         MS_UR,          const_flag);
  REGISTER_LONG_CONSTANT("MS_LL",         MS_LL,          const_flag);
  REGISTER_LONG_CONSTANT("MS_CR",         MS_CR,          const_flag);
  REGISTER_LONG_CONSTANT("MS_CL",         MS_CL,          const_flag);
  REGISTER_LONG_CONSTANT("MS_UC",         MS_UC,          const_flag);
  REGISTER_LONG_CONSTANT("MS_LC",         MS_LC,          const_flag);
  REGISTER_LONG_CONSTANT("MS_CC",         MS_CC,          const_flag);
  REGISTER_LONG_CONSTANT("MS_AUTO",       MS_AUTO,        const_flag);
  REGISTER_LONG_CONSTANT("MS_XY",         MS_XY,          const_flag);
  REGISTER_LONG_CONSTANT("MS_FOLLOW",     MS_FOLLOW,      const_flag);
  REGISTER_LONG_CONSTANT("MS_AUTO2",      MS_AUTO2,       const_flag);
  REGISTER_LONG_CONSTANT("MS_NONE",       MS_NONE,        const_flag);

  /* alignment constants*/
  REGISTER_LONG_CONSTANT("MS_ALIGN_LEFT",  MS_ALIGN_LEFT,  const_flag);
  REGISTER_LONG_CONSTANT("MS_ALIGN_CENTER",MS_ALIGN_CENTER,const_flag);
  REGISTER_LONG_CONSTANT("MS_ALIGN_RIGHT", MS_ALIGN_RIGHT, const_flag);

  /* shape type constants*/
  REGISTER_LONG_CONSTANT("MS_SHAPE_POINT",MS_SHAPE_POINT, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPE_LINE",  MS_SHAPE_LINE, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPE_POLYGON",MS_SHAPE_POLYGON, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPE_NULL", MS_SHAPE_NULL,  const_flag);

  /* shapefile type constants*/
  /* Old names ... */
  REGISTER_LONG_CONSTANT("MS_SHP_POINT",  SHP_POINT,      const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_ARC",    SHP_ARC,        const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_POLYGON",SHP_POLYGON,    const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_MULTIPOINT",SHP_MULTIPOINT, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_POINTM",  SHP_POINTM,      const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_ARCM",    SHP_ARCM,        const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_POLYGONM",SHP_POLYGONM,    const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_MULTIPOINTM",SHP_MULTIPOINTM, const_flag);
  /* new names??? */
  REGISTER_LONG_CONSTANT("SHP_POINT",     SHP_POINT,      const_flag);
  REGISTER_LONG_CONSTANT("SHP_ARC",       SHP_ARC,        const_flag);
  REGISTER_LONG_CONSTANT("SHP_POLYGON",   SHP_POLYGON,    const_flag);
  REGISTER_LONG_CONSTANT("SHP_MULTIPOINT",SHP_MULTIPOINT, const_flag);
  REGISTER_LONG_CONSTANT("SHP_POINTM",     SHP_POINTM,      const_flag);
  REGISTER_LONG_CONSTANT("SHP_ARCM",       SHP_ARCM,        const_flag);
  REGISTER_LONG_CONSTANT("SHP_POLYGONM",   SHP_POLYGONM,    const_flag);
  REGISTER_LONG_CONSTANT("SHP_MULTIPOINTM",SHP_MULTIPOINTM, const_flag);

  /* query/join type constants*/
  REGISTER_LONG_CONSTANT("MS_SINGLE",     MS_SINGLE,      const_flag);
  REGISTER_LONG_CONSTANT("MS_MULTIPLE",   MS_MULTIPLE,    const_flag);

  /* connection type constants*/
  REGISTER_LONG_CONSTANT("MS_INLINE",     MS_INLINE,      const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPEFILE",  MS_SHAPEFILE,   const_flag);
  REGISTER_LONG_CONSTANT("MS_TILED_SHAPEFILE",MS_TILED_SHAPEFILE,const_flag);
  REGISTER_LONG_CONSTANT("MS_FLATGEOBUF", MS_FLATGEOBUF,  const_flag);
  REGISTER_LONG_CONSTANT("MS_OGR",        MS_OGR,         const_flag);
  REGISTER_LONG_CONSTANT("MS_POSTGIS",    MS_POSTGIS,     const_flag);
  REGISTER_LONG_CONSTANT("MS_WMS",        MS_WMS,         const_flag);
  REGISTER_LONG_CONSTANT("MS_ORACLESPATIAL", MS_ORACLESPATIAL,const_flag);
  REGISTER_LONG_CONSTANT("MS_WFS",        MS_WFS,         const_flag);
  REGISTER_LONG_CONSTANT("MS_GRATICULE",  MS_GRATICULE,   const_flag);
  REGISTER_LONG_CONSTANT("MS_RASTER",     MS_RASTER,      const_flag);
  REGISTER_LONG_CONSTANT("MS_PLUGIN",     MS_PLUGIN,      const_flag);
  REGISTER_LONG_CONSTANT("MS_UNION",      MS_UNION,      const_flag);
  REGISTER_LONG_CONSTANT("MS_UVRASTER",   MS_UVRASTER, const_flag);

  /* output image type constants*/
  /*
  REGISTER_LONG_CONSTANT("MS_GIF",        MS_GIF,         const_flag);
  REGISTER_LONG_CONSTANT("MS_PNG",        MS_PNG,         const_flag);
  REGISTER_LONG_CONSTANT("MS_JPEG",       MS_JPEG,        const_flag);
  REGISTER_LONG_CONSTANT("MS_WBMP",       MS_WBMP,        const_flag);
  REGISTER_LONG_CONSTANT("MS_SWF",        MS_SWF,        const_flag);
  */

  /* querymap style constants */
  REGISTER_LONG_CONSTANT("MS_NORMAL",     MS_NORMAL,      const_flag);
  REGISTER_LONG_CONSTANT("MS_HILITE",     MS_HILITE,      const_flag);
  REGISTER_LONG_CONSTANT("MS_SELECTED",   MS_SELECTED,    const_flag);

  /* return value constants */
  REGISTER_LONG_CONSTANT("MS_SUCCESS",    MS_SUCCESS,     const_flag);
  REGISTER_LONG_CONSTANT("MS_FAILURE",    MS_FAILURE,     const_flag);
  REGISTER_LONG_CONSTANT("MS_DONE",       MS_DONE,        const_flag);

  /* error code constants */
  REGISTER_LONG_CONSTANT("MS_NOERR",      MS_NOERR,       const_flag);
  REGISTER_LONG_CONSTANT("MS_IOERR",      MS_IOERR,       const_flag);
  REGISTER_LONG_CONSTANT("MS_MEMERR",     MS_MEMERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_TYPEERR",    MS_TYPEERR,     const_flag);
  REGISTER_LONG_CONSTANT("MS_SYMERR",     MS_SYMERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_REGEXERR",   MS_REGEXERR,    const_flag);
  REGISTER_LONG_CONSTANT("MS_TTFERR",     MS_TTFERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_DBFERR",     MS_DBFERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_IDENTERR",   MS_IDENTERR,    const_flag);
  REGISTER_LONG_CONSTANT("MS_EOFERR",     MS_EOFERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_PROJERR",    MS_PROJERR,     const_flag);
  REGISTER_LONG_CONSTANT("MS_MISCERR",    MS_MISCERR,     const_flag);
  REGISTER_LONG_CONSTANT("MS_CGIERR",     MS_CGIERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_WEBERR",     MS_WEBERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_IMGERR",     MS_IMGERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_HASHERR",    MS_HASHERR,     const_flag);
  REGISTER_LONG_CONSTANT("MS_JOINERR",    MS_JOINERR,     const_flag);
  REGISTER_LONG_CONSTANT("MS_NOTFOUND",   MS_NOTFOUND,    const_flag);
  REGISTER_LONG_CONSTANT("MS_SHPERR",     MS_SHPERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_PARSEERR",   MS_PARSEERR,    const_flag);
  REGISTER_LONG_CONSTANT("MS_OGRERR",     MS_OGRERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERYERR",   MS_QUERYERR,    const_flag);
  REGISTER_LONG_CONSTANT("MS_WMSERR",     MS_WMSERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_WMSCONNERR", MS_WMSCONNERR,  const_flag);
  REGISTER_LONG_CONSTANT("MS_ORACLESPATIALERR", MS_ORACLESPATIALERR, const_flag);
  REGISTER_LONG_CONSTANT("MS_WFSERR",     MS_WFSERR,      const_flag);
  REGISTER_LONG_CONSTANT("MS_WFSCONNERR", MS_WFSCONNERR,  const_flag);
  REGISTER_LONG_CONSTANT("MS_MAPCONTEXTERR", MS_MAPCONTEXTERR, const_flag);
  REGISTER_LONG_CONSTANT("MS_HTTPERR",    MS_HTTPERR,     const_flag);
  REGISTER_LONG_CONSTANT("MS_WCSERR",    MS_WCSERR,     const_flag);

  /*symbol types */
  REGISTER_LONG_CONSTANT("MS_SYMBOL_SIMPLE", MS_SYMBOL_SIMPLE, const_flag);
  REGISTER_LONG_CONSTANT("MS_SYMBOL_VECTOR", MS_SYMBOL_VECTOR, const_flag);
  REGISTER_LONG_CONSTANT("MS_SYMBOL_ELLIPSE", MS_SYMBOL_ELLIPSE, const_flag);
  REGISTER_LONG_CONSTANT("MS_SYMBOL_PIXMAP", MS_SYMBOL_PIXMAP, const_flag);
  REGISTER_LONG_CONSTANT("MS_SYMBOL_TRUETYPE", MS_SYMBOL_TRUETYPE, const_flag);
  /* MS_IMAGEMODE types for use with outputFormatObj */
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_PC256", MS_IMAGEMODE_PC256, const_flag);
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_RGB",  MS_IMAGEMODE_RGB, const_flag);
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_RGBA", MS_IMAGEMODE_RGBA, const_flag);
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_INT16", MS_IMAGEMODE_INT16, const_flag);
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_FLOAT32", MS_IMAGEMODE_FLOAT32, const_flag);
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_BYTE", MS_IMAGEMODE_BYTE, const_flag);
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_FEATURE", MS_IMAGEMODE_FEATURE, const_flag);
  REGISTER_LONG_CONSTANT("MS_IMAGEMODE_NULL", MS_IMAGEMODE_NULL, const_flag);

  /*binding types*/
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_SIZE",  MS_STYLE_BINDING_SIZE, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_WIDTH",  MS_STYLE_BINDING_WIDTH, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_ANGLE", MS_STYLE_BINDING_ANGLE, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_COLOR", MS_STYLE_BINDING_COLOR, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OUTLINECOLOR", MS_STYLE_BINDING_OUTLINECOLOR, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_SYMBOL", MS_STYLE_BINDING_SYMBOL, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OUTLINEWIDTH", MS_STYLE_BINDING_OUTLINEWIDTH, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OPACITY", MS_STYLE_BINDING_OPACITY, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OFFSET_X", MS_STYLE_BINDING_OFFSET_X, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_OFFSET_Y", MS_STYLE_BINDING_OFFSET_Y, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_POLAROFFSET_PIXEL", MS_STYLE_BINDING_POLAROFFSET_PIXEL, const_flag);
  REGISTER_LONG_CONSTANT("MS_STYLE_BINDING_POLAROFFSET_ANGLE", MS_STYLE_BINDING_POLAROFFSET_ANGLE, const_flag);

  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_SIZE",  MS_LABEL_BINDING_SIZE, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_ANGLE", MS_LABEL_BINDING_ANGLE, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_COLOR", MS_LABEL_BINDING_COLOR, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_OUTLINECOLOR", MS_LABEL_BINDING_OUTLINECOLOR, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_FONT",  MS_LABEL_BINDING_FONT, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_PRIORITY", MS_LABEL_BINDING_PRIORITY, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_POSITION", MS_LABEL_BINDING_POSITION, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_SHADOWSIZEX", MS_LABEL_BINDING_SHADOWSIZEX, const_flag);
  REGISTER_LONG_CONSTANT("MS_LABEL_BINDING_SHADOWSIZEY", MS_LABEL_BINDING_SHADOWSIZEY, const_flag);

  /* MS_CAPS_JOINS_AND_CORNERS */
  REGISTER_LONG_CONSTANT("MS_CJC_NONE", MS_CJC_NONE, const_flag);
  REGISTER_LONG_CONSTANT("MS_CJC_BEVEL", MS_CJC_BEVEL, const_flag);
  REGISTER_LONG_CONSTANT("MS_CJC_BUTT", MS_CJC_BUTT, const_flag);
  REGISTER_LONG_CONSTANT("MS_CJC_MITER", MS_CJC_MITER, const_flag);
  REGISTER_LONG_CONSTANT("MS_CJC_ROUND", MS_CJC_ROUND, const_flag);
  REGISTER_LONG_CONSTANT("MS_CJC_SQUARE", MS_CJC_SQUARE, const_flag);
  REGISTER_LONG_CONSTANT("MS_CJC_TRIANGLE", MS_CJC_TRIANGLE, const_flag);

  /*cgi request types*/
  REGISTER_LONG_CONSTANT("MS_GET_REQUEST", MS_GET_REQUEST, const_flag);
  REGISTER_LONG_CONSTANT("MS_POST_REQUEST", MS_POST_REQUEST, const_flag);

  REGISTER_LONG_CONSTANT("MS_DB_CSV", MS_DB_CSV, const_flag);
  REGISTER_LONG_CONSTANT("MS_DB_MYSQL", MS_DB_MYSQL, const_flag);
  REGISTER_LONG_CONSTANT("MS_DB_ORACLE", MS_DB_ORACLE, const_flag);
  REGISTER_LONG_CONSTANT("MS_DB_POSTGRES", MS_DB_POSTGRES, const_flag);
  REGISTER_LONG_CONSTANT("MS_DB_XBASE", MS_DB_XBASE, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEBUGLEVEL_DEBUG", MS_DEBUGLEVEL_DEBUG, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEBUGLEVEL_ERRORSONLY", MS_DEBUGLEVEL_ERRORSONLY, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEBUGLEVEL_TUNING", MS_DEBUGLEVEL_TUNING, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEBUGLEVEL_V", MS_DEBUGLEVEL_V, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEBUGLEVEL_VV", MS_DEBUGLEVEL_VV, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEBUGLEVEL_VVV", MS_DEBUGLEVEL_VVV, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEFAULT_CGI_PARAMS", MS_DEFAULT_CGI_PARAMS, const_flag);
  REGISTER_LONG_CONSTANT("MS_DEFAULT_LABEL_PRIORITY", MS_DEFAULT_LABEL_PRIORITY, const_flag);
  REGISTER_STRING_CONSTANT("MS_ERROR_LANGUAGE", MS_ERROR_LANGUAGE, const_flag);
  REGISTER_LONG_CONSTANT("MS_FILE_MAP", MS_FILE_MAP, const_flag);
  REGISTER_LONG_CONSTANT("MS_FILE_SYMBOL", MS_FILE_SYMBOL, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_BEYOND", MS_GEOS_BEYOND, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_CONTAINS", MS_GEOS_CONTAINS, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_CROSSES", MS_GEOS_CROSSES, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_DISJOINT", MS_GEOS_DISJOINT, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_DWITHIN", MS_GEOS_DWITHIN, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_EQUALS", MS_GEOS_EQUALS, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_INTERSECTS", MS_GEOS_INTERSECTS, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_OVERLAPS", MS_GEOS_OVERLAPS, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_TOUCHES", MS_GEOS_TOUCHES, const_flag);
  REGISTER_LONG_CONSTANT("MS_GEOS_WITHIN", MS_GEOS_WITHIN, const_flag);
  REGISTER_LONG_CONSTANT("MS_JOIN_ONE_TO_MANY", MS_JOIN_ONE_TO_MANY, const_flag);
  REGISTER_LONG_CONSTANT("MS_JOIN_ONE_TO_ONE", MS_JOIN_ONE_TO_ONE, const_flag);
  REGISTER_LONG_CONSTANT("MS_MAXPATTERNLENGTH", MS_MAXPATTERNLENGTH, const_flag);
  REGISTER_LONG_CONSTANT("MS_MAXVECTORPOINTS", MS_MAXVECTORPOINTS, const_flag);
  REGISTER_LONG_CONSTANT("MS_MAX_LABEL_FONTS", MS_MAX_LABEL_FONTS, const_flag);
  REGISTER_LONG_CONSTANT("MS_MAX_LABEL_PRIORITY", MS_MAX_LABEL_PRIORITY, const_flag);
  REGISTER_LONG_CONSTANT("MS_MYSQL", MS_MYSQL, const_flag);
  REGISTER_LONG_CONSTANT("MS_NOOVERRIDE", MS_NOOVERRIDE, const_flag);
  REGISTER_LONG_CONSTANT("MS_NULLPARENTERR", MS_NULLPARENTERR, const_flag);
  REGISTER_LONG_CONSTANT("MS_NUMERRORCODES", MS_NUMERRORCODES, const_flag);
  REGISTER_LONG_CONSTANT("MS_PARSE_TYPE_BOOLEAN", MS_PARSE_TYPE_BOOLEAN, const_flag);
  REGISTER_LONG_CONSTANT("MS_PARSE_TYPE_SHAPE", MS_PARSE_TYPE_SHAPE, const_flag);
  REGISTER_LONG_CONSTANT("MS_PARSE_TYPE_STRING", MS_PARSE_TYPE_STRING, const_flag);
  REGISTER_LONG_CONSTANT("MS_PERCENTAGES", MS_PERCENTAGES, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_BY_ATTRIBUTE", MS_QUERY_BY_ATTRIBUTE, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_BY_FILTER", MS_QUERY_BY_FILTER, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_BY_INDEX", MS_QUERY_BY_INDEX, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_BY_POINT", MS_QUERY_BY_POINT, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_BY_RECT", MS_QUERY_BY_RECT, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_BY_SHAPE", MS_QUERY_BY_SHAPE, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_IS_NULL", MS_QUERY_IS_NULL, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_MULTIPLE", MS_QUERY_MULTIPLE, const_flag);
  REGISTER_LONG_CONSTANT("MS_QUERY_SINGLE", MS_QUERY_SINGLE, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDERERERR", MS_RENDERERERR, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_AGG", MS_RENDER_WITH_AGG, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_CAIRO_PDF", MS_RENDER_WITH_CAIRO_PDF, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_CAIRO_RASTER", MS_RENDER_WITH_CAIRO_RASTER, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_CAIRO_SVG", MS_RENDER_WITH_CAIRO_SVG, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_IMAGEMAP", MS_RENDER_WITH_IMAGEMAP, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_KML", MS_RENDER_WITH_KML, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_OGL", MS_RENDER_WITH_OGL, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_OGR", MS_RENDER_WITH_OGR, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_PLUGIN", MS_RENDER_WITH_PLUGIN, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_RAWDATA", MS_RENDER_WITH_RAWDATA, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_SWF", MS_RENDER_WITH_SWF, const_flag);
  REGISTER_LONG_CONSTANT("MS_RENDER_WITH_TEMPLATE", MS_RENDER_WITH_TEMPLATE, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPEFILE_ARC", MS_SHAPEFILE_ARC, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPEFILE_MULTIPOINT", MS_SHAPEFILE_MULTIPOINT, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPEFILE_POINT", MS_SHAPEFILE_POINT, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHAPEFILE_POLYGON", MS_SHAPEFILE_POLYGON, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_ARCZ", MS_SHP_ARCZ, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_MULTIPOINTZ", MS_SHP_MULTIPOINTZ, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_POINTZ", MS_SHP_POINTZ, const_flag);
  REGISTER_LONG_CONSTANT("MS_SHP_POLYGONZ", MS_SHP_POLYGONZ, const_flag);
  REGISTER_LONG_CONSTANT("MS_SOSERR", MS_SOSERR, const_flag);
  REGISTER_LONG_CONSTANT("MS_SYMBOL_HATCH", MS_SYMBOL_HATCH, const_flag);
  REGISTER_LONG_CONSTANT("MS_SYMBOL_SVG", MS_SYMBOL_SVG, const_flag);
  REGISTER_LONG_CONSTANT("MS_TRANSFORM_FULLRESOLUTION", MS_TRANSFORM_FULLRESOLUTION, const_flag);
  REGISTER_LONG_CONSTANT("MS_TRANSFORM_NONE", MS_TRANSFORM_NONE, const_flag);
  REGISTER_LONG_CONSTANT("MS_TRANSFORM_ROUND", MS_TRANSFORM_ROUND, const_flag);
  REGISTER_LONG_CONSTANT("MS_TRANSFORM_SIMPLIFY", MS_TRANSFORM_SIMPLIFY, const_flag);
  REGISTER_LONG_CONSTANT("MS_TRANSFORM_SNAPTOGRID", MS_TRANSFORM_SNAPTOGRID, const_flag);

  PHP_MINIT(mapscript_error)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(color)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(rect)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(hashtable)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(label)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(style)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(symbol)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(image)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(web)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(legend)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(outputformat)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(querymap)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(grid)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(error)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(referencemap)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(class)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(projection)(INIT_FUNC_ARGS_PASSTHRU);
#ifdef disabled
  PHP_MINIT(labelcachemember)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(labelcache)(INIT_FUNC_ARGS_PASSTHRU);
#endif
  PHP_MINIT(labelleader)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(result)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(scalebar)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(owsrequest)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(point)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(line)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(shape)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(shapefile)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(cluster)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(layer)(INIT_FUNC_ARGS_PASSTHRU);
  PHP_MINIT(map)(INIT_FUNC_ARGS_PASSTHRU);

  return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(mapscript)
{
  /* Cleanup MapServer resources */
  msCleanup();

  return SUCCESS;
}

PHP_RINIT_FUNCTION(mapscript)
{
  return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(mapscript)
{
  return SUCCESS;
}
