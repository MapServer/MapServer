/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Implementations for MapServer IO redirection capability.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.7  2005/07/22 17:26:11  frank
 * bug 1259: fixed POST support in fastcgi mode
 *
 * Revision 1.6  2004/11/04 21:13:02  frank
 * ensure we include io.h on win32
 *
 * Revision 1.5  2004/11/04 21:06:09  frank
 * centralize 'stdout binary mode setting' for win32, add for gdal output
 *
 * Revision 1.4  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.3  2004/10/08 19:11:39  frank
 * ensure msIO_vfprintf is undefed
 *
 * Revision 1.2  2004/10/05 04:23:10  sdlime
 * Added include for stdarg.h to mapio.c.
 *
 * Revision 1.1  2004/10/01 19:03:35  frank
 * New
 *
 */

#include <stdarg.h>

#include "map.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

MS_CVSID("$Id$")

static int is_msIO_initialized = MS_FALSE;

static msIOContext default_stdin_context;
static msIOContext default_stdout_context;
static msIOContext default_stderr_context;

static msIOContext current_stdin_context;
static msIOContext current_stdout_context;
static msIOContext current_stderr_context;

static void msIO_Initialize( void );

#ifdef msIO_printf
#  undef msIO_printf
#  undef msIO_fprintf
#  undef msIO_fwrite
#  undef msIO_vfprintf
#endif

/************************************************************************/
/*                        msIO_installHandlers()                        */
/************************************************************************/

int msIO_installHandlers( msIOContext *stdin_context,
                          msIOContext *stdout_context,
                          msIOContext *stderr_context )

{
    msIO_Initialize();

    if( stdin_context == NULL )
        current_stdin_context = default_stdin_context;
    else
        current_stdin_context = *stdin_context;
    
    if( stdout_context == NULL )
        current_stdout_context = default_stdout_context;
    else
        current_stdout_context = *stdout_context;
    
    if( stderr_context == NULL )
        current_stderr_context = default_stderr_context;
    else
        current_stderr_context = *stderr_context;

    return MS_TRUE;
}

/************************************************************************/
/*                          msIO_getHandler()                           */
/************************************************************************/

msIOContext *msIO_getHandler( FILE * fp )

{
    msIO_Initialize();

    if( fp == stdin )
        return &current_stdin_context;
    else if( fp == stdout )
        return &current_stdout_context;
    else if( fp == stderr )
        return &current_stderr_context;
    else
        return NULL;
}

/************************************************************************/
/*                          msIO_contextRead()                          */
/************************************************************************/

int msIO_contextRead( msIOContext *context, void *data, int byteCount )

{
    if( context->write_channel == MS_TRUE )
        return 0;
    else
        return context->readWriteFunc( context->cbData, data, byteCount );
}

/************************************************************************/
/*                         msIO_contextWrite()                          */
/************************************************************************/

int msIO_contextWrite( msIOContext *context, const void *data, int byteCount )

{
    if( context->write_channel == MS_FALSE )
        return 0;
    else
        return context->readWriteFunc( context->cbData, (void *) data, 
                                       byteCount );
}

/* ==================================================================== */
/* ==================================================================== */
/*      Stdio-like cover functions.                                     */
/* ==================================================================== */
/* ==================================================================== */

/************************************************************************/
/*                            msIO_printf()                             */
/************************************************************************/

int msIO_printf( const char *format, ... )

{
    va_list args;
    int     return_val;
    msIOContext *context;
    char workBuf[8000];

    va_start( args, format );
#if defined(HAVE_VSNPRINTF)
    return_val = vsnprintf( workBuf, sizeof(workBuf), format, args );
#else
    return_val = vsprintf( workBuf, format, args);
#endif

    va_end( args );

    if( return_val < 0 || return_val >= sizeof(workBuf) )
        return -1;

    context = msIO_getHandler( stdout );
    if( context == NULL )
        return -1;

    return msIO_contextWrite( context, workBuf, return_val );
}

/************************************************************************/
/*                            msIO_fprintf()                            */
/************************************************************************/

int msIO_fprintf( FILE *fp, const char *format, ... )

{
    va_list args;
    int     return_val;
    msIOContext *context;
    char workBuf[8000];

    va_start( args, format );
#if defined(HAVE_VSNPRINTF)
    return_val = vsnprintf( workBuf, sizeof(workBuf), format, args );
#else
    return_val = vsprintf( workBuf, format, args);
#endif

    va_end( args );

    if( return_val < 0 || return_val >= sizeof(workBuf) )
        return -1;

    context = msIO_getHandler( fp );
    if( context == NULL )
        return fwrite( workBuf, 1, return_val, fp );
    else
        return msIO_contextWrite( context, workBuf, return_val );
}

/************************************************************************/
/*                            msIO_vfprintf()                           */
/************************************************************************/

int msIO_vfprintf( FILE *fp, const char *format, va_list ap )

{
    int     return_val;
    msIOContext *context;
    char workBuf[8000];

#if defined(HAVE_VSNPRINTF)
    return_val = vsnprintf( workBuf, sizeof(workBuf), format, ap );
#else
    return_val = vsprintf( workBuf, format, ap );
#endif

    if( return_val < 0 || return_val >= sizeof(workBuf) )
        return -1;

    context = msIO_getHandler( fp );
    if( context == NULL )
        return fwrite( workBuf, 1, return_val, fp );
    else
        return msIO_contextWrite( context, workBuf, return_val );
}

/************************************************************************/
/*                            msIO_fwrite()                             */
/************************************************************************/

int msIO_fwrite( const void *data, size_t size, size_t nmemb, FILE *fp )

{
    msIOContext *context;

    context = msIO_getHandler( fp );
    if( context == NULL )
        return fwrite( data, size, nmemb, fp );
    else
        return msIO_contextWrite( context, data, size * nmemb ) / size;
}

/************************************************************************/
/*                            msIO_fread()                              */
/************************************************************************/

int msIO_fread( void *data, size_t size, size_t nmemb, FILE *fp )

{
    msIOContext *context;

    context = msIO_getHandler( fp );
    if( context == NULL )
        return fread( data, size, nmemb, fp );
    else
        return msIO_contextRead( context, data, size * nmemb ) / size;
}

/* ==================================================================== */
/* ==================================================================== */
/*      Internal default callbacks implementing stdio reading and       */
/*      writing.                                                        */
/* ==================================================================== */
/* ==================================================================== */

/************************************************************************/
/*                           msIO_stdioRead()                           */
/*                                                                      */
/*      This is the default implementation via stdio.                   */
/************************************************************************/

static int msIO_stdioRead( void *cbData, void *data, int byteCount )

{
    return fread( data, 1, byteCount, (FILE *) cbData );
}

/************************************************************************/
/*                          msIO_stdioWrite()                           */
/*                                                                      */
/*      This is the default implementation via stdio.                   */
/************************************************************************/

static int msIO_stdioWrite( void *cbData, void *data, int byteCount )

{
    return fwrite( data, 1, byteCount, (FILE *) cbData );
}

/************************************************************************/
/*                          msIO_Initialize()                           */
/************************************************************************/

static void msIO_Initialize( void )

{
    if( is_msIO_initialized == MS_TRUE )
        return;

    default_stdin_context.write_channel = MS_FALSE;
    default_stdin_context.readWriteFunc = msIO_stdioRead;
    default_stdin_context.cbData = (void *) stdin;

    default_stdout_context.write_channel = MS_TRUE;
    default_stdout_context.readWriteFunc = msIO_stdioWrite;
    default_stdout_context.cbData = (void *) stdout;

    default_stderr_context.write_channel = MS_TRUE;
    default_stderr_context.readWriteFunc = msIO_stdioWrite;
    default_stderr_context.cbData = (void *) stderr;

    current_stdin_context = default_stdin_context;
    current_stdout_context = default_stdout_context;
    current_stderr_context = default_stderr_context;

    is_msIO_initialized = MS_TRUE;
}

/* ==================================================================== */
/* ==================================================================== */
/*      Stuff related to having a gdIOCtx operate through the msIO      */
/*      layer.                                                          */
/* ==================================================================== */
/* ==================================================================== */

typedef struct {
    gdIOCtx        gd_io_ctx;
    msIOContext    *ms_io_ctx;
} msIO_gdIOCtx;

/************************************************************************/
/*                            msIO_gd_putC()                            */
/*                                                                      */
/*      A GD IO context callback to put a character, redirected         */
/*      through the msIO output context.                                */
/************************************************************************/

static void msIO_gd_putC( gdIOCtx *cbData, int out_char )

{
    msIO_gdIOCtx *merged_context = (msIO_gdIOCtx *) cbData;
    char out_char_as_char = (char) out_char;

    msIO_contextWrite( merged_context->ms_io_ctx, &out_char_as_char, 1 );
}

/************************************************************************/
/*                           msIO_gd_putBuf()                           */
/*                                                                      */
/*      The GD IO context callback to put a buffer of data,             */
/*      redirected through the msIO output context.                     */
/************************************************************************/

static int msIO_gd_putBuf( gdIOCtx *cbData, const void *data, int byteCount )

{
    msIO_gdIOCtx *merged_context = (msIO_gdIOCtx *) cbData;

    return msIO_contextWrite( merged_context->ms_io_ctx, data, byteCount );
}

/************************************************************************/
/*                          msIO_getGDIOCtx()                           */
/*                                                                      */
/*      The returned context should be freed with free() when no        */
/*      longer needed.                                                  */
/************************************************************************/

gdIOCtx *msIO_getGDIOCtx( FILE *fp )

{
    msIO_gdIOCtx *merged_context;
    msIOContext *context = msIO_getHandler( fp );

    if( context == NULL )
        return NULL;

    merged_context = (msIO_gdIOCtx *) calloc(1,sizeof(msIO_gdIOCtx));
    merged_context->gd_io_ctx.putC = msIO_gd_putC;
    merged_context->gd_io_ctx.putBuf = msIO_gd_putBuf;
    merged_context->ms_io_ctx = context;

    return (gdIOCtx *) merged_context;
}

/* ==================================================================== */
/* ==================================================================== */
/*      FastCGI output redirection functions.                           */
/* ==================================================================== */
/* ==================================================================== */

#ifdef USE_FASTCGI

#define NO_FCGI_DEFINES
#include "fcgi_stdio.h"

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

int msIO_installFastCGIRedirect()

{
    msIOContext stdin_ctx, stdout_ctx, stderr_ctx;

    stdin_ctx.write_channel = MS_FALSE;
    stdin_ctx.readWriteFunc = msIO_fcgiRead;
    stdin_ctx.cbData = (void *) FCGI_stdin;

    stdout_ctx.write_channel = MS_TRUE;
    stdout_ctx.readWriteFunc = msIO_fcgiWrite;
    stdout_ctx.cbData = (void *) FCGI_stdout;

    stderr_ctx.write_channel = MS_TRUE;
    stderr_ctx.readWriteFunc = msIO_fcgiWrite;
    stderr_ctx.cbData = (void *) FCGI_stderr;

    msIO_installHandlers( &stdin_ctx, &stdout_ctx, &stderr_ctx );

    return MS_TRUE;
}

#endif

/************************************************************************/
/*                       msIO_needBinaryStdout()                        */
/*                                                                      */
/*      This function is intended to ensure that stdout is in binary    */
/*      mode.                                                           */
/*                                                                      */
/*      But don't do it we are using FastCGI.  We will take care of     */
/*      doing it in the libfcgi library in that case for the normal     */
/*      cgi case, and for the fastcgi case the _setmode() call          */
/*      causes a crash.                                                 */
/************************************************************************/

int msIO_needBinaryStdout()

{
#if defined(_WIN32) && !defined(USE_FASTCGI)
    if(_setmode( _fileno(stdout), _O_BINARY) == -1) 
    {
      msSetError(MS_IOERR, 
                 "Unable to change stdout to binary mode.", 
                 "msIO_needBinaryStdout()" );
      return(MS_FAILURE);
    }
#endif
    
    return MS_SUCCESS;
}

