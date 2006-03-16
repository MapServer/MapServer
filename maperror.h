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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.35  2006/03/16 22:28:38  tamas
 * Fixed msGetErrorString so as not to truncate the length of the error messages
 * Added msAddErrorDisplayString to read the displayable messages separatedly
 *
 * Revision 1.34  2006/03/14 03:17:19  assefa
 * Add SOS error code (Bug 1710).
 * Correct error codes numbers for MS_TIMEERR and MS_GMLERR.
 *
 * Revision 1.33  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.32  2005/01/07 18:51:09  sdlime
 * Added MS_GMLERR code.
 *
 * Revision 1.31  2004/10/21 04:30:56  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef MAPERROR_H
#define MAPERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#define MS_NOERR 0 /* general error codes */
#define MS_IOERR 1 
#define MS_MEMERR 2
#define MS_TYPEERR 3
#define MS_SYMERR 4
#define MS_REGEXERR 5
#define MS_TTFERR 6
#define MS_DBFERR 7
#define MS_GDERR 8
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
#define MS_SDEERR 21
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
#define MS_NUMERRORCODES 38

#define MESSAGELENGTH 2048
#define ROUTINELENGTH 64

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#else
#define  MS_DLL_EXPORT
#endif

typedef struct error_obj {
  int code;
  char routine[ROUTINELENGTH];
  char message[MESSAGELENGTH];
#ifndef SWIG
  struct error_obj *next;
#endif
} errorObj;

/*
** Global variables
*/
/* extern errorObj ms_error; */

/*
** Function prototypes
*/
MS_DLL_EXPORT errorObj *msGetErrorObj(void);
MS_DLL_EXPORT void msResetErrorList(void);
MS_DLL_EXPORT char *msGetVersion(void);
MS_DLL_EXPORT char *msGetErrorString(char *delimiter);

#ifndef SWIG
MS_DLL_EXPORT void msSetError(int code, const char *message, const char *routine, ...);
MS_DLL_EXPORT void msWriteError(FILE *stream);
MS_DLL_EXPORT void msWriteErrorXML(FILE *stream);
MS_DLL_EXPORT char *msGetErrorCodeString(int code);
MS_DLL_EXPORT char *msAddErrorDisplayString(char *source, errorObj *error);

struct map_obj;
MS_DLL_EXPORT void msWriteErrorImage(struct map_obj *map, char *filename, int blank);

MS_DLL_EXPORT void msDebug( const char * pszFormat, ... );
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPERROR_H */
