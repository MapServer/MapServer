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

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500 // for putenv
#endif

#include "mapserver-config.h"
#include <stdbool.h>
#include <stdlib.h>

#ifdef USE_FASTCGI
#define NO_FCGI_DEFINES
#include "fcgi_stdio.h"
#endif

#include "mapserv.h"
#include "mapio.h"
#include "maptime.h"

#include "cpl_conv.h"

#ifndef _WIN32
#include <signal.h>
#endif


/************************************************************************/
/*                      FastCGI cleanup functions.                      */
/************************************************************************/

static int finish_process = 0;

#ifndef _WIN32
static void msCleanupOnSignal( int nInData )
{
  (void)nInData;
  finish_process = 1;
}
#endif

#ifdef _WIN32
static void msCleanupOnExit( void )
{
  finish_process = 1;
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
  return (int) FCGI_fread( data, 1, byteCount, (FCGI_FILE *) cbData );
}

/************************************************************************/
/*                           msIO_fcgiWrite()                           */
/*                                                                      */
/*      This is the default implementation via stdio.                   */
/************************************************************************/

static int msIO_fcgiWrite( void *cbData, void *data, int byteCount )

{
  return (int) FCGI_fwrite( data, 1, byteCount, (FCGI_FILE *) cbData );
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
  int sendheaders = MS_TRUE;
  struct mstimeval execstarttime, execendtime;
  struct mstimeval requeststarttime, requestendtime;
  mapservObj  *mapserv = NULL;
  configObj *config = NULL;

  /* 
  ** Process -v and -h command line arguments  first end exit. We want to avoid any error messages 
  ** associated with msLoadConfig() or msSetup().
  */
  const char* config_filename = NULL;
  const bool use_command_line_options = getenv("QUERY_STRING") == NULL;
  if (use_command_line_options) {
    /* WARNING:
     * Do not parse command line arguments (especially those that could have
     * dangerous consequences if controlled through a web request), without checking
     * that the QUERY_STRING environment variable is *not* set, because in a
     * CGI context, command line arguments can be generated from the content
     * of the QUERY_STRING, and thus cause a security problem.
     * For ex, "http://example.com/mapserv.cgi?-conf+bar
     * would result in "mapserv.cgi -conf bar" being invoked.
     * See https://github.com/MapServer/MapServer/pull/6429#issuecomment-952533589
     * and https://datatracker.ietf.org/doc/html/rfc3875#section-4.4
    */
    for( int iArg = 1; iArg < argc; iArg++ ) {
      if( strcmp(argv[iArg],"-v") == 0 ) {
        printf("%s\n", msGetVersion());
        fflush(stdout);
        exit(0);
      } else if (strcmp(argv[iArg], "-h") == 0 || strcmp(argv[iArg], "--help") == 0) {
        printf("Usage: mapserv [--help] [-v] [-nh] [QUERY_STRING=value] [PATH_INFO=value]\n");
        printf("                [-conf filename]\n");
        printf("\n");
        printf("Options :\n");
        printf("  -h, --help              Display this help message.\n");
        printf("  -v                      Display version and exit.\n");
        printf("  -nh                     Suppress HTTP headers in CGI mode.\n");
        printf("  -conf filename          Filename of the MapServer configuration file.\n");
        printf("  QUERY_STRING=value      Set the QUERY_STRING in GET request mode.\n");
        printf("  PATH_INFO=value         Set the PATH_INFO for an API request.\n");
        fflush(stdout);
        exit(0);
      } else if( iArg < argc-1 && strcmp(argv[iArg], "-conf") == 0) {
        config_filename = argv[iArg+1];
        ++iArg;
      }
    }
  }

  config = msLoadConfig(config_filename); // first thing
  if(config == NULL) {
#ifdef USE_FASTCGI
    msIO_installFastCGIRedirect(); // FastCGI setup for error handling here
    FCGI_Accept();
#endif
    msCGIWriteError(mapserv);
    exit(0);
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize mapserver.  This sets up threads, GD and GEOS as     */
  /*      well as using MS_ERRORFILE and MS_DEBUGLEVEL env vars if set.   */
  /* -------------------------------------------------------------------- */
  if( msSetup() != MS_SUCCESS ) {
#ifdef USE_FASTCGI
    msIO_installFastCGIRedirect(); // FastCGI setup for error handling here
    FCGI_Accept();
#endif
    msCGIWriteError(mapserv);
    msCleanup();
    msFreeConfig(config);
    exit(0);
  }

  if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING)
    msGettimeofday(&execstarttime, NULL);

  /* -------------------------------------------------------------------- */
  /*      Process arguments. In normal use as a cgi-bin there are no      */
  /*      commandline switches, but we provide a few for test/debug       */
  /*      purposes.                                                       */
  /* -------------------------------------------------------------------- */
  if(use_command_line_options) {
    for( int iArg = 1; iArg < argc; iArg++ ) {
      if(strcmp(argv[iArg], "-nh") == 0) {
        sendheaders = MS_FALSE;
        msIO_setHeaderEnabled( MS_FALSE );
      } else if( strncmp(argv[iArg], "QUERY_STRING=", 13) == 0 ) {
        /* Debugging hook... pass "QUERY_STRING=..." on the command-line */
        putenv( "REQUEST_METHOD=GET" );
        /* coverity[tainted_string] */
        putenv( argv[iArg] );
      } else if( strncmp(argv[iArg], "PATH_INFO=", 10) == 0 ) {
        /* Debugging hook for APIs... pass "PATH_INFO=..." on the command-line */
        putenv( "REQUEST_METHOD=GET" );
        /* coverity[tainted_string] */
        putenv( argv[iArg] );
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Setup cleanup magic, mainly for FastCGI case.                   */
  /* -------------------------------------------------------------------- */
#ifndef _WIN32
  signal( SIGUSR1, msCleanupOnSignal );
  signal( SIGTERM, msCleanupOnSignal );
#endif

#ifdef USE_FASTCGI
  msIO_installFastCGIRedirect();

  /* In FastCGI case we loop accepting multiple requests.  In normal CGI */
  /* use we only accept and process one request.  */
  while( !finish_process && FCGI_Accept() >= 0 ) {
#endif /* def USE_FASTCGI */

    /* -------------------------------------------------------------------- */
    /*      Process a request.                                              */
    /* -------------------------------------------------------------------- */
    mapserv = msAllocMapServObj();
    mapserv->sendheaders = sendheaders; /* override the default if necessary (via command line -nh switch) */

    mapserv->request->NumParams = loadParams(mapserv->request, NULL, NULL, 0, NULL);
    if(msCGIIsAPIRequest(mapserv) == MS_FALSE && mapserv->request->NumParams == -1) { /* no QUERY_STRING or PATH_INFO */
      msCGIWriteError(mapserv);
      goto end_request;
    }

    mapserv->map = msCGILoadMap(mapserv, config);
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

    if(mapserv->request->api_path != NULL && mapserv->request->api_path_length > 1) {
      // API requests require a map key and a path
      if(msCGIDispatchAPIRequest(mapserv) != MS_SUCCESS) {
	msCGIWriteError(mapserv);
	goto end_request;
      }
    } else if(msCGIDispatchRequest(mapserv) != MS_SUCCESS) {
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
  msCleanup();
  msFreeConfig(config);

#ifdef _WIN32
  /* flush pending writes to stdout */
  fflush(stdout);
#endif

  exit( 0 );
}
