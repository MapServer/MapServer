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

#define TLOCK_PARSER	1
#define TLOCK_GDAL	2
#define TLOCK_ERROROBJ  3
#define TLOCK_PROJ      4
#define TLOCK_TTF       5
#define TLOCK_POOL      6

#define TLOCK_STATIC_MAX 20
#define TLOCK_MAX       100

#ifdef __cplusplus
}
#endif

#endif /* MAPTHREAD_H */








