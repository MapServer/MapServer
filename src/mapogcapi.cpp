/**********************************************************************
 * $id$
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
#include "mapgml.h"
#include "maptime.h"
#include "mapogcfilter.h"

#include "cpl_conv.h"

#include "third-party/include_nlohmann_json.hpp"
#include "third-party/include_pantor_inja.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <utility>

using namespace inja;
using json = nlohmann::json;

#define OGCAPI_DEFAULT_TITLE "MapServer OGC API"

/*
** HTML Templates
*/
#define OGCAPI_TEMPLATE_HTML_LANDING "landing.html"
#define OGCAPI_TEMPLATE_HTML_CONFORMANCE "conformance.html"
#define OGCAPI_TEMPLATE_HTML_COLLECTION "collection.html"
#define OGCAPI_TEMPLATE_HTML_COLLECTIONS "collections.html"
#define OGCAPI_TEMPLATE_HTML_COLLECTION_ITEMS "collection-items.html"
#define OGCAPI_TEMPLATE_HTML_COLLECTION_ITEM "collection-item.html"
#define OGCAPI_TEMPLATE_HTML_COLLECTION_QUERYABLES "collection-queryables.html"
#define OGCAPI_TEMPLATE_HTML_OPENAPI "openapi.html"

#define OGCAPI_DEFAULT_LIMIT 10 // by specification
#define OGCAPI_MAX_LIMIT 10000

#define OGCAPI_DEFAULT_GEOMETRY_PRECISION 6

constexpr const char *EPSG_PREFIX_URL =
    "http://www.opengis.net/def/crs/EPSG/0/";
constexpr const char *CRS84_URL =
    "http://www.opengis.net/def/crs/OGC/1.3/CRS84";

#ifdef USE_OGCAPI_SVR

/** Returns whether we enforce compliance mode. Defaults to true */
static bool msOGCAPIComplianceMode(const mapObj *map) {
  const char *compliance_mode =
      msOWSLookupMetadata(&(map->web.metadata), "A", "compliance_mode");
  return compliance_mode == NULL || strcasecmp(compliance_mode, "true") == 0;
}

/*
** Returns a JSON object using and a description.
*/
void msOGCAPIOutputError(OGCAPIErrorType errorType,
                         const std::string &description) {
  const char *code = "";
  const char *status = "";
  switch (errorType) {
  case OGCAPI_SERVER_ERROR: {
    code = "ServerError";
    status = "500";
    break;
  }
  case OGCAPI_CONFIG_ERROR: {
    code = "ConfigError";
    status = "500";
    break;
  }
  case OGCAPI_PARAM_ERROR: {
    code = "InvalidParameterValue";
    status = "400";
    break;
  }
  case OGCAPI_NOT_FOUND_ERROR: {
    code = "NotFound";
    status = "404";
    break;
  }
  }

  json j = {{"code", code}, {"description", description}};

  msIO_setHeader("Content-Type", "%s", OGCAPI_MIMETYPE_JSON);
  msIO_setHeader("Status", "%s", status);
  msIO_sendHeaders();
  msIO_printf("%s\n", j.dump().c_str());
}

static int includeLayer(mapObj *map, layerObj *layer) {
  if (!msOWSRequestIsEnabled(map, layer, "AO", "OGCAPI", MS_FALSE) ||
      !msIsLayerSupportedForWFSOrOAPIF(layer) || !msIsLayerQueryable(layer)) {
    return MS_FALSE;
  } else {
    return MS_TRUE;
  }
}

/*
** Get stuff...
*/

/*
** Returns the value associated with an item from the request's query string and
*NULL if the item was not found.
*/
static const char *getRequestParameter(const cgiRequestObj *request,
                                       const char *item) {
  for (int i = 0; i < request->NumParams; i++) {
    if (strcmp(item, request->ParamNames[i]) == 0)
      return request->ParamValues[i];
  }

  return nullptr;
}

static int getMaxLimit(mapObj *map, layerObj *layer) {
  int max_limit = OGCAPI_MAX_LIMIT;
  const char *value;

  // check metadata, layer then map
  value = msOWSLookupMetadata(&(layer->metadata), "A", "max_limit");
  if (value == NULL)
    value = msOWSLookupMetadata(&(map->web.metadata), "A", "max_limit");

  if (value != NULL) {
    int status = msStringToInt(value, &max_limit, 10);
    if (status != MS_SUCCESS)
      max_limit = OGCAPI_MAX_LIMIT; // conversion failed
  }

  return max_limit;
}

static int getDefaultLimit(const mapObj *map, const layerObj *layer) {
  int default_limit = OGCAPI_DEFAULT_LIMIT;

  // check metadata, layer then map
  const char *value =
      msOWSLookupMetadata(&(layer->metadata), "A", "default_limit");
  if (value == NULL)
    value = msOWSLookupMetadata(&(map->web.metadata), "A", "default_limit");

  if (value != NULL) {
    int status = msStringToInt(value, &default_limit, 10);
    if (status != MS_SUCCESS)
      default_limit = OGCAPI_DEFAULT_LIMIT; // conversion failed
  }

  return default_limit;
}

static std::string getExtraParameterString(const mapObj *map,
                                           const layerObj *layer) {

  std::string extra_params;

  // first check layer metadata if layer is not null
  if (layer) {
    const char *layerVal =
        msOWSLookupMetadata(&(layer->metadata), "AO", "extra_params");
    if (layerVal)
      extra_params = std::string("&") + layerVal;
  }

  if (extra_params.empty() && map) {
    const char *mapVal =
        msOWSLookupMetadata(&(map->web.metadata), "AO", "extra_params");
    if (mapVal)
      extra_params = std::string("&") + mapVal;
  }

  return extra_params;
}

static std::set<std::string>
getExtraParameters(const char *pszExtraParameters) {
  std::set<std::string> ret;
  for (const auto &param : msStringSplit(pszExtraParameters, '&')) {
    const auto keyValue = msStringSplit(param.c_str(), '=');
    if (!keyValue.empty())
      ret.insert(keyValue[0]);
  }
  return ret;
}

static std::set<std::string> getExtraParameters(const mapObj *map,
                                                const layerObj *layer) {

  // first check layer metadata if layer is not null
  if (layer) {
    const char *layerVal =
        msOWSLookupMetadata(&(layer->metadata), "AO", "extra_params");
    if (layerVal)
      return getExtraParameters(layerVal);
  }

  if (map) {
    const char *mapVal =
        msOWSLookupMetadata(&(map->web.metadata), "AO", "extra_params");
    if (mapVal)
      return getExtraParameters(mapVal);
  }

  return {};
}

static bool
msOOGCAPICheckQueryParameters(const mapObj *map, const cgiRequestObj *request,
                              const std::set<std::string> &allowedParameters) {
  if (msOGCAPIComplianceMode(map)) {
    for (int j = 0; j < request->NumParams; j++) {
      const char *paramName = request->ParamNames[j];
      if (allowedParameters.find(paramName) == allowedParameters.end()) {
        msOGCAPIOutputError(
            OGCAPI_PARAM_ERROR,
            (std::string("Unknown query parameter: ") + paramName).c_str());
        return false;
      }
    }
  }
  return true;
}

static const char *getItemAliasOrName(const layerObj *layer, const char *item) {
  std::string key = std::string(item) + "_alias";
  if (const char *value =
          msOWSLookupMetadata(&(layer->metadata), "OGA", key.c_str())) {
    return value;
  }
  return item;
}

/** Return the list of queryable items */
static std::vector<std::string> msOOGCAPIGetLayerQueryables(
    layerObj *layer, const std::set<std::string> &reservedParams, bool &error) {
  error = false;
  std::vector<std::string> queryableItems;
  if (const char *value =
          msOWSLookupMetadata(&(layer->metadata), "OGA", "queryable_items")) {
    queryableItems = msStringSplit(value, ',');
    if (!queryableItems.empty()) {
      if (msLayerOpen(layer) != MS_SUCCESS ||
          msLayerGetItems(layer) != MS_SUCCESS) {
        msOGCAPIOutputError(OGCAPI_SERVER_ERROR, "Cannot get layer fields");
        return {};
      }
      if (queryableItems[0] == "all") {
        queryableItems.clear();
        for (int i = 0; i < layer->numitems; ++i) {
          if (reservedParams.find(layer->items[i]) == reservedParams.end()) {
            queryableItems.push_back(layer->items[i]);
          }
        }
      } else {
        std::set<std::string> validItems;
        for (int i = 0; i < layer->numitems; ++i) {
          validItems.insert(layer->items[i]);
        }
        for (auto &item : queryableItems) {
          if (validItems.find(item) == validItems.end()) {
            // This is not a known field
            msOGCAPIOutputError(OGCAPI_CONFIG_ERROR,
                                "Invalid item '" + item +
                                    "' in queryable_items");
            error = true;
            return {};
          } else if (reservedParams.find(item) != reservedParams.end()) {
            // Check clashes with OGC API Features reserved keywords (bbox,
            // etc.)
            msOGCAPIOutputError(
                OGCAPI_CONFIG_ERROR,
                "Item '" + item +
                    "' in queryable_items is a reserved parameter name");
            error = true;
            return {};
          } else {
            item = getItemAliasOrName(layer, item.c_str());
          }
        }
      }
    }
  }
  return queryableItems;
}

/*
** Returns the limit as an int - between 1 and getMaxLimit(). We always return a
*valid value...
*/
static int getLimit(mapObj *map, cgiRequestObj *request, layerObj *layer,
                    int *limit) {
  int status;
  const char *p;

  int max_limit;
  max_limit = getMaxLimit(map, layer);

  p = getRequestParameter(request, "limit");
  if (!p || (p && strlen(p) == 0)) { // missing or empty
    *limit = MS_MIN(getDefaultLimit(map, layer),
                    max_limit); // max could be smaller than the default
  } else {
    status = msStringToInt(p, limit, 10);
    if (status != MS_SUCCESS)
      return MS_FAILURE;

    if (*limit <= 0) {
      *limit = MS_MIN(getDefaultLimit(map, layer),
                      max_limit); // max could be smaller than the default
    } else {
      *limit = MS_MIN(*limit, max_limit);
    }
  }

  return MS_SUCCESS;
}

// Return the content of the "crs" member of the /collections/{name} response
static json getCrsList(mapObj *map, layerObj *layer) {
  char *pszSRSList = NULL;
  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "AOF", MS_FALSE,
                   &pszSRSList);
  if (!pszSRSList)
    msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "AOF", MS_FALSE,
                     &pszSRSList);
  json jCrsList;
  if (pszSRSList) {
    const auto tokens = msStringSplit(pszSRSList, ' ');
    for (const auto &crs : tokens) {
      if (crs.find("EPSG:") == 0) {
        if (jCrsList.empty()) {
          jCrsList.push_back(CRS84_URL);
        }
        const std::string url =
            std::string(EPSG_PREFIX_URL) + crs.substr(strlen("EPSG:"));
        jCrsList.push_back(url);
      }
    }
    msFree(pszSRSList);
  }
  return jCrsList;
}

// Return the content of the "storageCrs" member of the /collections/{name}
// response
static std::string getStorageCrs(layerObj *layer) {
  std::string storageCrs;
  char *pszFirstSRS = nullptr;
  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "AOF", MS_TRUE,
                   &pszFirstSRS);
  if (pszFirstSRS) {
    if (std::string(pszFirstSRS).find("EPSG:") == 0) {
      storageCrs =
          std::string(EPSG_PREFIX_URL) + (pszFirstSRS + strlen("EPSG:"));
    }
    msFree(pszFirstSRS);
  }
  return storageCrs;
}

/*
** Returns the bbox in output CRS (CRS84 by default, or "crs" request parameter
*when specified)
*/
static bool getBbox(mapObj *map, layerObj *layer, cgiRequestObj *request,
                    rectObj *bbox, projectionObj *outputProj) {
  int status;

  const char *bboxParam = getRequestParameter(request, "bbox");
  if (!bboxParam || strlen(bboxParam) == 0) { // missing or empty extent
    rectObj rect;
    if (FLTLayerSetInvalidRectIfSupported(layer, &rect, "AO")) {
      bbox->minx = rect.minx;
      bbox->miny = rect.miny;
      bbox->maxx = rect.maxx;
      bbox->maxy = rect.maxy;
    } else {
      // assign map->extent (no projection necessary)
      bbox->minx = map->extent.minx;
      bbox->miny = map->extent.miny;
      bbox->maxx = map->extent.maxx;
      bbox->maxy = map->extent.maxy;
    }
  } else {
    const auto tokens = msStringSplit(bboxParam, ',');
    if (tokens.size() != 4) {
      msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Bad value for bbox.");
      return false;
    }

    double values[4];
    for (int i = 0; i < 4; i++) {
      status = msStringToDouble(tokens[i].c_str(), &values[i]);
      if (status != MS_SUCCESS) {
        msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Bad value for bbox.");
        return false;
      }
    }

    bbox->minx = values[0]; // assign
    bbox->miny = values[1];
    bbox->maxx = values[2];
    bbox->maxy = values[3];

    // validate bbox is well-formed (degenerate is ok)
    if (MS_VALID_SEARCH_EXTENT(*bbox) != MS_TRUE) {
      msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Bad value for bbox.");
      return false;
    }

    std::string bboxCrs = "EPSG:4326";
    bool axisInverted =
        false; // because above EPSG:4326 is meant to be OGC:CRS84 actually
    const char *bboxCrsParam = getRequestParameter(request, "bbox-crs");
    if (bboxCrsParam) {
      bool isExpectedCrs = false;
      for (const auto &crsItem : getCrsList(map, layer)) {
        if (bboxCrsParam == crsItem.get<std::string>()) {
          isExpectedCrs = true;
          break;
        }
      }
      if (!isExpectedCrs) {
        msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Bad value for bbox-crs.");
        return false;
      }
      if (std::string(bboxCrsParam) != CRS84_URL) {
        if (std::string(bboxCrsParam).find(EPSG_PREFIX_URL) == 0) {
          const char *code = bboxCrsParam + strlen(EPSG_PREFIX_URL);
          bboxCrs = std::string("EPSG:") + code;
          axisInverted = msIsAxisInverted(atoi(code));
        }
      }
    }
    if (axisInverted) {
      std::swap(bbox->minx, bbox->miny);
      std::swap(bbox->maxx, bbox->maxy);
    }

    projectionObj bboxProj;
    msInitProjection(&bboxProj);
    msProjectionInheritContextFrom(&bboxProj, &(map->projection));
    if (msLoadProjectionString(&bboxProj, bboxCrs.c_str()) != 0) {
      msFreeProjection(&bboxProj);
      msOGCAPIOutputError(OGCAPI_SERVER_ERROR, "Cannot process bbox-crs.");
      return false;
    }

    status = msProjectRect(&bboxProj, outputProj, bbox);
    msFreeProjection(&bboxProj);
    if (status != MS_SUCCESS) {
      msOGCAPIOutputError(OGCAPI_SERVER_ERROR,
                          "Cannot reproject bbox from bbox-crs to output CRS.");
      return false;
    }
  }

  return true;
}

/*
** Returns the template directory location or NULL if it isn't set.
*/
std::string msOGCAPIGetTemplateDirectory(const mapObj *map, const char *key,
                                         const char *envvar) {
  const char *directory = NULL;

  if (map != NULL) {
    directory = msOWSLookupMetadata(&(map->web.metadata), "A", key);
  }

  if (directory == NULL) {
    directory = CPLGetConfigOption(envvar, NULL);
  }

  std::string s;
  if (directory != NULL) {
    s = directory;
    if (!s.empty() && (s.back() != '/' && s.back() != '\\')) {
      // add a trailing slash if missing
      std::string slash = "/";
#ifdef _WIN32
      slash = "\\";
#endif
      s += slash;
    }
  }

  return s;
}

/*
** Returns the service title from oga_{key} and/or ows_{key} or a default value
*if not set.
*/
static const char *getWebMetadata(const mapObj *map, const char *domain,
                                  const char *key, const char *defaultVal) {
  const char *value;

  if ((value = msOWSLookupMetadata(&(map->web.metadata), domain, key)) != NULL)
    return value;
  else
    return defaultVal;
}

/*
** Returns the service title from oga|ows_title or a default value if not set.
*/
static const char *getTitle(const mapObj *map) {
  return getWebMetadata(map, "OA", "title", OGCAPI_DEFAULT_TITLE);
}

/*
** Returns the API root URL from oga_onlineresource or builds a value if not
*set.
*/
std::string msOGCAPIGetApiRootUrl(const mapObj *map,
                                  const cgiRequestObj *request,
                                  const char *namespaces) {
  const char *root;
  if ((root = msOWSLookupMetadata(&(map->web.metadata), namespaces,
                                  "onlineresource")) != NULL) {
    return std::string(root);
  }

  std::string api_root;
  if (char *res = msBuildOnlineResource(NULL, request)) {
    api_root = res;
    free(res);

    // find last ogcapi in the string and strip the rest to get the root API
    std::size_t pos = api_root.rfind("ogcapi");
    if (pos != std::string::npos) {
      api_root = api_root.substr(0, pos + std::string("ogcapi").size());
    } else {
      // strip trailing '?' or '/' and append "/ogcapi"
      while (!api_root.empty() &&
             (api_root.back() == '?' || api_root.back() == '/')) {
        api_root.pop_back();
      }
      api_root += "/ogcapi";
    }
  }

  if (api_root.empty()) {
    api_root = "/ogcapi";
  }

  return api_root;
}

static json getFeatureConstant(const gmlConstantObj *constant) {
  json j; // empty (null)

  if (!constant)
    throw std::runtime_error("Null constant metadata.");
  if (!constant->value)
    return j;

  // initialize
  j = {{constant->name, constant->value}};

  return j;
}

static json getFeatureItem(const gmlItemObj *item, const char *value) {
  json j; // empty (null)
  const char *key;

  if (!item)
    throw std::runtime_error("Null item metadata.");
  if (!item->visible)
    return j;

  if (item->alias)
    key = item->alias;
  else
    key = item->name;

  // initialize
  j = {{key, value}};

  if (item->type &&
      (EQUAL(item->type, "Date") || EQUAL(item->type, "DateTime") ||
       EQUAL(item->type, "Time"))) {
    struct tm tm;
    if (msParseTime(value, &tm) == MS_TRUE) {
      char tmpValue[64];
      if (EQUAL(item->type, "Date"))
        snprintf(tmpValue, sizeof(tmpValue), "%04d-%02d-%02d",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
      else if (EQUAL(item->type, "Time"))
        snprintf(tmpValue, sizeof(tmpValue), "%02d:%02d:%02dZ", tm.tm_hour,
                 tm.tm_min, tm.tm_sec);
      else
        snprintf(tmpValue, sizeof(tmpValue), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                 tm.tm_min, tm.tm_sec);

      j = {{key, tmpValue}};
    }
  } else if (item->type &&
             (EQUAL(item->type, "Integer") || EQUAL(item->type, "Long"))) {
    try {
      j = {{key, std::stoll(value)}};
    } catch (const std::exception &) {
    }
  } else if (item->type && EQUAL(item->type, "Real")) {
    try {
      j = {{key, std::stod(value)}};
    } catch (const std::exception &) {
    }
  } else if (item->type && EQUAL(item->type, "Boolean")) {
    if (EQUAL(value, "0") || EQUAL(value, "false")) {
      j = {{key, false}};
    } else {
      j = {{key, true}};
    }
  }

  return j;
}

static double round_down(double value, int decimal_places) {
  const double multiplier = std::pow(10.0, decimal_places);
  return std::floor(value * multiplier) / multiplier;
}
// https://stackoverflow.com/questions/25925290/c-round-a-double-up-to-2-decimal-places
static double round_up(double value, int decimal_places) {
  const double multiplier = std::pow(10.0, decimal_places);
  return std::ceil(value * multiplier) / multiplier;
}

static json getFeatureGeometry(shapeObj *shape, int precision,
                               bool outputCrsAxisInverted) {
  json geometry; // empty (null)
  int *outerList = NULL, numOuterRings = 0;

  if (!shape)
    throw std::runtime_error("Null shape.");

  switch (shape->type) {
  case (MS_SHAPE_POINT):
    if (shape->numlines == 0 ||
        shape->line[0].numpoints == 0) // not enough info for a point
      return geometry;

    if (shape->line[0].numpoints == 1) {
      geometry["type"] = "Point";
      double x = shape->line[0].point[0].x;
      double y = shape->line[0].point[0].y;
      if (outputCrsAxisInverted)
        std::swap(x, y);
      geometry["coordinates"] = {round_up(x, precision),
                                 round_up(y, precision)};
    } else {
      geometry["type"] = "MultiPoint";
      geometry["coordinates"] = json::array();
      for (int j = 0; j < shape->line[0].numpoints; j++) {
        double x = shape->line[0].point[j].x;
        double y = shape->line[0].point[j].y;
        if (outputCrsAxisInverted)
          std::swap(x, y);
        geometry["coordinates"].push_back(
            {round_up(x, precision), round_up(y, precision)});
      }
    }
    break;
  case (MS_SHAPE_LINE):
    if (shape->numlines == 0 ||
        shape->line[0].numpoints < 2) // not enough info for a line
      return geometry;

    if (shape->numlines == 1) {
      geometry["type"] = "LineString";
      geometry["coordinates"] = json::array();
      for (int j = 0; j < shape->line[0].numpoints; j++) {
        double x = shape->line[0].point[j].x;
        double y = shape->line[0].point[j].y;
        if (outputCrsAxisInverted)
          std::swap(x, y);
        geometry["coordinates"].push_back(
            {round_up(x, precision), round_up(y, precision)});
      }
    } else {
      geometry["type"] = "MultiLineString";
      geometry["coordinates"] = json::array();
      for (int i = 0; i < shape->numlines; i++) {
        json part = json::array();
        for (int j = 0; j < shape->line[i].numpoints; j++) {
          double x = shape->line[i].point[j].x;
          double y = shape->line[i].point[j].y;
          if (outputCrsAxisInverted)
            std::swap(x, y);
          part.push_back({round_up(x, precision), round_up(y, precision)});
        }
        geometry["coordinates"].push_back(part);
      }
    }
    break;
  case (MS_SHAPE_POLYGON):
    if (shape->numlines == 0 ||
        shape->line[0].numpoints <
            4) // not enough info for a polygon (first=last)
      return geometry;

    outerList = msGetOuterList(shape);
    if (outerList == NULL)
      throw std::runtime_error("Unable to allocate list of outer rings.");
    for (int k = 0; k < shape->numlines; k++) {
      if (outerList[k] == MS_TRUE)
        numOuterRings++;
    }

    if (numOuterRings == 1) {
      geometry["type"] = "Polygon";
      geometry["coordinates"] = json::array();
      for (int i = 0; i < shape->numlines; i++) {
        json part = json::array();
        for (int j = 0; j < shape->line[i].numpoints; j++) {
          double x = shape->line[i].point[j].x;
          double y = shape->line[i].point[j].y;
          if (outputCrsAxisInverted)
            std::swap(x, y);
          part.push_back({round_up(x, precision), round_up(y, precision)});
        }
        geometry["coordinates"].push_back(part);
      }
    } else {
      geometry["type"] = "MultiPolygon";
      geometry["coordinates"] = json::array();

      for (int k = 0; k < shape->numlines; k++) {
        if (outerList[k] ==
            MS_TRUE) { // outer ring: generate polygon and add to coordinates
          int *innerList = msGetInnerList(shape, k, outerList);
          if (innerList == NULL) {
            msFree(outerList);
            throw std::runtime_error("Unable to allocate list of inner rings.");
          }

          json polygon = json::array();
          for (int i = 0; i < shape->numlines; i++) {
            if (i == k ||
                innerList[i] ==
                    MS_TRUE) { // add outer ring (k) and any inner rings
              json part = json::array();
              for (int j = 0; j < shape->line[i].numpoints; j++) {
                double x = shape->line[i].point[j].x;
                double y = shape->line[i].point[j].y;
                if (outputCrsAxisInverted)
                  std::swap(x, y);
                part.push_back(
                    {round_up(x, precision), round_up(y, precision)});
              }
              polygon.push_back(part);
            }
          }

          msFree(innerList);
          geometry["coordinates"].push_back(polygon);
        }
      }
    }
    msFree(outerList);
    break;
  default:
    throw std::runtime_error("Invalid shape type.");
    break;
  }

  return geometry;
}

/*
** Return a GeoJSON representation of a shape.
*/
static json getFeature(layerObj *layer, shapeObj *shape, gmlItemListObj *items,
                       gmlConstantListObj *constants, int geometry_precision,
                       bool outputCrsAxisInverted) {
  int i;
  json feature; // empty (null)

  if (!layer || !shape)
    throw std::runtime_error("Null arguments.");

  // initialize
  feature = {{"type", "Feature"}, {"properties", json::object()}};

  // id
  const char *featureIdItem =
      msOWSLookupMetadata(&(layer->metadata), "AGFO", "featureid");
  if (featureIdItem == NULL)
    throw std::runtime_error(
        "Missing required featureid metadata."); // should have been trapped
                                                 // earlier
  for (i = 0; i < items->numitems; i++) {
    if (strcasecmp(featureIdItem, items->items[i].name) == 0) {
      feature["id"] = shape->values[i];
      break;
    }
  }

  if (i == items->numitems)
    throw std::runtime_error("Feature id not found.");

  // properties - build from items and constants, no group support for now

  for (int i = 0; i < items->numitems; i++) {
    try {
      json item = getFeatureItem(&(items->items[i]), shape->values[i]);
      if (!item.is_null())
        feature["properties"].insert(item.begin(), item.end());
    } catch (const std::runtime_error &) {
      throw std::runtime_error("Error fetching item.");
    }
  }

  for (int i = 0; i < constants->numconstants; i++) {
    try {
      json constant = getFeatureConstant(&(constants->constants[i]));
      if (!constant.is_null())
        feature["properties"].insert(constant.begin(), constant.end());
    } catch (const std::runtime_error &) {
      throw std::runtime_error("Error fetching constant.");
    }
  }

  // geometry
  try {
    json geometry =
        getFeatureGeometry(shape, geometry_precision, outputCrsAxisInverted);
    if (!geometry.is_null())
      feature["geometry"] = std::move(geometry);
  } catch (const std::runtime_error &) {
    throw std::runtime_error("Error fetching geometry.");
  }

  return feature;
}

static json getLink(hashTableObj *metadata, const std::string &name) {
  json link;

  const char *href =
      msOWSLookupMetadata(metadata, "A", (name + "_href").c_str());
  if (!href)
    throw std::runtime_error("Missing required link href property.");

  const char *title =
      msOWSLookupMetadata(metadata, "A", (name + "_title").c_str());
  const char *type =
      msOWSLookupMetadata(metadata, "A", (name + "_type").c_str());

  link = {{"href", href},
          {"title", title ? title : href},
          {"type", type ? type : "text/html"}};

  return link;
}

static const char *getCollectionDescription(layerObj *layer) {
  const char *description =
      msOWSLookupMetadata(&(layer->metadata), "A", "description");
  if (!description)
    description = msOWSLookupMetadata(&(layer->metadata), "OF",
                                      "abstract"); // fallback on abstract
  if (!description)
    description =
        "<!-- Warning: unable to set the collection description. -->"; // finally
                                                                       // a
                                                                       // warning...
  return description;
}

static const char *getCollectionTitle(layerObj *layer) {
  const char *title = msOWSLookupMetadata(&(layer->metadata), "AOF", "title");
  if (!title)
    title = layer->name; // revert to layer name if no title found
  return title;
}

static int getGeometryPrecision(mapObj *map, layerObj *layer) {
  int geometry_precision = OGCAPI_DEFAULT_GEOMETRY_PRECISION;
  if (msOWSLookupMetadata(&(layer->metadata), "AF", "geometry_precision")) {
    geometry_precision = atoi(
        msOWSLookupMetadata(&(layer->metadata), "AF", "geometry_precision"));
  } else if (msOWSLookupMetadata(&map->web.metadata, "AF",
                                 "geometry_precision")) {
    geometry_precision = atoi(
        msOWSLookupMetadata(&map->web.metadata, "AF", "geometry_precision"));
  }
  return geometry_precision;
}

static json getCollection(mapObj *map, layerObj *layer, OGCAPIFormat format,
                          const std::string &api_root) {
  json collection; // empty (null)
  rectObj bbox;

  if (!map || !layer)
    return collection;

  if (!includeLayer(map, layer))
    return collection;

  // initialize some things
  if (msOWSGetLayerExtent(map, layer, "AOF", &bbox) == MS_SUCCESS) {
    if (layer->projection.numargs > 0)
      msOWSProjectToWGS84(&layer->projection, &bbox);
    else if (map->projection.numargs > 0)
      msOWSProjectToWGS84(&map->projection, &bbox);
    else
      throw std::runtime_error(
          "Unable to transform bounding box, no projection defined.");
  } else {
    throw std::runtime_error(
        "Unable to get collection bounding box."); // might be too harsh since
                                                   // extent is optional
  }

  const char *description = getCollectionDescription(layer);
  const char *title = getCollectionTitle(layer);

  const char *id = layer->name;
  char *id_encoded = msEncodeUrl(id); // free after use

  const int geometry_precision = getGeometryPrecision(map, layer);

  const std::string extra_params = getExtraParameterString(map, layer);

  // build collection object
  collection = {
      {"id", id},
      {"description", description},
      {"title", title},
      {"extent",
       {{"spatial",
         {{"bbox",
           {{round_down(bbox.minx, geometry_precision),
             round_down(bbox.miny, geometry_precision),
             round_up(bbox.maxx, geometry_precision),
             round_up(bbox.maxy, geometry_precision)}}},
          {"crs", CRS84_URL}}}}},
      {"links",
       {
           {{"rel", format == OGCAPIFormat::JSON ? "self" : "alternate"},
            {"type", OGCAPI_MIMETYPE_JSON},
            {"title", "This collection as JSON"},
            {"href", api_root + "/collections/" + std::string(id_encoded) +
                         "?f=json" + extra_params}},
           {{"rel", format == OGCAPIFormat::HTML ? "self" : "alternate"},
            {"type", OGCAPI_MIMETYPE_HTML},
            {"title", "This collection as HTML"},
            {"href", api_root + "/collections/" + std::string(id_encoded) +
                         "?f=html" + extra_params}},
           {{"rel", "items"},
            {"type", OGCAPI_MIMETYPE_GEOJSON},
            {"title", "Items for this collection as GeoJSON"},
            {"href", api_root + "/collections/" + std::string(id_encoded) +
                         "/items?f=json" + extra_params}},
           {{"rel", "items"},
            {"type", OGCAPI_MIMETYPE_HTML},
            {"title", "Items for this collection as HTML"},
            {"href", api_root + "/collections/" + std::string(id_encoded) +
                         "/items?f=html" + extra_params}},
           {{"rel", "http://www.opengis.net/def/rel/ogc/1.0/queryables"},
            {"type", OGCAPI_MIMETYPE_JSON_SCHEMA},
            {"title", "Queryables for this collection as JSON schema"},
            {"href", api_root + "/collections/" + std::string(id_encoded) +
                         "/queryables?f=json" + extra_params}},
           {{"rel", "http://www.opengis.net/def/rel/ogc/1.0/queryables"},
            {"type", OGCAPI_MIMETYPE_HTML},
            {"title", "Queryables for this collection as HTML"},
            {"href", api_root + "/collections/" + std::string(id_encoded) +
                         "/queryables?f=html" + extra_params}},

       }},
      {"itemType", "feature"}};

  msFree(id_encoded); // done

  // handle optional configuration (keywords and links)
  const char *value = msOWSLookupMetadata(&(layer->metadata), "A", "keywords");
  if (!value)
    value = msOWSLookupMetadata(&(layer->metadata), "OF",
                                "keywordlist"); // fallback on keywordlist
  if (value) {
    std::vector<std::string> keywords = msStringSplit(value, ',');
    for (const std::string &keyword : keywords) {
      collection["keywords"].push_back(keyword);
    }
  }

  value = msOWSLookupMetadata(&(layer->metadata), "A", "links");
  if (value) {
    std::vector<std::string> names = msStringSplit(value, ',');
    for (const std::string &name : names) {
      try {
        json link = getLink(&(layer->metadata), name);
        collection["links"].push_back(link);
      } catch (const std::runtime_error &e) {
        throw e;
      }
    }
  }

  // Part 2 - CRS support
  // Inspect metadata to set the "crs": [] member and "storageCrs" member

  json jCrsList = getCrsList(map, layer);
  if (!jCrsList.empty()) {
    collection["crs"] = std::move(jCrsList);

    std::string storageCrs = getStorageCrs(layer);
    if (!storageCrs.empty()) {
      collection["storageCrs"] = std::move(storageCrs);
    }
  }

  return collection;
}

/*
** Output stuff...
*/

void msOGCAPIOutputJson(
    const json &j, const char *mimetype,
    const std::map<std::string, std::vector<std::string>> &extraHeaders) {
  std::string js;

  try {
    js = j.dump();
  } catch (...) {
    msOGCAPIOutputError(OGCAPI_CONFIG_ERROR,
                        "Invalid UTF-8 data, check encoding.");
    return;
  }

  msIO_setHeader("Content-Type", "%s", mimetype);
  for (const auto &kvp : extraHeaders) {
    for (const auto &value : kvp.second) {
      msIO_setHeader(kvp.first.c_str(), "%s", value.c_str());
    }
  }
  msIO_sendHeaders();
  msIO_printf("%s\n", js.c_str());
}

void msOGCAPIOutputTemplate(const char *directory, const char *filename,
                            const json &j, const char *mimetype) {
  std::string _directory(directory);
  std::string _filename(filename);
  Environment env{_directory}; // catch

  // ERB-style instead of Mustache (we'll see)
  // env.set_expression("<%=", "%>");
  // env.set_statement("<%", "%>");

  // callbacks, need:
  //   - match (regex)
  //   - contains (substring)
  //   - URL encode

  try {
    std::string js = j.dump();
  } catch (...) {
    msOGCAPIOutputError(OGCAPI_CONFIG_ERROR,
                        "Invalid UTF-8 data, check encoding.");
    return;
  }

  try {
    Template t = env.parse_template(_filename); // catch
    std::string result = env.render(t, j);

    msIO_setHeader("Content-Type", "%s", mimetype);
    msIO_sendHeaders();
    msIO_printf("%s\n", result.c_str());
  } catch (const inja::RenderError &e) {
    msOGCAPIOutputError(OGCAPI_CONFIG_ERROR, "Template rendering error. " +
                                                 std::string(e.what()) + " (" +
                                                 std::string(filename) + ").");
    return;
  } catch (const inja::InjaError &e) {
    msOGCAPIOutputError(OGCAPI_CONFIG_ERROR,
                        "InjaError error. " + std::string(e.what()) + " (" +
                            std::string(filename) + ")." + " (" +
                            std::string(directory) + ").");
    return;
  } catch (...) {
    msOGCAPIOutputError(OGCAPI_SERVER_ERROR,
                        "General template handling error.");
    return;
  }
}

/*
** Generic response output.
*/
static void outputResponse(
    const mapObj *map, const cgiRequestObj *request, OGCAPIFormat format,
    const char *filename, const json &response,
    const std::map<std::string, std::vector<std::string>> &extraHeaders =
        std::map<std::string, std::vector<std::string>>()) {
  std::string path;
  char fullpath[MS_MAXPATHLEN];

  if (format == OGCAPIFormat::JSON) {
    msOGCAPIOutputJson(response, OGCAPI_MIMETYPE_JSON, extraHeaders);
  } else if (format == OGCAPIFormat::GeoJSON) {
    msOGCAPIOutputJson(response, OGCAPI_MIMETYPE_GEOJSON, extraHeaders);
  } else if (format == OGCAPIFormat::OpenAPI_V3) {
    msOGCAPIOutputJson(response, OGCAPI_MIMETYPE_OPENAPI_V3, extraHeaders);
  } else if (format == OGCAPIFormat::JSONSchema) {
    msOGCAPIOutputJson(response, OGCAPI_MIMETYPE_JSON_SCHEMA, extraHeaders);
  } else if (format == OGCAPIFormat::HTML) {
    path = msOGCAPIGetTemplateDirectory(map, "html_template_directory",
                                        "OGCAPI_HTML_TEMPLATE_DIRECTORY");
    if (path.empty()) {
      msOGCAPIOutputError(OGCAPI_CONFIG_ERROR, "Template directory not set.");
      return; // bail
    }
    msBuildPath(fullpath, map->mappath, path.c_str());

    json j;

    j["response"] = response; // nest the response so we could write the whole
                              // object in the template

    // extend the JSON with a few things that we need for templating
    const std::string extra_params = getExtraParameterString(map, nullptr);

    j["template"] = {{"path", json::array()},
                     {"params", json::object()},
                     {"api_root", msOGCAPIGetApiRootUrl(map, request)},
                     {"extra_params", extra_params},
                     {"title", getTitle(map)},
                     {"tags", json::object()}};

    // api path
    for (int i = 0; i < request->api_path_length; i++)
      j["template"]["path"].push_back(request->api_path[i]);

    // parameters (optional)
    for (int i = 0; i < request->NumParams; i++) {
      if (request->ParamValues[i] &&
          strlen(request->ParamValues[i]) > 0) { // skip empty params
        j["template"]["params"].update(
            {{request->ParamNames[i], request->ParamValues[i]}});
      }
    }

    // add custom tags (optional)
    const char *tags =
        msOWSLookupMetadata(&(map->web.metadata), "A", "html_tags");
    if (tags) {
      std::vector<std::string> names = msStringSplit(tags, ',');
      for (std::string name : names) {
        const char *value = msOWSLookupMetadata(&(map->web.metadata), "A",
                                                ("tag_" + name).c_str());
        if (value) {
          j["template"]["tags"].update({{name, value}}); // add object
        }
      }
    }

    msOGCAPIOutputTemplate(fullpath, filename, j, OGCAPI_MIMETYPE_HTML);
  } else {
    msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Unsupported format requested.");
  }
}

/*
** Process stuff...
*/
static int processLandingRequest(mapObj *map, cgiRequestObj *request,
                                 OGCAPIFormat format) {
  json response;

  auto allowedParameters = getExtraParameters(map, nullptr);
  allowedParameters.insert("f");
  if (!msOOGCAPICheckQueryParameters(map, request, allowedParameters)) {
    return MS_SUCCESS;
  }

  // define ambiguous elements
  const char *description =
      msOWSLookupMetadata(&(map->web.metadata), "A", "description");
  if (!description)
    description =
        msOWSLookupMetadata(&(map->web.metadata), "OF",
                            "abstract"); // fallback on abstract if necessary

  const std::string extra_params = getExtraParameterString(map, nullptr);

  // define api root url
  std::string api_root = msOGCAPIGetApiRootUrl(map, request);

  // build response object
  //   - consider conditionally excluding links for HTML format
  response = {
      {"title", getTitle(map)},
      {"description", description ? description : ""},
      {"links",
       {{{"rel", format == OGCAPIFormat::JSON ? "self" : "alternate"},
         {"type", OGCAPI_MIMETYPE_JSON},
         {"title", "This document as JSON"},
         {"href", api_root + "?f=json" + extra_params}},
        {{"rel", format == OGCAPIFormat::HTML ? "self" : "alternate"},
         {"type", OGCAPI_MIMETYPE_HTML},
         {"title", "This document as HTML"},
         {"href", api_root + "?f=html" + extra_params}},
        {{"rel", "conformance"},
         {"type", OGCAPI_MIMETYPE_JSON},
         {"title",
          "OCG API conformance classes implemented by this server (JSON)"},
         {"href", api_root + "/conformance?f=json" + extra_params}},
        {{"rel", "conformance"},
         {"type", OGCAPI_MIMETYPE_HTML},
         {"title", "OCG API conformance classes implemented by this server"},
         {"href", api_root + "/conformance?f=html" + extra_params}},
        {{"rel", "data"},
         {"type", OGCAPI_MIMETYPE_JSON},
         {"title", "Information about feature collections available from this "
                   "server (JSON)"},
         {"href", api_root + "/collections?f=json" + extra_params}},
        {{"rel", "data"},
         {"type", OGCAPI_MIMETYPE_HTML},
         {"title",
          "Information about feature collections available from this server"},
         {"href", api_root + "/collections?f=html" + extra_params}},
        {{"rel", "service-desc"},
         {"type", OGCAPI_MIMETYPE_OPENAPI_V3},
         {"title", "OpenAPI document"},
         {"href", api_root + "/api?f=json" + extra_params}},
        {{"rel", "service-doc"},
         {"type", OGCAPI_MIMETYPE_HTML},
         {"title", "API documentation"},
         {"href", api_root + "/api?f=html" + extra_params}}}}};

  // handle custom links (optional)
  const char *links = msOWSLookupMetadata(&(map->web.metadata), "A", "links");
  if (links) {
    std::vector<std::string> names = msStringSplit(links, ',');
    for (std::string name : names) {
      try {
        json link = getLink(&(map->web.metadata), name);
        response["links"].push_back(link);
      } catch (const std::runtime_error &e) {
        msOGCAPIOutputError(OGCAPI_CONFIG_ERROR, std::string(e.what()));
        return MS_SUCCESS;
      }
    }
  }

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_LANDING, response);
  return MS_SUCCESS;
}

static int processConformanceRequest(mapObj *map, cgiRequestObj *request,
                                     OGCAPIFormat format) {
  json response;

  auto allowedParameters = getExtraParameters(map, nullptr);
  allowedParameters.insert("f");
  if (!msOOGCAPICheckQueryParameters(map, request, allowedParameters)) {
    return MS_SUCCESS;
  }

  // build response object
  response = {
      {"conformsTo",
       {
           "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
           "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
           "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/core",
           "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/oas30",
           "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/html",
           "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/geojson",
           "http://www.opengis.net/spec/ogcapi-features-2/1.0/conf/crs",
           "http://www.opengis.net/spec/ogcapi-features-3/1.0/conf/queryables",
           "http://www.opengis.net/spec/ogcapi-features-3/1.0/conf/"
           "queryables-query-parameters",
           "http://www.opengis.net/spec/ogcapi-features-5/1.0/conf/queryables",
       }}};

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_CONFORMANCE,
                 response);
  return MS_SUCCESS;
}

static int findLayerIndex(const mapObj *map, const char *collectionId) {
  for (int i = 0; i < map->numlayers; i++) {
    if (strcmp(map->layers[i]->name, collectionId) == 0) {
      return i;
    }
  }
  return -1;
}

static int processCollectionItemsRequest(mapObj *map, cgiRequestObj *request,
                                         const char *collectionId,
                                         const char *featureId,
                                         OGCAPIFormat format) {
  json response;

  // find the right layer
  const int iLayer = findLayerIndex(map, collectionId);

  if (iLayer < 0) { // invalid collectionId
    msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  layerObj *layer = map->layers[iLayer];
  layer->status = MS_ON; // force on (do we need to save and reset?)

  if (!includeLayer(map, layer)) {
    msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  //
  // handle parameters specific to this endpoint
  //
  int limit = -1;
  if (getLimit(map, request, layer, &limit) != MS_SUCCESS) {
    msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Bad value for limit.");
    return MS_SUCCESS;
  }

  std::string api_root = msOGCAPIGetApiRootUrl(map, request);
  const char *crs = getRequestParameter(request, "crs");

  std::string outputCrs = "EPSG:4326";
  bool outputCrsAxisInverted =
      false; // because above EPSG:4326 is meant to be OGC:CRS84 actually
  std::map<std::string, std::vector<std::string>> extraHeaders;
  if (crs) {
    bool isExpectedCrs = false;
    for (const auto &crsItem : getCrsList(map, layer)) {
      if (crs == crsItem.get<std::string>()) {
        isExpectedCrs = true;
        break;
      }
    }
    if (!isExpectedCrs) {
      msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Bad value for crs.");
      return MS_SUCCESS;
    }
    extraHeaders["Content-Crs"].push_back('<' + std::string(crs) + '>');
    if (std::string(crs) != CRS84_URL) {
      if (std::string(crs).find(EPSG_PREFIX_URL) == 0) {
        const char *code = crs + strlen(EPSG_PREFIX_URL);
        outputCrs = std::string("EPSG:") + code;
        outputCrsAxisInverted = msIsAxisInverted(atoi(code));
      }
    }
  } else {
    extraHeaders["Content-Crs"].push_back('<' + std::string(CRS84_URL) + '>');
  }

  auto allowedParameters = getExtraParameters(map, layer);
  allowedParameters.insert("f");
  allowedParameters.insert("bbox");
  allowedParameters.insert("bbox-crs");
  allowedParameters.insert("datetime");
  allowedParameters.insert("limit");
  allowedParameters.insert("offset");
  allowedParameters.insert("crs");

  bool error = false;
  const std::vector<std::string> queryableItems =
      msOOGCAPIGetLayerQueryables(layer, allowedParameters, error);
  if (error) {
    return MS_SUCCESS;
  }

  for (const auto &item : queryableItems)
    allowedParameters.insert(item);

  if (!msOOGCAPICheckQueryParameters(map, request, allowedParameters)) {
    return MS_SUCCESS;
  }

  std::string filter;
  std::string query_kvp;
  if (!queryableItems.empty()) {
    for (int i = 0; i < request->NumParams; i++) {
      if (std::find(queryableItems.begin(), queryableItems.end(),
                    request->ParamNames[i]) != queryableItems.end()) {

        // Find actual item name from alias
        const char *pszItem = nullptr;
        for (int j = 0; j < layer->numitems; ++j) {
          if (strcmp(request->ParamNames[i],
                     getItemAliasOrName(layer, layer->items[j])) == 0) {
            pszItem = layer->items[j];
            break;
          }
        }
        assert(pszItem);

        const std::string expr = FLTGetBinaryComparisonCommonExpression(
            layer, pszItem, false, "=", request->ParamValues[i]);
        if (!filter.empty())
          filter += " AND ";
        filter += expr;

        query_kvp += '&';
        char *encoded = msEncodeUrl(request->ParamNames[i]);
        query_kvp += encoded;
        msFree(encoded);
        query_kvp += '=';
        encoded = msEncodeUrl(request->ParamValues[i]);
        query_kvp += encoded;
        msFree(encoded);
      }
    }
  }

  struct ReprojectionObjects {
    reprojectionObj *reprojector = NULL;
    projectionObj proj;

    ReprojectionObjects() { msInitProjection(&proj); }

    ~ReprojectionObjects() {
      msProjectDestroyReprojector(reprojector);
      msFreeProjection(&proj);
    }

    int executeQuery(mapObj *map) {
      projectionObj backupMapProjection = map->projection;
      map->projection = proj;
      int ret = msExecuteQuery(map);
      map->projection = backupMapProjection;
      return ret;
    }
  };
  ReprojectionObjects reprObjs;

  msProjectionInheritContextFrom(&reprObjs.proj, &(map->projection));
  if (msLoadProjectionString(&reprObjs.proj, outputCrs.c_str()) != 0) {
    msOGCAPIOutputError(OGCAPI_SERVER_ERROR, "Cannot instantiate output CRS.");
    return MS_SUCCESS;
  }

  if (layer->projection.numargs > 0) {
    if (msProjectionsDiffer(&(layer->projection), &reprObjs.proj)) {
      reprObjs.reprojector =
          msProjectCreateReprojector(&(layer->projection), &reprObjs.proj);
      if (reprObjs.reprojector == NULL) {
        msOGCAPIOutputError(OGCAPI_SERVER_ERROR,
                            "Error creating re-projector.");
        return MS_SUCCESS;
      }
    }
  } else if (map->projection.numargs > 0) {
    if (msProjectionsDiffer(&(map->projection), &reprObjs.proj)) {
      reprObjs.reprojector =
          msProjectCreateReprojector(&(map->projection), &reprObjs.proj);
      if (reprObjs.reprojector == NULL) {
        msOGCAPIOutputError(OGCAPI_SERVER_ERROR,
                            "Error creating re-projector.");
        return MS_SUCCESS;
      }
    }
  } else {
    msOGCAPIOutputError(
        OGCAPI_CONFIG_ERROR,
        "Unable to transform geometries, no projection defined.");
    return MS_SUCCESS;
  }

  if (map->projection.numargs > 0) {
    msProjectRect(&(map->projection), &reprObjs.proj, &map->extent);
  }

  rectObj bbox;
  if (!getBbox(map, layer, request, &bbox, &reprObjs.proj)) {
    return MS_SUCCESS;
  }

  int offset = 0;
  int numberMatched = 0;
  if (featureId) {
    const char *featureIdItem =
        msOWSLookupMetadata(&(layer->metadata), "AGFO", "featureid");
    if (featureIdItem == NULL) {
      msOGCAPIOutputError(OGCAPI_CONFIG_ERROR,
                          "Missing required featureid metadata.");
      return MS_SUCCESS;
    }

    // TODO: does featureIdItem exist in the data?

    // optional validation
    const char *featureIdValidation =
        msLookupHashTable(&(layer->validation), featureIdItem);
    if (featureIdValidation &&
        msValidateParameter(featureId, featureIdValidation, NULL, NULL, NULL) !=
            MS_SUCCESS) {
      msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid feature id.");
      return MS_SUCCESS;
    }

    map->query.type = MS_QUERY_BY_FILTER;
    map->query.mode = MS_QUERY_SINGLE;
    map->query.layer = iLayer;
    map->query.rect = bbox;
    map->query.filteritem = strdup(featureIdItem);

    msInitExpression(&map->query.filter);
    map->query.filter.type = MS_STRING;
    map->query.filter.string = strdup(featureId);

    if (reprObjs.executeQuery(map) != MS_SUCCESS) {
      msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR,
                          "Collection items id query failed.");
      return MS_SUCCESS;
    }

    if (!layer->resultcache || layer->resultcache->numresults != 1) {
      msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR,
                          "Collection items id query failed.");
      return MS_SUCCESS;
    }
  } else { // bbox query
    map->query.type = MS_QUERY_BY_RECT;
    map->query.mode = MS_QUERY_MULTIPLE;
    map->query.layer = iLayer;
    map->query.rect = bbox;
    map->query.only_cache_result_count = MS_TRUE;

    if (!filter.empty()) {
      map->query.type = MS_QUERY_BY_FILTER;
      msInitExpression(&map->query.filter);
      map->query.filter.string = msStrdup(filter.c_str());
      map->query.filter.type = MS_EXPRESSION;
    }

    // get number matched
    if (reprObjs.executeQuery(map) != MS_SUCCESS) {
      msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR,
                          "Collection items query failed.");
      return MS_SUCCESS;
    }

    if (!layer->resultcache) {
      msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR,
                          "Collection items query failed.");
      return MS_SUCCESS;
    }

    numberMatched = layer->resultcache->numresults;

    if (numberMatched > 0) {
      map->query.only_cache_result_count = MS_FALSE;
      map->query.maxfeatures = limit;

      const char *offsetStr = getRequestParameter(request, "offset");
      if (offsetStr) {
        if (msStringToInt(offsetStr, &offset, 10) != MS_SUCCESS) {
          msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Bad value for offset.");
          return MS_SUCCESS;
        }

        if (offset < 0 || offset >= numberMatched) {
          msOGCAPIOutputError(OGCAPI_PARAM_ERROR, "Offset out of range.");
          return MS_SUCCESS;
        }

        // msExecuteQuery() use a 1-based offset convention, whereas the API
        // uses a 0-based offset convention.
        map->query.startindex = 1 + offset;
        layer->startindex = 1 + offset;
      }

      if (reprObjs.executeQuery(map) != MS_SUCCESS || !layer->resultcache) {
        msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR,
                            "Collection items query failed.");
        return MS_SUCCESS;
      }
    }
  }

  const std::string extra_params = getExtraParameterString(map, layer);

  // build response object
  if (!featureId) {
    const char *id = layer->name;
    char *id_encoded = msEncodeUrl(id); // free after use

    std::string extra_kvp = "&limit=" + std::to_string(limit);
    extra_kvp += "&offset=" + std::to_string(offset);

    std::string other_extra_kvp;
    if (crs)
      other_extra_kvp += "&crs=" + std::string(crs);
    const char *bbox = getRequestParameter(request, "bbox");
    if (bbox)
      other_extra_kvp += "&bbox=" + std::string(bbox);
    const char *bboxCrs = getRequestParameter(request, "bbox-crs");
    if (bboxCrs)
      other_extra_kvp += "&bbox-crs=" + std::string(bboxCrs);

    other_extra_kvp += query_kvp;

    response = {{"type", "FeatureCollection"},
                {"numberMatched", numberMatched},
                {"numberReturned", layer->resultcache->numresults},
                {"features", json::array()},
                {"links",
                 {{{"rel", format == OGCAPIFormat::JSON ? "self" : "alternate"},
                   {"type", OGCAPI_MIMETYPE_GEOJSON},
                   {"title", "Items for this collection as GeoJSON"},
                   {"href", api_root + "/collections/" +
                                std::string(id_encoded) + "/items?f=json" +
                                extra_kvp + other_extra_kvp + extra_params}},
                  {{"rel", format == OGCAPIFormat::HTML ? "self" : "alternate"},
                   {"type", OGCAPI_MIMETYPE_HTML},
                   {"title", "Items for this collection as HTML"},
                   {"href", api_root + "/collections/" +
                                std::string(id_encoded) + "/items?f=html" +
                                extra_kvp + other_extra_kvp + extra_params}}}}};

    if (offset + layer->resultcache->numresults < numberMatched) {
      response["links"].push_back(
          {{"rel", "next"},
           {"type", format == OGCAPIFormat::JSON ? OGCAPI_MIMETYPE_GEOJSON
                                                 : OGCAPI_MIMETYPE_HTML},
           {"title", "next page"},
           {"href",
            api_root + "/collections/" + std::string(id_encoded) +
                "/items?f=" + (format == OGCAPIFormat::JSON ? "json" : "html") +
                "&limit=" + std::to_string(limit) +
                "&offset=" + std::to_string(offset + limit) + other_extra_kvp +
                extra_params}});
    }

    if (offset > 0) {
      response["links"].push_back(
          {{"rel", "prev"},
           {"type", format == OGCAPIFormat::JSON ? OGCAPI_MIMETYPE_GEOJSON
                                                 : OGCAPI_MIMETYPE_HTML},
           {"title", "previous page"},
           {"href",
            api_root + "/collections/" + std::string(id_encoded) +
                "/items?f=" + (format == OGCAPIFormat::JSON ? "json" : "html") +
                "&limit=" + std::to_string(limit) +
                "&offset=" + std::to_string(MS_MAX(0, (offset - limit))) +
                other_extra_kvp + extra_params}});
    }

    extraHeaders["OGC-NumberReturned"].push_back(
        std::to_string(layer->resultcache->numresults));
    extraHeaders["OGC-NumberMatched"].push_back(std::to_string(numberMatched));
    std::vector<std::string> linksHeaders;
    for (auto &link : response["links"]) {
      linksHeaders.push_back("<" + link["href"].get<std::string>() +
                             ">; rel=\"" + link["rel"].get<std::string>() +
                             "\"; title=\"" + link["title"].get<std::string>() +
                             "\"; type=\"" + link["type"].get<std::string>() +
                             "\"");
    }
    extraHeaders["Link"] = std::move(linksHeaders);

    msFree(id_encoded); // done
  }

  // features (items)
  {
    shapeObj shape;
    msInitShape(&shape);

    // we piggyback on GML configuration
    gmlItemListObj *items = msGMLGetItems(layer, "AG");
    gmlConstantListObj *constants = msGMLGetConstants(layer, "AG");

    if (!items || !constants) {
      msGMLFreeItems(items);
      msGMLFreeConstants(constants);
      msOGCAPIOutputError(OGCAPI_SERVER_ERROR,
                          "Error fetching layer attribute metadata.");
      return MS_SUCCESS;
    }

    const int geometry_precision = getGeometryPrecision(map, layer);

    for (int i = 0; i < layer->resultcache->numresults; i++) {
      int status =
          msLayerGetShape(layer, &shape, &(layer->resultcache->results[i]));
      if (status != MS_SUCCESS) {
        msGMLFreeItems(items);
        msGMLFreeConstants(constants);
        msOGCAPIOutputError(OGCAPI_SERVER_ERROR, "Error fetching feature.");
        return MS_SUCCESS;
      }

      if (reprObjs.reprojector) {
        status = msProjectShapeEx(reprObjs.reprojector, &shape);
        if (status != MS_SUCCESS) {
          msGMLFreeItems(items);
          msGMLFreeConstants(constants);
          msFreeShape(&shape);
          msOGCAPIOutputError(OGCAPI_SERVER_ERROR,
                              "Error reprojecting feature.");
          return MS_SUCCESS;
        }
      }

      try {
        json feature = getFeature(layer, &shape, items, constants,
                                  geometry_precision, outputCrsAxisInverted);
        if (featureId) {
          response = std::move(feature);
        } else {
          response["features"].emplace_back(std::move(feature));
        }
      } catch (const std::runtime_error &e) {
        msGMLFreeItems(items);
        msGMLFreeConstants(constants);
        msFreeShape(&shape);
        msOGCAPIOutputError(OGCAPI_SERVER_ERROR,
                            "Error getting feature. " + std::string(e.what()));
        return MS_SUCCESS;
      }

      msFreeShape(&shape); // next
    }

    msGMLFreeItems(items); // clean up
    msGMLFreeConstants(constants);
  }

  // extend the response a bit for templating (HERE)
  if (format == OGCAPIFormat::HTML) {
    const char *title = getCollectionTitle(layer);
    const char *id = layer->name;
    response["collection"] = {{"id", id}, {"title", title ? title : ""}};
  }

  if (featureId) {
    const char *id = layer->name;
    char *id_encoded = msEncodeUrl(id); // free after use

    response["links"] = {
        {{"rel", format == OGCAPIFormat::JSON ? "self" : "alternate"},
         {"type", OGCAPI_MIMETYPE_GEOJSON},
         {"title", "This document as GeoJSON"},
         {"href", api_root + "/collections/" + std::string(id_encoded) +
                      "/items/" + featureId + "?f=json" + extra_params}},
        {{"rel", format == OGCAPIFormat::HTML ? "self" : "alternate"},
         {"type", OGCAPI_MIMETYPE_HTML},
         {"title", "This document as HTML"},
         {"href", api_root + "/collections/" + std::string(id_encoded) +
                      "/items/" + featureId + "?f=html" + extra_params}},
        {{"rel", "collection"},
         {"type", OGCAPI_MIMETYPE_JSON},
         {"title", "This collection as JSON"},
         {"href", api_root + "/collections/" + std::string(id_encoded) +
                      "?f=json" + extra_params}},
        {{"rel", "collection"},
         {"type", OGCAPI_MIMETYPE_HTML},
         {"title", "This collection as HTML"},
         {"href", api_root + "/collections/" + std::string(id_encoded) +
                      "?f=html" + extra_params}}};

    msFree(id_encoded);

    outputResponse(
        map, request,
        format == OGCAPIFormat::JSON ? OGCAPIFormat::GeoJSON : format,
        OGCAPI_TEMPLATE_HTML_COLLECTION_ITEM, response, extraHeaders);
  } else {
    outputResponse(
        map, request,
        format == OGCAPIFormat::JSON ? OGCAPIFormat::GeoJSON : format,
        OGCAPI_TEMPLATE_HTML_COLLECTION_ITEMS, response, extraHeaders);
  }
  return MS_SUCCESS;
}

std::pair<const char *, const char *>
getQueryableTypeAndFormat(const layerObj *layer, const std::string &item) {
  const char *format = nullptr;
  const char *type = "string";
  const char *pszType =
      msOWSLookupMetadata(&(layer->metadata), "OFG", (item + "_type").c_str());
  if (pszType != NULL && (strcasecmp(pszType, "Character") == 0))
    type = "string";
  else if (pszType != NULL && (strcasecmp(pszType, "Date") == 0)) {
    type = "string";
    format = "date";
  } else if (pszType != NULL && (strcasecmp(pszType, "Time") == 0)) {
    type = "string";
    format = "time";
  } else if (pszType != NULL && (strcasecmp(pszType, "DateTime") == 0)) {
    type = "string";
    format = "date-time";
  } else if (pszType != NULL && (strcasecmp(pszType, "Integer") == 0 ||
                                 strcasecmp(pszType, "Long") == 0))
    type = "integer";
  else if (pszType != NULL && (strcasecmp(pszType, "Real") == 0))
    type = "numeric";
  else if (pszType != NULL && (strcasecmp(pszType, "Boolean") == 0))
    type = "boolean";

  return {type, format};
}

static int processCollectionQueryablesRequest(mapObj *map,
                                              const cgiRequestObj *request,
                                              const char *collectionId,
                                              OGCAPIFormat format) {

  // find the right layer
  const int iLayer = findLayerIndex(map, collectionId);

  if (iLayer < 0) { // invalid collectionId
    msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  layerObj *layer = map->layers[iLayer];
  if (!includeLayer(map, layer)) {
    msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  auto allowedParameters = getExtraParameters(map, layer);
  allowedParameters.insert("f");

  if (!msOOGCAPICheckQueryParameters(map, request, allowedParameters)) {
    return MS_SUCCESS;
  }

  bool error = false;
  const std::vector<std::string> queryableItems =
      msOOGCAPIGetLayerQueryables(layer, allowedParameters, error);
  if (error) {
    return MS_SUCCESS;
  }

  std::unique_ptr<char, decltype(&msFree)> id_encoded(msEncodeUrl(collectionId),
                                                      msFree);

  json response = {
      {"$schema", "https://json-schema.org/draft/2020-12/schema"},
      {"$id", msOGCAPIGetApiRootUrl(map, request) + "/collections/" +
                  std::string(id_encoded.get()) + "/queryables"},
      {"type", "object"},
      {"title", getCollectionTitle(layer)},
      {"description", getCollectionDescription(layer)},
      {"properties",
       {
#ifdef to_enable_once_we_support_geometry_requests
           {"shape",
            {
                {"title", "Geometry"},
                {"description", "The geometry of the collection."},
                {"x-ogc-role", "primary-geometry"},
                {"format", "geometry-any"},
            }},
#endif
       }},
      {"additionalProperties", false},
  };

  for (const std::string &item : queryableItems) {
    json j;
    const auto [type, format] = getQueryableTypeAndFormat(layer, item);
    j["description"] = "Queryable item '" + item + "'";
    j["type"] = type;
    if (format)
      j["format"] = format;
    response["properties"][item] = j;
  }

  std::map<std::string, std::vector<std::string>> extraHeaders;
  outputResponse(
      map, request,
      format == OGCAPIFormat::JSON ? OGCAPIFormat::JSONSchema : format,
      OGCAPI_TEMPLATE_HTML_COLLECTION_QUERYABLES, response, extraHeaders);

  return MS_SUCCESS;
}

static int processCollectionRequest(mapObj *map, cgiRequestObj *request,
                                    const char *collectionId,
                                    OGCAPIFormat format) {
  json response;
  int l;

  for (l = 0; l < map->numlayers; l++) {
    if (strcmp(map->layers[l]->name, collectionId) == 0)
      break; // match
  }

  if (l == map->numlayers) { // invalid collectionId
    msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  layerObj *layer = map->layers[l];
  auto allowedParameters = getExtraParameters(map, layer);
  allowedParameters.insert("f");
  if (!msOOGCAPICheckQueryParameters(map, request, allowedParameters)) {
    return MS_SUCCESS;
  }

  try {
    response =
        getCollection(map, layer, format, msOGCAPIGetApiRootUrl(map, request));
    if (response.is_null()) { // same as not found
      msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
      return MS_SUCCESS;
    }
  } catch (const std::runtime_error &e) {
    msOGCAPIOutputError(OGCAPI_CONFIG_ERROR,
                        "Error getting collection. " + std::string(e.what()));
    return MS_SUCCESS;
  }

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_COLLECTION,
                 response);
  return MS_SUCCESS;
}

static int processCollectionsRequest(mapObj *map, cgiRequestObj *request,
                                     OGCAPIFormat format) {
  json response;
  int i;

  auto allowedParameters = getExtraParameters(map, nullptr);
  allowedParameters.insert("f");
  if (!msOOGCAPICheckQueryParameters(map, request, allowedParameters)) {
    return MS_SUCCESS;
  }

  // define api root url
  std::string api_root = msOGCAPIGetApiRootUrl(map, request);
  const std::string extra_params = getExtraParameterString(map, nullptr);

  // build response object
  response = {{"links",
               {{{"rel", format == OGCAPIFormat::JSON ? "self" : "alternate"},
                 {"type", OGCAPI_MIMETYPE_JSON},
                 {"title", "This document as JSON"},
                 {"href", api_root + "/collections?f=json" + extra_params}},
                {{"rel", format == OGCAPIFormat::HTML ? "self" : "alternate"},
                 {"type", OGCAPI_MIMETYPE_HTML},
                 {"title", "This document as HTML"},
                 {"href", api_root + "/collections?f=html" + extra_params}}}},
              {"collections", json::array()}};

  for (i = 0; i < map->numlayers; i++) {
    try {
      json collection = getCollection(map, map->layers[i], format, api_root);
      if (!collection.is_null())
        response["collections"].push_back(collection);
    } catch (const std::runtime_error &e) {
      msOGCAPIOutputError(OGCAPI_CONFIG_ERROR,
                          "Error getting collection." + std::string(e.what()));
      return MS_SUCCESS;
    }
  }

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_COLLECTIONS,
                 response);
  return MS_SUCCESS;
}

static int processApiRequest(mapObj *map, cgiRequestObj *request,
                             OGCAPIFormat format) {
  // Strongly inspired from
  // https://github.com/geopython/pygeoapi/blob/master/pygeoapi/openapi.py

  auto allowedParameters = getExtraParameters(map, nullptr);
  allowedParameters.insert("f");
  if (!msOOGCAPICheckQueryParameters(map, request, allowedParameters)) {
    return MS_SUCCESS;
  }

  json response;

  response = {
      {"openapi", "3.0.2"},
      {"tags", json::array()},
  };

  response["info"] = {
      {"title", getTitle(map)},
      {"version", getWebMetadata(map, "A", "version", "1.0.0")},
  };

  for (const char *item : {"description", "termsOfService"}) {
    const char *value = getWebMetadata(map, "AO", item, nullptr);
    if (value) {
      response["info"][item] = value;
    }
  }

  for (const auto &pair : {
           std::make_pair("name", "contactperson"),
           std::make_pair("url", "contacturl"),
           std::make_pair("email", "contactelectronicmailaddress"),
       }) {
    const char *value = getWebMetadata(map, "AO", pair.second, nullptr);
    if (value) {
      response["info"]["contact"][pair.first] = value;
    }
  }

  for (const auto &pair : {
           std::make_pair("name", "licensename"),
           std::make_pair("url", "licenseurl"),
       }) {
    const char *value = getWebMetadata(map, "AO", pair.second, nullptr);
    if (value) {
      response["info"]["license"][pair.first] = value;
    }
  }

  {
    const char *value = getWebMetadata(map, "AO", "keywords", nullptr);
    if (value) {
      response["info"]["x-keywords"] = value;
    }
  }

  json server;
  server["url"] = msOGCAPIGetApiRootUrl(map, request);

  {
    const char *value =
        getWebMetadata(map, "AO", "server_description", nullptr);
    if (value) {
      server["description"] = value;
    }
  }
  response["servers"].push_back(server);

  const std::string oapif_schema_base_url = msOWSGetSchemasLocation(map);
  const std::string oapif_yaml_url = oapif_schema_base_url +
                                     "/ogcapi/features/part1/1.0/openapi/"
                                     "ogcapi-features-1.yaml";
  const std::string oapif_part2_yaml_url = oapif_schema_base_url +
                                           "/ogcapi/features/part2/1.0/openapi/"
                                           "ogcapi-features-2.yaml";

  json paths;

  paths["/"]["get"] = {
      {"summary", "Landing page"},
      {"description", "Landing page"},
      {"tags", {"server"}},
      {"operationId", "getLandingPage"},
      {"parameters",
       {
           {{"$ref", "#/components/parameters/f"}},
       }},
      {"responses",
       {{"200",
         {{"$ref", oapif_yaml_url + "#/components/responses/LandingPage"}}},
        {"400",
         {{"$ref",
           oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
        {"500",
         {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}}}}}};

  paths["/api"]["get"] = {
      {"summary", "API documentation"},
      {"description", "API documentation"},
      {"tags", {"server"}},
      {"operationId", "getOpenapi"},
      {"parameters",
       {
           {{"$ref", "#/components/parameters/f"}},
       }},
      {"responses",
       {{"200", {{"$ref", "#/components/responses/200"}}},
        {"400",
         {{"$ref",
           oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
        {"default", {{"$ref", "#/components/responses/default"}}}}}};

  paths["/conformance"]["get"] = {
      {"summary", "API conformance definition"},
      {"description", "API conformance definition"},
      {"tags", {"server"}},
      {"operationId", "getConformanceDeclaration"},
      {"parameters",
       {
           {{"$ref", "#/components/parameters/f"}},
       }},
      {"responses",
       {{"200",
         {{"$ref",
           oapif_yaml_url + "#/components/responses/ConformanceDeclaration"}}},
        {"400",
         {{"$ref",
           oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
        {"500",
         {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}}}}}};

  paths["/collections"]["get"] = {
      {"summary", "Collections"},
      {"description", "Collections"},
      {"tags", {"server"}},
      {"operationId", "getCollections"},
      {"parameters",
       {
           {{"$ref", "#/components/parameters/f"}},
       }},
      {"responses",
       {{"200",
         {{"$ref", oapif_yaml_url + "#/components/responses/Collections"}}},
        {"400",
         {{"$ref",
           oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
        {"500",
         {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}}}}}};

  for (int i = 0; i < map->numlayers; i++) {
    layerObj *layer = map->layers[i];
    if (!includeLayer(map, layer)) {
      continue;
    }

    json collection_get = {
        {"summary",
         std::string("Get ") + getCollectionTitle(layer) + " metadata"},
        {"description", getCollectionDescription(layer)},
        {"tags", {layer->name}},
        {"operationId", "describe" + std::string(layer->name) + "Collection"},
        {"parameters",
         {
             {{"$ref", "#/components/parameters/f"}},
         }},
        {"responses",
         {{"200",
           {{"$ref", oapif_yaml_url + "#/components/responses/Collection"}}},
          {"400",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
          {"500",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/ServerError"}}}}}};

    std::string collectionNamePath("/collections/");
    collectionNamePath += layer->name;
    paths[collectionNamePath]["get"] = std::move(collection_get);

    // check metadata, layer then map
    const char *max_limit_str =
        msOWSLookupMetadata(&(layer->metadata), "A", "max_limit");
    if (max_limit_str == nullptr)
      max_limit_str =
          msOWSLookupMetadata(&(map->web.metadata), "A", "max_limit");
    const int max_limit =
        max_limit_str ? atoi(max_limit_str) : OGCAPI_MAX_LIMIT;
    const int default_limit = getDefaultLimit(map, layer);

    json items_get = {
        {"summary", std::string("Get ") + getCollectionTitle(layer) + " items"},
        {"description", getCollectionDescription(layer)},
        {"tags", {layer->name}},
        {"operationId", "get" + std::string(layer->name) + "Features"},
        {"parameters",
         {
             {{"$ref", "#/components/parameters/f"}},
             {{"$ref", oapif_yaml_url + "#/components/parameters/bbox"}},
             {{"$ref", oapif_yaml_url + "#/components/parameters/datetime"}},
             {{"$ref",
               oapif_part2_yaml_url + "#/components/parameters/bbox-crs"}},
             {{"$ref", oapif_part2_yaml_url + "#/components/parameters/crs"}},
             {{"$ref", "#/components/parameters/offset"}},
             {{"$ref", "#/components/parameters/vendorSpecificParameters"}},
         }},
        {"responses",
         {{"200",
           {{"$ref", oapif_yaml_url + "#/components/responses/Features"}}},
          {"400",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
          {"500",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/ServerError"}}}}}};

    json param_limit = {
        {"name", "limit"},
        {"in", "query"},
        {"description", "The optional limit parameter limits the number of "
                        "items that are presented in the response document."},
        {"required", false},
        {"schema",
         {
             {"type", "integer"},
             {"minimum", 1},
             {"maximum", max_limit},
             {"default", default_limit},
         }},
        {"style", "form"},
        {"explode", false},
    };
    items_get["parameters"].emplace_back(param_limit);

    bool error = false;
    auto reservedParams = getExtraParameters(map, layer);
    reservedParams.insert("f");
    reservedParams.insert("bbox");
    reservedParams.insert("bbox-crs");
    reservedParams.insert("datetime");
    reservedParams.insert("limit");
    reservedParams.insert("offset");
    reservedParams.insert("crs");
    const std::vector<std::string> queryableItems =
        msOOGCAPIGetLayerQueryables(layer, reservedParams, error);
    for (const auto &item : queryableItems) {
      const auto [type, format] = getQueryableTypeAndFormat(layer, item);
      json queryable_param = {
          {"name", item},
          {"in", "query"},
          {"description", "Queryable item '" + item + "'"},
          {"required", false},
          {"schema",
           {
               {"type", type},
           }},
          {"style", "form"},
          {"explode", false},
      };
      if (format) {
        queryable_param["schema"]["format"] = format;
      }
      items_get["parameters"].emplace_back(queryable_param);
    }

    std::string itemsPath(collectionNamePath + "/items");
    paths[itemsPath]["get"] = std::move(items_get);

    json queryables_get = {
        {"summary",
         std::string("Get ") + getCollectionTitle(layer) + " queryables"},
        {"description",
         std::string("Get ") + getCollectionTitle(layer) + " queryables"},
        {"tags", {layer->name}},
        {"operationId", "get" + std::string(layer->name) + "Queryables"},
        {"parameters",
         {
             {{"$ref", "#/components/parameters/f"}},
             {{"$ref", "#/components/parameters/vendorSpecificParameters"}},
         }},
        {"responses",
         {{"200",
           {{"description", "The queryable properties of the collection."},
            {"content",
             {{"application/schema+json",
               {{"schema", {{"type", "object"}}}}}}}}},
          {"400",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
          {"500",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/ServerError"}}}}}};

    std::string queryablesPath(collectionNamePath + "/queryables");
    paths[queryablesPath]["get"] = std::move(queryables_get);

    json feature_id_get = {
        {"summary",
         std::string("Get ") + getCollectionTitle(layer) + " item by id"},
        {"description", getCollectionDescription(layer)},
        {"tags", {layer->name}},
        {"operationId", "get" + std::string(layer->name) + "Feature"},
        {"parameters",
         {
             {{"$ref", "#/components/parameters/f"}},
             {{"$ref", oapif_yaml_url + "#/components/parameters/featureId"}},
         }},
        {"responses",
         {{"200",
           {{"$ref", oapif_yaml_url + "#/components/responses/Feature"}}},
          {"400",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/InvalidParameter"}}},
          {"404",
           {{"$ref", oapif_yaml_url + "#/components/responses/NotFound"}}},
          {"500",
           {{"$ref",
             oapif_yaml_url + "#/components/responses/ServerError"}}}}}};
    std::string itemsFeatureIdPath(collectionNamePath + "/items/{featureId}");
    paths[itemsFeatureIdPath]["get"] = std::move(feature_id_get);
  }

  response["paths"] = std::move(paths);

  json components;
  components["responses"]["200"] = {{"description", "successful operation"}};
  components["responses"]["default"] = {
      {"description", "unexpected error"},
      {"content",
       {{"application/json",
         {{"schema",
           {{"$ref", "https://schemas.opengis.net/ogcapi/common/part1/1.0/"
                     "openapi/schemas/exception.yaml"}}}}}}}};

  json parameters;
  parameters["f"] = {
      {"name", "f"},
      {"in", "query"},
      {"description", "The optional f parameter indicates the output format "
                      "which the server shall provide as part of the response "
                      "document.  The default format is GeoJSON."},
      {"required", false},
      {"schema",
       {{"type", "string"}, {"enum", {"json", "html"}}, {"default", "json"}}},
      {"style", "form"},
      {"explode", false},
  };

  parameters["offset"] = {
      {"name", "offset"},
      {"in", "query"},
      {"description",
       "The optional offset parameter indicates the index within the result "
       "set from which the server shall begin presenting results in the "
       "response document.  The first element has an index of 0 (default)."},
      {"required", false},
      {"schema",
       {
           {"type", "integer"},
           {"minimum", 0},
           {"default", 0},
       }},
      {"style", "form"},
      {"explode", false},
  };

  parameters["vendorSpecificParameters"] = {
      {"name", "vendorSpecificParameters"},
      {"in", "query"},
      {"description",
       "Additional \"free-form\" parameters that are not explicitly defined"},
      {"schema",
       {
           {"type", "object"},
           {"additionalProperties", true},
       }},
      {"style", "form"},
  };

  components["parameters"] = std::move(parameters);

  response["components"] = std::move(components);

  // TODO: "tags" array ?

  outputResponse(map, request,
                 format == OGCAPIFormat::JSON ? OGCAPIFormat::OpenAPI_V3
                                              : format,
                 OGCAPI_TEMPLATE_HTML_OPENAPI, response);
  return MS_SUCCESS;
}

#endif

OGCAPIFormat msOGCAPIGetOutputFormat(const cgiRequestObj *request) {
  OGCAPIFormat format; // all endpoints need a format
  const char *p = getRequestParameter(request, "f");

  // if f= query parameter is not specified, use HTTP Accept header if available
  if (p == nullptr) {
    const char *accept = getenv("HTTP_ACCEPT");
    if (accept) {
      if (strcmp(accept, "*/*") == 0)
        p = OGCAPI_MIMETYPE_JSON;
      else
        p = accept;
    }
  }

  if (p &&
      (strcmp(p, "json") == 0 || strstr(p, OGCAPI_MIMETYPE_JSON) != nullptr ||
       strstr(p, OGCAPI_MIMETYPE_GEOJSON) != nullptr ||
       strstr(p, OGCAPI_MIMETYPE_OPENAPI_V3) != nullptr)) {
    format = OGCAPIFormat::JSON;
  } else if (p && (strcmp(p, "html") == 0 ||
                   strstr(p, OGCAPI_MIMETYPE_HTML) != nullptr)) {
    format = OGCAPIFormat::HTML;
  } else if (p) {
    std::string errorMsg("Unsupported format requested: ");
    errorMsg += p;
    msOGCAPIOutputError(OGCAPI_PARAM_ERROR, errorMsg.c_str());
    format = OGCAPIFormat::Invalid;
  } else {
    format = OGCAPIFormat::HTML; // default for now
  }

  return format;
}

int msOGCAPIDispatchRequest(mapObj *map, cgiRequestObj *request) {
#ifdef USE_OGCAPI_SVR

  // make sure ogcapi requests are enabled for this map
  int status = msOWSRequestIsEnabled(map, NULL, "AO", "OGCAPI", MS_FALSE);
  if (status != MS_TRUE) {
    msSetError(MS_OGCAPIERR, "OGC API requests are not enabled.",
               "msCGIDispatchAPIRequest()");
    return MS_FAILURE; // let normal error handling take over
  }

  for (int i = 0; i < request->NumParams; i++) {
    for (int j = i + 1; j < request->NumParams; j++) {
      if (strcmp(request->ParamNames[i], request->ParamNames[j]) == 0) {
        std::string errorMsg("Query parameter ");
        errorMsg += request->ParamNames[i];
        errorMsg += " is repeated";
        msOGCAPIOutputError(OGCAPI_PARAM_ERROR, errorMsg.c_str());
        return MS_SUCCESS;
      }
    }
  }

  const OGCAPIFormat format = msOGCAPIGetOutputFormat(request);

  if (format == OGCAPIFormat::Invalid) {
    return MS_SUCCESS; // avoid any downstream MapServer processing
  }

  if (request->api_path_length == 2) {

    return processLandingRequest(map, request, format);

  } else if (request->api_path_length == 3) {

    if (strcmp(request->api_path[2], "conformance") == 0) {
      return processConformanceRequest(map, request, format);
    } else if (strcmp(request->api_path[2], "conformance.html") == 0) {
      return processConformanceRequest(map, request, OGCAPIFormat::HTML);
    } else if (strcmp(request->api_path[2], "collections") == 0) {
      return processCollectionsRequest(map, request, format);
    } else if (strcmp(request->api_path[2], "collections.html") == 0) {
      return processCollectionsRequest(map, request, OGCAPIFormat::HTML);
    } else if (strcmp(request->api_path[2], "api") == 0) {
      return processApiRequest(map, request, format);
    }

  } else if (request->api_path_length == 4) {

    if (strcmp(request->api_path[2], "collections") ==
        0) { // next argument (3) is collectionId
      return processCollectionRequest(map, request, request->api_path[3],
                                      format);
    }

  } else if (request->api_path_length == 5) {

    if (strcmp(request->api_path[2], "collections") == 0 &&
        strcmp(request->api_path[4], "items") ==
            0) { // middle argument (3) is the collectionId
      return processCollectionItemsRequest(map, request, request->api_path[3],
                                           NULL, format);
    } else if (strcmp(request->api_path[2], "collections") == 0 &&
               strcmp(request->api_path[4], "queryables") == 0) {
      return processCollectionQueryablesRequest(map, request,
                                                request->api_path[3], format);
    }

  } else if (request->api_path_length == 6) {

    if (strcmp(request->api_path[2], "collections") == 0 &&
        strcmp(request->api_path[4], "items") ==
            0) { // middle argument (3) is the collectionId, last argument (5)
                 // is featureId
      return processCollectionItemsRequest(map, request, request->api_path[3],
                                           request->api_path[5], format);
    }
  }

  msOGCAPIOutputError(OGCAPI_NOT_FOUND_ERROR, "Invalid API path.");
  return MS_SUCCESS; // avoid any downstream MapServer processing
#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.",
             "msOGCAPIDispatchRequest()");
  return MS_FAILURE;
#endif
}
