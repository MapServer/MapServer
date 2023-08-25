/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations for Error and Debug functions.
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

#ifndef MAPERROR_H
#define MAPERROR_H

#include "mapthread.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*====================================================================
   *   maperror.c
   *====================================================================*/

#define MS_NOERR 0 /* general error codes */
#define MS_IOERR 1
#define MS_MEMERR 2
#define MS_TYPEERR 3
#define MS_SYMERR 4
#define MS_REGEXERR 5
#define MS_TTFERR 6
#define MS_DBFERR 7
#define MS_IDENTERR 9
#define MS_EOFERR 10
#define MS_PROJERR 11
#define MS_MISCERR 12
#define MS_CGIERR 13
#define MS_WEBERR 14
#define MS_IMGERR 15
#define MS_HASHERR 16
#define MS_JOINERR 17
#define MS_NOTFOUND 18 /* empty search results */
#define MS_SHPERR 19
#define MS_PARSEERR 20
#define MS_UNUSEDERR 21
#define MS_OGRERR 22
#define MS_QUERYERR 23
#define MS_WMSERR 24      /* WMS server error */
#define MS_WMSCONNERR 25  /* WMS connectiontype error */
#define MS_ORACLESPATIALERR 26
#define MS_WFSERR 27      /* WFS server error */
#define MS_WFSCONNERR 28  /* WFS connectiontype error */
#define MS_MAPCONTEXTERR 29 /* Map Context error */
#define MS_HTTPERR 30
#define MS_CHILDERR 31    /* Errors involving arrays of child objects */
#define MS_WCSERR 32
#define MS_GEOSERR 33
#define MS_RECTERR 34
#define MS_TIMEERR 35
#define MS_GMLERR 36
#define MS_SOSERR 37
#define MS_NULLPARENTERR 38
#define MS_AGGERR 39
#define MS_OWSERR 40
#define MS_OGLERR 41
#define MS_RENDERERERR 42
#define MS_V8ERR 43
#define MS_OGCAPIERR 44
#define MS_FGBERR 45
#define MS_NUMERRORCODES 46

#define MESSAGELENGTH 2048
#define ROUTINELENGTH 64

#define MS_ERROR_LANGUAGE "en-US"

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#else
#define  MS_DLL_EXPORT
#endif

#ifndef MS_PRINT_FUNC_FORMAT
#if defined(__GNUC__) && __GNUC__ >= 3 && !defined(DOXYGEN_SKIP)
#define MS_PRINT_FUNC_FORMAT( format_idx, arg_idx )  __attribute__((__format__ (__printf__, format_idx, arg_idx)))
#else
#define MS_PRINT_FUNC_FORMAT( format_idx, arg_idx )
#endif
#endif

/**
This class allows inspection of the MapServer error stack. 
Instances of errorObj are created internally by MapServer as errors happen. 
Errors are managed as a chained list with the first item being the most recent error.
*/
  typedef struct errorObj {
    int code; ///< MapServer error code such as :data:`MS_IMGERR`
    char routine[ROUTINELENGTH]; ///< MapServer function in which the error was set
    char message[MESSAGELENGTH]; ///< Context-dependent error message
    int isreported; ///< :data:`MS_TRUE` or :data:`MS_FALSE` flag indicating if the error has been output
    int errorcount; ///< Number of subsequent errors
#ifndef SWIG
    struct errorObj *next;
    int totalerrorcount;
#endif
  } errorObj;

  /*
  ** Function prototypes
  */

  /**
  Get the MapServer error object
  */
  MS_DLL_EXPORT errorObj *msGetErrorObj(void);
  /**
  Clear the list of error objects
  */
  MS_DLL_EXPORT void msResetErrorList(void);
  /**
  Returns a string containing MapServer version information, and details on what optional components 
  are built in - the same report as produced by ``mapserv -v``
  */
  MS_DLL_EXPORT char *msGetVersion(void);
  /**
  Returns the MapServer version number (x.y.z) as an integer (x*10000 + y*100 + z) 
  e.g. V7.4.2 would return 70402
  */
  MS_DLL_EXPORT int  msGetVersionInt(void);
  /**
  Return a string of all errors
  */
  MS_DLL_EXPORT char *msGetErrorString(const char *delimiter);

#ifndef SWIG
  MS_DLL_EXPORT void msRedactCredentials(char* str);
  MS_DLL_EXPORT void msSetError(int code, const char *message, const char *routine, ...) MS_PRINT_FUNC_FORMAT(2,4) ;
  MS_DLL_EXPORT void msWriteError(FILE *stream);
  MS_DLL_EXPORT void msWriteErrorXML(FILE *stream);
  MS_DLL_EXPORT char *msGetErrorCodeString(int code);
  MS_DLL_EXPORT char *msAddErrorDisplayString(char *source, errorObj *error);

  struct mapObj;
  MS_DLL_EXPORT void msWriteErrorImage(struct mapObj *map, char *filename, int blank);

#endif /* SWIG */

  /*====================================================================
   *   mapdebug.c (See also MS-RFC-28)
   *====================================================================*/

  typedef enum { MS_DEBUGLEVEL_ERRORSONLY = 0,  /* DEBUG OFF, log fatal errors */
                 MS_DEBUGLEVEL_DEBUG      = 1,  /* DEBUG ON */
                 MS_DEBUGLEVEL_TUNING     = 2,  /* Reports timing info */
                 MS_DEBUGLEVEL_V          = 3,  /* Verbose */
                 MS_DEBUGLEVEL_VV         = 4,  /* Very verbose */
                 MS_DEBUGLEVEL_VVV        = 5,  /* Very very verbose */
                 MS_DEBUGLEVEL_DEVDEBUG   = 20, /* Undocumented, will trigger debug messages only useful for developers */
               } debugLevel;

#ifndef SWIG

  typedef enum { MS_DEBUGMODE_OFF,
                 MS_DEBUGMODE_FILE,
                 MS_DEBUGMODE_STDERR,
                 MS_DEBUGMODE_STDOUT,
                 MS_DEBUGMODE_WINDOWSDEBUG
               } debugMode;

  typedef struct debug_info_obj {
    debugLevel  global_debug_level;
    debugMode   debug_mode;
    char        *errorfile;
    FILE        *fp;
    /* The following 2 members are used only with USE_THREAD (but we won't #ifndef them) */
    void* thread_id;
    struct debug_info_obj *next;
  } debugInfoObj;


  MS_DLL_EXPORT void msDebug( const char * pszFormat, ... ) MS_PRINT_FUNC_FORMAT(1,2) ;
  MS_DLL_EXPORT int msSetErrorFile(const char *pszErrorFile, const char *pszRelToPath);
  MS_DLL_EXPORT void msCloseErrorFile( void );
  MS_DLL_EXPORT const char *msGetErrorFile( void );
  MS_DLL_EXPORT void msSetGlobalDebugLevel(int level);
  MS_DLL_EXPORT debugLevel msGetGlobalDebugLevel( void );
  MS_DLL_EXPORT int msDebugInitFromEnv( void );
  MS_DLL_EXPORT void msDebugCleanup( void );

#endif /* SWIG */

#ifdef __cplusplus
}
#endif

#endif /* MAPERROR_H */
