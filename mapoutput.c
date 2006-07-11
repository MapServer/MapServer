/******************************************************************************
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
 * Revision 1.36  2006/07/11 01:09:42  frank
 * msCloneOutputFormat() needs to propagate inmapfile flag (bug 1823).
 *
 * Revision 1.35  2005/11/10 15:41:16  frank
 * Added missing function header.
 *
 * Revision 1.34  2005/04/07 17:23:16  assefa
 * Remove #ifdef USE_SVG. It was added during development.
 *
 * Revision 1.33  2005/03/08 19:53:25  frank
 * adjusted png24 mimetype per bug 1231
 *
 * Revision 1.32  2005/02/08 21:36:23  frank
 * Return actual error code.
 *
 * Revision 1.31  2005/02/03 00:06:57  assefa
 * Add SVG function prototypes and related calls.
 *
 * Revision 1.30  2004/11/18 20:55:38  frank
 * In msOutputFormatValidate() we now ensure that GD/JPEG does not have
 * alpha enabled, or transparent turned on.   Bug 1073.
 *
 * Revision 1.29  2004/11/18 19:27:16  frank
 * Fixed formatting of debug message.
 *
 * Revision 1.28  2004/11/17 23:53:07  assefa
 * Advertize only gd and gdal formats for wms capabilities (Bug 455).
 *
 * Revision 1.27  2004/11/05 23:04:50  assefa
 * Change a bit test in function msGetOutputFormatMimeListGD.
 *
 * Revision 1.26  2004/11/05 16:29:40  assefa
 * add function msGetOutputFormatMimeListGD.
 *
 * Revision 1.25  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.24  2004/10/20 20:46:51  frank
 * msOutputFormatValidate() now also fixes stuff up including setting
 * imagemode to MS_RENDER_WITH_RAWDATA if appropriate, and setting
 * imagemode to MS_IMAGEMODE_RGBA from RGB if transparent enabled.
 * See Bugs 724 and 425/977.
 *
 * Revision 1.23  2004/04/23 16:44:54  frank
 * msRemoteOutputFormat() now frees output formats if refcount==0
 *
 * Revision 1.22  2004/03/08 04:28:46  sean
 * New functions to support mapscript interface to outputformatlist
 *
 * Revision 1.21  2004/03/05 05:57:04  frank
 * support multi-band rawmode output
 *
 * Revision 1.20  2003/07/08 12:36:11  frank
 * don't crash if format==NULL
 *
 * Revision 1.19  2003/07/03 03:01:58  assefa
 * Add support for saving the output formats.
 *
 * Revision 1.18  2003/03/14 13:12:11  attila
 * Introduce MySQL generic support, enabled with --with-mygis when configuring
 *
 * Revision 1.17  2003/02/20 19:49:14  frank
 * fixed implicit setting of JPEG quality from IMAGEQUALITY keyword
 *
 * Revision 1.16  2003/01/17 15:47:48  frank
 * ensure that msApplyDefaultOutputFormats() does not modify map->imagetype
 *
 * Revision 1.15  2003/01/11 23:25:48  frank
 * Bug 249: msApplyOutputFormat() will switch imagemode from RGB to RGBA if
 * transparency is requested.
 * Added msOutputFormatValidate() function.
 *
 * Revision 1.14  2003/01/11 17:32:27  frank
 * When GD2 isn't available create a PC256 mode JPEG driver by default
 *
 * Revision 1.13  2002/12/21 22:04:54  frank
 * I regressed the changes Julien made, and reimplemented the same effect
 * with the logic in msApplyDefaultOutputFormats().  Should be much safer now.
 *
 * Revision 1.11  2002/12/17 23:02:19  dan
 * I assigned the wrong value to map->imagetype.  Duh!
 *
 * Revision 1.10  2002/12/17 22:52:43  dan
 * Update value of map->imagetype in msSelectOutputFormat()
 *
 * Revision 1.9  2002/12/17 22:04:58  dan
 * Added PNG24 to the list of default output formats with GD2
 *
 * Revision 1.8  2002/11/19 14:38:39  frank
 * fixed bug in msApplyOutputFormat with transparent override
 *
 * Revision 1.7  2002/11/19 05:47:37  frank
 * cleanup memory leak
 *
 * Revision 1.6  2002/10/05 16:32:40  zak
 * updated e-mail address
 *
 * Revision 1.5  2002/10/04 21:15:59  assefa
 * Add PDF support using the new output architecture. Work done
 * by Zak James (zak-ms@aiya.dhs.org).
 *
 * Revision 1.4  2002/06/21 18:29:45  frank
 * Added support for GD/PC256 as a pseudo-driver
 *
 * Revision 1.3  2002/06/13 19:56:15  frank
 * don't automatically emit Imagine format support
 *
 * Revision 1.2  2002/06/11 20:45:22  frank
 * add renderer copy to output format clone function
 *
 * Revision 1.1  2002/06/11 13:49:52  frank
 * New
 *
 */

#include <assert.h>
#include "map.h"

MS_CVSID("$Id$")

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
    outputFormatObj *format;

    /* Provide default output formats. */
    msApplyDefaultOutputFormats( map );

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

    if( strcasecmp(driver,"GD/PC256") == 0 )
    {
#ifdef USE_GD_GIF
        return msCreateDefaultOutputFormat( map, "GD/GIF" );
#elif defined(USE_GD_PNG)
        return msCreateDefaultOutputFormat( map, "GD/PNG" );
#else
        return NULL;
#endif
    }

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
#endif /* USE_GD_PNG */


#if defined(USE_GD_PNG) && GD2_VERS > 1
    if( strcasecmp(driver,"GD/PNG24") == 0 )
    {
        format = msAllocOutputFormat( map, "png24", "GD/PNG" );
        format->mimetype = strdup("image/png; mode=24bit");
        format->imagemode = MS_IMAGEMODE_RGB;
        format->extension = strdup("png");
        format->renderer = MS_RENDER_WITH_GD;
    }
#endif /* USE_GD_PNG */

#ifdef USE_GD_JPEG
    if( strcasecmp(driver,"GD/JPEG") == 0 )
    {
        format = msAllocOutputFormat( map, "jpeg", driver );
        format->mimetype = strdup("image/jpeg");
#if GD2_VERS > 1
        format->imagemode = MS_IMAGEMODE_RGB;
#else
        format->imagemode = MS_IMAGEMODE_PC256;
#endif
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
#ifdef USE_PDF
    if( strcasecmp(driver,"pdf") == 0 )
    {
        format = msAllocOutputFormat( map, "pdf", driver );
        format->mimetype = strdup("application/x-pdf");
        format->imagemode = MS_IMAGEMODE_PC256;
        format->extension = strdup("pdf");
        format->renderer = MS_RENDER_WITH_PDF;
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
    if( strcasecmp(driver,"imagemap") == 0 )
    {
        format = msAllocOutputFormat( map, "imagemap", driver );
        format->mimetype = strdup("text/html");
        format->extension = strdup("html");
        format->imagemode = 0;
        format->renderer = MS_RENDER_WITH_IMAGEMAP;
    }
    if( strcasecmp(driver,"svg") == 0 )
    {
        format = msAllocOutputFormat( map, "svg", driver );
        format->mimetype = strdup("image/svg+xml");
        format->imagemode = 0;
        format->extension = strdup("svg");
        format->renderer = MS_RENDER_WITH_SVG;
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

    if( map->imagetype == NULL )
        saved_imagetype = NULL;
    else
        saved_imagetype = strdup(map->imagetype);

    if( msSelectOutputFormat( map, "gif" ) == NULL )
        msCreateDefaultOutputFormat( map, "GD/GIF" );

    if( msSelectOutputFormat( map, "png" ) == NULL )
        msCreateDefaultOutputFormat( map, "GD/PNG" );

    if( msSelectOutputFormat( map, "png24" ) == NULL )
        msCreateDefaultOutputFormat( map, "GD/PNG24" );

    if( msSelectOutputFormat( map, "jpeg" ) == NULL )
        msCreateDefaultOutputFormat( map, "GD/JPEG" );

    if( msSelectOutputFormat( map, "wbmp" ) == NULL )
        msCreateDefaultOutputFormat( map, "GD/WBMP" );

    if( msSelectOutputFormat( map, "swf" ) == NULL )
        msCreateDefaultOutputFormat( map, "swf" );

    if( msSelectOutputFormat( map, "imagemap" ) == NULL )
        msCreateDefaultOutputFormat( map, "imagemap" );

    if( msSelectOutputFormat( map, "pdf" ) == NULL )
        msCreateDefaultOutputFormat( map, "pdf" );

    if( msSelectOutputFormat( map, "GTiff" ) == NULL )
        msCreateDefaultOutputFormat( map, "GDAL/GTiff" );

    if( msSelectOutputFormat( map, "svg" ) == NULL )
        msCreateDefaultOutputFormat( map, "svg" );

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
    if( format == NULL )
    {
        msSetError( MS_MEMERR, NULL, "msAllocOutputFormat()" );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Initialize various fields.                                      */
/* -------------------------------------------------------------------- */
    format->bands = 1;
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
    if (map != NULL)
    {
        map->numoutputformats++;
        if (map->outputformatlist == NULL)
            map->outputformatlist = (outputFormatObj **) malloc(sizeof(void*));
        else
            map->outputformatlist = (outputFormatObj **)
                realloc(map->outputformatlist,
                        sizeof(void*) * map->numoutputformats );

        map->outputformatlist[map->numoutputformats-1] = format;
        format->refcount++;
    }
    
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
    if (map != NULL)
    {
        if (map->outputformatlist == NULL)
        {
            msSetError(MS_CHILDERR, "Can't remove format from empty outputformatlist", "msRemoveOutputFormat()");
            return MS_FAILURE;
        }
        else
        {
            i = msGetOutputFormatIndex(map, name);
            if (i >= 0) 
            {
                map->numoutputformats--;
                if( map->outputformatlist[i]->refcount-- < 1 )
                    msFreeOutputFormat( map->outputformatlist[i] );

                for (j=i; j<map->numoutputformats-1; j++)
                {
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
    for (i = 0; i < map->numoutputformats; i++)
    {
        if (map->outputformatlist[i]->mimetype != NULL
            && strcasecmp(imagetype,
                          map->outputformatlist[i]->mimetype) == 0 )
            return i;
    }
    
    for( i = 0; i < map->numoutputformats; i++ )
    {
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
    int i, index;
    outputFormatObj *format = NULL;

    if( map == NULL || imagetype == NULL || strlen(imagetype) == 0 )
        return NULL;
    
    index = msGetOutputFormatIndex(map, imagetype);
    if (index >= 0)
        format = map->outputformatlist[index];

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
    
    if (format)
    {
        if (map->imagetype)
            free(map->imagetype);
        map->imagetype = strdup(format->name);
    }

    if( format != NULL )
        msOutputFormatValidate( format );

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

    msOutputFormatValidate( format );

/* -------------------------------------------------------------------- */
/*      Do we need to change any values?  If not, then just apply       */
/*      and return.                                                     */
/* -------------------------------------------------------------------- */
    if( transparent != MS_NOOVERRIDE && !format->transparent != !transparent )
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
        {
            format->transparent = transparent;
            if( format->imagemode == MS_IMAGEMODE_RGB )
                format->imagemode = MS_IMAGEMODE_RGBA;
        }

        if( imagequality != MS_NOOVERRIDE && imagequality != old_imagequality )
        {
            sprintf( new_value, "%d", imagequality );
            msSetOutputFormatOption( format, "QUALITY", new_value );
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
    dst->renderer = src->renderer;

    dst->transparent = src->transparent;
    dst->bands = src->bands;

    dst->numformatoptions = src->numformatoptions;
    dst->formatoptions = (char **) 
        malloc(sizeof(char *) * src->numformatoptions );

    for( i = 0; i < src->numformatoptions; i++ )
        dst->formatoptions[i] = strdup(src->formatoptions[i]);

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

/************************************************************************/
/*                     msGetOutputFormatMimeList()                      */
/************************************************************************/

void msGetOutputFormatMimeListGD( mapObj *map, char **mime_list, int max_mime )

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

        if( j == mime_count && map->outputformatlist[i]->driver &&
            strncasecmp(map->outputformatlist[i]->driver, "GD/", 3)==0)
            mime_list[mime_count++] = map->outputformatlist[i]->mimetype;
    }

    if( mime_count < max_mime )
        mime_list[mime_count] = NULL;
}


/************************************************************************/
/*                  msGetOutputFormatMimeListRaster()                   */
/************************************************************************/

void msGetOutputFormatMimeListRaster( mapObj *map, char **mime_list, int max_mime )

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

        if( j == mime_count && map->outputformatlist[i]->driver &&
            (strncasecmp(map->outputformatlist[i]->driver, "GD/", 3)==0 ||
             strncasecmp(map->outputformatlist[i]->driver, "GDAL/", 5)==0))
            mime_list[mime_count++] = map->outputformatlist[i]->mimetype;
    }

    if( mime_count < max_mime )
        mime_list[mime_count] = NULL;
}

/************************************************************************/
/*                       msOutputFormatValidate()                       */
/*                                                                      */
/*      Do some validation of the output format, and report to debug    */
/*      output if it doesn't seem valid.  Fixup in place as best as     */
/*      possible.                                                       */
/************************************************************************/

int msOutputFormatValidate( outputFormatObj *format )

{
    int result = MS_TRUE;

    format->bands = 
            atoi(msGetOutputFormatOption( format, "BAND_COUNT", "1" ));

    /* Enforce the requirement that GD/JPEG be RGB and TRANSPARENT=OFF */
    if( strcasecmp(format->driver,"GD/JPEG") == 0 && format->transparent )
    {
        msDebug( "GD/JPEG OUTPUTFORMAT %s has TRANSPARENT set ON, but this is not supported.\n"
                 "It has been disabled.\n", 
                 format->name );
        format->transparent = MS_FALSE;
        result = MS_FALSE;
    }

    if( strcasecmp(format->driver,"GD/JPEG") == 0 
        && format->imagemode == MS_IMAGEMODE_RGBA )
    {
        msDebug( "GD/JPEG OUTPUTFORMAT %s has IMAGEMODE RGBA, but this is not supported.\n"
                 "IMAGEMODE forced to RGB.\n", 
                 format->name );
        format->imagemode = MS_IMAGEMODE_RGB;
        result = MS_FALSE;
    }

    if( format->transparent && format->imagemode == MS_IMAGEMODE_RGB )
    {
        msDebug( "OUTPUTFORMAT %s has TRANSPARENT set ON, but an IMAGEMODE\n"
                 " of RGB instead of RGBA.  Changing imagemode to RGBA.\n", 
                 format->name );
        format->imagemode = MS_IMAGEMODE_RGBA;
        result = MS_FALSE;
    }

    /* see bug 724 */
    if( ( format->imagemode == MS_IMAGEMODE_INT16 
          || format->imagemode == MS_IMAGEMODE_FLOAT32 
          || format->imagemode == MS_IMAGEMODE_BYTE )
        && format->renderer != MS_RENDER_WITH_RAWDATA )
        format->renderer = MS_RENDER_WITH_RAWDATA;

    return result;
}
