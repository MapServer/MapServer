/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Utility functions to access files via HTTP (requires libcurl)
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2001-2002, Daniel Morissette, DM Solutions Group Inc
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

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <unistd.h>

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
    if (!gbCurlInitialized && 
        curl_global_init(CURL_GLOBAL_ALL) != 0)
    {
        msSetError(MS_HTTPERR, "Libcurl initialization failed.", 
                   "msHTTPInit()");
        return MS_FAILURE;
    }

    gbCurlInitialized = MS_TRUE;

    return MS_SUCCESS;
}


/**********************************************************************
 *                          msHTTPCleanup()
 *
 **********************************************************************/
void msHTTPCleanup()
{
    if (gbCurlInitialized)
        curl_global_cleanup();

    gbCurlInitialized = MS_FALSE;
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
    for(i=0; i<numRequests; i++)
    {
        pasReqInfo[i].pszGetUrl = NULL;
        pasReqInfo[i].pszOutputFile = NULL;
        pasReqInfo[i].nLayerId = 0;
        pasReqInfo[i].nTimeout = 0;
        pasReqInfo[i].nStatus = 0;
        pasReqInfo[i].pszContentType = NULL;
        pasReqInfo[i].pszErrBuf = NULL;

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

        if (pasReqInfo[i].pszOutputFile)
            free(pasReqInfo[i].pszOutputFile);
        pasReqInfo[i].pszOutputFile = NULL;

        if (pasReqInfo[i].pszContentType)
            free(pasReqInfo[i].pszContentType);
        pasReqInfo[i].pszContentType = NULL;

        if (pasReqInfo[i].pszErrBuf)
            free(pasReqInfo[i].pszErrBuf);
        pasReqInfo[i].pszErrBuf = NULL;

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

    msDebug("msHTTPWriteFct(id=%d, %d bytes)\n",
            psReq->nLayerId, size*nmemb);

    return fwrite(buffer, size, nmemb, psReq->fp);
}


/**********************************************************************
 *                          msHTTPExecuteRequests()
 *
 * Fetch a map slide via HTTP request and save to specified temp file.
 *
 * If bCheckLocalCache==MS_TRUE then if the pszOutputfile already exists 
 * then is is not downloaded again, and status 242 is returned.
 **********************************************************************/
int msHTTPExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                          int bCheckLocalCache)
{
    int     i, nStatus = MS_SUCCESS, nTimeout, still_running=0, num_msgs=0;
    CURLM   *multi_handle;
    CURLMsg *curl_msg;

    if (numRequests == 0)
        return MS_SUCCESS;  /* Nothing to do */

    /* Establish the timeout (seconds) for how long we are going to wait 
     * for a response. 
     * We use the longest timeout value in the array of requests 
     */
    nTimeout = pasReqInfo[0].nTimeout;
    for (i=1; i<numRequests; i++)
    {
        if (pasReqInfo[i].nTimeout > nTimeout)
            nTimeout = pasReqInfo[i].nTimeout;
    }

    if (nTimeout <= 0)
        nTimeout = 30;

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

        msDebug("HTTP request: id=%d, %s\n", 
                pasReqInfo[i].nLayerId, pasReqInfo[i].pszGetUrl);

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
                // File already there, don't download again.
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
        if (multi_handle == NULL)
        {
            msSetError(MS_HTTPERR, "curl_easy_init() failed.", 
                       "msHTTPExecuteRequests()");
            return(MS_FAILURE);
        }

        pasReqInfo[i].curl_handle = http_handle;

        /* set URL, note that curl keeps only a ref to our string buffer */
        curl_easy_setopt(http_handle, CURLOPT_URL, pasReqInfo[i].pszGetUrl );

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

        /* Add to multi handle */
        curl_multi_add_handle(multi_handle, http_handle);

    }
    msDebug("HTTP: Before download loop\n");

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
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        /* get file descriptors from the transfers */
        curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

        switch(rc) 
        {
          case -1:
            /* select error */
            break;
          case 0:
          default:
            /* timeout or readable/writable sockets */
            curl_multi_perform(multi_handle, &still_running);
            break;
        }
    }

    msDebug("HTTP: After download loop\n");

    /* Scan message stack from CURL and report fatal errors*/

    // __TODO__ We really need an error stack in maperror.c to report
    // multiple errors in places like here.
    // For now we'll just log errors using msDebug()

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

                switch(curl_msg->data.result)
                {
                  case CURLE_OPERATION_TIMEOUTED:
                    msDebug("HTTP: TIMEOUT of %d seconds execeeded for %s\n",
                            nTimeout, psReq->pszGetUrl );
                    break;
                  default:
                    msDebug("HTTP: request failed with curl error "
                            "code %d (%s) for %s\n",
                            curl_msg->data.result, psReq->pszErrBuf, 
                            psReq->pszGetUrl);

                    msSetError(MS_HTTPERR, 
                               "HTTP: request failed with curl error "
                               "code %d (%s) for %s",
                               "msHTTPExecuteRequests()", 
                               curl_msg->data.result, psReq->pszErrBuf, 
                               psReq->pszGetUrl);
                    break;
                }
            }
        }
    }

    /* Check status of all requests, close files, report errors and cleanup
     * handles 
     */
    for (i=0; i<numRequests; i++)
    {
        CURL *http_handle;
        long lVal=0;

        if (pasReqInfo[i].nStatus == 242)
            continue;  // Nothing to do here, this file was in cache already

        if (pasReqInfo[i].fp)
            fclose(pasReqInfo[i].fp);
        pasReqInfo[i].fp = NULL;

        http_handle = (CURL*)(pasReqInfo[i].curl_handle);

        if (pasReqInfo[i].nStatus == 0 &&
            curl_easy_getinfo(http_handle,
                              CURLINFO_HTTP_CODE, &lVal) == CURLE_OK)
        {
            pasReqInfo[i].nStatus = lVal;
        }

        if (!MS_HTTP_SUCCESS(pasReqInfo[i].nStatus))
        {
            nStatus = MS_FAILURE;

            if (pasReqInfo[i].nStatus == -(CURLE_OPERATION_TIMEOUTED))
            {
                nStatus = MS_SUCCESS;  // Timeout isn't a fatal error
            }
            else if (pasReqInfo[i].nStatus > 0)
            {
                msDebug("HTTP: HTTP GET request failed with status %d (%s) "
                        "for %s\n",
                        pasReqInfo[i].nStatus, pasReqInfo[i].pszErrBuf, 
                        pasReqInfo[i].pszGetUrl);

                msSetError(MS_HTTPERR, 
                           "HTTP GET request failed with status %d (%s) "
                           "for %s",
                           "msHTTPExecuteRequests()", pasReqInfo[i].nStatus, 
                           pasReqInfo[i].pszErrBuf, pasReqInfo[i].pszGetUrl);
            }
        }

        /* Cleanup this handle */
        curl_easy_setopt(http_handle, CURLOPT_URL, "" );
        curl_multi_remove_handle(multi_handle, http_handle);
        curl_easy_cleanup(http_handle);
        pasReqInfo[i].curl_handle = NULL;

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
int msHTTPGetFile(char *pszGetUrl, char *pszOutputFile, int *pnHTTPStatus,
                  int nTimeout, int bCheckLocalCache)
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
    
    if (msHTTPExecuteRequests(pasReqInfo, 1, bCheckLocalCache) != MS_SUCCESS)
    {
        *pnHTTPStatus = pasReqInfo[0].nStatus;
        msDebug("HTTP request failed.\n", pszGetUrl);
        free(pszGetUrl);
        return MS_FAILURE;
    }

    *pnHTTPStatus = pasReqInfo[0].nStatus;

    msHTTPFreeRequestObj(pasReqInfo, 2);
    free(pasReqInfo);

    return MS_SUCCESS;
}


#endif /* defined(USE_WMS_LYR) || defined(USE_WMS_SVR) */
