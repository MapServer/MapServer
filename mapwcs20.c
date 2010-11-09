/******************************************************************************
 * $Id: mapwcs20.c
 *
 * Project:  MapServer
 * Purpose:  OpenGIS Web Coverage Server (WCS) 2.0 implementation.
 * Author:   Stephan Meissl <stephan.meissl@eox.at>
 *           Fabian Schindler <fabian.schindler@eox.at>
 *
 ******************************************************************************
 * Copyright (c) 2010 EOX IT Services GmbH, Austria
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <assert.h>
#include "mapserver.h"
#include "maperror.h"
#include "mapthread.h"
#include "mapwcs.h"
#include <libxml/tree.h>
#include "maplibxml2.h"
#include <float.h>
#include "gdal.h"
#include "cpl_port.h"
#include "maptime.h"
#include "mapprimitive.h"
#include <libxml/parser.h>
#include "cpl_string.h"

/************************************************************************/
/*                   msWCSGetBandInterpretationString20()               */
/*                                                                      */
/*      Returns a descriptive string to a GDALColorInterp value.        */
/************************************************************************/

/************************************************************************/
/*                   msWCSParseDouble20()                               */
/*                                                                      */
/*      Tries to parse a string as a double value. If not possible      */
/*      the value MS_WCS20_UNBOUNDED is returned.                       */
/************************************************************************/

static double msWCSParseDouble20(const char* string)
{
    char *conversionCheck = NULL;
    double v;
    v = strtod(string, &conversionCheck);
    if (conversionCheck - strlen(string) != string)
    {
        return MS_WCS20_UNBOUNDED;
    }
    return v;
}

/************************************************************************/
/*                   msWCSParseTimeOrScalar20()                         */
/*                                                                      */
/*      Parses a string, either as a time or a scalar value and         */
/*      writes the output into the timeScalarUnion.                     */
/************************************************************************/

static int msWCSParseTimeOrScalar20(timeScalarUnion *u, const char *string)
{
    struct tm time;
    msDebug("msWCSParseTimeOrScalar20(): Parsing string %s\n", string);
    msStringTrim((char *) string);

    if (!string || strlen(string) == 0 || !u)
    {
        msSetError(MS_WCSERR, "Invalid string", "msWCSParseTimeOrScalar20()");
        return MS_WCS20_ERROR_VALUE;
    }
    /* if the string is equal to "*" it means the value
     *  of the interval is unbounded                    */
    if (EQUAL(string, "*"))
    {
        u->scalar = MS_WCS20_UNBOUNDED;
        u->unbounded = 1;
        return MS_WCS20_UNDEFINED_VALUE;
    }
    u->scalar = msWCSParseDouble20(string);
    /* if returned a valid value, use it */
    if (u->scalar != MS_WCS20_UNBOUNDED)
    {
        return MS_WCS20_SCALAR_VALUE;
    }
    /* otherwise it might be a time value */
    msTimeInit(&time);
    if (msParseTime(string, &time))
    {
        u->time = mktime(&time);
        return MS_WCS20_TIME_VALUE;
    }
    /* the value could neither be parsed as a double nor as a time value */
    else
    {
        msSetError(MS_WCSERR,
                "String %s could not be parsed to a time or scalar value",
                "msWCSParseTimeOrScalar20()");
        return MS_WCS20_ERROR_VALUE;
    }
}

/************************************************************************/
/*                   msWCSCreateSubsetObj20()                           */
/*                                                                      */
/*      Creates a new wcs20SubsetObj and initializes it to standard     */
/*      values.                                                         */
/************************************************************************/

static wcs20SubsetObj *msWCSCreateSubsetObj20()
{
    wcs20SubsetObj *subset = (wcs20SubsetObj *) malloc(sizeof(wcs20SubsetObj));
    if (subset)
    {
        subset->axis = NULL;
        subset->crs = NULL;
        subset->min.scalar = subset->max.scalar = MS_WCS20_UNBOUNDED;
        subset->min.unbounded = subset->max.unbounded = 0;
        subset->operation = MS_WCS20_SLICE;
    }
    else
        msSetError(MS_WCSERR, "Subset could not be created. ",
                "msWCSCreateSubsetObj20()");
    return subset;
}

/************************************************************************/
/*                   msWCSFreeSubsetObj20()                             */
/*                                                                      */
/*      Frees a wcs20SubsetObj and releases all linked resources.       */
/************************************************************************/

static void msWCSFreeSubsetObj20(wcs20SubsetObj *subset)
{
    if (!subset)
        return;
    msDebug("msWCSFreeSubsetObj20(): freeing subset for axis %s\n",
            subset->axis);
    msFree(subset->axis);
    msFree(subset->crs);
    msFree(subset);
}

/************************************************************************/
/*                   msWCSCreateParamsObj20()                           */
/*                                                                      */
/*      Creates a new wcs20ParamsObj and initializes it to standard     */
/*      values.                                                         */
/************************************************************************/

wcs20ParamsObj *msWCSCreateParamsObj20()
{
    wcs20ParamsObj *params = (wcs20ParamsObj *) malloc(sizeof(wcs20ParamsObj));
    if (params != NULL)
    {
        params->version = NULL; /* should be 2.0.0 */
        params->request = NULL;
        params->service = NULL; /* should be WCS anyway*/
        params->ids = NULL;
        params->numsubsets = 0;
        params->subsets = NULL;
        params->format = NULL;
        params->multipart = 0;
    }
    else
    {
        msSetError(MS_WCSERR, "Params object could not be created. ",
                "msWCSCreateParamsObj20()");
    }
    return params;
}

/************************************************************************/
/*                   msWCSFreeParamsObj20()                             */
/*                                                                      */
/*      Frees a wcs20ParamsObj and releases all linked resources.       */
/************************************************************************/

void msWCSFreeParamsObj20(wcs20ParamsObj *params)
{
    int i = 0;
    if (!params)
    {
        return;
    }
    msDebug("msWCSFreeParamsObj20(): freeing params object.\n");
    msFree(params->request);
    msFree(params->version);
    msFree(params->service);
    msFree(params->format);
    for (i = 0; params->ids && params->ids[i]; ++i)
    {
        msFree(params->ids[i]);
    }
    msFree(params->ids);

    for (i = 0; i < params->numsubsets; ++i)
    {
        msWCSFreeSubsetObj20(params->subsets[i]);
    }
    msFree(params->subsets);
    msFree(params);
}

/************************************************************************/
/*                   msWCSParseSubset20()                               */
/*                                                                      */
/*      Parses several string parameters and fills them into the        */
/*      subset object.                                                  */
/************************************************************************/

static int msWCSParseSubset20(wcs20SubsetObj *subset, const char *axis,
        const char *crs, const char *min, const char *max)
{
    int ts1, ts2;
    ts1 = ts2 = MS_WCS20_UNDEFINED_VALUE;

    if (subset == NULL)
    {
        return MS_FAILURE;
    }

    if (axis == NULL || strlen(axis) == 0)
    {
        msSetError(MS_WCSERR, "Subset axis is not given.",
                "msWCSParseSubset20()");
        return MS_FAILURE;
    }

    msDebug("msWCSParseSubset20(): axis: %s, crs: %s, min: %s, max: %s\n",
            axis, crs ? crs : "none", min, max ? max : "none");

    subset->axis = strdup(axis);
    if (crs != NULL)
    {
        subset->crs = strdup(crs);
    }

    /* Parse first (probably only) part of interval/point;
     * check whether its a time value or a scalar value     */
    ts1 = msWCSParseTimeOrScalar20(&(subset->min), min);
    if (ts1 == MS_WCS20_ERROR_VALUE)
    {
        return MS_FAILURE;
    }

    /* check if its an interval */
    /* if there is a comma, then it is */
    if (max != NULL && strlen(max) > 0)
    {
        subset->operation = MS_WCS20_TRIM;

        /* Parse the second value of the interval */
        ts2 = msWCSParseTimeOrScalar20(&(subset->max), max);
        if (ts2 == MS_WCS20_ERROR_VALUE)
        {
            return MS_FAILURE;
        }

        /* if at least one boundary is not undefined, use that value */
        if ((ts1 == MS_WCS20_UNDEFINED_VALUE) ^ (ts2
                == MS_WCS20_UNDEFINED_VALUE))
        {
            if (ts1 == MS_WCS20_UNDEFINED_VALUE)
            {
                ts1 = ts2;
            }
        }
        /* if time and scalar values do not fit, throw an error */
        else if (ts1 != MS_WCS20_UNDEFINED_VALUE && ts2
                != MS_WCS20_UNDEFINED_VALUE && ts1 != ts2)
        {
            msSetError(
                    MS_WCSERR,
                    "Interval error: minimum is a %s value, maximum is a %s value",
                    "msWCSParseSubset20()", ts1 ? "time" : "scalar",
                    ts2 ? "time" : "scalar");
            return MS_FAILURE;
        }
        /* if both min and max are unbounded -> throw an error */
        if (subset->min.unbounded && subset->max.unbounded)
        {
            msSetError(MS_WCSERR, "Invalid values: no bounds could be parsed",
                    "msWCSParseSubset20()");
            return MS_FAILURE;
        }
    }
    /* there is no second value, therefore it is a point.
     * consequently set the operation to slice */
    else
    {
        subset->operation = MS_WCS20_SLICE;
        if (ts1 == MS_WCS20_UNDEFINED_VALUE)
        {
            msSetError(MS_WCSERR, "Invalid point value given",
                    "msWCSParseSubset20()");
            return MS_FAILURE;
        }
    }

    subset->timeOrScalar = ts1;

    /* check whether the min is smaller than the max */
    if (subset->operation == MS_WCS20_SLICE)
    {
        if (subset->timeOrScalar == MS_WCS20_TIME_VALUE && subset->min.time
                >= subset->max.time)
        {
            msSetError(MS_WCSERR,
                    "Minimum time value is smaller than maximum time value.",
                    "msWCSParseSubset20()");
            return MS_FAILURE;
        }
        if (subset->timeOrScalar == MS_WCS20_SCALAR_VALUE && subset->min.scalar
                >= subset->max.scalar)
        {
            msSetError(MS_WCSERR,
                    "Minimum value is smaller than maximum value",
                    "msWCSParseSubset20()");
            return MS_FAILURE;
        }
    }
    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSParseSubsetKVPString20()                      */
/*                                                                      */
/*      Creates a new wcs20SubsetObj, parses a string and fills         */
/*      the parsed values into the object. Returns NULL on failure.     */
/************************************************************************/

static int msWCSParseSubsetKVPString20(wcs20SubsetObj *subset, char *string)
{
    char *axis, *crs, *min, *max;

    msDebug("msWCSParseSubsetKVPString20(): Parsing KVP-string '%s'\n", string);

    axis = string;
    crs = NULL;
    min = NULL;
    max = NULL;

    /* subset string: dimension [ , crs ] ( intervalOrPoint ) */

    /* find first '(' */
    min = strchr(string, '(');

    /* if min could not be found */
    if (min == NULL)
    {
        msSetError(MS_WCSERR, "Invalid axis subset string: '%s'",
                "msWCSParseSubsetKVPString20()", string);
        return MS_FAILURE;
    }
    /* set min to first letter */
    *min = '\0';
    ++min;

    /* cut the trailing ')' */
    if (min[strlen(min) - 1] == ')')
    {
        min[strlen(min) - 1] = '\0';
    }
    /* look if also a max is defined */
    max = strchr(min, ',');
    if (max != NULL)
    {
        *max = '\0';
        ++max;
    }

    /* look if also a crs is defined */
    crs = strchr(axis, ',');
    if (crs != NULL)
    {
        *crs = '\0';
        ++crs;
    }

    return msWCSParseSubset20(subset, axis, crs, min, max);
}

/************************************************************************/
/*                   msWCSParseRequest20()                              */
/*                                                                      */
/*      Parses a CGI-request to a WCS 20 params object. It is           */
/*      either a POST or a GET request. In case of a POST request       */
/*      the xml content has to be parsed to a DOM structure             */
/*      before the parameters can be extracted.                         */
/************************************************************************/

int msWCSParseRequest20(cgiRequestObj *request, wcs20ParamsObj *params)
{
    int i;
    if (!params || !request)
        return MS_FAILURE;

    /* Parse the POST request */
    if (request->type == MS_POST_REQUEST && request->postrequest && strlen(
            request->postrequest))
    {
        xmlDocPtr doc = NULL;
        xmlNodePtr root = NULL, child = NULL;
        char *tmp = NULL;
        int xmlParsingError = 0, numIds = 0;

        msDebug("msWCSParseRequest20(): Parsing request %s\n",
                request->postrequest);

        if (params->version || params->request || params->service
                || params->ids || params->subsets)
        {
            msSetError(MS_WCSERR, "Params object has already been parsed",
                    "msWCSParseRequest20()");
            return MS_FAILURE;
        }

        /* parse to DOM-Structure and get root element */
        if((doc = xmlParseMemory(request->postrequest, strlen(request->postrequest)))
                == NULL) {
            xmlErrorPtr error = xmlGetLastError();
            msSetError(MS_WCSERR, "XML parsing error: %s",
                    "msWCSParseRequest20()", error->message);
            return MS_FAILURE;
        }
        root = xmlDocGetRootElement(doc);

        /* Get service, version and request from root */
        params->request = strdup((char *) root->name);
        if ((tmp = (char *) xmlGetProp(root, BAD_CAST "service")) != NULL)
            params->service = tmp;
        if ((tmp = (char *) xmlGetProp(root, BAD_CAST "version")) != NULL)
            params->version = tmp;


        /* exit if request is GetCapabilities and the version is already set */
        if(params->version && EQUAL(params->request, "GetCapabilities"))
        {
            xmlFreeDoc(doc);
            xmlCleanupParser();
            return MS_DONE;
        }

        /* if version or service are not compliant -> exit */
        if (params->version && params->service
                && (!EQUALN(params->version, "2.0", 3)
                        || !EQUAL(params->service, "WCS")))
        {
            xmlFreeDoc(doc);
            xmlCleanupParser();
            return MS_DONE;
        }

        /* search first level children, either CoverageID,  */
        for (child = root->children; child != NULL; child = child->next)
        {
            if(EQUAL((char *)child->name, "AcceptVersions"))
            {
                xmlNodePtr versionNode = NULL;
                for(versionNode = child->children; versionNode != NULL; versionNode = versionNode->next)
                {
                    char *content = (char *)xmlNodeGetContent(versionNode);
                    /* only accept versions 2.0.x */
                    if(EQUALN(content, "2.0", 3))
                    {
                        params->version = strdup(content);
                    }
                    xmlFree(content);
                }

                /* version 2.0.x was not in the list -> exiting */
                if(!params->version)
                {
                    xmlFreeDoc(doc);
                    xmlCleanupParser();
                    return MS_DONE;
                }
            }
            else if (EQUAL((char *)child->name, "CoverageID"))
            {
                /* Node content is the coverage ID */
                tmp = (char *)xmlNodeGetContent(child);
                if (tmp == NULL || strlen(tmp) == 0)
                {
                    msSetError(MS_WCSERR, "CoverageID could not be parsed.",
                            "msWCSParseRequest20()");
                    xmlParsingError = 1;
                    break;
                }
                /* insert coverage ID into the list */
                ++numIds;
                params->ids = realloc(params->ids, sizeof(char *)
                        * (numIds + 1));
                params->ids[numIds - 1] = strdup((char *) tmp);
                params->ids[numIds] = NULL;
                xmlFree(tmp);
            }
            else if (EQUAL((char *) child->name, "format"))
            {
                params->format = (char *)xmlNodeGetContent(child);
            }
            else if (EQUAL((char *) child->name, "DimensionTrim"))
            {
                wcs20SubsetObj *subset = NULL;
                xmlNodePtr temp = NULL;
                char *axis = NULL, *min = NULL, *max = NULL, *crs = NULL;

                /* get strings for axis, min and max */
                for (temp = child->children; temp != NULL; temp = temp->next)
                {
                    if (EQUAL((char *)temp->name, "Dimension"))
                    {
                        if (axis != NULL)
                        {
                            msSetError(MS_WCSERR,
                                    "Parameter 'Dimension' is already set.",
                                    "msWCSParseRequest20()");
                            xmlParsingError = 1;
                            break;
                        }
                        axis = (char *) xmlNodeGetContent(temp);
                        crs = (char *) xmlGetProp(temp, BAD_CAST "crs");
                    }
                    else if (EQUAL((char *)temp->name, "trimLow"))
                    {
                        min = (char *) xmlNodeGetContent(temp);
                    }
                    else if (EQUAL((char *)temp->name, "trimHigh"))
                    {
                        max = (char *) xmlNodeGetContent(temp);
                    }
                }
                subset = msWCSCreateSubsetObj20();

                /* min and max have to have a value */
                if(min == NULL )
                {
                    min = strdup("*");
                }
                if(max == NULL)
                {
                    max = strdup("*");
                }
                if (msWCSParseSubset20(subset, axis, crs, min, max)
                        == MS_FAILURE)
                {
                    msWCSFreeSubsetObj20(subset);
                    xmlParsingError = 1;
                }
                params->numsubsets++;
                params->subsets = (wcs20SubsetObj**) realloc(params->subsets,
                        sizeof(wcs20SubsetObj*) * (params->numsubsets));
                params->subsets[params->numsubsets - 1] = subset;

                /* cleanup */
                msFree(axis);
                msFree(min);
                msFree(max);
                msFree(crs);
            }
            /* TODO: memory corruption */
            /*else if(EQUAL((char *) child->name, "DimensionSlice")) {
                wcs20SubsetObj *subset = NULL;
                xmlNodePtr temp = NULL;
                char *axis, *point;
                axis = NULL;
                point = strdup("*\0");
                for (temp = child->children; temp != NULL; temp = temp->next)
                {
                    if (EQUAL((char *)temp->name, "Dimension"))
                    {
                        if (axis != NULL)
                        {
                            msSetError(MS_WCSERR,
                                    "Parameter 'Dimension' is already set.",
                                    "msWCSParseRequest20()");
                            xmlParsingError = 1;
                            break;
                        }
                        axis = (char *) xmlNodeGetContent(temp);
                    }
                    else if (EQUAL((char *)temp->name, "SlicePoint"))
                    {
                        free(point);
                        point = (char *) xmlNodeGetContent(temp);
                    }
                    subset = msWCSCreateSubsetObj20();
                    if (msWCSParseSubset20(subset, axis, NULL, point, NULL)
                            == MS_FAILURE)
                    {
                        msWCSFreeSubsetObj20(subset);
                        xmlParsingError = 1;
                    }
                    params->numsubsets++;
                    params->subsets = (wcs20SubsetObj**) realloc(
                            params->subsets, sizeof(wcs20SubsetObj*)
                                    * (params->numsubsets));
                    params->subsets[params->numsubsets - 1] = subset;
                    xmlFree(BAD_CAST axis);
                    xmlFree(BAD_CAST point);
                }
            }*/
        }
        xmlFreeDoc(doc);
        xmlCleanupParser();
        if (xmlParsingError)
        {
            msSetError(MS_WCSERR,
                    "POST request could not be parsed correctly.",
                    "msWCSParseRequest20()");
            return MS_FAILURE;
        }
        msDebug("msWCSParseRequest20(): Request parsed\n");
        return MS_SUCCESS;
    }

    /* Parse the KVP GET request */
    for (i = 0; i < request->NumParams; ++i)
    {
        char *key = NULL, *value = NULL;
        key = request->ParamNames[i];
        value = request->ParamValues[i];

        if (EQUAL(key, "SERVICE"))
        {
            if (params->service)
            {
                msSetError(MS_WCSERR, "Parameter 'Service' is already set.",
                        "msWCSParseRequest20()");
                return MS_FAILURE;
            }
            params->service = strdup(value);
        }
        else if (EQUAL(key, "VERSION"))
        {
            if (params->version)
            {
                msSetError(MS_WCSERR, "Parameter 'Version' is already set.",
                        "msWCSParseRequest20()");
                return MS_FAILURE;
            }
            params->version = strdup(value);
        }
        else if (EQUAL(key, "REQUEST"))
        {
            if (params->request)
            {
                msSetError(MS_WCSERR, "Parameter 'Request' is already set.",
                        "msWCSParseRequest20()");
                return MS_FAILURE;
            }
            params->request = strdup(value);
        }
        else if (EQUAL(key, "FORMAT"))
        {
            params->format = strdup(value);
        }
        else if (EQUAL(key, "COVERAGEID"))
        {
            char **tokens;
            int num = 0, j = 0;
            if (params->ids != NULL)
            {
                msSetError(MS_WCSERR, "Parameter 'CoverageID' is already set. "
                        "For multiple IDs use a comma separated list.",
                        "msWCSParseRequest20()");
                return MS_FAILURE;
            }
            tokens = msStringSplit(value, ',', &num);
            params->ids = (char **) calloc(num + 1, sizeof(char*));
            for (j = 0; j < num; ++j)
            {
                params->ids[j] = strdup(tokens[j]);
            }
            msFreeCharArray(tokens, num);
        }
        else if (EQUALN(key, "SUBSET", 6))// EQUAL(key, "SUBSET")
        {
            wcs20SubsetObj *pTemp = msWCSCreateSubsetObj20();
            if (msWCSParseSubsetKVPString20(pTemp, value) == MS_FAILURE)
            {
                msWCSFreeSubsetObj20(pTemp);
                return MS_FAILURE;
            }
            params->numsubsets++;
            params->subsets = (wcs20SubsetObj**) realloc(params->subsets,
                    sizeof(wcs20SubsetObj*) * (params->numsubsets));
            params->subsets[params->numsubsets - 1] = pTemp;
        }
        else if (EQUAL(key, "MEDIATYPE"))
        {
            if(EQUAL(value, "multipart/mixed"))
            {
                params->multipart = 1;
            }
        }
    }
    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSFindSubset20()                                */
/*                                                                      */
/*      Iterates over every subset in the parameters and returns        */
/*      the first subset which name is in the subset list.              */
/*      Returns NULL if none is found                                   */
/************************************************************************/
static wcs20SubsetObj* msWCSFindSubset20(wcs20ParamsObj *params, ...)
{
    va_list vl;
    int i;
    char *arg;
    va_start(vl, params);
    while ((arg = va_arg(vl, char*)) != NULL)
    {
        for (i = 0; i < params->numsubsets; ++i)
        {
            if (EQUAL(arg, params->subsets[i]->axis))
            {
                va_end(vl);
                return params->subsets[i];
            }
        }
    }
    va_end(vl);
    return NULL;
}

/************************************************************************/
/*                   msWCSCreateBoundingBox20()                         */
/*                                                                      */
/*      Creates a bounding box out of the parameter informations        */
/*      and sets it as the maps maximum extent.                         */
/************************************************************************/
int msWCSCreateBoundingBox20(wcs20ParamsObj *params, mapObj *map, layerObj *layer)
{
    wcs20SubsetObj *subset = NULL;
    rectObj ret = layer->extent;
    int i;
    msDebug("Extent before: %.2f,%.2f,%.2f,%.2f\n", ret.minx, ret.miny,
            ret.maxx, ret.maxy);

    msDebug("Numsubsets: %d (", params->numsubsets);
    for(i = 0; i < params->numsubsets; ++i) {
        msDebug("%s,", params->subsets[i]->axis);
    }
    msDebug("\b)\n");


    /* find subset for x-axis */
    subset = msWCSFindSubset20(params, "x", "xaxis", "x-axis", "x_axis", "lon", "lon_axis", "lon-axis", NULL);
    if (subset != NULL)
    {
        if(subset->operation == MS_WCS20_TRIM
                && subset->timeOrScalar == MS_WCS20_SCALAR_VALUE)
        {
            msDebug("Subset for X-axis found: %s\n", subset->axis);
            if (!subset->min.unbounded)
                ret.minx = MAX(subset->min.scalar, ret.minx);
            if (!subset->max.unbounded)
                ret.maxx = MIN(subset->max.scalar, ret.maxx);
        }
        else
        {
            msSetError(MS_WCSERR, "Error creating BBOX for x axis. ",
                    "msWCSCreateBoundingBox20()");
            return MS_FAILURE;
        }
    }

    /* find subset for y-axis */
    subset = msWCSFindSubset20(params, "y", "yaxis", "y-axis", "y_axis", "lat", "lat_axis", "lat-axis", NULL);
    if (subset != NULL)
    {
        if(subset->operation == MS_WCS20_TRIM
            && subset->timeOrScalar == MS_WCS20_SCALAR_VALUE)
        {
            msDebug("Subset for Y-axis found: %s\n", subset->axis);
            if (!subset->min.unbounded)
                ret.miny = MAX(subset->min.scalar, ret.miny);
            if (!subset->max.unbounded)
                ret.maxy = MIN(subset->max.scalar, ret.maxy);
        }
        else
        {
            msSetError(MS_WCSERR, "Error creating BBOX for y axis. ",
                    "msWCSCreateBoundingBox20()");
            return MS_FAILURE;
        }
    }

    /* if size changed compute new width */
    if(ABS(map->extent.maxx - map->extent.minx) != ABS(ret.maxx - ret.minx))
    {
        double total = ABS(map->extent.maxx - map->extent.minx),
                part = ABS(ret.maxx - ret.minx);
        map->width = map->width * (part / total);
        msDebug("New image width: %d\n", map->width);
    }

    /* if size changed compute new height */
    if(ABS(map->extent.maxy - map->extent.miny) != ABS(ret.maxy - ret.miny))
    {
        double total = ABS(map->extent.maxy - map->extent.miny),
                part = ABS(ret.maxy - ret.miny);
        map->height = map->height * (part / total);
        msDebug("New image height: %d\n", map->height);
    }


    /* TODO: add/subtract 0.5*resolution to/from bounds */
    /* maybe improve this */
    ret.minx += map->cellsize*0.5;
    ret.miny += map->cellsize*0.5;
    ret.maxx -= map->cellsize*0.5;
    ret.maxy -= map->cellsize*0.5;


    msDebug("Extent after:  %.2f,%.2f,%.2f,%.2f\n", ret.minx, ret.miny,
            ret.maxx, ret.maxy);
    layer->extent = map->extent = ret;
    return MS_DONE;
}


/************************************************************************/
/*                   msWCSCreateBoundingBoxAndGetProjection20()         */
/*                                                                      */
/*      Creates a bounding box out of the parameter informations        */
/*      and sets it as the maps maximum extent.                         */
/************************************************************************/
int msWCSCreateBoungingBoxAndGetProjection20(wcs20ParamsObj *params, rectObj *outBBOX, char *crsString, size_t maxCRSStringSize)
{
    wcs20SubsetObj *subset = NULL;
    char *projection = NULL;

    if(outBBOX == NULL)
    {
        return MS_FAILURE;
    }

    outBBOX->minx = outBBOX->miny = -DBL_MAX;
    outBBOX->maxx = outBBOX->maxy = DBL_MAX;

    /* find subset for x-axis */
    subset = msWCSFindSubset20(params, "x", "xaxis", "x-axis", "x_axis", "lon", "lon_axis", "lon-axis", NULL);
    if (subset != NULL)
    {
        if(subset->operation == MS_WCS20_TRIM
                && subset->timeOrScalar == MS_WCS20_SCALAR_VALUE)
        {
            msDebug("Subset for X-axis found: %s\n", subset->axis);
            if (!subset->min.unbounded)
                outBBOX->minx = subset->min.scalar;
            if (!subset->max.unbounded)
                outBBOX->maxx = subset->max.scalar;
            if (subset->crs)
                projection = subset->crs;
        }
        else
        {
            msSetError(MS_WCSERR, "Error creating BBOX for x axis. ",
                    "msWCSCreateBoundingBox20()");
            return MS_FAILURE;
        }
    }

    /* find subset for y-axis */
    subset = msWCSFindSubset20(params, "y", "yaxis", "y-axis", "y_axis", "lat", "lat_axis", "lat-axis", NULL);
    if (subset != NULL)
    {
        if(subset->operation == MS_WCS20_TRIM
            && subset->timeOrScalar == MS_WCS20_SCALAR_VALUE)
        {
            msDebug("Subset for Y-axis found: %s\n", subset->axis);
            if (!subset->min.unbounded)
                outBBOX->miny = subset->min.scalar;
            if (!subset->max.unbounded)
                outBBOX->maxy = subset->max.scalar;
            if (subset->crs)
            {
                if(projection)
                {
                    if(!EQUAL(projection, subset->crs))
                    {
                        msSetError(MS_WCSERR, "CRS for X and Y axis are not the same.",
                                "msWCSCreateBoundingBox20()");
                        return MS_FAILURE;
                    }
                }
                else
                {
                    projection = subset->crs;
                }
            }
        }
        else
        {
            msSetError(MS_WCSERR, "Error creating BBOX for y axis. ",
                    "msWCSCreateBoundingBox20()");
            return MS_FAILURE;
        }
    }
    if(projection != NULL)
    {
        snprintf(crsString, maxCRSStringSize, "%s", projection);
    }
    else
    {
        sprintf(crsString, "%s", "imageCRS");
    }
    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSProperTime20()                                */
/*                                                                      */
/*      Convenience function to remove trailing '\n' in function        */
/*      ctime().                                                        */
/************************************************************************/

static char *msWCSProperTime20(time_t *time)
{
    char *str = ctime(time);
    str[strlen(str) - 1] = '\0';
    return str;
}

/************************************************************************/
/*                   msWCSPrepareNamespaces20()                         */
/*                                                                      */
/*      Inserts namespace definitions into the root node of a DOM       */
/*      structure.                                                      */
/************************************************************************/

static void msWCSPrepareNamespaces20( xmlDocPtr pDoc, xmlNodePtr psRootNode, mapObj* map )
{
    xmlNsPtr psXsiNs;
    char *schemaLocation = NULL;
    char *xsi_schemaLocation = NULL;

    xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WCS_20_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX));

    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_20_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI,           BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX );
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,       BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI,     BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,       BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WCS_20_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_GML_32_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_GMLCOV_10_NAMESPACE_URI,     BAD_CAST MS_OWSCOMMON_GMLCOV_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_SWE_20_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_SWE_NAMESPACE_PREFIX);

    psXsiNs = xmlSearchNs(pDoc, psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

    schemaLocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );
    xsi_schemaLocation = strdup(MS_OWSCOMMON_WCS_20_NAMESPACE_URI);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemaLocation);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, MS_OWSCOMMON_WCS_20_SCHEMAS_LOCATION);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");

    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, MS_OWSCOMMON_OWS_20_NAMESPACE_URI);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemaLocation);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, MS_OWSCOMMON_OWS_20_SCHEMAS_LOCATION);

    xmlNewNsProp(psRootNode, psXsiNs, BAD_CAST "schemaLocation", BAD_CAST xsi_schemaLocation);
}

/************************************************************************/
/*                   msWCSGetFormatList20()                             */
/*                                                                      */
/*      Copied from mapwcs.c.                                           */
/************************************************************************/

static char *msWCSGetFormatsList20( mapObj *map, layerObj *layer )
{
    char *format_list = strdup("");
    char **tokens = NULL, **formats = NULL;
    int  i, numtokens = 0, numformats;
    const char *value;

/* -------------------------------------------------------------------- */
/*      Parse from layer metadata.                                      */
/* -------------------------------------------------------------------- */
    if( layer != NULL
        && (value = msOWSGetEncodeMetadata( &(layer->metadata),"COM","formats",
                                            "GTiff" )) != NULL ) {
        tokens = msStringSplit(value, ' ', &numtokens);
    }

/* -------------------------------------------------------------------- */
/*      Or generate from all configured raster output formats that      */
/*      look plausible.                                                 */
/* -------------------------------------------------------------------- */
    else
    {
        tokens = (char **) calloc(map->numoutputformats,sizeof(char*));
        for( i = 0; i < map->numoutputformats; i++ )
        {
            switch( map->outputformatlist[i]->renderer )
            {
                /* seemingly normal raster format */
              case MS_RENDER_WITH_GD:
              case MS_RENDER_WITH_AGG:
              case MS_RENDER_WITH_RAWDATA:
                tokens[numtokens++] = strdup(map->outputformatlist[i]->name);
                break;

                /* rest of formats aren't really WCS compatible */
              default:
                break;

            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Convert outputFormatObj names into mime types and remove        */
/*      duplicates.                                                     */
/* -------------------------------------------------------------------- */
    numformats = 0;
    formats = (char **) calloc(sizeof(char*),numtokens);

    for( i = 0; i < numtokens; i++ )
    {
        int format_i, j;
        const char *mimetype;

        for( format_i = 0; format_i < map->numoutputformats; format_i++ )
        {
            if( strcasecmp(map->outputformatlist[format_i]->name,
                           tokens[i]) == 0 )
                break;
        }

        if( format_i == map->numoutputformats )
        {
            msDebug("Failed to find outputformat info on format '%s', ignore.\n",
                    tokens[i] );
            continue;
        }

        mimetype = map->outputformatlist[format_i]->mimetype;
        if( mimetype == NULL || strlen(mimetype) == 0 )
        {
            msDebug("No mimetime for format '%s', ignoring.\n",
                    tokens[i] );
            continue;
        }

        for( j = 0; j < numformats; j++ )
        {
            if( strcasecmp(mimetype,formats[j]) == 0 )
                break;
        }

        if( j < numformats )
        {
            msDebug( "Format '%s' ignored since mimetype '%s' duplicates another outputFormatObj.\n",
                     tokens[i], mimetype );
            continue;
        }

        formats[numformats++] = strdup(mimetype);
    }

    msFreeCharArray(tokens,numtokens);

/* -------------------------------------------------------------------- */
/*      Turn mimetype list into comma delimited form for easy use       */
/*      with xml functions.                                             */
/* -------------------------------------------------------------------- */
    for(i=0; i<numformats; i++)
    {
        int       new_length;
        const char *format = formats[i];

        new_length = strlen(format_list) + strlen(format) + 2;
        format_list = (char *) realloc(format_list,new_length);

        if( strlen(format_list) > 0 )
            strcat( format_list, "," );
        strcat( format_list, format );
    }
    msFreeCharArray(formats,numformats);

    return format_list;
}

/************************************************************************/
/*                   msWCSCommon20_CreateBoundedBy()                    */
/*                                                                      */
/*      Inserts the BoundedBy section into an existing DOM structure.   */
/************************************************************************/

static void msWCSCommon20_CreateBoundedBy(layerObj *layer, coverageMetadataObj *cm,
        xmlNsPtr psGmlNs, xmlNodePtr psRoot)
{
    xmlNodePtr psBoundedBy, psEnvelope;
    char lowerCorner[100], upperCorner[100], uomLabels[100];

    psBoundedBy = xmlNewChild( psRoot, psGmlNs, BAD_CAST "boundedBy", NULL);
    {
        psEnvelope = xmlNewChild(psBoundedBy, psGmlNs, BAD_CAST "Envelope", NULL);
        {
            xmlNewProp(psEnvelope, BAD_CAST "srsName", BAD_CAST cm->srs_urn);
            xmlNewProp(psEnvelope, BAD_CAST "axisLabels", BAD_CAST "Lat Long");

            /* FIXME: find out real UOMs */
            snprintf(uomLabels, 100, "%s %s", "deg", "deg");

            xmlNewProp(psEnvelope, BAD_CAST "uomLabels", BAD_CAST uomLabels);
            xmlNewProp(psEnvelope, BAD_CAST "srsDimension", BAD_CAST "2");

            snprintf(lowerCorner, 100, "%.15g %.15g", cm->extent.minx, cm->extent.miny);
            snprintf(upperCorner, 100, "%.15g %.15g", cm->extent.maxx, cm->extent.maxy);

            xmlNewChild(psEnvelope, psGmlNs, BAD_CAST "lowerCorner", BAD_CAST lowerCorner);
            xmlNewChild(psEnvelope, psGmlNs, BAD_CAST "upperCorner", BAD_CAST upperCorner);
        }
    }
}

/************************************************************************/
/*                   msWCSCommon20_CreateDomainSet()                    */
/*                                                                      */
/*      Inserts the DomainSet section into an existing DOM structure.   */
/************************************************************************/

static void msWCSCommon20_CreateDomainSet(layerObj* layer, coverageMetadataObj* cm,
        xmlNsPtr psGmlNs, xmlNodePtr psRoot)
{
    xmlNodePtr psDomainSet, psGrid, psLimits, psGridEnvelope, psOrigin,
        psPos, psOffsetX, psOffsetY;
    char low[100], high[100], id[100], point[100], resx[100], resy[100]; // TODO: ID limited to 100 chars??? Why?

    psDomainSet = xmlNewChild( psRoot, psGmlNs, BAD_CAST "domainSet", NULL);
    {
        psGrid = xmlNewChild(psDomainSet, psGmlNs, BAD_CAST "RectifiedGrid", NULL);
        {
            xmlNewProp(psGrid, BAD_CAST "dimension", BAD_CAST "2");
            sprintf(id, "grid_%s", layer->name);
            xmlNewNsProp(psGrid, psGmlNs, BAD_CAST "id", BAD_CAST id);

            psLimits = xmlNewChild(psGrid, psGmlNs, BAD_CAST "limits", NULL);
            {
                psGridEnvelope = xmlNewChild(psLimits, psGmlNs, BAD_CAST "GridEnvelope", NULL);
                {
                    sprintf(low, "1 1");
                    sprintf(high, "%d %d", cm->xsize, cm->ysize);

                    xmlNewChild(psGridEnvelope, psGmlNs, BAD_CAST "low", BAD_CAST low);
                    xmlNewChild(psGridEnvelope, psGmlNs, BAD_CAST "high", BAD_CAST high);
                }
            }
            xmlNewChild(psGrid, psGmlNs, BAD_CAST "axisLabels", BAD_CAST "xAxis yAxis");

            psOrigin = xmlNewChild(psGrid, psGmlNs, BAD_CAST "origin", NULL);
            {
                sprintf(point, "%f %f", cm->extent.minx, cm->extent.miny);
                psOrigin = xmlNewChild(psOrigin, psGmlNs, BAD_CAST "Point", NULL);
                sprintf(id, "grid_origin_%s", layer->name);
                xmlNewNsProp(psOrigin, psGmlNs, BAD_CAST "id", BAD_CAST id);
                xmlNewProp(psOrigin, BAD_CAST "srsName", BAD_CAST cm->srs_urn);

                psPos = xmlNewChild(psOrigin, psGmlNs, BAD_CAST "pos", BAD_CAST point);
            }
            sprintf(resx, "%f 0", cm->xresolution);
            sprintf(resy, "0 %f", cm->yresolution);
            psOffsetX = xmlNewChild(psGrid, psGmlNs, BAD_CAST "offsetVector", BAD_CAST resx);
            xmlNewProp(psOffsetX, BAD_CAST "srsName", BAD_CAST cm->srs_urn);
            psOffsetY = xmlNewChild(psGrid, psGmlNs, BAD_CAST "offsetVector", BAD_CAST resy);
            xmlNewProp(psOffsetY, BAD_CAST "srsName", BAD_CAST cm->srs_urn);
        }
    }
}

/************************************************************************/
/*                   msWCSCommon20_CreateRangeType()                    */
/*                                                                      */
/*      Inserts the RangeType section into an existing DOM structure.   */
/************************************************************************/

static void msWCSCommon20_CreateRangeType(layerObj* layer, coverageMetadataObj* cm,
        xmlNsPtr psGmlNs, xmlNsPtr psGmlcovNs, xmlNsPtr psSweNs, xmlNsPtr psXLinkNs, xmlNodePtr psRoot)
{
    xmlNodePtr psRangeType, psDataRecord, psField, psQuantity,
        psUom, psConstraint, psAllowedValues = NULL/*, psNilValues = NULL*/;
    char * value;
    int i, numBandDescriptions = 0, numBandUoms = 0, numNilValues = 0,
        numNilValueReasons = 0;
    char ** bandNames = NULL, ** bandDescriptions = NULL, **bandUoms = NULL,
        **nilValues = NULL, **nilValueReasons = NULL;

    /* setup iteration */
    /* get information concerning multiple bands */
    value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "band_definition", NULL);

    if((value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "band_description", NULL)) != NULL)
    {
        bandDescriptions = msStringSplit(value, ';', &numBandDescriptions);
    }

    if((value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "band_uom", NULL)) != NULL)
    {
        bandUoms = msStringSplit(value, ';', &numBandUoms);
    }

    if((value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "rangeset_nullvalue", NULL)) != NULL)
    {
        nilValues = msStringSplit(value, ';', &numNilValues);
    }
    else if((value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "nilvalues", NULL)) != NULL)
    {
        nilValues = msStringSplit(value, ';', &numNilValues);
    }

    if((value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "nilvalues_reason", NULL)) != NULL)
    {
        nilValueReasons = msStringSplit(value, ';', &numNilValueReasons);
    }

    psRangeType = xmlNewChild( psRoot, psGmlcovNs, BAD_CAST "rangeType", NULL);
    psDataRecord  = xmlNewChild(psRangeType, psSweNs, BAD_CAST "DataRecord", NULL);

    /* iterate over every band */
    for(i = 0; i < cm->bandcount && i < 10; ++i)
    {
        /* add field tag */
        psField = xmlNewChild(psDataRecord, psSweNs, BAD_CAST "field", NULL);

        xmlNewProp(psField, BAD_CAST "name", BAD_CAST cm->bandinterpretation[i]);
        /* add Quantity tag */
        psQuantity = xmlNewChild(psField, psSweNs, BAD_CAST "Quantity", NULL);
        if((value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "band_definition", NULL)) != NULL)
        {
            xmlNewProp(psQuantity, BAD_CAST "definition", BAD_CAST value);
        }
        if(i < numBandDescriptions)
        {
            xmlNewChild(psQuantity, psGmlNs, BAD_CAST "description", BAD_CAST bandDescriptions[i]);
        }
        if((value = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "band_name", NULL)) != NULL)
        {
            xmlNewChild(psQuantity, psGmlNs, BAD_CAST "name", BAD_CAST value);
        }

        /* TODO check with specs */
        psUom = xmlNewChild(psQuantity, psSweNs, BAD_CAST "uom", NULL);
        xmlNewProp(psUom, BAD_CAST "code", BAD_CAST "W/cm2");

        /* add constraint */
        psConstraint = xmlNewChild(psQuantity, psSweNs, BAD_CAST "constraint", NULL);
        char interval[100];

        /* first band gets information about allowed values */
        /* others get reference to it */
        if(psAllowedValues == NULL)
        {
            char id[100];
            psAllowedValues = xmlNewChild(psConstraint, psSweNs, BAD_CAST "AllowedValues", NULL);
            sprintf(id, "VALUE_SPACE_%s", layer->name);
            xmlNewNsProp(psAllowedValues, psGmlNs, BAD_CAST "id", BAD_CAST id);
            switch(cm->imagemode)
            {
                case MS_IMAGEMODE_BYTE:
                    sprintf(interval, "%d %d", 0, 255);
                    break;
                case MS_IMAGEMODE_INT16:
                    sprintf(interval, "%d %d", 0, USHRT_MAX);
                    break;
                case MS_IMAGEMODE_FLOAT32:
                    sprintf(interval, "%.5f %.5f", -FLT_MAX, FLT_MAX);
                    break;
            }
            xmlNewChild(psAllowedValues, psSweNs, BAD_CAST "interval", BAD_CAST interval);
            if((value  = msOWSGetEncodeMetadata(&(layer->metadata), "COM", "band_significant_figures", NULL)) != NULL)
            {
                xmlNewChild(psAllowedValues, psSweNs, BAD_CAST "significantFigures", BAD_CAST value);
            }
        }
        /* TODO: not in Spec anymore?? */
        /*else
        {
            char href[100];
            xmlNodePtr psTemp =
                xmlNewChild(psConstraint, psSweNs, BAD_CAST "AllowedValues", NULL);
            sprintf(href, "#VALUE_SPACE_%s", layer->name);
            xmlNewNsProp(psTemp, psXLinkNs, BAD_CAST "href", BAD_CAST href);
        }*/

        /* if there are given nilvalues -> add them to the first field */
        /* all other fields get a reference to these */
        /* TODO: look up specs for nilValue */
        /*if(psNilValues == NULL && numNilValues > 0)
        {
            int j;
            char id[100];
            psNilValues = xmlNewChild(
                xmlNewChild(psQuantity, psSweNs, BAD_CAST "nilValues", NULL),
                psSweNs, BAD_CAST "NilValues", NULL);
            sprintf(id, "NIL_VALUES_%s", layer->name);
            xmlNewNsProp(psNilValues, psGmlNs, BAD_CAST "id", BAD_CAST id);
            for(j = 0; j < numNilValues; ++j)
            {
                xmlNodePtr psTemp =
                    xmlNewChild(psNilValues, psSweNs, BAD_CAST "nilValue", BAD_CAST nilValues[j]);
                if(j < numNilValueReasons)
                    xmlNewProp(psTemp, BAD_CAST "reason", BAD_CAST nilValueReasons[j]);
            }
        }
        else if(numNilValues > 0)
        {
            char href[100];
            xmlNodePtr psTemp =
                xmlNewChild(psQuantity, psSweNs, BAD_CAST "nilValues", NULL);
            sprintf(href, "#NIL_VALUES_%s", layer->name);
            xmlNewNsProp(psTemp, psXLinkNs, BAD_CAST "href", BAD_CAST href);
        }*/
    }
    msFreeCharArray(bandDescriptions, numBandDescriptions);
    msFreeCharArray(bandUoms, numBandUoms);
    msFreeCharArray(nilValues, numNilValues);
    msFreeCharArray(nilValueReasons, numNilValueReasons);
}

/************************************************************************/
/*                   msWCSWriteDocument20()                             */
/*                                                                      */
/*      Writes out an xml structure to the stream.                      */
/************************************************************************/

static int msWCSWriteDocument20(mapObj* map, xmlDocPtr psDoc)
{
    xmlChar *buffer = NULL;
    int size = 0;
    msIOContext *context = NULL;

    const char *encoding = msOWSLookupMetadata(&(map->web.metadata), "CO", "encoding");

    if( msIO_needBinaryStdout() == MS_FAILURE )
    {
        return MS_FAILURE;
    }

    if (encoding)
    {
        msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
    }
    else
    {
        msIO_printf("Content-type: text/xml%c%c",10,10);
    }

    context = msIO_getHandler(stdout);

    xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, (encoding ? encoding : "ISO-8859-1"), 1);
    msIO_contextWrite(context, buffer, size);
    xmlFree(buffer);

    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSWriteFile20()                                 */
/*                                                                      */
/*      Writes an image object to the stream. If multipart is set,      */
/*      then content sections are inserted.                             */
/************************************************************************/

static int msWCSWriteFile20(mapObj* map, imageObj* image, wcs20ParamsObj* params, int multipart)
{
    int status;
    char* filename = NULL;
    char* encoding = NULL;
    int i;
    /*if(multipart)
    {
        msIO_fprintf(
                stdout,
                "--wcs\n"
                "Content-Type: %s\n"
                "Content-Description: coverage data\n"
                "Content-Transfer-Encoding: binary\n"
                "Content-ID: coverage/wcs.%s\n"
                "Content-Disposition: INLINE%c%c",
                MS_IMAGE_EXTENSION(map->outputformat),
                MS_IMAGE_MIME_TYPE(map->outputformat),
                MS_IMAGE_EXTENSION(map->outputformat),
                10, 10 );
    }
    else
    {
        msIO_fprintf(
                stdout,
                "Content-Type: %s\n",
                "Content-Transfer-Encoding: binary\n",
                MS_IMAGE_MIME_TYPE(map->outputformat));
    }
    status = msSaveImage(map, image, NULL);
    if( status != MS_SUCCESS )
    {
            msSetError( MS_MISCERR, "msSaveImage() failed", "msWCSReturnCoverage11()");
            return msWCSException(map, "mapserv", "NoApplicableCode", "2.0.0");
    }

    if(multipart)
        msIO_fprintf( stdout, "--wcs--%c%c", 10, 10 );*/


    /* -------------------------------------------------------------------- */
    /*      Fetch the driver we will be using and check if it supports      */
    /*      VSIL IO.                                                        */
    /* -------------------------------------------------------------------- */
    #ifdef GDAL_DCAP_VIRTUALIO
        if( EQUALN(image->format->driver,"GDAL/",5) )
        {
            GDALDriverH hDriver;
            const char *pszExtension = image->format->extension;

            msDebug("In GDAL\n");

            msAcquireLock( TLOCK_GDAL );
            hDriver = GDALGetDriverByName( image->format->driver+5 );
            if( hDriver == NULL )
            {
                msReleaseLock( TLOCK_GDAL );
                msSetError( MS_MISCERR,
                        "Failed to find %s driver.",
                        "msWCSWriteFile20()",
                        image->format->driver+5 );
                return msWCSException(map, "mapserv", "NoApplicableCode",
                        params->version);
            }

            if( pszExtension == NULL )
                pszExtension = "img.tmp";

            if( GDALGetMetadataItem( hDriver, GDAL_DCAP_VIRTUALIO, NULL )
                != NULL )
            {
                /*            CleanVSIDir( "/vsimem/wcsout" ); */
                filename = strdup(CPLFormFilename("/vsimem/wcsout",
                                        "out", pszExtension ));

                msReleaseLock( TLOCK_GDAL );
                status = msSaveImage(map, image, filename);
                if( status != MS_SUCCESS )
                {
                    msSetError(MS_MISCERR, "msSaveImage() failed",
                            "msWCSWriteFile20()");
                    return msWCSException20(map, "mapserv", "NoApplicableCode",
                            params->version);
                }
            }
            msReleaseLock( TLOCK_GDAL );
        }
    #endif

    /* -------------------------------------------------------------------- */
    /*      If we weren't able to write data under /vsimem, then we just    */
    /*      output a single "stock" filename.                               */
    /* -------------------------------------------------------------------- */
    if( filename == NULL )
    {
        msDebug("filename == NULL\n");
        if(multipart)
        {
            msIO_fprintf(
            stdout,
            "--wcs\n"
            "Content-Type: %s\n"
            "Content-Description: coverage data\n"
            "Content-Transfer-Encoding: binary\n"
            "Content-ID: coverage/wcs.%s\n"
            "Content-Disposition: INLINE%c%c",
            MS_IMAGE_EXTENSION(map->outputformat),
            MS_IMAGE_MIME_TYPE(map->outputformat),
            MS_IMAGE_EXTENSION(map->outputformat),
            10, 10 );
        }
        else
        {
            msIO_fprintf(
                stdout,
                "Content-Type: %s\n%c",
                MS_IMAGE_MIME_TYPE(map->outputformat),
                10);
        }

        status = msSaveImage(map, image, NULL);
        if( status != MS_SUCCESS )
        {
            msSetError( MS_MISCERR, "msSaveImage() failed", "msWCSWriteFile20()");
            return msWCSException11(map, "mapserv", "NoApplicableCode", params->version);
        }
        if(multipart)
            msIO_fprintf( stdout, "--wcs--%c%c", 10, 10 );
        return MS_SUCCESS;
    }

    /* -------------------------------------------------------------------- */
    /*      When potentially listing multiple files, we take great care     */
    /*      to identify the "primary" file and list it first.  In fact      */
    /*      it is the only file listed in the coverages document.           */
    /* -------------------------------------------------------------------- */
    #ifdef GDAL_DCAP_VIRTUALIO
    {
        char **all_files = CPLReadDir( "/vsimem/wcsout" );
        int count = CSLCount(all_files);

        if( msIO_needBinaryStdout() == MS_FAILURE )
            return MS_FAILURE;

        msDebug("NumFiles: %d\n", count);

        msAcquireLock( TLOCK_GDAL );
        for( i = count-1; i >= 0; i-- )
        {
            const char *this_file = all_files[i];
            msDebug("ThisFile: %s\n", this_file);

            if( EQUAL(this_file,".") || EQUAL(this_file,"..") )
            {
                msDebug("yy\n");
                all_files = CSLRemoveStrings( all_files, i, 1, NULL );
                continue;
            }

            if( i > 0 && EQUAL(this_file,CPLGetFilename(filename)) )
            {
                msDebug("xx\n");
                all_files = CSLRemoveStrings( all_files, i, 1, NULL );
                all_files = CSLInsertString(all_files,0,CPLGetFilename(filename));
                i++;
            }
        }

        msDebug("%s\n", all_files[0]);
    /* -------------------------------------------------------------------- */
    /*      Dump all the files in the memory directory as mime sections.    */
    /* -------------------------------------------------------------------- */
        count = CSLCount(all_files);

        msDebug("%s\n", all_files[0]);

        for( i = 0; i < count; i++ )
        {
            const char *mimetype = NULL;
            FILE *fp;
            unsigned char block[4000];
            int bytes_read;

            msDebug("Outputformat: %s\nFile: %s\n", map->outputformat->name, all_files[i]);

            if( i == 0 )
                mimetype = MS_IMAGE_MIME_TYPE(map->outputformat);

            if( mimetype == NULL )
                mimetype = "application/octet-stream";
            if(multipart)
            {
                msIO_fprintf(
                    stdout,
                    "--wcs\n"
                    "Content-Type: %s\n"
                    "Content-Description: coverage data\n"
                    "Content-Transfer-Encoding: binary\n"
                    "Content-ID: coverage/%s\n"
                    "Content-Disposition: INLINE%c%c",
                    mimetype,
                    all_files[i],
                    10, 10 );
            }
            else
            {
                msIO_fprintf(
                    stdout,
                    "Content-Type: %s\n%c",
                    mimetype,
                    10 );
            }

            fp = VSIFOpenL(
                CPLFormFilename("/vsimem/wcsout", all_files[i], NULL),
                "rb" );
            if( fp == NULL )
            {
                msReleaseLock( TLOCK_GDAL );
                msSetError( MS_MISCERR,
                            "Failed to open %s for streaming to stdout.",
                            "msWCSWriteFile20()", all_files[i] );
                return MS_FAILURE;
            }

            while( (bytes_read = VSIFReadL(block, 1, sizeof(block), fp)) > 0 )
                msIO_fwrite( block, 1, bytes_read, stdout );

            VSIFCloseL( fp );

            VSIUnlink( all_files[i] );
        }

        CSLDestroy( all_files );
        msReleaseLock( TLOCK_GDAL );
        if(multipart)
            msIO_fprintf( stdout, "--wcs--%c%c", 10, 10 );
        return MS_SUCCESS;
    }
#endif /* def GDAL_DCAP_VIRTUALIO */

    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetCoverageMetadata20()                       */
/*                                                                      */
/*      Inits a coverageMetadataObj. Uses msWCSGetCoverageMetadata()    */
/*      but exchanges the SRS URN by an URI for compliancy with 2.0.    */
/************************************************************************/

static int msWCSGetCoverageMetadata20(layerObj *layer, coverageMetadataObj *cm)
{
    char *srs_urn;
    int status = msWCSGetCoverageMetadata(layer, cm);
    if(status != MS_SUCCESS)
        return status;

    if((srs_urn = msOWSGetProjURI(&(layer->projection), &(layer->metadata),
    "COM", MS_TRUE)) == NULL)
    {
        srs_urn = msOWSGetProjURI(&(layer->map->projection),
            &(layer->map->web.metadata),
            "COM", MS_TRUE);
    }
    if( srs_urn != NULL )
    {
        if( strlen(srs_urn) > sizeof(cm->srs_urn) - 1 )
        {
            msSetError(MS_WCSERR, "SRS URN too long!",
                "msWCSGetCoverageMetadata20()");
                return MS_FAILURE;
        }

        strcpy( cm->srs_urn, srs_urn );
        msFree( srs_urn );
    }
    else
        cm->srs_urn[0] = '\0';
    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSException20()                                 */
/*                                                                      */
/*      Writes out an OWS compliant exception.                          */
/************************************************************************/

int msWCSException20(mapObj *map, const char *exceptionCode,
                     const char *locator, const char *version)
{
    int size = 0;
    char *errorString = NULL;
    char *errorMessage = NULL;
    char *schemasLocation = NULL;
    const char * encoding;
    char *xsi_schemaLocation = NULL;
    char version_string[OWS_VERSION_MAXLEN];

    xmlDocPtr psDoc = NULL;
    xmlNodePtr psRootNode = NULL;
    xmlNodePtr psMainNode = NULL;
    xmlNodePtr psNode = NULL;
    xmlNsPtr psNsOws = NULL;
    xmlNsPtr psNsXsi = NULL;
    xmlChar *buffer = NULL;

    psNsOws = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_20_NAMESPACE_URI,
            BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

    encoding = msOWSLookupMetadata(&(map->web.metadata), "CO", "encoding");
    errorString = msGetErrorString("\n");
    errorMessage = msEncodeHTMLEntities(errorString);
    schemasLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

    psDoc = xmlNewDoc(BAD_CAST "1.0");

    psRootNode = xmlNewNode(psNsOws, BAD_CAST "ExceptionReport");
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_20_NAMESPACE_URI,
                BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

    psNsXsi = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

    /* add attributes to root element */
    xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST version);

    xmlNewProp(psRootNode, BAD_CAST "xml:lang", BAD_CAST msOWSGetLanguage(map, "exception"));

    /* get 2 digits version string: '2.0' */
    msOWSGetVersionString(OWS_2_0_0, version_string);
    version_string[3]= '\0';

    xsi_schemaLocation = strdup((char *)psNsOws->href);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *)schemasLocation);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/ows/");
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, version_string);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/owsExceptionReport.xsd");

    /* add namespace'd attributes to root element */
    xmlNewNsProp(psRootNode, psNsXsi, BAD_CAST "schemaLocation", BAD_CAST xsi_schemaLocation);

    /* add child element */
    psMainNode = xmlNewChild(psRootNode, NULL, BAD_CAST "Exception", NULL);

    /* add attributes to child */
    xmlNewProp(psMainNode, BAD_CAST "exceptionCode", BAD_CAST exceptionCode);

    if (locator != NULL)
    {
        xmlNewProp(psMainNode, BAD_CAST "locator", BAD_CAST locator);
    }

    if (errorMessage != NULL)
    {
        psNode = xmlNewChild(psMainNode, NULL, BAD_CAST "ExceptionText", BAD_CAST errorMessage);
    }

    /*psRootNode = msOWSCommonExceptionReport(psNsOws, OWS_2_0_0,
            schemasLocation, version, msOWSGetLanguage(map, "exception"),
            exceptionCode, locator, errorMessage);*/

    xmlDocSetRootElement(psDoc, psRootNode);

    if (encoding)
        msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding, 10, 10);
    else
        msIO_printf("Content-type: text/xml%c%c", 10, 10);

    xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, (encoding ? encoding
            : "ISO-8859-1"), 1);

    msIO_printf("%s", buffer);

    //free buffer and the document
    free(errorString);
    free(errorMessage);
    free(schemasLocation);
    free(xsi_schemaLocation);
    xmlFree(buffer);
    xmlFreeDoc(psDoc);

    /* clear error since we have already reported it */
    msResetErrorList();
    return MS_FAILURE;
}

static int msWCSGetCapabilities20_CreateProfiles(
        mapObj *map, xmlNodePtr psServiceIdentification, xmlNsPtr psOwsNs)
{
    xmlNodePtr psProfile, psTmpNode;
    static const int max_mimes = 20;
    char *available_mime_types[max_mimes];
    int i = 0;
    /* even indices are urls, uneven are mime-types, or null*/
    /* TODO: add urls for EPSG/imageCRS and for other format extensions */
    char *urls_and_mime_types[] =
    {
        MS_WCS_20_PROFILE_CORE,     NULL,
        MS_WCS_20_PROFILE_KVP,      NULL,
        MS_WCS_20_PROFILE_POST,     NULL,
        MS_WCS_20_PROFILE_EPSG,     NULL,
        MS_WCS_20_PROFILE_IMAGECRS, NULL,
        MS_WCS_20_PROFILE_GEOTIFF,  "image/tiff",
        MS_WCS_20_PROFILE_GML_GEOTIFF, NULL,
        NULL, NULL /* guardian */
    };

    /* navigate to node where profiles shall be inserted */
    for(psTmpNode = psServiceIdentification->children; psTmpNode->next != NULL; psTmpNode = psTmpNode->next)
    {
        if(EQUAL((char *)psTmpNode->name, "ServiceTypeVersion"))
            break;
    }

    /* get list of all available mime types */
    /* TODO: maybe driver names are better */
    msGetOutputFormatMimeList(map, available_mime_types, max_mimes);

    while(urls_and_mime_types[i] != NULL)
    {
        char *mime_type;
        mime_type = urls_and_mime_types[i+1];

        /* check if there is a mime type */
        if(mime_type != NULL)
        {
            /* check if the mime_type is in the list of outputformats */
            if(CSLPartialFindString(available_mime_types, mime_type) == -1)
                continue;
        }

        /* create a new node and attach it in the tree */
        psProfile = xmlNewNode(psOwsNs, BAD_CAST "Profile");
        xmlNodeSetContent(psProfile, BAD_CAST urls_and_mime_types[i]);
        xmlAddNextSibling(psTmpNode, psProfile);

        psTmpNode = psProfile;
        i += 2;
    }
}

/************************************************************************/
/*                   msWCSGetCapabilities20_CoverageSummary()           */
/*                                                                      */
/*      Helper function to create a short summary for a specific        */
/*      coverage.                                                       */
/************************************************************************/

static int msWCSGetCapabilities20_CoverageSummary(
    mapObj *map, wcs20ParamsObj *params,
    xmlDocPtr doc, xmlNodePtr psContents, layerObj *layer )
{
    coverageMetadataObj cm;
    int status;
    xmlNodePtr psCSummary;

    xmlNsPtr psWcsNs = xmlSearchNs( doc, xmlDocGetRootElement(doc), BAD_CAST "wcs" );

    status = msWCSGetCoverageMetadata20(layer, &cm);
    if(status != MS_SUCCESS) return MS_FAILURE;

    psCSummary = xmlNewChild( psContents, psWcsNs, BAD_CAST "CoverageSummary", NULL );
    xmlNewChild(psCSummary, psWcsNs, BAD_CAST "CoverageId", BAD_CAST layer->name);
    xmlNewChild(psCSummary, psWcsNs, BAD_CAST "CoverageSubtype", BAD_CAST "RectifiedGridCoverage");

    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetCapabilities20()                           */
/*                                                                      */
/*      Performs the GetCapabilities operation. Writes out a xml        */
/*      structure with server specific information and a summary        */
/*      of all available coverages.                                     */
/************************************************************************/

int msWCSGetCapabilities20(mapObj *map, cgiRequestObj *req,
                           wcs20ParamsObj *params)
{
    xmlDocPtr psDoc = NULL;       /* document pointer */
    xmlNodePtr psRootNode, psOperationsNode, psNode;
    xmlNodePtr psTmpNode, psServiceMetadataNode;
    char *identifier_list = NULL, *format_list = NULL;
    const char *updatesequence = NULL;
    const char *encoding;
    xmlNsPtr psOwsNs = NULL,
            psXLinkNs = NULL,
            psWcsNs = NULL,
            psGmlNs = NULL;
    char *script_url=NULL, *script_url_encoded=NULL;

    xmlChar *buffer = NULL;
    int size = 0, i;
    msIOContext *context = NULL;

    int ows_version = OWS_2_0_0;

/* -------------------------------------------------------------------- */
/*      Build list of layer identifiers available.                      */
/* -------------------------------------------------------------------- */
    identifier_list = strdup("");
    for(i=0; i<map->numlayers; i++)
    {
        layerObj *layer = map->layers[i];
        int       new_length;

        if(!msWCSIsLayerSupported(layer))
            continue;

        new_length = strlen(identifier_list) + strlen(layer->name) + 2;
        identifier_list = (char *) realloc(identifier_list,new_length);

        if( strlen(identifier_list) > 0 )
            strcat( identifier_list, "," );
        strcat( identifier_list, layer->name );
    }

/* -------------------------------------------------------------------- */
/*      Create document.                                                */
/* -------------------------------------------------------------------- */
    psDoc = xmlNewDoc(BAD_CAST "1.0");

    psRootNode = xmlNewNode(NULL, BAD_CAST "Capabilities");

    xmlDocSetRootElement(psDoc, psRootNode);

/* -------------------------------------------------------------------- */
/*      Name spaces                                                     */
/* -------------------------------------------------------------------- */

    msWCSPrepareNamespaces20(psDoc, psRootNode, map);

    /* lookup namespaces */
    psOwsNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX );
    psWcsNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX );
    psGmlNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX );
    psXLinkNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST "xlink" );

    xmlSetNs(psRootNode, psWcsNs);

    xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST params->version );

    /* TODO: remove updatesequence? */
    updatesequence = msOWSLookupMetadata(&(map->web.metadata), "CO", "updatesequence");

    if (updatesequence)
      xmlNewProp(psRootNode, BAD_CAST "updateSequence", BAD_CAST updatesequence);

    psServiceMetadataNode = xmlAddChild(psRootNode, xmlNewNode(psWcsNs, BAD_CAST "ServiceMetadata"));
    xmlNewProp(psServiceMetadataNode, BAD_CAST "version", BAD_CAST "1.0.0");

/* -------------------------------------------------------------------- */
/*      Service metadata.                                               */
/* -------------------------------------------------------------------- */

    psTmpNode = xmlAddChild(psServiceMetadataNode, msOWSCommonServiceIdentification(
                                psOwsNs, map, "OGC WCS", params->version, "CO"));


    msWCSGetCapabilities20_CreateProfiles(map, psTmpNode, psOwsNs);

    psTmpNode = xmlAddChild(psServiceMetadataNode, msOWSCommonServiceProvider(
                                psOwsNs, psXLinkNs, map, "CO"));

/* -------------------------------------------------------------------- */
/*      Operations metadata.                                            */
/* -------------------------------------------------------------------- */
    /*operation metadata */
    if ((script_url=msOWSGetOnlineResource(map, "COM", "onlineresource", req)) == NULL
        || (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
    {
        msSetError(MS_WCSERR, "Server URL not found", "msWCSGetCapabilities20()");
        return msWCSException(map, "mapserv", "NoApplicableCode", params->version);
    }
    free( script_url );


    psOperationsNode = xmlAddChild(psServiceMetadataNode,msOWSCommonOperationsMetadata(psOwsNs));

/* -------------------------------------------------------------------- */
/*      GetCapabilities - add Sections and AcceptVersions?              */
/* -------------------------------------------------------------------- */
    psNode = msOWSCommonOperationsMetadataOperation(
        psOwsNs, psXLinkNs,
        "GetCapabilities", OWS_METHOD_GET, script_url_encoded);

    xmlAddChild(psOperationsNode, psNode);
    /* TODO: Check if 'Parameter' tags are valid in the schema */
    /*xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "service", "WCS"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "version", (char *)params->version));*/

/* -------------------------------------------------------------------- */
/*      DescribeCoverage                                                */
/* -------------------------------------------------------------------- */
    psNode = msOWSCommonOperationsMetadataOperation(
        psOwsNs, psXLinkNs,
        "DescribeCoverage", OWS_METHOD_GET, script_url_encoded);

    xmlAddChild(psOperationsNode, psNode);
    /*xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "service", "WCS"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "version", (char *)params->version));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "identifiers", identifier_list )); */

/* -------------------------------------------------------------------- */
/*      GetCoverage                                                     */
/* -------------------------------------------------------------------- */
    psNode = msOWSCommonOperationsMetadataOperation(
        psOwsNs, psXLinkNs,
        "GetCoverage", OWS_METHOD_GET, script_url_encoded);

    format_list = msWCSGetFormatsList20( map, NULL );

    xmlAddChild(psOperationsNode, psNode);
    /*xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "service", "WCS"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "version", (char *)params->version));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "Identifier", identifier_list ));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "InterpolationType",
                    "NEAREST_NEIGHBOUR,BILINEAR" ));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "format", format_list ));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "store", "false" ));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "GridBaseCRS",
                    "urn:ogc:def:crs:epsg::4326" ));*/

    msFree( format_list );

/* -------------------------------------------------------------------- */
/*      Contents section.                                               */
/* -------------------------------------------------------------------- */
    psOperationsNode = xmlNewChild( psRootNode, psWcsNs, BAD_CAST "Contents", NULL );

    for(i=0; i<map->numlayers; i++)
    {
        layerObj *layer = map->layers[i];
        int       status;

        if(!msWCSIsLayerSupported(layer))
            continue;

        status = msWCSGetCapabilities20_CoverageSummary(
            map, params, psDoc, psOperationsNode, layer );
        if(status != MS_SUCCESS) return MS_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Write out the document.                                         */
/* -------------------------------------------------------------------- */

    msDebug("Write Document");

    msWCSWriteDocument20(map, psDoc);
    xmlFreeDoc(psDoc);

    xmlCleanupParser();

    /* clean up */
    free( script_url_encoded );
    free( identifier_list );

    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSDescribeCoverage20_CoverageDescription()      */
/*                                                                      */
/*      Creates a description of a specific coverage with the three     */
/*      major sections: BoundedBy, DomainSet and RangeType.             */
/************************************************************************/

static int msWCSDescribeCoverage_CoverageDescription20(mapObj *map,
    layerObj *layer, wcs20ParamsObj *params, xmlDocPtr psDoc, xmlNodePtr psRootNode )
{
    int status;
    coverageMetadataObj cm;
    xmlNodePtr psCD;
    xmlNsPtr psWcsNs, psGmlNs, psGmlcovNs, psSweNs, psXLinkNs;
    psWcsNs = psGmlNs = psGmlcovNs = psSweNs = psXLinkNs = NULL;

    psWcsNs    = xmlSearchNs(psDoc, xmlDocGetRootElement(psDoc), BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX);
    psGmlNs    = xmlSearchNs(psDoc, xmlDocGetRootElement(psDoc), BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX);
    psGmlcovNs = xmlSearchNs(psDoc, xmlDocGetRootElement(psDoc), BAD_CAST MS_OWSCOMMON_GMLCOV_NAMESPACE_PREFIX);
    psSweNs    = xmlSearchNs(psDoc, xmlDocGetRootElement(psDoc), BAD_CAST MS_OWSCOMMON_SWE_NAMESPACE_PREFIX);
    psXLinkNs  = xmlSearchNs(psDoc, xmlDocGetRootElement(psDoc), BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);

    /* -------------------------------------------------------------------- */
    /*      Verify layer is processable.                                    */
    /* -------------------------------------------------------------------- */
    if( msCheckParentPointer(layer->map,"map") == MS_FAILURE )
        return MS_FAILURE;

    if(!msWCSIsLayerSupported(layer))
        return MS_SUCCESS;

    /* -------------------------------------------------------------------- */
    /*      Setup coverage metadata.                                        */
    /* -------------------------------------------------------------------- */
    status = msWCSGetCoverageMetadata20(layer, &cm);
    if(status != MS_SUCCESS)
        return status;

    /* fill in bands rangeset info, if required. */
    /* msWCSSetDefaultBandsRangeSetInfo( NULL, &cm, layer ); */

    /* -------------------------------------------------------------------- */
    /*      Create CoverageDescription node.                                */
    /* -------------------------------------------------------------------- */
    psCD = xmlNewChild( psRootNode, psWcsNs, BAD_CAST "CoverageDescription", NULL );
    xmlNewNsProp(psCD, psGmlNs, BAD_CAST "id", BAD_CAST layer->name);

    /* -------------------------------------------------------------------- */
    /*      gml:boundedBy                                                   */
    /* -------------------------------------------------------------------- */
    msWCSCommon20_CreateBoundedBy(layer, &cm, psGmlNs, psCD);

    xmlNewChild(psCD, psWcsNs, BAD_CAST "CoverageId", BAD_CAST layer->name);

    /* -------------------------------------------------------------------- */
    /*      gml:domainSet                                                   */
    /* -------------------------------------------------------------------- */
    msWCSCommon20_CreateDomainSet(layer, &cm, psGmlNs, psCD);

    /* -------------------------------------------------------------------- */
    /*      gmlcov:rangeType                                                */
    /* -------------------------------------------------------------------- */
    msWCSCommon20_CreateRangeType(layer, &cm, psGmlNs, psGmlcovNs, psSweNs, psXLinkNs, psCD);

    /* -------------------------------------------------------------------- */
    /*      wcs:ServiceParameters                                           */
    /* -------------------------------------------------------------------- */
    {
        xmlNodePtr psSP;

        psSP = xmlNewChild( psCD, psWcsNs, BAD_CAST "ServiceParameters", NULL);
        xmlNewChild(psSP, psWcsNs, BAD_CAST "CoverageSubtype", BAD_CAST "RectifiedGridCoverage");

        /* -------------------------------------------------------------------- */
        /*      SupportedCRS                                                    */
        /* -------------------------------------------------------------------- */
        /* TODO: wait for WCS 2.0 CRS extension
         * {
            xmlNodePtr psSupportedCrsList;
            char *owned_value;

            psSupportedCrsList = xmlNewChild(psSP, psWcsNs,
                    BAD_CAST "supportedCrsList", NULL);

            if ((owned_value = msOWSGetProjURN(&(layer->projection),
                    &(layer->metadata), "COM", MS_FALSE)) != NULL)
            {

            }
            else if ((owned_value = msOWSGetProjURN(&(layer->map->projection),
                    &(layer->map->web.metadata), "COM", MS_FALSE)) != NULL)
            {

            }
            else
                msDebug(
                        "mapwcs20.c: missing required information, no SRSs defined.\n");

            if (owned_value != NULL && strlen(owned_value) > 0)
                msLibXml2GenerateList(psSupportedCrsList, psWcsNs,
                        "supportedCRS", owned_value, ' ');

            msFree(owned_value);
        }*/

        /* -------------------------------------------------------------------- */
        /*      SupportedFormats                                                */
        /* -------------------------------------------------------------------- */
        /*{
            xmlNodePtr psSupportedFormatList;
            char *format_list;

            psSupportedFormatList = xmlNewChild(psSP, psWcsNs,
                    BAD_CAST "supportedFomatList", NULL);

            format_list = msWCSGetFormatsList20(layer->map, layer);

            if (strlen(format_list) > 0)
                msLibXml2GenerateList(psSupportedFormatList, psWcsNs,
                        "supportedFormat", format_list, ',');

            msFree(format_list);
        }*/
    }
    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSDescribeCoverage20()                          */
/*                                                                      */
/*      Implementation of the DescibeCoverage Operation. The result     */
/*      of this operation is a xml document, containing specific        */
/*      information about a coverage identified by an ID. The result    */
/*      is written on the stream.                                       */
/************************************************************************/

int msWCSDescribeCoverage20(mapObj *map, wcs20ParamsObj *params)
{
    xmlDocPtr psDoc = NULL; /* document pointer */
    xmlNodePtr psRootNode;
    xmlNsPtr psWcsNs = NULL;
    int i, j;

    /* create DOM document and root node */
    psDoc = xmlNewDoc(BAD_CAST "1.0");
    psRootNode = xmlNewNode(NULL, BAD_CAST "CoverageDescriptions");
    xmlDocSetRootElement(psDoc, psRootNode);

    /* prepare initial namespace definitions */
    msWCSPrepareNamespaces20(psDoc, psRootNode, map);

    psWcsNs = xmlSearchNs(psDoc, psRootNode,
            BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX);
    xmlSetNs(psRootNode, psWcsNs);

    /* check if IDs are given */
    if (params->ids)
    {
        /* for each given ID in the ID-list */
        for (j = 0; params->ids[j]; j++)
        {
            i = msGetLayerIndex(map, params->ids[j]);
            if (i == -1)
            {
                msSetError(MS_WCSERR, "Unknown coverage: (%s)",
                        "msWCSDescribeCoverage20()", params->ids[j]);
                return msWCSException(map, "NoSuchCoverage", "coverage",
                        params->version);
            }
            /* create coverage description for the specified layer */
            if(msWCSDescribeCoverage_CoverageDescription20(map, (GET_LAYER(map, i)),
                    params, psDoc, psRootNode) == MS_FAILURE)
            {
                msSetError(MS_WCSERR, "Error retrieving coverage description.", "msWCSDescribeCoverage20()");
                return msWCSException(map, "MissingParameterValue", "coverage",
                        params->version);
            }
        }
    }
    else
    {   /* Throw error, since IDs are mandatory */
        msSetError(MS_WCSERR, "Missing COVERAGEID parameter.", "msWCSDescribeCoverage20()");
        return msWCSException(map, "MissingParameterValue", "coverage",
                params->version);
    }

    /* write out the DOM document to the stream */
    msWCSWriteDocument20(map, psDoc);

    /* cleanup */
    xmlFreeDoc(psDoc);
    xmlCleanupParser();

    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetCoverage20()                               */
/*                                                                      */
/*      Implementation of the GetCoverage Operation. The coverage       */
/*      is either returned as an image or as a multipart xml/image.     */
/*      The result is written on the stream.                            */
/************************************************************************/

int msWCSGetCoverage20(mapObj *map, cgiRequestObj *request,
                       wcs20ParamsObj *params)
{
    layerObj *layer = NULL;
    coverageMetadataObj cm;
    imageObj *image = NULL;
    outputFormatObj *format = NULL;

    int status, i;
    double x_1, x_2, y_1, y_2;
    char *coverageName, *bandlist=NULL, numbands[8];

    msDebug("msWCSGetCoverage20(): Start\n");

    /* number of coverage ids should be 1 */
    if (params->ids == NULL || params->ids[0] == NULL) {
        msSetError(MS_WCSERR, "Required parameter CoverageID was not supplied.",
                   "msWCSGetCoverage20()");
        return msWCSException(map, "MissingParameterValue", "coverage",
                              params->version);
    }
    if (params->ids[1] != NULL) {
        msSetError(MS_WCSERR, "GetCoverage operation supports only one layer.",
                   "msWCSGetCoverage20()");
        return msWCSException(map, "TooManyParameterValues", "coverage",
                              params->version);
    }

    msDebug("msWCSGetCoverage20(): Find Layer\n");

    /* find the right layer */
    layer = NULL;
    for(i = 0; i < map->numlayers; i++) {
        coverageName = msOWSGetEncodeMetadata(&(GET_LAYER(map, i)->metadata),
                                              "COM", "name",
                                              GET_LAYER(map, i)->name);
        if (EQUAL(coverageName, params->ids[0]))
        {
            layer = GET_LAYER(map, i);
            break;
        }
    }

    /* throw exception if no Layer was found */
    if (layer == NULL)
    {
        msSetError(MS_WCSERR,
                "COVERAGE=%s not found, not in supported layer list.",
                "msWCSGetCoverage20()", params->ids[0]);
        return msWCSException(map, "InvalidParameterValue", "coverage",
                params->version);
    }

    /* retrieve coverage metadata  */
    status = msWCSGetCoverageMetadata20(layer, &cm);
    if (status != MS_SUCCESS) return MS_FAILURE;

    /* fill in bands rangeset info, if required.  */
    msWCSSetDefaultBandsRangeSetInfo(NULL, &cm, layer );

    /* set  resolution, size and maximum extent */
    layer->extent = map->extent = cm.extent;
    map->cellsize = cm.xresolution;
    map->width = cm.xsize;
    map->height = cm.ysize;


    /* project image and create bounding box */
    {
        rectObj subsets, tmpBBOX;
        projectionObj imageProj;

        char crs[100];
        msInitProjection(&imageProj);
        msLoadProjectionString(&imageProj, cm.srs);

        if(msWCSCreateBoungingBoxAndGetProjection20(params,
                &subsets, crs, sizeof(crs)/sizeof(char)) == MS_FAILURE)
        {
            return msWCSException(map, "ExtentError", "extent", params->version);
        }
        msDebug("msWCSGetCoverage20(): subsets %f %f %f %f\n", subsets.minx, subsets.miny, subsets.maxx,subsets.maxy);

        if(!EQUAL(crs, "imageCRS"))
        {
            projectionObj reqProj;
            /* if the subsets have a crs given, project the image extent to it */
            msInitProjection(&reqProj);
            msLoadProjectionString(&reqProj, crs);

            msProjectRect(&imageProj, &reqProj, &(layer->extent));
            map->extent = layer->extent;
            map->projection = reqProj;
        }
        else
        {
            /* subsets are in imageCRS; reproject them to real coordinates */
            rectObj orig_bbox = subsets;

            map->projection = imageProj;

            if(subsets.minx != -DBL_MAX || subsets.maxx != DBL_MAX)
            {
                x_1 =
                        cm.geotransform[0]
                    + orig_bbox.minx * cm.geotransform[1]
                    + orig_bbox.miny * cm.geotransform[2];
                /*subsets.minx += cm.geotransform[1]/2 + cm.geotransform[2]/2;*/
                x_2 =
                        cm.geotransform[0]
                    + (orig_bbox.maxx+1) * cm.geotransform[1]
                    + (orig_bbox.maxy+1) * cm.geotransform[2];
                /*subsets.maxx -= cm.geotransform[1]/2 + cm.geotransform[2]/2;*/

                subsets.minx = MIN(x_1, x_2);
                subsets.maxx = MAX(x_1, x_2);
            }
            if(subsets.miny != -DBL_MAX || subsets.maxy != DBL_MAX)
            {
                y_1 =
                        cm.geotransform[3]
                    + (orig_bbox.maxx+1) * cm.geotransform[4]
                    + (orig_bbox.maxy+1) * cm.geotransform[5];
                /*subsets.miny -= cm.geotransform[4]/2 + cm.geotransform[5]/2;*/
                y_2 =
                        cm.geotransform[3]
                    + orig_bbox.minx * cm.geotransform[4]
                    + orig_bbox.miny * cm.geotransform[5];
                /*subsets.maxy += cm.geotransform[4]/2 + cm.geotransform[5]/2;*/

                subsets.miny = MIN(y_1, y_2);
                subsets.maxy = MAX(y_1, y_2);
            }
        }

        /* create boundings of params subsets and image extent */
        if(msRectOverlap(&subsets, &(layer->extent)) == MS_FALSE)
        {
            /* extent and bbox do not overlap -> exit */
            char extent[500], bbox[1000];
            msRectToFormattedString(&(map->extent), "(%f,%f,%f,%f)", extent, 499);
            msRectToFormattedString(&subsets, "(%f,%f,%f,%f)", bbox, 999);
            msDebug("msWCSGetCoverage20(): extent %s, bbox %s\n", extent, bbox);
            msSetError(MS_WCSERR, "Image extent does not intersect"
                    "with desired region.", "msWCSGetCoverage20()");
            return msWCSException(map, "ExtentError", "extent", params->version);
        }
        /* write combined bounding box */
        tmpBBOX.minx = MAX(subsets.minx, map->extent.minx);
        tmpBBOX.miny = MAX(subsets.miny, map->extent.miny);
        tmpBBOX.maxx = MIN(subsets.maxx, map->extent.maxx);
        tmpBBOX.maxy = MIN(subsets.maxy, map->extent.maxy);
        msDebug("msWCSGetCoverage20(): BBOX %f %f %f %f\n", tmpBBOX.minx, tmpBBOX.miny, tmpBBOX.maxx,tmpBBOX.maxy);

        /* recalculate image width and height */
        if(ABS(tmpBBOX.maxx - tmpBBOX.minx) != ABS(map->extent.maxx - map->extent.minx))
        {
            double total = ABS(map->extent.maxx - map->extent.minx),
                    part = ABS(tmpBBOX.maxx - tmpBBOX.minx);
            map->width = lround((part * map->width) / total);
        }
        if(ABS(tmpBBOX.maxy - tmpBBOX.miny) != ABS(map->extent.maxy - map->extent.miny))
        {
            double total = ABS(map->extent.maxy - map->extent.miny),
                    part = ABS(tmpBBOX.maxy - tmpBBOX.miny);
            map->height = lround((part * map->height) / total);
        }

        msDebug("msGetCoverage20(): width: %d height: %d\n", map->width, map->height);

        /* set the bounding box as new map extent */
        layer->extent = map->extent = tmpBBOX;
    }


    msDebug("msWCSGetCoverage20(): Set parameters from original"
                   "data. Width: %d, height: %d, cellsize: %f, extent: %f,%f,%f,%f\n",
               map->width, map->height, map->cellsize, map->extent.minx,
               map->extent.miny, map->extent.maxx, map->extent.maxy);

    if (!params->format)
    {
        msSetError(MS_WCSERR, "Parameter FORMAT is required",
                "msWCSGetCoverage20()");
        return msWCSException(map, "MissingParameterValue", "format",
                params->version);
    }

    /*    make sure layer is on   */
    layer->status = MS_ON;

    msMapComputeGeotransform(map);

    /*    fill in bands rangeset info, if required.  */
    //msWCSSetDefaultBandsRangeSetInfo(params, &cm, layer);
    //msDebug("Bandcount: %d\n", cm.bandcount);

    if (msGetOutputFormatIndex(map, params->format) == -1)
    {
        msSetError(MS_WCSERR, "Unrecognized value for the FORMAT parameter.",
                "msWCSGetCoverage20()");
        return msWCSException(map, "InvalidParameterValue", "format",
                params->version);
    }

    /* create a temporary outputformat (we likely will need to tweak parts) */
    format = msCloneOutputFormat(msSelectOutputFormat(map, params->format));
    msApplyOutputFormat(&(map->outputformat), format, MS_NOOVERRIDE,
            MS_NOOVERRIDE, MS_NOOVERRIDE);

    //status = msWCSGetCoverageBands11( map, request, params, layer, &bandlist );
    if(!bandlist) { // build a bandlist (default is ALL bands)
        bandlist = (char *) malloc(cm.bandcount*30+30 );
        strcpy(bandlist, "1");
        for(i = 1; i < cm.bandcount; i++)
            sprintf(bandlist+strlen(bandlist),",%d", i+1);
    }
    msLayerSetProcessingKey(layer, "BANDS", bandlist);
    sprintf(numbands, "%d", msCountChars(bandlist, ',')+1);
    msSetOutputFormatOption(map->outputformat, "BAND_COUNT", numbands);

    /* create the image object  */
    if (!map->outputformat)
    {
        msSetError(MS_WCSERR, "The map outputformat is missing!",
                "msWCSGetCoverage20()");
        return msWCSException(map, NULL, NULL, params->version);
    }
    else if (MS_RENDERER_GD(map->outputformat))
    {
        image = msImageCreateGD(map->width, map->height, map->outputformat,
                map->web.imagepath, map->web.imageurl, map->resolution,
                map->defresolution);
        if (image != NULL)
            msImageInitGD(image, &map->imagecolor);
#ifdef USE_AGG
    }
    else if (MS_RENDERER_AGG(map->outputformat))
    {
        image = msImageCreateAGG(map->width, map->height, map->outputformat,
                map->web.imagepath, map->web.imageurl, map->resolution,
                map->defresolution);
        if (image != NULL)
            msImageInitAGG(image, &map->imagecolor);
#endif
    }
    else if (MS_RENDERER_RAWDATA(map->outputformat))
    {
        image = msImageCreate(map->width, map->height, map->outputformat,
                map->web.imagepath, map->web.imageurl, map);
    }
    else
    {
        msSetError(MS_WCSERR, "Map outputformat not supported for WCS!",
                "msWCSGetCoverage20()");
        return msWCSException(map, NULL, NULL, params->version);
    }

    if (image == NULL)
        return msWCSException(map, NULL, NULL, params->version);

    /* Actually produce the "grid". */
    status = msDrawRasterLayerLow( map, layer, image, NULL );
    if( status != MS_SUCCESS ) {
        return msWCSException(map, NULL, NULL, params->version );
    }
    msDebug("image:%s\n", image->imagepath);

    /* GML+GeoTIFF */
    /* Embed the GeoTIFF into multipart message */
    if(params->multipart)
    {
        xmlDocPtr psDoc = NULL;       /* document pointer */
        xmlNodePtr psRootNode, psRangeSet, psFile;
        xmlNsPtr psGmlNs = NULL,
            psGmlcovNs = NULL,
            psSweNs = NULL,
            psWcsNs = NULL,
            psXLinkNs = NULL;
        coverageMetadataObj tmpCm;

        /* Create Document  */
        psDoc = xmlNewDoc(BAD_CAST "1.0");
        psRootNode = xmlNewNode(NULL, BAD_CAST MS_WCS_GML_COVERAGETYPE_RECTIFIED_GRID_COVERAGE);
        xmlDocSetRootElement(psDoc, psRootNode);

        msWCSPrepareNamespaces20(psDoc, psRootNode, map);

        psGmlNs    = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX);
        psGmlcovNs = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_GMLCOV_NAMESPACE_PREFIX);
        psSweNs    = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_SWE_NAMESPACE_PREFIX);
        psWcsNs    = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX);
        psXLinkNs  = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);

        xmlNewNsProp(psRootNode, psGmlNs, BAD_CAST "id", BAD_CAST layer->name);

        xmlSetNs(psRootNode, psGmlcovNs);


        tmpCm = cm;
        tmpCm.extent = map->extent;
        tmpCm.xsize = map->width;
        tmpCm.ysize = map->height;

        /* Setup layer information  */
        msWCSCommon20_CreateBoundedBy(layer, &tmpCm, psGmlNs, psRootNode);
        msWCSCommon20_CreateDomainSet(layer, &tmpCm, psGmlNs, psRootNode);

        psRangeSet = xmlNewChild(psRootNode, psGmlNs, BAD_CAST "rangeSet", NULL);
        psFile     = xmlNewChild(psRangeSet, psGmlNs, BAD_CAST "File", NULL);

        /* TODO: wait for updated specifications */
        xmlNewChild(psFile, psGmlNs, BAD_CAST "rangeParameters", NULL);
        xmlNewChild(psFile, psGmlNs, BAD_CAST "fileReference", BAD_CAST "file");
        xmlNewChild(psFile, psGmlNs, BAD_CAST "fileStructure", NULL);

        msWCSCommon20_CreateRangeType(layer, &cm, psGmlNs, psGmlcovNs, psSweNs, psXLinkNs, psRootNode);

        msIO_printf( "Content-Type: multipart/mixed; boundary=wcs%c%c"
                     "--wcs\n", 10, 10);

        msWCSWriteDocument20(map, psDoc);
        msWCSWriteFile20(map, image, params, 1);
        xmlFreeDoc(psDoc);
        xmlCleanupParser();
    }
    /* just print out the file without gml */
    else
    {
        msWCSWriteFile20(map, image, params, 0);
    }
    return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSDispatch20()                                  */
/*                                                                      */
/*      Dispatches a mapserver request. First the cgiRequest is         */
/*      parsed. Afterwards version and service are beeing checked.      */
/*      If they aren't compliant, MS_DONE is returned. Otherwise        */
/*      either GetCapabilities, DescribeCoverage or GetCoverage         */
/*      operations are executed.                                        */
/************************************************************************/

int msWCSDispatch20(mapObj *map, cgiRequestObj *request)
{
    wcs20ParamsObj *params = NULL;
    int returnValue = MS_FAILURE, status;

    params = msWCSCreateParamsObj20();
    status = msWCSParseRequest20(request, params);

    if(status == MS_FAILURE)
    {
        msDebug("msWCSDispatch20: Parse error occured.");
        msWCSException20(map, "ParseFailure", "request", "2.0.0" );
        msWCSFreeParamsObj20(params);
        return MS_FAILURE;
    }
    else if(status == MS_DONE)
    {
        /* could not be parsed, but no error */
        /* continue for now... */
    }

    /* if version and service are not set, or not
    * right for WCS 2.0 -> return with MS_DONE */
    if(!params->version || !params->service
        || !EQUALN(params->version, "2.0", 3)
        || !EQUAL(params->service, "WCS"))
    {
        msDebug("msWCSDispatch20: version and service are not compliant with WCS 2.0\n");
        msWCSFreeParamsObj20(params);
        msResetErrorList();
        return MS_DONE;
    }

    /* if no request was set */
    if(!params->request)
    {
        msSetError(MS_WCSERR, "Missing REQUEST parameter", "msWCSDispatch20()");
        msWCSException20(map, "MissingParameterValue", "request",
                params->version );
        msWCSFreeParamsObj20(params); /* clean up */
        return MS_FAILURE;
    }

    /*debug output the parsed parameters*/
    int x = 0;
    msDebug("ParamsObj:\n");
    msDebug("\tVersion: %s\n\tRequest: %s\n\tService: %s\n", params->version, params->request, params->service);
    msDebug("\tIDs:\n");
    for (x = 0; params->ids && params->ids[x]; ++x) {
        msDebug("\t\t%s\n", params->ids[x]);
    }
    if(params->numsubsets > 0)
        msDebug("\tSubsets:\n");
    for(x = 0; x < params->numsubsets; ++x)
    {
        wcs20SubsetObj* p = params->subsets[x];
        msDebug("\t\tAxisName: %s\n", p->axis);
        msDebug("\t\tOperation: %s\n", p->operation?"SLICE":"TRIM");
        msDebug("\t\tCRS: %s\n", p->crs?p->crs:"None");
        if(p->timeOrScalar == MS_WCS20_TIME_VALUE) {
            if (p->operation == MS_WCS20_TRIM) {
                msDebug("\t\tInterval: [");
                if (!p->min.unbounded) msDebug("%s;", msWCSProperTime20(&(p->min.time)));
                else msDebug("*;");
                if (!p->max.unbounded) msDebug("%s]\n", msWCSProperTime20(&(p->max.time)));
                else msDebug("*]\n");
            }
            if (p->operation == MS_WCS20_TRIM) {
                if (p->min.unbounded)
                    msDebug("\t\tPoint: (%s)\n", msWCSProperTime20(&(p->min.time)));
            }
        }
        else {
            if (p->operation == MS_WCS20_TRIM) {
                msDebug("\t\tInterval: [");
                if (!p->min.unbounded) msDebug("%.2f;", p->min.scalar);
                else msDebug("*;");
                if (!p->max.unbounded) msDebug("%.2f]\n", p->max.scalar);
                else msDebug("*]\n");
            }
            if(p->operation == MS_WCS20_SLICE)
                msDebug("\t\tPoint: (%.2f)\n", p->min.scalar);
        }
        msDebug("\n");
    }

    /* Call operation specific functions */
    if (EQUAL(params->request, "GetCapabilities"))
    {
        returnValue = msWCSGetCapabilities20(map, request, params);
    }
    else if (EQUAL(params->request, "DescribeCoverage"))
    {
        returnValue = msWCSDescribeCoverage20(map, params);
    }
    else if (EQUAL(params->request, "GetCoverage"))
    {
        returnValue = msWCSGetCoverage20(map, request, params);
    }
    else
    {
        msSetError(MS_WCSERR, "Unknown request", "msWCSDispatch20()");
        returnValue = msWCSException20(map, "MissingParameterValue", "request",
                                   params->version);
    }
    /* clean up */
    msWCSFreeParamsObj20(params);
    return returnValue;
}
