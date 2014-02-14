/******************************************************************************
 * $id: mapserv.c 9470 2009-10-16 16:09:31Z sdlime $
 *
 * Project:  MapServer
 * Purpose:  MapServer CGI mainline.
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
#include "mapserver-config.h"
#ifdef USE_FASTCGI
#define NO_FCGI_DEFINES
#include "fcgi_stdio.h"
#endif

#include "mapserv.h"
#include "mapio.h"
#include "maptime.h"

#ifndef WIN32
#include <signal.h>
#endif



/************************************************************************/
/*                      FastCGI cleanup functions.                      */
/************************************************************************/
#ifndef WIN32
void msCleanupOnSignal( int nInData )
{
  /* For some reason, the fastcgi message code does not seem to work */
  /* from within the signal handler on Unix.  So we force output through */
  /* normal stdio functions. */
  msIO_installHandlers( NULL, NULL, NULL );
  msIO_fprintf( stderr, "In msCleanupOnSignal.\n" );
  msCleanup(1);
  exit(0);
}
#endif

#ifdef WIN32
void msCleanupOnExit( void )
{
  /* note that stderr and stdout seem to be non-functional in the */
  /* fastcgi/win32 case.  If you really want to check functioning do */
  /* some sort of hack logging like below ... otherwise just trust it! */

#ifdef notdef
  FILE *fp_out = fopen( "D:\\temp\\mapserv.log", "w" );

  fprintf( fp_out, "In msCleanupOnExit\n" );
  fclose( fp_out );
#endif
  msCleanup(1);
}
#endif

#ifdef USE_FASTCGI

/************************************************************************/
/*                           msIO_fcgiRead()                            */
/*                                                                      */
/*      This is the default implementation via stdio.                   */
/************************************************************************/

static int msIO_fcgiRead( void *cbData, void *data, int byteCount )

{
  return FCGI_fread( data, 1, byteCount, (FCGI_FILE *) cbData );
}

/************************************************************************/
/*                           msIO_fcgiWrite()                           */
/*                                                                      */
/*      This is the default implementation via stdio.                   */
/************************************************************************/

static int msIO_fcgiWrite( void *cbData, void *data, int byteCount )

{
  return FCGI_fwrite( data, 1, byteCount, (FCGI_FILE *) cbData );
}

/************************************************************************/
/*                    msIO_installFastCGIRedirect()                     */
/************************************************************************/
static int msIO_installFastCGIRedirect()

{
  msIOContext stdin_ctx, stdout_ctx, stderr_ctx;

  stdin_ctx.label = "fcgi";
  stdin_ctx.write_channel = MS_FALSE;
  stdin_ctx.readWriteFunc = msIO_fcgiRead;
  stdin_ctx.cbData = (void *) FCGI_stdin;

  stdout_ctx.label = "fcgi";
  stdout_ctx.write_channel = MS_TRUE;
  stdout_ctx.readWriteFunc = msIO_fcgiWrite;
  stdout_ctx.cbData = (void *) FCGI_stdout;

  stderr_ctx.label = "fcgi";
  stderr_ctx.write_channel = MS_TRUE;
  stderr_ctx.readWriteFunc = msIO_fcgiWrite;
  stderr_ctx.cbData = (void *) FCGI_stderr;

  msIO_installHandlers( &stdin_ctx, &stdout_ctx, &stderr_ctx );

  return MS_TRUE;
}

#endif
/************************************************************************/
/*                                main()                                */
/************************************************************************/
int main(int argc, char *argv[])
{
  int iArg;
  int sendheaders = MS_TRUE;
  struct mstimeval execstarttime, execendtime;
  struct mstimeval requeststarttime, requestendtime;
  mapservObj* mapserv = NULL;

  /* -------------------------------------------------------------------- */
  /*      Initialize mapserver.  This sets up threads, GD and GEOS as     */
  /*      well as using MS_ERRORFILE and MS_DEBUGLEVEL env vars if set.   */
  /* -------------------------------------------------------------------- */
  if( msSetup() != MS_SUCCESS ) {
    msCGIWriteError(mapserv);
    msCleanup(0);
    exit(0);
  }

  if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING)
    msGettimeofday(&execstarttime, NULL);

  /* -------------------------------------------------------------------- */
  /*      Process arguments.  In normal use as a cgi-bin there are no     */
  /*      commandline switches, but we provide a few for test/debug       */
  /*      purposes, and to query the version info.                        */
  /* -------------------------------------------------------------------- */
  for( iArg = 1; iArg < argc; iArg++ ) {
    /* Keep only "-v", "-nh" and "QUERY_STRING=..." enabled by default.
     * The others will require an explicit -DMS_ENABLE_CGI_CL_DEBUG_ARGS
     * at compile time.
     */
    if( strcmp(argv[iArg],"-v") == 0 ) {
      printf("%s\n", msGetVersion());
      fflush(stdout);
      exit(0);
    } else if(strcmp(argv[iArg], "-nh") == 0) {
      sendheaders = MS_FALSE;
    } else if( strncmp(argv[iArg], "QUERY_STRING=", 13) == 0 ) {
      /* Debugging hook... pass "QUERY_STRING=..." on the command-line */
      putenv( "REQUEST_METHOD=GET" );
      putenv( argv[iArg] );
    }
#ifdef MS_ENABLE_CGI_CL_DEBUG_ARGS
    else if( iArg < argc-1 && strcmp(argv[iArg], "-tmpbase") == 0) {
      msForceTmpFileBase( argv[++iArg] );
    } else if( iArg < argc-1 && strcmp(argv[iArg], "-t") == 0) {
      char **tokens;
      int numtokens=0;

      if((tokens=msTokenizeMap(argv[iArg+1], &numtokens)) != NULL) {
        int i;
        for(i=0; i<numtokens; i++)
          printf("%s\n", tokens[i]);
        msFreeCharArray(tokens, numtokens);
      } else {
        msCGIWriteError(mapserv);
      }

      exit(0);
    } else if( strncmp(argv[iArg], "MS_ERRORFILE=", 13) == 0 ) {
      msSetErrorFile( argv[iArg] + 13, NULL );
    } else if( strncmp(argv[iArg], "MS_DEBUGLEVEL=", 14) == 0) {
      msSetGlobalDebugLevel( atoi(argv[iArg] + 14) );
    }
#endif /* MS_ENABLE_CGI_CL_DEBUG_ARGS */
    else {
      /* we don't produce a usage message as some web servers pass junk arguments */
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Setup cleanup magic, mainly for FastCGI case.                   */
  /* -------------------------------------------------------------------- */
#ifndef WIN32
  signal( SIGUSR1, msCleanupOnSignal );
  signal( SIGTERM, msCleanupOnSignal );
#endif

#ifdef USE_FASTCGI
  msIO_installFastCGIRedirect();

#ifdef WIN32
  atexit( msCleanupOnExit );
#endif

  /* In FastCGI case we loop accepting multiple requests.  In normal CGI */
  /* use we only accept and process one request.  */
  while( FCGI_Accept() >= 0 ) {
#endif /* def USE_FASTCGI */

    /* -------------------------------------------------------------------- */
    /*      Process a request.                                              */
    /* -------------------------------------------------------------------- */
    mapserv = msAllocMapServObj();
    mapserv->sendheaders = sendheaders; /* override the default if necessary (via command line -nh switch) */

    mapserv->request->NumParams = loadParams(mapserv->request, NULL, NULL, 0, NULL);
    if( mapserv->request->NumParams == -1 ) {
      msCGIWriteError(mapserv);
      goto end_request;
    }

    mapserv->map = msCGILoadMap(mapserv);
    if(!mapserv->map) {
      msCGIWriteError(mapserv);
      goto end_request;
    }

    if( mapserv->map->debug >= MS_DEBUGLEVEL_TUNING)
      msGettimeofday(&requeststarttime, NULL);

#ifdef USE_FASTCGI
    if( mapserv->map->debug ) {
      static int nRequestCounter = 1;

      msDebug( "CGI Request %d on process %d\n", nRequestCounter, getpid() );
      nRequestCounter++;
    }
#endif




    if(msCGIDispatchRequest(mapserv) != MS_SUCCESS) {
      msCGIWriteError(mapserv);
      goto end_request;
    }


end_request:
    if(mapserv->map && mapserv->map->debug >= MS_DEBUGLEVEL_TUNING) {
      msGettimeofday(&requestendtime, NULL);
      msDebug("mapserv request processing time (msLoadMap not incl.): %.3fs\n",
              (requestendtime.tv_sec+requestendtime.tv_usec/1.0e6)-
              (requeststarttime.tv_sec+requeststarttime.tv_usec/1.0e6) );
    }
    msCGIWriteLog(mapserv,MS_FALSE);
    msFreeMapServObj(mapserv);
#ifdef USE_FASTCGI
    /* FCGI_ --- return to top of loop */
    msResetErrorList();
    continue;
  } /* end fastcgi loop */
#endif

  /* normal case, processing is complete */
  if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&execendtime, NULL);
    msDebug("mapserv total execution time: %.3fs\n",
            (execendtime.tv_sec+execendtime.tv_usec/1.0e6)-
            (execstarttime.tv_sec+execstarttime.tv_usec/1.0e6) );
  }
  msCleanup(0);

#ifdef _WIN32
  /* flush pending writes to stdout */
  fflush(stdout);
#endif

  exit( 0 );
}
