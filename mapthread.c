/******************************************************************************
 * $Id$
 *
 * Project:  UMN MapServer
 * Purpose:  Support code for abstracting thread issues.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam
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
 * Revision 1.1  2002/01/09 05:19:28  frank
 * New
 *
 */

#include <assert.h>
#include "map.h"
#include "mapthread.h"

#if defined(USE_THREAD) && defined(unix)

#include "pthread.h"

static int mutexes_initialized = 0;
static pthread_mutex_t mutex_locks[TLOCK_MAX];

/************************************************************************/
/*                            msThreadInit()                            */
/************************************************************************/

void msThreadInit()

{
    static pthread_mutex_t core_lock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock( &core_lock );

    for( ; mutexes_initialized < TLOCK_STATIC_MAX; mutexes_initialized++ )
        pthread_mutex_init( mutex_locks + mutexes_initialized, NULL );

    pthread_mutex_unlock( &core_lock );
}

/************************************************************************/
/*                           msGetThreadId()                            */
/************************************************************************/

int msGetThreadId()

{
    return (int) pthread_self();
}

/************************************************************************/
/*                           msAcquireLock()                            */
/************************************************************************/

void msAcquireLock( int nLockId )

{
    if( mutexes_initialized == 0 )
        msThreadInit();

    assert( nLockId >= 0 && nLockId < mutexes_initialized );

    pthread_mutex_lock( mutex_locks + nLockId );
}

/************************************************************************/
/*                           msReleaseLock()                            */
/************************************************************************/

void msReleaseLock( int nLockId )

{
    assert( mutexes_initialized > 0 );
    assert( nLockId >= 0 && nLockId < mutexes_initialized );

    pthread_mutex_unlock( mutex_locks + nLockId );
}

#endif /* defined(USE_THREAD) && defined(unix) */
