/******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.75.2.3  2006/12/06 05:51:18  sdlime
 * Make sure to declare errormsg at the top of the function for MSVC.
 *
 * Revision 1.75.2.2  2006/12/06 05:44:21  sdlime
 * Applied user supplied patch to fix long in-image error wrapping. (bug 1963)
 *
 * Revision 1.75.2.1  2006/11/24 22:11:27  frank
 * fix closing of stderr/stdout after writing error msg (bug 1970)
 *
 * Revision 1.75  2006/03/16 22:28:38  tamas
 * Fixed msGetErrorString so as not to truncate the length of the error messages
 * Added msAddErrorDisplayString to read the displayable messages separatedly
 *
 * Revision 1.74  2006/03/15 18:17:14  dan
 * Added SUPPORTS=SOS_SERVER to msGetVersion() (bug 1710)
 *
 * Revision 1.73  2006/03/14 03:17:19  assefa
 * Add SOS error code (Bug 1710).
 * Correct error codes numbers for MS_TIMEERR and MS_GMLERR.
 *
 * Revision 1.72  2006/02/18 21:11:35  hobu
 * make sure we cast the tv_sec to a long because on os x, the type is
 * really __darwin_suseconds_t, which is ultimately a long.
 *
 * Revision 1.71  2005/12/12 16:57:56  sean
 * write whether point/shape z and m is supported in version message
 *
 * Revision 1.70  2005/11/17 14:38:21  assefa
 * Correct function msGetVersion to indicate if mapserver was build with
 * MYGIS support.
 *
 * Revision 1.69  2005/09/13 23:43:31  frank
 * fix leak in threaded case of error object
 *
 * Revision 1.68  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.67  2005/04/07 21:51:22  frank
 * added SUPPORTS=THREADS to version info
 *
 * Revision 1.66  2005/04/07 17:23:16  assefa
 * Remove #ifdef USE_SVG. It was added during development.
 *
 * Revision 1.65  2005/04/07 13:46:46  frank
 * added report of SVG support
 *
 * Revision 1.64  2005/02/22 07:40:27  sdlime
 * A bunch of updates to GEOS integration. Can move many primatives between MapServer and GEOS, still need to do collections (e.g. multi-point/line/polygon). Added buffer method to mapscript (mapscript/shape.i).
 *
 * Revision 1.63  2005/02/18 03:06:45  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.62  2005/02/02 02:21:50  sdlime
 * Applied Jerry/Dan patch for bug 1194.
 *
 * Revision 1.61  2005/01/28 06:16:53  sdlime
 * Applied patch to make function prototypes ANSI C compliant. Thanks to Petter Reinholdtsen. This fixes but 1181.
 *
 * Revision 1.60  2005/01/26 14:42:13  frank
 * Removed msWebDebug() ... really this time!
 *
 * Revision 1.59  2005/01/07 18:51:09  sdlime
 * Added MS_GMLERR code.
 *
 * Revision 1.58  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"
#include "maperror.h"
#include "mapthread.h"
#include "maptime.h"

#include "gdfonts.h"

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdarg.h>

#ifdef NEED_NONBLOCKING_STDERR
#include <fcntl.h>
#endif

MS_CVSID("$Id$")

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
                                                "SOS server error."
};

#ifndef USE_THREAD

errorObj *msGetErrorObj()
{
    static errorObj ms_error = {MS_NOERR, "", "", NULL};

    return &ms_error;
}
#endif

#ifdef USE_THREAD

typedef struct te_info
{
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
    if( error_list != NULL && error_list->thread_id == thread_id )
    {
    }

    /* We don't have one ... initialize one. */
    else if( link == NULL || link->next == NULL )
    {
        te_info_t *new_link;
        errorObj   error_obj = { MS_NOERR, "", "", NULL };

        new_link = (te_info_t *) malloc(sizeof(te_info_t));
        new_link->next = error_list;
        new_link->thread_id = thread_id;
        new_link->ms_error = error_obj;

        error_list = new_link;
    }

    /* If the link is not already at the head of the list, promote it */
    else if( link != NULL && link->next != NULL )
    {
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

  if (ms_error->code != MS_NOERR)
  {
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
      if (new_error)
      {
          new_error->next = ms_error->next;
          new_error->code = ms_error->code;
          strcpy(new_error->routine, ms_error->routine);
          strcpy(new_error->message, ms_error->message);

          ms_error->next = new_error;
          ms_error->code = MS_NOERR;
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
  while( this_error != NULL)
  {
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
      
      if( link->thread_id == thread_id )
      { 
          /* presumably link is at head of list.  */
          if( error_list == link )
              error_list = link->next;

          free( link );
      }
      else if( link->next != NULL && link->next->thread_id == thread_id )
      {
          te_info_t *next_link = link->next;
          link->next = link->next->next;
          free( next_link );
      }
      msReleaseLock( TLOCK_ERROROBJ );
  }
#endif
}

char *msGetErrorCodeString(int code) {
  
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
	if((source = strcatalloc(source, error->routine)) == NULL) return(NULL);
	if((source = strcatalloc(source, ": ")) == NULL) return(NULL);
	if((source = strcatalloc(source, ms_errorCodes[error->code])) == NULL) return(NULL);
	if((source = strcatalloc(source, " ")) == NULL) return(NULL);
	if((source = strcatalloc(source, error->message)) == NULL) return(NULL);
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
		if((errstr = strcatalloc(errstr, delimiter)) == NULL) return(NULL);
	}
    error = error->next;   
  }

  return(errstr);
}

void msSetError(int code, const char *message_fmt, const char *routine, ...)
{
  char *errfile=NULL;
  FILE *errstream;
  time_t errtime;
  errorObj *ms_error = msInsertErrorObj();
  va_list args;

  ms_error->code = code;

  if(!routine)
    strcpy(ms_error->routine, "");
  else {
    strncpy(ms_error->routine, routine, ROUTINELENGTH);
    ms_error->routine[ROUTINELENGTH-1] = '\0';
  }

  if(!message_fmt)
    strcpy(ms_error->message, "");
  else
  {
    va_start(args, routine);
    vsprintf( ms_error->message, message_fmt, args );
    va_end(args);
  }

  errfile = getenv("MS_ERRORFILE");
  if(errfile) {
    if(strcmp(errfile, "stderr") == 0)
      errstream = stderr;
    else if(strcmp(errfile, "stdout") == 0)
      errstream = stdout;
    else
      errstream = fopen(errfile, "a");
    if(!errstream) return;
    errtime = time(NULL);
    fprintf(errstream, "%s - %s: %s %s\n", chop(ctime(&errtime)), ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);
    if( errstream != stderr && errstream != stdout )
        fclose(errstream);
  }
}

void msWriteError(FILE *stream)
{
  errorObj *ms_error = msGetErrorObj();

  while (ms_error && ms_error->code != MS_NOERR)
  {
      msIO_fprintf(stream, "%s: %s %s <br>\n", ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);
      ms_error = ms_error->next;
  }
}

void msWriteErrorXML(FILE *stream)
{
  char *message;
  errorObj *ms_error = msGetErrorObj();

  while (ms_error && ms_error->code != MS_NOERR)
  {
      message = msEncodeHTMLEntities(ms_error->message);

      msIO_fprintf(stream, "%s: %s %s\n", ms_error->routine, 
                   ms_errorCodes[ms_error->code], message);
      ms_error = ms_error->next;

      msFree(message);
  }
}

void msWriteErrorImage(mapObj *map, char *filename, int blank) {
  gdFontPtr font = gdFontSmall;
  gdImagePtr img=NULL;
  int width=400, height=300, color;
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
  int nSpaceBewteenLines = font->h;
  int nBlack = 0;   
  outputFormatObj *format = NULL;
  char *errormsg = msGetErrorString("; ");

  if(map) {
    if(map->width != -1 && map->height != -1) {
      width = map->width;
      height = map->height;
    }
    format = map->outputformat;
  }

  if (format == NULL) format = msCreateDefaultOutputFormat( NULL, "GD/PC256" );

  img = gdImageCreate(width, height);
  color = gdImageColorAllocate(img, map->imagecolor.red, 
                               map->imagecolor.green,
                               map->imagecolor.blue); /* BG color */
  nBlack = gdImageColorAllocate(img, 0,0,0); /* Text color */

  if (map->outputformat && map->outputformat->transparent)
    gdImageColorTransparent(img, 0);

  nTextLength = strlen(errormsg); 
  nWidthTxt  =  nTextLength * font->w;
  nUsableWidth = width - (nMargin*2);

  /* Check to see if it all fits on one line. If not, split the text on several lines. */
  if(!blank) {
    if (nWidthTxt > nUsableWidth) {
      nMaxCharsPerLine =  nUsableWidth/font->w;
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
      papszLines[0] = strdup(errormsg);
    }   
    for (i=0; i<nLines; i++) {
      nYPos = (nSpaceBewteenLines) * ((i*2) +1); 
      nXPos = nSpaceBewteenLines;

      gdImageString(img, font, nXPos, nYPos, (unsigned char *)papszLines[i], nBlack);
    }
    if (papszLines) {
      for (i=0; i<nLines; i++) {
	free(papszLines[i]);
      }
      free(papszLines);
    }
  }

  /* actually write the image */
  if(!filename) 
      msIO_printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(format), 10,10);
  msSaveImageGD(img, filename, format);
  gdImageDestroy(img);

  if (format->refcount == 0)
    msFreeOutputFormat(format);
  msFree(errormsg);  
}

char *msGetVersion() {
  static char version[1024];

  sprintf(version, "MapServer version %s", MS_VERSION);

#ifdef USE_GD_GIF
  strcat(version, " OUTPUT=GIF");
#endif
#ifdef USE_GD_PNG
  strcat(version, " OUTPUT=PNG");
#endif
#ifdef USE_GD_JPEG
  strcat(version, " OUTPUT=JPEG");
#endif
#ifdef USE_GD_WBMP
  strcat(version, " OUTPUT=WBMP");
#endif
#ifdef USE_PDF
  strcat(version, " OUTPUT=PDF");
#endif
#ifdef USE_MING_FLASH
  strcat(version, " OUTPUT=SWF");
#endif
  strcat(version, " OUTPUT=SVG");
#ifdef USE_PROJ
  strcat(version, " SUPPORTS=PROJ");
#endif
#ifdef USE_GD_FT
  strcat(version, " SUPPORTS=FREETYPE");
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
#ifdef USE_TIFF
  strcat(version, " INPUT=TIFF");
#endif
#ifdef USE_EPPL
  strcat(version, " INPUT=EPPL7");
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
#ifdef USE_MYGIS
  strcat(version, " INPUT=MYGIS");
#endif
  strcat(version, " INPUT=SHAPEFILE");
#ifdef ENABLE_STDERR_DEBUG
  strcat(version, " DEBUG=MSDEBUG");
#endif
  return(version);
}

void msDebug( const char * pszFormat, ... )
{
#ifdef ENABLE_STDERR_DEBUG
    va_list args;

#if defined(NEED_NONBLOCKING_STDERR) && !defined(USE_MAPIO) && !defined(_WIN32)
    static char nonblocking_set = 0;
    if (!nonblocking_set)
    {
        fcntl(fileno(stderr), F_SETFL, O_NONBLOCK);
        nonblocking_set = 1;
    }
#endif

#if !defined(USE_FASTCGI) && !defined(_WIN32)
    /* It seems the FastCGI stuff inserts a timestamp anyways, so  */
    /* we might as well skip this one.  And the struct timeval doesn't */
    /* appear to exist on win32.  */
    {
        struct timeval tv;
        msGettimeofday(&tv, NULL);
        msIO_fprintf(stderr, "[%s].%ld ", 
                     chop(ctime(&(tv.tv_sec))), (long)tv.tv_usec);
    }
#endif

    va_start(args, pszFormat);
    msIO_vfprintf(stderr, pszFormat, args);
    va_end(args);
#endif
}

