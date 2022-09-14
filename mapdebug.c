/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of debug/logging, msDebug() and related functions.
 * Author:   Daniel Morissette
 *
 ******************************************************************************
 * Copyright (c) 1996-2007 Regents of the University of Minnesota.
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

#ifdef NEED_NONBLOCKING_STDERR
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <windows.h> /* OutputDebugStringA() */
#endif




#ifndef USE_THREAD

debugInfoObj *msGetDebugInfoObj()
{
  static debugInfoObj debuginfo = {MS_DEBUGLEVEL_ERRORSONLY,
                                   MS_DEBUGMODE_OFF, NULL, NULL
                                  };
  return &debuginfo;
}

#else

static debugInfoObj *debuginfo_list = NULL;

debugInfoObj *msGetDebugInfoObj()
{
  debugInfoObj *link;
  void* thread_id;
  debugInfoObj *ret_obj;

  msAcquireLock( TLOCK_DEBUGOBJ );

  thread_id = msGetThreadId();

  /* find link for this thread */

  for( link = debuginfo_list;
       link != NULL && link->thread_id != thread_id
       && link->next != NULL && link->next->thread_id != thread_id;
       link = link->next ) {}

  /* If the target thread link is already at the head of the list were ok */
  if( debuginfo_list != NULL && debuginfo_list->thread_id == thread_id ) {
  }

  /* We don't have one ... initialize one. */
  else if( link == NULL || link->next == NULL ) {
    debugInfoObj *new_link;

    new_link = (debugInfoObj *) msSmallMalloc(sizeof(debugInfoObj));
    new_link->next = debuginfo_list;
    new_link->thread_id = thread_id;
    new_link->global_debug_level = MS_DEBUGLEVEL_ERRORSONLY;
    new_link->debug_mode = MS_DEBUGMODE_OFF;
    new_link->errorfile = NULL;
    new_link->fp = NULL;
    debuginfo_list = new_link;
  }

  /* If the link is not already at the head of the list, promote it */
  else if( link != NULL && link->next != NULL ) {
    debugInfoObj *target = link->next;

    link->next = link->next->next;
    target->next = debuginfo_list;
    debuginfo_list = target;
  }

  ret_obj = debuginfo_list;

  msReleaseLock( TLOCK_DEBUGOBJ );

  return ret_obj;
}
#endif


/* msSetErrorFile()
**
** Set output target, ready to write to it, open file if necessary
**
** If pszRelToPath != NULL then we will try to make the value relative to
** this path if it is not absolute already and it's not one of the special
** values (stderr, stdout, windowsdebug)
**
** Returns MS_SUCCESS/MS_FAILURE
*/
int msSetErrorFile(const char *pszErrorFile, const char *pszRelToPath)
{
  char extended_path[MS_MAXPATHLEN];
  debugInfoObj *debuginfo = msGetDebugInfoObj();

  if (strcmp(pszErrorFile, "stderr") != 0 &&
      strcmp(pszErrorFile, "stdout") != 0 &&
      strcmp(pszErrorFile, "windowsdebug") != 0) {
    /* Try to make the path relative */
    if(msBuildPath(extended_path, pszRelToPath, pszErrorFile) == NULL)
      return MS_FAILURE;
    pszErrorFile = extended_path;
  }

  if (debuginfo->errorfile && pszErrorFile &&
      strcmp(debuginfo->errorfile, pszErrorFile) == 0) {
    /* Nothing to do, already writing to the right place */
    return MS_SUCCESS;
  }

  /* Close current output file if any */
  msCloseErrorFile();

  /* NULL or empty target will just close current output and return */
  if (pszErrorFile == NULL || *pszErrorFile == '\0')
    return MS_SUCCESS;

  if (strcmp(pszErrorFile, "stderr") == 0) {
    debuginfo->fp = stderr;
    debuginfo->errorfile = msStrdup(pszErrorFile);
    debuginfo->debug_mode = MS_DEBUGMODE_STDERR;
  } else if (strcmp(pszErrorFile, "stdout") == 0) {
    debuginfo->fp = stdout;
    debuginfo->errorfile = msStrdup(pszErrorFile);
    debuginfo->debug_mode = MS_DEBUGMODE_STDOUT;
  } else if (strcmp(pszErrorFile, "windowsdebug") == 0) {
#ifdef _WIN32
    debuginfo->errorfile = msStrdup(pszErrorFile);
    debuginfo->fp = NULL;
    debuginfo->debug_mode = MS_DEBUGMODE_WINDOWSDEBUG;
#else
    msSetError(MS_MISCERR, "'MS_ERRORFILE windowsdebug' is available only on Windows platforms.", "msSetErrorFile()");
    return MS_FAILURE;
#endif
  } else {
    debuginfo->fp = fopen(pszErrorFile, "a");
    if (debuginfo->fp == NULL) {
      msSetError(MS_MISCERR, "Failed to open MS_ERRORFILE %s", "msSetErrorFile()", pszErrorFile);
      return MS_FAILURE;
    }
    debuginfo->errorfile = msStrdup(pszErrorFile);
    debuginfo->debug_mode = MS_DEBUGMODE_FILE;
  }

  return MS_SUCCESS;
}

/* msCloseErrorFile()
**
** Close current output file (if one is open) and reset related members
*/
void msCloseErrorFile()
{
  debugInfoObj *debuginfo = msGetDebugInfoObj();

  if (debuginfo && debuginfo->debug_mode != MS_DEBUGMODE_OFF) {
    if (debuginfo->fp && debuginfo->debug_mode == MS_DEBUGMODE_FILE)
      fclose(debuginfo->fp);

    if (debuginfo->fp && (debuginfo->debug_mode == MS_DEBUGMODE_STDERR ||
                          debuginfo->debug_mode == MS_DEBUGMODE_STDOUT))
      fflush(debuginfo->fp); /* just flush stderr or stdout */

    debuginfo->fp = NULL;

    msFree(debuginfo->errorfile);
    debuginfo->errorfile = NULL;

    debuginfo->debug_mode = MS_DEBUGMODE_OFF;
  }
}



/* msGetErrorFile()
**
** Returns name of current error file
**
** Returns NULL if not set.
*/
const char *msGetErrorFile()
{
  debugInfoObj *debuginfo = msGetDebugInfoObj();

  if (debuginfo)
    return debuginfo->errorfile;

  return NULL;
}

/* Set/Get the current global debug level value, used as default value for
** new map and layer objects and to control msDebug() calls outside of
** the context of mapObj or layerObj.
**
*/
void msSetGlobalDebugLevel(int level)
{
  debugInfoObj *debuginfo = msGetDebugInfoObj();

  if (debuginfo)
    debuginfo->global_debug_level = (debugLevel)level;
}

debugLevel msGetGlobalDebugLevel()
{
  debugInfoObj *debuginfo = msGetDebugInfoObj();

  if (debuginfo)
    return debuginfo->global_debug_level;

  return MS_DEBUGLEVEL_ERRORSONLY;
}


/* msDebugInitFromEnv()
**
** Init debug state from MS_ERRORFILE and MS_DEBUGLEVEL env vars if set
**
** Returns MS_SUCCESS/MS_FAILURE
*/
int msDebugInitFromEnv()
{
  const char *val;

  if( (val=getenv( "MS_ERRORFILE" )) != NULL ) {
    if ( msSetErrorFile(val, NULL) != MS_SUCCESS )
      return MS_FAILURE;
  }

  if( (val=getenv( "MS_DEBUGLEVEL" )) != NULL )
    msSetGlobalDebugLevel(atoi(val));

  return MS_SUCCESS;
}


/* msDebugCleanup()
**
** Called by msCleanup to remove info related to this thread.
*/
void msDebugCleanup()
{
  /* make sure file is closed */
  msCloseErrorFile();

#ifdef USE_THREAD
  {
    void*  thread_id = msGetThreadId();
    debugInfoObj *link;

    msAcquireLock( TLOCK_DEBUGOBJ );

    /* find link for this thread */

    for( link = debuginfo_list;
         link != NULL && link->thread_id != thread_id
         && link->next != NULL && link->next->thread_id != thread_id;
         link = link->next ) {}

    if( link->thread_id == thread_id ) {
      /* presumably link is at head of list.  */
      if( debuginfo_list == link )
        debuginfo_list = link->next;

      free( link );
    } else if( link->next != NULL && link->next->thread_id == thread_id ) {
      debugInfoObj *next_link = link->next;
      link->next = link->next->next;
      free( next_link );
    }
    msReleaseLock( TLOCK_DEBUGOBJ );
  }
#endif

}

/* msDebug()
**
** Outputs/logs messages to the MS_ERRORFILE if one is set
** (see msSetErrorFile())
**
*/
void msDebug( const char * pszFormat, ... )
{
  va_list args;
  debugInfoObj *debuginfo = msGetDebugInfoObj();
  char szMessage[MESSAGELENGTH];

  if (debuginfo == NULL || debuginfo->debug_mode == MS_DEBUGMODE_OFF)
    return;  /* Don't waste time here! */

  va_start(args, pszFormat);
  vsnprintf( szMessage, MESSAGELENGTH, pszFormat, args );
  va_end(args);
  szMessage[MESSAGELENGTH-1] = '\0';

  msRedactCredentials(szMessage);

  if (debuginfo->fp) {
    /* Writing to a stdio file handle */

#if defined(USE_FASTCGI)
    /* It seems the FastCGI stuff inserts a timestamp anyways, so  */
    /* we might as well skip this one if writing to stderr w/ FastCGI. */
    if (debuginfo->debug_mode != MS_DEBUGMODE_STDERR)
#endif
    {
      struct mstimeval tv;
      time_t t;
      msGettimeofday(&tv, NULL);
      t = tv.tv_sec;
      msIO_fprintf(debuginfo->fp, "[%s].%ld ",
                   msStringChop(ctime(&t)), (long)tv.tv_usec);
    }

    msIO_fprintf(debuginfo->fp, "%s", szMessage);
  }
#ifdef _WIN32
  else if (debuginfo->debug_mode == MS_DEBUGMODE_WINDOWSDEBUG) {
    /* Writing to Windows Debug Console */
    OutputDebugStringA(szMessage);
  }
#endif

}


/* msDebug2()
**
** Variadic function with no operation
**
*/
void msDebug2( int level, ... )
{
}
