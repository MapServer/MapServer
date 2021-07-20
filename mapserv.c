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
#include <stdlib.h>

#ifdef USE_FASTCGI
#define NO_FCGI_DEFINES
#include "fcgi_stdio.h"
#endif

#include "mapserv.h"
#include "mapio.h"
#include "maptime.h"

#include "cpl_conv.h"

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
#ifndef NDEBUG
  msIO_fprintf( stderr, "In msCleanupOnSignal.\n" );
#endif
  msCleanup();
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
  msCleanup();
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
  int iArg;
  int sendheaders = MS_TRUE;
  struct mstimeval execstarttime, execendtime;
  struct mstimeval requeststarttime, requestendtime;
  mapservObj  *mapserv = NULL;
  configObj *config = NULL;

  config = msLoadConfig(NULL); // is the right spot to do this?
  if(config == NULL) {
    msCGIWriteError(mapserv);
    exit(0);
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize mapserver.  This sets up threads, GD and GEOS as     */
  /*      well as using MS_ERRORFILE and MS_DEBUGLEVEL env vars if set.   */
  /* -------------------------------------------------------------------- */
  if( msSetup() != MS_SUCCESS ) {
    msCGIWriteError(mapserv);
    msCleanup();
    msFreeConfig(config);
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
     * at compile time. Do *NOT* enable them since they can cause security
     * problems : https://github.com/mapserver/mapserver/issues/3485
     */
    if( strcmp(argv[iArg],"-v") == 0 ) {
      printf("%s\n", msGetVersion());
      fflush(stdout);
      exit(0);
    } else if(strcmp(argv[iArg], "-nh") == 0) {
      sendheaders = MS_FALSE;
      msIO_setHeaderEnabled( MS_FALSE );
    } else if( strncmp(argv[iArg], "QUERY_STRING=", 13) == 0 ) {
      /* Debugging hook... pass "QUERY_STRING=..." on the command-line */
      putenv( "REQUEST_METHOD=GET" );
      /* coverity[tainted_string] */
      putenv( argv[iArg] );
    } else if( strncmp(argv[iArg], "PATH_INFO=", 10) == 0 ) {
      /* Debugging hook for APIs... pass "PATH_INFO=..." on the command-line */
      putenv( argv[iArg] );
    } else if (strcmp(argv[iArg], "--h") == 0 || strcmp(argv[iArg], "--help") == 0) {
      printf("Usage: mapserv [--help] [-v] [-nh] [QUERY_STRING=value]\n");
#ifdef MS_ENABLE_CGI_CL_DEBUG_ARGS
      printf("               [-tmpbase dirname] [-t mapfilename] [MS_ERRORFILE=value] [MS_DEBUGLEVEL=value]\n");
#endif
      printf("\n");
      printf("Options :\n");
      printf("  -h, --help              Display this help message.\n");
      printf("  -v                      Display version and exit.\n");
      printf("  -nh                     Suppress HTTP headers in CGI mode.\n");
      printf("  QUERY_STRING=value      Set the QUERY_STRING in GET request mode.\n");
      printf("  PATH_INFO=value         Set the PATH_INFO for an API request.\n");
#ifdef MS_ENABLE_CGI_CL_DEBUG_ARGS
      printf("  -tmpbase dirname        Define a forced temporary directory.\n");
      printf("  -t mapfilename          Display the tokens of the mapfile after parsing.\n");
      printf("  MS_ERRORFILE=filename   Set error file.\n");
      printf("  MS_DEBUGLEVEL=value     Set debug level.\n");
#endif
      exit(0);
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

    if(mapserv->request->api_path != NULL) {
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
