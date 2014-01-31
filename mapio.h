/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Definitions for MapServer IO redirection capability.
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

#ifndef MAPIO_H
#define MAPIO_H

/*
** We deliberately emulate stdio functions in the msIO cover functions.
** This makes it easy for folks to understand the semantics, and means we
** can just #define things to use stdio in the simplest case.
*/

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MS_PRINT_FUNC_FORMAT
#if defined(__GNUC__) && __GNUC__ >= 3 && !defined(DOXYGEN_SKIP)
#define MS_PRINT_FUNC_FORMAT( format_idx, arg_idx )  __attribute__((__format__ (__printf__, format_idx, arg_idx)))
#else
#define MS_PRINT_FUNC_FORMAT( format_idx, arg_idx )
#endif
#endif

  /* stdio analogs */
  int MS_DLL_EXPORT msIO_printf( const char *format, ... ) MS_PRINT_FUNC_FORMAT(1,2);
  int MS_DLL_EXPORT msIO_fprintf( FILE *stream, const char *format, ... ) MS_PRINT_FUNC_FORMAT(2,3);
  int MS_DLL_EXPORT msIO_fwrite( const void *ptr, size_t size, size_t nmemb, FILE *stream );
  int MS_DLL_EXPORT msIO_fread( void *ptr, size_t size, size_t nmemb, FILE *stream );
  int MS_DLL_EXPORT msIO_vfprintf( FILE *fp, const char *format, va_list ap );

#ifdef USE_GD
  gdIOCtx MS_DLL_EXPORT *msIO_getGDIOCtx( FILE *fp );
#endif

  /*
  ** Definitions for the callback function and the details of the IO
  ** channel contexts.
  */

  typedef int (*msIO_llReadWriteFunc)( void *cbData, void *data, int byteCount );

  typedef struct msIOContext_t {
    const char           *label;
    int                   write_channel; /* 1 for stdout/stderr, 0 for stdin */
    msIO_llReadWriteFunc  readWriteFunc;
    void                  *cbData;
  } msIOContext;

  int MS_DLL_EXPORT msIO_installHandlers( msIOContext *stdin_context,
                                          msIOContext *stdout_context,
                                          msIOContext *stderr_context );
  msIOContext MS_DLL_EXPORT *msIO_getHandler( FILE * );
  void msIO_setHeader (const char *header, const char* value, ...) MS_PRINT_FUNC_FORMAT(2,3);
  void msIO_sendHeaders(void);

  /*
  ** These can be used instead of the stdio style functions if you have
  ** msIOContext's for the channel in question.
  */
  int msIO_contextRead( msIOContext *context, void *data, int byteCount );
  int msIO_contextWrite( msIOContext *context, const void *data, int byteCount );

  /*
  ** For redirecting IO to a memory buffer.
  */

  typedef struct {
    unsigned char *data;
    int            data_len;    /* really buffer length */
    int            data_offset; /* really buffer used */
  } msIOBuffer;

  int MS_DLL_EXPORT msIO_bufferRead( void *, void *, int );
  int MS_DLL_EXPORT msIO_bufferWrite( void *, void *, int );

  void MS_DLL_EXPORT msIO_resetHandlers(void);
  void MS_DLL_EXPORT msIO_installStdoutToBuffer(void);
  void MS_DLL_EXPORT msIO_installStdinFromBuffer(void);
  void MS_DLL_EXPORT msIO_Cleanup(void);
  char MS_DLL_EXPORT *msIO_stripStdoutBufferContentType(void);
  void MS_DLL_EXPORT msIO_stripStdoutBufferContentHeaders(void);
  int MS_DLL_EXPORT msIO_isStdContext(void);

  /* this is just for setting normal stdout's to binary mode on windows */

  int msIO_needBinaryStdout( void );
  int msIO_needBinaryStdin( void );

#ifdef __cplusplus
}
#endif

#endif /* nef MAPIO_H */
