/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implement new style connection pooling.
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
 ******************************************************************************

             New MapServer Connection Pooling
             ================================

This attempts to describe how the new connection pooling support (introduced
Sept/2004) works and what the maintainer of a connection type needs to do
to take advantage of it.

First, the new connection pooling support makes the assumption that
a connection can be abstracted as a void pointer.  Further it assumes that
the connection identify is unique defined by the connection string and
the connection type on layers.  So if two layers have the same connection
type and connection string that they can share a connection handle.

The old connection sharing was based on searching previous layers in the
same map for a layer to copy an existing connection from.  That was ok for
sharing connection between layers in the same map, but didn't address the
need to share connections over a longer term as is the case in a FastCGI
situation where the cgi is long running and handles many CGI requests.

The mapool.c code essentially maintains a cache of open connections with the
following information for each:
  - connectiontype / connection
  - connection handle
  - reference count
  - life span indicator
  - callback function to close the connection.

The life span indicator is controlled by the CLOSE_CONNECTION PROCESSING
option on the layer(s).  If this is set to NORMAL (or not set at all) the
connection will be closed as soon as the reference count drops to zero.  This
is MS_LIFE_ZEROREF, and in normal use results in connections only be shared
between layers in a given map as the connection will be closed when no layers
are open against it anymore.  The other possible value for the CLOSE_CONNECTION
option is DEFER which basically results in the connection being kept open
till the application closes (msCleanup() will ensure the connection is closed
on exit if called).  This case is called MS_LIFE_FOREVER.
The CLOSE_CONNECTION=ALWAYS setting provides to suppress the connection pooling
for a particular layer. In this case the MS_LIFE_SINGLE setting is used, which
ensures that a new connection is created for each request and it is always
closed when the connection is released. This kind of connection cannot be
reused.

The callback is a function provided with the connection handle when it is
registered.  It takes a single "void *" argument which is the connection
handle.

Updating a Driver
-----------------

The following are the steps to ensure a particular format/database supports
connection pooling (ie. mappostgis.c, maporacle.c, mapogr.cpp, etc)
We will use POSTGIS names for convenience.

1) in msPOSTGISLayerOpen() call msConnPoolRequest(layer) in order to get
   a database connection.

        layerinfo->conn = (PGconn *) msConnPoolRequest( layer );

2) In msPOSTGISLayerOpen(): if msConnPoolRequest() returned NULL then
   manually open a connection to the database (ie. PQconnectcb()) and then
   register this handle with the pool API by calling msmsConnPoolRegister().

      layerinfo->conn = PQconnectdb( layer->connection );

            if (PQstatus(layerinfo->conn) == CONNECTION_BAD)
               <report error>
            else
                msConnPoolRegister( layer, layerinfo->conn,
                                    msPOSTGISCloseConnection );

3) Implement a callback function (msPOSTGISCloseConnection) that the
   connection pooling API can call when it wants to close the connection.

   static void msPOSTGISCloseConnection( void *conn_handle )

   {
       PQfinish( (PGconn*) conn_handle );
   }

4) In msPOSTGISLayerClose() release the connection handle instead of
   directly closing it.

            msConnPoolRelease( layer, layerinfo->conn );
            layerinfo->conn = NULL;

5) If there was any use of msCheckConnection() or the "sameconnection"
   member of the layerObj, then old style connection pooling is already
   present.  Remove it.

Thats it!

Other Notes
-----------

o The connection pooling API will report details about connection
  registrations, requests, releases and closes if the layer debug flag is
  on for the layers in question.

o The connection pooling API will let a connection be used/referenced multiple
  times from a single thread, but will not allow a connection to be shared
  between different threads concurrently.  But if a connection is released
  by one thread, it is available for use by another thread.

 ****************************************************************************/

#include "mapserver.h"
#include "mapthread.h"

/* defines for lifetime.
   A positive number is a time-from-last use in seconds */

#define MS_LIFE_FOREVER -1
#define MS_LIFE_ZEROREF -2
#define MS_LIFE_SINGLE -3

typedef struct {
  enum MS_CONNECTION_TYPE connectiontype;
  char *connection;

  int lifespan;
  int ref_count;
  void *thread_id;
  int debug;

  time_t last_used;

  void *conn_handle;

  void (*close)(void *);
} connectionObj;

/*
** These static structures are protected by the TLOCK_POOL mutex.
*/

static int connectionCount = 0;
static int connectionMax = 0;
static connectionObj *connections = NULL;

/************************************************************************/
/*                         msConnPoolRegister()                         */
/*                                                                      */
/*      Register a new connection with the connection pool tracker.     */
/************************************************************************/

void msConnPoolRegister(layerObj *layer, void *conn_handle,
                        void (*close_func)(void *))

{
  const char *close_connection = NULL;
  connectionObj *conn = NULL;

  if (layer->debug)
    msDebug("msConnPoolRegister(%s,%s,%p)\n", layer->name, layer->connection,
            conn_handle);

  /* -------------------------------------------------------------------- */
  /*      We can't meaningful keep a connection with no connection or     */
  /*      connection type string on the layer.                            */
  /* -------------------------------------------------------------------- */
  if (layer->connection == NULL) {
    if (layer->tileindex != NULL && layer->connectiontype == MS_OGR) {
      /* this is ok, no need to make a fuss */
    } else {
      msDebug("%s: Missing CONNECTION on layer %s.\n", "msConnPoolRegister()",
              layer->name);

      msSetError(MS_MISCERR, "Missing CONNECTION on layer %s.",
                 "msConnPoolRegister()", layer->name);
    }
    return;
  }

  /* -------------------------------------------------------------------- */
  /*      Grow the array of connection information objects if needed.     */
  /* -------------------------------------------------------------------- */
  msAcquireLock(TLOCK_POOL);

  if (connectionCount == connectionMax) {
    connectionMax += 10;
    connectionObj *newConnections = (connectionObj *)realloc(
        connections, sizeof(connectionObj) * connectionMax);
    if (newConnections == NULL) {
      msSetError(MS_MEMERR, NULL, "msConnPoolRegister()");
      msReleaseLock(TLOCK_POOL);
      return;
    }
    connections = newConnections;
  }

  /* -------------------------------------------------------------------- */
  /*      Set the new connection information.                             */
  /* -------------------------------------------------------------------- */
  conn = connections + connectionCount;

  connectionCount++;

  conn->connectiontype = layer->connectiontype;
  conn->connection = msStrdup(layer->connection);
  conn->close = close_func;
  conn->ref_count = 1;
  conn->thread_id = msGetThreadId();
  conn->last_used = time(NULL);
  conn->conn_handle = conn_handle;
  conn->debug = layer->debug;

  /* -------------------------------------------------------------------- */
  /*      Categorize the connection handling information.                 */
  /* -------------------------------------------------------------------- */
  close_connection = msLayerGetProcessingKey(layer, "CLOSE_CONNECTION");

  if (close_connection == NULL)
    close_connection = "NORMAL";

  if (strcasecmp(close_connection, "NORMAL") == 0)
    conn->lifespan = MS_LIFE_ZEROREF;
  else if (strcasecmp(close_connection, "DEFER") == 0)
    conn->lifespan = MS_LIFE_FOREVER;
  else if (strcasecmp(close_connection, "ALWAYS") == 0)
    conn->lifespan = MS_LIFE_SINGLE;
  else {
    msDebug("msConnPoolRegister(): "
            "Unrecognised CLOSE_CONNECTION value '%s'\n",
            close_connection);

    msSetError(MS_MISCERR, "Unrecognised CLOSE_CONNECTION value '%s'",
               "msConnPoolRegister()", close_connection);
    conn->lifespan = MS_LIFE_ZEROREF;
  }

  msReleaseLock(TLOCK_POOL);
}

/************************************************************************/
/*                          msConnPoolClose()                           */
/*                                                                      */
/*      Close the indicated connection.  The index in the connection    */
/*      table is passed.  Remove the connection from the table as       */
/*      well.                                                           */
/************************************************************************/

static void msConnPoolClose(int conn_index)

{
  connectionObj *conn = connections + conn_index;

  if (conn->ref_count > 0) {
    if (conn->debug)
      msDebug("msConnPoolClose(): "
              "Closing connection %s even though ref_count=%d.\n",
              conn->connection, conn->ref_count);

    msSetError(MS_MISCERR, "Closing connection %s even though ref_count=%d.",
               "msConnPoolClose()", conn->connection, conn->ref_count);
  }

  if (conn->debug)
    msDebug("msConnPoolClose(%s,%p)\n", conn->connection, conn->conn_handle);

  if (conn->close != NULL)
    conn->close(conn->conn_handle);

  /* free malloced() stuff in this connection */
  free(conn->connection);

  connectionCount--;
  if (connectionCount == 0) {
    /* if there are no connections left we will "cleanup".  */
    connectionMax = 0;
    free(connections);
    connections = NULL;
  } else {
    /* move the last connection in place of our now closed one */
    memcpy(connections + conn_index, connections + connectionCount,
           sizeof(connectionObj));
  }
}

/************************************************************************/
/*                         msConnPoolRequest()                          */
/*                                                                      */
/*      Ask for a connection from the connection pool for use with      */
/*      the current layer.  If found (CONNECTION and CONNECTIONTYPE     */
/*      match) then return it and up the ref count.  Otherwise          */
/*      return NULL.                                                    */
/************************************************************************/

void *msConnPoolRequest(layerObj *layer)

{
  int i;
  const char *close_connection;

  if (layer->connection == NULL)
    return NULL;

  /* check if we must always create a new connection */
  close_connection = msLayerGetProcessingKey(layer, "CLOSE_CONNECTION");
  if (close_connection && strcasecmp(close_connection, "ALWAYS") == 0)
    return NULL;

  msAcquireLock(TLOCK_POOL);
  for (i = 0; i < connectionCount; i++) {
    connectionObj *conn = connections + i;

    if (layer->connectiontype == conn->connectiontype &&
        strcasecmp(layer->connection, conn->connection) == 0 &&
        (conn->ref_count == 0 || conn->thread_id == msGetThreadId()) &&
        conn->lifespan != MS_LIFE_SINGLE) {
      void *conn_handle = NULL;

      conn->ref_count++;
      conn->thread_id = msGetThreadId();
      conn->last_used = time(NULL);

      if (layer->debug) {
        msDebug("msConnPoolRequest(%s,%s) -> got %p\n", layer->name,
                layer->connection, conn->conn_handle);
        conn->debug = layer->debug;
      }

      conn_handle = conn->conn_handle;

      msReleaseLock(TLOCK_POOL);
      return conn_handle;
    }
  }

  msReleaseLock(TLOCK_POOL);

  return NULL;
}

/************************************************************************/
/*                         msConnPoolRelease()                          */
/*                                                                      */
/*      Release the passed connection for the given layer.              */
/*      Internally the reference count is dropped, and the              */
/*      connection may be closed.  We assume the caller has already     */
/*      acquired the pool lock.                                         */
/************************************************************************/

void msConnPoolRelease(layerObj *layer, void *conn_handle)

{
  int i;

  if (layer->debug)
    msDebug("msConnPoolRelease(%s,%s,%p)\n", layer->name, layer->connection,
            conn_handle);

  if (layer->connection == NULL)
    return;

  msAcquireLock(TLOCK_POOL);
  for (i = 0; i < connectionCount; i++) {
    connectionObj *conn = connections + i;

    if (layer->connectiontype == conn->connectiontype &&
        strcasecmp(layer->connection, conn->connection) == 0 &&
        conn->conn_handle == conn_handle) {
      conn->ref_count--;
      conn->last_used = time(NULL);

      if (conn->ref_count == 0)
        conn->thread_id = 0;

      if (conn->ref_count == 0 && (conn->lifespan == MS_LIFE_ZEROREF ||
                                   conn->lifespan == MS_LIFE_SINGLE))
        msConnPoolClose(i);

      msReleaseLock(TLOCK_POOL);
      return;
    }
  }

  msReleaseLock(TLOCK_POOL);

  msDebug("%s: Unable to find handle for layer '%s'.\n", "msConnPoolRelease()",
          layer->name);

  msSetError(MS_MISCERR, "Unable to find handle for layer '%s'.",
             "msConnPoolRelease()", layer->name);
}

/************************************************************************/
/*                   msConnPoolMapCloseUnreferenced()                   */
/*                                                                      */
/*      Close any unreferenced connections.                             */
/************************************************************************/

void msConnPoolCloseUnreferenced()

{
  int i;

  /* this really needs to be commented out before commiting.  */
  /* msDebug( "msConnPoolCloseUnreferenced()\n" ); */

  msAcquireLock(TLOCK_POOL);
  for (i = connectionCount - 1; i >= 0; i--) {
    connectionObj *conn = connections + i;

    if (conn->ref_count == 0) {
      /* for now we don't assume the locks are re-entrant, so release */
      /* it so msConnPoolClose() can get it.  */
      msConnPoolClose(i);
    }
  }
  msReleaseLock(TLOCK_POOL);
}

/************************************************************************/
/*                       msConnPoolFinalCleanup()                       */
/*                                                                      */
/*      Close any remaining open connections.   This is normally        */
/*      called just before (voluntary) application termination.         */
/************************************************************************/

void msConnPoolFinalCleanup()

{
  /* this really needs to be commented out before commiting.  */
  /* msDebug( "msConnPoolFinalCleanup()\n" ); */

  msAcquireLock(TLOCK_POOL);
  while (connectionCount > 0)
    msConnPoolClose(0);
  msReleaseLock(TLOCK_POOL);
}
