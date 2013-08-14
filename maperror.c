/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of msSetError(), msDebug() and related functions.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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
 ****************************************************************************/

#include "mapserver.h"
#include "maperror.h"
#include "mapthread.h"
#include "maptime.h"

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdarg.h>



static char *ms_errorCodes[MS_NUMERRORCODES] = {"",
    "Unable to access file.",
    "Memory allocation error.",
    "Incorrect data type.",
    "Symbol definition error.",
    "Regular expression error.",
    "TrueType Font error.",
    "DBASE file error.",
    "GD library error.",
    "Unknown identifier.",
    "Premature End-of-File.",
    "Projection library error.",
    "General error message.",
    "CGI error.",
    "Web application error.",
    "Image handling error.",
    "Hash table error.",
    "Join error.",
    "Search returned no results.",
    "Shapefile error.",
    "Expression parser error.",
    "SDE error.",
    "OGR error.",
    "Query error.",
    "WMS server error.",
    "WMS connection error.",
    "OracleSpatial error.",
    "WFS server error.",
    "WFS connection error.",
    "WMS Map Context error.",
    "HTTP request error.",
    "Child array error.",
    "WCS server error.",
    "GEOS library error.",
    "Invalid rectangle.",
    "Date/time error.",
    "GML encoding error.",
    "SOS server error.",
    "NULL parent pointer error.",
    "AGG library error.",
    "OWS error.",
    "OpenGL renderer error.",
    "Renderer error."
                                               };
#ifndef USE_THREAD

errorObj *msGetErrorObj()
{
  static errorObj ms_error = {MS_NOERR, "", "", MS_FALSE, NULL};

  return &ms_error;
}
#endif

#ifdef USE_THREAD

typedef struct te_info {
  struct te_info *next;
  int             thread_id;
  errorObj        ms_error;
} te_info_t;

static te_info_t *error_list = NULL;

errorObj *msGetErrorObj()
{
  te_info_t *link;
  int        thread_id;
  errorObj   *ret_obj;

  msAcquireLock( TLOCK_ERROROBJ );

  thread_id = msGetThreadId();

  /* find link for this thread */

  for( link = error_list;
       link != NULL && link->thread_id != thread_id
       && link->next != NULL && link->next->thread_id != thread_id;
       link = link->next ) {}

  /* If the target thread link is already at the head of the list were ok */
  if( error_list != NULL && error_list->thread_id == thread_id ) {
  }

  /* We don't have one ... initialize one. */
  else if( link == NULL || link->next == NULL ) {
    te_info_t *new_link;
    errorObj   error_obj = { MS_NOERR, "", "", 0 };

    new_link = (te_info_t *) malloc(sizeof(te_info_t));
    new_link->next = error_list;
    new_link->thread_id = thread_id;
    new_link->ms_error = error_obj;

    error_list = new_link;
  }

  /* If the link is not already at the head of the list, promote it */
  else if( link != NULL && link->next != NULL ) {
    te_info_t *target = link->next;

    link->next = link->next->next;
    target->next = error_list;
    error_list = target;
  }

  ret_obj = &(error_list->ms_error);

  msReleaseLock( TLOCK_ERROROBJ );

  return ret_obj;
}
#endif

/* msInsertErrorObj()
**
** We maintain a chained list of errorObj in which the first errorObj is
** the most recent (i.e. a stack).  msErrorReset() should be used to clear
** the list.
**
** Note that since some code in MapServer will fetch the head of the list and
** keep a handle on it for a while, the head of the chained list is static
** and never changes.
** A new errorObj is always inserted after the head, and only if the
** head of the list already contains some information.  i.e. If the static
** errorObj at the head of the list is empty then it is returned directly,
** otherwise a new object is inserted after the head and the data that was in
** the head is moved to the new errorObj, freeing the head errorObj to receive
** the new error information.
*/
static errorObj *msInsertErrorObj(void)
{
  errorObj *ms_error;
  ms_error = msGetErrorObj();

  if (ms_error->code != MS_NOERR) {
    /* Head of the list already in use, insert a new errorObj after the head
     * and move head contents to this new errorObj, freeing the errorObj
     * for reuse.
     */
    errorObj *new_error;
    new_error = (errorObj *)malloc(sizeof(errorObj));

    /* Note: if malloc() failed then we simply do nothing and the head will
     * be overwritten by the caller... we cannot produce an error here
     * since we are already inside a msSetError() call.
     */
    if (new_error) {
      new_error->next = ms_error->next;
      new_error->code = ms_error->code;
      new_error->isreported = ms_error->isreported;
      strlcpy(new_error->routine, ms_error->routine, sizeof(new_error->routine));
      strlcpy(new_error->message, ms_error->message, sizeof(new_error->message));

      ms_error->next = new_error;
      ms_error->code = MS_NOERR;
      ms_error->isreported = MS_FALSE;
      ms_error->routine[0] = '\0';
      ms_error->message[0] = '\0';
    }
  }

  return ms_error;
}

/* msResetErrorList()
**
** Clear the list of error objects.
*/
void msResetErrorList()
{
  errorObj *ms_error, *this_error;
  ms_error = msGetErrorObj();

  this_error = ms_error->next;
  while( this_error != NULL) {
    errorObj *next_error;

    next_error = this_error->next;
    msFree(this_error);
    this_error = next_error;
  }

  ms_error->next = NULL;
  ms_error->code = MS_NOERR;
  ms_error->routine[0] = '\0';
  ms_error->message[0] = '\0';

  /* -------------------------------------------------------------------- */
  /*      Cleanup our entry in the thread list.  This is mainly           */
  /*      imprortant when msCleanup() calls msResetErrorList().           */
  /* -------------------------------------------------------------------- */
#ifdef USE_THREAD
  {
    int  thread_id = msGetThreadId();
    te_info_t *link;

    msAcquireLock( TLOCK_ERROROBJ );

    /* find link for this thread */

    for( link = error_list;
         link != NULL && link->thread_id != thread_id
         && link->next != NULL && link->next->thread_id != thread_id;
         link = link->next ) {}

    if( link->thread_id == thread_id ) {
      /* presumably link is at head of list.  */
      if( error_list == link )
        error_list = link->next;

      free( link );
    } else if( link->next != NULL && link->next->thread_id == thread_id ) {
      te_info_t *next_link = link->next;
      link->next = link->next->next;
      free( next_link );
    }
    msReleaseLock( TLOCK_ERROROBJ );
  }
#endif
}

char *msGetErrorCodeString(int code)
{

  if(code<0 || code>MS_NUMERRORCODES-1)
    return("Invalid error code.");

  return(ms_errorCodes[code]);
}

/* -------------------------------------------------------------------- */
/*      Adding the displayable error string to a given string           */
/*      and reallocates the memory enough to hold the characters.       */
/*      If source is null returns a newly allocated string              */
/* -------------------------------------------------------------------- */
char *msAddErrorDisplayString(char *source, errorObj *error)
{
  if((source = msStringConcatenate(source, error->routine)) == NULL) return(NULL);
  if((source = msStringConcatenate(source, ": ")) == NULL) return(NULL);
  if((source = msStringConcatenate(source, ms_errorCodes[error->code])) == NULL) return(NULL);
  if((source = msStringConcatenate(source, " ")) == NULL) return(NULL);
  if((source = msStringConcatenate(source, error->message)) == NULL) return(NULL);
  return source;
}

char *msGetErrorString(char *delimiter)
{
  char *errstr=NULL;

  errorObj *error = msGetErrorObj();

  if(!delimiter || !error) return(NULL);

  while(error && error->code != MS_NOERR) {
    if((errstr = msAddErrorDisplayString(errstr, error)) == NULL) return(NULL);

    if(error->next && error->next->code != MS_NOERR) { /* (peek ahead) more errors, use delimiter */
      if((errstr = msStringConcatenate(errstr, delimiter)) == NULL) return(NULL);
    }
    error = error->next;
  }

  return(errstr);
}

void msSetError(int code, const char *message_fmt, const char *routine, ...)
{
  errorObj *ms_error = msInsertErrorObj();
  va_list args;

  ms_error->code = code;

  if(!routine)
    strcpy(ms_error->routine, "");
  else {
    strlcpy(ms_error->routine, routine, sizeof(ms_error->routine));
  }

  if(!message_fmt)
    strcpy(ms_error->message, "");
  else {
    va_start(args, routine);
    vsnprintf( ms_error->message, MESSAGELENGTH, message_fmt, args );
    va_end(args);
  }

  /* Log a copy of errors to MS_ERRORFILE if set (handled automatically inside msDebug()) */
  msDebug("%s: %s %s\n", ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);

}

void msWriteError(FILE *stream)
{
  errorObj *ms_error = msGetErrorObj();

  while (ms_error && ms_error->code != MS_NOERR) {
    msIO_fprintf(stream, "%s: %s %s <br>\n", ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);
    ms_error->isreported = MS_TRUE;
    ms_error = ms_error->next;
  }
}

void msWriteErrorXML(FILE *stream)
{
  char *message;
  errorObj *ms_error = msGetErrorObj();

  while (ms_error && ms_error->code != MS_NOERR) {
    message = msEncodeHTMLEntities(ms_error->message);

    msIO_fprintf(stream, "%s: %s %s\n", ms_error->routine,
                 ms_errorCodes[ms_error->code], message);
    ms_error->isreported = MS_TRUE;
    ms_error = ms_error->next;

    msFree(message);
  }
}

void msWriteErrorImage(mapObj *map, char *filename, int blank)
{
  imageObj *img;
  rendererVTableObj *renderer;
  int font_index = 0;
  int width=400, height=300;
  int nMargin =5;
  int nTextLength = 0;
  int nUsableWidth = 0;
  int nMaxCharsPerLine = 0;
  int nLines = 0;
  int i = 0;
  int nStart = 0;
  int nEnd = 0;
  int nLength = 0;
  char **papszLines = NULL;
  int nXPos = 0;
  int nYPos = 0;
  int nWidthTxt = 0;
  outputFormatObj *format = NULL;
  char *errormsg = msGetErrorString("; ");
  errorObj *error = msGetErrorObj();
  fontMetrics *font = NULL;
  char *imagepath = NULL, *imageurl = NULL;
  labelStyleObj ls;
  colorObj labelcolor, labeloutlinecolor, imagecolor, *imagecolorptr=NULL;
  ls.color = &labelcolor;
  ls.outlinecolor = &labeloutlinecolor;
  if(!errormsg) {
    errormsg = msStrdup("No error found sorry. This is likely a bug");
  }

  if (map) {
    if( map->width > 0 && map->height > 0 ) {
      width = map->width;
      height = map->height;
    }
    format = map->outputformat;
    imagepath = map->web.imagepath;
    imageurl = map->web.imageurl;
  }

  /* Default to GIF if no suitable GD output format set */
  if (format == NULL || !MS_RENDERER_PLUGIN(format) || !format->vtable->supports_bitmap_fonts)
    format = msCreateDefaultOutputFormat( NULL, "AGG/PNG8", "png" );

  if(!format->transparent) {
    if(map && MS_VALID_COLOR(map->imagecolor)) {
      imagecolorptr = &map->imagecolor;
    } else {
      MS_INIT_COLOR(imagecolor,255,255,255,255);
      imagecolorptr = &imagecolor;
    }
  }

  img = msImageCreate(width,height,format,imagepath,imageurl,MS_DEFAULT_RESOLUTION,MS_DEFAULT_RESOLUTION,imagecolorptr);
  renderer = MS_IMAGE_RENDERER(img);

  for(i=0; i<5; i++) {
    /* use the first font we find */
    if((font = renderer->bitmapFontMetrics[font_index]) != NULL) {
      ls.size = i;
      MS_INIT_COLOR(*ls.color,0,0,0,255);
      MS_INIT_COLOR(*ls.outlinecolor,255,255,255,255);
      ls.outlinewidth = 1;
      break;
    }
  }
  /* if no font found we can't do much. this shouldn't happen */
  if(font) {

    nTextLength = strlen(errormsg);
    nWidthTxt  =  nTextLength * font->charWidth;
    nUsableWidth = width - (nMargin*2);

    /* Check to see if it all fits on one line. If not, split the text on several lines. */
    if(!blank) {
      if (nWidthTxt > nUsableWidth) {
        nMaxCharsPerLine =  nUsableWidth/font->charWidth;
        nLines = (int) ceil ((double)nTextLength / (double)nMaxCharsPerLine);
        if (nLines > 0) {
          papszLines = (char **)malloc(nLines*sizeof(char *));
          for (i=0; i<nLines; i++) {
            papszLines[i] = (char *)malloc((nMaxCharsPerLine+1)*sizeof(char));
            papszLines[i][0] = '\0';
          }
        }
        for (i=0; i<nLines; i++) {
          nStart = i*nMaxCharsPerLine;
          nEnd = nStart + nMaxCharsPerLine;
          if (nStart < nTextLength) {
            if (nEnd > nTextLength)
              nEnd = nTextLength;
            nLength = nEnd-nStart;

            strncpy(papszLines[i], errormsg+nStart, nLength);
            papszLines[i][nLength] = '\0';
          }
        }
      } else {
        nLines = 1;
        papszLines = (char **)malloc(nLines*sizeof(char *));
        papszLines[0] = msStrdup(errormsg);
      }
      for (i=0; i<nLines; i++) {
        nYPos = (font->charHeight) * ((i*2) +1);
        nXPos = font->charWidth;;
        renderer->renderBitmapGlyphs(img, nXPos, nYPos, &ls, papszLines[i]);
      }
      if (papszLines) {
        for (i=0; i<nLines; i++) {
          free(papszLines[i]);
        }
        free(papszLines);
      }
    }
  }

  /* actually write the image */
  if(!filename) {
    msIO_setHeader("Content-Type","%s", MS_IMAGE_MIME_TYPE(format));
    msIO_sendHeaders();
  }
  msSaveImage(NULL,img,filename);
  msFreeImage(img);

  /* the errors are reported */
  while(error && error->code != MS_NOERR) {
    error->isreported = MS_TRUE;
    error = error->next;
  }

  if (format->refcount == 0)
    msFreeOutputFormat(format);
  msFree(errormsg);
}

char *msGetVersion()
{
  static char version[1024];

  sprintf(version, "MapServer version %s", MS_VERSION);

#ifdef USE_GD_GIF
  strcat(version, " OUTPUT=GIF");
#endif
#if (defined USE_GD_PNG || defined USE_PNG)
  strcat(version, " OUTPUT=PNG");
#endif
#if (defined USE_GD_JPEG || defined USE_JPEG)
  strcat(version, " OUTPUT=JPEG");
#endif
#ifdef USE_KML
  strcat(version, " OUTPUT=KML");
#endif
#ifdef USE_PROJ
  strcat(version, " SUPPORTS=PROJ");
#endif
#ifdef USE_GD
  strcat(version, " SUPPORTS=GD");
#endif
  strcat(version, " SUPPORTS=AGG");
  strcat(version, " SUPPORTS=FREETYPE");
#ifdef USE_CAIRO
  strcat(version, " SUPPORTS=CAIRO");
#endif
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
  strcat(version, " SUPPORTS=SVG_SYMBOLS");
  #ifdef USE_SVG_CAIRO
    strcat(version, " SUPPORTS=SVGCAIRO");
  #else
    strcat(version, " SUPPORTS=RSVG");
  #endif
#endif
#ifdef USE_OGL
  strcat(version, " SUPPORTS=OPENGL");
#endif
#ifdef USE_ICONV
  strcat(version, " SUPPORTS=ICONV");
#endif
#ifdef USE_EXEMPI
  strcat(version, " SUPPORTS=XMP");
#endif
#ifdef USE_FRIBIDI
  strcat(version, " SUPPORTS=FRIBIDI");
#endif
#ifdef USE_WMS_SVR
  strcat(version, " SUPPORTS=WMS_SERVER");
#endif
#ifdef USE_WMS_LYR
  strcat(version, " SUPPORTS=WMS_CLIENT");
#endif
#ifdef USE_WFS_SVR
  strcat(version, " SUPPORTS=WFS_SERVER");
#endif
#ifdef USE_WFS_LYR
  strcat(version, " SUPPORTS=WFS_CLIENT");
#endif
#ifdef USE_WCS_SVR
  strcat(version, " SUPPORTS=WCS_SERVER");
#endif
#ifdef USE_SOS_SVR
  strcat(version, " SUPPORTS=SOS_SERVER");
#endif
#ifdef USE_FASTCGI
  strcat(version, " SUPPORTS=FASTCGI");
#endif
#ifdef USE_THREAD
  strcat(version, " SUPPORTS=THREADS");
#endif
#ifdef USE_GEOS
  strcat(version, " SUPPORTS=GEOS");
#endif
#ifdef USE_POINT_Z_M
  strcat(version, " SUPPORTS=POINT_Z_M");
#endif
#ifdef USE_JPEG
  strcat(version, " INPUT=JPEG");
#endif
#ifdef USE_SDE
  strcat(version, " INPUT=SDE");
#endif
#ifdef USE_POSTGIS
  strcat(version, " INPUT=POSTGIS");
#endif
#ifdef USE_ORACLESPATIAL
  strcat(version, " INPUT=ORACLESPATIAL");
#endif
#ifdef USE_OGR
  strcat(version, " INPUT=OGR");
#endif
#ifdef USE_GDAL
  strcat(version, " INPUT=GDAL");
#endif
  strcat(version, " INPUT=SHAPEFILE");
  return(version);
}

int msGetVersionInt()
{
  return MS_VERSION_NUM;
}
