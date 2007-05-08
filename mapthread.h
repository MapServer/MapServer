/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Multithreading / locking related declarations.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.13  2006/08/16 13:27:59  dan
 * Added TLOCK_TMPFILE (bug 1322)
 *
 * Revision 1.12  2006/07/05 05:54:53  frank
 * implement per-thread io contexts
 *
 * Revision 1.11  2005/10/29 02:03:43  jani
 * MS RFC 8: Pluggable External Feature Layer Providers (bug 1477).
 *
 * Revision 1.10  2005/07/15 13:36:46  frank
 * Added comment on lock_names[] in mapthread.c.
 *
 * Revision 1.9  2005/07/14 21:20:44  hobu
 * added TLOCK_OWS definition
 *
 * Revision 1.8  2005/07/07 14:38:00  frank
 * added TLOCK_ORACLE and TLOCK_SDE definitions
 *
 * Revision 1.7  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.6  2005/02/02 17:42:55  frank
 * added TLOCK_POOL
 *
 * Revision 1.5  2005/01/28 06:16:54  sdlime
 * Applied patch to make function prototypes ANSI C compliant. Thanks to Petter Reinholdtsen. This fixes but 1181.
 *
 * Revision 1.4  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef MAPTHREAD_H
#define MAPTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_THREAD
void msThreadInit(void);
int msGetThreadId(void);
void msAcquireLock(int);
void msReleaseLock(int);
#else
#define msThreadInit()
#define msGetThreadId() (0)
#define msAcquireLock(x)
#define msReleaseLock(x)
#endif

/*
** lock ids - note there is a corresponding lock_names[] array in 
** mapthread.c that needs to be extended when new ids are added.
*/

#define TLOCK_PARSER	1
#define TLOCK_GDAL	2
#define TLOCK_ERROROBJ  3
#define TLOCK_PROJ      4
#define TLOCK_TTF       5
#define TLOCK_POOL      6
#define TLOCK_SDE       7
#define TLOCK_ORACLE    8
#define TLOCK_OWS       9
#define TLOCK_LAYER_VTABLE 10
#define TLOCK_IOCONTEXT 11
#define TLOCK_TMPFILE   12

#define TLOCK_STATIC_MAX 20
#define TLOCK_MAX       100

#ifdef __cplusplus
}
#endif

#endif /* MAPTHREAD_H */








