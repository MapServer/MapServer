/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of WMS CONNECTIONTYPE - client to WMS servers
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2001, Daniel Morissette, DM Solutions Group Inc
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
 * Revision 1.1  2001/08/14 21:26:54  dan
 * Initial revision - only loads and draws WMS CONNECTION URL for now.
 *
 **********************************************************************/

#include "map.h"
#include "maperror.h"

#ifdef USE_WMS_LYR

/*
** Note: This code uses the w3c-libwww library to access remote WMS servers 
** via the HTTP protocol.  
** See http://www.w3.org/Library/ for the lib's source code and install docs.
** See http://www.w3.org/Library/Examples/LoadToFile.c for sample code on which
** big portions of msWMSGetImage was based.
*/

#include <WWWLib.h>
#include <WWWHTTP.h>
#include <WWWInit.h>

static int tracer (const char * fmt, va_list pArgs)
{
/* Do not write to stderr on windoze as this makes Apache hang. */
#ifndef _WIN32
    return (vfprintf(stderr, fmt, pArgs));
#endif
}


#ifndef PREEMPTIVE_MODE

/**********************************************************************
 *                          msWMSGetImage()
 *
 * Fetch a map slide via HTTP request and save to specified temp file.
 *
 * Based on sample code from http://www.w3.org/Library/Examples/LoadToFile.c
 **********************************************************************/
int msWMSGetImage(const char *geturl, const char *outputfile)
{
    HTRequest * request;

    if (geturl == NULL || outputfile == NULL)
    {
        msSetError(MS_MISCERR, "URL or output file parameter missing.", 
                   "msGetImage()");
        return(MS_FAILURE);
    }

    request = HTRequest_new();
    HTProfile_newPreemptiveClient("MapServer WMS Client", "1.0");

    HTPrint_setCallback(tracer);
    HTTrace_setCallback(tracer);

    HTLoadToFile(geturl, request, outputfile);

    HTRequest_delete(request);                  /* Delete the request object */
    HTProfile_delete();

    return MS_SUCCESS;
}


#else

/*
**  We get called here from the event loop when we are done
**  loading.
*/
static int terminate_handler (HTRequest * request, HTResponse * response,
                              void * param, int status)
{
    static char errmsg[100];

    /* Delete our request again */
    HTRequest_delete(request);

    /* Delete our profile */
    HTProfile_delete();

    if (status)
    {
        sprintf(errmsg, "WMS HTTP GET request failed with status %d", status);
        msSetError(MS_MISCERR, errmsg, "msGetImage()");
    }

    return status;
}


/**********************************************************************
 *                          msWMSGetImage()
 *
 * Fetch a map slide via HTTP request and save to specified temp file.
 *
 * Based on sample code from http://www.w3.org/Library/Examples/LoadToFile.c
 **********************************************************************/
int msWMSGetImage(const char *geturl, const char *outputfile)
{
    HTRequest *         request = NULL;

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
    HTNet_addAfter(terminate_handler, NULL, NULL, HT_ALL, HT_FILTER_LAST);

    /* Set the timeout for how long we are going to wait for a response */
    HTHost_setEventTimeout(25000);

    if (geturl == NULL || outputfile == NULL)
    {
        msSetError(MS_MISCERR, "URL or output file parameter missing.", 
                   "msGetImage()");
        return(MS_FAILURE);
    }

    request = HTRequest_new();

    /* Start the load */
    if (HTLoadToFile(geturl, request, outputfile) != YES) 
    {
        msSetError(MS_MISCERR, "Can't open output file.", "msGetImage()");
        HTProfile_delete();
        return(MS_FAILURE);
    }

    /* Go into the event loop... */
    HTEventList_loop(request);

    /* If load failed then msSetError() will have been called */
   return (ms_error.code == 0) ? MS_SUCCESS : MS_FAILURE;
}


#endif /* USE_WMS_LYR */

#endif /* PREEMPTIVE_MODE */


/**********************************************************************
 *                          msDrawWMSLayer()
 *
 **********************************************************************/

int msDrawWMSLayer(mapObj *map, layerObj *lp, gdImagePtr img) 
{
#ifdef USE_WMS_LYR
    if (lp->connectiontype != MS_WMS || lp->connection == NULL)
        return MS_FAILURE;

    if (msWMSGetImage(lp->connection, "/tmp/ttt.gif") != MS_SUCCESS)
        return MS_FAILURE;

    lp->type = MS_LAYER_RASTER;
    if (lp->data) free(lp->data);
    lp->data = strdup("/tmp/ttt.gif");
    lp->transform = MS_FALSE;

    msDrawRasterLayer(map, lp, img);

    return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * WMS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_MISCERR, "WMS CLIENT CONNECTION support is not available.", 
             "msDrawWMSLayer()");
  return(MS_FAILURE);

#endif /* USE_WMS_LYR */

}


