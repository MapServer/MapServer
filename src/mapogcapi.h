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

#ifndef MAPOGCAPI_H
#define MAPOGCAPI_H

#ifdef __cplusplus
extern "C" {
#endif

int msOGCAPIDispatchRequest(mapObj *map, cgiRequestObj *request);

#ifdef __cplusplus
}

#include <string>
#include <map>
#include "third-party/include_nlohmann_json.hpp"
#include "third-party/include_pantor_inja.hpp"

// Error types
typedef enum {
  OGCAPI_SERVER_ERROR = 0,
  OGCAPI_CONFIG_ERROR = 1,
  OGCAPI_PARAM_ERROR = 2,
  OGCAPI_NOT_FOUND_ERROR = 3,
} OGCAPIErrorType;

enum class OGCAPIFormat {
  JSON,
  GeoJSON,
  OpenAPI_V3,
  JSONSchema,
  HTML,
  Invalid
};

#define OGCAPI_MIMETYPE_JSON "application/json"
#define OGCAPI_MIMETYPE_GEOJSON "application/geo+json"
#define OGCAPI_MIMETYPE_OPENAPI_V3                                             \
  "application/vnd.oai.openapi+json;version=3.0"
#define OGCAPI_MIMETYPE_JSON_SCHEMA "application/schema+json"
#define OGCAPI_MIMETYPE_HTML "text/html"

std::string msOGCAPIGetTemplateDirectory(const mapObj *map, const char *key,
                                         const char *envvar);

OGCAPIFormat msOGCAPIGetOutputFormat(const cgiRequestObj *request);

std::string msOGCAPIGetApiRootUrl(const mapObj *map,
                                  const cgiRequestObj *request,
                                  const char *namespaces = "AO");

void msOGCAPIOutputError(OGCAPIErrorType errorType,
                         const std::string &description);

void msOGCAPIOutputJson(
    const nlohmann::json &j, const char *mimetype,
    const std::map<std::string, std::vector<std::string>> &extraHeaders);

void msOGCAPIOutputTemplate(const char *directory, const char *filename,
                            const nlohmann::json &j, const char *mimetype);

#endif

#endif
