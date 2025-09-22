/**********************************************************************
 *
 * Project:  MapServer
 * Purpose:  MapServer Index Page Implementation
 * Author:   Seth Girvin and the MapServer team.
 *
 **********************************************************************
 * Copyright (c) 1996-2025 Regents of the University of Minnesota.
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
#include "mapserv-index.h"
#include "maptemplate.h"
#include "mapows.h"
#include "mapserv-config.h"

#include "cpl_conv.h"

#include "third-party/include_nlohmann_json.hpp"
#include "third-party/include_pantor_inja.hpp"

using namespace inja;
using json = nlohmann::json;

/*
** HTML Templates
*/
#define TEMPLATE_HTML_INDEX "landing.html"
#define TEMPLATE_HTML_MAP_INDEX "map.html"

/**
 * Get an online resource using the supplied namespaces
 */
std::string getOnlineResource(mapObj *map, cgiRequestObj *request,
                              const std::string &namespaces) {
  std::string onlineResource;

  if (char *res = msOWSGetOnlineResource(map, namespaces.c_str(),
                                         "onlineresource", request)) {
    onlineResource = res;
    free(res);
  } else {
    // fallback: use a relative URL if no onlineresource can be found or created
    onlineResource = "./?";
  }

  if (!onlineResource.empty() && onlineResource.back() != '?') {
    onlineResource += '?';
  }

  return onlineResource;
}

static json processCGI(mapObj *map, cgiRequestObj *request) {

  json response;

  // check if CGI modes have been disabled
  const char *enable_modes =
      msLookupHashTable(&(map->web.metadata), "ms_enable_modes");

  int disabled = MS_FALSE;
  msOWSParseRequestMetadata(enable_modes, "BROWSE", &disabled);

  if (disabled == MS_FALSE) {
    response["title"] = map->name;

    std::string onlineResource = getOnlineResource(map, request, "MCFGOA");

    json serviceDocs = json::array(
        {{{"href",
           onlineResource + "template=openlayers&mode=browse&layers=all"},
          {"title", "OpenLayers Viewer"},
          {"service", "CGI"},
          {"type", "text/html"}}});
    response["service-doc"] = serviceDocs;
  }

  return response;
}

static json processOGCAPI(mapObj *map, cgiRequestObj *request) {

  json response;

  const char *title = msOWSLookupMetadata(&(map->web.metadata), "AO", "title");
  if (!title) {
    title = map->name;
  }

  int status = msOWSRequestIsEnabled(map, NULL, "AO", "OGCAPI", MS_FALSE);
  if (status == MS_TRUE) {
    response["title"] = title;

    std::string onlineResource = getApiRootUrl(map, request, "AO");

    // strip any trailing slash as we add it back manually in the hrefs
    if (!onlineResource.empty() && onlineResource.back() == '/') {
      onlineResource.pop_back();
    }

    json serviceDescriptions =
        json::array({{{"href", onlineResource + "/?f=json"},
                      {"title", "OGC API Root Service"},
                      {"service", "OGC API"},
                      {"type", "application/json"}}});
    json serviceDocs = json::array({{{"href", onlineResource + "/?f=html"},
                                     {"title", "OGC API Landing Page"},
                                     {"service", "OGC API"},
                                     {"type", "text/html"}}});
    response["service-desc"] = serviceDescriptions;
    response["service-doc"] = serviceDocs;
  }
  return response;
}

static json processWMS(mapObj *map, cgiRequestObj *request) {

  json response;

  const char *title = msOWSLookupMetadata(&(map->web.metadata), "MO", "title");
  if (!title) {
    title = map->name;
  }

  int globally_enabled = MS_FALSE;
  int disabled = MS_FALSE;

  const char *enable_request =
      msOWSLookupMetadata(&map->web.metadata, "MO", "enable_request");

  globally_enabled =
      msOWSParseRequestMetadata(enable_request, "GetCapabilities", &disabled);

  if (globally_enabled == MS_TRUE) {
    response["title"] = title;

    std::string onlineResource = getOnlineResource(map, request, "MO");

    std::vector<std::string> versions = {"1.0.0", "1.1.0", "1.1.1", "1.3.0"};
    json serviceDescriptions = json::array();

    for (const auto &ver : versions) {
      serviceDescriptions.push_back(
          {{"href", std::string(onlineResource) + "version=" + ver +
                        "&request=GetCapabilities&service=WMS"},
           {"title", "WMS GetCapabilities URL (version " + ver + ")"},
           {"service", "WMS"},
           {"type", "text/xml"}});
    }
    response["service-desc"] = serviceDescriptions;
  }

  return response;
}

static json processWFS(mapObj *map, cgiRequestObj *request) {

  json response;

  const char *title = msOWSLookupMetadata(&(map->web.metadata), "FO", "title");
  if (!title) {
    title = map->name;
  }

  int globally_enabled = MS_FALSE;
  int disabled = MS_FALSE;

  const char *enable_request =
      msOWSLookupMetadata(&map->web.metadata, "FO", "enable_request");

  globally_enabled =
      msOWSParseRequestMetadata(enable_request, "GetCapabilities", &disabled);

  if (globally_enabled == MS_TRUE) {
    response["title"] = title;

    std::string onlineResource = getOnlineResource(map, request, "MO");

    std::vector<std::string> versions = {"1.0.0", "1.1.0", "2.0.0"};
    json serviceDescriptions = json::array();

    for (const auto &ver : versions) {
      serviceDescriptions.push_back(
          {{"href", onlineResource + "version=" + ver +
                        "&request=GetCapabilities&service=WFS"},
           {"title", "WFS GetCapabilities URL (version " + ver + ")"},
           {"service", "WFS"},
           {"type", "text/xml"}});
    }
    response["service-desc"] = serviceDescriptions;
  }

  return response;
}

static json processWCS(mapObj *map, cgiRequestObj *request) {

  json response;

  const char *title = msOWSLookupMetadata(&(map->web.metadata), "CO", "title");
  if (!title) {
    title = map->name;
  }

  int globally_enabled = MS_FALSE;
  int disabled = MS_FALSE;

  const char *enable_request =
      msOWSLookupMetadata(&map->web.metadata, "CO", "enable_request");

  globally_enabled =
      msOWSParseRequestMetadata(enable_request, "GetCapabilities", &disabled);

  if (globally_enabled == MS_TRUE) {
    response["title"] = title;

    std::string onlineResource = getOnlineResource(map, request, "CO");

    std::vector<std::string> versions = {"1.0.0", "1.1.0", "2.0.0", "2.0.1"};
    json serviceDescriptions = json::array();

    for (auto &ver : versions) {
      serviceDescriptions.push_back(
          {{"href", onlineResource + "version=" + ver +
                        "&request=GetCapabilities&service=WCS"},
           {"title", "WCS GetCapabilities URL (version " + ver + ")"},
           {"service", "WCS"},
           {"type", "text/xml"}});
    }
    response["service-desc"] = serviceDescriptions;
  }
  return response;
}

static json createMapDetails(mapObj *map, cgiRequestObj *request) {

  json links = json::array();
  json result;

  result = processCGI(map, request);
  if (!result.empty()) {
    links.push_back(result);
  }

#ifdef USE_OGCAPI_SVR
  result = processOGCAPI(map, request);
  if (!result.empty()) {
    links.push_back(result);
  }
#endif
#ifdef USE_WMS_SVR
  result = processWMS(map, request);
  if (!result.empty()) {
    links.push_back(result);
  }
#endif
#ifdef USE_WFS_SVR
  result = processWFS(map, request);
  if (!result.empty()) {
    links.push_back(result);
  }
#endif
#ifdef USE_WCS_SVR
  result = processWCS(map, request);
  if (!result.empty()) {
    links.push_back(result);
  }
#endif

  return {{"linkset", links}};
}

/**
 * Create a JSON object with a summary of the Mapfile
 **/
static json createMapSummary(mapObj *map, const char *key,
                             cgiRequestObj *request) {
  json mapJson;
  const char *value =
      msOWSLookupMetadata(&(map->web.metadata), "MCFGOA", "title");

  mapJson["key"] = key;
  mapJson["has-error"] = false;

  if (value) {
    mapJson["title"] = value;
  } else {
    mapJson["title"] = map->name;
  }

  mapJson["layer-count"] = map->numlayers;
  mapJson["name"] = map->name;

  std::string onlineResource;

  if (char *res = msBuildOnlineResource(NULL, request)) {
    onlineResource = res;
    free(res);

    // remove trailing '?' if present
    if (!onlineResource.empty() && onlineResource.back() == '?') {
      onlineResource.pop_back();
    }
  } else {
    // use a relative URL if no onlineresource cannot be created
    onlineResource = "./"; // fallback
  }

  mapJson["service-desc"] =
      json::array({{{"href", onlineResource + std::string(key) + "/?f=json"},
                    {"title", key},
                    {"type", "application/vnd.oai.openapi+json"}}});

  mapJson["service-doc"] =
      json::array({{{"href", onlineResource + std::string(key) + "/"},
                    {"title", key},
                    {"type", "text/html"}}});

  return mapJson;
}

/**
 * For invalid maps return a JSON object reporting the error
 **/
static json createMapError(const char *key) {
  json mapJson;
  mapJson["key"] = key;
  mapJson["has-error"] = true;
  mapJson["title"] = " <b>Error loading the map</b>";
  mapJson["layer-count"] = 0;
  mapJson["name"] = key;
  mapJson["service-desc"] =
      json::array({{{"href", "./" + std::string(key) + "/?f=json"},
                    {"title", key},
                    {"type", "application/vnd.oai.openapi+json"}}});

  mapJson["service-doc"] =
      json::array({{{"href", "./" + std::string(key) + "/"},
                    {"title", key},
                    {"type", "text/html"}}});

  return mapJson;
}

/**
 * Load the Map for the key, by getting its file path
 * from the CONFIG file
 */
static mapObj *getMapFromConfig(configObj *config, const char *key) {
  const char *mapfilePath = msLookupHashTable(&config->maps, key);

  if (!mapfilePath)
    return nullptr;

  char pathBuf[MS_MAXPATHLEN];
  mapfilePath = msConfigGetMap(config, key, pathBuf);

  return msLoadMap(mapfilePath, nullptr, config);
}

/**
 * Generate HTML by populating a template with the dynamic JSON
 */
static int createHTMLOutput(json response, const char *templateName) {
  std::string path;
  char fullpath[MS_MAXPATHLEN];

  path = getTemplateDirectory(NULL, "html_template_directory",
                              "MS_INDEX_TEMPLATE_DIRECTORY");
  if (path.empty()) {
    outputError(OGCAPI_CONFIG_ERROR, "Template directory not set.");
    return MS_FAILURE;
  }
  msBuildPath(fullpath, NULL, path.c_str());

  json j = {{"response", response}};
  outputTemplate(fullpath, templateName, j, OGCAPI_MIMETYPE_HTML);

  return MS_SUCCESS;
}

/**
 * Return an individual map landing page response
 */
int msOGCAPIDispatchMapIndexRequest(mapservObj *mapserv, configObj *config) {
#ifdef USE_OGCAPI_SVR
  if (!mapserv || !config)
    return -1; // Handle null pointers

  cgiRequestObj *request = mapserv->request;

  if (request->api_path_length != 1) {
    const char *pathInfo = getenv("PATH_INFO");
    msSetError(MS_OGCAPIERR, "Invalid PATH_INFO format: \"%s\"",
               "msOGCAPIDispatchMapIndexRequest()", pathInfo);
    return MS_FAILURE;
  }

  OGCAPIFormat format = msGetOutputFormat(request);
  const char *key = request->api_path[0];

  mapObj *map = getMapFromConfig(config, key);
  json response = createMapDetails(map, request);

  // add in summary map details
  json summary = createMapSummary(map, key, request);

  // remove unwanted properties, before merging objects
  summary.erase("service-desc");
  summary.erase("service-doc");

  response.update(summary);

  if (format == OGCAPIFormat::JSON) {
    outputJson(response, OGCAPI_MIMETYPE_JSON, {});
  } else if (format == OGCAPIFormat::HTML) {
    createHTMLOutput(response, TEMPLATE_HTML_MAP_INDEX);
  } else {
    outputError(OGCAPI_PARAM_ERROR, "Unsupported format requested.");
  }

  return MS_SUCCESS;

#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.",
             "msOGCAPIDispatchMapIndexRequest()");
  return MS_FAILURE;
#endif
}

/**
 * Return the MapServer landing page response
 */
int msOGCAPIDispatchIndexRequest(mapservObj *mapserv, configObj *config) {
#ifdef USE_OGCAPI_SVR
  if (!mapserv || !config)
    return -1; // Handle null pointers

  cgiRequestObj *request = mapserv->request;
  OGCAPIFormat format;
  format = msGetOutputFormat(request);

  const char *key = NULL;
  json links = json::array();

  while ((key = msNextKeyFromHashTable(&config->maps, key)) != NULL) {
    if (mapObj *map = getMapFromConfig(config, key)) {
      links.push_back(createMapSummary(map, key, request));
      msFreeMap(map);
    } else {
      // there was a problem loading the map
      links.push_back(createMapError(key));
    }
  }

  json response = {{"linkset", links}};

  if (format == OGCAPIFormat::JSON) {
    outputJson(response, OGCAPI_MIMETYPE_JSON, {});
  } else if (format == OGCAPIFormat::HTML) {
    createHTMLOutput(response, TEMPLATE_HTML_INDEX);
  } else {
    outputError(OGCAPI_PARAM_ERROR, "Unsupported format requested.");
  }

  return MS_SUCCESS;

#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.",
             "msOGCAPIDispatchIndexRequest()");
  return MS_FAILURE;
#endif
}
