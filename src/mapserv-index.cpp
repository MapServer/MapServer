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

#include "cpl_conv.h"

#include "third-party/include_nlohmann_json.hpp"
#include "third-party/include_pantor_inja.hpp"

using namespace inja;
using json = nlohmann::json;

/*
** HTML Templates
*/
#define TEMPLATE_HTML_INDEX "landing.html"

static std::vector<json> processWMS(mapObj *map) {

  std::vector<json> serviceDescs; // List of JSON objects

  // Create the first JSON object
  json serviceDesc1;
  const char *title = msOWSLookupMetadata(&(map->web.metadata), "MO", "title");
  if (!title) {
    title = map->name;
  }

  std::string api_root = getApiRootUrl(map, "MO");

  serviceDesc1["title"] = title;
  serviceDesc1["type"] = "WMS";
  serviceDesc1["url"] = api_root;

  serviceDescs.push_back(serviceDesc1); // Add to the list

  json serviceDesc2;
  serviceDesc2["title"] = title;
  serviceDesc2["type"] = "WMS";
  serviceDesc1["url"] = api_root;

  serviceDescs.push_back(serviceDesc2); // Add to the list

  return serviceDescs; // Return the list of JSON objects
}

static json processMap(mapObj *map) {
  json mapJson;
  mapJson["name"] = map->name;
  mapJson["title"] = "title";
  // object getTitle(map)

  // service-meta intended for machine consumption
  //
  // Create the "service-desc" array
  mapJson["service-desc"] = json::array();

  // json serviceDesc;
  // serviceDesc["href"] =
  //     "https://demo.mapserver.org/cgi-bin/"
  //     "msautotest?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities";
  // serviceDesc["title"] = "World WMS service";
  // serviceDesc["type"] = "text/xml";

#ifdef USE_WMS_SVR

  // WMS services
  std::vector<json> wmsServiceDescs = processWMS(map);

  // Add each JSON object to the "service-desc" array
  for (const auto &serviceDesc : wmsServiceDescs) {
    mapJson["service-desc"].push_back(serviceDesc);
  }

#endif /* USE_WMS_SVR */

  // Append the nested object to the "service-desc" array
  // mapJson["service-desc"].push_back(serviceDesc);

  return mapJson;
}

static json getMapJsonFromConfig(configObj *config) {

  const char *key = NULL;
  json links =
      json::array(); // Create an empty JSON array to store nested objects

  while (true) {
    key = msNextKeyFromHashTable(&config->maps, key);
    if (!key) {
      break; // No more keys
    }

    const char *pszValue = msLookupHashTable(&config->maps, key);
    if (pszValue) {
      mapObj *map = msLoadMap(pszValue, nullptr, config);
      if (!map) {
        // return -1; // Handle error case
        // try paths relative to the CONFIG file
        // TODO - do we want this apart from to make it easier for testing?
        const char *val;
        if ((val = CPLGetConfigOption("MAPSERVER_CONFIG_FILE", NULL)) != NULL) {
          char szPath[MS_MAXPATHLEN];
          char *path = msGetPath(val);
          if (path) {
            msBuildPath(szPath, path, pszValue);
            free(path); // Ensure msGetPath and free are compatible
            map = msLoadMap(szPath, nullptr, config);
          }
        }
      }
      if (!map) {
        // log error output
      } else {
        // Append the new JSON object to the "links" array
        links.push_back(processMap(map));
        msFreeMap(map);
      }
    }
  }

  return links;
}

static std::string getEnvVar(const char *envVar) {
  const char *value = getenv(envVar);
  return value ? std::string(value) : std::string();
}

static int processIndexRequest(configObj *config, OGCAPIFormat format) {
  json response;
  std::map<std::string, std::string> extraHeaders;
  std::string path;
  char fullpath[MS_MAXPATHLEN];

  response = {{"title", "Index Page"},
              {"description", "List of Mapfiles"},
              {"linkset", getMapJsonFromConfig(config)}};

  if (format == OGCAPIFormat::JSON) {
    outputJson(response, OGCAPI_MIMETYPE_JSON, extraHeaders);
  } else if (format == OGCAPIFormat::GeoJSON) {
    outputJson(response, OGCAPI_MIMETYPE_GEOJSON, extraHeaders);
  } else if (format == OGCAPIFormat::OpenAPI_V3) {
    outputJson(response, OGCAPI_MIMETYPE_OPENAPI_V3, extraHeaders);
  } else if (format == OGCAPIFormat::HTML) {
    path = getTemplateDirectory(NULL, "html_template_directory",
                                "MS_INDEX_TEMPLATE_DIRECTORY");
    if (path.empty()) {
      outputError(OGCAPI_CONFIG_ERROR, "Template directory not set.");
      return MS_FAILURE;
    }
    msBuildPath(fullpath, NULL, path.c_str());

    json j;
    j["response"] = response;

    // std::string api_root = "http://" + std::string(getenv("SERVER_NAME")) +
    //                        ":" + std::string(getenv("SERVER_PORT")) +
    //                        std::string(getenv("SCRIPT_NAME")) +
    //                        std::string(getenv("PATH_INFO"));

    std::string serverName = getEnvVar("SERVER_NAME");
    std::string serverPort = getEnvVar("SERVER_PORT");
    std::string scriptName = getEnvVar("SCRIPT_NAME");
    std::string pathInfo = getEnvVar("PATH_INFO");

    std::string api_root =
        "http://" + serverName + ":" + serverPort + scriptName + pathInfo;

    // extend the JSON with a few things that we need for templating
    j["template"] = {{"path", json::array()},
                     {"params", json::object()},
                     {"api_root", api_root},
                     {"title", "MapServer Index Page"},
                     {"tags", json::object()}};

    outputTemplate(fullpath, TEMPLATE_HTML_INDEX, j, OGCAPI_MIMETYPE_HTML);
  }

  return MS_SUCCESS;
}

int msOGCAPIDispatchIndexRequest(mapservObj *mapserv, configObj *config) {
#ifdef USE_OGCAPI_SVR
  if (!mapserv || !config)
    return -1; // Handle null pointers

  cgiRequestObj *request = mapserv->request;
  OGCAPIFormat format;
  format = msGetOutputFormat(request);

  return processIndexRequest(config, format);

#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.",
             "msOGCAPIDispatchIndexRequest()");
  return MS_FAILURE;
#endif
}
