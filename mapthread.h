/******************************************************************************
 * $Id$
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
 ****************************************************************************/

#ifndef MAPTHREAD_H
#define MAPTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_THREAD
  void msThreadInit(void);
  void* msGetThreadId(void);
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

#define TLOCK_PARSER  1
#define TLOCK_GDAL  2
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
#define TLOCK_DEBUGOBJ  13
#define TLOCK_OGR       14
#define TLOCK_TIME      15
#define TLOCK_FRIBIDI   16
#define TLOCK_WxS       17
#define TLOCK_GEOS       18

#define TLOCK_STATIC_MAX 20
#define TLOCK_MAX       100

#ifdef __cplusplus
}
#endif

#endif /* MAPTHREAD_H */








