/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  http requests related functions
 * Author:   MapServer team.
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

#ifndef MAPHTTP_H
#define MAPHTTP_H



#include "mapprimitive.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MS_HTTP_SUCCESS(status)  (status == 200 || status == 242)

  enum MS_HTTP_PROXY_TYPE
  {
    MS_HTTP,
    MS_SOCKS5
  };

  enum MS_HTTP_AUTH_TYPE {
    MS_BASIC,
    MS_DIGEST,
    MS_NTLM,
    MS_ANY,
    MS_ANYSAFE
  };

  typedef struct http_request_info {
    int     nLayerId;
    char    *pszGetUrl;
    char    *pszOutputFile;
    int     nTimeout;
    int     nMaxBytes;
    rectObj bbox;
    int     width;
    int     height;
    int     nStatus;            /* 200=success, value < 0 if request failed */
    char    *pszContentType;    /* Content-Type of the response */
    char    *pszErrBuf;         /* Buffer where curl can write errors */
    char    *pszPostRequest;    /* post request content (NULL for GET) */
    char    *pszPostContentType;/* post request MIME type */
    char    *pszUserAgent;      /* User-Agent, auto-generated if not set */
    char    *pszHTTPCookieData; /* HTTP Cookie data */

    char    *pszProxyAddress;   /* The address (IP or hostname) of proxy svr */
    long     nProxyPort;        /* The proxy's port                          */
    enum MS_HTTP_PROXY_TYPE eProxyType; /* The type of proxy                 */
    enum MS_HTTP_AUTH_TYPE  eProxyAuthType; /* Auth method against proxy     */
    char    *pszProxyUsername;  /* Proxy authentication username             */
    char    *pszProxyPassword;  /* Proxy authentication password             */

    enum MS_HTTP_AUTH_TYPE eHttpAuthType; /* HTTP Authentication type        */
    char    *pszHttpUsername;   /* HTTP Authentication username              */
    char    *pszHttpPassword;   /* HTTP Authentication password              */

    /* For debugging/profiling */
    int         debug;         /* Debug mode?  MS_TRUE/MS_FALSE */

    /* Private members */
    void      * curl_handle;   /* CURLM * handle */
    FILE      * fp;            /* FILE * used during download */

    char      * result_data;   /* output if pszOutputFile is NULL */
    int       result_size;
    int       result_buf_size;

  } httpRequestObj;

#ifdef USE_CURL

  int msHTTPInit(void);
  void msHTTPCleanup(void);

  void msHTTPInitRequestObj(httpRequestObj *pasReqInfo, int numRequests);
  void msHTTPFreeRequestObj(httpRequestObj *pasReqInfo, int numRequests);
  int  msHTTPExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                             int bCheckLocalCache);
  int  msHTTPGetFile(const char *pszGetUrl, const char *pszOutputFile,
                     int *pnHTTPStatus, int nTimeout, int bCheckLocalCache,
                     int bDebug, int nMaxBytes);

  int msHTTPAuthProxySetup(hashTableObj *mapmd, hashTableObj *lyrmd,
                           httpRequestObj *pasReqInfo, int numRequests,
                           mapObj *map, const char* namespaces);
#endif /*USE_CURL*/

#ifdef __cplusplus
}
#endif




#endif /* MAPHTTP_H */
