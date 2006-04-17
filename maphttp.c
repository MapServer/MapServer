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
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.22  2006/04/17 19:06:49  dan
 * Set User-Agent in HTTP headers of client WMS/WFS connections (bug 1749)
 *
 * Revision 1.21  2005/07/15 13:37:47  frank
 * Acquire TLOCK_OWS around whole func in msHTTPCleanup().
 *
 * Revision 1.20  2005/07/14 21:43:26  hobu
 * Fix bug 1417 msHTTPInit() not ever called
 *
 * Revision 1.19  2005/05/13 17:23:34  dan
 * First pass at properly handling XML exceptions from CONNECTIONTYPE WMS
 * layers. Still needs some work. (bug 1246)
 *
 * Revision 1.18  2005/02/18 03:06:45  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.17  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.16  2004/03/29 14:41:56  dan
 * Use CURL's internal timer instead of custom gettimeofday() calls for
 * timing WMS/WFS requests
 *
 * Revision 1.15  2004/03/18 23:11:12  dan
 * Added detailed reporting (using msDebug) of layer rendering times
 *
 * Revision 1.14  2004/03/11 22:45:39  dan
 * Added pszPostContentType in httpRequestObj instead of using hardcoded
 * text/html mime type for all post requests.
 *
 * Revision 1.13  2003/09/19 21:54:19  assefa
 * Add support fot the Post request.
 *
 * Revision 1.12  2003/06/12 20:35:16  dan
 * Fixed test on result of curl_easy_init() in msHTTPExecuteRequests()
 *
 * Revision 1.11  2003/04/23 21:32:25  dan
 * More fixes to return value and error reporting in msHTTPExecuteRequests()
 *
 * Revision 1.10  2003/04/23 15:06:56  dan
 * Added better error reporting for timeout situations
 *
 * Revision 1.9  2003/03/26 20:24:38  dan
 * Do not call msDebug() unless debug flag is turned on
 *
 * Revision 1.8  2003/01/10 19:23:22  dan
 * Do not produce a fatal error if WMS layer download fails, just like in 3.6
 *
 * Revision 1.7  2003/01/08 22:19:35  assefa
 * Patch for windows with problems related to the select function.
 *
 * Revision 1.6  2003/01/07 23:47:00  assefa
 * Correct Compilation error.
 *
 * Revision 1.5  2002/12/19 06:30:59  dan
 * Enable caching WMS/WFS request using tmp filename built from URL
 *
 * Revision 1.4  2002/12/17 21:33:54  dan
 * Enable following redirections with libcurl (requires libcurl 7.10.1+)
 *
 * Revision 1.3  2002/12/16 21:32:59  dan
 * Made CURLOPT_NOSIGNAL setting optional for now to work with libcurl 7.9.6+
 *
 * Revision 1.2  2002/12/16 20:34:59  dan
 * Flush libwww and use libcurl instead for HTTP requests in WMS/WFS client
 *
 * Revision 1.1  2002/10/09 02:29:03  dan
 * Initial implementation of WFS client support.
 *
 **********************************************************************/

/* For now this code is enabled only when WMS/WFS client is enabled.
 * This should be changed to a test on the presence of libcurl which
 * is really what the real dependency is.
 */
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

#include "map.h"
#include "maperror.h"
#include "mapows.h"
#include "mapthread.h"

MS_CVSID("$Id$")

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

int msHTTPInit()
{
    /* curl_global_init() should only be called once (no matter how
     * many threads or libcurl sessions that'll be used) by every
     * application that uses libcurl.
     */
     
    msAcquireLock(TLOCK_OWS);
    if (!gbCurlInitialized && 
        curl_global_init(CURL_GLOBAL_ALL) != 0)
    {
        msReleaseLock(TLOCK_OWS);
        msSetError(MS_HTTPERR, "Libcurl initialization failed.", 
                   "msHTTPInit()");
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
void msHTTPCleanup()
{
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
 * Note that users of this module should always allocate and init one
 * more instance of httpRequestObj in their array than what they plan to 
 * use because the terminate_handler() needs the last entry in the array 
 * to have reqObj->request == NULL
 *
 **********************************************************************/
void msHTTPInitRequestObj(httpRequestObj *pasReqInfo, int numRequests) 
{
    int i;
    if (!gbCurlInitialized)
        msHTTPInit();
  
    for(i=0; i<numRequests; i++)
    {
        pasReqInfo[i].pszGetUrl = NULL;
        pasReqInfo[i].pszPostRequest = NULL;
        pasReqInfo[i].pszPostContentType = NULL;
        pasReqInfo[i].pszOutputFile = NULL;
        pasReqInfo[i].nLayerId = 0;
        pasReqInfo[i].nTimeout = 0;
        pasReqInfo[i].nStatus = 0;
        pasReqInfo[i].pszContentType = NULL;
        pasReqInfo[i].pszErrBuf = NULL;
        pasReqInfo[i].pszUserAgent = NULL;

        pasReqInfo[i].debug = MS_FALSE;

        pasReqInfo[i].curl_handle = NULL;
        pasReqInfo[i].fp = NULL;

    }
}


/**********************************************************************
 *                          msHTTPFreeRequestObj()
 *
 **********************************************************************/
void msHTTPFreeRequestObj(httpRequestObj *pasReqInfo, int numRequests) 
{
    int i;
    for(i=0; i<numRequests; i++)
    {
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

        pasReqInfo[i].curl_handle = NULL;
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
                             void *reqInfo)
{
    httpRequestObj *psReq;

    psReq = (httpRequestObj *)reqInfo;

    if (psReq->debug)
    {
        msDebug("msHTTPWriteFct(id=%d, %d bytes)\n",
                psReq->nLayerId, size*nmemb);
    }

    return fwrite(buffer, size, nmemb, psReq->fp);
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
                          int bCheckLocalCache)
{
    int     i, nStatus = MS_SUCCESS, nTimeout, still_running=0, num_msgs=0;
    CURLM   *multi_handle;
    CURLMsg *curl_msg;
    char     debug = MS_FALSE;

    if (numRequests == 0)
        return MS_SUCCESS;  /* Nothing to do */

    /* Establish the timeout (seconds) for how long we are going to wait 
     * for a response. 
     * We use the longest timeout value in the array of requests 
     */
    nTimeout = pasReqInfo[0].nTimeout;
    for (i=0; i<numRequests; i++)
    {
        if (pasReqInfo[i].nTimeout > nTimeout)
            nTimeout = pasReqInfo[i].nTimeout;

        if (pasReqInfo[i].debug)
            debug = MS_TRUE;  /* For the download loop */
    }

    if (nTimeout <= 0)
        nTimeout = 30;

    if (debug)
    {
        msDebug("HTTP: Starting to prepare HTTP requests.\n");
    }

    /* Alloc a curl-multi handle, and add a curl-easy handle to it for each
     * file to download.
     */
    multi_handle = curl_multi_init();
    if (multi_handle == NULL)
    {
        msSetError(MS_HTTPERR, "curl_multi_init() failed.", 
                   "msHTTPExecuteRequests()");
        return(MS_FAILURE);
    }

    for (i=0; i<numRequests; i++)
    {
        CURL *http_handle;
        FILE *fp;

        if (pasReqInfo[i].pszGetUrl == NULL || 
            pasReqInfo[i].pszOutputFile == NULL)
        {
            msSetError(MS_HTTPERR, "URL or output file parameter missing.", 
                       "msHTTPExecuteRequests()");
            return(MS_FAILURE);
        }

        if (pasReqInfo[i].debug)
        {
            msDebug("HTTP request: id=%d, %s\n", 
                    pasReqInfo[i].nLayerId, pasReqInfo[i].pszGetUrl);
        }

        /* Reset some members */
        pasReqInfo[i].nStatus = 0;
        if (pasReqInfo[i].pszContentType)
            free(pasReqInfo[i].pszContentType);
        pasReqInfo[i].pszContentType = NULL;

        /* Check local cache if requested */
        if (bCheckLocalCache)
        {
            fp = fopen(pasReqInfo[i].pszOutputFile, "r");
            if (fp)
            {
                /* File already there, don't download again. */
                if (pasReqInfo[i].debug)
                    msDebug("HTTP request: id=%d, found in cache, skipping.\n",
                            pasReqInfo[i].nLayerId);
                fclose(fp);
                pasReqInfo[i].nStatus = 242;
                pasReqInfo[i].pszContentType = strdup("unknown/cached");
                continue;
            }
        }

        /* Alloc curl handle */
        http_handle = curl_easy_init();
        if (http_handle == NULL)
        {
            msSetError(MS_HTTPERR, "curl_easy_init() failed.", 
                       "msHTTPExecuteRequests()");
            return(MS_FAILURE);
        }

        pasReqInfo[i].curl_handle = http_handle;

        /* set URL, note that curl keeps only a ref to our string buffer */
        curl_easy_setopt(http_handle, CURLOPT_URL, pasReqInfo[i].pszGetUrl );

        /* Set User-Agent (auto-generate if not set by caller */
        if (pasReqInfo[i].pszUserAgent == NULL)
        {
            curl_version_info_data *psCurlVInfo;

            psCurlVInfo = curl_version_info(CURLVERSION_NOW);

            pasReqInfo[i].pszUserAgent = (char*)malloc(100*sizeof(char));

            if (pasReqInfo[i].pszUserAgent)
            {
                sprintf(pasReqInfo[i].pszUserAgent, 
                        "MapServer/%s libcurl/%d.%d.%d",
                        MS_VERSION, 
                        psCurlVInfo->version_num/0x10000 & 0xff,
                        psCurlVInfo->version_num/0x100 & 0xff,
                        psCurlVInfo->version_num & 0xff );
            }
        }
        if (pasReqInfo[i].pszUserAgent)
        {
            curl_easy_setopt(http_handle, 
                             CURLOPT_USERAGENT, pasReqInfo[i].pszUserAgent );
        }

        /* Enable following redirections.  Requires libcurl 7.10.1 at least */
        curl_easy_setopt(http_handle, CURLOPT_FOLLOWLOCATION, 1 );
        curl_easy_setopt(http_handle, CURLOPT_MAXREDIRS, 10 );

        /* Set timeout.*/
        curl_easy_setopt(http_handle, CURLOPT_TIMEOUT, nTimeout );

        /* NOSIGNAL should be set to true for timeout to work in multithread
         * environments on Unix, requires libcurl 7.10 or more recent.
         * (this force avoiding the use of sgnal handlers)
         */
#ifdef CURLOPT_NOSIGNAL
        curl_easy_setopt(http_handle, CURLOPT_NOSIGNAL, 1 );
#endif

        /* Open output file and set write handler */
        if ( (fp = fopen(pasReqInfo[i].pszOutputFile, "wb")) == NULL)
        {
            msSetError(MS_HTTPERR, "Can't open output file %s.", 
                       "msHTTPExecuteRequests()", pasReqInfo[i].pszOutputFile);
            return(MS_FAILURE);
        }

        pasReqInfo[i].fp = fp;
        curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, &(pasReqInfo[i]));
        curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, msHTTPWriteFct);

        /* Provide a buffer where libcurl can write human readable error msgs
         */
        if (pasReqInfo[i].pszErrBuf == NULL)
            pasReqInfo[i].pszErrBuf = (char *)malloc((CURL_ERROR_SIZE+1)*
                                                     sizeof(char));
        pasReqInfo[i].pszErrBuf[0] = '\0';

        curl_easy_setopt(http_handle, CURLOPT_ERRORBUFFER, 
                         pasReqInfo[i].pszErrBuf);

        if(pasReqInfo[i].pszPostRequest != NULL )
        {
            char szBuf[100];

            struct curl_slist *headers=NULL;
            snprintf(szBuf, 100, 
                     "Content-Type: %s", pasReqInfo[i].pszPostContentType);
            headers = curl_slist_append(headers, szBuf);

            curl_easy_setopt(http_handle, CURLOPT_POST, 1 );
            curl_easy_setopt(http_handle, CURLOPT_POSTFIELDS, 
                             pasReqInfo[i].pszPostRequest);
            curl_easy_setopt(http_handle, CURLOPT_HTTPHEADER, headers);
            /* curl_slist_free_all(headers); */ /* free the header list */
        }
        /* Add to multi handle */
        curl_multi_add_handle(multi_handle, http_handle);

    }

    if (debug)
    {
        msDebug("HTTP: Before download loop\n");
    }

    /* DOWNLOAD LOOP ... inspired from multi-double.c example */

    /* we start some action by calling perform right away */
    while(CURLM_CALL_MULTI_PERFORM ==
          curl_multi_perform(multi_handle, &still_running));

    while(still_running) 
    {
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

        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

        switch(rc) 
        {
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

    while((curl_msg = curl_multi_info_read( multi_handle, &num_msgs)) != NULL)
    {
        httpRequestObj *psReq = NULL;

        if (curl_msg->msg == CURLMSG_DONE &&
            curl_msg->data.result != CURLE_OK)
        {
            /* Something went wrong with this transfer... report error */

            for (i=0; i<numRequests; i++)
            {
                if (pasReqInfo[i].curl_handle == curl_msg->easy_handle)
                {
                    psReq = &(pasReqInfo[i]);
                    break;
                }
            }

            if (psReq != NULL)
            {
                /* Record error code in nStatus as a negative value */
                psReq->nStatus = -curl_msg->data.result;
            }
        }
    }

    if (debug)
    {
        /* Print a msDebug header for timings reported in the loop below */
        msDebug("msHTTPExecuteRequests() timing summary per layer (connect_time + time_to_first_packet + download_time = total_time in seconds)\n");
    }

    /* Check status of all requests, close files, report errors and cleanup
     * handles 
     */
    for (i=0; i<numRequests; i++)
    {
        httpRequestObj *psReq;
        CURL *http_handle;
        long lVal=0;

        psReq = &(pasReqInfo[i]);

        if (psReq->nStatus == 242)
            continue;  /* Nothing to do here, this file was in cache already */

        if (psReq->fp)
            fclose(psReq->fp);
        psReq->fp = NULL;

        http_handle = (CURL*)(psReq->curl_handle);

        if (psReq->nStatus == 0 &&
            curl_easy_getinfo(http_handle,
                              CURLINFO_HTTP_CODE, &lVal) == CURLE_OK)
        {
            char *pszContentType = NULL;

            psReq->nStatus = lVal;

            /* Fetch content type of response */
            if (curl_easy_getinfo(http_handle, 
                                  CURLINFO_CONTENT_TYPE, 
                                  &pszContentType) == CURLE_OK &&
                pszContentType != NULL)
            {
                psReq->pszContentType = strdup(pszContentType);
            }
        }

        if (!MS_HTTP_SUCCESS(psReq->nStatus))
        {
            /* Set status to MS_DONE to indicate that transfers were  */
            /* completed but may not be succesfull */
            nStatus = MS_DONE;

            if (psReq->nStatus == -(CURLE_OPERATION_TIMEOUTED))
            {
                /* Timeout isn't a fatal error */
                if (psReq->debug)
                    msDebug("HTTP: TIMEOUT of %d seconds exceeded for %s\n",
                            nTimeout, psReq->pszGetUrl );

                msSetError(MS_HTTPERR, 
                           "HTTP: TIMEOUT of %d seconds exceeded for %s\n",
                           "msHTTPExecuteRequests()", 
                           nTimeout, psReq->pszGetUrl);

                /* Rewrite error message, the curl timeout message isn't
                 * of much use to our users.
                 */
                sprintf(psReq->pszErrBuf, 
                        "TIMEOUT of %d seconds exceeded.", nTimeout);
            }
            else if (psReq->nStatus > 0)
            {
                /* Got an HTTP Error, e.g. 404, etc. */

                if (psReq->debug)
                    msDebug("HTTP: HTTP GET request failed with status %d (%s)"
                            " for %s\n",
                            psReq->nStatus, psReq->pszErrBuf, 
                            psReq->pszGetUrl);

                msSetError(MS_HTTPERR, 
                           "HTTP GET request failed with status %d (%s) "
                           "for %s",
                           "msHTTPExecuteRequests()", psReq->nStatus, 
                           psReq->pszErrBuf, psReq->pszGetUrl);
            }
            else
            {
                /* Got a curl error */

                if (psReq->debug)
                    msDebug("HTTP: request failed with curl error "
                            "code %d (%s) for %s",
                            -psReq->nStatus, psReq->pszErrBuf, 
                            psReq->pszGetUrl);

                msSetError(MS_HTTPERR, 
                           "HTTP: request failed with curl error "
                           "code %d (%s) for %s",
                           "msHTTPExecuteRequests()", 
                           -psReq->nStatus, psReq->pszErrBuf, 
                           psReq->pszGetUrl);
            }
        }

        /* Report download times foreach handle, in debug mode */
        if (psReq->debug)
        {
            double dConnectTime=0.0, dTotalTime=0.0, dStartTfrTime=0.0;

            curl_easy_getinfo(http_handle, 
                              CURLINFO_CONNECT_TIME, &dConnectTime);
            curl_easy_getinfo(http_handle, 
                              CURLINFO_STARTTRANSFER_TIME, &dStartTfrTime);
            curl_easy_getinfo(http_handle, 
                              CURLINFO_TOTAL_TIME, &dTotalTime);
            /* STARTTRANSFER_TIME includes CONNECT_TIME, but TOTAL_TIME
             * doesn't, so we need to add it.
             */
            dTotalTime += dConnectTime;

            msDebug("Layer %d: %.3f + %.3f + %.3f = %.3fs\n", psReq->nLayerId,
                    dConnectTime, dStartTfrTime-dConnectTime, 
                    dTotalTime-dStartTfrTime, dTotalTime);
        }

        /* Cleanup this handle */
        curl_easy_setopt(http_handle, CURLOPT_URL, "" );
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
                  int bDebug)
{
    httpRequestObj *pasReqInfo;

    /* Alloc httpRequestInfo structs through which status of each request 
     * will be returned.
     * We need to alloc 2 instance of requestobj so that the last
     * object in the array can be set to NULL. 
    */
    pasReqInfo = (httpRequestObj*)calloc(2, sizeof(httpRequestObj));
    msHTTPInitRequestObj(pasReqInfo, 2);

    pasReqInfo[0].pszGetUrl = strdup(pszGetUrl);
    pasReqInfo[0].pszOutputFile = strdup(pszOutputFile);
    pasReqInfo[0].debug = (char)bDebug;

    if (msHTTPExecuteRequests(pasReqInfo, 1, bCheckLocalCache) != MS_SUCCESS)
    {
        *pnHTTPStatus = pasReqInfo[0].nStatus;
        if (pasReqInfo[0].debug)
            msDebug("HTTP request failed for %s.\n", pszGetUrl);
        return MS_FAILURE;
    }

    *pnHTTPStatus = pasReqInfo[0].nStatus;

    msHTTPFreeRequestObj(pasReqInfo, 2);
    free(pasReqInfo);

    return MS_SUCCESS;
}


#endif /* defined(USE_WMS_LYR) || defined(USE_WMS_SVR) */
