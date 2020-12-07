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
#include "mapows.h"

#include "third-party/include_nlohmann_json.hpp"
#include "third-party/include_pantor_inja.hpp"

#include <string>
#include <iostream>

using namespace inja;
using json = nlohmann::json;

#define OGCAPI_TEMPLATE_HTML_LANDING "landing.html"
#define OCGAPI_TEMPLATE_HTML_CONFORMANCE "conformance.html"
#define OCGAPI_TEMPLATE_HTML_COLLECTIONS "collections.html"

#define OGCAPI_FORMAT_JSON 1
#define OGCAPI_FORMAT_HTML 2

#define OGCAPI_MIMETYPE_JSON "application/json"
#define OGCAPI_MIMETYPE_HTML "text/html"

#ifdef USE_OGCAPI_SVR

/*
** private functions
*/

static void writeJson(json j) 
{
  msIO_setHeader("Content-Type", OGCAPI_MIMETYPE_JSON);
  msIO_sendHeaders();
  msIO_printf("%s\n", j.dump().c_str());
}

static void writeHtml(const char *path, const char *filename, json j)
{
  std::string _path(path);
  std::string _filename(filename);
  Environment env {_path};

  // ERB-style instead of Mustache (we'll see)
  env.set_expression("<%=", "%>");
  env.set_statement("<%", "%>");

  // extend the JSON with a few things that the we need for templating
  j["request"] = {
    { "route", getenv("PATH_INFO") },
    { "host", getenv("HTTP_HOST") },
    { "port", getenv("SERVER_PORT") }
  };

  Template t = env.parse_template(_filename);
  std::string result = env.render(t, j);

  msIO_setHeader("Content-Type", OGCAPI_MIMETYPE_HTML);
  msIO_sendHeaders();
  msIO_printf("%s\n", result.c_str()); 
}

/*
** Returns a JSON object using MapServer error codes and a description.
**   - should this be JSON only?
**   - should this rely on the msSetError() pipeline or be stand-alone?
*/
static void processError(int code, const char *description)
{
  json j;

  j["code"] = code;
  j["description"] = description;

  writeJson(j);
  return;
}

/*
** Returns the value associated with an item from the request's query string and NULL if the item was not found.
*/
static const char *getRequestParameter(cgiRequestObj *request, const char *item)
{
  int i;

  for(i=0; i<request->NumParams; i++) {
    if(strcmp(item, request->ParamNames[i]) == 0)
      return request->ParamValues[i];
  }

  return NULL;
}

static const char *getTemplateDirectory(mapObj *map)
{
  const char *directory;

  // TODO: if directory is provided then perhaps we need to check for a trailing slash

  if((directory = msOWSLookupMetadata(&(map->web.metadata), "AO","template_directory")) != NULL) 
    return directory;
  else if((directory = getenv("OGCAPI_TEMPLATE_DIRECTORY")) != NULL)
    return directory;
  else
    return NULL;
}

static int processLandingRequest(mapObj *map)
{
  processError(400, "Landing request support coming soon...");
  return MS_SUCCESS;
}

static int processConformanceRequest(mapObj *map, int format)
{
  json j;
 
  // build response object
  j = {
    { "conformsTo", {
        "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
        "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections"
      }
    }
  };

  // outout response
  if(format == OGCAPI_FORMAT_JSON) {
    writeJson(j);
  } else if(format == OGCAPI_FORMAT_HTML) {
    writeHtml(getTemplateDirectory(map), OCGAPI_TEMPLATE_HTML_CONFORMANCE, j);
  } else {
    processError(400, "Unsupported format requested.");
  }

  return MS_SUCCESS;
}

static int processCollectionsRequest(mapObj *map)
{
  processError(400, "Collections request support coming soon...");
  return MS_SUCCESS;
}
#endif

int msOGCAPIDispatchRequest(mapObj *map, cgiRequestObj *request, char **api_path, int api_path_length)
{
  int format;
  const char *f;

  f = getRequestParameter(request, "f");
  if(f && (strcmp(f, "json") == 0 || strcmp(f, OGCAPI_MIMETYPE_JSON) == 0))
    format = OGCAPI_FORMAT_JSON;
  else if(f && (strcmp(f, "html") == 0 || strcmp(f, OGCAPI_MIMETYPE_HTML) == 0))
    format = OGCAPI_FORMAT_HTML;
  else if(f)
    processError(500, "Unsupported format requested.");
  else {
    format = OGCAPI_FORMAT_JSON; // default for now, need to derive from http headers
  }

#ifdef USE_OGCAPI_SVR
  if(api_path_length == 3) {
    return processLandingRequest(map);
  } else if(api_path_length == 4) {
    if(strcmp(api_path[3], "conformance") == 0) {
      return processConformanceRequest(map, format);
    } else if(strcmp(api_path[3], "conformance.html") == 0) {
      return processConformanceRequest(map, OGCAPI_FORMAT_HTML);
    } else if(strcmp(api_path[3], "collections") == 0) {
      return processCollectionsRequest(map);
    }
  }

  processError(500, "Invalid API request.");
  return MS_SUCCESS; // avoid any downstream MapServer processing
#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.", "msOGCAPIDispatchRequest()");
  return MS_FAILURE;
#endif
}
