/**********************************************************************
 *
 * Project:  MapServer
 * Purpose:  OGCAPI Implementation
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
#include "mapserv-homepage.h" // why include self?
#include "maptemplate.h"
#include "mapows.h"

#include "cpl_conv.h"

 //#include <string>
 //#include <optional>
 //#include <unordered_map>


#include "third-party/include_nlohmann_json.hpp"
#include "third-party/include_pantor_inja.hpp"

using namespace inja;
using json = nlohmann::json;

/*
** HTML Templates
*/
#define TEMPLATE_HTML_HOMEPAGE "homepage.html"

enum class APIFormat { JSON, HTML };

#define OGCAPI_MIMETYPE_JSON "application/json"
#define OGCAPI_MIMETYPE_HTML "text/html"


int msReadConfig(mapservObj* mapserv, configObj* config) {
    if (!mapserv || !config) return -1; // Handle null pointers

    std::string mode = CPLGetConfigOption("MS_MODE", "");

    // Search for "mode" in request parameters
    for (int i = 0; i < mapserv->request->NumParams; i++) {
        if (strcasecmp(mapserv->request->ParamNames[i], "mode") == 0) {
            mode = mapserv->request->ParamValues[i];
            break;
        }
    }

    if (strcasecmp(mode.c_str(), "config") == 0) {
        // std::string key;
        const char* key = NULL;
        while (true) {
            key = msNextKeyFromHashTable(&config->maps, key);
            if (!key) {
                break; // No more keys
            }

            const char* pszValue = msLookupHashTable(&config->maps, key);
            if (pszValue) {
                mapObj* map = msLoadMap(pszValue, nullptr, config);
                if (!map) {
                    // return -1; // Handle error case
                }
                else {
                    // Perform operations with the map object here
                    // Free the map object to avoid memory leaks
                    msFreeMap(map);
                }
            }
        }
        return 0;
    }

    return -1; // If "mode" is not "config"
}


/*
** Generic response output.
*/
static void
outputResponse(mapObj* map, cgiRequestObj* request, APIFormat format,
    const char* filename, const json& response,
    const std::map<std::string, std::string>& extraHeaders =
    std::map<std::string, std::string>()) {
    std::string path;
    char fullpath[MS_MAXPATHLEN];

    if (format == APIFormat::JSON) {
        outputJson(response, OGCAPI_MIMETYPE_JSON, extraHeaders);
    }
    else if (format == APIFormat::HTML) {
        path = getTemplateDirectory(map, "html_template_directory",
            "OGCAPI_HTML_TEMPLATE_DIRECTORY");
        if (path.empty()) {
            outputError(OGCAPI_CONFIG_ERROR, "Template directory not set.");
            return; // bail
        }
        msBuildPath(fullpath, map->mappath, path.c_str());

        json j;

        j["response"] = response; // nest the response so we could write the whole
        // object in the template

// extend the JSON with a few things that we need for templating
        j["template"] = { {"path", json::array()},
                         {"params", json::object()},
                         {"api_root", "Test"},
                         {"title", "Test"},
                         {"tags", json::object()} };

        // api path
        for (int i = 0; i < request->api_path_length; i++)
            j["template"]["path"].push_back(request->api_path[i]);

        // parameters (optional)
        for (int i = 0; i < request->NumParams; i++) {
            if (request->ParamValues[i] &&
                strlen(request->ParamValues[i]) > 0) { // skip empty params
                j["template"]["params"].update(
                    { {request->ParamNames[i], request->ParamValues[i]} });
            }
        }

        // add custom tags (optional)
        const char* tags =
            msOWSLookupMetadata(&(map->web.metadata), "A", "html_tags");
        if (tags) {
            std::vector<std::string> names = msStringSplit(tags, ',');
            for (std::string name : names) {
                const char* value = msOWSLookupMetadata(&(map->web.metadata), "A",
                    ("tag_" + name).c_str());
                if (value) {
                    j["template"]["tags"].update({ {name, value} }); // add object
                }
            }
        }

        // outputTemplate(fullpath, filename, j, OGCAPI_MIMETYPE_HTML);
    }
    else {
        outputError(OGCAPI_PARAM_ERROR, "Unsupported format requested.");
    }
}

static int processLandingRequest(mapObj* map, cgiRequestObj* request,
    APIFormat format) {
    json response;

    // define ambiguous elements
    const char* description =
        msOWSLookupMetadata(&(map->web.metadata), "A", "description");
    if (!description)
        description =
        msOWSLookupMetadata(&(map->web.metadata), "OF",
            "abstract"); // fallback on abstract if necessary

    // define api root url
    // std::string api_root = getApiRootUrl(map);

    // build response object
    //   - consider conditionally excluding links for HTML format
    //response = {
    //    {"title", getTitle(map)},
    //    {"description", description ? description : ""},
    //    {"links",
    //     {{{"rel", format == APIFormat::JSON ? "self" : "alternate"},
    //       {"type", OGCAPI_MIMETYPE_JSON},
    //       {"title", "This document as JSON"},
    //       {"href", api_root + "?f=json"}},
    //      {{"rel", format == APIFormat::HTML ? "self" : "alternate"},
    //       {"type", OGCAPI_MIMETYPE_HTML},
    //       {"title", "This document as HTML"},
    //       {"href", api_root + "?f=html"}},
    //      {{"rel", "conformance"},
    //       {"type", OGCAPI_MIMETYPE_JSON},
    //       {"title",
    //        "OCG API conformance classes implemented by this server (JSON)"},
    //       {"href", api_root + "/conformance?f=json"}},
    //      {{"rel", "conformance"},
    //       {"type", OGCAPI_MIMETYPE_HTML},
    //       {"title", "OCG API conformance classes implemented by this server"},
    //       {"href", api_root + "/conformance?f=html"}},
    //      {{"rel", "data"},
    //       {"type", OGCAPI_MIMETYPE_JSON},
    //       {"title", "Information about feature collections available from this "
    //                 "server (JSON)"},
    //       {"href", api_root + "/collections?f=json"}},
    //      {{"rel", "data"},
    //       {"type", OGCAPI_MIMETYPE_HTML},
    //       {"title",
    //        "Information about feature collections available from this server"},
    //       {"href", api_root + "/collections?f=html"}},
    //      {{"rel", "service-doc"},
    //       {"type", OGCAPI_MIMETYPE_HTML},
    //       {"title", "API documentation"},
    //       {"href", api_root + "/api?f=html"}}}} };

    //// handle custom links (optional)
    //const char* links = msOWSLookupMetadata(&(map->web.metadata), "A", "links");
    //if (links) {
    //    std::vector<std::string> names = msStringSplit(links, ',');
    //    for (std::string name : names) {
    //        try {
    //            json link = getLink(&(map->web.metadata), name);
    //            response["links"].push_back(link);
    //        }
    //        catch (const std::runtime_error& e) {
    //            outputError(OGCAPI_CONFIG_ERROR, std::string(e.what()));
    //            return MS_SUCCESS;
    //        }
    //    }
    //}

    outputResponse(map, request, format, TEMPLATE_HTML_HOMEPAGE, response);
    return MS_SUCCESS;
}

//int msOGCAPIDispatchRequest(mapObj* map, cgiRequestObj* request) {
//    // #ifdef USE_OGCAPI_SVR
//
//        // make sure ogcapi requests are enabled for this map
//    int status = msOWSRequestIsEnabled(map, NULL, "AO", "OGCAPI", MS_FALSE);
//    if (status != MS_TRUE) {
//        msSetError(MS_OGCAPIERR, "OGC API requests are not enabled.",
//            "msCGIDispatchAPIRequest()");
//        return MS_FAILURE; // let normal error handling take over
//    }

    //for (int i = 0; i < request->NumParams; i++) {
    //    for (int j = i + 1; j < request->NumParams; j++) {
    //        if (strcmp(request->ParamNames[i], request->ParamNames[j]) == 0) {
    //            std::string errorMsg("Query parameter ");
    //            errorMsg += request->ParamNames[i];
    //            errorMsg += " is repeated";
    //            outputError(OGCAPI_PARAM_ERROR, errorMsg.c_str());
    //            return MS_SUCCESS;
    //        }
    //    }
    //}

    //OGCAPIFormat format; // all endpoints need a format
    //const char* p = getRequestParameter(request, "f");

    //// if f= query parameter is not specified, use HTTP Accept header if available
    //if (p == nullptr) {
    //    const char* accept = getenv("HTTP_ACCEPT");
    //    if (accept) {
    //        if (strcmp(accept, "*/*") == 0)
    //            p = OGCAPI_MIMETYPE_JSON;
    //        else
    //            p = accept;
    //    }
    //}

    //if (p &&
    //    (strcmp(p, "json") == 0 || strstr(p, OGCAPI_MIMETYPE_JSON) != nullptr) {
    //    format = OGCAPIFormat::JSON;
    //}
    //else if (p && (strcmp(p, "html") == 0 ||
    //    strstr(p, OGCAPI_MIMETYPE_HTML) != nullptr)) {
    //    format = OGCAPIFormat::HTML;
    //}
    //else if (p) {
    //    std::string errorMsg("Unsupported format requested: ");
    //    errorMsg += p;
    //    outputError(OGCAPI_PARAM_ERROR, errorMsg.c_str());
    //    return MS_SUCCESS; // avoid any downstream MapServer processing
    //}
    //else {
    //    format = OGCAPIFormat::HTML; // default for now
    //}

    ////if (request->api_path_length == 2) {

    //return processLandingRequest(map, request, format);

    // outputError(OGCAPI_NOT_FOUND_ERROR, "Invalid API path.");
    // return MS_SUCCESS; // avoid any downstream MapServer processing
    //#else
    //    msSetError(MS_OGCAPIERR, "OGC API server support is not enabled.",
    //        "msOGCAPIDispatchRequest()");
    //    return MS_FAILURE;
    //#endif
//}
