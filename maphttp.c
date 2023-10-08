/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Utility functions to access files via HTTP (requires libcurl)
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2001-2003, Daniel Morissette, DM Solutions Group Inc
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ****************************************************************************/

#define NEED_IGNORE_RET_VAL

/* For now this code is enabled only when WMS/WFS client is enabled.
 * This should be changed to a test on the presence of libcurl which
 * is really what the real dependency is.
 */
#include "mapserver-config.h"
#if defined(USE_CURL)

#include "mapserver.h"
#include "maphttp.h"
#include "maperror.h"
#include "mapthread.h"
#include "mapows.h"

#include "cpl_conv.h"

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>

#endif

/*
 * Note: This code uses libcurl to access remote files via the HTTP protocol.
 * Requires libcurl v 7.10 or more recent.
 * See http://curl.haxx.se/libcurl/c/ for the lib source code and docs.
 */
#include <curl/curl.h>

#define unchecked_curl_easy_setopt(handle, opt, param)                         \
  IGNORE_RET_VAL(curl_easy_setopt(handle, opt, param))

/**********************************************************************
 *                          msHTTPInit()
 *
 * This function is called to init libcurl before the first HTTP request
 * in this process is executed.
 * On further calls (when gbCurlInitialized = MS_TRUE) it simply doest nothing.
 *
 * Returns MS_SUCCESS/MS_FAILURE.
 *
 * msHTTPCleanup() will have to be called in msCleanup() when this process
 * exits.
 **********************************************************************/
static int gbCurlInitialized = MS_FALSE;

int msHTTPInit() {
  /* curl_global_init() should only be called once (no matter how
   * many threads or libcurl sessions that'll be used) by every
   * application that uses libcurl.
   */

  msAcquireLock(TLOCK_OWS);
  if (!gbCurlInitialized && curl_global_init(CURL_GLOBAL_ALL) != 0) {
    msReleaseLock(TLOCK_OWS);
    msSetError(MS_HTTPERR, "Libcurl initialization failed.", "msHTTPInit()");
    return MS_FAILURE;
  }

  gbCurlInitialized = MS_TRUE;

  msReleaseLock(TLOCK_OWS);
  return MS_SUCCESS;
}

/**********************************************************************
 *                          msHTTPCleanup()
 *
 **********************************************************************/
void msHTTPCleanup() {
  msAcquireLock(TLOCK_OWS);
  if (gbCurlInitialized)
    curl_global_cleanup();

  gbCurlInitialized = MS_FALSE;
  msReleaseLock(TLOCK_OWS);
}

/**********************************************************************
 *                          msHTTPInitRequestObj()
 *
 * Should be called on a new array of httpRequestObj to initialize them
 * for use with msHTTPExecuteRequest(), etc.
 *
 **********************************************************************/
void msHTTPInitRequestObj(httpRequestObj *pasReqInfo, int numRequests) {
  int i;

  for (i = 0; i < numRequests; i++) {
    pasReqInfo[i].pszGetUrl = NULL;
    pasReqInfo[i].pszPostRequest = NULL;
    pasReqInfo[i].pszPostContentType = NULL;
    pasReqInfo[i].pszOutputFile = NULL;
    pasReqInfo[i].nLayerId = 0;
    pasReqInfo[i].nTimeout = 0;
    pasReqInfo[i].nMaxBytes = 0;
    pasReqInfo[i].nStatus = 0;
    pasReqInfo[i].pszContentType = NULL;
    pasReqInfo[i].pszErrBuf = NULL;
    pasReqInfo[i].pszUserAgent = NULL;
    pasReqInfo[i].pszHTTPCookieData = NULL;
    pasReqInfo[i].pszProxyAddress = NULL;
    pasReqInfo[i].pszProxyUsername = NULL;
    pasReqInfo[i].pszProxyPassword = NULL;
    pasReqInfo[i].pszHttpUsername = NULL;
    pasReqInfo[i].pszHttpPassword = NULL;

    pasReqInfo[i].debug = MS_FALSE;

    pasReqInfo[i].curl_handle = NULL;
    pasReqInfo[i].fp = NULL;
    pasReqInfo[i].result_data = NULL;
    pasReqInfo[i].result_size = 0;
    pasReqInfo[i].result_buf_size = 0;
  }
}

/**********************************************************************
 *                          msHTTPFreeRequestObj()
 *
 **********************************************************************/
void msHTTPFreeRequestObj(httpRequestObj *pasReqInfo, int numRequests) {
  int i;
  for (i = 0; i < numRequests; i++) {
    if (pasReqInfo[i].pszGetUrl)
      free(pasReqInfo[i].pszGetUrl);
    pasReqInfo[i].pszGetUrl = NULL;

    if (pasReqInfo[i].pszPostRequest)
      free(pasReqInfo[i].pszPostRequest);
    pasReqInfo[i].pszPostRequest = NULL;

    if (pasReqInfo[i].pszPostContentType)
      free(pasReqInfo[i].pszPostContentType);
    pasReqInfo[i].pszPostContentType = NULL;

    if (pasReqInfo[i].pszOutputFile)
      free(pasReqInfo[i].pszOutputFile);
    pasReqInfo[i].pszOutputFile = NULL;

    if (pasReqInfo[i].pszContentType)
      free(pasReqInfo[i].pszContentType);
    pasReqInfo[i].pszContentType = NULL;

    if (pasReqInfo[i].pszErrBuf)
      free(pasReqInfo[i].pszErrBuf);
    pasReqInfo[i].pszErrBuf = NULL;

    if (pasReqInfo[i].pszUserAgent)
      free(pasReqInfo[i].pszUserAgent);
    pasReqInfo[i].pszUserAgent = NULL;

    if (pasReqInfo[i].pszHTTPCookieData)
      free(pasReqInfo[i].pszHTTPCookieData);
    pasReqInfo[i].pszHTTPCookieData = NULL;

    pasReqInfo[i].curl_handle = NULL;

    free(pasReqInfo[i].result_data);
    pasReqInfo[i].result_data = NULL;
    pasReqInfo[i].result_size = 0;
    pasReqInfo[i].result_buf_size = 0;
  }
}

/**********************************************************************
 *                          msHTTPWriteFct()
 *
 * CURL_OPTWRITEFUNCTION, called to write blocks of data to the file we
 * are downloading.  Should return the number of bytes that were taken
 * care of.  If that  amount  differs  from  the  amount passed to it
 * it'll signal an error to the library and it will abort the transfer
 * and produce a CURLE_WRITE_ERROR.
 *
 **********************************************************************/
static size_t msHTTPWriteFct(void *buffer, size_t size, size_t nmemb,
                             void *reqInfo) {
  httpRequestObj *psReq;

  psReq = (httpRequestObj *)reqInfo;

  if (psReq->debug) {
    msDebug("msHTTPWriteFct(id=%d, %d bytes)\n", psReq->nLayerId,
            (int)(size * nmemb));
  }

  if (psReq->nMaxBytes > 0 &&
      (psReq->result_size + size * nmemb) > (size_t)psReq->nMaxBytes) {
    msSetError(MS_HTTPERR,
               "Requested transfer larger than configured maximum %d.",
               "msHTTPWriteFct()", psReq->nMaxBytes);
    return -1;
  }
  /* Case where we are writing to a disk file. */
  if (psReq->fp != NULL) {
    psReq->result_size += size * nmemb;
    return fwrite(buffer, size, nmemb, psReq->fp);
  }

  /* Case where we build up the result in memory */
  else {
    if (psReq->result_data == NULL) {
      psReq->result_buf_size = size * nmemb + 10000;
      psReq->result_data = (char *)msSmallMalloc(psReq->result_buf_size);
    } else if (psReq->result_size + nmemb * size >
               (size_t)psReq->result_buf_size) {
      psReq->result_buf_size = psReq->result_size + nmemb * size + 10000;
      psReq->result_data =
          (char *)msSmallRealloc(psReq->result_data, psReq->result_buf_size);
    }

    if (psReq->result_data == NULL) {
      msSetError(MS_HTTPERR, "Unable to grow HTTP result buffer to size %d.",
                 "msHTTPWriteFct()", psReq->result_buf_size);
      psReq->result_buf_size = 0;
      psReq->result_size = 0;
      return -1;
    }

    memcpy(psReq->result_data + psReq->result_size, buffer, size * nmemb);
    psReq->result_size += size * nmemb;

    return size * nmemb;
  }
}

/**********************************************************************
 *                          msGetCURLAuthType()
 *
 * Returns the equivalent CURL CURLAUTH_ constant given a
 * MS_HTTP_AUTH_TYPE, or CURLAUTH_BASIC if no match is found.
 **********************************************************************/
long msGetCURLAuthType(enum MS_HTTP_AUTH_TYPE authType) {
  switch (authType) {
  case MS_BASIC:
    return CURLAUTH_BASIC;
  case MS_DIGEST:
    return CURLAUTH_DIGEST;
  case MS_NTLM:
    return CURLAUTH_NTLM;
  case MS_ANY:
    return CURLAUTH_ANY;
  case MS_ANYSAFE:
    return CURLAUTH_ANYSAFE;
  default:
    return CURLAUTH_BASIC;
  }
}

/**********************************************************************
 *                          msHTTPAuthProxySetup()
 *
 * Common code used by msPrepareWFSLayerRequest() and
 * msPrepareWMSLayerRequest() to handle proxy / http auth for requests
 *
 * Return value:
 * MS_SUCCESS if all requests completed succesfully.
 * MS_FAILURE if a fatal error happened
 **********************************************************************/
int msHTTPAuthProxySetup(hashTableObj *mapmd, hashTableObj *lyrmd,
                         httpRequestObj *pasReqInfo, int numRequests,
                         mapObj *map, const char *namespaces) {

  const char *pszTmp;
  char *pszProxyHost = NULL;
  long nProxyPort = 0;
  char *pszProxyUsername = NULL, *pszProxyPassword = NULL;
  char *pszHttpAuthUsername = NULL, *pszHttpAuthPassword = NULL;
  enum MS_HTTP_AUTH_TYPE eHttpAuthType = MS_BASIC;
  enum MS_HTTP_AUTH_TYPE eProxyAuthType = MS_BASIC;
  enum MS_HTTP_PROXY_TYPE eProxyType = MS_HTTP;

  /* ------------------------------------------------------------------
   * Check for authentication and proxying metadata. If the metadata is not
   * found in the layer metadata, check the map-level metadata.
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces, "proxy_host")) !=
      NULL) {
    pszProxyHost = msStrdup(pszTmp);
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces, "proxy_port")) !=
      NULL) {
    nProxyPort = atol(pszTmp);
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces, "proxy_type")) !=
      NULL) {

    if (strcasecmp(pszTmp, "HTTP") == 0)
      eProxyType = MS_HTTP;
    else if (strcasecmp(pszTmp, "SOCKS5") == 0)
      eProxyType = MS_SOCKS5;
    else {
      msSetError(MS_WMSERR, "Invalid proxy_type metadata '%s' specified",
                 "msHTTPAuthProxySetup()", pszTmp);
      return MS_FAILURE;
    }
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces,
                                     "proxy_auth_type")) != NULL) {
    if (strcasecmp(pszTmp, "BASIC") == 0)
      eProxyAuthType = MS_BASIC;
    else if (strcasecmp(pszTmp, "DIGEST") == 0)
      eProxyAuthType = MS_DIGEST;
    else if (strcasecmp(pszTmp, "NTLM") == 0)
      eProxyAuthType = MS_NTLM;
    else if (strcasecmp(pszTmp, "ANY") == 0)
      eProxyAuthType = MS_ANY;
    else if (strcasecmp(pszTmp, "ANYSAFE") == 0)
      eProxyAuthType = MS_ANYSAFE;
    else {
      msSetError(MS_WMSERR, "Invalid proxy_auth_type metadata '%s' specified",
                 "msHTTPAuthProxySetup()", pszTmp);
      return MS_FAILURE;
    }
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces,
                                     "proxy_username")) != NULL) {
    pszProxyUsername = msStrdup(pszTmp);
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces,
                                     "proxy_password")) != NULL) {
    pszProxyPassword = msDecryptStringTokens(map, pszTmp);
    if (pszProxyPassword == NULL) {
      msFree(pszProxyHost);
      msFree(pszProxyUsername);
      return (MS_FAILURE); /* An error should already have been produced */
    }
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces, "auth_type")) !=
      NULL) {
    if (strcasecmp(pszTmp, "BASIC") == 0)
      eHttpAuthType = MS_BASIC;
    else if (strcasecmp(pszTmp, "DIGEST") == 0)
      eHttpAuthType = MS_DIGEST;
    else if (strcasecmp(pszTmp, "NTLM") == 0)
      eHttpAuthType = MS_NTLM;
    else if (strcasecmp(pszTmp, "ANY") == 0)
      eHttpAuthType = MS_ANY;
    else if (strcasecmp(pszTmp, "ANYSAFE") == 0)
      eHttpAuthType = MS_ANYSAFE;
    else {
      msSetError(MS_WMSERR, "Invalid auth_type metadata '%s' specified",
                 "msHTTPAuthProxySetup()", pszTmp);
      return MS_FAILURE;
    }
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces,
                                     "auth_username")) != NULL) {
    pszHttpAuthUsername = msStrdup(pszTmp);
  }

  if ((pszTmp = msOWSLookupMetadata2(lyrmd, mapmd, namespaces,
                                     "auth_password")) != NULL) {
    pszHttpAuthPassword = msDecryptStringTokens(map, pszTmp);
    if (pszHttpAuthPassword == NULL) {
      msFree(pszHttpAuthUsername);
      msFree(pszProxyHost);
      msFree(pszProxyUsername);
      msFree(pszProxyPassword);
      return (MS_FAILURE); /* An error should already have been produced */
    }
  }

  pasReqInfo[numRequests].pszProxyAddress = pszProxyHost;
  pasReqInfo[numRequests].nProxyPort = nProxyPort;
  pasReqInfo[numRequests].eProxyType = eProxyType;
  pasReqInfo[numRequests].eProxyAuthType = eProxyAuthType;
  pasReqInfo[numRequests].pszProxyUsername = pszProxyUsername;
  pasReqInfo[numRequests].pszProxyPassword = pszProxyPassword;
  pasReqInfo[numRequests].eHttpAuthType = eHttpAuthType;
  pasReqInfo[numRequests].pszHttpUsername = pszHttpAuthUsername;
  pasReqInfo[numRequests].pszHttpPassword = pszHttpAuthPassword;

  return MS_SUCCESS;
}

/**********************************************************************
 *                          msHTTPExecuteRequests()
 *
 * Fetch a map slide via HTTP request and save to specified temp file.
 *
 * If bCheckLocalCache==MS_TRUE then if the pszOutputfile already exists
 * then is is not downloaded again, and status 242 is returned.
 *
 * Return value:
 * MS_SUCCESS if all requests completed succesfully.
 * MS_FAILURE if a fatal error happened
 * MS_DONE if some requests failed with 40x status for instance (not fatal)
 **********************************************************************/
int msHTTPExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                          int bCheckLocalCache) {
  int i, nStatus = MS_SUCCESS, nTimeout, still_running = 0, num_msgs = 0;
  CURLM *multi_handle;
  CURLMsg *curl_msg;
  char debug = MS_FALSE;
  const char *pszCurlCABundle = NULL;

  if (numRequests == 0)
    return MS_SUCCESS; /* Nothing to do */

  if (!gbCurlInitialized)
    msHTTPInit();

  /* Establish the timeout (seconds) for how long we are going to wait
   * for a response.
   * We use the longest timeout value in the array of requests
   */
  nTimeout = pasReqInfo[0].nTimeout;
  for (i = 0; i < numRequests; i++) {
    if (pasReqInfo[i].nTimeout > nTimeout)
      nTimeout = pasReqInfo[i].nTimeout;

    if (pasReqInfo[i].debug)
      debug = MS_TRUE; /* For the download loop */
  }

  if (nTimeout <= 0)
    nTimeout = 30;

  /* Check if we've got a CURL_CA_BUNDLE env. var.
   * If set then the value is the full path to the ca-bundle.crt file
   * e.g. CURL_CA_BUNDLE=/usr/local/share/curl/curl-ca-bundle.crt
   */
  pszCurlCABundle = CPLGetConfigOption("CURL_CA_BUNDLE", NULL);

  if (debug) {
    msDebug("HTTP: Starting to prepare HTTP requests.\n");
    if (pszCurlCABundle)
      msDebug("Using CURL_CA_BUNDLE=%s\n", pszCurlCABundle);
  }

  const char *pszHttpVersion = CPLGetConfigOption("CURL_HTTP_VERSION", NULL);

  /* Alloc a curl-multi handle, and add a curl-easy handle to it for each
   * file to download.
   */
  multi_handle = curl_multi_init();
  if (multi_handle == NULL) {
    msSetError(MS_HTTPERR, "curl_multi_init() failed.",
               "msHTTPExecuteRequests()");
    return (MS_FAILURE);
  }

  for (i = 0; i < numRequests; i++) {
    CURL *http_handle;
    FILE *fp;

    if (pasReqInfo[i].pszGetUrl == NULL) {
      msSetError(MS_HTTPERR, "URL or output file parameter missing.",
                 "msHTTPExecuteRequests()");
      return (MS_FAILURE);
    }

    if (pasReqInfo[i].debug) {
      msDebug("HTTP request: id=%d, %s\n", pasReqInfo[i].nLayerId,
              pasReqInfo[i].pszGetUrl);
    }

    /* Reset some members */
    pasReqInfo[i].nStatus = 0;
    if (pasReqInfo[i].pszContentType)
      free(pasReqInfo[i].pszContentType);
    pasReqInfo[i].pszContentType = NULL;

    /* Check local cache if requested */
    if (bCheckLocalCache && pasReqInfo[i].pszOutputFile != NULL) {
      fp = fopen(pasReqInfo[i].pszOutputFile, "r");
      if (fp) {
        /* File already there, don't download again. */
        if (pasReqInfo[i].debug)
          msDebug("HTTP request: id=%d, found in cache, skipping.\n",
                  pasReqInfo[i].nLayerId);
        fclose(fp);
        pasReqInfo[i].nStatus = 242;
        pasReqInfo[i].pszContentType = msStrdup("unknown/cached");
        continue;
      }
    }

    /* Alloc curl handle */
    http_handle = curl_easy_init();
    if (http_handle == NULL) {
      msSetError(MS_HTTPERR, "curl_easy_init() failed.",
                 "msHTTPExecuteRequests()");
      return (MS_FAILURE);
    }

    pasReqInfo[i].curl_handle = http_handle;

    /* set URL, note that curl keeps only a ref to our string buffer */
    unchecked_curl_easy_setopt(http_handle, CURLOPT_URL,
                               pasReqInfo[i].pszGetUrl);

    unchecked_curl_easy_setopt(http_handle, CURLOPT_PROTOCOLS,
                               CURLPROTO_HTTP | CURLPROTO_HTTPS);

    if (pszHttpVersion && strcmp(pszHttpVersion, "1.0") == 0)
      unchecked_curl_easy_setopt(http_handle, CURLOPT_HTTP_VERSION,
                                 CURL_HTTP_VERSION_1_0);
    else if (pszHttpVersion && strcmp(pszHttpVersion, "1.1") == 0)
      unchecked_curl_easy_setopt(http_handle, CURLOPT_HTTP_VERSION,
                                 CURL_HTTP_VERSION_1_1);

    /* Set User-Agent (auto-generate if not set by caller */
    if (pasReqInfo[i].pszUserAgent == NULL) {
      curl_version_info_data *psCurlVInfo;

      psCurlVInfo = curl_version_info(CURLVERSION_NOW);

      pasReqInfo[i].pszUserAgent = (char *)msSmallMalloc(100 * sizeof(char));

      if (pasReqInfo[i].pszUserAgent) {
        sprintf(pasReqInfo[i].pszUserAgent, "MapServer/%s libcurl/%d.%d.%d",
                MS_VERSION, psCurlVInfo->version_num / 0x10000 & 0xff,
                psCurlVInfo->version_num / 0x100 & 0xff,
                psCurlVInfo->version_num & 0xff);
      }
    }
    if (pasReqInfo[i].pszUserAgent) {
      unchecked_curl_easy_setopt(http_handle, CURLOPT_USERAGENT,
                                 pasReqInfo[i].pszUserAgent);
    }

    /* Enable following redirections.  Requires libcurl 7.10.1 at least */
    unchecked_curl_easy_setopt(http_handle, CURLOPT_FOLLOWLOCATION, 1);
    unchecked_curl_easy_setopt(http_handle, CURLOPT_MAXREDIRS, 10);

    /* Set timeout.*/
    unchecked_curl_easy_setopt(http_handle, CURLOPT_TIMEOUT, nTimeout);

    /* Pass CURL_CA_BUNDLE if set */
    if (pszCurlCABundle)
      unchecked_curl_easy_setopt(http_handle, CURLOPT_CAINFO, pszCurlCABundle);

    /* Set proxying settings */
    if (pasReqInfo[i].pszProxyAddress != NULL &&
        strlen(pasReqInfo[i].pszProxyAddress) > 0) {
      long nProxyType = CURLPROXY_HTTP;

      unchecked_curl_easy_setopt(http_handle, CURLOPT_PROXY,
                                 pasReqInfo[i].pszProxyAddress);

      if (pasReqInfo[i].nProxyPort > 0 && pasReqInfo[i].nProxyPort < 65535) {
        unchecked_curl_easy_setopt(http_handle, CURLOPT_PROXYPORT,
                                   pasReqInfo[i].nProxyPort);
      }

      switch (pasReqInfo[i].eProxyType) {
      case MS_HTTP:
        nProxyType = CURLPROXY_HTTP;
        break;
      case MS_SOCKS5:
        nProxyType = CURLPROXY_SOCKS5;
        break;
      }
      unchecked_curl_easy_setopt(http_handle, CURLOPT_PROXYTYPE, nProxyType);

      /* If there is proxy authentication information, set it */
      if (pasReqInfo[i].pszProxyUsername != NULL &&
          pasReqInfo[i].pszProxyPassword != NULL &&
          strlen(pasReqInfo[i].pszProxyUsername) > 0 &&
          strlen(pasReqInfo[i].pszProxyPassword) > 0) {
        char szUsernamePasswd[128];
#if LIBCURL_VERSION_NUM >= 0x070a07
        long nProxyAuthType = CURLAUTH_BASIC;
        /* CURLOPT_PROXYAUTH available only in Curl 7.10.7 and up */
        nProxyAuthType = msGetCURLAuthType(pasReqInfo[i].eProxyAuthType);
        unchecked_curl_easy_setopt(http_handle, CURLOPT_PROXYAUTH,
                                   nProxyAuthType);
#else
        /* We log an error but don't abort processing */
        msSetError(MS_HTTPERR,
                   "CURLOPT_PROXYAUTH not supported. Requires Curl 7.10.7 and "
                   "up. *_proxy_auth_type setting ignored.",
                   "msHTTPExecuteRequests()");
#endif /* LIBCURL_VERSION_NUM */

        snprintf(szUsernamePasswd, 127, "%s:%s", pasReqInfo[i].pszProxyUsername,
                 pasReqInfo[i].pszProxyPassword);
        unchecked_curl_easy_setopt(http_handle, CURLOPT_PROXYUSERPWD,
                                   szUsernamePasswd);
      }
    }

    /* Set HTTP Authentication settings */
    if (pasReqInfo[i].pszHttpUsername != NULL &&
        pasReqInfo[i].pszHttpPassword != NULL &&
        strlen(pasReqInfo[i].pszHttpUsername) > 0 &&
        strlen(pasReqInfo[i].pszHttpPassword) > 0) {
      char szUsernamePasswd[128];
      long nHttpAuthType = CURLAUTH_BASIC;

      snprintf(szUsernamePasswd, 127, "%s:%s", pasReqInfo[i].pszHttpUsername,
               pasReqInfo[i].pszHttpPassword);
      unchecked_curl_easy_setopt(http_handle, CURLOPT_USERPWD,
                                 szUsernamePasswd);

      nHttpAuthType = msGetCURLAuthType(pasReqInfo[i].eHttpAuthType);
      unchecked_curl_easy_setopt(http_handle, CURLOPT_HTTPAUTH, nHttpAuthType);
    }

    /* NOSIGNAL should be set to true for timeout to work in multithread
     * environments on Unix, requires libcurl 7.10 or more recent.
     * (this force avoiding the use of sgnal handlers)
     */
#ifdef CURLOPT_NOSIGNAL
    unchecked_curl_easy_setopt(http_handle, CURLOPT_NOSIGNAL, 1);
#endif

    /* If we are writing file to disk, open the file now. */
    if (pasReqInfo[i].pszOutputFile != NULL) {
      if ((fp = fopen(pasReqInfo[i].pszOutputFile, "wb")) == NULL) {
        msSetError(MS_HTTPERR, "Can't open output file %s.",
                   "msHTTPExecuteRequests()", pasReqInfo[i].pszOutputFile);
        return (MS_FAILURE);
      }

      pasReqInfo[i].fp = fp;
    }

    /* coverity[bad_sizeof] */
    unchecked_curl_easy_setopt(http_handle, CURLOPT_WRITEDATA,
                               &(pasReqInfo[i]));
    unchecked_curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION,
                               msHTTPWriteFct);

    /* Provide a buffer where libcurl can write human readable error msgs
     */
    if (pasReqInfo[i].pszErrBuf == NULL)
      pasReqInfo[i].pszErrBuf =
          (char *)msSmallMalloc((CURL_ERROR_SIZE + 1) * sizeof(char));
    pasReqInfo[i].pszErrBuf[0] = '\0';

    unchecked_curl_easy_setopt(http_handle, CURLOPT_ERRORBUFFER,
                               pasReqInfo[i].pszErrBuf);

    if (pasReqInfo[i].pszPostRequest != NULL) {
      char szBuf[100];

      struct curl_slist *headers = NULL;
      snprintf(szBuf, 100, "Content-Type: %s",
               pasReqInfo[i].pszPostContentType);
      headers = curl_slist_append(headers, szBuf);

      unchecked_curl_easy_setopt(http_handle, CURLOPT_POST, 1);
      unchecked_curl_easy_setopt(http_handle, CURLOPT_POSTFIELDS,
                                 pasReqInfo[i].pszPostRequest);
      if (debug) {
        msDebug("HTTP: POST = %s", pasReqInfo[i].pszPostRequest);
      }
      unchecked_curl_easy_setopt(http_handle, CURLOPT_HTTPHEADER, headers);
      /* curl_slist_free_all(headers); */ /* free the header list */
    }

    /* Added by RFC-42 HTTP Cookie Forwarding */
    if (pasReqInfo[i].pszHTTPCookieData != NULL) {
      /* Check if there's no end of line in the Cookie string */
      /* This could break the HTTP Header */
      for (size_t nPos = 0; nPos < strlen(pasReqInfo[i].pszHTTPCookieData);
           nPos++) {
        if (pasReqInfo[i].pszHTTPCookieData[nPos] == '\n') {
          msSetError(MS_HTTPERR,
                     "Can't use cookie containing a newline character.",
                     "msHTTPExecuteRequests()");
          return (MS_FAILURE);
        }
      }

      /* Set the Curl option to send Cookie */
      unchecked_curl_easy_setopt(http_handle, CURLOPT_COOKIE,
                                 pasReqInfo[i].pszHTTPCookieData);
    }

    /* Add to multi handle */
    curl_multi_add_handle(multi_handle, http_handle);
  }

  if (debug) {
    msDebug("HTTP: Before download loop\n");
  }

  /* DOWNLOAD LOOP ... inspired from multi-double.c example */

  /* we start some action by calling perform right away */
  while (CURLM_CALL_MULTI_PERFORM ==
         curl_multi_perform(multi_handle, &still_running))
    ;

  while (still_running) {
    struct timeval timeout;
    int rc; /* select() return code */

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    /* set a suitable timeout to play around with */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    /* get file descriptors from the transfers */
    curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

    rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    switch (rc) {
    case -1:
      /* select error */

      /* ==================================================================== */
      /*      On Windows the select function (just above) returns -1 when     */
      /*      it is called the second time and all the calls after            */
      /*      that. This causes an infinite loop.                             */
      /*      I do not really know why.                                       */
      /*      To sovle the problem the break frop case -1 has been removed.   */
      /* ==================================================================== */
#ifndef _WIN32
      break;
#endif
    case 0:
    default:
      /* timeout or readable/writable sockets */
      curl_multi_perform(multi_handle, &still_running);
      break;
    }
  }

  if (debug)
    msDebug("HTTP: After download loop\n");

  /* Scan message stack from CURL and report fatal errors*/

  while ((curl_msg = curl_multi_info_read(multi_handle, &num_msgs)) != NULL) {
    httpRequestObj *psReq = NULL;

    if (curl_msg->msg == CURLMSG_DONE && curl_msg->data.result != CURLE_OK) {
      /* Something went wrong with this transfer... report error */

      for (i = 0; i < numRequests; i++) {
        if (pasReqInfo[i].curl_handle == curl_msg->easy_handle) {
          psReq = &(pasReqInfo[i]);
          break;
        }
      }

      if (psReq != NULL) {
        /* Record error code in nStatus as a negative value */
        psReq->nStatus = -curl_msg->data.result;
      }
    }
  }

  if (debug) {
    /* Print a msDebug header for timings reported in the loop below */
    msDebug("msHTTPExecuteRequests() timing summary per layer (connect_time + "
            "time_to_first_packet + download_time = total_time in seconds)\n");
  }

  /* Check status of all requests, close files, report errors and cleanup
   * handles
   */
  for (i = 0; i < numRequests; i++) {
    httpRequestObj *psReq;
    CURL *http_handle;
    long lVal = 0;

    psReq = &(pasReqInfo[i]);

    if (psReq->nStatus == 242)
      continue; /* Nothing to do here, this file was in cache already */

    if (psReq->fp)
      fclose(psReq->fp);
    psReq->fp = NULL;

    http_handle = (CURL *)(psReq->curl_handle);

    if (psReq->nStatus == 0 &&
        curl_easy_getinfo(http_handle, CURLINFO_HTTP_CODE, &lVal) == CURLE_OK) {
      char *pszContentType = NULL;

      psReq->nStatus = lVal;

      /* Fetch content type of response */
      if (curl_easy_getinfo(http_handle, CURLINFO_CONTENT_TYPE,
                            &pszContentType) == CURLE_OK &&
          pszContentType != NULL) {
        psReq->pszContentType = msStrdup(pszContentType);
      }
    }

    if (!MS_HTTP_SUCCESS(psReq->nStatus)) {
      /* Set status to MS_DONE to indicate that transfers were  */
      /* completed but may not be succesfull */
      nStatus = MS_DONE;

      if (psReq->nStatus == -(CURLE_OPERATION_TIMEOUTED)) {
        /* Timeout isn't a fatal error */
        if (psReq->debug)
          msDebug("HTTP: TIMEOUT of %d seconds exceeded for %s\n", nTimeout,
                  psReq->pszGetUrl);

        msSetError(MS_HTTPERR, "HTTP: TIMEOUT of %d seconds exceeded for %s\n",
                   "msHTTPExecuteRequests()", nTimeout, psReq->pszGetUrl);

        /* Rewrite error message, the curl timeout message isn't
         * of much use to our users.
         */
        sprintf(psReq->pszErrBuf, "TIMEOUT of %d seconds exceeded.", nTimeout);
      } else if (psReq->nStatus > 0) {
        /* Got an HTTP Error, e.g. 404, etc. */

        if (psReq->debug)
          msDebug("HTTP: HTTP GET request failed with status %d (%s)"
                  " for %s\n",
                  psReq->nStatus, psReq->pszErrBuf, psReq->pszGetUrl);

        msSetError(MS_HTTPERR,
                   "HTTP GET request failed with status %d (%s) "
                   "for %s",
                   "msHTTPExecuteRequests()", psReq->nStatus, psReq->pszErrBuf,
                   psReq->pszGetUrl);
      } else {
        /* Got a curl error */

        errorObj *error = msGetErrorObj();
        if (psReq->debug)
          msDebug("HTTP: request failed with curl error "
                  "code %d (%s) for %s",
                  -psReq->nStatus, psReq->pszErrBuf, psReq->pszGetUrl);

        if (!error ||
            error->code ==
                MS_NOERR) /* only set error if one hasn't already been set */
          msSetError(MS_HTTPERR,
                     "HTTP: request failed with curl error "
                     "code %d (%s) for %s",
                     "msHTTPExecuteRequests()", -psReq->nStatus,
                     psReq->pszErrBuf, psReq->pszGetUrl);
      }
    }

    /* Report download times foreach handle, in debug mode */
    if (psReq->debug) {
      double dConnectTime = 0.0, dTotalTime = 0.0, dStartTfrTime = 0.0;

      curl_easy_getinfo(http_handle, CURLINFO_CONNECT_TIME, &dConnectTime);
      curl_easy_getinfo(http_handle, CURLINFO_STARTTRANSFER_TIME,
                        &dStartTfrTime);
      curl_easy_getinfo(http_handle, CURLINFO_TOTAL_TIME, &dTotalTime);
      /* STARTTRANSFER_TIME includes CONNECT_TIME, but TOTAL_TIME
       * doesn't, so we need to add it.
       */
      dTotalTime += dConnectTime;

      msDebug("Layer %d: %.3f + %.3f + %.3f = %.3fs\n", psReq->nLayerId,
              dConnectTime, dStartTfrTime - dConnectTime,
              dTotalTime - dStartTfrTime, dTotalTime);
    }

    /* Cleanup this handle */
    unchecked_curl_easy_setopt(http_handle, CURLOPT_URL, "");
    curl_multi_remove_handle(multi_handle, http_handle);
    curl_easy_cleanup(http_handle);
    psReq->curl_handle = NULL;
  }

  /* Cleanup multi handle, each handle had to be cleaned up individually */
  curl_multi_cleanup(multi_handle);

  return nStatus;
}

/**********************************************************************
 *                          msHTTPGetFile()
 *
 * Wrapper to call msHTTPExecuteRequests() for a single file.
 **********************************************************************/
int msHTTPGetFile(const char *pszGetUrl, const char *pszOutputFile,
                  int *pnHTTPStatus, int nTimeout, int bCheckLocalCache,
                  int bDebug, int nMaxBytes) {
  httpRequestObj *pasReqInfo;

  /* Alloc httpRequestInfo structs through which status of each request
   * will be returned.
   * We need to alloc 2 instance of requestobj so that the last
   * object in the array can be set to NULL.
   */
  pasReqInfo = (httpRequestObj *)calloc(2, sizeof(httpRequestObj));
  MS_CHECK_ALLOC(pasReqInfo, 2 * sizeof(httpRequestObj), MS_FAILURE);

  msHTTPInitRequestObj(pasReqInfo, 2);

  pasReqInfo[0].pszGetUrl = msStrdup(pszGetUrl);
  pasReqInfo[0].pszOutputFile = msStrdup(pszOutputFile);
  pasReqInfo[0].debug = (char)bDebug;
  pasReqInfo[0].nTimeout = nTimeout;
  pasReqInfo[0].nMaxBytes = nMaxBytes;

  if (msHTTPExecuteRequests(pasReqInfo, 1, bCheckLocalCache) != MS_SUCCESS) {
    *pnHTTPStatus = pasReqInfo[0].nStatus;
    if (pasReqInfo[0].debug)
      msDebug("HTTP request failed for %s.\n", pszGetUrl);
    msHTTPFreeRequestObj(pasReqInfo, 2);
    free(pasReqInfo);
    return MS_FAILURE;
  }

  *pnHTTPStatus = pasReqInfo[0].nStatus;

  msHTTPFreeRequestObj(pasReqInfo, 2);
  free(pasReqInfo);

  return MS_SUCCESS;
}

#endif /* defined(USE_WMS_LYR) || defined(USE_WMS_SVR) */
