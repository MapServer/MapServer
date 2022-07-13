/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Various support code related to the outputFormatObj.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include <assert.h>
#include "mapserver.h"
#include "mapows.h"



static outputFormatObj *msAllocOutputFormat( mapObj *map, const char *name,
    const char *driver );

/*************************************************************************

NOTES on outputFormatObj:

typedef struct {
  char *name;
  char *mimetype;
  char *driver;
  int  imagemode; // MS_IMAGEMODE_* value.
  int  transparent;
  int  numformatoptions;
  char **formatoptions;
} outputFormatObj;

 NAME - Associates an internal name with the declaration.  The value used
        has not intrinsic meaning and is just used to associate with the
        MAP level IMAGETYPE.  It is also the "name" used for the format
        in WMS capabilities documents, and FORMAT= requests. (required)

 MIMETYPE - The mime type to use for this format.  If omitted, the value
            will be derived from the DRIVER or default to the value for
            untyped binary data will be used.  (optional - may be NULL)

 DRIVER - This indicates which internal driver mechanism is to be used.
          Anything prefixed by "GDAL/" will be handled by the GDAL driver, with
          the remainder taken as the GDAL format name. (required)

 IMAGEMODE - Has one of "PC256", "RGB", or "RGBA" indicating whether
             the imaging should be done to a 256 pseudo-colored, 24bit RGB, or
             32bit RGBA (A=alpha/transparency) result image.  Note that the
             IMAGEMODE actually affects how all the rendering for the map is
             done, long before it is output to a particular output format.
             The default value is the traditional "PC256".  Some output
             formats can only support some IMAGEMODE values. (optional)

             Translate as MS_IMAGEMODE_PC256, MS_IMAGEMODE_RGB and
             MS_IMAGEMODE_RGBA.

             For "GDAL/" drivers, the following extra imagemodes are supported:
                "BYTE" / MS_IMAGEMODE_BYTE
                "INT16" / MS_IMAGEMODE_INT16
                "FLOAT32" / MS_IMAGEMODE_FLOAT32

             Not too sure what this should be set to for output formats like
             flash and SVG.

 TRANSPARENT - A value of "ON" or "OFF" indicating whether transparency
               support should be enabled.  Same as the old TRANSPARENT flag
               at the MAP level.

 FORMATOPTION - Contains an argument for the specific driver used.  There may
                be any number of format options for a given OUTPUTFORMAT
                declaration.  FORMATOPTION will be used to encode the older
                INTERLACE, and QUALITY values.

                Handled as a MapServer style CharArray.

 *************************************************************************/


struct defaultOutputFormatEntry {
  const char *name;
  const char *driver;
  const char *mimetype;
} ;

struct defaultOutputFormatEntry defaultoutputformats[] = {
  {"png","AGG/PNG","image/png"},
  {"jpeg","AGG/JPEG","image/jpeg"},
  {"png8","AGG/PNG8","image/png; mode=8bit"},
  {"png24","AGG/PNG","image/png; mode=24bit"},
  {"jpegpng", "AGG/MIXED", "image/vnd.jpeg-png"},
  {"jpegpng8", "AGG/MIXED", "image/vnd.jpeg-png8"},
#ifdef USE_CAIRO
  {"pdf","CAIRO/PDF","application/x-pdf"},
  {"svg","CAIRO/SVG","image/svg+xml"},
  {"cairopng","CAIRO/PNG","image/png"},
#endif
  {"GTiff","GDAL/GTiff","image/tiff"},
#ifdef USE_KML
  {"kml","KML","application/vnd.google-earth.kml+xml"},
  {"kmz","KMZ","application/vnd.google-earth.kmz"},
#endif
#ifdef USE_PBF
  {"mvt","MVT","application/vnd.mapbox-vector-tile"},
  // The following format is added to keep backward compatibility
  // for the application/x-protobuf media type
  {"mvtxprotobuf","MVT","application/x-protobuf"},
#endif
  {"json","UTFGrid","application/json"},
  {NULL,NULL,NULL}
};

/************************************************************************/
/*                  msPostMapParseOutputFormatSetup()                   */
/************************************************************************/

int msPostMapParseOutputFormatSetup( mapObj *map )

{
  outputFormatObj *format;

  /* If IMAGETYPE not set use the first user defined OUTPUTFORMAT.
     If none, use the first default format. */
  if( map->imagetype == NULL && map->numoutputformats > 0 )
    map->imagetype = msStrdup(map->outputformatlist[0]->name);
  if( map->imagetype == NULL)
    map->imagetype = msStrdup(defaultoutputformats[0].name);

  /* select the current outputformat into map->outputformat */
  format = msSelectOutputFormat( map, map->imagetype );
  if( format == NULL ) {
    msSetError(MS_MISCERR,
               "Unable to select IMAGETYPE `%s'.",
               "msPostMapParseOutputFormatSetup()",
               map->imagetype ? map->imagetype : "(null)" );
    return MS_FAILURE;
  }

  msApplyOutputFormat( &(map->outputformat), format, MS_NOOVERRIDE);

  return MS_SUCCESS;
}

/************************************************************************/
/*                    msCreateDefaultOutputFormat()                     */
/************************************************************************/

outputFormatObj *msCreateDefaultOutputFormat( mapObj *map,
    const char *driver,
    const char *name,
    const char *mimetype )

{

  outputFormatObj *format = NULL;
  if( strncasecmp(driver,"GD/",3) == 0 ) {
    return msCreateDefaultOutputFormat( map, "AGG/PNG8", name, mimetype );
  }

  if( strcasecmp(driver,"UTFGRID") == 0 ) {
    if(!name) name="utfgrid";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("application/json");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("json");
    format->renderer = MS_RENDER_WITH_UTFGRID;
  }

  else if( strcasecmp(driver,"AGG/PNG") == 0 ) {
    if(!name) name="png";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/png");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("png");
    format->renderer = MS_RENDER_WITH_AGG;
  }

  else if( strcasecmp(driver,"AGG/PNG8") == 0 ) {
    if(!name) name="png8";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/png; mode=8bit");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("png");
    format->renderer = MS_RENDER_WITH_AGG;
    msSetOutputFormatOption( format, "QUANTIZE_FORCE", "on");
    msSetOutputFormatOption( format, "QUANTIZE_COLORS", "256");
  }

  else if( strcasecmp(driver,"AGG/JPEG") == 0 ) {
    if(!name) name="jpeg";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/jpeg");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("jpg");
    format->renderer = MS_RENDER_WITH_AGG;
  }
#if defined(USE_PBF)
  else if( strcasecmp(driver,"MVT") == 0 ) {
    if(!name) name="mvt";
    format = msAllocOutputFormat( map, name, driver );
    if (mimetype) {
      format->mimetype = msStrdup(mimetype);
    } else {
      format->mimetype = msStrdup("application/vnd.mapbox-vector-tile");
    }

    format->imagemode = MS_IMAGEMODE_FEATURE;
    format->extension = msStrdup("pbf");
    format->renderer = MS_RENDER_WITH_MVT;
  }
#endif

  else if( strcasecmp(driver,"AGG/MIXED") == 0 &&
           name != NULL && strcasecmp(name,"jpegpng") == 0 ) {
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/vnd.jpeg-png");
    format->imagemode = MS_IMAGEMODE_RGBA;
    format->extension = msStrdup("XXX");
    format->renderer = MS_RENDER_WITH_AGG;
    msSetOutputFormatOption( format, "OPAQUE_FORMAT", "jpeg");
    msSetOutputFormatOption( format, "TRANSPARENT_FORMAT", "png24");
  }

  else if( strcasecmp(driver,"AGG/MIXED") == 0 &&
           name != NULL && strcasecmp(name,"jpegpng8") == 0 ) {
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/vnd.jpeg-png8");
    format->imagemode = MS_IMAGEMODE_RGBA;
    format->extension = msStrdup("XXX");
    format->renderer = MS_RENDER_WITH_AGG;
    msSetOutputFormatOption( format, "OPAQUE_FORMAT", "jpeg");
    msSetOutputFormatOption( format, "TRANSPARENT_FORMAT", "png8");
  }

  else if( strcasecmp(driver,"AGG/MIXED") == 0 ) {
    if(!name) name="mixed";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/mixed");
    format->imagemode = MS_IMAGEMODE_RGBA;
    format->extension = msStrdup("XXX");
    format->renderer = MS_RENDER_WITH_AGG;
  }

#if defined(USE_CAIRO)
  else if( strcasecmp(driver,"CAIRO/PNG") == 0 ) {
    if(!name) name="cairopng";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/png; mode=24bit");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("png");
    format->renderer = MS_RENDER_WITH_CAIRO_RASTER;
  }
  else if( strcasecmp(driver,"CAIRO/JPEG") == 0 ) {
    if(!name) name="cairojpeg";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/jpeg");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("jpg");
    format->renderer = MS_RENDER_WITH_CAIRO_RASTER;
  }
  else if( strcasecmp(driver,"CAIRO/PDF") == 0 ) {
    if(!name) name="pdf";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("application/x-pdf");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("pdf");
    format->renderer = MS_RENDER_WITH_CAIRO_PDF;
  }
  else if( strcasecmp(driver,"CAIRO/SVG") == 0 ) {
    if(!name) name="svg";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/svg+xml");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("svg");
    format->renderer = MS_RENDER_WITH_CAIRO_SVG;
  }
#ifdef _WIN32
  else if( strcasecmp(driver,"CAIRO/WINGDI") == 0 ) {
    if(!name) name="cairowinGDI";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("");
    format->renderer = MS_RENDER_WITH_CAIRO_RASTER;
  }
  else if( strcasecmp(driver,"CAIRO/WINGDIPRINT") == 0 ) {
    if(!name) name="cairowinGDIPrint";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("");
    format->renderer = MS_RENDER_WITH_CAIRO_RASTER;
  }
#endif
#endif

#if defined(USE_OGL)
  else if( strcasecmp(driver,"OGL/PNG") == 0 ) {
    if(!name) name="oglpng24";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("image/png; mode=24bit");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("png");
    format->renderer = MS_RENDER_WITH_OGL;
  }
#endif

#if defined(USE_KML)
  else if( strcasecmp(driver,"KML") == 0 ) {
    if(!name) name="kml";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("application/vnd.google-earth.kml+xml");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("kml");
    format->renderer = MS_RENDER_WITH_KML;
    msSetOutputFormatOption( format, "ATTACHMENT", "mapserver.kml");
  }
  else if( strcasecmp(driver,"KMZ") == 0 ) {
    if(!name) name="kmz";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("application/vnd.google-earth.kmz");
    format->imagemode = MS_IMAGEMODE_RGB;
    format->extension = msStrdup("kmz");
    format->renderer = MS_RENDER_WITH_KML;
    msSetOutputFormatOption( format, "ATTACHMENT", "mapserver.kmz");
  }
#endif



  else if( strncasecmp(driver,"gdal/",5) == 0 ) {
    if(!name) name=driver+5;
    format = msAllocOutputFormat( map, name, driver );
    if( msInitDefaultGDALOutputFormat( format ) == MS_FAILURE ) {
      if( map != NULL ) {
        map->numoutputformats--;
        map->outputformatlist[map->numoutputformats] = NULL;
      }

      msFreeOutputFormat( format );
      format = NULL;
    }
  }

  else if( strncasecmp(driver,"ogr/",4) == 0 ) {
    if(!name) name=driver+4;
    format = msAllocOutputFormat( map, name, driver );
    if( msInitDefaultOGROutputFormat( format ) == MS_FAILURE ) {
      if( map != NULL ) {
        map->numoutputformats--;
        map->outputformatlist[map->numoutputformats] = NULL;
      }

      msFreeOutputFormat( format );
      format = NULL;
    }
  }

  else if( strcasecmp(driver,"imagemap") == 0 ) {
    if(!name) name="imagemap";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("text/html; driver=imagemap");
    format->extension = msStrdup("html");
    format->imagemode = MS_IMAGEMODE_NULL;
    format->renderer = MS_RENDER_WITH_IMAGEMAP;
  }

  else if( strcasecmp(driver,"template") == 0 ) {
    if(!name) name="template";
    format = msAllocOutputFormat( map, name, driver );
    format->mimetype = msStrdup("text/html");
    format->extension = msStrdup("html");
    format->imagemode = MS_IMAGEMODE_FEATURE;
    format->renderer = MS_RENDER_WITH_TEMPLATE;
  }

  if( format != NULL )
    format->inmapfile = MS_FALSE;


  return format;
}

/************************************************************************/
/*                    msApplyDefaultOutputFormats()                     */
/************************************************************************/


void msApplyDefaultOutputFormats( mapObj *map )
{
  char *saved_imagetype;
  struct defaultOutputFormatEntry *defEntry;

  if( map->imagetype == NULL )
    saved_imagetype = NULL;
  else
    saved_imagetype = msStrdup(map->imagetype);

  defEntry = defaultoutputformats;
  while(defEntry->name) {
    if( msSelectOutputFormat( map, defEntry->name ) == NULL )
      msCreateDefaultOutputFormat( map, defEntry->driver, defEntry->name, defEntry->mimetype );
    defEntry++;
  }
  if( map->imagetype != NULL )
    free( map->imagetype );
  map->imagetype = saved_imagetype;
}

/************************************************************************/
/*                         msFreeOutputFormat()                         */
/************************************************************************/

void msFreeOutputFormat( outputFormatObj *format )

{
  if( format == NULL )
    return;
  if( MS_REFCNT_DECR_IS_NOT_ZERO(format) ) {
    return;
  }

  if(MS_RENDERER_PLUGIN(format) && format->vtable) {
    format->vtable->cleanup(MS_RENDERER_CACHE(format->vtable));
    free( format->vtable );
  }
  msFree( format->name );
  msFree( format->mimetype );
  msFree( format->driver );
  msFree( format->extension );
  msFreeCharArray( format->formatoptions, format->numformatoptions );
  msFree( format );
}

/************************************************************************/
/*                        msAllocOutputFormat()                         */
/************************************************************************/

static outputFormatObj *msAllocOutputFormat( mapObj *map, const char *name,
    const char *driver )

{
  outputFormatObj *format;

  /* -------------------------------------------------------------------- */
  /*      Allocate the format object.                                     */
  /* -------------------------------------------------------------------- */
  format = (outputFormatObj *) calloc(1,sizeof(outputFormatObj));
  if( format == NULL ) {
    msSetError( MS_MEMERR, NULL, "msAllocOutputFormat()" );
    return NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize various fields.                                      */
  /* -------------------------------------------------------------------- */
  format->bands = 1;
  format->name = msStrdup(name);
  format->driver = msStrdup(driver);
  format->refcount = 0;
  format->vtable = NULL;
  format->device = NULL;
  format->imagemode = MS_IMAGEMODE_RGB;

  /* -------------------------------------------------------------------- */
  /*      Attach to map.                                                  */
  /* -------------------------------------------------------------------- */
  if( map != NULL ) {
    map->numoutputformats++;
    if( map->outputformatlist == NULL )
      map->outputformatlist = (outputFormatObj **) malloc(sizeof(void*));
    else
      map->outputformatlist = (outputFormatObj **)
                              realloc(map->outputformatlist,
                                      sizeof(void*) * map->numoutputformats );

    map->outputformatlist[map->numoutputformats-1] = format;
    format->refcount++;
  }

  return format;
}

/************************************************************************/
/*                        msAppendOutputFormat()                        */
/*                                                                      */
/*      Add an output format  .                                         */
/*      http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=511           */
/************************************************************************/
int msAppendOutputFormat(mapObj *map, outputFormatObj *format)
{
  /* -------------------------------------------------------------------- */
  /*      Attach to map.                                                  */
  /* -------------------------------------------------------------------- */
  assert(map);
  map->numoutputformats++;
  if (map->outputformatlist == NULL)
    map->outputformatlist = (outputFormatObj **) malloc(sizeof(void*));
  else
    map->outputformatlist = (outputFormatObj **)
                            realloc(map->outputformatlist,
                                    sizeof(void*) * map->numoutputformats );

  map->outputformatlist[map->numoutputformats-1] = format;
  MS_REFCNT_INCR(format);

  return map->numoutputformats;
}

/************************************************************************/
/*                        msRemoveOutputFormat()                        */
/*                                                                      */
/*      Remove an output format (by name).                              */
/*      http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=511           */
/************************************************************************/
int msRemoveOutputFormat(mapObj *map, const char *name)
{
  int i, j;
  /* -------------------------------------------------------------------- */
  /*      Detach from map.                                                */
  /* -------------------------------------------------------------------- */
  if (map != NULL) {
    if (map->outputformatlist == NULL) {
      msSetError(MS_CHILDERR, "Can't remove format from empty outputformatlist", "msRemoveOutputFormat()");
      return MS_FAILURE;
    } else {
      i = msGetOutputFormatIndex(map, name);
      if (i >= 0) {
        map->numoutputformats--;
	if(MS_REFCNT_DECR_IS_ZERO(map->outputformatlist[i]))
           msFreeOutputFormat( map->outputformatlist[i] );

        for (j=i; j<map->numoutputformats-1; j++) {
          map->outputformatlist[j] = map->outputformatlist[j+1];
        }
      }
      map->outputformatlist = (outputFormatObj **)
                              realloc(map->outputformatlist,
                                      sizeof(void*) * (map->numoutputformats) );
      return MS_SUCCESS;
    }
  }
  return MS_FAILURE;
}

/************************************************************************/
/*                       msGetOutputFormatIndex()                       */
/*                                                                      */
/*      Pulled this out of msSelectOutputFormat for use in other cases. */
/************************************************************************/

int msGetOutputFormatIndex(mapObj *map, const char *imagetype)
{
  int i;
  /* -------------------------------------------------------------------- */
  /*      Try to find the format in the maps list of formats, first by    */
  /*      mime type, and then by output format name.                      */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < map->numoutputformats; i++) {
    if (map->outputformatlist[i]->mimetype != NULL
        && strcasecmp(imagetype,
                      map->outputformatlist[i]->mimetype) == 0 )
      return i;
  }

  for( i = 0; i < map->numoutputformats; i++ ) {
    if( strcasecmp(imagetype,map->outputformatlist[i]->name) == 0 )
      return i;
  }

  return -1;
}

/************************************************************************/
/*                        msSelectOutputFormat()                        */
/************************************************************************/



outputFormatObj *msSelectOutputFormat( mapObj *map,
                                       const char *imagetype )

{
  int index;
  outputFormatObj *format = NULL;

  if( map == NULL || imagetype == NULL || strlen(imagetype) == 0 )
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Try to find the format in the maps list of formats, first by    */
  /*      mime type, and then by output format name.                      */
  /* -------------------------------------------------------------------- */
  index = msGetOutputFormatIndex(map, imagetype);
  if (index >= 0) {
    format = map->outputformatlist[index];
  } else {
    struct defaultOutputFormatEntry *formatEntry = defaultoutputformats;
    while(formatEntry->name) {
      if(!strcasecmp(imagetype,formatEntry->name) || !strcasecmp(imagetype,formatEntry->mimetype)) {
        format = msCreateDefaultOutputFormat( map, formatEntry->driver, formatEntry->name, formatEntry->mimetype );
        break;
      }
      formatEntry++;
    }

  }

  if (format) {
    if (map->imagetype)
      free(map->imagetype);
    map->imagetype = msStrdup(format->name);
  }

  if( format != NULL )
    msOutputFormatValidate( format, MS_FALSE );

  return format;
}

/************************************************************************/
/*                        msApplyOutputFormat()                         */
/************************************************************************/

void msApplyOutputFormat( outputFormatObj **target,
                          outputFormatObj *format,
                          int transparent)

{
  int       change_needed = MS_FALSE;
  outputFormatObj *formatToFree = NULL;

  assert( target != NULL );

  if( *target != NULL && MS_REFCNT_DECR_IS_ZERO( (*target) ) ) {
    formatToFree = *target;
    *target = NULL;
  }

  if( format == NULL ) {
    if( formatToFree )
      msFreeOutputFormat( formatToFree );
    *target = NULL;
    return;
  }

  msOutputFormatValidate( format, MS_FALSE );

  /* -------------------------------------------------------------------- */
  /*      Do we need to change any values?  If not, then just apply       */
  /*      and return.                                                     */
  /* -------------------------------------------------------------------- */
  if( transparent != MS_NOOVERRIDE && !format->transparent != !transparent )
      change_needed = MS_TRUE;

  if( change_needed ) {

    if( format->refcount > 0 )
      format = msCloneOutputFormat( format );

    if( transparent != MS_NOOVERRIDE ) {
        format->transparent = transparent;
        if( format->imagemode == MS_IMAGEMODE_RGB )
            format->imagemode = MS_IMAGEMODE_RGBA;
    }
  }

  *target = format;
  format->refcount++;
  if( MS_RENDERER_PLUGIN(format) ) {
    msInitializeRendererVTable(format);
  }


  if( formatToFree )
    msFreeOutputFormat( formatToFree );
}

/************************************************************************/
/*                        msCloneOutputFormat()                         */
/************************************************************************/

outputFormatObj *msCloneOutputFormat( outputFormatObj *src )

{
  outputFormatObj *dst;
  int             i;

  dst = msAllocOutputFormat( NULL, src->name, src->driver );

  msFree( dst->mimetype );
  if( src->mimetype )
    dst->mimetype = msStrdup( src->mimetype );
  else
    dst->mimetype = NULL;

  msFree( dst->extension );
  if( src->extension )
    dst->extension = msStrdup( src->extension );
  else
    dst->extension = NULL;

  dst->imagemode = src->imagemode;
  dst->renderer = src->renderer;

  dst->transparent = src->transparent;
  dst->bands = src->bands;

  dst->numformatoptions = src->numformatoptions;
  dst->formatoptions = (char **)
                       malloc(sizeof(char *) * src->numformatoptions );

  for( i = 0; i < src->numformatoptions; i++ )
    dst->formatoptions[i] = msStrdup(src->formatoptions[i]);

  dst->inmapfile = src->inmapfile;

  return dst;
}

/************************************************************************/
/*                      msGetOutputFormatOption()                       */
/*                                                                      */
/*      Fetch the value of a particular option.  It is assumed the      */
/*      options are in "KEY=VALUE" format.                              */
/************************************************************************/

const char *msGetOutputFormatOption( outputFormatObj *format,
                                     const char *optionkey,
                                     const char *defaultresult )

{
  int    i, len = strlen(optionkey);

  for( i = 0; i < format->numformatoptions; i++ ) {
    if( strncasecmp(format->formatoptions[i],optionkey,len) == 0
        && format->formatoptions[i][len] == '=' )
      return format->formatoptions[i] + len + 1;
  }

  return defaultresult;
}

/************************************************************************/
/*                      msSetOutputFormatOption()                       */
/************************************************************************/

void msSetOutputFormatOption( outputFormatObj *format,
                              const char *key, const char *value )

{
  char *newline;
  int   i, len;

  if( value == NULL )
    return;

  /* -------------------------------------------------------------------- */
  /*      Format the name=value pair into a newly allocated string.       */
  /* -------------------------------------------------------------------- */
  newline = (char *) malloc(strlen(key)+strlen(value)+2);
  if( newline == NULL ) {
    assert( newline != NULL );
    return;
  }

  sprintf( newline, "%s=%s", key, value );

  /* -------------------------------------------------------------------- */
  /*      Does this key already occur?  If so replace it.                 */
  /* -------------------------------------------------------------------- */
  len = strlen(key);

  for( i = 0; i < format->numformatoptions; i++ ) {
    if( strncasecmp(format->formatoptions[i],key,len) == 0
        && format->formatoptions[i][len] == '=' ) {
      free( format->formatoptions[i] );
      format->formatoptions[i] = newline;
      return;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      otherwise, we need to grow the list.                            */
  /* -------------------------------------------------------------------- */
  format->numformatoptions++;
  format->formatoptions = (char **)
                          realloc( format->formatoptions,
                                   sizeof(char*) * format->numformatoptions );

  format->formatoptions[format->numformatoptions-1] = newline;

  /* -------------------------------------------------------------------- */
  /*      Capture generic value(s) we are interested in.                  */
  /* -------------------------------------------------------------------- */
  if( strcasecmp(key,"BAND_COUNT") == 0 )
    format->bands = atoi(value);
}

/************************************************************************/
/*                     msGetOutputFormatMimeList()                      */
/************************************************************************/

void msGetOutputFormatMimeList( mapObj *map, char **mime_list, int max_mime )

{
  int mime_count = 0, i;

  msApplyDefaultOutputFormats(map);
  for( i = 0; i < map->numoutputformats && mime_count < max_mime; i++ ) {
    int  j;

    if( map->outputformatlist[i]->mimetype == NULL )
      continue;

    for( j = 0; j < mime_count; j++ ) {
      if( strcasecmp(mime_list[j],
                     map->outputformatlist[i]->mimetype) == 0 )
        break;
    }

    if( j == mime_count )
      mime_list[mime_count++] = map->outputformatlist[i]->mimetype;
  }

  if( mime_count < max_mime )
    mime_list[mime_count] = NULL;
}

/************************************************************************/
/*                     msGetOutputFormatMimeList()                      */
/************************************************************************/
void msGetOutputFormatMimeListImg( mapObj *map, const char **mime_list, int max_mime )

{
  int mime_count = 0, i,j;
  const char *format_list = NULL;
  char **tokens = NULL;
  int numtokens = 0;
  outputFormatObj *format;

  msApplyDefaultOutputFormats(map);
  format_list = msOWSLookupMetadata(&(map->web.metadata), "M","getlegendgraphic_formatlist");
  if ( format_list && strlen(format_list) > 0)
    tokens = msStringSplit(format_list,  ',', &numtokens);

  if (tokens && numtokens > 0) {
    for(j = 0; j < numtokens; j++ ) {
      format = msSelectOutputFormat(map, tokens[j]);
      if (format != NULL) {
        mime_list[mime_count++] = format->mimetype;
      }
    }
  } else
    for( i = 0; i < map->numoutputformats && mime_count < max_mime; i++ ) {
      int  j;

      if( map->outputformatlist[i]->mimetype == NULL )
        continue;

      for( j = 0; j < mime_count; j++ ) {
        if( strcasecmp(mime_list[j],
                       map->outputformatlist[i]->mimetype) == 0 )
          break;
      }

      if( j == mime_count && map->outputformatlist[i]->driver &&
          strncasecmp(map->outputformatlist[i]->driver, "AGG/", 4)==0)
        mime_list[mime_count++] = map->outputformatlist[i]->mimetype;
    }

  if( mime_count < max_mime )
    mime_list[mime_count] = NULL;
  if(tokens)
    msFreeCharArray(tokens, numtokens);
}

/************************************************************************/
/*                  msGetOutputFormatMimeListWMS()                      */
/************************************************************************/

void msGetOutputFormatMimeListWMS( mapObj *map, const char **mime_list, int max_mime )
{
  int mime_count = 0, i,j;
  const char *format_list = NULL;
  char **tokens = NULL;
  int numtokens = 0;
  outputFormatObj *format;
  msApplyDefaultOutputFormats(map);
  format_list = msOWSLookupMetadata(&(map->web.metadata), "M","getmap_formatlist");
  if ( format_list && strlen(format_list) > 0)
    tokens = msStringSplit(format_list,  ',', &numtokens);

  if (tokens && numtokens > 0) {
    for(j = 0; j < numtokens; j++ ) {
      format = msSelectOutputFormat(map, tokens[j]);
      if (format != NULL) {
        mime_list[mime_count++] = format->mimetype;
      }
    }
  } else {
    for( i = 0; i < map->numoutputformats && mime_count < max_mime; i++ ) {
      int  j;

      if( map->outputformatlist[i]->mimetype == NULL )
        continue;

      for( j = 0; j < mime_count; j++ ) {
        if( strcasecmp(mime_list[j],
                       map->outputformatlist[i]->mimetype) == 0 )
          break;
      }

      if( j == mime_count && map->outputformatlist[i]->driver &&
          (
            strncasecmp(map->outputformatlist[i]->driver, "GDAL/", 5)==0 ||
            strncasecmp(map->outputformatlist[i]->driver, "AGG/", 4)==0 ||
            strcasecmp(map->outputformatlist[i]->driver, "CAIRO/SVG")==0 ||
            strcasecmp(map->outputformatlist[i]->driver, "CAIRO/PDF")==0 ||
            strcasecmp(map->outputformatlist[i]->driver, "kml")==0 ||
            strcasecmp(map->outputformatlist[i]->driver, "kmz")==0 ||
            strcasecmp(map->outputformatlist[i]->driver, "mvt")==0 ||
            strcasecmp(map->outputformatlist[i]->driver, "UTFGRID")==0))
        mime_list[mime_count++] = map->outputformatlist[i]->mimetype;
    }
  }
  if( mime_count < max_mime )
    mime_list[mime_count] = NULL;
  if(tokens)
    msFreeCharArray(tokens, numtokens);
}


/************************************************************************/
/*                       msOutputFormatValidate()                       */
/*                                                                      */
/*      Do some validation of the output format, and report to debug    */
/*      output if it doesn't seem valid.  Fixup in place as best as     */
/*      possible.                                                       */
/************************************************************************/

int msOutputFormatValidate( outputFormatObj *format, int issue_error )

{
  int result = MS_TRUE;
  char *driver_ext;

  format->bands = atoi(msGetOutputFormatOption( format, "BAND_COUNT", "1" ));

  /* Enforce the requirement that JPEG be RGB and TRANSPARENT=OFF */
  driver_ext = strstr(format->driver,"/");
  if( driver_ext && ++driver_ext && !strcasecmp(driver_ext,"JPEG")) {
    if( format->transparent ) {
      if( issue_error )
        msSetError( MS_MISCERR,
                    "JPEG OUTPUTFORMAT %s has TRANSPARENT set ON, but this is not supported.\n"
                    "It has been disabled.\n",
                    "msOutputFormatValidate()", format->name );
      else
        msDebug( "JPEG OUTPUTFORMAT %s has TRANSPARENT set ON, but this is not supported.\n"
                 "It has been disabled.\n",
                 format->name );

      format->transparent = MS_FALSE;
      result = MS_FALSE;
    }
    if( format->imagemode == MS_IMAGEMODE_RGBA ) {
      if( issue_error )
        msSetError( MS_MISCERR,
                    "JPEG OUTPUTFORMAT %s has IMAGEMODE RGBA, but this is not supported.\n"
                    "IMAGEMODE forced to RGB.\n",
                    "msOutputFormatValidate()", format->name );
      else
        msDebug( "JPEG OUTPUTFORMAT %s has IMAGEMODE RGBA, but this is not supported.\n"
                 "IMAGEMODE forced to RGB.\n",
                 format->name );

      format->imagemode = MS_IMAGEMODE_RGB;
      result = MS_FALSE;
    }
  }

  if( format->transparent && format->imagemode == MS_IMAGEMODE_RGB ) {
    if( issue_error )
      msSetError( MS_MISCERR,
                  "OUTPUTFORMAT %s has TRANSPARENT set ON, but an IMAGEMODE\n"
                  "of RGB instead of RGBA.  Changing imagemode to RGBA.\n",
                  "msOutputFormatValidate()", format->name );
    else
      msDebug("OUTPUTFORMAT %s has TRANSPARENT set ON, but an IMAGEMODE\n"
              "of RGB instead of RGBA.  Changing imagemode to RGBA.\n",
              format->name );

    format->imagemode = MS_IMAGEMODE_RGBA;
    result = MS_FALSE;
  }

  /* Special checking around RAWMODE image modes. */
  if( format->imagemode == MS_IMAGEMODE_INT16
      || format->imagemode == MS_IMAGEMODE_FLOAT32
      || format->imagemode == MS_IMAGEMODE_BYTE ) {
    if( strncmp(format->driver,"GDAL/",5) != 0 ) {
      result = MS_FALSE;
      if( issue_error )
        msSetError( MS_MISCERR,
                    "OUTPUTFORMAT %s has IMAGEMODE BYTE/INT16/FLOAT32, but this is only supported for GDAL drivers.",
                    "msOutputFormatValidate()", format->name );
      else
        msDebug( "OUTPUTFORMAT %s has IMAGEMODE BYTE/INT16/FLOAT32, but this is only supported for GDAL drivers.",
                 format->name );
    }

    if( format->renderer != MS_RENDER_WITH_RAWDATA ) /* see bug 724 */
      format->renderer = MS_RENDER_WITH_RAWDATA;
  }

  if( !strcasecmp(format->driver,"AGG/MIXED") )
  {
    if( !msGetOutputFormatOption(format, "TRANSPARENT_FORMAT", NULL) )
    {
      result = MS_FALSE;
      if( issue_error )
        msSetError( MS_MISCERR,
                    "OUTPUTFORMAT %s lacks a 'TRANSPARENT_FORMAT' FORMATOPTION.",
                    "msOutputFormatValidate()", format->name );
      else
        msDebug( "OUTPUTFORMAT %s lacks a 'TRANSPARENT_FORMAT' FORMATOPTION.",
                 format->name );
    }
    if( !msGetOutputFormatOption(format, "OPAQUE_FORMAT", NULL) )
    {
      result = MS_FALSE;
      if( issue_error )
        msSetError( MS_MISCERR,
                    "OUTPUTFORMAT %s lacks a 'OPAQUE_FORMAT' FORMATOPTION.",
                    "msOutputFormatValidate()", format->name );
      else
        msDebug( "OUTPUTFORMAT %s lacks a 'OPAQUE_FORMAT' FORMATOPTION.",
                 format->name );
    }
  }

  return result;
}

/************************************************************************/
/*                     msInitializeRendererVTable()                     */
/************************************************************************/

int msInitializeRendererVTable(outputFormatObj *format)
{
  assert(format);
  if(format->vtable) {
    return MS_SUCCESS;
  }
  format->vtable = (rendererVTableObj*)calloc(1,sizeof(rendererVTableObj));

  msInitializeDummyRenderer(format->vtable);

  switch(format->renderer) {
    case MS_RENDER_WITH_SKIA:
      return msPopulateRendererVTableSkia(format->vtable);
    case MS_RENDER_WITH_AGG:
      return msPopulateRendererVTableAGG(format->vtable);
    case MS_RENDER_WITH_UTFGRID:
      return msPopulateRendererVTableUTFGrid(format->vtable);
#ifdef USE_PBF
    case MS_RENDER_WITH_MVT:
      return msPopulateRendererVTableMVT(format->vtable);
#endif
#ifdef USE_CAIRO
    case MS_RENDER_WITH_CAIRO_RASTER:
      return msPopulateRendererVTableCairoRaster(format->vtable);
    case MS_RENDER_WITH_CAIRO_PDF:
      return msPopulateRendererVTableCairoPDF(format->vtable);
    case MS_RENDER_WITH_CAIRO_SVG:
      return msPopulateRendererVTableCairoSVG(format->vtable);
#endif
#ifdef USE_OGL
    case MS_RENDER_WITH_OGL:
      return msPopulateRendererVTableOGL(format->vtable);
#endif
#ifdef USE_KML
    case MS_RENDER_WITH_KML:
      return msPopulateRendererVTableKML(format->vtable);
#endif

    case MS_RENDER_WITH_OGR:
      return msPopulateRendererVTableOGR(format->vtable);

    default:
      msSetError(MS_MISCERR, "unsupported RendererVtable renderer %d",
                 "msInitializeRendererVTable()",format->renderer);
      return MS_FAILURE;
  }
  /* this code should never be executed */
  return MS_FAILURE;
}

/************************************************************************/
/*                    msOutputFormatResolveFromImage()                  */
/************************************************************************/

void msOutputFormatResolveFromImage( mapObj *map, imageObj* img )
{
  outputFormatObj* format = map->outputformat;
  assert( img->format == format );
  assert( img->format->refcount >= 2 );

  if( format->renderer == MS_RENDER_WITH_AGG &&
      strcmp(format->driver, "AGG/MIXED") == 0 &&
      (format->imagemode == MS_IMAGEMODE_RGB ||
       format->imagemode == MS_IMAGEMODE_RGBA) )
  {
    outputFormatObj * new_format;
    int has_non_opaque_pixels = MS_FALSE;
    const char* underlying_driver_type = NULL;
    const char* underlying_driver_name = NULL;

    // Check if the image has non opaque pixels
    if( format->imagemode == MS_IMAGEMODE_RGBA )
    {
      rasterBufferObj rb;
      int ret;

      ret = format->vtable->getRasterBufferHandle(img,&rb);
      assert( ret == MS_SUCCESS );
      (void)ret;
      if( rb.data.rgba.a )
      {
        for(unsigned row=0; row<rb.height && !has_non_opaque_pixels; row++) {
          unsigned char *a;
          a=rb.data.rgba.a+row*rb.data.rgba.row_step;
          for(unsigned col=0; col<rb.width && !has_non_opaque_pixels; col++) {
            if(*a < 255) {
              has_non_opaque_pixels = MS_TRUE;
            }
            a+=rb.data.rgba.pixel_step;
          }
        }
      }
    }

    underlying_driver_type = ( has_non_opaque_pixels ) ?
                                    "TRANSPARENT_FORMAT" : "OPAQUE_FORMAT";
    underlying_driver_name = msGetOutputFormatOption(format, underlying_driver_type,
                                                NULL);
    if( underlying_driver_name == NULL ) {
      msSetError(MS_MISCERR,
                 "Missing %s format option on %s.",
                 "msOutputFormatResolveFromImage()",
                 underlying_driver_type, format->name );
      return;
    }
    new_format = msSelectOutputFormat( map, underlying_driver_name );
    if( new_format == NULL ) {
      msSetError(MS_MISCERR,
                 "Cannot find %s output format.",
                 "msOutputFormatResolveFromImage()",
                 underlying_driver_name );
      return;
    }
    if( new_format->renderer != MS_RENDER_WITH_AGG )
    {
      msSetError(MS_MISCERR,
                 "%s cannot be used as the %s format of %s since it is not AGG based.",
                 "msOutputFormatResolveFromImage()",
                 underlying_driver_name, underlying_driver_type, format->name );
      return;
    }

    msApplyOutputFormat( &(map->outputformat),
                         new_format,
                         has_non_opaque_pixels);

    msFreeOutputFormat( format );
    img->format = map->outputformat;
    img->format->refcount ++;
  }
}
