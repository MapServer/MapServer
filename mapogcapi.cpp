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
#include "mapgml.h"

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
#define OGCAPI_TEMPLATE_HTML_COLLECTION_ITEMS "collection-items.html"

#define OGCAPI_FORMAT_JSON 1
#define OGCAPI_FORMAT_GEOJSON 2
#define OGCAPI_FORMAT_HTML 3

#define OGCAPI_MIMETYPE_JSON "application/json"
#define OGCAPI_MIMETYPE_GEOJSON "application/geo+json"
#define OGCAPI_MIMETYPE_HTML "text/html"

#define OGCAPI_LIMIT_DEFAULT 10 // by specification
#define OGCAPI_LIMIT_MAXIMUM 10000

#ifdef USE_OGCAPI_SVR

/* prototypes */
static void processError(int code, std::string message, const char *routine);

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
** Returns the limit as an int - between 1 and OGCAPI_LIMIT_MAXIMUM. We always return a valid value...
*/
static int getLimit(mapObj *map, cgiRequestObj *request, int *limit)  
{
  int status;
  const char *p;

  p = getRequestParameter(request, "limit");
  if(!p) {
    *limit = OGCAPI_LIMIT_DEFAULT; // limit not explicitly set
  } else {
    status = msStringToInt(p, limit, 10);

    if(status != MS_SUCCESS)
      return MS_FAILURE;

    if(*limit <= 0) {
      *limit = OGCAPI_LIMIT_DEFAULT;
    } else {
      *limit = MS_MIN(*limit, OGCAPI_LIMIT_MAXIMUM);
    }
  }

  return MS_SUCCESS;
}

/*
** Returns the bbox in SRS of the map. If not set, the map's EXTENT is used.
*/
static int getBbox(mapObj *map, cgiRequestObj *request, rectObj *bbox) 
{
  int status;
  const char *p;

  p = getRequestParameter(request, "bbox");
  if(!p) {
    bbox->minx = map->extent.minx; // no projection necessary
    bbox->miny = map->extent.miny;
    bbox->maxx = map->extent.maxx;
    bbox->maxy = map->extent.maxy;
  } else {
    int ntokens = 0;
    char **tokens = NULL;
    double values[4];

    tokens = msStringSplit(p, ',', &ntokens);
    if(tokens == NULL || ntokens != 4) {
      msFreeCharArray(tokens, ntokens);
      return MS_FAILURE;
    }

    for(int i=0; i<4; i++) {
      status = msStringToDouble(tokens[i], &values[i]);
      if(status != MS_SUCCESS) {
        msFreeCharArray(tokens, ntokens);
        return MS_FAILURE;
      }
    }

    bbox->minx = values[0]; // assign
    bbox->miny = values[1];
    bbox->maxx = values[2];
    bbox->maxy = values[3];

    msFreeCharArray(tokens, ntokens); // done with tokens

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

json getConstant(gmlConstantObj *constant)
{
  json j; // empty (null)

  if(!constant) throw std::runtime_error("Null constant metadata.");
  if(!constant->value) return j;
  
  // initialize
  j = { { constant->name, constant->value } };  

  return j;
}

json getItem(gmlItemObj *item, char *value)
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

/*
** Return a GeoJSON representation of a shape.
*/
json getFeature(layerObj *layer, shapeObj *shape, gmlItemListObj *items, gmlConstantListObj *constants, gmlGroupListObj *groups)
{
  json feature; // empty (null)

  if(!layer || !shape) throw std::runtime_error("Null arguments.");

  // initialize
  feature = {
    { "type", "Feature" },
    { "properties", json::object() }
  };

  // id

  // properties

  for(int i=0; i<items->numitems; i++) {    
    if(msItemInGroups(items->items[i].name, groups) == MS_FALSE) {
      try {
        json item = getItem(&(items->items[i]), shape->values[i]);
        if(!item.is_null()) feature["properties"].insert(item.begin(), item.end());
      } catch (const std::runtime_error &e) {
	throw std::runtime_error("Error fetching item.");
      }
    }
  }

  for(int i=0; i<constants->numconstants; i++) {
    if(msItemInGroups(constants->constants[i].name, groups) == MS_FALSE) {
      try {
        json constant = getConstant(&(constants->constants[i]));
        if(!constant.is_null()) feature["properties"].insert(constant.begin(), constant.end());
      } catch (const std::runtime_error &e) {
        throw std::runtime_error("Error fetching constant.");
      }
    }
  }

  // geometry

  return feature;
}

json getCollection(mapObj *map, layerObj *layer, int format)
{
  json collection; // empty (null)
  rectObj bbox;

  if(!map || !layer) return collection;

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

  const char *description = msOWSLookupMetadata(&(layer->metadata), "A", "description");
  if(!description) description = msOWSLookupMetadata(&(layer->metadata), "OF", "abstract"); // fallback on abstract

  const char *title = msOWSLookupMetadata(&(layer->metadata), "AOF", "title");

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
          { "rel", format==OGCAPI_FORMAT_JSON?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "This collection as JSON" },
          { "href", api_root + "/collections/" + std::string(id_encoded) + "?f=json" }
        },{
          { "rel", format==OGCAPI_FORMAT_HTML?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_HTML },
          { "title", "This collection as HTML" },
          { "href", api_root + "/collections/" + std::string(id_encoded) + "?f=html" }
        },{
          { "rel", "items" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "Items for his collection as JSON" },
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
      const char *link = msOWSLookupMetadata(&(layer->metadata), "A", ("link_" + name).c_str()); 
      if(link) {
        try {  
          collection["links"].push_back(json::parse(link));
        } catch(...) {
          throw std::runtime_error("Error parsing custom link (link_" + name + ").");
        }
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
    processError(400, "Template rendering error. " + std::string(e.what()) + " (" + std::string(filename) + ").", "outputTemplate()");
    return;
  } catch(...) {
    processError(400, "General template handling error.", "outputTemplate()");
    return;
  }
}

/*
** Generic response outputr.
*/
static void outputResponse(mapObj *map, int format, const char *filename, json response)
{
  const char *directory = NULL;

  if(format == OGCAPI_FORMAT_JSON) {
    outputJson(response, OGCAPI_MIMETYPE_JSON);
  } else if(format == OGCAPI_FORMAT_GEOJSON) {
    outputJson(response, OGCAPI_MIMETYPE_GEOJSON);
  } else if(format == OGCAPI_FORMAT_HTML) {
    if((directory = getTemplateDirectory(map, "html_template_directory", "OGCAPI_HTML_TEMPLATE_DIRECTORY")) == NULL) {
      processError(400, "Template directory not set.", "outputResponse()");
      return; // bail
    }

    json j;

    j["response"] = response; // nest the response so we could write the whole object in the template

    // extend the JSON with a few things that we need for templating
    j["template"] = {
      { "path", msStringSplit(getenv("PATH_INFO"), '/') },
      { "api_root", getApiRootUrl(map) },
      { "title", getTitle(map) },
      { "tags", json::object() }
    };

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

    outputTemplate(directory, filename, j, OGCAPI_MIMETYPE_HTML);
  } else {
    processError(400, "Unsupported format requested.", "outputResponse()");
  }
}

/*
** Process stuff...
*/

/*
** Returns a JSON object using MapServer error codes and a description.
*/
static void processError(int code, std::string message, const char *routine)
{
  json j;

  msSetError(MS_OGCAPIERR, "%s", routine, message.c_str());

  j["code"] = code;
  j["description"] = msGetErrorString(" ");

  outputJson(j, OGCAPI_MIMETYPE_JSON);
}

static int processLandingRequest(mapObj *map, int format)
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
	  { "rel", format==OGCAPI_FORMAT_JSON?"self":"alternate" },
	  { "type", OGCAPI_MIMETYPE_JSON },
	  { "title", "This document as JSON" },
	  { "href", api_root + "?f=json" }
        },{
	  { "rel", format==OGCAPI_FORMAT_HTML?"self":"alternate" },
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
        }
      }
    }
  };

  // handle custom links (optional)
  const char *links = msOWSLookupMetadata(&(map->web.metadata), "A", "links");
  if(links) {
    std::vector<std::string> names = msStringSplit(links, ',');
    for(std::string name : names) {
      const char *link = msOWSLookupMetadata(&(map->web.metadata), "A", ("link_" + name).c_str()); 
      if(link) {
        try {  
          response["links"].push_back(json::parse(link));
        } catch(...) {
          // TODO: generate an error (see getCollection)
        }
      }
    }
  }

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_LANDING, response);
  return MS_SUCCESS;
}

static int processConformanceRequest(mapObj *map, int format)
{
  json response;
 
  // build response object
  response = {
    { "conformsTo", {
        "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
        "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections"
      }
    }
  };

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_CONFORMANCE, response);
  return MS_SUCCESS;
}

static int processCollectionItemsRequest(mapObj *map, const char *collectionId, const char *itemId, rectObj bbox, int limit, int format)
{
  json response;
  int i, j;
  layerObj *lp;

  for(i=0; i<map->numlayers; i++) {
    if(strcmp(map->layers[i]->name, collectionId) == 0) break; // match
  }

  if(i == map->numlayers) { // invalid collectionId
    processError(404, "Invalid collection.", "processCollectionItemsRequest()");
    return MS_SUCCESS;
  }

  lp = map->layers[i]; // for convenience
  lp->status = MS_ON; // force on (do we need to save and reset?)

  if(itemId) {
    // TODO
  } else { // bbox query
    map->query.type = MS_QUERY_BY_RECT;
    map->query.mode = MS_QUERY_MULTIPLE;
    map->query.layer = i;
    map->query.rect = bbox;
  }

  if(msExecuteQuery(map) != MS_SUCCESS) {
    processError(404, "Collection items query failed.", "processCollectionItemsRequest()");
    return MS_SUCCESS;
  }

  // build response object
  response = {
    { "type", "FeatureCollection" },
    { "numMatched", lp->resultcache->numresults },
    { "numReturned", MS_MIN(limit, lp->resultcache->numresults) },
    { "features", json::array() }
  };

  // features (items) - if found
  if(lp->resultcache && lp->resultcache->numresults > 0) {
    shapeObj shape;
    msInitShape(&shape);

    // we piggyback on GML configuration
    gmlItemListObj *items = msGMLGetItems(lp, "AG");
    gmlConstantListObj *constants = msGMLGetConstants(lp, "AG");
    gmlGroupListObj *groups = msGMLGetGroups(lp, "AG");    

    if(!items || !constants || !groups) {
      msGMLFreeItems(items);
      msGMLFreeConstants(constants);
      msGMLFreeGroups(groups);
      processError(400, "Error fetching layer attribute metadata.", "processCollectionItemsRequest()");
      return MS_SUCCESS;
    }

    // reprojection
    reprojectionObj *reprojector = NULL;
    if(msProjectionsDiffer(&(lp->projection), &(map->latlon))) {
      reprojector = msProjectCreateReprojector(&(lp->projection), &(map->projection));
      if(reprojector == NULL) {
	msGMLFreeItems(items);
	msGMLFreeConstants(constants);
	msGMLFreeGroups(groups);
        processError(400, "Error creating re-projector.", "processCollectionItemsRequest()");
	return MS_SUCCESS;
      }
    }

    int start = 0; // TODO: will need to pass in
    int end = start + MS_MIN(limit, lp->resultcache->numresults);

    for(i=start; i<end; i++) {
      int status = msLayerGetShape(lp, &shape, &(lp->resultcache->results[i]));
      if(status != MS_SUCCESS) {
	msGMLFreeItems(items);
        msGMLFreeConstants(constants);
        msGMLFreeGroups(groups);
        msProjectDestroyReprojector(reprojector);
	processError(400, "Error fetching feature.", "processCollectionItemsRequest()");
        return MS_SUCCESS;
      }

      try {
	json feature = getFeature(lp, &shape, items, constants, groups);
	if(!feature.is_null()) response["features"].push_back(feature);
      } catch (const std::runtime_error &e) {
        msGMLFreeItems(items);
        msGMLFreeConstants(constants);
        msGMLFreeGroups(groups);
        msProjectDestroyReprojector(reprojector);
	processError(400, "Error getting feature. " + std::string(e.what()), "processCollectionItemsRequest()");
	return MS_SUCCESS;
      }
    }

    msGMLFreeItems(items); // clean up
    msGMLFreeConstants(constants);
    msGMLFreeGroups(groups);
    msProjectDestroyReprojector(reprojector);
  }

  // links

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_COLLECTION_ITEMS, response);
  return MS_SUCCESS;
}

static int processCollectionRequest(mapObj *map, const char *collectionId, int format)
{
  json response;
  int l;

  for(l=0; l<map->numlayers; l++) {
    if(strcmp(map->layers[l]->name, collectionId) == 0) break; // match
  }

  if(l == map->numlayers) { // invalid collectionId
    processError(404, "Invalid collection.", "processCollectionRequest()");
    return MS_SUCCESS;
  }

  try {
    response = getCollection(map, map->layers[l], format);
    if(response.is_null()) { // same as not found
      processError(404, "Invalid collection.", "processCollectionRequest()");
      return MS_SUCCESS;
    }
  } catch (const std::runtime_error &e) {
    processError(400, "Error getting collection. " + std::string(e.what()), "processCollectionRequest()");
    return MS_SUCCESS;
  }

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_COLLECTION, response);
  return MS_SUCCESS;
}

static int processCollectionsRequest(mapObj *map, int format)
{
  json response;
  int i;

  // define api root url
  std::string api_root = getApiRootUrl(map);

  // build response object
  response = {
    { "links", {
        {
          { "rel", format==OGCAPI_FORMAT_JSON?"self":"alternate" },
          { "type", OGCAPI_MIMETYPE_JSON },
          { "title", "This document as JSON" },
          { "href", api_root + "/collections?f=json" }
        },{
          { "rel", format==OGCAPI_FORMAT_HTML?"self":"alternate" },
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
      processError(400, "Error getting collection." + std::string(e.what()), "processCollectionsRequest()");
      return MS_SUCCESS;
    }
  }

  outputResponse(map, format, OGCAPI_TEMPLATE_HTML_COLLECTIONS, response);
  return MS_SUCCESS;
}
#endif

int msOGCAPIDispatchRequest(mapObj *map, cgiRequestObj *request, char **api_path, int api_path_length)
{
  int format, limit;
  rectObj bbox;

  const char *p = getRequestParameter(request, "f");
  if(p && (strcmp(p, "json") == 0 || strcmp(p, OGCAPI_MIMETYPE_JSON) == 0)) {
    format = OGCAPI_FORMAT_JSON;
  } else if(p && (strcmp(p, "html") == 0 || strcmp(p, OGCAPI_MIMETYPE_HTML) == 0)) {
    format = OGCAPI_FORMAT_HTML;
  } else if(p) {
    processError(500, "Unsupported format requested.", "msOGCAPIDispatchRequest()");
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

  } else if(api_path_length == 5) {

    if(strcmp(api_path[3], "collections") == 0) { // next argument (4) is collectionId
      return processCollectionRequest(map, api_path[4], format);
    }

  } else if(api_path_length == 6) {

    if(strcmp(api_path[3], "collections") == 0 && strcmp(api_path[5], "items") == 0)  { // middle argument (4) is the collectionId
      
      if(getLimit(map, request, &limit) != MS_SUCCESS) {
        processError(500, "Bad value for limit.", "msOGCAPIDispatchRequest()");
        return MS_SUCCESS;
      }

      if(getBbox(map, request, &bbox) != MS_SUCCESS) {
	processError(500, "Bad value for bbox.", "msOGCAPIDispatchRequest()");
	return MS_SUCCESS;
      }
      
      return processCollectionItemsRequest(map, api_path[4], NULL, bbox, limit, format);
    }

  }

  processError(500, "Invalid API request.", "msOGCAPIDispatchRequest()");
  return MS_SUCCESS; // avoid any downstream MapServer processing
#else
  msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.", "msOGCAPIDispatchRequest()");
  return MS_FAILURE;
#endif
}
