/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGCAPI Implementation
 * Author:   Steve Lime and the MapServer team.
 *
 **********************************************************************
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ****************************************************************************/
#include "mapserver.h"
#include "mapogcapi.h"

#include "third-party/include_nlohmann_json.hpp"

#include <string>
#include <iostream>

using json = nlohmann::json;

#define JSON_MIMETYPE "application/json"
#define HTML_MIMETYPE "text/html"

#ifdef USE_OGCAPI_SVR

/*
** private functions
*/

/*
** Returns a JSON object using MapServer error codes and a description.
**   - should this be JSON only?
**   - should this rely on the msSetError() pipeline or be stand-alone?
*/
static void processError(int code, const char *description)
{
  json j;

  j["code"] = msGetErrorCodeString(code);
  j["description"] = description;

  msIO_setHeader("Content-Type", JSON_MIMETYPE);
  msIO_sendHeaders();
  msIO_printf("%s\n", j.dump().c_str());

  return;
}

/*
** Returns the value associated with an item from the request's query string and NULL if the item was not found.
*/
static const char *getRequestParamter(cgiRequestObj *request, const char *item)
{
  return NULL;
}

static int processLandingRequest(mapObj *map)
{
  processError(MS_OGCAPIERR, "Landing request support coming soon...");
  return MS_SUCCESS;
}

static int processConformanceRequest(mapObj *map)
{
  processError(MS_OGCAPIERR, "Conformance request support coming soon...");
  return MS_SUCCESS;
}

static int processCollectionsRequest(mapObj *map)
{
  processError(MS_OGCAPIERR, "Collections request support coming soon...");
  return MS_SUCCESS;
}

static int processHtmlTemplate() 
{
  return MS_SUCCESS;
}
#endif

int msOGCAPIDispatchRequest(mapObj *map, cgiRequestObj *request, char **api_path, int api_path_length)
{
#ifdef USE_OGCAPI_SVR
  if(api_path_length == 3) {
    return processLandingRequest(map);
  } else if(api_path_length == 4) {
    if(strcmp(api_path[3], "conformance") == 0)
      return processConformanceRequest(map);
    else if(strcmp(api_path[3], "collections") == 0)
      return processCollectionsRequest(map);
  }

  processError(MS_OGCAPIERR, "Invalid API request.");
  return MS_SUCCESS; // avoid any downstream MapServer processing
#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.", "msOGCAPIDispatchRequest()");
  return MS_FAILURE;
#endif
}
