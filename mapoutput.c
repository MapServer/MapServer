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
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.1  2002/06/11 13:49:52  frank
 * New
 *
 */

#include <assert.h>
#include "map.h"

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

/************************************************************************/
/*                  msPostMapParseOutputFormatSetup()                   */
/************************************************************************/

int msPostMapParseOutputFormatSetup( mapObj *map )

{
    int added_default_formats = 0;
    outputFormatObj *format;

    /* Provide default output formats if none specified. */
    if(map->numoutputformats == 0)
    {
        added_default_formats = 1;
        msApplyDefaultOutputFormats( map );
    }

    /* default to the first of these if IMAGETYPE not set. */
    if( map->imagetype == NULL && map->numoutputformats > 0 )
        map->imagetype = strdup(map->outputformatlist[0]->name);

    /* select the current outputformat into map->outputformat */
    format = msSelectOutputFormat( map, map->imagetype );
    if( format == NULL ) 
    {
        msSetError(MS_MISCERR, 
                   "Unable to select IMAGETYPE `%s'.", 
                   "msPostMapParseOutputFormatSetup()",
                   map->imagetype ? map->imagetype : "(null)" );
        return MS_FAILURE;
    }

    msApplyOutputFormat( &(map->outputformat), format, 
                         map->transparent, map->interlace, map->imagequality );

    return MS_SUCCESS;
}

/************************************************************************/
/*                    msCreateDefaultOutputFormat()                     */
/************************************************************************/

outputFormatObj *msCreateDefaultOutputFormat( mapObj *map, 
                                              const char *driver )

{
    outputFormatObj *format = NULL;

#ifdef USE_GD_GIF
    if( strcasecmp(driver,"GD/GIF") == 0 )
    {
        format = msAllocOutputFormat( map, "gif", driver );
        format->mimetype = strdup("image/gif");
        format->imagemode = MS_IMAGEMODE_PC256;
        format->extension = strdup("gif");
        format->renderer = MS_RENDER_WITH_GD;
    }
#endif

#ifdef USE_GD_PNG
    if( strcasecmp(driver,"GD/PNG") == 0 )
    {
        format = msAllocOutputFormat( map, "png", driver );
        format->mimetype = strdup("image/png");
        format->imagemode = MS_IMAGEMODE_PC256;
        format->extension = strdup("png");
        format->renderer = MS_RENDER_WITH_GD;
    }
#endif

#ifdef USE_GD_JPEG
    if( strcasecmp(driver,"GD/JPEG") == 0 )
    {
        format = msAllocOutputFormat( map, "jpeg", driver );
        format->mimetype = strdup("image/jpeg");
        format->imagemode = MS_IMAGEMODE_RGB;
        format->extension = strdup("jpg");
        format->renderer = MS_RENDER_WITH_GD;
    }
#endif
#ifdef USE_GD_WBMP
    if( strcasecmp(driver,"GD/WBMP") == 0 )
    {
        format = msAllocOutputFormat( map, "wbmp", driver );
        format->mimetype = strdup("image/wbmp");
        format->imagemode = MS_IMAGEMODE_PC256;
        format->extension = strdup("wbmp");
        format->renderer = MS_RENDER_WITH_GD;
    }
#endif
#ifdef USE_MING_FLASH
    if( strcasecmp(driver,"swf") == 0 )
    {
        format = msAllocOutputFormat( map, "swf", driver );
        format->mimetype = strdup("application/x-shockwave-flash");
        format->imagemode = MS_IMAGEMODE_PC256;
        format->extension = strdup("swf");
        format->renderer = MS_RENDER_WITH_SWF;
    }
#endif
#ifdef USE_GDAL
    if( strncasecmp(driver,"gdal/",5) == 0 )
    {
        format = msAllocOutputFormat( map, driver+5, driver );
        if( msInitDefaultGDALOutputFormat( format ) == MS_FAILURE )
        {
            if( map != NULL )
            {
                map->numoutputformats--;
                map->outputformatlist[map->numoutputformats] = NULL;
            }

            msFreeOutputFormat( format );
            format = NULL;
        }
    }
#endif

    return format;
}

/************************************************************************/
/*                    msApplyDefaultOutputFormats()                     */
/************************************************************************/

void msApplyDefaultOutputFormats( mapObj *map )

{
    if( map->numoutputformats > 0 )
        return;

    msCreateDefaultOutputFormat( map, "GD/GIF" );
    msCreateDefaultOutputFormat( map, "GD/PNG" );
    msCreateDefaultOutputFormat( map, "GD/JPEG" );
    msCreateDefaultOutputFormat( map, "GD/WBMP" );
    msCreateDefaultOutputFormat( map, "swf" );

#ifdef USE_GDAL
    msCreateDefaultOutputFormat( map, "GDAL/GTiff" );
    msCreateDefaultOutputFormat( map, "GDAL/HFA" );
#endif

    assert( map->numoutputformats > 0 );
}

/************************************************************************/
/*                         msFreeOutputFormat()                         */
/************************************************************************/

void msFreeOutputFormat( outputFormatObj *format )

{
    if( format == NULL )
        return;

    msFree( format->name );
    msFree( format->mimetype );
    msFree( format->driver );
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
    if( format == NULL )
    {
        msSetError( MS_MEMERR, NULL, "msAllocOutputFormat()" );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Initialize various fields.                                      */
/* -------------------------------------------------------------------- */
    format->name = strdup(name);
    format->driver = strdup(driver);
    format->refcount = 0;

    format->imagemode = MS_IMAGEMODE_PC256;

/* -------------------------------------------------------------------- */
/*      Attach to map.                                                  */
/* -------------------------------------------------------------------- */
    if( map != NULL )
    {
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
/*                        msSelectOutputFormat()                        */
/************************************************************************/

outputFormatObj *msSelectOutputFormat( mapObj *map, 
                                       const char *imagetype )

{
    int   i;
    outputFormatObj *format = NULL;

    if( map == NULL || imagetype == NULL || strlen(imagetype) == 0 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Try to find the format in the maps list of formats, first by    */
/*      mime type, and then by output format name.                      */
/* -------------------------------------------------------------------- */
    for( i = 0; i < map->numoutputformats && format == NULL; i++ )
    {
        if( map->outputformatlist[i]->mimetype != NULL
            && strcasecmp(imagetype,
                          map->outputformatlist[i]->mimetype) == 0 )
            format = map->outputformatlist[i];
    }
    
    for( i = 0; i < map->numoutputformats && format == NULL; i++ )
    {
        if( strcasecmp(imagetype,map->outputformatlist[i]->name) == 0 )
            format = map->outputformatlist[i];
    }
        
    return format;
}

/************************************************************************/
/*                        msApplyOutputFormat()                         */
/************************************************************************/

void msApplyOutputFormat( outputFormatObj **target, 
                          outputFormatObj *format,
                          int transparent, 
                          int interlaced, 
                          int imagequality )

{
    int       change_needed = MS_FALSE;
    int       old_imagequality, old_interlaced;
    outputFormatObj *formatToFree = NULL;

    assert( target != NULL );

    
    if( *target != NULL && --((*target)->refcount) < 1 )
    {
        formatToFree = *target;
        *target = NULL;
    }

    if( format == NULL )
    {
        if( formatToFree )
            msFreeOutputFormat( formatToFree );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Do we need to change any values?  If not, then just apply       */
/*      and return.                                                     */
/* -------------------------------------------------------------------- */
    if( transparent != MS_NOOVERRIDE && !format->transparent == !transparent )
        change_needed = MS_TRUE;

    old_imagequality = atoi(msGetOutputFormatOption( format, "QUALITY", "75"));
    if( imagequality != MS_NOOVERRIDE && old_imagequality != imagequality )
        change_needed = MS_TRUE;

    old_interlaced = 
        strcasecmp(msGetOutputFormatOption( format, "INTERLACE", "ON"),
                   "OFF") != 0;
    if( interlaced != MS_NOOVERRIDE && !interlaced != !old_interlaced )
        change_needed = MS_TRUE;

    if( change_needed )
    {
        char new_value[128];

        if( format->refcount > 0 )
            format = msCloneOutputFormat( format );

        if( transparent != MS_NOOVERRIDE )
            format->transparent = transparent;

        if( imagequality != MS_NOOVERRIDE && imagequality != old_imagequality )
        {
            sprintf( new_value, "%d", imagequality );
            msSetOutputFormatOption( format, "IMAGEQUALITY", new_value );
        }

        if( interlaced != MS_NOOVERRIDE && !interlaced != !old_interlaced )
        {
            if( interlaced )
                msSetOutputFormatOption( format, "INTERLACE", "ON" );
            else
                msSetOutputFormatOption( format, "INTERLACE", "OFF" );
        }
    }

    *target = format;
    format->refcount++;

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
        dst->mimetype = strdup( src->mimetype );
    else
        dst->mimetype = NULL;

    msFree( dst->extension );
    if( src->extension )
        dst->extension = strdup( src->extension );
    else
        dst->extension = NULL;

    dst->imagemode = src->imagemode;

    dst->transparent = src->transparent;

    dst->numformatoptions = src->numformatoptions;
    dst->formatoptions = (char **) 
        malloc(sizeof(char *) * src->numformatoptions );

    for( i = 0; i < src->numformatoptions; i++ )
        dst->formatoptions[i] = strdup(src->formatoptions[i]);

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

    for( i = 0; i < format->numformatoptions; i++ )
    {
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

/* -------------------------------------------------------------------- */
/*      Format the name=value pair into a newly allocated string.       */
/* -------------------------------------------------------------------- */
    newline = (char *) malloc(strlen(key)+strlen(value)+2);
    if( newline == NULL )
    {
        assert( newline != NULL );
        return;
    }

    sprintf( newline, "%s=%s", key, value );
    
/* -------------------------------------------------------------------- */
/*      Does this key already occur?  If so replace it.                 */
/* -------------------------------------------------------------------- */
    len = strlen(key);

    for( i = 0; i < format->numformatoptions; i++ )
    {
        if( strncasecmp(format->formatoptions[i],key,len) == 0
            && format->formatoptions[i][len] == '=' )
        {
            free( format->formatoptions[i] );
            format->formatoptions[i] = newline;
            return;
        }
    }

/* -------------------------------------------------------------------- */
/*      Otherwise, we need to grow the list.                            */
/* -------------------------------------------------------------------- */
    format->numformatoptions++;
    format->formatoptions = (char **) 
        realloc( format->formatoptions, 
                 sizeof(char*) * format->numformatoptions );

    format->formatoptions[format->numformatoptions-1] = newline;
}

/************************************************************************/
/*                     msGetOutputFormatMimeList()                      */
/************************************************************************/

void msGetOutputFormatMimeList( mapObj *map, char **mime_list, int max_mime )

{
    int mime_count = 0, i;

    for( i = 0; i < map->numoutputformats && mime_count < max_mime; i++ )
    {
        int  j;
        
        if( map->outputformatlist[i]->mimetype == NULL )
            continue;

        for( j = 0; j < mime_count; j++ )
        {
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
