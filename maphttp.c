/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Utility functions to access files via HTTP (requires libwww)
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
 * Revision 1.1  2002/10/09 02:29:03  dan
 * Initial implementation of WFS client support.
 *
 **********************************************************************/

/* For now this code is enabled only when WMS/WFS client is enabled.
 * This should be changed to a test on the presence of libwww which
 * is really what the real dependency is.
 */
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

#include "map.h"
#include "maperror.h"
#include "mapows.h"

/*
** Note: This code uses the w3c-libwww library to access remote WMS servers 
** via the HTTP protocol.  
** See http://www.w3.org/Library/ for the lib's source code and install docs.
** See http://www.w3.org/Library/Examples/LoadToFile.c for sample code on which
** big portions of msHTTPExecuteRequests() was based.
*/

#include <WWWLib.h>
#include <WWWHTTP.h>
#include <WWWInit.h>


static int tracer (const char * fmt, va_list pArgs)
{
/* Do not write to stderr on windoze as this makes Apache hang. */
#ifdef ENABLE_STDERR_DEBUG
    return (vfprintf(stderr, fmt, pArgs));
#else
    return 0;
#endif
}


/* Non-preemptive version: uses an event loop.  
 * Works on both Linux and Windows.
 */

/*
**  We get called here from the event loop when we are done
**  loading.
*/
static int terminate_handler (HTRequest * request, HTResponse * response,
                              void * param, int status)
{
    httpRequestObj *pasReqInfo = NULL;
    int i, bAllDone=TRUE;
    
    if (param) 
        pasReqInfo = (httpRequestObj *)param;

    /* Record status information for this request */
    for(i=0; pasReqInfo[i].request != NULL; i++)
    {
        if (pasReqInfo[i].request == request)
        {
            pasReqInfo[i].nStatus = status;
        }
        else if (pasReqInfo[i].nStatus == 0)
        {
            bAllDone = FALSE; // At least one request is not done yet.
        }
    }

   /* We are done with this request */
    HTRequest_delete(request);

    msDebug("terminate_handler()\n");

    /* Stop event loop once all requests are completed */
    if (bAllDone)
    {
        msDebug("terminate_handler stopping event loop\n");
        HTEventList_stopLoop();
    }

    return 0;
}

/**********************************************************************
 *                          msHTTPInitRequestObj()
 *
 * Should be called on a new array of httpRequestObj to initialize them
 * for use with mxHTTPExecuteRequest(), etc.
 *
 * Note that users of this module should always allocate and init one \
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
        pasReqInfo[i].request = NULL;
        pasReqInfo[i].pszGetUrl = NULL;
        pasReqInfo[i].pszOutputFile = NULL;
        pasReqInfo[i].nLayerId = 0;
        pasReqInfo[i].nStatus = 0;
        pasReqInfo[i].nTimeout = 0;
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

        pasReqInfo[i].request = NULL;
    }
}


/**********************************************************************
 *                          msHTTPExecuteRequests()
 *
 * Fetch a map slide via HTTP request and save to specified temp file.
 *
 * Based on sample code from http://www.w3.org/Library/Examples/LoadToFile.c
 **********************************************************************/
int msHTTPExecuteRequests(httpRequestObj *pasReqInfo, int numRequests)
{
    HTRequest *         request = NULL;
    int                 i, nStatus = MS_SUCCESS, nTimeout;

    if (numRequests == 0)
        return MS_SUCCESS;  /* Nothing to do */

    /* Initiate W3C Reference Library with a client profile */
    HTProfile_newNoCacheClient("MapServer WMS Client", "1.0");

    /* And the traces... */
#if 0
    HTSetTraceMessageMask("sop");
#endif

    /* Need our own trace and print functions */
    HTPrint_setCallback(tracer);
    HTTrace_setCallback(tracer);

    /* Add our own filter to terminate the application */
    HTNet_addAfter(terminate_handler, NULL, pasReqInfo, 
                   HT_ALL, HT_FILTER_LAST);

    /* Set the timeout for how long we are going to wait for a response 
     *  we use the longest timeout value in the array of requests 
     */
    nTimeout = pasReqInfo[0].nTimeout;
    for (i=1; i<numRequests; i++)
    {
        if (pasReqInfo[i].nTimeout > nTimeout)
            nTimeout = pasReqInfo[i].nTimeout;
    }

    if (nTimeout <= 0)
        nTimeout = 30000;
            
    HTHost_setEventTimeout(nTimeout);

    /* Somehow libwww defaults to route XML documents through some handler
     * or something... I'm not sure what it does, but downloading a file
     * with Content-type: text/xml always results in a 0-bytes file.
     * This addConversion() call will bypass the text/xml handler and
     * allow us to download text/xml files.
     *
     * This kind of magic is one more reason to get rid of libwww ASAP and
     * replace it with something lighter and simpler.
     */
    HTFormat_addConversion("text/xml", "*/*", HTThroughLine, 1.0, 0.0, 0.0);

    for (i=0; i<numRequests; i++)
    {
        if (pasReqInfo[i].pszGetUrl == NULL || 
            pasReqInfo[i].pszOutputFile == NULL)
        {
            msSetError(MS_WMSCONNERR, "URL or output file parameter missing.", 
                       "msHTTPExecuteRequests()");
            return(MS_FAILURE);
        }

        request = HTRequest_new();

        pasReqInfo[i].request = request;

        msDebug("WMS GET %s\n", pasReqInfo[i].pszGetUrl);

        /* Start the load */
        if (HTLoadToFile(pasReqInfo[i].pszGetUrl, request,
                         pasReqInfo[i].pszOutputFile) != YES) 
        {
            msSetError(MS_WMSCONNERR, "Can't open output file %s.", 
                       "msHTTPExecuteRequests()", pasReqInfo[i].pszOutputFile);
            HTProfile_delete();
            return(MS_FAILURE);
        }

    }
    msDebug("WMS: Before eventloop\n");

    /* Go into the event loop... */
    HTEventList_loop(request);
//    HTEventList_newLoop();

    msDebug("WMS: After eventloop\n");

    /* Check status of all requests and report errors */
    for (i=0; i<numRequests; i++)
    {
        if (pasReqInfo[i].nStatus != 200)
        {
            nStatus = MS_FAILURE;

            if (pasReqInfo[i].nStatus == HT_TIMEOUT)
            {
                msDebug("WMS: TIMEOUT of %d seconds execeeded for %s", 
                        nTimeout/1000, pasReqInfo[i].pszGetUrl );

                nStatus = MS_SUCCESS;  // Timeout isn't a fatal error
            }
            else
            {
                msDebug("WMS: HTTP GET request failed with status %d for %s",
                        pasReqInfo[i].nStatus, pasReqInfo[i].pszGetUrl);

                msSetError(MS_WMSCONNERR, 
                           "HTTP GET request failed with status %d for %s",
                           "msHTTPExecuteRequests()", pasReqInfo[i].nStatus, 
                           pasReqInfo[i].pszGetUrl);
            }
        }
    }

    /* cleanup */
    HTProfile_delete();

    return nStatus;
}

/**********************************************************************
 *                          msHTTPGetFile()
 *
 * Wrapper to call msHTTPExecuteRequests() for a single file.
 **********************************************************************/
int msHTTPGetFile(char *pszGetUrl, char *pszOutputFile, int *pnHTTPStatus,
                  int nTimeout)
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
    
    if (msHTTPExecuteRequests(pasReqInfo, 1) != MS_SUCCESS)
    {
        *pnHTTPStatus = pasReqInfo[0].nStatus;
        msDebug("WMS GET failed.\n", pszGetUrl);
        free(pszGetUrl);
        return MS_FAILURE;
    }

    *pnHTTPStatus = pasReqInfo[0].nStatus;

    msHTTPFreeRequestObj(pasReqInfo, 2);
    free(pasReqInfo);

    return MS_SUCCESS;
}


#endif /* defined(USE_WMS_LYR) || defined(USE_WMS_SVR) */
