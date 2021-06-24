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

#include "third-party/include_nlohmann_json.hpp"
#include "third-party/include_pantor_inja.hpp"

#include <string>
#include <iostream>

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
#define OGCAPI_TEMPLATE_HTML_OPENAPI "openapi.html"

enum class OGCAPIFormat
{
    JSON,
    GeoJSON,
    OpenAPI_V3,
    HTML
};

#define OGCAPI_MIMETYPE_JSON "application/json"
#define OGCAPI_MIMETYPE_GEOJSON "application/geo+json"
#define OGCAPI_MIMETYPE_OPENAPI_V3 "application/vnd.oai.openapi+json;version=3.0"
#define OGCAPI_MIMETYPE_HTML "text/html"

// TODO: it would make sense to offer an option to override this default limit. 10 is really small for JSON output
#define OGCAPI_DEFAULT_LIMIT 10 // by specification
#define OGCAPI_MAX_LIMIT 10000

#ifdef USE_OGCAPI_SVR

#define OGCAPI_SERVER_ERROR 0 // error codes
#define OGCAPI_CONFIG_ERROR 1
#define OGCAPI_PARAM_ERROR 2
#define OGCAPI_NOT_FOUND_ERROR 3
#define OGCAPI_NUM_ERROR_CODES 4

/*
** Returns a JSON object using and a description.
*/
static void outputError(int code, std::string description)
{
  if(code < 0 || code >= OGCAPI_NUM_ERROR_CODES) code = 0;

  json error_codes = {
    { {"text", "ServerError"}, {"status", "500"} },
    { {"text", "ConfigError"}, {"status", "500"} },
    { {"text", "InvalidParamterValue"}, {"status", "400"} },
    { {"text", "NotFound"}, {"status", "404"} }
  };

  json j = {
    { "code", error_codes[code]["text"] },
    { "description", description }
  };

  std::string status = error_codes[code]["status"];

  msIO_setHeader("Content-Type", "%s", OGCAPI_MIMETYPE_JSON);
  msIO_setHeader("Status", "%s", status.c_str());
  msIO_sendHeaders();
  msIO_printf("%s\n", j.dump().c_str());
}

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

static int getMaxLimit(mapObj *map, layerObj *layer)
{
  int max_limit = OGCAPI_MAX_LIMIT;
  const char *value;  

  // check metadata, layer then map
  value = msOWSLookupMetadata(&(layer->metadata), "A", "max_limit");
  if(value == NULL) value = msOWSLookupMetadata(&(map->web.metadata), "A", "max_limit");

  if(value != NULL) {
    int status = msStringToInt(value, &max_limit, 10);
    if(status != MS_SUCCESS) max_limit = OGCAPI_MAX_LIMIT; // conversion failed
  }

  return max_limit;
}

/*
** Returns the limit as an int - between 1 and getMaxLimit(). We always return a valid value...
*/
static int getLimit(mapObj *map, cgiRequestObj *request, layerObj *layer, int *limit)
{
  int status;
  const char *p;

  int max_limit;
  max_limit = getMaxLimit(map, layer);

  p = getRequestParameter(request, "limit");
  if(!p || (p && strlen(p) == 0)) { // missing or empty
    *limit = MS_MIN(OGCAPI_DEFAULT_LIMIT, max_limit); // max could be smaller than the default
  } else {
    status = msStringToInt(p, limit, 10);
    if(status != MS_SUCCESS)
      return MS_FAILURE;

    if(*limit <= 0) {
      *limit = MS_MIN(OGCAPI_DEFAULT_LIMIT, max_limit); // max could be smaller than the default
    } else {
      *limit = MS_MIN(*limit, max_limit);
    }
  }

  return MS_SUCCESS;
}

/*
** Returns the bbox in SRS of the map.
*/
static int getBbox(mapObj *map, cgiRequestObj *request, rectObj *bbox) 
{
  int status;
  const char *p;

  p = getRequestParameter(request, "bbox");
  if(!p || (p && strlen(p) == 0)) { // missing or empty - assign map->extent (no projection necessary)
    bbox->minx = map->extent.minx;
    bbox->miny = map->extent.miny;
    bbox->maxx = map->extent.maxx;
    bbox->maxy = map->extent.maxy;
  } else {
    const auto tokens = msStringSplit(p, ',');
    if(tokens.size() != 4) {
      return MS_FAILURE;
    }

    double values[4];
    for(int i=0; i<4; i++) {
      status = msStringToDouble(tokens[i].c_str(), &values[i]);
      if(status != MS_SUCCESS) {
        return MS_FAILURE;
      }
    }

    bbox->minx = values[0]; // assign
    bbox->miny = values[1];
    bbox->maxx = values[2];
    bbox->maxy = values[3];

    // TODO: validate bbox is well-formed and lat/lon

    // at the moment we are assuming the bbox is given in lat/lon
    status = msProjectRect(&map->latlon, &map->projection, bbox);
    if(status != MS_SUCCESS) return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/*
** Returns the template directory location or NULL if it isn't set.
*/
static const char *getTemplateDirectory(mapObj *map, const char *key, const char *envvar)
{
  const char *directory;

  // TODO: if directory is provided then perhaps we need to check for a trailing slash

  if((directory = msOWSLookupMetadata(&(map->web.metadata), "A", key)) != NULL) 
    return directory;
  else if((directory = getenv(envvar)) != NULL)
    return directory;
  else
    return NULL;
}

/*
** Returns the service title from oga_{key} and/or ows_{key} or a default value if not set.
*/
static const char *getWebMetadata(mapObj *map, const char* domain, const char* key, const char* defaultVal)
{
  const char *value;

  if((value = msOWSLookupMetadata(&(map->web.metadata), domain, key)) != NULL)
    return value;
  else
    return defaultVal;
}

/*
** Returns the service title from oga|ows_title or a default value if not set.
*/
static const char *getTitle(mapObj *map)
{
  return getWebMetadata(map, "OA", "title", OGCAPI_DEFAULT_TITLE);
}

/*
** Returns the API root URL from oga_onlineresource or builds a value if not set.
*/
static std::string getApiRootUrl(mapObj *map)
{
  const char *root;

  if((root = msOWSLookupMetadata(&(map->web.metadata), "A", "onlineresource")) != NULL)
    return std::string(root);
  else
    return "http://" + std::string(getenv("SERVER_NAME")) + ":" + std::string(getenv("SERVER_PORT")) + std::string(getenv("SCRIPT_NAME")) + std::string(getenv("PATH_INFO"));
}

static json getFeatureConstant(gmlConstantObj *constant)
{
  json j; // empty (null)

  if(!constant) throw std::runtime_error("Null constant metadata.");
  if(!constant->value) return j;
  
  // initialize
  j = { { constant->name, constant->value } };  

  return j;
}

static json getFeatureItem(gmlItemObj *item, char *value)
{
  json j; // empty (null)
  const char *key;

  if(!item) throw std::runtime_error("Null item metadata.");
  if(!item->visible) return j;

  if(item->alias)
    key = item->alias;
  else
    key = item->name;

  // initialize
  j = { { key, value } };

  return j;
}

// https://stackoverflow.com/questions/25925290/c-round-a-double-up-to-2-decimal-places
static double round_up(double value, int decimal_places) {
  const double multiplier = std::pow(10.0, decimal_places);
  return std::ceil(value * multiplier) / multiplier;
}

static json getFeatureGeometry(shapeObj *shape, int precision)
{
  json geometry; // empty (null)
  int *outerList=NULL, numOuterRings=0;

  if(!shape) throw std::runtime_error("Null shape.");

  switch(shape->type) {
  case(MS_SHAPE_POINT):
    if(shape->numlines == 0 || shape->line[0].numpoints == 0) // not enough info for a point
      return geometry;

    if(shape->line[0].numpoints == 1) {
      geometry["type"] = "Point";
      geometry["coordinates"] = { round_up(shape->line[0].point[0].x, precision), round_up(shape->line[0].point[0].y, precision) };
    } else {
      geometry["type"] = "MultiPoint";
      geometry["coordinates"] = json::array();
      for(int j=0; j<shape->line[0].numpoints; j++) {
        geometry["coordinates"].push_back( { round_up(shape->line[0].point[j].x, precision), round_up(shape->line[0].point[j].y, precision) } );
      }
    }
    break;
  case(MS_SHAPE_LINE):
    if(shape->numlines == 0 || shape->line[0].numpoints < 2) // not enough info for a line
      return geometry;

    if(shape->numlines == 1) {
      geometry["type"] = "LineString";
      geometry["coordinates"] = json::array();
      for(int j=0; j<shape->line[0].numpoints; j++) {
	geometry["coordinates"].push_back( { round_up(shape->line[0].point[j].x, precision), round_up(shape->line[0].point[j].y, precision) } );
      }
    } else {
      geometry["type"] = "MultiLineString";
      geometry["coordinates"] = json::array();
      for(int i=0; i<shape->numlines; i++) {
        json part = json::array();
        for(int j=0; j<shape->line[i].numpoints; j++) {
          part.push_back( { round_up(shape->line[i].point[j].x, precision), round_up(shape->line[i].point[j].y, precision) } );
        }
        geometry["coordinates"].push_back(part);
      }
    }
    break;
  case(MS_SHAPE_POLYGON):
    if(shape->numlines == 0 || shape->line[0].numpoints < 4) // not enough info for a polygon (first=last)
      return geometry;

    outerList = msGetOuterList(shape);
    if(outerList == NULL) throw std::runtime_error("Unable to allocate list of outer rings.");
    for(int k=0; k<shape->numlines; k++) {
      if(outerList[k] == MS_TRUE)
	numOuterRings++;
    }

    if(numOuterRings == 1) {
      geometry["type"] = "Polygon";
      geometry["coordinates"] = json::array();
      for(int i=0; i<shape->numlines; i++) {
        json part = json::array();
        for(int j=0; j<shape->line[i].numpoints; j++) {
          part.push_back( { round_up(shape->line[i].point[j].x, precision), round_up(shape->line[i].point[j].y, precision) } );
        }
        geometry["coordinates"].push_back(part);
      }
    } else {
      geometry["type"] = "MultiPolygon";
      geometry["coordinates"] = json::array();

      for(int k=0; k<shape->numlines; k++) {
        if(outerList[k] == MS_TRUE) { // outer ring: generate polygon and add to coordinates
          int *innerList = msGetInnerList(shape, k, outerList);
          if(innerList == NULL) {
            msFree(outerList);
            throw std::runtime_error("Unable to allocate list of inner rings.");
          }

          json polygon = json::array();
          for(int i=0; i<shape->numlines; i++) {
            if(i == k || outerList[i] == MS_TRUE) { // add outer ring (k) and any inner rings
              json part = json::array();
              for(int j=0; j<shape->line[i].numpoints; j++) {
                part.push_back( { round_up(shape->line[i].point[j].x, precision), round_up(shape->line[i].point[j].y, precision) } );
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
static json getFeature(layerObj *layer, shapeObj *shape, gmlItemListObj *items, gmlConstantListObj *constants, int geometry_precision)
{
  int i;
  json feature; // empty (null)

  if(!layer || !shape) throw std::runtime_error("Null arguments.");

  // initialize
  feature = {
    { "type", "Feature" },
    { "properties", json::object() }
  };

  // id
  const char *featureIdItem = msOWSLookupMetadata(&(layer->metadata), "AGFO", "featureid");
  if(featureIdItem == NULL) throw std::runtime_error("Missing required featureid metadata."); // should have been trapped earlier
  for(i=0; i<items->numitems; i++) {
    if(strcasecmp(featureIdItem, items->items[i].name) == 0) {
      feature["id"] = shape->values[i];
      break;
    }
  }

  if(i == items->numitems) throw std::runtime_error("Feature id not found.");

  // properties - build from items and constants, no group support for now

  for(int i=0; i<items->numitems; i++) {    
    try {
      json item = getFeatureItem(&(items->items[i]), shape->values[i]);
      if(!item.is_null()) feature["properties"].insert(item.begin(), item.end());
    } catch (const std::runtime_error &e) {
      throw std::runtime_error("Error fetching item.");
    }
  }

  for(int i=0; i<constants->numconstants; i++) {
    try {
      json constant = getFeatureConstant(&(constants->constants[i]));
      if(!constant.is_null()) feature["properties"].insert(constant.begin(), constant.end());
    } catch (const std::runtime_error &e) {
      throw std::runtime_error("Error fetching constant.");  
    }
  }

  // geometry
  try {
    json geometry = getFeatureGeometry(shape, geometry_precision);
    if(!geometry.is_null()) feature["geometry"] = geometry;
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Error fetching geometry.");
  }

  return feature;
}

static json getLink(hashTableObj *metadata, std::string name)
{
  json link;

  const char *href = msOWSLookupMetadata(metadata, "A", (name + "_href").c_str());
  if(!href) throw std::runtime_error("Missing required link href property.");

  const char *title = msOWSLookupMetadata(metadata, "A", (name + "_title").c_str());
  const char *type = msOWSLookupMetadata(metadata, "A", (name + "_type").c_str());

  link = {
    { "href", href },
    { "title", title?title:href },
    { "type", type?type:"text/html" }
  };

  return link;
}

static const char* getCollectionDescription(layerObj* layer)
{
  const char *description = msOWSLookupMetadata(&(layer->metadata), "A", "description");
  if(!description) description = msOWSLookupMetadata(&(layer->metadata), "OF", "abstract"); // fallback on abstract
  return description;
}

static const char* getCollectionTitle(layerObj* layer)
{
  return msOWSLookupMetadata(&(layer->metadata), "AOF", "title");
}

static json getCollection(mapObj *map, layerObj *layer, OGCAPIFormat format)
{
  json collection; // empty (null)
  rectObj bbox;

  if(!map || !layer) return collection;

  if(!msOWSRequestIsEnabled(map, layer, "AO", "OGCAPI", MS_FALSE) || !msWFSIsLayerSupported(layer)) return collection;

  // initialize some things
  std::string api_root = getApiRootUrl(map);

  if(msOWSGetLayerExtent(map, layer, "AOF", &bbox) == MS_SUCCESS) {
    if (layer->projection.numargs > 0)
      msOWSProjectToWGS84(&layer->projection, &bbox);
    else
      msOWSProjectToWGS84(&map->projection, &bbox);
  } else {
    throw std::runtime_error("Unable to get collection bounding box."); // might be too harsh since extent is optional
  }

  const char *description = getCollectionDescription(layer);
  const char *title = getCollectionTitle(layer);

  const char *id = layer->name;
  char *id_encoded = msEncodeUrl(id); // free after use

  // build collection object
  collection = {
    { "id", id },
    { "description", description?description:"" },
    { "title", title?title:"" },
    { "extent", {
        { "spatial", {
            { "bbox", {{ bbox.minx, bbox.miny, bbox.maxx, bbox.maxy }}},
            { "crs", "http://www.opengis.net/def/crs/OGC/1.3/CRS84" }
          }
        }
      }
    },
    { "links", {
        {
          { "rel", format==OGCAPIFormat::JSON?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "This collection as JSON" },
          { "href", api_root + "/collections/" + std::string(id_encoded) + "?f=json" }
        },{
          { "rel", format==OGCAPIFormat::HTML?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "This collection as HTML" },
          { "href", api_root + "/collections/" + std::string(id_encoded) + "?f=html" }
        },{
          { "rel", "items" },
          { "type", OGCAPI_MIMETYPE_GEOJSON },
          { "title", "Items for this collection as GeoJSON" },
          { "href", api_root + "/collections/" + std::string(id_encoded) + "/items?f=json" }
        },{
          { "rel", "items" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "Items for this collection as HTML" },
          { "href", api_root + "/collections/" + std::string(id_encoded) + "/items?f=html" }
        }

      }
    },
    { "itemType", "feature" }
  };

  msFree(id_encoded); // done

  // handle optional configuration (keywords and links)
  const char *value = msOWSLookupMetadata(&(layer->metadata), "A", "keywords");
  if(!value) value = msOWSLookupMetadata(&(layer->metadata), "OF", "keywordlist"); // fallback on keywordlist
  if(value) {
    std::vector<std::string> keywords = msStringSplit(value, ',');
    for(std::string keyword : keywords) {
      collection["keywords"].push_back(keyword);
    }
  }

  value = msOWSLookupMetadata(&(layer->metadata), "A", "links");
  if(value) {
    std::vector<std::string> names = msStringSplit(value, ',');
    for(std::string name : names) {
      try {
        json link = getLink(&(layer->metadata), name);
        collection["links"].push_back(link);
      } catch(const std::runtime_error &e) {
        throw e;
      }
    }
  }

  return collection;
}

/*
** Output stuff...
*/

static void outputJson(json j, const char *mimetype)
{
  msIO_setHeader("Content-Type", "%s", mimetype);
  msIO_sendHeaders();
  msIO_printf("%s\n", j.dump().c_str());
}

static void outputTemplate(const char *directory, const char *filename, json j, const char *mimetype)
{
  std::string _directory(directory);
  std::string _filename(filename);
  Environment env {_directory}; // catch

  // somehow need to limit include processing to the directory

  // ERB-style instead of Mustache (we'll see)
  // env.set_expression("<%=", "%>");
  // env.set_statement("<%", "%>");

  // callbacks, need:
  //   - match (regex)
  //   - contains (substring)
  //   - URL encode

  try {
    Template t = env.parse_template(_filename); // catch
    std::string result = env.render(t, j);

    msIO_setHeader("Content-Type", "%s", mimetype);
    msIO_sendHeaders();
    msIO_printf("%s\n", result.c_str()); 
  } catch(const inja::RenderError &e) {
    outputError(OGCAPI_CONFIG_ERROR, "Template rendering error. " + std::string(e.what()) + " (" + std::string(filename) + ").");
    return;
  } catch(...) {
    outputError(OGCAPI_SERVER_ERROR, "General template handling error.");
    return;
  }
}

/*
** Generic response outputr.
*/
static void outputResponse(mapObj *map, cgiRequestObj *request, OGCAPIFormat format, const char *filename, json response)
{
  const char *path = NULL;
  char fullpath[MS_MAXPATHLEN];

  if(format == OGCAPIFormat::JSON) {
    outputJson(response, OGCAPI_MIMETYPE_JSON);
  } else if(format == OGCAPIFormat::GeoJSON) {
    outputJson(response, OGCAPI_MIMETYPE_GEOJSON);
  } else if(format == OGCAPIFormat::OpenAPI_V3) {
    outputJson(response, OGCAPI_MIMETYPE_OPENAPI_V3);
  } else if(format == OGCAPIFormat::HTML) {
    if((path = getTemplateDirectory(map, "html_template_directory", "OGCAPI_HTML_TEMPLATE_DIRECTORY")) == NULL) {
      outputError(OGCAPI_CONFIG_ERROR, "Template directory not set.");
      return; // bail
    }
    msBuildPath(fullpath, map->mappath, path);

    json j;

    j["response"] = response; // nest the response so we could write the whole object in the template

    // extend the JSON with a few things that we need for templating
    j["template"] = {
      { "path", json::array() },
      { "params", json::object() },
      { "api_root", getApiRootUrl(map) },
      { "title", getTitle(map) },
      { "tags", json::object() }
    };

    // api path
    for( int i=0; i<request->api_path_length; i++ )
      j["template"]["path"].push_back(request->api_path[i]);

    // parameters (optional)
    for( int i=0; i<request->NumParams; i++)
      j["template"]["params"].update({{ request->ParamNames[i], request->ParamValues[i] }});

    // add custom tags (optional)
    const char *tags = msOWSLookupMetadata(&(map->web.metadata), "A", "html_tags");
    if(tags) {
      std::vector<std::string> names = msStringSplit(tags, ',');
      for(std::string name : names) {
        const char *value = msOWSLookupMetadata(&(map->web.metadata), "A", ("tag_" + name).c_str());
        if(value) {
              j["template"]["tags"].update({{ name, value }}); // add object
        }
      }
    }

    outputTemplate(fullpath, filename, j, OGCAPI_MIMETYPE_HTML);
  } else {
    outputError(OGCAPI_PARAM_ERROR, "Unsupported format requested.");
  }
}

/*
** Process stuff...
*/
static int processLandingRequest(mapObj *map, cgiRequestObj *request, OGCAPIFormat format)
{
  json response;

  // define ambiguous elements
  const char *description = msOWSLookupMetadata(&(map->web.metadata), "A", "description");
  if(!description) description = msOWSLookupMetadata(&(map->web.metadata), "OF", "abstract"); // fallback on abstract if necessary

  // define api root url
  std::string api_root = getApiRootUrl(map);

  // build response object
  //   - consider conditionally excluding links for HTML format
  response =  {
    { "title", getTitle(map) },
    { "description", description?description:"" },
    { "links", {
        {
          { "rel", format==OGCAPIFormat::JSON?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "This document as JSON" },
          { "href", api_root + "?f=json" }
        },{
          { "rel", format==OGCAPIFormat::HTML?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "This document as HTML" },
          { "href", api_root + "?f=html" }
        },{
          { "rel", "data" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "OCG API conformance classes implemented by this server (JSON)" },
          { "href", api_root + "/conformance?f=json" }
        },{
          { "rel", "conformance" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "OCG API conformance classes implemented by this server" },
          { "href", api_root + "/conformance?f=html" }
        },{
          { "rel", "data" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "Information about feature collections available from this server (JSON)" },
          { "href", api_root + "/collections?f=json" }
        },{
          { "rel", "collections" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "Information about feature collections available from this server" },
          { "href", api_root + "/collections?f=html" }
        },{
          { "rel", "service-desc" },
          { "type", OGCAPI_MIMETYPE_OPENAPI_V3 },
          { "title", "OpenAPI document" },
          { "href", api_root + "/api?f=json" }
        },{
          { "rel", "service-doc" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "API documentation" },
          { "href", api_root + "/api?f=html" }
        }
      }
    }
  };

  // handle custom links (optional)
  const char *links = msOWSLookupMetadata(&(map->web.metadata), "A", "links");
  if(links) {
    std::vector<std::string> names = msStringSplit(links, ',');
    for(std::string name : names) {
      try {
        json link = getLink(&(map->web.metadata), name);
        response["links"].push_back(link);
      } catch(const std::runtime_error &e) {
        outputError(OGCAPI_CONFIG_ERROR, std::string(e.what()));
        return MS_SUCCESS;
      }
    }
  }

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_LANDING, response);
  return MS_SUCCESS;
}

static int processConformanceRequest(mapObj *map, cgiRequestObj *request, OGCAPIFormat format)
{
  json response;
 
  // build response object
  response = {
    { "conformsTo", {
        "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
        "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
        "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/core",
        "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/oas30",
        "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/html",
        "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/geojson",
      }
    }
  };

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_CONFORMANCE, response);
  return MS_SUCCESS;
}

static int processCollectionItemsRequest(mapObj *map, cgiRequestObj *request, const char *collectionId, const char *featureId, OGCAPIFormat format)
{
  json response;
  int i;
  layerObj *layer;

  int limit;
  rectObj bbox;

  int numberMatched = 0;

  // find the right layer
  for(i=0; i<map->numlayers; i++) {
    if(strcmp(map->layers[i]->name, collectionId) == 0) break; // match
  }

  if(i == map->numlayers) { // invalid collectionId
    outputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  layer = map->layers[i]; // for convenience
  layer->status = MS_ON; // force on (do we need to save and reset?)

  if(!msOWSRequestIsEnabled(map, layer, "AO", "OGCAPI", MS_FALSE) || !msWFSIsLayerSupported(layer)) {
    outputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  //
  // handle parameters specific to this endpoint
  //
  if(getLimit(map, request, layer, &limit) != MS_SUCCESS) {
    outputError(OGCAPI_PARAM_ERROR, "Bad value for limit.");
    return MS_SUCCESS;
  }

  if(getBbox(map, request, &bbox) != MS_SUCCESS) {
    outputError(OGCAPI_PARAM_ERROR, "Bad value for bbox.");
    return MS_SUCCESS;
  }

  int offset = 0;
  if(featureId) {
    const char *featureIdItem = msOWSLookupMetadata(&(layer->metadata), "AGFO", "featureid");
    if(featureIdItem == NULL) {
      outputError(OGCAPI_CONFIG_ERROR, "Missing required featureid metadata.");
      return MS_SUCCESS;
    }

    // TODO: does featureIdItem exist in the data?

    // optional validation
    const char *featureIdValidation = msLookupHashTable(&(layer->validation), featureIdItem);
    if(featureIdValidation && msValidateParameter(featureId, featureIdValidation, NULL, NULL, NULL) != MS_SUCCESS) {
      outputError(OGCAPI_NOT_FOUND_ERROR, "Invalid feature id.");
      return MS_SUCCESS;
    }

    map->query.type = MS_QUERY_BY_FILTER;
    map->query.mode = MS_QUERY_SINGLE;
    map->query.layer = i;
    map->query.rect = bbox;
    map->query.filteritem = strdup(featureIdItem);

    msInitExpression(&map->query.filter);
    map->query.filter.type = MS_STRING;
    map->query.filter.string = strdup(featureId);

    if(msExecuteQuery(map) != MS_SUCCESS) {
      outputError(OGCAPI_NOT_FOUND_ERROR, "Collection items id query failed.");
      return MS_SUCCESS;
    }

    if(!layer->resultcache || layer->resultcache->numresults != 1) {
      outputError(OGCAPI_NOT_FOUND_ERROR, "Collection items id query failed.");
      return MS_SUCCESS;
    }
  } else { // bbox query
    map->query.type = MS_QUERY_BY_RECT;
    map->query.mode = MS_QUERY_MULTIPLE;
    map->query.layer = i;
    map->query.rect = bbox;
    map->query.only_cache_result_count = MS_TRUE;

    // get number matched
    if(msExecuteQuery(map) != MS_SUCCESS) {
      outputError(OGCAPI_NOT_FOUND_ERROR, "Collection items query failed.");
      return MS_SUCCESS;
    }

    if(!layer->resultcache) {
      outputError(OGCAPI_NOT_FOUND_ERROR, "Collection items query failed.");
      return MS_SUCCESS;
    }

    numberMatched = layer->resultcache->numresults;

    if( numberMatched > 0 ) {
        map->query.only_cache_result_count = MS_FALSE;
        map->query.maxfeatures = limit;

        const char* offsetStr = getRequestParameter(request, "offset");
        if( offsetStr )
        {
            if( msStringToInt(offsetStr, &offset, 10) != MS_SUCCESS )
            {
              outputError(OGCAPI_PARAM_ERROR, "Bad value fo offset");
              return MS_SUCCESS;
            }

            // msExecuteQuery() use a 1-based offst convention, whereas the API
            // uses a 0-based offset convention.
            map->query.startindex = 1 + offset;
        }

        if(msExecuteQuery(map) != MS_SUCCESS) {
          outputError(OGCAPI_NOT_FOUND_ERROR, "Collection items query failed.");
          return MS_SUCCESS;
        }
    }
  }

  // build response object
  if(!featureId) {

    std::string api_root = getApiRootUrl(map);
    const char *id = layer->name;
    char *id_encoded = msEncodeUrl(id); // free after use

    std::string extra_kvp = "&limit=" + std::to_string(limit);
    extra_kvp += "&offset=" + std::to_string(offset);

    response = {
      { "type", "FeatureCollection" },
      { "numberMatched", numberMatched },
      { "numberReturned", layer->resultcache->numresults },
      { "features", json::array() },
      { "links", {
          {
              { "rel", format==OGCAPIFormat::JSON?"self":"alternate" },
              { "type", OGCAPI_MIMETYPE_GEOJSON },
              { "title", "Items for this collection as GeoJSON" },
              { "href", api_root + "/collections/" + std::string(id_encoded) + "/items?f=json" + extra_kvp }
          },{
              { "rel", format==OGCAPIFormat::HTML?"self":"alternate" },
              { "type", OGCAPI_MIMETYPE_HTML },
              { "title", "Items for this collection as HTML" },
              { "href", api_root + "/collections/" + std::string(id_encoded) + "/items?f=html" + extra_kvp }
          }
      }}
    };

    if( offset + layer->resultcache->numresults < numberMatched )
    {
        response["links"].push_back({
            { "rel", "next" },
            { "type", format==OGCAPIFormat::JSON? OGCAPI_MIMETYPE_GEOJSON : OGCAPI_MIMETYPE_HTML },
            { "title", "next page" },
            { "href", api_root + "/collections/" + std::string(id_encoded) +
                      "/items?f=" + (format==OGCAPIFormat::JSON? "json" : "html") +
                      "&limit=" + std::to_string(limit) +
                      "&offset=" + std::to_string(offset + limit) }
        });
    }

    msFree(id_encoded); // done
  }

  // features (items)
  {
    shapeObj shape;
    msInitShape(&shape);

    // we piggyback on GML configuration
    gmlItemListObj *items = msGMLGetItems(layer, "AG");
    gmlConstantListObj *constants = msGMLGetConstants(layer, "AG");

    if(!items || !constants) {
      msGMLFreeItems(items);
      msGMLFreeConstants(constants);
      outputError(OGCAPI_SERVER_ERROR, "Error fetching layer attribute metadata.");
      return MS_SUCCESS;
    }

    int geometry_precision = 6; // default
    if(msOWSLookupMetadata(&(layer->metadata), "AF", "geometry_precision")) {
      geometry_precision = atoi(msOWSLookupMetadata(&(layer->metadata), "AF", "geometry_precision"));
    } else if(msOWSLookupMetadata(&map->web.metadata, "AF", "geometry_precision")) {
      geometry_precision = atoi(msOWSLookupMetadata(&map->web.metadata, "AF", "geometry_precision"));
    }

    // reprojection (layer projection to EPSG:4326)
    reprojectionObj *reprojector = NULL;
    if(msProjectionsDiffer(&(layer->projection), &(map->latlon))) {
      reprojector = msProjectCreateReprojector(&(layer->projection), &(map->latlon));
      if(reprojector == NULL) {
        msGMLFreeItems(items);
        msGMLFreeConstants(constants);
        outputError(OGCAPI_SERVER_ERROR, "Error creating re-projector.");
        return MS_SUCCESS;
      }
    }

    for(i=0; i<layer->resultcache->numresults; i++) {
      int status = msLayerGetShape(layer, &shape, &(layer->resultcache->results[i]));
      if(status != MS_SUCCESS) {
        msGMLFreeItems(items);
        msGMLFreeConstants(constants);
        msProjectDestroyReprojector(reprojector);
        outputError(OGCAPI_SERVER_ERROR, "Error fetching feature.");
        return MS_SUCCESS;
      }

      if(reprojector) {
        status = msProjectShapeEx(reprojector, &shape);
        if(status != MS_SUCCESS) {
          msGMLFreeItems(items);
          msGMLFreeConstants(constants);
          msProjectDestroyReprojector(reprojector);
          msFreeShape(&shape);
          outputError(OGCAPI_SERVER_ERROR, "Error reprojecting feature.");
          return MS_SUCCESS;
        }
      }

      try {
        json feature = getFeature(layer, &shape, items, constants, geometry_precision);
        if(featureId) {
          response = feature;
        } else {
          response["features"].push_back(feature);
        }
      } catch (const std::runtime_error &e) {
        msGMLFreeItems(items);
        msGMLFreeConstants(constants);
        msProjectDestroyReprojector(reprojector);
        msFreeShape(&shape);
        outputError(OGCAPI_SERVER_ERROR, "Error getting feature. " + std::string(e.what()));
        return MS_SUCCESS;
      }

      msFreeShape(&shape); // next
    }

    msGMLFreeItems(items); // clean up
    msGMLFreeConstants(constants);
    msProjectDestroyReprojector(reprojector);
  }

  // extend the response a bit for templating (HERE)
  if(format == OGCAPIFormat::HTML) {
    const char *title = msOWSLookupMetadata(&(layer->metadata), "AOF", "title");
    const char *id = layer->name;
    response["collection"] = {
      { "id", id },
      { "title", title?title:"" }
    };
  }

  // links

  if(featureId)
    outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_COLLECTION_ITEM, response);
  else
    outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_COLLECTION_ITEMS, response);
  return MS_SUCCESS;
}

static int processCollectionRequest(mapObj *map, cgiRequestObj *request, const char *collectionId, OGCAPIFormat format)
{
  json response;
  int l;

  for(l=0; l<map->numlayers; l++) {
    if(strcmp(map->layers[l]->name, collectionId) == 0) break; // match
  }

  if(l == map->numlayers) { // invalid collectionId
    outputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
    return MS_SUCCESS;
  }

  try {
    response = getCollection(map, map->layers[l], format);
    if(response.is_null()) { // same as not found
      outputError(OGCAPI_NOT_FOUND_ERROR, "Invalid collection.");
      return MS_SUCCESS;
    }
  } catch (const std::runtime_error &e) {
    outputError(OGCAPI_CONFIG_ERROR, "Error getting collection. " + std::string(e.what()));
    return MS_SUCCESS;
  }

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_COLLECTION, response);
  return MS_SUCCESS;
}

static int processCollectionsRequest(mapObj *map, cgiRequestObj *request, OGCAPIFormat format)
{
  json response;
  int i;

  // define api root url
  std::string api_root = getApiRootUrl(map);

  // build response object
  response = {
    { "links", {
        {
          { "rel", format==OGCAPIFormat::JSON?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "This document as JSON" },
          { "href", api_root + "/collections?f=json" }
        },{
          { "rel", format==OGCAPIFormat::HTML?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "This document as HTML" },
          { "href", api_root + "/collections?f=html" }
        }
      }
    },{
      "collections", json::array()
    }
  };

  for(i=0; i<map->numlayers; i++) {
    try {
      json collection = getCollection(map, map->layers[i], format);
      if(!collection.is_null()) response["collections"].push_back(collection);
    } catch (const std::runtime_error &e) {
      outputError(OGCAPI_CONFIG_ERROR, "Error getting collection." + std::string(e.what()));
      return MS_SUCCESS;
    }
  }

  outputResponse(map, request, format, OGCAPI_TEMPLATE_HTML_COLLECTIONS, response);
  return MS_SUCCESS;
}

static int processApiRequest(mapObj *map, cgiRequestObj *request, OGCAPIFormat format)
{
  // Strongly inspired from https://github.com/geopython/pygeoapi/blob/master/pygeoapi/openapi.py

  json response;

  response = {
      { "openapi", "3.0.2" },
      { "tags", json::array() },
  };

  response["info"] = {
      { "title", getTitle(map) },
      { "version", getWebMetadata(map, "A", "version", "1.0.0") },
  };

  for( const char* item: { "description", "termsOfService" }) {
      const char* value = getWebMetadata(map, "AO", item, nullptr);
      if( value ) {
          response["info"][item] = value;
      }
  }

  for( const auto& pair: {
          std::make_pair("name", "contactperson"),
          std::make_pair("url", "contacturl"),
          std::make_pair("email", "contactelectronicmailaddress"),
        }) {
      const char* value = getWebMetadata(map, "AO", pair.second, nullptr);
      if( value ) {
          response["info"]["contact"][pair.first] = value;
      }
  }

  for( const auto& pair: {
          std::make_pair("name", "licensename"),
          std::make_pair("url", "licenseurl"),
        }) {
      const char* value = getWebMetadata(map, "AO", pair.second, nullptr);
      if( value ) {
          response["info"]["license"][pair.first] = value;
      }
  }

  {
      const char* value = getWebMetadata(map, "AO", "keywords", nullptr);
      if( value ) {
          response["info"]["x-keywords"] = value;
      }
  }

  json server;
  server["url"] = getApiRootUrl(map);
  {
      const char* value = getWebMetadata(map, "AO", "server_description", nullptr);
      if( value ) {
          server["description"] = value;
      }
  }
  response["servers"] = { server };

  const std::string oapif_yaml_url = "http://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/ogcapi-features-1.yaml";

  json paths;

  paths["/"]["get"] = {
      { "summary", "Landing page" },
      { "description", "Landing page" },
      { "tags", { "server" } },
      { "operationId", "getLandingPage" },
        { "parameters", {
          {{ "$ref", "#/components/parameters/f"}},
      }},
      { "responses", {
          { "200", {{"$ref", oapif_yaml_url + "#/components/responses/LandingPage"}} },
          { "400", {{"$ref", oapif_yaml_url + "#/components/responses/InvalidParameter"}} },
          { "500", {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}} }
      }}
  };

  paths["/api"]["get"] = {
      { "summary", "API documentation" },
      { "description", "API documentation" },
      { "tags", { "server" } },
      { "operationId", "getOpenapi" },
      { "parameters", {
          {{ "$ref", "#/components/parameters/f"}},
      }},
      { "responses", {
          { "200", {{"$ref", "#/components/responses/200"}} },
          { "400", {{"$ref", oapif_yaml_url + "#/components/responses/InvalidParameter"}} },
          { "default", {{"$ref", "#/components/responses/default"}} }
      }}
  };

  paths["/conformance"]["get"] = {
      { "summary", "API conformance definition" },
      { "description", "API conformance definition" },
      { "tags", { "server" } },
      { "operationId", "getConformanceDeclaration" },
      { "parameters", {
          {{ "$ref", "#/components/parameters/f"}},
      }},
      { "responses", {
          { "200", {{"$ref", oapif_yaml_url + "#/components/responses/ConformanceDeclaration"}} },
          { "400", {{"$ref", oapif_yaml_url + "#/components/responses/InvalidParameter"}} },
          { "500", {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}} }
      }}
  };

  paths["/collections"]["get"] = {
      { "summary", "Collections" },
      { "description", "Collections" },
      { "tags", { "server" } },
      { "operationId", "getCollections" },
      { "parameters", {
          {{ "$ref", "#/components/parameters/f"}},
      }},
      { "responses", {
          { "200", {{"$ref", oapif_yaml_url + "#/components/responses/Collections"}} },
          { "400", {{"$ref", oapif_yaml_url + "#/components/responses/InvalidParameter"}} },
          { "500", {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}} }
      }}
  };

  for(int i=0; i<map->numlayers; i++) {
      layerObj* layer = map->layers[i];
      if(!msOWSRequestIsEnabled(map, layer, "AO", "OGCAPI", MS_FALSE) ||
         !msWFSIsLayerSupported(layer))
      {
          continue;
      }

      json collection_get = {
          { "summary", std::string("Get ") + getCollectionTitle(layer) + " metadata" },
          { "description", getCollectionDescription(layer) },
          { "tags", { layer->name } },
          { "operationId", "describe" + std::string(layer->name) + "Collection" },
          { "parameters", {
              {{ "$ref", "#/components/parameters/f"}},
          }},
          { "responses", {
              { "200", {{"$ref", oapif_yaml_url + "#/components/responses/Collection"}} },
              { "400", {{"$ref", oapif_yaml_url + "#/components/responses/InvalidParameter"}} },
              { "500", {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}} }
          }}
      };

      std::string collectionNamePath("/collections/");
      collectionNamePath += layer->name;
      paths[collectionNamePath]["get"] = collection_get;

      // check metadata, layer then map
      const char* max_limit_str = msOWSLookupMetadata(&(layer->metadata), "A", "max_limit");
      if(max_limit_str == nullptr)
          max_limit_str = msOWSLookupMetadata(&(map->web.metadata), "A", "max_limit");
      const int max_limit = max_limit_str ? atoi(max_limit_str) : OGCAPI_MAX_LIMIT;
      const int default_limit = OGCAPI_DEFAULT_LIMIT;

      json items_get = {
          { "summary", std::string("Get ") + getCollectionTitle(layer) + " items" },
          { "description", getCollectionDescription(layer) },
          { "tags", { layer->name } },
          { "operationId", "get" + std::string(layer->name) + "Features" },
          { "parameters", {
              {{ "$ref", "#/components/parameters/f"}},
              {{ "$ref", oapif_yaml_url + "#/components/parameters/bbox"}},
              {{ "$ref", oapif_yaml_url + "#/components/parameters/datetime"}},
              {{ "$ref", "#/components/parameters/offset"}},
          }},
          { "responses", {
              { "200", {{"$ref", oapif_yaml_url + "#/components/responses/Features"}} },
              { "400", {{"$ref", oapif_yaml_url + "#/components/responses/InvalidParameter"}} },
              { "500", {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}} }
          }}
      };

      json param_limit = {
        { "name", "limit"},
        { "in", "query"},
        { "description", "The optional limit parameter limits the number of items that are presented in the response document."},
        { "required", false},
        { "schema", {
            { "type", "integer"},
            { "minimum", 1},
            { "maximum", max_limit},
            { "default", default_limit},
        }},
        { "style", "form" },
        { "explode", false },
      };
      items_get["parameters"].emplace_back(param_limit);

      std::string itemsPath(collectionNamePath + "/items");
      paths[itemsPath]["get"] = items_get;

      json feature_id_get = {
          { "summary", std::string("Get ") + getCollectionTitle(layer) + " item by id" },
          { "description", getCollectionDescription(layer) },
          { "tags", { layer->name } },
          { "operationId", "get" + std::string(layer->name) + "Feature" },
          { "parameters", {
              {{ "$ref", "#/components/parameters/f"}},
              {{ "$ref", oapif_yaml_url + "#/components/parameters/featureId"}},
          }},
          { "responses", {
              { "200", {{"$ref", oapif_yaml_url + "#/components/responses/Feature"}} },
              { "400", {{"$ref", oapif_yaml_url + "#/components/responses/InvalidParameter"}} },
              { "404", {{"$ref", oapif_yaml_url + "#/components/responses/NotFound"}} },
              { "500", {{"$ref", oapif_yaml_url + "#/components/responses/ServerError"}} }
          }}
      };
      std::string itemsFeatureIdPath(collectionNamePath + "/items/{featureId}");
      paths[itemsFeatureIdPath]["get"] = feature_id_get;
  }

  response["paths"] = paths;

  json components;
  components["responses"]["200"] = {
      { "description", "successful operation" }
  };
  components["responses"]["default"] = {
      { "description", "unexpected error" },
      { "content", {
          { "application/json", {
              { "schema", {
                  { "$ref", "https://raw.githubusercontent.com/opengeospatial/ogcapi-processes/master/core/openapi/schemas/exception.yaml" }
              }}
          }}
      }}
  };

  json parameters;
  parameters["f"] = {
    { "name", "f"},
    { "in", "query"},
    { "description", "The optional f parameter indicates the output format which the server shall provide as part of the response document.  The default format is GeoJSON."},
    { "required", false},
    { "schema", {
        { "type", "string"},
            {"enum", {"json", "html"}},
            {"default", "json"}
    }},
    { "style", "form" },
    { "explode", false },
  };

  parameters["offset"] = {
    { "name", "offset"},
    { "in", "query"},
    { "description", "The optional offset parameter indicates the index within the result set from which the server shall begin presenting results in the response document.  The first element has an index of 0 (default)."},
    { "required", false},
    { "schema", {
        { "type", "integer"},
        { "minimum", 0},
        { "default", 0},
    }},
    { "style", "form" },
    { "explode", false },
  };

  components["parameters"] = parameters;

  response["components"] = components;

  // TODO: "tags" array ?

  outputResponse(map, request,
                 format == OGCAPIFormat::JSON ? OGCAPIFormat::OpenAPI_V3 : format,
                 OGCAPI_TEMPLATE_HTML_OPENAPI, response);
  return MS_SUCCESS;
}

#endif

int msOGCAPIDispatchRequest(mapObj *map, cgiRequestObj *request)
{
#ifdef USE_OGCAPI_SVR

  // make sure ogcapi requests are enabled for this map
  int status = msOWSRequestIsEnabled(map, NULL, "AO", "OGCAPI", MS_FALSE);
  if(status != MS_TRUE) {
    msSetError(MS_OGCAPIERR, "OGC API requests are not enabled.", "msCGIDispatchAPIRequest()");
    return MS_FAILURE; // let normal error handling take over
  }

  OGCAPIFormat format; // all endpoints need a format
  const char *p = getRequestParameter(request, "f");

  // if f= query parameter is not specified, use HTTP Accept header if available
  if( p == nullptr )
  {
      const char* accept = getenv("HTTP_ACCEPT");
      if( accept )
      {
          if( strcmp(accept, "*/*") == 0 )
              p = OGCAPI_MIMETYPE_JSON;
          else
              p = accept;
      }
  }

  if(p && (strcmp(p, "json") == 0 ||
           strstr(p, OGCAPI_MIMETYPE_JSON) != nullptr ||
           strstr(p, OGCAPI_MIMETYPE_GEOJSON) != nullptr ||
           strstr(p, OGCAPI_MIMETYPE_OPENAPI_V3) != nullptr)) {
    format = OGCAPIFormat::JSON;
  } else if(p && (strcmp(p, "html") == 0 ||
                  strstr(p, OGCAPI_MIMETYPE_HTML) != nullptr)) {
    format = OGCAPIFormat::HTML;
  } else if(p) {
    std::string errorMsg("Unsupported format requested: ");
    errorMsg += p;
    outputError(OGCAPI_PARAM_ERROR, errorMsg.c_str());
    return MS_SUCCESS; // avoid any downstream MapServer processing
  } else {
    format = OGCAPIFormat::HTML; // default for now
  }

  if(request->api_path_length == 2) {

    return processLandingRequest(map, request, format);

  } else if(request->api_path_length == 3) {

    if(strcmp(request->api_path[2], "conformance") == 0) {
      return processConformanceRequest(map, request, format);
    } else if(strcmp(request->api_path[2], "conformance.html") == 0) {
      return processConformanceRequest(map, request, OGCAPIFormat::HTML);
    } else if(strcmp(request->api_path[2], "collections") == 0) {
      return processCollectionsRequest(map, request, format);
    } else if(strcmp(request->api_path[2], "collections.html") == 0) {
      return processCollectionsRequest(map, request, OGCAPIFormat::HTML);
    } else if(strcmp(request->api_path[2], "api") == 0) {
      return processApiRequest(map, request, format);
    }

  } else if(request->api_path_length == 4) {

    if(strcmp(request->api_path[2], "collections") == 0) { // next argument (3) is collectionId
      return processCollectionRequest(map, request, request->api_path[3], format);
    }

  } else if(request->api_path_length == 5) {

    if(strcmp(request->api_path[2], "collections") == 0 && strcmp(request->api_path[4], "items") == 0)  { // middle argument (3) is the collectionId
      return processCollectionItemsRequest(map, request, request->api_path[3], NULL, format);
    }

  } else if(request->api_path_length == 6) {

    if(strcmp(request->api_path[2], "collections") == 0 && strcmp(request->api_path[4], "items") == 0)  { // middle argument (3) is the collectionId, last argument (5) is featureId
      return processCollectionItemsRequest(map, request, request->api_path[3], request->api_path[5], format);
    }
  }

  outputError(OGCAPI_NOT_FOUND_ERROR, "Invalid API path.");
  return MS_SUCCESS; // avoid any downstream MapServer processing
#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.", "msOGCAPIDispatchRequest()");
  return MS_FAILURE;
#endif
}
