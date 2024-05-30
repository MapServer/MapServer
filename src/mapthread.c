/******************************************************************************
 * $Id$
 *
 * Project:  UMN MapServer
 * Purpose:  Support code for abstracting thread issues.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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

/******************************************************************************

            THREAD-SAFE SUPPORT IN MAPSERVER
            ================================

If thread safety is enabled the USE_THREAD macro will be defined.

Thread API (mapthread.h/c)
--------------------------

This API is made available to avoid having dependencies on different
thread libraries in lots of places in MapServer.  It is intended to provide
minimal services required by mapserver and isn't intended to be broadly useful.
It should be available for Win32 and pthreads environments.  It should be
possible to implement for other thread libraries if needed.

  int msGetThreadId():
    Returns the current threads integer id.  This can be used for making
        some information thread specific, as has been done for the global
        error context in maperror.c.

  void msAcquireLock(int):
        Acquires the indicated Mutex.  If it is already held by another thread
        then this thread will block till the other thread releases it.  If
        this thread already holds the mutex then behavior is undefined.  If
        the mutex id is not valid (not in the range 0 to TLOCK_STATIC_MAX)
        then results are undefined.

  void msReleaseLock(int):
        Releases the indicated mutex.  If the lock id is invalid, or if the
        mutex is not currently held by this thread then results are undefined.

It is incredibly important to ensure that any mutex that is acquired is
released as soon as possible.  Any flow of control that could result in a
mutex not being release is going to be a disaster.

The mutex numbers are defined in mapthread.h with the TLOCK_* codes.  If you
need a new mutex, add a #define in mapthread.h for it.  Currently there is
no "dynamic" mutex allocation, but this could be added.


Making Things Thread-safe
-------------------------

Generally, to make MapServer thread-safe it is necessary to ensure that
different threads aren't read and updating common datastructures at the same
time and that all appropriate state be kept thread specific.

Generally this will mean:
  o The previously global error status (errorObj ms_error) is now thread
    specific.  Use msGetErrorObj() to get the current threads error state.

  o Use of subcomponents that are not thread safe need to be protected by
    a Mutex (lock).

Currently a mutex is used for the entire map file parsing operation
(msLoadMap() in mapfile.c) since the yacc parser uses a number of global
variables.

It is also done with pj_init() from PROJ.4 since this does not appear to be
thread safe.  It isn't yet clear if pj_transform() is thread safe.

It is expected that mutexes will need to be employed in a variety of other
places to ensure serialized access to risky functionality.  This may apply
to sublibraries like GDAL for instance.

If a new section that is not thread-safe is identified (and assuming it
can't be internally modified to make it thread-safe easily), it is necessary
to define a new mutex (#define a TLOCK code in mapthread.h), and then
surround the resource with acquire and release lock calls.

eg.
    msAcquireLock( TLOCK_PROJ );
    if( !(p->proj = pj_init(p->numargs, p->args)) ) {
        msReleaseLock( TLOCK_PROJ );
        msSetError(MS_PROJERR, pj_strerrno(pj_errno),
                   "msProcessProjection()");
        return(-1);
    }

    msReleaseLock( TLOCK_PROJ );

It is imperative that any acquired locks be released on all possible
control paths or else the MapServer will lock up as other thread try to
acquire the lock and block forever.


Other Thread-safe Issues
------------------------

Some issues are not easily corrected with Mutexes or other similar
mechanisms.  The following restrictions currently apply to MapServer when
trying to use it in thread-safe mode.  Note that failure to adhere to these
constraints will not usually generate nice error messages, instead operation
will just fail sometimes.

1) It is currently assumed that a mapObj belongs only to one thread at a time.
That is, there is no effort to synchronize access to a mapObj itself.

2) Stuff that results in a chdir() call are problematic.  In particular, the
.map file SHAPEPATH directive should not be used.  Use full paths to data
files instead.

******************************************************************************/

#include <assert.h>
#include "mapserver.h"
#include "mapthread.h"

#if defined(USE_THREAD)
static int thread_debug = 0;

static char *const lock_names[] = {
    NULL,           "PARSER",    "GDAL",    "ERROROBJ", "PROJ",
    "TTF",          "POOL",      "SDE",     "ORACLE",   "OWS",
    "LAYER_VTABLE", "IOCONTEXT", "TMPFILE", "DEBUGOBJ", "OGR",
    "TIME",         "FRIBIDI",   "WXS",     "GEOS",     NULL};
#endif

/************************************************************************/
/* ==================================================================== */
/*                               PTHREADS                               */
/* ==================================================================== */
/************************************************************************/

#if defined(USE_THREAD) && !defined(_WIN32)

#include "pthread.h"

static int mutexes_initialized = 0;
static pthread_mutex_t mutex_locks[TLOCK_MAX];

/************************************************************************/
/*                            msThreadInit()                            */
/************************************************************************/

void msThreadInit()

{
  static pthread_mutex_t core_lock = PTHREAD_MUTEX_INITIALIZER;

  if (thread_debug)
    fprintf(stderr, "msThreadInit() (posix)\n");

  pthread_mutex_lock(&core_lock);

  for (; mutexes_initialized < TLOCK_STATIC_MAX; mutexes_initialized++)
    pthread_mutex_init(mutex_locks + mutexes_initialized, NULL);

  pthread_mutex_unlock(&core_lock);
}

/************************************************************************/
/*                           msGetThreadId()                            */
/************************************************************************/

void *msGetThreadId()

{
  return (void *)pthread_self();
}

/************************************************************************/
/*                           msAcquireLock()                            */
/************************************************************************/

void msAcquireLock(int nLockId)

{
  if (mutexes_initialized == 0)
    msThreadInit();

  assert(nLockId >= 0 && nLockId < mutexes_initialized);

  if (thread_debug)
    fprintf(stderr, "msAcquireLock(%d/%s) (posix)\n", nLockId,
            lock_names[nLockId]);

  pthread_mutex_lock(mutex_locks + nLockId);
}

/************************************************************************/
/*                           msReleaseLock()                            */
/************************************************************************/

void msReleaseLock(int nLockId)

{
  assert(mutexes_initialized > 0);
  assert(nLockId >= 0 && nLockId < mutexes_initialized);

  if (thread_debug)
    fprintf(stderr, "msReleaseLock(%d/%s) (posix)\n", nLockId,
            lock_names[nLockId]);

  pthread_mutex_unlock(mutex_locks + nLockId);
}

#endif /* defined(USE_THREAD) && !defined(_WIN32) */

/************************************************************************/
/* ==================================================================== */
/*                          WIN32 THREADS                               */
/* ==================================================================== */
/************************************************************************/

#if defined(USE_THREAD) && defined(_WIN32)

#include <windows.h>

static int mutexes_initialized = 0;
static HANDLE mutex_locks[TLOCK_MAX];

/************************************************************************/
/*                            msThreadInit()                            */
/************************************************************************/

void msThreadInit()

{
  /* static pthread_mutex_t core_lock = PTHREAD_MUTEX_INITIALIZER; */
  static HANDLE core_lock = NULL;

  if (mutexes_initialized >= TLOCK_STATIC_MAX)
    return;

  if (thread_debug)
    fprintf(stderr, "msThreadInit() (win32)\n");

  if (core_lock == NULL)
    core_lock = CreateMutex(NULL, TRUE, NULL);
  else
    WaitForSingleObject(core_lock, INFINITE);

  for (; mutexes_initialized < TLOCK_STATIC_MAX; mutexes_initialized++)
    mutex_locks[mutexes_initialized] = CreateMutex(NULL, FALSE, NULL);

  ReleaseMutex(core_lock);
}

/************************************************************************/
/*                           msGetThreadId()                            */
/************************************************************************/

void *msGetThreadId()

{
  return (void *)(size_t)GetCurrentThreadId();
}

/************************************************************************/
/*                           msAcquireLock()                            */
/************************************************************************/

void msAcquireLock(int nLockId)

{
  if (mutexes_initialized == 0)
    msThreadInit();

  assert(nLockId >= 0 && nLockId < mutexes_initialized);

  if (thread_debug)
    fprintf(stderr, "msAcquireLock(%d/%s) (win32)\n", nLockId,
            lock_names[nLockId]);

  WaitForSingleObject(mutex_locks[nLockId], INFINITE);
}

/************************************************************************/
/*                           msReleaseLock()                            */
/************************************************************************/

void msReleaseLock(int nLockId)

{
  assert(mutexes_initialized > 0);
  assert(nLockId >= 0 && nLockId < mutexes_initialized);

  if (thread_debug)
    fprintf(stderr, "msReleaseLock(%d/%s) (win32)\n", nLockId,
            lock_names[nLockId]);

  ReleaseMutex(mutex_locks[nLockId]);
}

#endif /* defined(USE_THREAD) && defined(_WIN32) */
