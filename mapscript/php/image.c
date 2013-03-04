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
#include "ext/standard/head.h"
#include "main/php_output.h"

zend_class_entry *mapscript_ce_image;

ZEND_BEGIN_ARG_INFO_EX(image___get_args, 0, 0, 1)
ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(image___set_args, 0, 0, 2)
ZEND_ARG_INFO(0, property)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(image_pasteImage_args, 0, 0, 2)
ZEND_ARG_INFO(0, srcImg)
ZEND_ARG_INFO(0, transparentColorHex)
ZEND_ARG_INFO(0, dstX)
ZEND_ARG_INFO(0, dstY)
ZEND_ARG_INFO(0, angle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(image_saveImage_args, 0, 0, 2)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_OBJ_INFO(0, map, mapObj, 0)
ZEND_END_ARG_INFO()


/* {{{ proto image __construct()
   imageObj CANNOT be instanciated, this will throw an exception on use */
PHP_METHOD(imageObj, __construct)
{
  mapscript_throw_exception("imageObj cannot be constructed" TSRMLS_CC);
}
/* }}} */

PHP_METHOD(imageObj, __get)
{
  char *property;
  long property_len = 0;
  zval *zobj = getThis();
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                            &property, &property_len) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_image = (php_image_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_GET_LONG("width", php_image->image->width)
  else IF_GET_LONG("height", php_image->image->height)
    else IF_GET_LONG("resolution", php_image->image->resolution)
      else IF_GET_LONG("resolutionfactor", php_image->image->resolutionfactor)
        else IF_GET_STRING("imagepath", php_image->image->imagepath)
          else IF_GET_STRING("imageurl", php_image->image->imageurl)
            else IF_GET_STRING("imagetype", php_image->image->format->name)
              else {
                mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
              }
}

PHP_METHOD(imageObj, __set)
{
  char *property;
  long property_len = 0;
  zval *value;
  zval *zobj = getThis();
  php_image_object *php_image;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",
                            &property, &property_len, &value) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_image = (php_image_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  IF_SET_STRING("imagepath", php_image->image->imagepath, value)
  else IF_SET_STRING("imageurl", php_image->image->imageurl, value)
    else IF_SET_STRING("imagetype", php_image->image->format->name, value)
      else if ( (STRING_EQUAL("width", property)) ||
                (STRING_EQUAL("resolution", property)) ||
                (STRING_EQUAL("resolutionfactor", property)) ||
                (STRING_EQUAL("height", property)) ) {
        mapscript_throw_exception("Property '%s' is read-only and cannot be set." TSRMLS_CC, property);
      } else {
        mapscript_throw_exception("Property '%s' does not exist in this object." TSRMLS_CC, property);
      }
}

/* {{{ proto int saveWebImage()
   Writes image to temp directory.  Returns image URL. */
PHP_METHOD(imageObj, saveWebImage)
{
  zval *zobj = getThis();
  php_image_object *php_image;
  char *imageFile = NULL;
  char *imageFilename = NULL;
  char path[MS_MAXPATHLEN];
  char *imageUrlFull = NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters_none() == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_image = (php_image_object *) zend_object_store_get_object(zobj TSRMLS_CC);

  imageFilename = msTmpFilename(php_image->image->format->extension);
  imageFile = msBuildPath(path, php_image->image->imagepath, imageFilename);

  if (msSaveImage(NULL, php_image->image, imageFile) != MS_SUCCESS) {
    mapscript_throw_mapserver_exception("Failed writing image to %s" TSRMLS_CC, imageFile);
    return;
  }

  imageUrlFull = msBuildPath(path, php_image->image->imageurl, imageFilename);
  msFree(imageFilename);

  RETURN_STRING(imageUrlFull, 1);
}
/* }}} */

/* {{{ proto void pasteImage(imageObj Src, int transparentColor [[,int dstx, int dsty], int angle])
   Pastes another imageObj on top of this imageObj. transparentColor is
   the color (0xrrggbb) from srcImg that should be considered transparent.
   Pass transparentColor=-1 if you don't want any transparent color.
   If optional dstx,dsty are provided then they define the position where the
   image should be copied (dstx,dsty = top-left corner position).
   The optional angle is a value between 0 and 360 degrees to rotate the
   source image counterclockwise.  Note that if a rotation is requested then
   the dstx and dsty coordinates specify the CENTER of the destination area.
   NOTE : this function only works for 8 bits GD images.
*/
PHP_METHOD(imageObj, pasteImage)
{
  long transparent=-1, dstx=0, dsty=0, angle=0;
  int angleSet=MS_FALSE;
  zval *zimage;
  zval *zobj = getThis();
  php_image_object *php_image, *php_imageSrc;
  /*int oldTransparentColor, newTransparentColor=-1, r, g, b;*/
  rendererVTableObj *renderer = NULL;
  rasterBufferObj rb;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ol|lll",
                            &zimage, mapscript_ce_image, &transparent,
                            &dstx, &dsty, &angle) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  if  (ZEND_NUM_ARGS() == 3) {
    mapscript_report_php_error(E_WARNING, "dstX parameter given but not dstY" TSRMLS_CC);
  } else
    angleSet = MS_TRUE;

  php_image = (php_image_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  php_imageSrc = (php_image_object *) zend_object_store_get_object(zimage TSRMLS_CC);

  if (!MS_RENDERER_PLUGIN(php_imageSrc->image->format) ||
      !MS_RENDERER_PLUGIN(php_image->image->format)) {
    mapscript_throw_exception("PasteImage function should only be used with renderer plugin drivers." TSRMLS_CC);
    return;
  }

#ifdef undef //USE_AGG
  if( MS_RENDERER_AGG(php_imageSrc->image->format))
    msAlphaAGG2GD(php_imageSrc->image);
  if( MS_RENDERER_AGG(php_image->image->format))
    msAlphaAGG2GD(php_image->image);
#endif


  renderer = MS_IMAGE_RENDERER(php_image->image);
  memset(&rb,0,sizeof(rasterBufferObj));

  renderer->getRasterBufferHandle(php_imageSrc->image, &rb);
  renderer->mergeRasterBuffer(php_image->image, &rb, 1.0, 0, 0, dstx, dsty, rb.width, rb.height);

  /* Look for r,g,b in color table and make it transparent.
   * will return -1 if there is no exact match which will result in
   * no transparent color in the call to gdImageColorTransparent().
   */
  /*    if (transparent != -1)
      {
          r = (transparent / 0x010000) & 0xff;
          g = (transparent / 0x0100) & 0xff;
          b = transparent & 0xff;
          newTransparentColor = gdImageColorExact(php_imageSrc->image->img.gd, r, g, b);
      }

      oldTransparentColor = gdImageGetTransparent(php_imageSrc->image->img.gd);
      gdImageColorTransparent(php_imageSrc->image->img.gd, newTransparentColor);

      if (!angleSet)
          gdImageCopy(php_image->image->img.gd, php_imageSrc->image->img.gd, dstx, dsty,
                      0, 0, php_imageSrc->image->img.gd->sx, php_imageSrc->image->img.gd->sy);
      else
          gdImageCopyRotated(php_image->image->img.gd, php_imageSrc->image->img.gd, dstx, dsty,
                             0, 0, php_imageSrc->image->img.gd->sx, php_imageSrc->image->img.gd->sy,
                             angle);

                             gdImageColorTransparent(php_imageSrc->image->img.gd, oldTransparentColor);*/

  RETURN_LONG(MS_SUCCESS);

}
/* }}} */

/* {{{ proto int saveImage(string filename, mapObj map)
   Writes image object to specifed filename.  If filename is empty then
   write to stdout.  Returns MS_FAILURE on error. Second aregument oMap is not
   manadatory. It is usful when saving to other formats like GTIFF to get
   georeference infos.*/

PHP_METHOD(imageObj, saveImage)
{
  zval *zobj = getThis();
  zval *zmap = NULL;
  char *filename = NULL;
  long filename_len = 0;
  php_image_object *php_image;
  php_map_object *php_map;
  int status = MS_SUCCESS;
  /* stdout specific vars */
  int size=0;
  void *iptr=NULL;

  PHP_MAPSCRIPT_ERROR_HANDLING(TRUE);
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sO",
                            &filename, &filename_len,
                            &zmap, mapscript_ce_map) == FAILURE) {
    PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);
    return;
  }
  PHP_MAPSCRIPT_RESTORE_ERRORS(TRUE);

  php_image = (php_image_object *) zend_object_store_get_object(zobj TSRMLS_CC);
  if (zmap)
    php_map = (php_map_object *) zend_object_store_get_object(zmap TSRMLS_CC);

  if(filename_len > 0) {
    if ((status = msSaveImage((zmap ? php_map->map:NULL), php_image->image, filename) != MS_SUCCESS)) {
      mapscript_throw_mapserver_exception("Failed writing image to %s" TSRMLS_CC, filename);
      return;
    }

    RETURN_LONG(status);
  }

  /* no filename - read stdout */

  /* if there is no output buffer active, set the header */
  //handle changes in PHP 5.4.x
#if PHP_VERSION_ID < 50399
  if (OG(ob_nesting_level)<=0) {
    php_header(TSRMLS_C);
  }
#else
  if (php_output_get_level(TSRMLS_C)<=0) {
    php_header(TSRMLS_C);
  }
#endif

  if (MS_RENDERER_PLUGIN(php_image->image->format)) {
    iptr = (void *)msSaveImageBuffer(php_image->image, &size, php_image->image->format);
  } else if (php_image->image->format->name && (strcasecmp(php_image->image->format->name, "imagemap")==0)) {
    iptr = php_image->image->img.imagemap;
    size = strlen(php_image->image->img.imagemap);
  }

  if (size == 0) {
    mapscript_throw_mapserver_exception("Failed writing image to stdout" TSRMLS_CC);
    return;
  } else {
    php_write(iptr, size TSRMLS_CC);
    status = MS_SUCCESS;
    /* status = size;  why should we return the size ?? */
    msFree(iptr);
  }

  RETURN_LONG(status);
}
/* }}} */

zend_function_entry image_functions[] = {
  PHP_ME(imageObj, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(imageObj, __get, image___get_args, ZEND_ACC_PUBLIC)
  PHP_ME(imageObj, __set, image___set_args, ZEND_ACC_PUBLIC)
  PHP_MALIAS(imageObj, set, __set, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(imageObj, saveWebImage, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(imageObj, pasteImage, image_pasteImage_args, ZEND_ACC_PUBLIC)
  PHP_ME(imageObj, saveImage, image_saveImage_args, ZEND_ACC_PUBLIC) {
    NULL, NULL, NULL
  }
};


void mapscript_create_image(imageObj *image, zval *return_value TSRMLS_DC)
{
  php_image_object * php_image;
  object_init_ex(return_value, mapscript_ce_image);
  php_image = (php_image_object *)zend_object_store_get_object(return_value TSRMLS_CC);
  php_image->image = image;
}

static void mapscript_image_object_destroy(void *object TSRMLS_DC)
{
  php_image_object *php_image = (php_image_object *)object;

  MAPSCRIPT_FREE_OBJECT(php_image);

  msFreeImage(php_image->image);

  efree(object);
}

static zend_object_value mapscript_image_object_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_image_object *php_image;

  MAPSCRIPT_ALLOC_OBJECT(php_image, php_image_object);

  retval = mapscript_object_new(&php_image->std, ce,
                                &mapscript_image_object_destroy TSRMLS_CC);

  return retval;
}

PHP_MINIT_FUNCTION(image)
{
  zend_class_entry ce;

  MAPSCRIPT_REGISTER_CLASS("imageObj",
                           image_functions,
                           mapscript_ce_image,
                           mapscript_image_object_new);

  mapscript_ce_image->ce_flags |= ZEND_ACC_FINAL_CLASS;

  return SUCCESS;
}
