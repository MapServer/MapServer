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

#define OGCAPI_DEFAULT_TITLE "MapServer OGC API"

#define OGCAPI_TEMPLATE_HTML_LANDING "landing.html"
#define OGCAPI_TEMPLATE_HTML_CONFORMANCE "conformance.html"
#define OGCAPI_TEMPLATE_HTML_COLLECTION "collection.html"
#define OGCAPI_TEMPLATE_HTML_COLLECTIONS "collections.html"

#define OGCAPI_FORMAT_JSON 1
#define OGCAPI_FORMAT_GEOJSON 2
#define OGCAPI_FORMAT_HTML 3

#define OGCAPI_MIMETYPE_JSON "application/json"
#define OGCAPI_MIMETYPE_GEOJSON "application/geo+json"
#define OGCAPI_MIMETYPE_HTML "text/html"

#ifdef USE_OGCAPI_SVR

/* prototypes */
static void processError(int code, const char *description);

/*
** Get stuff...
*/

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

/*
** Returns the template directory location or NULL if it isn't set.
*/
static const char *getTemplateDirectory(mapObj *map)
{
  const char *directory;

  // TODO: if directory is provided then perhaps we need to check for a trailing slash

  if((directory = msOWSLookupMetadata(&(map->web.metadata), "AO", "template_directory")) != NULL) 
    return directory;
  else if((directory = getenv("OGCAPI_TEMPLATE_DIRECTORY")) != NULL)
    return directory;
  else
    return NULL;
}

/*
** Returns the service title from oga|ows_title or a default value if not set.
*/
static const char *getTitle(mapObj *map)
{
  const char *title;

  if((title = msOWSLookupMetadata(&(map->web.metadata), "AO", "title")) != NULL)
    return title;
  else
    return OGCAPI_DEFAULT_TITLE;
}

/*
** Returns the API root URL from oga_onlineresource or builds a value if not set.
*/
std::string getApiRootUrl(mapObj *map)
{
  const char *root;

  if((root = msOWSLookupMetadata(&(map->web.metadata), "A", "onlineresource")) != NULL)
    return std::string(root);
  else
    return "http://" + std::string(getenv("SERVER_NAME")) + ":" + std::string(getenv("SERVER_PORT")) + std::string(getenv("SCRIPT_NAME")) + std::string(getenv("PATH_INFO"));
}

/*
** Output stuff...
*/

static void outputJson(json j) 
{
  msIO_setHeader("Content-Type", OGCAPI_MIMETYPE_JSON);
  msIO_sendHeaders();
  msIO_printf("%s\n", j.dump().c_str());
}

static void outputGeoJson(json j) 
{
  msIO_setHeader("Content-Type", OGCAPI_MIMETYPE_GEOJSON);
  msIO_sendHeaders();
  msIO_printf("%s\n", j.dump().c_str());
}

static void outputHtml(const char *directory, const char *filename, json j)
{
  std::string _directory(directory);
  std::string _filename(filename);
  Environment env {_directory}; // catch

  // ERB-style instead of Mustache (we'll see)
  env.set_expression("<%=", "%>");
  env.set_statement("<%", "%>");

  Template t = env.parse_template(_filename); // catch
  std::string result = env.render(t, j);

  msIO_setHeader("Content-Type", OGCAPI_MIMETYPE_HTML);
  msIO_sendHeaders();
  msIO_printf("%s\n", result.c_str()); 
}

/*
** Generic response outputr.
*/
static void outputResponse(mapObj *map, int format, const char *filename, json j)
{
  const char *directory = NULL;

  if(format == OGCAPI_FORMAT_JSON) {
    outputJson(j);
  } else if(format == OGCAPI_FORMAT_GEOJSON) {
    outputGeoJson(j);
  } else if(format == OGCAPI_FORMAT_HTML) {
    if((directory = getTemplateDirectory(map)) == NULL) {
      processError(400, "Template directory not set.");
      return; // bail
    }

    // extend the JSON with a few things that we need for templating
    j["template"] = {
      { "path", msStringSplit(getenv("PATH_INFO"), '/') },
      { "api_root", getApiRootUrl(map) },
      { "title", getTitle(map) },
    };

    outputHtml(directory, filename, j);
  } else {
    processError(400, "Unsupported format requested.");
  }
}

/*
** Process stuff...
*/

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

  outputJson(j);
}

static int processLandingRequest(mapObj *map, int format)
{
  json j;

  // define ambiguous elements
  const char *description = msOWSLookupMetadata(&(map->web.metadata), "AO", "description");
  if(!description) description = msOWSLookupMetadata(&(map->web.metadata), "AO", "abstract"); // fallback on abstract if necessary

  // define api root url
  std::string api_root = getApiRootUrl(map);

  // build response object
  j =  {
    { "title", getTitle(map) },
    { "description", description?description:"" },
    { "links", {
        {
	  { "rel", format==OGCAPI_FORMAT_JSON?"self":"alternate" },
	  { "type", OGCAPI_MIMETYPE_JSON },
	  { "title", "This document as JSON." },
	  { "href", api_root + "?f=json" }
        },{
	  { "rel", format==OGCAPI_FORMAT_HTML?"self":"alternate" },
	  { "type", OGCAPI_MIMETYPE_HTML },
	  { "title", "This document as HTML." },
	  { "href", api_root + "?f=html" }
        },{
          { "rel", "conformance" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "OCG API conformance classes implemented by this server (JSON)." },
          { "href", api_root + "/conformance?f=json" }
        },{
          { "rel", "conformance" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "OCG API conformance classes implemented by this server." },
          { "href", api_root + "/conformance?f=html" }
        },{
          { "rel", "data" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "Information about feature collections available from this server (JSON)." },
          { "href", api_root + "/collections?f=json" }
        },{
          { "rel", "data" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "Information about feature collections available from this server." },
          { "href", api_root + "/collections?f=html" }
        }
      }
    }
  };

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_LANDING, j);
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

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_CONFORMANCE, j);
  return MS_SUCCESS;
}

static int processCollectionsRequest(mapObj *map, int format)
{
  json j;

  // build response object
  j = {
    // here
  };

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_COLLECTIONS, j);
  return MS_SUCCESS;
}
#endif

int msOGCAPIDispatchRequest(mapObj *map, cgiRequestObj *request, char **api_path, int api_path_length)
{
  int format;
  const char *f;

  f = getRequestParameter(request, "f");
  if(f && (strcmp(f, "json") == 0 || strcmp(f, OGCAPI_MIMETYPE_JSON) == 0)) {
    format = OGCAPI_FORMAT_JSON;
  } else if(f && (strcmp(f, "html") == 0 || strcmp(f, OGCAPI_MIMETYPE_HTML) == 0)) {
    format = OGCAPI_FORMAT_HTML;
  } else if(f) {
    processError(500, "Unsupported format requested.");
    return MS_SUCCESS; // avoid any downstream MapServer processing
  } else {
    format = OGCAPI_FORMAT_HTML; // default for now, need to derive from http headers (possible w/CGI?)
  }

#ifdef USE_OGCAPI_SVR
  if(api_path_length == 3) {
    return processLandingRequest(map, format);
  } else if(api_path_length == 4) {
    if(strcmp(api_path[3], "conformance") == 0) {
      return processConformanceRequest(map, format);
    } else if(strcmp(api_path[3], "conformance.html") == 0) {
      return processConformanceRequest(map, OGCAPI_FORMAT_HTML);
    } else if(strcmp(api_path[3], "collections") == 0) {
      return processCollectionsRequest(map, format);
    } else if(strcmp(api_path[3], "collections.html") == 0) {
      return processCollectionsRequest(map, OGCAPI_FORMAT_HTML);
    }
  }

  processError(500, "Invalid API request."); 
  return MS_SUCCESS; // avoid any downstream MapServer processing
#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.", "msOGCAPIDispatchRequest()");
  return MS_FAILURE;
#endif
}
