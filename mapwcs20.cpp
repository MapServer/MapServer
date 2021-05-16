/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OpenGIS Web Coverage Server (WCS) 2.0 implementation.
 * Author:   Stephan Meissl <stephan.meissl@eox.at>
 *           Fabian Schindler <fabian.schindler@eox.at>
 *           and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 2010, 2011 EOX IT Services GmbH, Austria
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

#include "mapserver-config.h"
#if defined(USE_WCS_SVR)

#include <assert.h>
#include "mapserver.h"
#include "maperror.h"
#include "mapthread.h"
#include "mapows.h"
#include "mapwcs.h"
#include "mapgdal.h"
#include <float.h>
#include "gdal.h"
#include "cpl_port.h"
#include "maptime.h"
#include "mapprimitive.h"
#include "cpl_string.h"
#include <string.h>

#if defined(USE_LIBXML2)

#include <libxml/tree.h>
#include "maplibxml2.h"
#include <libxml/parser.h>

#endif /* defined(USE_LIBXML2) */

#include <string>

/************************************************************************/
/*                          msXMLStripIndentation                       */
/************************************************************************/

static void msXMLStripIndentation(char* ptr)
{
    /* Remove spaces between > and < to get properly indented result */
    char* afterLastClosingBracket = NULL;
    if( *ptr == ' ' )
        afterLastClosingBracket = ptr;
    while( *ptr != '\0' )
    {
        if( *ptr == '<' && afterLastClosingBracket != NULL )
        {
            memmove(afterLastClosingBracket, ptr, strlen(ptr) + 1);
            ptr = afterLastClosingBracket;
        }
        else if( *ptr == '>' )
        {
            afterLastClosingBracket = ptr + 1;
        }
        else if( *ptr != ' ' &&  *ptr != '\n' )
            afterLastClosingBracket = NULL;
        ptr ++;
    }
}



/************************************************************************/
/*                   msStringParseInteger()                             */
/*                                                                      */
/*      Tries to parse a string as a integer value. If not possible     */
/*      the value MS_WCS20_UNBOUNDED is stored as value.                */
/*      If no characters could be parsed, MS_FAILURE is returned. If at */
/*      least some characters could be parsed, MS_DONE is returned and  */
/*      only if all characters could be parsed, MS_SUCCESS is returned. */
/************************************************************************/

static int msStringParseInteger(const char *string, int *dest)
{
  char *parse_check;
  *dest = (int)strtol(string, &parse_check, 0);
  if(parse_check == string) {
    return MS_FAILURE;
  } else if(parse_check - strlen(string) != string) {
    return MS_DONE;
  }
  return MS_SUCCESS;
}

/************************************************************************/
/*                   msStringParseDouble()                              */
/*                                                                      */
/*      Tries to parse a string as a double value. If not possible      */
/*      the value 0 is stored as value.                                 */
/*      If no characters could be parsed, MS_FAILURE is returned. If at */
/*      least some characters could be parsed, MS_DONE is returned and  */
/*      only if all characters could be parsed, MS_SUCCESS is returned. */
/************************************************************************/

static int msStringParseDouble(const char *string, double *dest)
{
  char *parse_check = NULL;
  *dest = strtod(string, &parse_check);
  if(parse_check == string) {
    return MS_FAILURE;
  } else if(parse_check - strlen(string) != string) {
    return MS_DONE;
  }
  return MS_SUCCESS;
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
  if (string) {
    while(*string == ' ')
      string ++;
  }

  if (!string || strlen(string) == 0 || !u) {
    msSetError(MS_WCSERR, "Invalid string", "msWCSParseTimeOrScalar20()");
    return MS_WCS20_ERROR_VALUE;
  }
  /* if the string is equal to "*" it means the value
   *  of the interval is unbounded                    */
  if (EQUAL(string, "*")) {
    u->scalar = MS_WCS20_UNBOUNDED;
    u->unbounded = 1;
    return MS_WCS20_UNDEFINED_VALUE;
  }

  /* if returned a valid value, use it */
  if (msStringParseDouble(string, &(u->scalar)) == MS_SUCCESS) {
    return MS_WCS20_SCALAR_VALUE;
  }
  /* otherwise it might be a time value */
  msTimeInit(&time);
  if (msParseTime(string, &time) == MS_TRUE) {
    u->time = mktime(&time);
    return MS_WCS20_TIME_VALUE;
  }
  /* the value could neither be parsed as a double nor as a time value */
  else {
    msSetError(MS_WCSERR,
               "String %s could not be parsed to a time or scalar value",
               "msWCSParseTimeOrScalar20()",string);
    return MS_WCS20_ERROR_VALUE;
  }
}

/************************************************************************/
/*                   msWCSCreateSubsetObj20()                           */
/*                                                                      */
/*      Creates a new wcs20SubsetObj and initializes it to standard     */
/*      values.                                                         */
/************************************************************************/

static wcs20SubsetObjPtr msWCSCreateSubsetObj20()
{
  wcs20SubsetObjPtr subset = (wcs20SubsetObjPtr) malloc(sizeof(wcs20SubsetObj));
  MS_CHECK_ALLOC(subset, sizeof(wcs20SubsetObj), NULL);

  subset->axis = NULL;
  subset->crs = NULL;
  subset->min.scalar = subset->max.scalar = MS_WCS20_UNBOUNDED;
  subset->min.unbounded = subset->max.unbounded = 0;
  subset->operation = MS_WCS20_SLICE;

  return subset;
}

/************************************************************************/
/*                   msWCSFreeSubsetObj20()                             */
/*                                                                      */
/*      Frees a wcs20SubsetObj and releases all linked resources.       */
/************************************************************************/

static void msWCSFreeSubsetObj20(wcs20SubsetObjPtr subset)
{
  if (NULL == subset) {
    return;
  }
  msFree(subset->axis);
  msFree(subset->crs);
  msFree(subset);
}

/************************************************************************/
/*                   msWCSCreateAxisObj20()                             */
/*                                                                      */
/*      Creates a new wcs20AxisObj and initializes it to standard       */
/*      values.                                                         */
/************************************************************************/

static wcs20AxisObjPtr msWCSCreateAxisObj20()
{
  wcs20AxisObj *axis = (wcs20AxisObjPtr)malloc(sizeof(wcs20AxisObj));
  MS_CHECK_ALLOC(axis, sizeof(wcs20AxisObj), NULL);

  axis->name = NULL;
  axis->size = 0;
  axis->resolution = MS_WCS20_UNBOUNDED;
  axis->scale = MS_WCS20_UNBOUNDED;
  axis->resolutionUOM = NULL;
  axis->subset = NULL;

  return axis;
}

/************************************************************************/
/*                   msWCSFreeAxisObj20()                               */
/*                                                                      */
/*      Frees a wcs20AxisObj and releases all linked resources.         */
/************************************************************************/

static void msWCSFreeAxisObj20(wcs20AxisObjPtr axis)
{
  if(NULL == axis) {
    return;
  }

  msFree(axis->name);
  msFree(axis->resolutionUOM);
  msWCSFreeSubsetObj20(axis->subset);
  msFree(axis);
}

/************************************************************************/
/*                   msWCSFindAxis20()                                  */
/*                                                                      */
/*      Helper function to retrieve an axis by the name from a params   */
/*      object.                                                         */
/************************************************************************/

static wcs20AxisObjPtr msWCSFindAxis20(wcs20ParamsObjPtr params,
                                       const char *name)
{
  int i = 0;
  for(i = 0; i < params->numaxes; ++i) {
    if(EQUAL(params->axes[i]->name, name)) {
      return params->axes[i];
    }
  }
  return NULL;
}

/************************************************************************/
/*                   msWCSInsertAxisObj20()                             */
/*                                                                      */
/*      Helper function to insert an axis object into the axes list of  */
/*      a params object.                                                */
/************************************************************************/

static void msWCSInsertAxisObj20(wcs20ParamsObjPtr params, wcs20AxisObjPtr axis)
{
  params->numaxes++;
  params->axes = (wcs20AxisObjPtr*) msSmallRealloc(params->axes,
                 sizeof(wcs20AxisObjPtr) * (params->numaxes));
  params->axes[params->numaxes - 1] = axis;
}

/************************************************************************/
/*                   msWCSCreateParamsObj20()                           */
/*                                                                      */
/*      Creates a new wcs20ParamsObj and initializes it to standard     */
/*      values.                                                         */
/************************************************************************/

wcs20ParamsObjPtr msWCSCreateParamsObj20()
{
  wcs20ParamsObjPtr params
    = (wcs20ParamsObjPtr) malloc(sizeof(wcs20ParamsObj));
  MS_CHECK_ALLOC(params, sizeof(wcs20ParamsObj), NULL);

  params->version         = NULL;
  params->request         = NULL;
  params->service         = NULL;
  params->accept_versions = NULL;
  params->accept_languages = NULL;
  params->sections        = NULL;
  params->updatesequence  = NULL;
  params->ids             = NULL;
  params->width           = 0;
  params->height          = 0;
  params->resolutionX     = MS_WCS20_UNBOUNDED;
  params->resolutionY     = MS_WCS20_UNBOUNDED;
  params->scale           = MS_WCS20_UNBOUNDED;
  params->scaleX          = MS_WCS20_UNBOUNDED;
  params->scaleY          = MS_WCS20_UNBOUNDED;
  params->resolutionUnits = NULL;
  params->numaxes         = 0;
  params->axes            = NULL;
  params->format          = NULL;
  params->multipart       = 0;
  params->interpolation   = NULL;
  params->outputcrs       = NULL;
  params->subsetcrs       = NULL;
  params->bbox.minx = params->bbox.miny = -DBL_MAX;
  params->bbox.maxx = params->bbox.maxy =  DBL_MAX;
  params->range_subset    = NULL;
  params->format_options  = NULL;

  return params;
}

/************************************************************************/
/*                   msWCSFreeParamsObj20()                             */
/*                                                                      */
/*      Frees a wcs20ParamsObj and releases all linked resources.       */
/************************************************************************/

void msWCSFreeParamsObj20(wcs20ParamsObjPtr params)
{
  if (NULL == params) {
    return;
  }

  msFree(params->version);
  msFree(params->request);
  msFree(params->service);
  CSLDestroy(params->accept_versions);
  CSLDestroy(params->accept_languages);
  CSLDestroy(params->sections);
  msFree(params->updatesequence);
  CSLDestroy(params->ids);
  msFree(params->resolutionUnits);
  msFree(params->format);
  msFree(params->interpolation);
  msFree(params->outputcrs);
  msFree(params->subsetcrs);
  while(params->numaxes > 0) {
    params->numaxes -= 1;
    msWCSFreeAxisObj20(params->axes[params->numaxes]);
  }
  msFree(params->axes);
  CSLDestroy(params->range_subset);
  CSLDestroy(params->format_options);
  msFree(params);
}

/************************************************************************/
/*                   msWCSParseSubset20()                               */
/*                                                                      */
/*      Parses several string parameters and fills them into the        */
/*      subset object.                                                  */
/************************************************************************/

static int msWCSParseSubset20(wcs20SubsetObjPtr subset, const char *axis,
                              const char *crs, const char *min, const char *max)
{
  int ts1, ts2;
  ts1 = ts2 = MS_WCS20_UNDEFINED_VALUE;

  if (subset == NULL) {
    return MS_FAILURE;
  }

  if (axis == NULL || strlen(axis) == 0) {
    msSetError(MS_WCSERR, "Subset axis is not given.",
               "msWCSParseSubset20()");
    return MS_FAILURE;
  }

  subset->axis = msStrdup(axis);
  if (crs != NULL) {
    subset->crs = msStrdup(crs);
  }

  /* Parse first (probably only) part of interval/point;
   * check whether its a time value or a scalar value     */
  ts1 = msWCSParseTimeOrScalar20(&(subset->min), min);
  if (ts1 == MS_WCS20_ERROR_VALUE) {
    return MS_FAILURE;
  }

  /* check if its an interval */
  /* if there is a comma, then it is */
  if (max != NULL && strlen(max) > 0) {
    subset->operation = MS_WCS20_TRIM;

    /* Parse the second value of the interval */
    ts2 = msWCSParseTimeOrScalar20(&(subset->max), max);
    if (ts2 == MS_WCS20_ERROR_VALUE) {
      return MS_FAILURE;
    }

    /* if at least one boundary is defined, use that value */
    if ((ts1 == MS_WCS20_UNDEFINED_VALUE) ^ (ts2
        == MS_WCS20_UNDEFINED_VALUE)) {
      if (ts1 == MS_WCS20_UNDEFINED_VALUE) {
        ts1 = ts2;
      }
    }
    /* if time and scalar values do not fit, throw an error */
    else if (ts1 != MS_WCS20_UNDEFINED_VALUE && ts2
             != MS_WCS20_UNDEFINED_VALUE && ts1 != ts2) {
      msSetError(MS_WCSERR,
                 "Interval error: minimum is a %s value, maximum is a %s value",
                 "msWCSParseSubset20()", ts1 ? "time" : "scalar",
                 ts2 ? "time" : "scalar");
      return MS_FAILURE;
    }
    /* if both min and max are unbounded -> throw an error */
    if (subset->min.unbounded && subset->max.unbounded) {
      msSetError(MS_WCSERR, "Invalid values: no bounds could be parsed",
                 "msWCSParseSubset20()");
      return MS_FAILURE;
    }
  }
  /* there is no second value, therefore it is a point.
   * consequently set the operation to slice */
  else {
    subset->operation = MS_WCS20_SLICE;
    if (ts1 == MS_WCS20_UNDEFINED_VALUE) {
      msSetError(MS_WCSERR, "Invalid point value given",
                 "msWCSParseSubset20()");
      return MS_FAILURE;
    }
  }

  subset->timeOrScalar = ts1;

  /* check whether the min is smaller than the max */
  if (subset->operation == MS_WCS20_TRIM) {
    if(subset->timeOrScalar == MS_WCS20_SCALAR_VALUE && subset->min.scalar == MS_WCS20_UNBOUNDED) {
      subset->min.scalar = -MS_WCS20_UNBOUNDED;
    }

    if (subset->timeOrScalar == MS_WCS20_TIME_VALUE && subset->min.time
        > subset->max.time) {
      msSetError(MS_WCSERR,
                 "Minimum value of subset axis %s is larger than maximum value",
                 "msWCSParseSubset20()", subset->axis);
      return MS_FAILURE;
    }
    if (subset->timeOrScalar == MS_WCS20_SCALAR_VALUE && subset->min.scalar > subset->max.scalar) {
      msSetError(MS_WCSERR,
                 "Minimum value (%f) of subset axis '%s' is larger than maximum value (%f).",
                 "msWCSParseSubset20()", subset->min.scalar, subset->axis, subset->max.scalar);
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
/*      Subset string: axis [ , crs ] ( intervalOrPoint )               */
/************************************************************************/

static int msWCSParseSubsetKVPString20(wcs20SubsetObjPtr subset, char *string)
{
  char *axis, *crs, *min, *max;

  axis = string;
  crs = NULL;
  min = NULL;
  max = NULL;

  /* find first '(' */
  min = strchr(string, '(');

  /* if min could not be found, the string is invalid */
  if (min == NULL) {
    msSetError(MS_WCSERR, "Invalid axis subset string: '%s'",
               "msWCSParseSubsetKVPString20()", string);
    return MS_FAILURE;
  }
  /* set min to first letter */
  *min = '\0';
  ++min;

  /* cut the trailing ')' */
  if (min[strlen(min) - 1] == ')') {
    min[strlen(min) - 1] = '\0';
  }
  /* look if also a max is defined */
  max = strchr(min, ',');
  if (max != NULL) {
    *max = '\0';
    ++max;
  }

  /* look if also a crs is defined */
  crs = strchr(axis, ',');
  if (crs != NULL) {
    *crs = '\0';
    ++crs;
  }

  return msWCSParseSubset20(subset, axis, crs, min, max);
}

/************************************************************************/
/*                   msWCSParseSizeString20()                           */
/*                                                                      */
/*      Parses a string containing the axis and the size as an integer. */
/*      Size string: axis ( size )                                      */
/************************************************************************/

static int msWCSParseSizeString20(char *string, char *outAxis, size_t axisStringLen, int *outSize)
{
  char *number = NULL;
  char *check = NULL;

  /* find first '(', the character before the number */
  number = strchr(string, '(');

  if(NULL == number) {
    msSetError(MS_WCSERR, "Invalid size parameter value.",
               "msWCSParseSize20()");
    return MS_FAILURE;
  }

  /* cut trailing ')' */
  check = strchr(string, ')');
  if(NULL == check) {
    msSetError(MS_WCSERR, "Invalid size parameter value.",
               "msWCSParseSize20()");
    return MS_FAILURE;
  }
  *number = '\0';
  ++number;
  *check = '\0';

  strlcpy(outAxis, string, axisStringLen);

  /* parse size value */
  if(msStringParseInteger(number, outSize) != MS_SUCCESS) {
    msSetError(MS_WCSERR, "Parameter value '%s' is not a valid integer.",
               "msWCSParseSize20()", number);
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSParseResolutionString20()                     */
/*                                                                      */
/*      Parses a resolution string and returns the axis, the units of   */
/*      measure and the resolution value.                               */
/*      Subset string: axis ( value )                                   */
/************************************************************************/

static int msWCSParseResolutionString20(char *string,
                                        char *outAxis, size_t axisStringLen, double *outResolution)
{
  char *number = NULL;
  char *check = NULL;

  /* find brackets */
  number = strchr(string, '(');

  if(NULL == number) {
    msSetError(MS_WCSERR, "Invalid resolution parameter value : %s.",
               "msWCSParseSize20()", string);
    return MS_FAILURE;
  }

  /* cut trailing ')' */
  check = strchr(string, ')');
  if(NULL == check) {
    msSetError(MS_WCSERR, "Invalid size parameter value.",
               "msWCSParseSize20()");
    return MS_FAILURE;
  }

  *number = '\0';
  ++number;
  *check = '\0';

  strlcpy(outAxis, string, axisStringLen);

  if(msStringParseDouble(number, outResolution) != MS_SUCCESS) {
    *outResolution = MS_WCS20_UNBOUNDED;
    msSetError(MS_WCSERR, "Invalid resolution parameter value : %s.",
               "msWCSParseSize20()", number);
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSParseScaleString20()                          */
/*                                                                      */
/*      Parses a scale string and returns the axis, the and the value.  */
/*      Subset string: axis ( value )                                   */
/************************************************************************/

static int msWCSParseScaleString20(char *string,
                                  char *outAxis, size_t axisStringLen, double *outScale)
{
  char *number = NULL;
  char *check = NULL;

  /* find brackets */
  number = strchr(string, '(');

  if(NULL == number) {
    msSetError(MS_WCSERR, "Invalid resolution parameter value : %s.",
               "msWCSParseScaleString20()", string);
    return MS_FAILURE;
  }

  /* cut trailing ')' */
  check = strchr(string, ')');
  if(NULL == check || check < number) {
    msSetError(MS_WCSERR, "Invalid scale parameter value.",
               "msWCSParseScaleString20()");
    return MS_FAILURE;
  }

  *number = '\0';
  ++number;
  *check = '\0';

  strlcpy(outAxis, string, axisStringLen);

  if(msStringParseDouble(number, outScale) != MS_SUCCESS || *outScale <= 0.0) {
    *outScale = MS_WCS20_UNBOUNDED;
    msSetError(MS_WCSERR, "Invalid scale parameter value : %s.",
               "msWCSParseScaleString20()", number);
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSParseResolutionString20()                     */
/*                                                                      */
/*      Parses a resolution string and returns the axis, the units of   */
/*      measure and the resolution value.                               */
/*      Subset string: axis ( value )                                   */
/************************************************************************/

static int msWCSParseScaleExtentString20(char *string, char *outAxis,
                                         size_t axisStringLen,
                                         int *outMin, int *outMax)
{
  char *number = NULL;
  char *check = NULL;
  char *colon = NULL;

  /* find brackets */
  number = strchr(string, '(');

  if(NULL == number) {
    msSetError(MS_WCSERR, "Invalid extent parameter value : %s.",
               "msWCSParseScaleExtentString20()", string);
    return MS_FAILURE;
  }

  /* find colon */
  colon = strchr(string, ':');

  if(NULL == colon || colon < number) {
    msSetError(MS_WCSERR, "Invalid extent parameter value : %s.",
               "msWCSParseScaleExtentString20()", string);
    return MS_FAILURE;
  }

  /* cut trailing ')' */
  check = strchr(string, ')');

  if(NULL == check || check < colon) {
    msSetError(MS_WCSERR, "Invalid extent parameter value.",
               "msWCSParseScaleExtentString20()");
    return MS_FAILURE;
  }

  *number = '\0';
  ++number;
  *colon = '\0';
  ++colon;
  *check = '\0';

  strlcpy(outAxis, string, axisStringLen);

  if(msStringParseInteger(number, outMin) != MS_SUCCESS) {
    *outMin = 0;
    msSetError(MS_WCSERR, "Invalid min parameter value : %s.",
               "msWCSParseScaleExtentString20()", number);
    return MS_FAILURE;
  }
  else if(msStringParseInteger(colon, outMax) != MS_SUCCESS) {
    *outMax = 0;
    msSetError(MS_WCSERR, "Invalid resolution parameter value : %s.",
               "msWCSParseScaleExtentString20()", colon);
    return MS_FAILURE;
  }

  if (*outMin > *outMax) {
    msSetError(MS_WCSERR, "Invalid extent: lower part is higher than upper part.",
               "msWCSParseScaleExtentString20()");
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

#if defined(USE_LIBXML2)
/*
  Utility function to get the first child of a node with a given node name
  */

xmlNodePtr msLibXml2GetFirstChild(xmlNodePtr parent, const char *name) {
  xmlNodePtr node;
  if (!parent || !name) {
    return NULL;
  }

  XML_FOREACH_CHILD(parent, node) {
    XML_LOOP_IGNORE_COMMENT_OR_TEXT(node);
    if (EQUAL((char *)node->name, name)) {
      return node;
    }
  }
  return NULL;
}


/*
  Utility function to get the first child of a node with a given node name and
  namespace.
  */

xmlNodePtr msLibXml2GetFirstChildNs(xmlNodePtr parent, const char *name, xmlNsPtr ns) {
  xmlNodePtr node;
  if (!parent || !name || !ns) {
    return NULL;
  }

  XML_FOREACH_CHILD(parent, node) {
    XML_LOOP_IGNORE_COMMENT_OR_TEXT(node);
    if (EQUAL((char *)node->name, name) && ns == node->ns) {
      return node;
    }
  }
  return NULL;
}

/*
  Utility function to get the first child of a node with a given node name
  */

xmlNodePtr msLibXml2GetFirstChildElement(xmlNodePtr parent) {
  xmlNodePtr node;
  if (!parent) {
    return NULL;
  }

  XML_FOREACH_CHILD(parent, node) {
    if (node->type == XML_ELEMENT_NODE) {
      return node;
    }
  }
  return NULL;
}
#endif /* defined(USE_LIBXML2) */

/************************************************************************/
/*                   msWCSParseRequest20_XMLGetCapabilities()           */
/*                                                                      */
/*      Parses a DOM element, representing a GetCapabilities-request    */
/*      to a params object.                                             */
/************************************************************************/
#if defined(USE_LIBXML2)
static int msWCSParseRequest20_XMLGetCapabilities(
  xmlNodePtr root, wcs20ParamsObjPtr params)
{
  xmlNodePtr child;
  char *content = NULL;
  XML_FOREACH_CHILD(root, child) {
    XML_LOOP_IGNORE_COMMENT_OR_TEXT(child)
    else if (EQUAL((char *)child->name, "AcceptVersions")) {
      xmlNodePtr versionNode = NULL;
      XML_FOREACH_CHILD(child, versionNode) {
        XML_LOOP_IGNORE_COMMENT_OR_TEXT(versionNode);
        XML_ASSERT_NODE_NAME(versionNode, "Version");

        content = (char *)xmlNodeGetContent(versionNode);
        params->accept_versions = CSLAddString(params->accept_versions, content);
        xmlFree(content);
      }
    } else if(EQUAL((char *)child->name, "Sections")) {
      xmlNodePtr sectionNode = NULL;
      XML_FOREACH_CHILD(child, sectionNode) {
        XML_LOOP_IGNORE_COMMENT_OR_TEXT(sectionNode)
        XML_ASSERT_NODE_NAME(sectionNode, "Section");

        content = (char *)xmlNodeGetContent(sectionNode);
        params->sections = CSLAddString(params->sections, content);
        xmlFree(content);
      }
    } else if(EQUAL((char *)child->name, "UpdateSequence")) {
      params->updatesequence =
        (char *)xmlNodeGetContent(child);
    } else if(EQUAL((char *)child->name, "AcceptFormats")) {
      /* Maybe not necessary, since only format is xml.   */
      /* At least ignore it, to not generate an error.    */
    } else if(EQUAL((char *)child->name, "AcceptLanguages")) {
      xmlNodePtr languageNode;
      XML_FOREACH_CHILD(child, languageNode) {
        XML_LOOP_IGNORE_COMMENT_OR_TEXT(languageNode)
        XML_ASSERT_NODE_NAME(languageNode, "Language");

        content = (char *)xmlNodeGetContent(languageNode);
        params->accept_languages = CSLAddString(params->accept_languages, content);
        xmlFree(content);
      }
    } else {
      XML_UNKNOWN_NODE_ERROR(child);
    }
  }
  return MS_SUCCESS;
}
#endif

/************************************************************************/
/*                   msWCSParseRequest20_XMLDescribeCoverage()          */
/*                                                                      */
/*      Parses a DOM element, representing a DescribeCoverage-request   */
/*      to a params object.                                             */
/************************************************************************/
#if defined(USE_LIBXML2)
static int msWCSParseRequest20_XMLDescribeCoverage(
  xmlNodePtr root, wcs20ParamsObjPtr params)
{
  xmlNodePtr child;
  int numIds = 0;
  char *id;

  XML_FOREACH_CHILD(root, child) {
    XML_LOOP_IGNORE_COMMENT_OR_TEXT(child)
    XML_ASSERT_NODE_NAME(child, "CoverageID");

    /* Node content is the coverage ID */
    id = (char *)xmlNodeGetContent(child);
    if (id == NULL || strlen(id) == 0) {
      msSetError(MS_WCSERR, "CoverageID could not be parsed.",
                 "msWCSParseRequest20_XMLDescribeCoverage()");
      return MS_FAILURE;
    }
    /* insert coverage ID into the list */
    ++numIds;
    params->ids = CSLAddString(params->ids, (char *)id);
    xmlFree(id);
  }
  return MS_SUCCESS;
}
#endif

/************************************************************************/
/*                   msWCSParseRequest20_XMLGetCoverage()               */
/*                                                                      */
/*      Parses a DOM element, representing a GetCoverage-request to a   */
/*      params object.                                                  */
/************************************************************************/
#if defined(USE_LIBXML2)
static int msWCSParseRequest20_XMLGetCoverage(
  mapObj* map, xmlNodePtr root, wcs20ParamsObjPtr params)
{
  xmlNodePtr child;
  int numIds = 0;
  char *id;

  XML_FOREACH_CHILD(root, child) {
    XML_LOOP_IGNORE_COMMENT_OR_TEXT(child)
    else if (EQUAL((char *)child->name, "CoverageID")) {
      /* Node content is the coverage ID */
      id = (char *)xmlNodeGetContent(child);
      if (id == NULL || strlen(id) == 0) {
        msSetError(MS_WCSERR, "CoverageID could not be parsed.",
                   "msWCSParseRequest20_XMLGetCoverage()");
        return MS_FAILURE;
      }

      /* insert coverage ID into the list */
      ++numIds;
      params->ids = CSLAddString(params->ids, (char *)id);
      xmlFree(id);
    } else if (EQUAL((char *) child->name, "Format")) {
      msFree(params->format);
      params->format = (char *)xmlNodeGetContent(child);
    } else if (EQUAL((char *) child->name, "Mediatype")) {
      char *content = (char *)xmlNodeGetContent(child);
      if(content != NULL && (EQUAL(content, "multipart/mixed")
                          || EQUAL(content, "multipart/related"))) {
        params->multipart = MS_TRUE;
      }
      else {
        msSetError(MS_WCSERR, "Invalid value '%s' for parameter 'Mediatype'.",
                  "msWCSParseRequest20()", content);
        xmlFree(content);
        return MS_FAILURE;
      }
      xmlFree(content);
    } else if (EQUAL((char *) child->name, "DimensionTrim")) {
      wcs20AxisObjPtr axis = NULL;
      wcs20SubsetObjPtr subset = NULL;
      xmlNodePtr node = NULL;
      char *axisName = NULL, *min = NULL, *max = NULL, *crs = NULL;

      /* get strings for axis, min and max */
      XML_FOREACH_CHILD(child, node) {
        XML_LOOP_IGNORE_COMMENT_OR_TEXT(node)
        else if (EQUAL((char *)node->name, "Dimension")) {
          if (axisName != NULL) {
            msSetError(MS_WCSERR,
                       "Parameter 'Dimension' is already set.",
                       "msWCSParseRequest20_XMLGetCoverage()");
            return MS_FAILURE;
          }
          axisName = (char *) xmlNodeGetContent(node);
          crs = (char *) xmlGetProp(node, BAD_CAST "crs");
        } else if (EQUAL((char *)node->name, "trimLow")) {
          min = (char *) xmlNodeGetContent(node);
        } else if (EQUAL((char *)node->name, "trimHigh")) {
          max = (char *) xmlNodeGetContent(node);
        } else {
          msFree(axisName);
          msFree(min);
          msFree(max);
          msFree(crs);
          XML_UNKNOWN_NODE_ERROR(node);
        }
      }
      if(NULL == (subset = msWCSCreateSubsetObj20())) {
        msFree(axisName);
        msFree(min);
        msFree(max);
        msFree(crs);
        return MS_FAILURE;
      }

      /* min and max have to have a value */
      if(min == NULL ) {
        min = msStrdup("*");
      }
      if(max == NULL) {
        max = msStrdup("*");
      }
      if (msWCSParseSubset20(subset, axisName, crs, min, max)
          == MS_FAILURE) {
        msWCSFreeSubsetObj20(subset);
        msWCSException(map, "InvalidSubsetting", "subset", "2.0.1");
        return MS_DONE;
      }

      if(NULL == (axis = msWCSFindAxis20(params, subset->axis))) {
        if(NULL == (axis = msWCSCreateAxisObj20())) {
          msFree(axisName);
          msFree(min);
          msFree(max);
          msFree(crs);
          return MS_FAILURE;
        }
        axis->name = msStrdup(subset->axis);
        msWCSInsertAxisObj20(params, axis);
      }

      axis->subset = subset;

      /* cleanup */
      msFree(axisName);
      msFree(min);
      msFree(max);
      msFree(crs);
    } else if(EQUAL((char *) child->name, "DimensionSlice")) {
      msSetError(MS_WCSERR, "Operation '%s' is not supported by MapServer.",
                 "msWCSParseRequest20_XMLGetCoverage()", (char *)child->name);
      return MS_FAILURE;
    } else if(EQUAL((char *) child->name, "Size")) {
      wcs20AxisObjPtr axis;
      char *axisName;
      char *content;

      if(NULL == (axisName = (char *) xmlGetProp(child, BAD_CAST "dimension")) ) {
        msSetError(MS_WCSERR, "Attribute 'dimension' is missing in element 'Size'.",
                   "msWCSParseRequest20_XMLGetCoverage()");
        return MS_FAILURE;
      }

      if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
        if(NULL == (axis = msWCSCreateAxisObj20())) {
          xmlFree(axisName);
          return MS_FAILURE;
        }
        axis->name = msStrdup(axisName);
        msWCSInsertAxisObj20(params, axis);
      }
      xmlFree(axisName);

      content = (char *)xmlNodeGetContent(child);
      if(msStringParseInteger(content, &(axis->size)) != MS_SUCCESS) {
        xmlFree(content);
        msSetError(MS_WCSERR, "Value of element 'Size' could not "
                   "be parsed to a valid integer.",
                   "msWCSParseRequest20_XMLGetCoverage()");
        return MS_FAILURE;
      }
      xmlFree(content);
    } else if(EQUAL((char *) child->name, "Resolution")) {
      wcs20AxisObjPtr axis;
      char *axisName;
      char *content;

      if(NULL == (axisName = (char *) xmlGetProp(child, BAD_CAST "dimension"))) {
        msSetError(MS_WCSERR, "Attribute 'dimension' is missing "
                   "in element 'Resolution'.",
                   "msWCSParseRequest20_XMLGetCoverage()");
        return MS_FAILURE;
      }

      if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
        if(NULL == (axis = msWCSCreateAxisObj20())) {
          xmlFree(axisName);
          return MS_FAILURE;
        }
        axis->name = msStrdup(axisName);
        msWCSInsertAxisObj20(params, axis);
      }
      xmlFree(axisName);

      axis->resolutionUOM = (char *) xmlGetProp(child, BAD_CAST "uom");

      content = (char *)xmlNodeGetContent(child);
      if(msStringParseDouble(content, &(axis->resolution)) != MS_SUCCESS) {
        msSetError(MS_WCSERR, "Value of element 'Resolution' could not "
                   "be parsed to a valid value.",
                   "msWCSParseRequest20_XMLGetCoverage()");
        xmlFree(content);
        return MS_FAILURE;
      }
      xmlFree(content);
    } else if(EQUAL((char *) child->name, "Interpolation")) {
      /* Deprecated, use wcs:Extension/int:Interpolation/int:globalInterpolation */
      msFree(params->interpolation);
      params->interpolation = (char *) xmlNodeGetContent(child);
    } else if(EQUAL((char *) child->name, "OutputCRS")) {
      params->outputcrs = (char *) xmlNodeGetContent(child);
    } else if(EQUAL((char *) child->name, "rangeSubset")) {
      /* Deprecated, use wcs:Extension/rsub:RangeSubset */
      xmlNodePtr bandNode = NULL;
      XML_FOREACH_CHILD(child, bandNode) {
        char *content = NULL;
        XML_LOOP_IGNORE_COMMENT_OR_TEXT(bandNode);
        XML_ASSERT_NODE_NAME(bandNode, "band");

        content = (char *)xmlNodeGetContent(bandNode);
        params->range_subset =
          CSLAddString(params->range_subset, content);
        xmlFree(content);
      }
    } else if (EQUAL((char *)child->name, "Extension")) {
      xmlNodePtr extensionNode = NULL;
      XML_FOREACH_CHILD(child, extensionNode) {
        XML_LOOP_IGNORE_COMMENT_OR_TEXT(extensionNode);

        if (EQUAL((char *) extensionNode->name, "Scaling")) {
          xmlNodePtr scaleMethodNode = msLibXml2GetFirstChildElement(extensionNode);

          if (EQUAL((char *) scaleMethodNode->name, "ScaleByFactor")) {
            xmlNodePtr scaleFactorNode = msLibXml2GetFirstChildElement(scaleMethodNode);
            char *content;
            if (!scaleFactorNode || !EQUAL((char *)scaleFactorNode->name, "scaleFactor")) {
              msSetError(MS_WCSERR, "Missing 'scaleFactor' node.",
                         "msWCSParseRequest20_XMLGetCoverage()");
              return MS_FAILURE;
            }
            content = (char *)xmlNodeGetContent(scaleFactorNode);
            if (msStringParseDouble(content, &(params->scale)) != MS_SUCCESS
                || params->scale < 0.0) {
              msSetError(MS_WCSERR, "Invalid scaleFactor '%s'.",
                         "msWCSParseRequest20_XMLGetCoverage()", content);
              xmlFree(content);
              return MS_FAILURE;
            }
            xmlFree(content);
          }

          else if (EQUAL((char *) scaleMethodNode->name, "ScaleAxesByFactor")) {
            xmlNodePtr scaleAxisNode, axisNode, scaleFactorNode;
            char *axisName, *content;
            wcs20AxisObjPtr axis;

            XML_FOREACH_CHILD(scaleMethodNode, scaleAxisNode) {
              XML_LOOP_IGNORE_COMMENT_OR_TEXT(scaleAxisNode);

              if (!EQUAL((char *)scaleAxisNode->name, "ScaleAxis")) {
                msSetError(MS_WCSERR, "Invalid ScaleAxesByFactor.",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              /* axis */
              if (NULL == (axisNode = msLibXml2GetFirstChild(scaleAxisNode, "axis"))) {
                msSetError(MS_WCSERR, "Missing axis node",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }
              axisName = (char *)xmlNodeGetContent(axisNode);
              if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
                if(NULL == (axis = msWCSCreateAxisObj20())) {
                  xmlFree(axisName);
                  return MS_FAILURE;
                }
                axis->name = msStrdup(axisName);
                msWCSInsertAxisObj20(params, axis);
              }
              xmlFree(axisName);

              if (axis->scale != MS_WCS20_UNBOUNDED) {
                msSetError(MS_WCSERR, "scaleFactor was already set for axis '%s'.",
                           "msWCSParseRequest20_XMLGetCoverage()", axis->name);
                return MS_FAILURE;
              }

              /* scaleFactor */
              if (NULL == (scaleFactorNode = msLibXml2GetFirstChild(scaleAxisNode, "scaleFactor"))) {
                msSetError(MS_WCSERR, "Missing scaleFactor node",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              content = (char *)xmlNodeGetContent(scaleFactorNode);
              if (msStringParseDouble(content, &(axis->scale)) != MS_SUCCESS
                  || axis->scale < 0.0) {
                msSetError(MS_WCSERR, "Invalid scaleFactor '%s'.",
                           "msWCSParseRequest20_XMLGetCoverage()", content);
                xmlFree(content);
                return MS_FAILURE;
              }
              xmlFree(content);
            }
          }

          else if (EQUAL((char *) scaleMethodNode->name, "ScaleToSize")) {
            xmlNodePtr scaleAxisNode, axisNode, targetSizeNode;
            char *axisName, *content;
            wcs20AxisObjPtr axis;

            XML_FOREACH_CHILD(scaleMethodNode, scaleAxisNode) {
              XML_LOOP_IGNORE_COMMENT_OR_TEXT(scaleAxisNode);

              if (!EQUAL((char *)scaleAxisNode->name, "TargetAxisSize")) {
                msSetError(MS_WCSERR, "Invalid ScaleToSize.",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              /* axis */
              if (NULL == (axisNode = msLibXml2GetFirstChild(scaleAxisNode, "axis"))) {
                msSetError(MS_WCSERR, "Missing axis node",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }
              axisName = (char *)xmlNodeGetContent(axisNode);
              if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
                if(NULL == (axis = msWCSCreateAxisObj20())) {
                  xmlFree(axisName);
                  return MS_FAILURE;
                }
                axis->name = msStrdup(axisName);
                msWCSInsertAxisObj20(params, axis);
              }
              xmlFree(axisName);

              if (axis->size != 0) {
                msSetError(MS_WCSERR, "targetSize was already set for axis '%s'.",
                           "msWCSParseRequest20_XMLGetCoverage()", axis->name);
                return MS_FAILURE;
              }

              /* targetSize */
              if (NULL == (targetSizeNode = msLibXml2GetFirstChild(scaleAxisNode, "targetSize"))) {
                msSetError(MS_WCSERR, "Missing targetSize node",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              content = (char *)xmlNodeGetContent(targetSizeNode);
              if (msStringParseInteger(content, &(axis->size)) != MS_SUCCESS
                  || axis->size <= 0) {
                msSetError(MS_WCSERR, "Invalid targetSize '%s'.",
                           "msWCSParseRequest20_XMLGetCoverage()", content);
                xmlFree(content);
                return MS_FAILURE;
              }
              xmlFree(content);
            }
          }

          else if (EQUAL((char *) scaleMethodNode->name, "ScaleToExtent")) {
            xmlNodePtr scaleAxisNode, axisNode, lowNode, highNode;
            char *axisName, *content;
            wcs20AxisObjPtr axis;
            int low, high;

            XML_FOREACH_CHILD(scaleMethodNode, scaleAxisNode) {
              XML_LOOP_IGNORE_COMMENT_OR_TEXT(scaleAxisNode);

              if (!EQUAL((char *)scaleAxisNode->name, "TargetAxisExtent")) {
                msSetError(MS_WCSERR, "Invalid ScaleToExtent.",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              /* axis */
              if (NULL == (axisNode = msLibXml2GetFirstChild(scaleAxisNode, "axis"))) {
                msSetError(MS_WCSERR, "Missing axis node",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }
              axisName = (char *)xmlNodeGetContent(axisNode);
              if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
                if(NULL == (axis = msWCSCreateAxisObj20())) {
                  xmlFree(axisName);
                  return MS_FAILURE;
                }
                axis->name = msStrdup(axisName);
                msWCSInsertAxisObj20(params, axis);
              }
              xmlFree(axisName);

              if (axis->size != 0) {
                msSetError(MS_WCSERR, "targetSize was already set for axis '%s'.",
                           "msWCSParseRequest20_XMLGetCoverage()", axis->name);
                return MS_FAILURE;
              }

              /* targetSize */
              if (NULL == (lowNode = msLibXml2GetFirstChild(scaleAxisNode, "low"))) {
                msSetError(MS_WCSERR, "Missing low node",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              if (NULL == (highNode = msLibXml2GetFirstChild(scaleAxisNode, "high"))) {
                msSetError(MS_WCSERR, "Missing high node",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              content = (char *)xmlNodeGetContent(lowNode);
              if (msStringParseInteger(content, &low) != MS_SUCCESS) {
                msSetError(MS_WCSERR, "Invalid low value '%s'.",
                           "msWCSParseRequest20_XMLGetCoverage()", content);
                xmlFree(content);
                return MS_FAILURE;
              }
              xmlFree(content);


              content = (char *)xmlNodeGetContent(highNode);
              if (msStringParseInteger(content, &high) != MS_SUCCESS) {
                msSetError(MS_WCSERR, "Invalid high value '%s'.",
                           "msWCSParseRequest20_XMLGetCoverage()", content);
                xmlFree(content);
                return MS_FAILURE;
              }
              xmlFree(content);

              if (high <= low) {
                msSetError(MS_WCSERR, "Invalid extent, high is lower than low.",
                           "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              axis->size = high - low;
            }
          }
        }

        /* Range Subset */
        else if(EQUAL((char *) extensionNode->name, "RangeSubset")) {
          xmlNodePtr rangeItemNode = NULL;

          XML_FOREACH_CHILD(extensionNode, rangeItemNode) {
            xmlNodePtr rangeItemNodeChild = msLibXml2GetFirstChildElement(rangeItemNode);
            XML_LOOP_IGNORE_COMMENT_OR_TEXT(rangeItemNode);

            XML_ASSERT_NODE_NAME(rangeItemNode, "RangeItem");

            if (!rangeItemNodeChild) {
              msSetError(MS_WCSERR, "Missing RangeComponent or RangeInterval.",
                                    "msWCSParseRequest20_XMLGetCoverage()");
              return MS_FAILURE;
            }
            else if (EQUAL((char *) rangeItemNodeChild->name, "RangeComponent")) {
              char *content = (char *)xmlNodeGetContent(rangeItemNodeChild);
              params->range_subset =
                CSLAddString(params->range_subset, content);
              xmlFree(content);
            }
            else if (EQUAL((char *) rangeItemNodeChild->name, "RangeInterval")) {
              xmlNodePtr intervalNode = rangeItemNodeChild;
              xmlNodePtr startComponentNode = msLibXml2GetFirstChild(intervalNode, "startComponent");
              xmlNodePtr endComponentNode = msLibXml2GetFirstChild(intervalNode, "endComponent");
              char *start;
              char *stop;

              if (!startComponentNode || !endComponentNode) {
                msSetError(MS_WCSERR, "Wrong RangeInterval.",
                                      "msWCSParseRequest20_XMLGetCoverage()");
                return MS_FAILURE;
              }

              start = (char *)xmlNodeGetContent(startComponentNode);
              stop = (char *)xmlNodeGetContent(endComponentNode);

              std::string value(start);
              value += ':';
              value += stop;

              xmlFree(start);
              xmlFree(stop);

              params->range_subset =
                CSLAddString(params->range_subset, value.c_str());
            }
          }
        }

        else if (EQUAL((char *) extensionNode->name, "subsettingCrs")) {
          msFree(params->subsetcrs);
          params->subsetcrs = (char *)xmlNodeGetContent(extensionNode);
        }

        else if (EQUAL((char *) extensionNode->name, "outputCrs")) {
          msFree(params->outputcrs);
          params->outputcrs = (char *)xmlNodeGetContent(extensionNode);
        }

        else if (EQUAL((char *) extensionNode->name, "Interpolation")) {
          xmlNodePtr globalInterpolation = msLibXml2GetFirstChild(extensionNode, "globalInterpolation");
          char *content;
          if (globalInterpolation == NULL) {
            msSetError(MS_WCSERR, "Missing 'globalInterpolation' node.",
                                  "msWCSParseRequest20_XMLGetCoverage()");
            return MS_FAILURE;
          }
          content = (char *)xmlNodeGetContent(globalInterpolation);
          msFree(params->interpolation);
          /* TODO: use URIs/URLs once they are specified */
          params->interpolation = msStrdup(content);
          xmlFree(content);
        }

        /* GeoTIFF parameters */
        else if (EQUAL((char *) extensionNode->name, "parameters")
                && extensionNode->ns
                && EQUAL((char *)extensionNode->ns->href, "http://www.opengis.net/gmlcov/geotiff/1.0")) {

          xmlNodePtr parameter;

          XML_FOREACH_CHILD(extensionNode, parameter) {
            char *content;
            XML_LOOP_IGNORE_COMMENT_OR_TEXT(parameter);

            content = (char *)xmlNodeGetContent(parameter);

            params->format_options =
              CSLAddNameValue(params->format_options, (char *)parameter->name, content);
            xmlFree(content);
          }
        }
      }
    } else {
      XML_UNKNOWN_NODE_ERROR(child);
    }
  }
  return MS_SUCCESS;
}
#endif

/************************************************************************/
/*                   msWCSParseRequest20()                              */
/*                                                                      */
/*      Parses a CGI-request to a WCS 20 params object. It is           */
/*      either a POST or a GET request. In case of a POST request       */
/*      the xml content has to be parsed to a DOM structure             */
/*      before the parameters can be extracted.                         */
/************************************************************************/

int msWCSParseRequest20(mapObj *map,
                        cgiRequestObj *request,
                        owsRequestObj *ows_request,
                        wcs20ParamsObjPtr params)
{
  int i;
  if (params == NULL || request == NULL || ows_request == NULL) {
    msSetError(MS_WCSERR, "Internal error.", "msWCSParseRequest20()");
    return MS_FAILURE;
  }

  /* Copy arbitrary service, version and request. */
  params->service = msStrdup(ows_request->service);
  if(ows_request->version != NULL) {
    params->version = msStrdup(ows_request->version);
  }
  params->request = msStrdup(ows_request->request);


  /* Parse the POST request */
  if (request->type == MS_POST_REQUEST) {
#if defined(USE_LIBXML2)
    xmlDocPtr doc = static_cast<xmlDocPtr>(ows_request->document);
    xmlNodePtr root = NULL;
    const char *validate;
    int ret = MS_SUCCESS;

    /* parse to DOM-Structure and get root element */
    if(doc == NULL) {
      xmlErrorPtr error = xmlGetLastError();
      msSetError(MS_WCSERR, "XML parsing error: %s",
                 "msWCSParseRequest20()", error->message);
      return MS_FAILURE;
    }

    root = xmlDocGetRootElement(doc);

    validate = msOWSLookupMetadata(&(map->web.metadata), "CO", "validate_xml");
    if (validate != NULL && EQUAL(validate, "TRUE")) {
      char *schema_dir = msStrdup(msOWSLookupMetadata(&(map->web.metadata),
                                  "CO", "schemas_dir"));
      if (schema_dir != NULL
          && (params->version == NULL ||
              EQUALN(params->version, "2.0", 3))) {
        schema_dir = msStringConcatenate(schema_dir,
                                         "wcs/2.0.0/wcsAll.xsd");
        if (msOWSSchemaValidation(schema_dir, request->postrequest) != 0) {
          msSetError(MS_WCSERR, "Invalid POST request. "
                     "XML is not valid",
                     "msWCSParseRequest20()");
          return MS_FAILURE;
        }
      }
      msFree(schema_dir);
    }

    if(EQUAL(params->request, "GetCapabilities")) {
      ret = msWCSParseRequest20_XMLGetCapabilities(root, params);
    } else if(params->version != NULL && EQUALN(params->version, "2.0", 3)) {
      if(EQUAL(params->request, "DescribeCoverage")) {
        ret = msWCSParseRequest20_XMLDescribeCoverage(root, params);
      } else if(EQUAL(params->request, "GetCoverage")) {
        ret = msWCSParseRequest20_XMLGetCoverage(map, root, params);
      }
    }
    return ret;

#else /* defined(USE_LIBXML2) */
    /* TODO: maybe with CPLXML? */
    return MS_FAILURE;
#endif /* defined(USE_LIBXML2) */
  }

  /* Parse the KVP GET request */
  for (i = 0; i < request->NumParams; ++i) {
    char *key = NULL, *value = NULL;
    char **tokens;
    int num, j;
    key = request->ParamNames[i];
    value = request->ParamValues[i];

    if (EQUAL(key, "VERSION")) {
      continue;
    } else if (EQUAL(key, "REQUEST")) {
      continue;
    } else if (EQUAL(key, "SERVICE")) {
      continue;
    } else if (EQUAL(key, "ACCEPTVERSIONS")) {
      tokens = msStringSplit(value, ',', &num);
      for(j = 0; j < num; ++j) {
        params->accept_versions =
          CSLAddString(params->accept_versions, tokens[j]);
      }
      msFreeCharArray(tokens, num);
    } else if (EQUAL(key, "SECTIONS")) {
      tokens = msStringSplit(value, ',', &num);
      for(j = 0; j < num; ++j) {
        params->sections =
          CSLAddString(params->sections, tokens[j]);
      }
      msFreeCharArray(tokens, num);
    } else if (EQUAL(key, "UPDATESEQUENCE")) {
      msFree(params->updatesequence);
      params->updatesequence = msStrdup(value);
    } else if (EQUAL(key, "ACCEPTFORMATS")) {
      /* ignore */
    } else if (EQUAL(key, "ACCEPTLANGUAGES")) {
      if (params->accept_languages != NULL) {
        CSLDestroy(params->accept_languages);
      }
      params->accept_languages = CSLTokenizeString2(value, ",", 0);
    } else if (EQUAL(key, "COVERAGEID")) {
      if (params->ids != NULL) {
        msSetError(MS_WCSERR, "Parameter 'CoverageID' is already set. "
                   "For multiple IDs use a comma separated list.",
                   "msWCSParseRequest20()");
        return MS_FAILURE;
      }
      params->ids = CSLTokenizeString2(value, ",",0);
    } else if (EQUAL(key, "FORMAT")) {
      msFree(params->format);
      params->format = msStrdup(value);
    } else if (EQUAL(key, "MEDIATYPE")) {
      if(EQUAL(value, "multipart/mixed") || EQUAL(value, "multipart/related")) {
        params->multipart = MS_TRUE;
      }
      else {
         msSetError(MS_WCSERR, "Invalid value '%s' for parameter 'Mediatype'.",
                    "msWCSParseRequest20()", value);
         return MS_FAILURE;
      }
    } else if (EQUAL(key, "INTERPOLATION")) {
      msFree(params->interpolation);
      params->interpolation = msStrdup(value);
    } else if (EQUAL(key, "OUTPUTCRS")) {
      msFree(params->outputcrs);
      params->outputcrs = msStrdup(value);
    } else if (EQUAL(key, "SUBSETTINGCRS")) {
      msFree(params->subsetcrs);
      params->subsetcrs = msStrdup(value);
    } else if (EQUAL(key, "SCALEFACTOR")) {
      double scale = MS_WCS20_UNBOUNDED;
      if (params->scale != MS_WCS20_UNBOUNDED) {
        msSetError(MS_WCSERR, "Parameter 'SCALEFACTOR' already set.",
                    "msWCSParseRequest20()");
        return MS_FAILURE;
      } else if (msStringParseDouble(value, &scale) != MS_SUCCESS) {
        msSetError(MS_WCSERR, "Could not parse parameter 'SCALEFACTOR'.",
                   "msWCSParseRequest20()");
        return MS_FAILURE;
      } else if (scale <= 0.0) {
        msSetError(MS_WCSERR, "Invalid value for 'SCALEFACTOR'.",
                   "msWCSParseRequest20()");
        return MS_FAILURE;
      }
      params->scale = scale;
    } else if (EQUAL(key, "SCALEAXES")) {
      wcs20AxisObjPtr axis = NULL;
      tokens = msStringSplit(value, ',', &num);
      for(j = 0; j < num; ++j) {
        char axisName[500];
        double scale;

        if (msWCSParseScaleString20(tokens[j], axisName, sizeof(axisName), &scale) != MS_SUCCESS) {
          msFreeCharArray(tokens, num);
          return MS_FAILURE;
        }

        if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
          if(NULL == (axis = msWCSCreateAxisObj20())) {
            msFreeCharArray(tokens, num);
            return MS_FAILURE;
          }
          axis->name = msStrdup(axisName);
          msWCSInsertAxisObj20(params, axis);
        }

        /* check if the size of the axis is already set */
        if(axis->scale != MS_WCS20_UNBOUNDED) {
          msFreeCharArray(tokens, num);
          msSetError(MS_WCSERR, "The scale of the axis is already set.",
                     "msWCSParseRequest20()");
          return MS_FAILURE;
        }
        axis->scale = scale;
      }
      msFreeCharArray(tokens, num);
    } else if (EQUAL(key, "SCALESIZE")) {
      wcs20AxisObjPtr axis = NULL;
      tokens = msStringSplit(value, ',', &num);
      for(j = 0; j < num; ++j) {
        char axisName[500];
        int size;

        if (msWCSParseSizeString20(tokens[j], axisName, sizeof(axisName), &size) != MS_SUCCESS) {
          msFreeCharArray(tokens, num);
          return MS_FAILURE;
        }

        if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
          if(NULL == (axis = msWCSCreateAxisObj20())) {
            msFreeCharArray(tokens, num);
            return MS_FAILURE;
          }
          axis->name = msStrdup(axisName);
          msWCSInsertAxisObj20(params, axis);
        }

        /* check if the size of the axis is already set */
        if(axis->size != 0) {
          msFreeCharArray(tokens, num);
          msSetError(MS_WCSERR, "The size of the axis is already set.",
                     "msWCSParseRequest20()");
          return MS_FAILURE;
        }
        axis->size = size;
      }
      msFreeCharArray(tokens, num);
    } else if (EQUAL(key, "SCALEEXTENT")) {
      wcs20AxisObjPtr axis = NULL;
      /* No real support for scaleextent, we just interprete it as SCALESIZE */
      tokens = msStringSplit(value, ',', &num);
      for(j = 0; j < num; ++j) {
        char axisName[500];
        int min, max;

        if (msWCSParseScaleExtentString20(tokens[j], axisName, sizeof(axisName),
                                          &min, &max) != MS_SUCCESS) {
          msFreeCharArray(tokens, num);
          return MS_FAILURE;
        }

        if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
          if(NULL == (axis = msWCSCreateAxisObj20())) {
            msFreeCharArray(tokens, num);
            return MS_FAILURE;
          }
          axis->name = msStrdup(axisName);
          msWCSInsertAxisObj20(params, axis);
        }

        /* check if the size of the axis is already set */
        if(axis->size != 0) {
          msFreeCharArray(tokens, num);
          msSetError(MS_WCSERR, "The size of the axis is already set.",
                     "msWCSParseRequest20()");
          return MS_FAILURE;
        }
        axis->size = max - min;
      }
      msFreeCharArray(tokens, num);
      /* We explicitly don't test for strict equality as the parameter name is supposed to be unique */
    } else if (EQUALN(key, "SIZE", 4)) {
      /* Deprecated scaling */
      wcs20AxisObjPtr axis = NULL;
      char axisName[500];
      int size = 0;

      if(msWCSParseSizeString20(value, axisName, sizeof(axisName), &size) == MS_FAILURE) {
        return MS_FAILURE;
      }

      if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
        if(NULL == (axis = msWCSCreateAxisObj20())) {
          return MS_FAILURE;
        }
        axis->name = msStrdup(axisName);
        msWCSInsertAxisObj20(params, axis);
      }

      /* check if the size of the axis is already set */
      if(axis->size != 0) {
        msSetError(MS_WCSERR, "The size of the axis is already set.",
                   "msWCSParseRequest20()");
        return MS_FAILURE;
      }
      axis->size = size;
    /* We explicitly don't test for strict equality as the parameter name is supposed to be unique */
    } else if (EQUALN(key, "RESOLUTION", 10)) {
      wcs20AxisObjPtr axis = NULL;
      char axisName[500];
      double resolution = 0;

      if(msWCSParseResolutionString20(value, axisName, sizeof(axisName), &resolution) == MS_FAILURE) {
        return MS_FAILURE;
      }

      /* check if axis object already exists, otherwise create a new one */
      if(NULL == (axis = msWCSFindAxis20(params, axisName))) {
        if(NULL == (axis = msWCSCreateAxisObj20())) {
          return MS_FAILURE;
        }
        axis->name = msStrdup(axisName);
        msWCSInsertAxisObj20(params, axis);
      }

      /* check if the resolution of the axis is already set */
      if(axis->resolution != MS_WCS20_UNBOUNDED) {
        msSetError(MS_WCSERR, "The resolution of the axis is already set.",
                   "msWCSParseRequest20()");
        return MS_FAILURE;
      }
      axis->resolution = resolution;
    /* We explicitly don't test for strict equality as the parameter name is supposed to be unique */
    } else if (EQUALN(key, "SUBSET", 6)) {
      wcs20AxisObjPtr axis = NULL;
      wcs20SubsetObjPtr subset = msWCSCreateSubsetObj20();
      if(NULL == subset) {
        return MS_FAILURE;
      }
      if (msWCSParseSubsetKVPString20(subset, value) == MS_FAILURE) {
        msWCSFreeSubsetObj20(subset);
        msWCSException(map, "InvalidSubsetting", "subset", ows_request->version);
        return MS_DONE;
      }

      if(NULL == (axis = msWCSFindAxis20(params, subset->axis))) {
        if(NULL == (axis = msWCSCreateAxisObj20())) {
          return MS_FAILURE;
        }
        axis->name = msStrdup(subset->axis);
        msWCSInsertAxisObj20(params, axis);
      }

      if(NULL != axis->subset) {
        msSetError(MS_WCSERR, "The axis '%s' is already subsetted.",
                   "msWCSParseRequest20()", axis->name);
        msWCSFreeSubsetObj20(subset);
        msWCSException(map, "InvalidAxisLabel", "subset", ows_request->version);
        return MS_DONE;
      }
      axis->subset = subset;
    } else if(EQUAL(key, "RANGESUBSET")) {
      tokens = msStringSplit(value, ',', &num);
      for(j = 0; j < num; ++j) {
        params->range_subset =
          CSLAddString(params->range_subset, tokens[j]);
      }
      msFreeCharArray(tokens, num);
    } else if (EQUALN(key, "GEOTIFF:", 8)) {
      params->format_options =
        CSLAddNameValue(params->format_options, key, value);
    }
    /* Ignore all other parameters here */
  }

  return MS_SUCCESS;
}

#if defined(USE_LIBXML2)

/************************************************************************/
/*                   msWCSValidateAndFindSubsets20()                    */
/*                                                                      */
/*      Iterates over every axis in the parameters and checks if the    */
/*      axis name is in any string list. If found, but there already is */
/*      a sample found for this axis, an Error is returned. Also if no  */
/*      axis is found for a given axis, an error is returned.           */
/************************************************************************/
static int msWCSValidateAndFindAxes20(
  wcs20ParamsObjPtr params,
  wcs20AxisObjPtr outAxes[])
{
  static const int numAxis = 2;
  const char * const validXAxisNames[] = {"x", "xaxis", "x-axis", "x_axis", "long", "long_axis", "long-axis", "lon", "lon_axis", "lon-axis", NULL};
  const char * const validYAxisNames[] = {"y", "yaxis", "y-axis", "y_axis", "lat", "lat_axis", "lat-axis", NULL};
  const char * const * const validAxisNames[2] = { validXAxisNames, validYAxisNames };
  int iParamAxis, iAcceptedAxis, iName, i;

  for(i = 0; i < numAxis; ++i) {
    outAxes[i] = NULL;
  }

  /* iterate over all subsets */
  for(iParamAxis = 0; iParamAxis < params->numaxes; ++iParamAxis) {
    int found = 0;

    /* iterate over all given axes */
    for(iAcceptedAxis = 0; iAcceptedAxis < numAxis; ++iAcceptedAxis ) {
      /* iterate over all possible names for the current axis */
      for(iName = 0; validAxisNames[iAcceptedAxis][iName] != NULL ; ++iName) {
        /* compare axis name with current possible name */
        if(EQUAL(params->axes[iParamAxis]->name, validAxisNames[iAcceptedAxis][iName])) {
          /* if there is already a sample for the axis, throw error */
          if(outAxes[iAcceptedAxis] != NULL) {
            msSetError(MS_WCSERR, "The axis with the name '%s' corresponds "
                       "to the same axis as the subset with the name '%s'.",
                       "msWCSValidateAndFindAxes20()",
                       outAxes[iAcceptedAxis]->name, params->axes[iParamAxis]->name);
            return MS_FAILURE;
          }

          /* if match is found, save it */
          outAxes[iAcceptedAxis] = params->axes[iParamAxis];
          found = 1;
          break;
        }
      }
      if (found) {
        break;
      }
    }

    /* no valid representation for current subset found */
    /* exit and throw error                             */
    if(found == 0) {
      msSetError(MS_WCSERR, "Invalid subset axis '%s'.",
                 "msWCSValidateAndFindAxes20()", params->axes[iParamAxis]->name);
      return MS_FAILURE;
    }
  }
  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSPrepareNamespaces20()                         */
/*                                                                      */
/*      Inserts namespace definitions into the root node of a DOM       */
/*      structure.                                                      */
/************************************************************************/

static void msWCSPrepareNamespaces20(xmlDocPtr pDoc, xmlNodePtr psRootNode, mapObj* map, int addInspire)
{
  xmlNsPtr psXsiNs;
  char *schemaLocation = NULL;
  char *xsi_schemaLocation = NULL;

  xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WCS_20_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX));

  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_20_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI,           BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX );
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,       BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI,     BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WCS_20_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_GML_32_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_GMLCOV_10_NAMESPACE_URI,     BAD_CAST MS_OWSCOMMON_GMLCOV_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_SWE_20_NAMESPACE_URI,        BAD_CAST MS_OWSCOMMON_SWE_NAMESPACE_PREFIX);

  if (addInspire) {
    xmlNewNs(psRootNode, BAD_CAST MS_INSPIRE_COMMON_NAMESPACE_URI, BAD_CAST MS_INSPIRE_COMMON_NAMESPACE_PREFIX);
    xmlNewNs(psRootNode, BAD_CAST MS_INSPIRE_DLS_NAMESPACE_URI, BAD_CAST MS_INSPIRE_DLS_NAMESPACE_PREFIX);
  }

  psXsiNs = xmlSearchNs(pDoc, psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

  schemaLocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );
  xsi_schemaLocation = msStrdup(MS_OWSCOMMON_WCS_20_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemaLocation);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, MS_OWSCOMMON_WCS_20_SCHEMAS_LOCATION);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");

  if (addInspire) {
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, MS_INSPIRE_DLS_NAMESPACE_URI " ");
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, msOWSGetInspireSchemasLocation(map));
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, MS_INSPIRE_DLS_SCHEMA_LOCATION);
  }

  xmlNewNsProp(psRootNode, psXsiNs, BAD_CAST "schemaLocation", BAD_CAST xsi_schemaLocation);

  msFree(schemaLocation);
  msFree(xsi_schemaLocation);
}

/************************************************************************/
/*                   msWCSGetFormatList20()                             */
/*                                                                      */
/*      Copied from mapwcs.c.                                           */
/************************************************************************/

static char *msWCSGetFormatsList20( mapObj *map, layerObj *layer )
{
  char *format_list = msStrdup("");
  char **tokens = NULL, **formats = NULL;
  int  i, numtokens = 0, numformats;
  char *value;

  /* -------------------------------------------------------------------- */
  /*      Parse from layer metadata.                                      */
  /* -------------------------------------------------------------------- */
  if( layer != NULL
      && (value = msOWSGetEncodeMetadata( &(layer->metadata),"CO","formats",
                                          NULL )) != NULL ) {
    tokens = msStringSplit(value, ' ', &numtokens);
    msFree(value);
  }

  /* -------------------------------------------------------------------- */
  /*      Parse from map.web metadata.                                    */
  /* -------------------------------------------------------------------- */
  else if((value = msOWSGetEncodeMetadata( &(map->web.metadata), "CO", "formats",
                                           NULL)) != NULL ) {
    tokens = msStringSplit(value, ' ', &numtokens);
    msFree(value);
  }

  /* -------------------------------------------------------------------- */
  /*      Or generate from all configured raster output formats that      */
  /*      look plausible.                                                 */
  /* -------------------------------------------------------------------- */
  else {
    tokens = (char **) msSmallCalloc(map->numoutputformats,sizeof(char*));
    for( i = 0; i < map->numoutputformats; i++ ) {
      switch( map->outputformatlist[i]->renderer ) {
          /* seemingly normal raster format */
        case MS_RENDER_WITH_AGG:
        case MS_RENDER_WITH_RAWDATA:
          tokens[numtokens++] = msStrdup(map->outputformatlist[i]->name);
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
  formats = (char **) msSmallCalloc(sizeof(char*),numtokens);

  for( i = 0; i < numtokens; i++ ) {
    int format_i, j;
    const char *mimetype;

    for( format_i = 0; format_i < map->numoutputformats; format_i++ ) {
      if( EQUAL(map->outputformatlist[format_i]->name, tokens[i]) )
        break;
    }

    if( format_i == map->numoutputformats ) {
      msDebug("Failed to find outputformat info on format '%s', ignore.\n",
              tokens[i] );
      continue;
    }

    mimetype = map->outputformatlist[format_i]->mimetype;
    if( mimetype == NULL || strlen(mimetype) == 0 ) {
      msDebug("No mimetime for format '%s', ignoring.\n",
              tokens[i] );
      continue;
    }

    for( j = 0; j < numformats; j++ ) {
      if( EQUAL(mimetype,formats[j]) )
        break;
    }

    if( j < numformats ) {
      msDebug( "Format '%s' ignored since mimetype '%s' duplicates another outputFormatObj.\n",
               tokens[i], mimetype );
      continue;
    }

    formats[numformats++] = msStrdup(mimetype);
  }

  msFreeCharArray(tokens,numtokens);

  /* -------------------------------------------------------------------- */
  /*      Turn mimetype list into comma delimited form for easy use       */
  /*      with xml functions.                                             */
  /* -------------------------------------------------------------------- */
  for(i = 0; i < numformats; i++) {
    if(i > 0) {
      format_list = msStringConcatenate(format_list, (char *) ",");
    }
    format_list = msStringConcatenate(format_list, formats[i]);
  }
  msFreeCharArray(formats,numformats);

  return format_list;
}

/************************************************************************/
/*                   msWCSSwapAxes20                                    */
/*                                                                      */
/*      Helper function to determine if a SRS mandates swapped axes.    */
/************************************************************************/

static int msWCSSwapAxes20(char *srs_uri)
{
  int srid = 0;

  /* get SRID from the srs uri */
  if(srs_uri != NULL && strlen(srs_uri) > 0) {
    if(sscanf(srs_uri, "http://www.opengis.net/def/crs/EPSG/0/%d", &srid) != EOF)
      ;
    else if(sscanf(srs_uri, "http://www.opengis.net/def/crs/%d", &srid) != EOF)
      ;
    else
      srid = 0;
  }
  if (srid == 0)
    return MS_FALSE;

  return msIsAxisInverted(srid);
}

/************************************************************************/
/*                   msWCSCommon20_CreateBoundedBy()                    */
/*                                                                      */
/*      Inserts the BoundedBy section into an existing DOM structure.   */
/************************************************************************/

static void msWCSCommon20_CreateBoundedBy(wcs20coverageMetadataObjPtr cm,
    xmlNsPtr psGmlNs, xmlNodePtr psRoot, projectionObj *projection, int swapAxes)
{
  xmlNodePtr psBoundedBy, psEnvelope;
  char lowerCorner[100], upperCorner[100], axisLabels[100], uomLabels[100];

  psBoundedBy = xmlNewChild( psRoot, psGmlNs, BAD_CAST "boundedBy", NULL);
  {
    psEnvelope = xmlNewChild(psBoundedBy, psGmlNs, BAD_CAST "Envelope", NULL);
    {
      xmlNewProp(psEnvelope, BAD_CAST "srsName", BAD_CAST cm->srs_uri);

      if(projection->proj != NULL && msProjIsGeographicCRS(projection)) {
        if (swapAxes == MS_FALSE) {
          strlcpy(axisLabels, "long lat", sizeof(axisLabels));
        } else {
          strlcpy(axisLabels, "lat long", sizeof(axisLabels));
        }
        strlcpy(uomLabels, "deg deg", sizeof(uomLabels));
      } else {
        if (swapAxes == MS_FALSE) {
          strlcpy(axisLabels, "x y", sizeof(axisLabels));
        } else {
          strlcpy(axisLabels, "y x", sizeof(axisLabels));
        }
        strlcpy(uomLabels, "m m", sizeof(uomLabels));
      }
      xmlNewProp(psEnvelope, BAD_CAST "axisLabels", BAD_CAST axisLabels);
      xmlNewProp(psEnvelope, BAD_CAST "uomLabels", BAD_CAST uomLabels);
      xmlNewProp(psEnvelope, BAD_CAST "srsDimension", BAD_CAST "2");

      if (swapAxes == MS_FALSE) {
        snprintf(lowerCorner, sizeof(lowerCorner), "%.15g %.15g", cm->extent.minx, cm->extent.miny);
        snprintf(upperCorner, sizeof(upperCorner), "%.15g %.15g", cm->extent.maxx, cm->extent.maxy);
      } else {
        snprintf(lowerCorner, sizeof(lowerCorner), "%.15g %.15g", cm->extent.miny, cm->extent.minx);
        snprintf(upperCorner, sizeof(upperCorner), "%.15g %.15g", cm->extent.maxy, cm->extent.maxx);
      }

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

static void msWCSCommon20_CreateDomainSet(layerObj* layer, wcs20coverageMetadataObjPtr cm,
    xmlNsPtr psGmlNs, xmlNodePtr psRoot, projectionObj *projection, int swapAxes)
{
  xmlNodePtr psDomainSet, psGrid, psLimits, psGridEnvelope, psOrigin,
             psOffsetX, psOffsetY;
  char low[100], high[100], id[100], point[100];
  char offsetVector1[100], offsetVector2[100], axisLabels[100];

  psDomainSet = xmlNewChild( psRoot, psGmlNs, BAD_CAST "domainSet", NULL);
  {
    psGrid = xmlNewChild(psDomainSet, psGmlNs, BAD_CAST "RectifiedGrid", NULL);
    {
      double x0 = cm->geotransform[0]+cm->geotransform[1]/2+cm->geotransform[2]/2;
      double y0 = cm->geotransform[3]+cm->geotransform[4]/2+cm->geotransform[5]/2;
      double resx = cm->geotransform[1];
      double resy = cm->geotransform[5];

      xmlNewProp(psGrid, BAD_CAST "dimension", BAD_CAST "2");
      snprintf(id, sizeof(id), "grid_%s", layer->name);
      xmlNewNsProp(psGrid, psGmlNs, BAD_CAST "id", BAD_CAST id);

      psLimits = xmlNewChild(psGrid, psGmlNs, BAD_CAST "limits", NULL);
      {
        psGridEnvelope = xmlNewChild(psLimits, psGmlNs, BAD_CAST "GridEnvelope", NULL);
        {
          strlcpy(low, "0 0", sizeof(low));
          snprintf(high, sizeof(high), "%d %d", cm->xsize - 1, cm->ysize - 1);

          xmlNewChild(psGridEnvelope, psGmlNs, BAD_CAST "low", BAD_CAST low);
          xmlNewChild(psGridEnvelope, psGmlNs, BAD_CAST "high", BAD_CAST high);
        }
      }

      if(projection->proj != NULL && msProjIsGeographicCRS(projection)) {
        strlcpy(axisLabels, "long lat", sizeof(axisLabels));
      } else {
        strlcpy(axisLabels, "x y", sizeof(axisLabels));
      }

      xmlNewChild(psGrid, psGmlNs, BAD_CAST "axisLabels", BAD_CAST axisLabels);

      psOrigin = xmlNewChild(psGrid, psGmlNs, BAD_CAST "origin", NULL);
      {
        if (swapAxes == MS_FALSE) {
          snprintf(point, sizeof(point), "%f %f", x0, y0);
        } else {
          snprintf(point, sizeof(point), "%f %f", y0, x0);
        }
        psOrigin = xmlNewChild(psOrigin, psGmlNs, BAD_CAST "Point", NULL);
        snprintf(id, sizeof(id), "grid_origin_%s", layer->name);
        xmlNewNsProp(psOrigin, psGmlNs, BAD_CAST "id", BAD_CAST id);
        xmlNewProp(psOrigin, BAD_CAST "srsName", BAD_CAST cm->srs_uri);

        xmlNewChild(psOrigin, psGmlNs, BAD_CAST "pos", BAD_CAST point);
      }

      if (swapAxes == MS_FALSE) {
        snprintf(offsetVector1, sizeof(offsetVector1), "%f 0", resx);
        snprintf(offsetVector2, sizeof(offsetVector2), "0 %f", resy);
      } else {
        snprintf(offsetVector1, sizeof(offsetVector1), "0 %f", resx);
        snprintf(offsetVector2, sizeof(offsetVector2), "%f 0", resy);
      }
      psOffsetX = xmlNewChild(psGrid, psGmlNs, BAD_CAST "offsetVector", BAD_CAST offsetVector1);
      psOffsetY = xmlNewChild(psGrid, psGmlNs, BAD_CAST "offsetVector", BAD_CAST offsetVector2);

      xmlNewProp(psOffsetX, BAD_CAST "srsName", BAD_CAST cm->srs_uri);
      xmlNewProp(psOffsetY, BAD_CAST "srsName", BAD_CAST cm->srs_uri);
    }
  }
}

/************************************************************************/
/*                   msWCSCommon20_CreateRangeType()                    */
/*                                                                      */
/*      Inserts the RangeType section into an existing DOM structure.   */
/************************************************************************/

static void msWCSCommon20_CreateRangeType(wcs20coverageMetadataObjPtr cm, char *bands,
    xmlNsPtr psGmlcovNs, xmlNsPtr psSweNs, xmlNodePtr psRoot)
{
  xmlNodePtr psRangeType, psDataRecord, psField, psQuantity,
             psUom, psConstraint, psAllowedValues = NULL, psNilValues = NULL;
  char **arr = NULL;
  int num = 0;

  if(NULL != bands) {
    arr = msStringSplit(bands, ',', &num);
  }

  psRangeType = xmlNewChild( psRoot, psGmlcovNs, BAD_CAST "rangeType", NULL);
  psDataRecord  = xmlNewChild(psRangeType, psSweNs, BAD_CAST "DataRecord", NULL);

  /* iterate over every band */
  for(unsigned i = 0; i < cm->numbands; ++i) {
    /* only add bands that are in the range subset */
    if (NULL != arr && num > 0) {
      int found = MS_FALSE, j;
      for(j = 0; j < num; ++j) {
        int repr = 0;
        if(msStringParseInteger(arr[j], &repr) == MS_SUCCESS &&
           static_cast<unsigned>(repr) == i + 1) {
          found = MS_TRUE;
          break;
        }
      }
      if(found == MS_FALSE) {
        /* ignore this band since it is not in the range subset */
        continue;
      }
    }

    /* add field tag */
    psField = xmlNewChild(psDataRecord, psSweNs, BAD_CAST "field", NULL);

    if(cm->bands[i].name != NULL) {
      xmlNewProp(psField, BAD_CAST "name", BAD_CAST cm->bands[i].name);
    } else {
      xmlNewProp(psField, BAD_CAST "name", BAD_CAST "band");
    }
    /* add Quantity tag */
    psQuantity = xmlNewChild(psField, psSweNs, BAD_CAST "Quantity", NULL);
    if(cm->bands[i].definition != NULL) {
      xmlNewProp(psQuantity, BAD_CAST "definition", BAD_CAST cm->bands[i].definition);
    }
    if(cm->bands[i].description != NULL) {
      xmlNewChild(psQuantity, psSweNs, BAD_CAST "description", BAD_CAST cm->bands[i].description);
    }

    /* if there are given nilvalues -> add them to the first field */
    /* all other fields get a reference to these */
    if(cm->numnilvalues > 0) {
      psNilValues = xmlNewChild(
                      xmlNewChild(psQuantity, psSweNs, BAD_CAST "nilValues", NULL),
                      psSweNs, BAD_CAST "NilValues", NULL);
      for(unsigned j = 0; j < cm->numnilvalues; ++j) {
        xmlNodePtr psTemp =
          xmlNewChild(psNilValues, psSweNs, BAD_CAST "nilValue", BAD_CAST cm->nilvalues[j]);
        if(j < cm->numnilvalues)
          xmlNewProp(psTemp, BAD_CAST "reason", BAD_CAST cm->nilvalues_reasons[j]);
      }
    } else { /* create an empty nilValues tag */
      xmlNewChild(psQuantity, psSweNs, BAD_CAST "nilValues", NULL);
    }

    psUom = xmlNewChild(psQuantity, psSweNs, BAD_CAST "uom", NULL);
    if(cm->bands[i].uom != NULL) {
      xmlNewProp(psUom, BAD_CAST "code", BAD_CAST cm->bands[i].uom);
    } else {
      xmlNewProp(psUom, BAD_CAST "code", BAD_CAST "W.m-2.Sr-1");
    }

    /* add constraint */
    psConstraint = xmlNewChild(psQuantity, psSweNs, BAD_CAST "constraint", NULL);

    {
      char interval[100], significant_figures[100];
      psAllowedValues = xmlNewChild(psConstraint, psSweNs, BAD_CAST "AllowedValues", NULL);

      /* Interval */
      snprintf(interval, sizeof(interval), "%.5g %.5g", cm->bands[i].interval_min, cm->bands[i].interval_max);
      xmlNewChild(psAllowedValues, psSweNs, BAD_CAST "interval", BAD_CAST interval);

      /* Significant figures */
      snprintf(significant_figures, sizeof(significant_figures), "%d", cm->bands[i].significant_figures);
      xmlNewChild(psAllowedValues, psSweNs, BAD_CAST "significantFigures", BAD_CAST significant_figures);
    }
  }
  msFreeCharArray(arr,num);
}

/************************************************************************/
/*                   msWCSWriteDocument20()                             */
/*                                                                      */
/*      Writes out an xml structure to the stream.                      */
/************************************************************************/

static int msWCSWriteDocument20(xmlDocPtr psDoc)
{
  xmlChar *buffer = NULL;
  int size = 0;
  msIOContext *context = NULL;
  char *contenttype = NULL;

  if( msIO_needBinaryStdout() == MS_FAILURE ) {
    return MS_FAILURE;
  }

  if( EQUAL((char *)xmlDocGetRootElement(psDoc)->name, "RectifiedGridCoverage") )
    contenttype = msStrdup("application/gml+xml");
  else
    contenttype = msStrdup("text/xml");

  msIO_setHeader("Content-Type","%s; charset=UTF-8", contenttype);
  msIO_sendHeaders();
  msFree(contenttype);

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, "UTF-8", 1);
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

static int msWCSWriteFile20(mapObj* map, imageObj* image, wcs20ParamsObjPtr params, int multipart)
{
  int status;
  char* filename = NULL;
  char *base_dir = NULL;
  const char *fo_filename;
  int i;

  fo_filename = msGetOutputFormatOption( image->format, "FILENAME", NULL );

  /* -------------------------------------------------------------------- */
  /*      Fetch the driver we will be using and check if it supports      */
  /*      VSIL IO.                                                        */
  /* -------------------------------------------------------------------- */
  if( EQUALN(image->format->driver,"GDAL/",5) ) {
    GDALDriverH hDriver;
    const char *pszExtension = image->format->extension;

    msAcquireLock( TLOCK_GDAL );
    hDriver = GDALGetDriverByName( image->format->driver+5 );
    if( hDriver == NULL ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_MISCERR,
                  "Failed to find %s driver.",
                  "msWCSWriteFile20()",
                  image->format->driver+5 );
      return msWCSException(map, "NoApplicableCode", "mapserv",
                            params->version);
    }

    if( pszExtension == NULL )
      pszExtension = "img.tmp";

    if( msGDALDriverSupportsVirtualIOOutput(hDriver) ) {
      base_dir = msTmpFile(map, map->mappath, "/vsimem/wcsout", NULL);
      if( fo_filename )
        filename = msStrdup(CPLFormFilename(base_dir,
                                            fo_filename,NULL));
      else
        filename = msStrdup(CPLFormFilename(base_dir,
                                            "out", pszExtension ));

      /*            CleanVSIDir( "/vsimem/wcsout" ); */

      msReleaseLock( TLOCK_GDAL );
      status = msSaveImage(map, image, filename);
      if( status != MS_SUCCESS ) {
        msSetError(MS_MISCERR, "msSaveImage() failed",
                   "msWCSWriteFile20()");
        return msWCSException20(map, "NoApplicableCode", "mapserv",
                                params->version);
      }
    }
    msReleaseLock( TLOCK_GDAL );
  }

  /* -------------------------------------------------------------------- */
  /*      If we weren't able to write data under /vsimem, then we just    */
  /*      output a single "stock" filename.                               */
  /* -------------------------------------------------------------------- */
  if( filename == NULL ) {
    msOutputFormatResolveFromImage( map, image );
    if(multipart) {
      msIO_fprintf( stdout, "\r\n--wcs\r\n" );
      msIO_fprintf(
        stdout,
        "Content-Type: %s\r\n"
        "Content-Description: coverage data\r\n"
        "Content-Transfer-Encoding: binary\r\n",
        MS_IMAGE_MIME_TYPE(map->outputformat));

      if( fo_filename != NULL )
        msIO_fprintf( stdout,
                      "Content-ID: coverage/%s\r\n"
                      "Content-Disposition: INLINE; filename=%s\r\n\r\n",
                      fo_filename,
                      fo_filename);
      else
        msIO_fprintf( stdout,
                      "Content-ID: coverage/wcs.%s\r\n"
                      "Content-Disposition: INLINE\r\n\r\n",
                      MS_IMAGE_EXTENSION(map->outputformat));
    } else {
      msIO_setHeader("Content-Type","%s",MS_IMAGE_MIME_TYPE(map->outputformat));
      msIO_setHeader("Content-Description","coverage data");
      msIO_setHeader("Content-Transfer-Encoding","binary");

      if( fo_filename != NULL ) {
        msIO_setHeader("Content-ID","coverage/%s",fo_filename);
        msIO_setHeader("Content-Disposition","INLINE; filename=%s",fo_filename);
      } else {
        msIO_setHeader("Content-ID","coverage/wcs.%s",MS_IMAGE_EXTENSION(map->outputformat));
        msIO_setHeader("Content-Disposition","INLINE");
      }
      msIO_sendHeaders();
    }

    status = msSaveImage(map, image, NULL);
    if( status != MS_SUCCESS ) {
      msSetError( MS_MISCERR, "msSaveImage() failed", "msWCSWriteFile20()");
      return msWCSException(map, "NoApplicableCode", "mapserv", params->version);
    }
    if(multipart)
      msIO_fprintf( stdout, "\r\n--wcs--\r\n" );
    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      When potentially listing multiple files, we take great care     */
  /*      to identify the "primary" file and list it first.  In fact      */
  /*      it is the only file listed in the coverages document.           */
  /* -------------------------------------------------------------------- */
  {
    char **all_files = CPLReadDir( base_dir );
    int count = CSLCount(all_files);

    if( msIO_needBinaryStdout() == MS_FAILURE )
      return MS_FAILURE;

    msAcquireLock( TLOCK_GDAL );
    for( i = count-1; i >= 0; i-- ) {
      const char *this_file = all_files[i];

      if( EQUAL(this_file,".") || EQUAL(this_file,"..") ) {
        all_files = CSLRemoveStrings( all_files, i, 1, NULL );
        continue;
      }

      if( i > 0 && EQUAL(this_file,CPLGetFilename(filename)) ) {
        all_files = CSLRemoveStrings( all_files, i, 1, NULL );
        all_files = CSLInsertString(all_files,0,CPLGetFilename(filename));
        i++;
      }
    }

    /* -------------------------------------------------------------------- */
    /*      Dump all the files in the memory directory as mime sections.    */
    /* -------------------------------------------------------------------- */
    count = CSLCount(all_files);

    if(count > 1 && multipart == MS_FALSE) {
      msDebug( "msWCSWriteFile20(): force multipart output without gml summary because we have multiple files in the result.\n" );

      multipart = MS_TRUE;
      msIO_setHeader("Content-Type","multipart/related; boundary=wcs");
      msIO_sendHeaders();
    }

    for( i = 0; i < count; i++ ) {
      const char *mimetype = NULL;
      FILE *fp;
      unsigned char block[4000];
      int bytes_read;

      if( i == 0
          && !EQUAL(MS_IMAGE_MIME_TYPE(map->outputformat), "unknown") )
        mimetype = MS_IMAGE_MIME_TYPE(map->outputformat);

      if( mimetype == NULL )
        mimetype = "application/octet-stream";
      if(multipart) {
        msIO_fprintf( stdout, "\r\n--wcs\r\n" );
        msIO_fprintf(
          stdout,
          "Content-Type: %s\r\n"
          "Content-Description: coverage data\r\n"
          "Content-Transfer-Encoding: binary\r\n"
          "Content-ID: coverage/%s\r\n"
          "Content-Disposition: INLINE; filename=%s\r\n\r\n",
          mimetype,
          all_files[i],
          all_files[i]);
      } else {
        msIO_setHeader("Content-Type","%s",mimetype);
        msIO_setHeader("Content-Description","coverage data");
        msIO_setHeader("Content-Transfer-Encoding","binary");
        msIO_setHeader("Content-ID","coverage/%s",all_files[i]);
        msIO_setHeader("Content-Disposition","INLINE; filename=%s",all_files[i]);
        msIO_sendHeaders();
      }


      fp = VSIFOpenL(
             CPLFormFilename(base_dir, all_files[i], NULL),
             "rb" );
      if( fp == NULL ) {
        msReleaseLock( TLOCK_GDAL );
        msSetError( MS_MISCERR,
                    "Failed to open %s for streaming to stdout.",
                    "msWCSWriteFile20()", all_files[i] );
        return MS_FAILURE;
      }

      while( (bytes_read = VSIFReadL(block, 1, sizeof(block), fp)) > 0 )
        msIO_fwrite( block, 1, bytes_read, stdout );

      VSIFCloseL( fp );

      VSIUnlink( CPLFormFilename(base_dir, all_files[i], NULL) );
    }

    msFree(base_dir);
    msFree(filename);
    CSLDestroy( all_files );
    msReleaseLock( TLOCK_GDAL );
    if(multipart)
      msIO_fprintf( stdout, "\r\n--wcs--\r\n" );
    return MS_SUCCESS;
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetRangesetAxisMetadata20()                   */
/*                                                                      */
/*      Looks up a layers metadata for a specific axis information.     */
/************************************************************************/

static const char *msWCSLookupRangesetAxisMetadata20(hashTableObj *table,
    const char *axis, const char *item)
{
  char buf[500];
  const char* value;

  if(table == NULL || axis == NULL || item == NULL) {
    return NULL;
  }

  strlcpy(buf, axis, sizeof(buf));
  strlcat(buf, "_", sizeof(buf));
  strlcat(buf, item, sizeof(buf));
  if((value = msLookupHashTable(table, buf)) != NULL) {
    return value;
  }
  return msOWSLookupMetadata(table, "CO", buf);
}

/************************************************************************/
/*                   msWCSGetCoverageMetadata20()                       */
/*                                                                      */
/*      Inits a coverageMetadataObj. Uses msWCSGetCoverageMetadata()    */
/*      but exchanges the SRS URN by an URI for compliancy with 2.0.    */
/************************************************************************/

static int msWCSGetCoverageMetadata20(layerObj *layer, wcs20coverageMetadataObj *cm)
{
  char  *srs_uri = NULL;
  memset(cm,0,sizeof(wcs20coverageMetadataObj));
  if ( msCheckParentPointer(layer->map,"map") == MS_FAILURE )
    return MS_FAILURE;

  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "CO", MS_TRUE, &(cm->srs_epsg));
  if(!cm->srs_epsg) {
    msOWSGetEPSGProj(&(layer->map->projection),
                                   &(layer->map->web.metadata), "CO", MS_TRUE, &cm->srs_epsg);
    if(!cm->srs_epsg) {
      msSetError(MS_WCSERR, "Unable to determine the SRS for this layer, "
                 "no projection defined and no metadata available.",
                 "msWCSGetCoverageMetadata20()");
      return MS_FAILURE;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Get the SRS in uri format.                                      */
  /* -------------------------------------------------------------------- */
  if((srs_uri = msOWSGetProjURI(&(layer->projection), &(layer->metadata),
                                "CO", MS_TRUE)) == NULL) {
    srs_uri = msOWSGetProjURI(&(layer->map->projection),
                              &(layer->map->web.metadata), "CO", MS_TRUE);
  }

  if( srs_uri != NULL ) {
    if( strlen(srs_uri) > sizeof(cm->srs_uri) - 1 ) {
      msSetError(MS_WCSERR, "SRS URI too long!",
                 "msWCSGetCoverageMetadata()");
      return MS_FAILURE;
    }

    strlcpy( cm->srs_uri, srs_uri, sizeof(cm->srs_uri) );
    msFree( srs_uri );
  } else {
    cm->srs_uri[0] = '\0';
  }

  /* setup nilvalues */
  cm->numnilvalues = 0;
  cm->nilvalues = NULL;
  cm->nilvalues_reasons = NULL;
  cm->native_format = NULL;

  /* -------------------------------------------------------------------- */
  /*      If we have "virtual dataset" metadata on the layer, then use    */
  /*      that in preference to inspecting the file(s).                   */
  /*      We require extent and either size or resolution.                */
  /* -------------------------------------------------------------------- */
  if( msOWSLookupMetadata(&(layer->metadata), "CO", "extent") != NULL
      && (msOWSLookupMetadata(&(layer->metadata), "CO", "resolution") != NULL
          || msOWSLookupMetadata(&(layer->metadata), "CO", "size") != NULL) ) {
    const char *value;

    /* get extent */
    cm->extent.minx = 0.0;
    cm->extent.maxx = 0.0;
    cm->extent.miny = 0.0;
    cm->extent.maxy = 0.0;
    if( msOWSGetLayerExtent( layer->map, layer, "CO", &cm->extent ) == MS_FAILURE )
      return MS_FAILURE;

    /* get resolution */
    cm->xresolution = 0.0;
    cm->yresolution = 0.0;
    if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", "resolution")) != NULL ) {
      char **tokens;
      int n;

      tokens = msStringSplit(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_resolution metadata.", "msWCSGetCoverageMetadata20()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xresolution = atof(tokens[0]);
      cm->yresolution = atof(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    /* get Size (in pixels and lines) */
    cm->xsize = 0;
    cm->ysize = 0;
    if( (value=msOWSLookupMetadata(&(layer->metadata), "CO", "size")) != NULL ) {
      char **tokens;
      int n;

      tokens = msStringSplit(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_size metadata.", "msWCSGetCoverageMetadata20()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xsize = atoi(tokens[0]);
      cm->ysize = atoi(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    /* try to compute raster size */
    if( cm->xsize == 0 && cm->ysize == 0 && cm->xresolution != 0.0 && cm->yresolution != 0.0 && cm->extent.minx != cm->extent.maxx && cm->extent.miny != cm->extent.maxy ) {
      cm->xsize = (int) ((cm->extent.maxx - cm->extent.minx) / cm->xresolution + 0.5);
      cm->ysize = (int) fabs((cm->extent.maxy - cm->extent.miny) / cm->yresolution + 0.5);
    }

    /* try to compute raster resolution */
    if( (cm->xresolution == 0.0 || cm->yresolution == 0.0) && cm->xsize != 0 && cm->ysize != 0 ) {
      cm->xresolution = (cm->extent.maxx - cm->extent.minx) / cm->xsize;
      cm->yresolution = (cm->extent.maxy - cm->extent.miny) / cm->ysize;
    }

    /* do we have information to do anything */
    if( cm->xresolution == 0.0 || cm->yresolution == 0.0 || cm->xsize == 0 || cm->ysize == 0 ) {
      msSetError( MS_WCSERR, "Failed to collect extent and resolution for WCS coverage from metadata for layer '%s'.  Need value wcs|ows_resolution or wcs|ows_size values.", "msWCSGetCoverageMetadata20()", layer->name );
      return MS_FAILURE;
    }

    /* compute geotransform */
    cm->geotransform[0] = cm->extent.minx;
    cm->geotransform[1] = cm->xresolution;
    cm->geotransform[2] = 0.0;
    cm->geotransform[3] = cm->extent.maxy;
    cm->geotransform[4] = 0.0;
    cm->geotransform[5] = -fabs(cm->yresolution);

    /* get bands count, or assume 1 if not found */
    cm->numbands = 1;
    if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", "bandcount")) != NULL) {
      int numbands = 0;
      msStringParseInteger(value, &numbands);
      cm->numbands = (size_t)numbands;
    }

    cm->bands = static_cast<wcs20rasterbandMetadataObj*>(msSmallCalloc(sizeof(wcs20rasterbandMetadataObj), cm->numbands));

    /* get bands type, or assume float if not found */
    cm->imagemode = MS_IMAGEMODE_FLOAT32;
    if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", "imagemode")) != NULL ) {
      if( EQUAL(value,"INT16") )
        cm->imagemode = MS_IMAGEMODE_INT16;
      else if( EQUAL(value,"FLOAT32") )
        cm->imagemode = MS_IMAGEMODE_FLOAT32;
      else if( EQUAL(value,"BYTE") )
        cm->imagemode = MS_IMAGEMODE_BYTE;
      else {
        msSetError( MS_WCSERR, "Content of wcs|ows_imagemode (%s) not recognised.  Should be one of BYTE, INT16 or FLOAT32.", "msWCSGetCoverageMetadata20()", value );
        return MS_FAILURE;
      }
    }

    if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", "native_format")) != NULL ) {
      cm->native_format = msStrdup(value);
    }

    /* determine nilvalues and reasons */
    {
      int n_nilvalues = 0, n_nilvalues_reasons = 0;
      char **t_nilvalues = NULL, **t_nilvalues_reasons = NULL;
      if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", "nilvalues")) != NULL ) {
        t_nilvalues = msStringSplit(value, ' ', &n_nilvalues);
      } else if((value = msOWSLookupMetadata(&(layer->metadata), "CO", "rangeset_nullvalue")) != NULL) {
        t_nilvalues = msStringSplit(value, ' ', &n_nilvalues);
      }

      if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", "nilvalues_reasons")) != NULL ) {
        t_nilvalues_reasons = msStringSplit(value, ' ', &n_nilvalues_reasons);
      }

      if(n_nilvalues > 0) {
        int i;
        cm->numnilvalues = n_nilvalues;
        cm->nilvalues = static_cast<char**>(msSmallCalloc(sizeof(char*), n_nilvalues));
        cm->nilvalues_reasons = static_cast<char**>(msSmallCalloc(sizeof(char*), n_nilvalues));
        for(i = 0; i < n_nilvalues; ++i) {
          cm->nilvalues[i] = msStrdup(t_nilvalues[i]);
          if(i < n_nilvalues_reasons) {
            cm->nilvalues_reasons[i] = msStrdup(t_nilvalues_reasons[i]);
          } else {
            cm->nilvalues_reasons[i] = msStrdup("");
          }
        }
      }
      msFreeCharArray(t_nilvalues, n_nilvalues);
      msFreeCharArray(t_nilvalues_reasons, n_nilvalues_reasons);
    }

    {
      int num_band_names = 0, j;
      char **band_names = NULL;

      const char *wcs11_band_names_key = "rangeset_axes";
      const char *wcs20_band_names_key = "band_names";

      const char *wcs11_interval_key = "interval";
      const char *wcs20_interval_key = "interval";
      const char *interval_key = NULL;

      const char *significant_figures_key = "significant_figures";

      const char *wcs11_keys[] =
      { "label", "semantic", "values_type", "values_semantic", "description" };
      const char *wcs20_keys[] =
      { "band_name", "band_interpretation", "band_uom", "band_definition", "band_description" };
      const char **keys = NULL;

      char **interval_array;
      int num_interval;

      wcs20rasterbandMetadataObj default_values;

      /* Decide whether WCS1.1 or WCS2.0 keys should be used */
      if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", wcs20_band_names_key) ) != NULL ) {
        keys = wcs20_keys;
        interval_key = wcs20_interval_key;
        band_names = msStringSplit(value, ' ', &num_band_names);
      } else if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", wcs11_band_names_key)) != NULL ) {
        keys = wcs11_keys;
        interval_key = wcs11_interval_key;
        /* "bands" has a special processing in WCS 1.0. See */
        /* msWCSSetDefaultBandsRangeSetInfo */
        if( EQUAL(value, "bands") )
        {
            num_band_names = cm->numbands;
            band_names = (char**) msSmallMalloc( sizeof(char*) * num_band_names );
            for( int i = 0; i < num_band_names; i++ )
            {
                char szName[30];
                snprintf(szName, sizeof(szName), "Band%d", i+1);
                band_names[i] = msStrdup(szName);
            }
        }
        else
        {
            /* WARNING: in WCS 1.x,, "rangeset_axes" has never been intended */
            /* to contain the list of band names... This code should probably */
            /* be removed */
            band_names = msStringSplit(value, ' ', &num_band_names);
        }
      }

      /* return with error when number of bands does not match    */
      /* the number of names                                      */
      if (static_cast<size_t>(num_band_names) != cm->numbands && num_band_names != 0) {
        msFreeCharArray(band_names, num_band_names);
        msSetError( MS_WCSERR,
                    "Wrong number of band names given in layer '%s'. "
                    "Expected %d, got %d.", "msWCSGetCoverageMetadata20()",
                    layer->name, (int)cm->numbands, num_band_names );
        return MS_FAILURE;
      }

      /* look up default metadata values */
      for(j = 1; j < 5; ++j) {
        if(keys != NULL) {
          default_values.values[j] = (char *)msOWSLookupMetadata(&(layer->metadata), "CO", keys[j]);
        }
      }

      /* set default values for interval and significant figures */
      switch(cm->imagemode) {
        case MS_IMAGEMODE_BYTE:
          default_values.interval_min = 0.;
          default_values.interval_max = 255.;
          default_values.significant_figures = 3;
          break;
        case MS_IMAGEMODE_INT16:
          default_values.interval_min = 0.;
          default_values.interval_max = (double)USHRT_MAX;
          default_values.significant_figures = 5;
          break;
        case MS_IMAGEMODE_FLOAT32:
          default_values.interval_min = -FLT_MAX;
          default_values.interval_max = FLT_MAX;
          default_values.significant_figures = 12;
          break;
        default:
          // other imagemodes are invalid (see above), just keep the compiler happy
          msFreeCharArray(band_names, num_band_names);
          return MS_FAILURE;
          break;
      }

      /* lookup default interval values */
      if (interval_key != NULL
          && (value = msOWSLookupMetadata(&(layer->metadata), "CO", interval_key)) != NULL) {
        interval_array = msStringSplit(value, ' ', &num_interval);

        if (num_interval != 2
            || msStringParseDouble(interval_array[0], &(default_values.interval_min)) != MS_SUCCESS
            || msStringParseDouble(interval_array[1], &(default_values.interval_max)) != MS_SUCCESS) {
          msFreeCharArray(band_names, num_band_names);
          msFreeCharArray(interval_array, num_interval);
          msSetError(MS_WCSERR, "Wrong interval format for default axis.",
                     "msWCSGetCoverageMetadata20()");
          return MS_FAILURE;
        }
        msFreeCharArray(interval_array, num_interval);
      }

      /* lookup default value for significant figures */
      if((value = msOWSLookupMetadata(&(layer->metadata), "CO", significant_figures_key)) != NULL) {
        if(msStringParseInteger(value, &(default_values.significant_figures)) != MS_SUCCESS) {
          msFreeCharArray(band_names, num_band_names);
          msSetError(MS_WCSERR, "Wrong significant figures format "
                     "for default axis.",
                     "msWCSGetCoverageMetadata20()");
          return MS_FAILURE;
        }
      }

      /* iterate over every band */
      for(unsigned i = 0; i < cm->numbands; ++i) {
        cm->bands[i].name = NULL;

        /* look up band metadata or copy defaults */
        if(num_band_names != 0) {
          cm->bands[i].name = msStrdup(band_names[i]);
          for(j = 1; j < 5; ++j) {
            value = (char *)msWCSLookupRangesetAxisMetadata20(&(layer->metadata),
                    cm->bands[i].name, keys[j]);
            if(value != NULL) {
              cm->bands[i].values[j] = msStrdup(value);
            } else if(default_values.values[j] != NULL) {
              cm->bands[i].values[j] = msStrdup(default_values.values[j]);
            }
          }
        }

        /* set up interval */
        value = (char *)msWCSLookupRangesetAxisMetadata20(&(layer->metadata),
                cm->bands[i].name, interval_key);
        if(value != NULL) {
          num_interval = 0;
          interval_array = msStringSplit(value, ' ', &num_interval);

          if (num_interval != 2
              || msStringParseDouble(interval_array[0], &(cm->bands[i].interval_min)) != MS_SUCCESS
              || msStringParseDouble(interval_array[1], &(cm->bands[i].interval_max)) != MS_SUCCESS) {
            msFreeCharArray(band_names, num_band_names);
            msFreeCharArray(interval_array, num_interval);
            msSetError(MS_WCSERR, "Wrong interval format for axis %s.",
                       "msWCSGetCoverageMetadata20()", cm->bands[i].name);
            return MS_FAILURE;
          }

          msFreeCharArray(interval_array, num_interval);
        } else {
          cm->bands[i].interval_min = default_values.interval_min;
          cm->bands[i].interval_max = default_values.interval_max;
        }

        /* set up significant figures */
        value = (char *)msWCSLookupRangesetAxisMetadata20(&(layer->metadata),
                cm->bands[i].name, significant_figures_key);
        if(value != NULL) {
          if(msStringParseInteger(value, &(cm->bands[i].significant_figures)) != MS_SUCCESS) {
            msFreeCharArray(band_names, num_band_names);
            msSetError(MS_WCSERR, "Wrong significant figures format "
                       "for axis %s.",
                       "msWCSGetCoverageMetadata20()", cm->bands[i].name);
            return MS_FAILURE;
          }
        } else {
          cm->bands[i].significant_figures = default_values.significant_figures;
        }
      }

      msFreeCharArray(band_names, num_band_names);
    }
  } else if( layer->data == NULL ) {
    /* no virtual metadata, not ok unless we're talking 1 image, hopefully we can fix that */
    msSetError( MS_WCSERR, "RASTER Layer with no DATA statement and no WCS virtual dataset metadata.  Tileindexed raster layers not supported for WCS without virtual dataset metadata (cm->extent, wcs_res, wcs_size).", "msWCSGetCoverageMetadata20()" );
    return MS_FAILURE;
  } else {
    /* work from the file (e.g. DATA) */
    GDALDatasetH hDS;
    GDALDriverH hDriver;
    GDALRasterBandH hBand;
    char szPath[MS_MAXPATHLEN];
    const char *driver_short_name, *driver_long_name;

    msGDALInitialize();

    msTryBuildPath3(szPath,  layer->map->mappath, layer->map->shapepath, layer->data);
    msAcquireLock( TLOCK_GDAL );
    {
        char** connectionoptions = msGetStringListFromHashTable(&(layer->connectionoptions));
        hDS = GDALOpenEx( szPath, GDAL_OF_RASTER, NULL,
                          (const char* const*)connectionoptions, NULL);
        CSLDestroy(connectionoptions);
    }
    if( hDS == NULL ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_IOERR, "%s", "msWCSGetCoverageMetadata20()", CPLGetLastErrorMsg() );
      return MS_FAILURE;
    }

    msGetGDALGeoTransform( hDS, layer->map, layer, cm->geotransform );

    cm->xsize = GDALGetRasterXSize( hDS );
    cm->ysize = GDALGetRasterYSize( hDS );

    cm->extent.minx = cm->geotransform[0];
    cm->extent.maxx = cm->geotransform[0] + cm->geotransform[1] * cm->xsize + cm->geotransform[2] * cm->ysize;
    cm->extent.miny = cm->geotransform[3] + cm->geotransform[4] * cm->xsize + cm->geotransform[5] * cm->ysize;
    cm->extent.maxy = cm->geotransform[3];

    cm->xresolution = cm->geotransform[1];
    cm->yresolution = cm->geotransform[5];

    cm->numbands = GDALGetRasterCount( hDS );

    /* TODO nilvalues? */

    if( cm->numbands == 0 ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_WCSERR, "Raster file %s has no raster bands.  This cannot be used in a layer.", "msWCSGetCoverageMetadata20()", layer->data );
      return MS_FAILURE;
    }

    hBand = GDALGetRasterBand( hDS, 1 );
    switch( GDALGetRasterDataType( hBand ) ) {
      case GDT_Byte:
        cm->imagemode = MS_IMAGEMODE_BYTE;
        break;
      case GDT_Int16:
        cm->imagemode = MS_IMAGEMODE_INT16;
        break;
      default:
        cm->imagemode = MS_IMAGEMODE_FLOAT32;
        break;
    }

    cm->bands = static_cast<wcs20rasterbandMetadataObj*>(msSmallCalloc(sizeof(wcs20rasterbandMetadataObj), cm->numbands));

    /* get as much band metadata as possible */
    for(unsigned i = 0; i < cm->numbands; ++i) {
      char bandname[32];
      GDALColorInterp colorInterp;
      hBand = GDALGetRasterBand(hDS, i + 1);
      snprintf(bandname, sizeof(bandname), "band%d", i+1);
      cm->bands[i].name = msStrdup(bandname);
      colorInterp = GDALGetRasterColorInterpretation(hBand);
      cm->bands[i].interpretation = msStrdup(GDALGetColorInterpretationName(colorInterp));
      cm->bands[i].uom = msStrdup(GDALGetRasterUnitType(hBand));
      if(strlen(cm->bands[i].uom) == 0) {
        msFree(cm->bands[i].uom);
        cm->bands[i].uom = NULL;
      }

      /* set up interval and significant figures */
      switch(cm->imagemode) {
        case MS_IMAGEMODE_BYTE:
        case MS_IMAGEMODE_PC256:
          cm->bands[i].interval_min = 0.;
          cm->bands[i].interval_max = 255.;
          cm->bands[i].significant_figures = 3;
          break;
        case MS_IMAGEMODE_INT16:
          cm->bands[i].interval_min = 0.;
          cm->bands[i].interval_max = (double)USHRT_MAX;
          cm->bands[i].significant_figures = 5;
          break;
        case MS_IMAGEMODE_FLOAT32:
          cm->bands[i].interval_min = -FLT_MAX;
          cm->bands[i].interval_max = FLT_MAX;
          cm->bands[i].significant_figures = 12;
          break;
      }
    }

    hDriver = GDALGetDatasetDriver(hDS);
    driver_short_name = GDALGetDriverShortName(hDriver);
    driver_long_name = GDALGetDriverLongName(hDriver);
    /* TODO: improve this, exchange strstr() */
    for(int i = 0; i < layer->map->numoutputformats; ++i) {
      if(strstr( layer->map->outputformatlist[i]->driver, driver_short_name) != NULL
          || strstr(layer->map->outputformatlist[i]->driver, driver_long_name) != NULL) {
        cm->native_format = msStrdup(layer->map->outputformatlist[i]->mimetype);
        break;
      }
    }

    GDALClose( hDS );
    msReleaseLock( TLOCK_GDAL );
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSClearCoverageMetadata20()                     */
/*                                                                      */
/*      Returns all dynamically allocated memory from a coverage meta-  */
/*      data object.                                                    */
/************************************************************************/

static int msWCSClearCoverageMetadata20(wcs20coverageMetadataObj *cm)
{
  msFree(cm->native_format);
  for(unsigned i = 0; i < cm->numnilvalues; ++i) {
    msFree(cm->nilvalues[i]);
    msFree(cm->nilvalues_reasons[i]);
  }
  msFree(cm->nilvalues);
  msFree(cm->nilvalues_reasons);

  for(unsigned i = 0; i < cm->numbands; ++i) {
    for(int j = 0; j < 5; ++j) {
      msFree(cm->bands[i].values[j]);
    }
  }
  msFree(cm->bands);
  msFree(cm->srs_epsg);
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
  const char *status = "400 Bad Request";
  char *errorString = NULL;
  char *schemasLocation = NULL;
  char *xsi_schemaLocation = NULL;
  char version_string[OWS_VERSION_MAXLEN];

  xmlDocPtr psDoc = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNodePtr psMainNode = NULL;
  xmlNsPtr psNsOws = NULL;
  xmlNsPtr psNsXsi = NULL;
  xmlChar *buffer = NULL;

  errorString = msGetErrorString("\n");
  schemasLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "ExceptionReport");
  psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_20_NAMESPACE_URI,
                     BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  xmlSetNs(psRootNode, psNsOws);

  psNsXsi = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

  /* add attributes to root element */
  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST version);
  xmlNewProp(psRootNode, BAD_CAST "xml:lang", BAD_CAST msOWSGetLanguage(map, "exception"));

  /* get 2 digits version string: '2.0' */
  msOWSGetVersionString(OWS_2_0_0, version_string);
  version_string[3]= '\0';

  xsi_schemaLocation = msStrdup((char *)psNsOws->href);
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

  if (locator != NULL) {
    xmlNewProp(psMainNode, BAD_CAST "locator", BAD_CAST locator);
  }

  if (errorString != NULL) {
    char* errorMessage = msEncodeHTMLEntities(errorString);
    xmlNewChild(psMainNode, NULL, BAD_CAST "ExceptionText", BAD_CAST errorMessage);
    msFree(errorMessage);
  }

  /*psRootNode = msOWSCommonExceptionReport(psNsOws, OWS_2_0_0,
          schemasLocation, version, msOWSGetLanguage(map, "exception"),
          exceptionCode, locator, errorMessage);*/

  xmlDocSetRootElement(psDoc, psRootNode);

  if (exceptionCode == NULL) {
    /* Do nothing */
  }
  else if(EQUAL(exceptionCode, "OperationNotSupported")
     || EQUAL(exceptionCode, "OptionNotSupported")) {
    status = "501 Not Implemented";
  }
  else if (EQUAL(exceptionCode, "NoSuchCoverage")
           || EQUAL(exceptionCode, "emptyCoverageIdList")
           || EQUAL(exceptionCode, "InvalidAxisLabel")
           || EQUAL(exceptionCode, "InvalidSubsetting")) {
    status = "404 Not Found";
  }

  msIO_setHeader("Status", "%s", status);
  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, "UTF-8", 1);

  msIO_printf("%s", buffer);

  /* free buffer and the document */
  free(errorString);
  free(schemasLocation);
  free(xsi_schemaLocation);
  xmlFree(buffer);
  xmlFreeDoc(psDoc);

  /* clear error since we have already reported it */
  msResetErrorList();
  return MS_FAILURE;
}

#define MAX_MIMES 20

static int msWCSGetCapabilities20_CreateProfiles(
  mapObj *map, xmlNodePtr psServiceIdentification, xmlNsPtr psOwsNs)
{
  xmlNodePtr psProfile, psTmpNode;
  char *available_mime_types[MAX_MIMES];
  int i = 0;

  /* even indices are urls, uneven are mime-types, or null*/
  const char * const urls_and_mime_types[] = {
    MS_WCS_20_PROFILE_CORE,     NULL,
    MS_WCS_20_PROFILE_KVP,      NULL,
    MS_WCS_20_PROFILE_POST,     NULL,
    MS_WCS_20_PROFILE_GML,      NULL,
    MS_WCS_20_PROFILE_GML_MULTIPART, NULL,
    MS_WCS_20_PROFILE_GML_SPECIAL, NULL,
    MS_WCS_20_PROFILE_GML_GEOTIFF, "image/tiff",
    MS_WCS_20_PROFILE_CRS,     NULL,
    MS_WCS_20_PROFILE_SCALING, NULL,
    MS_WCS_20_PROFILE_RANGESUBSET, NULL,
    MS_WCS_20_PROFILE_INTERPOLATION, NULL,
    NULL, NULL /* guardian */
  };

  /* navigate to node where profiles shall be inserted */
  for(psTmpNode = psServiceIdentification->children; psTmpNode->next != NULL; psTmpNode = psTmpNode->next) {
    if(EQUAL((char *)psTmpNode->name, "ServiceTypeVersion") && !EQUAL((char *)psTmpNode->next->name, "ServiceTypeVersion"))
      break;
  }

  /* get list of all available mime types */
  msGetOutputFormatMimeList(map, available_mime_types, MAX_MIMES);

  while(urls_and_mime_types[i] != NULL) {
    const char* mime_type = urls_and_mime_types[i+1];

    /* check if there is a mime type */
    if(mime_type != NULL) {
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
  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetCapabilities20_CoverageSummary()           */
/*                                                                      */
/*      Helper function to create a short summary for a specific        */
/*      coverage.                                                       */
/************************************************************************/

static int msWCSGetCapabilities20_CoverageSummary(
  xmlDocPtr doc, xmlNodePtr psContents, layerObj *layer )
{
  wcs20coverageMetadataObj cm;
  int status;
  xmlNodePtr psCSummary;

  xmlNsPtr psWcsNs = xmlSearchNs( doc, xmlDocGetRootElement(doc), BAD_CAST "wcs" );

  status = msWCSGetCoverageMetadata20(layer, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;

  psCSummary = xmlNewChild( psContents, psWcsNs, BAD_CAST "CoverageSummary", NULL );
  xmlNewChild(psCSummary, psWcsNs, BAD_CAST "CoverageId", BAD_CAST layer->name);
  xmlNewChild(psCSummary, psWcsNs, BAD_CAST "CoverageSubtype", BAD_CAST "RectifiedGridCoverage");

  msWCS_11_20_PrintMetadataLinks(layer, doc, psCSummary);

  msWCSClearCoverageMetadata20(&cm);

  return MS_SUCCESS;
}


/************************************************************************/
/*                          msWCSAddInspireDSID20                       */
/************************************************************************/

static void msWCSAddInspireDSID20(mapObj *map,
                                  xmlNsPtr psNsInspireDls,
                                  xmlNsPtr psNsInspireCommon,
                                  xmlNodePtr pDlsExtendedCapabilities)
{
    const char* dsid_code = msOWSLookupMetadata(&(map->web.metadata), "CO", "inspire_dsid_code");
    const char* dsid_ns = msOWSLookupMetadata(&(map->web.metadata), "CO", "inspire_dsid_ns");
    if( dsid_code == NULL )
    {
        xmlAddChild(pDlsExtendedCapabilities, xmlNewComment(BAD_CAST "WARNING: Required metadata \"inspire_dsid_code\" missing"));
    }
    else
    {
        int ntokensCode = 0, ntokensNS = 0;
        char** tokensCode;
        char** tokensNS = NULL;
        int i;

        tokensCode = msStringSplit(dsid_code, ',', &ntokensCode);
        if( dsid_ns != NULL )
            tokensNS = msStringSplitComplex( dsid_ns, ",", &ntokensNS, MS_ALLOWEMPTYTOKENS);
        if( ntokensNS > 0 && ntokensNS != ntokensCode )
        {
            xmlAddChild(pDlsExtendedCapabilities,
                        xmlNewComment(BAD_CAST "WARNING: \"inspire_dsid_code\" and \"inspire_dsid_ns\" have not the same number of elements. Ignoring inspire_dsid_ns"));
            msFreeCharArray(tokensNS, ntokensNS);
            tokensNS = NULL;
            ntokensNS = 0;
        }
        for(i = 0; i<ntokensCode; i++ )
        {
            xmlNodePtr pSDSI = xmlNewNode(psNsInspireDls, BAD_CAST "SpatialDataSetIdentifier");
            xmlAddChild(pDlsExtendedCapabilities, pSDSI);
            xmlNewTextChild(pSDSI, psNsInspireCommon, BAD_CAST "Code", BAD_CAST tokensCode[i]);
            if( ntokensNS > 0 && tokensNS[i][0] != '\0' )
                xmlNewTextChild(pSDSI, psNsInspireCommon, BAD_CAST "Namespace", BAD_CAST tokensNS[i]);
        }
        msFreeCharArray(tokensCode, ntokensCode);
        if( ntokensNS > 0 )
            msFreeCharArray(tokensNS, ntokensNS);
    }
}


/************************************************************************/
/*                   msWCSGetCapabilities20()                           */
/*                                                                      */
/*      Performs the GetCapabilities operation. Writes out a xml        */
/*      structure with server specific information and a summary        */
/*      of all available coverages.                                     */
/************************************************************************/

int msWCSGetCapabilities20(mapObj *map, cgiRequestObj *req,
                           wcs20ParamsObjPtr params, owsRequestObj *ows_request)
{
  xmlDocPtr psDoc = NULL;       /* document pointer */
  xmlNodePtr psRootNode,
             psOperationsNode,
             psNode;
  const char *updatesequence = NULL;
  xmlNsPtr psOwsNs = NULL,
           psXLinkNs = NULL,
           psWcsNs = NULL,
           psCrsNs = NULL,
           psIntNs = NULL;
  char *script_url=NULL, *script_url_encoded=NULL, *format_list=NULL;
  int i;
  xmlDocPtr pInspireTmpDoc = NULL;

  const char *inspire_capabilities = msOWSLookupMetadata(&(map->web.metadata), "CO", "inspire_capabilities");

  char *validated_language = msOWSLanguageNegotiation(map, "CO", params->accept_languages, CSLCount(params->accept_languages));

  /* -------------------------------------------------------------------- */
  /*      Create document.                                                */
  /* -------------------------------------------------------------------- */
  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "Capabilities");

  xmlDocSetRootElement(psDoc, psRootNode);

  /* -------------------------------------------------------------------- */
  /*      Name spaces                                                     */
  /* -------------------------------------------------------------------- */

  msWCSPrepareNamespaces20(psDoc, psRootNode, map, inspire_capabilities != NULL);

  /* lookup namespaces */
  psOwsNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX );
  psWcsNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX );
  xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX );
  psXLinkNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST "xlink" );
  psCrsNs = xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/wcs/crs/1.0", BAD_CAST "crs");
  psIntNs = xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/wcs/interpolation/1.0", BAD_CAST "int");

  xmlSetNs(psRootNode, psWcsNs);

  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST params->version );

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "CO", "updatesequence");
  if (params->updatesequence != NULL) {
    i = msOWSNegotiateUpdateSequence(params->updatesequence, updatesequence);
    if (i == 0) { /* current */
      msSetError(MS_WCSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)",
                 "msWCSGetCapabilities20()", params->updatesequence, updatesequence);
      xmlFreeDoc(psDoc);
      return msWCSException(map, "CurrentUpdateSequence", "updatesequence",
                            params->version);
    }
    if (i > 0) { /* invalid */
      msSetError(MS_WCSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)",
                 "msWCSGetCapabilities20()", params->updatesequence, updatesequence);
      xmlFreeDoc(psDoc);
      return msWCSException(map, "InvalidUpdateSequence", "updatesequence",
                            params->version);
    }
  }
  if(updatesequence != NULL) {
    xmlNewProp(psRootNode, BAD_CAST "updateSequence", BAD_CAST updatesequence);
  }

  /* -------------------------------------------------------------------- */
  /*      Service identification.                                         */
  /* -------------------------------------------------------------------- */
  if ( MS_WCS_20_CAPABILITIES_INCLUDE_SECTION(params, "ServiceIdentification") ) {
    psNode = xmlAddChild(psRootNode, msOWSCommonServiceIdentification(
                           psOwsNs, map, "OGC WCS", "2.0.1,1.1.1,1.0.0", "CO", validated_language));
    msWCSGetCapabilities20_CreateProfiles(map, psNode, psOwsNs);
  }

  /* Service Provider */
  if ( MS_WCS_20_CAPABILITIES_INCLUDE_SECTION(params, "ServiceProvider") ) {
    xmlAddChild(psRootNode,
                msOWSCommonServiceProvider(psOwsNs, psXLinkNs, map, "CO", validated_language));
  }

  /* -------------------------------------------------------------------- */
  /*      Operations metadata.                                            */
  /* -------------------------------------------------------------------- */
  if ( MS_WCS_20_CAPABILITIES_INCLUDE_SECTION(params, "OperationsMetadata") ) {
    if ((script_url = msOWSGetOnlineResource(map, "CO", "onlineresource", req)) == NULL
        || (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
      xmlFreeDoc(psDoc);
      msSetError(MS_WCSERR, "Server URL not found", "msWCSGetCapabilities20()");
      return msWCSException(map, "NoApplicableCode", "mapserv", params->version);
    }
    free(script_url);

    psOperationsNode = xmlAddChild(psRootNode,msOWSCommonOperationsMetadata(psOwsNs));

    /* -------------------------------------------------------------------- */
    /*      GetCapabilities - add Sections and AcceptVersions?              */
    /* -------------------------------------------------------------------- */
    psNode = msOWSCommonOperationsMetadataOperation(
               psOwsNs, psXLinkNs,
               "GetCapabilities", OWS_METHOD_GETPOST, script_url_encoded);

    xmlAddChild(psNode->last->last->last,
                msOWSCommonOperationsMetadataDomainType(OWS_2_0_0, psOwsNs, "Constraint",
                    "PostEncoding", "XML"));
    xmlAddChild(psOperationsNode, psNode);

    /* -------------------------------------------------------------------- */
    /*      DescribeCoverage                                                */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "C", "DescribeCoverage", MS_FALSE)) {
      psNode = msOWSCommonOperationsMetadataOperation(
                 psOwsNs, psXLinkNs,
                 "DescribeCoverage", OWS_METHOD_GETPOST, script_url_encoded);
      xmlAddChild(psNode->last->last->last,
                  msOWSCommonOperationsMetadataDomainType(OWS_2_0_0, psOwsNs, "Constraint",
                      "PostEncoding", "XML"));
      xmlAddChild(psOperationsNode, psNode);
    }

    /* -------------------------------------------------------------------- */
    /*      GetCoverage                                                     */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "C", "GetCoverage", MS_FALSE)) {
      psNode = msOWSCommonOperationsMetadataOperation(
                 psOwsNs, psXLinkNs,
                 "GetCoverage", OWS_METHOD_GETPOST, script_url_encoded);

      xmlAddChild(psNode->last->last->last,
                  msOWSCommonOperationsMetadataDomainType(OWS_2_0_0, psOwsNs, "Constraint",
                      "PostEncoding", "XML"));
      xmlAddChild(psOperationsNode, psNode);
    }
    msFree(script_url_encoded);


    /* -------------------------------------------------------------------- */
    /*      Extended Capabilities for inspire                               */
    /* -------------------------------------------------------------------- */

    if (inspire_capabilities) {
      msIOContext* old_context;
      msIOContext* new_context;
      msIOBuffer* buffer;

      xmlNodePtr pRoot;
      xmlNodePtr pOWSExtendedCapabilities;
      xmlNodePtr pDlsExtendedCapabilities;
      xmlNodePtr pChild;

      xmlNsPtr psInspireCommonNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_INSPIRE_COMMON_NAMESPACE_PREFIX );
      xmlNsPtr psInspireDlsNs = xmlSearchNs( psDoc, psRootNode, BAD_CAST MS_INSPIRE_DLS_NAMESPACE_PREFIX );


      old_context = msIO_pushStdoutToBufferAndGetOldContext();
      msOWSPrintInspireCommonExtendedCapabilities(stdout, map, "CO", OWS_WARN,
                                                  "foo",
                                                  "xmlns:" MS_INSPIRE_COMMON_NAMESPACE_PREFIX "=\"" MS_INSPIRE_COMMON_NAMESPACE_URI "\" "
                                                  "xmlns:" MS_INSPIRE_DLS_NAMESPACE_PREFIX "=\"" MS_INSPIRE_DLS_NAMESPACE_URI "\" "
                                                  "xmlns:xsi=\"" MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI "\"", validated_language, OWS_WCS);

      new_context = msIO_getHandler(stdout);
      buffer = (msIOBuffer *) new_context->cbData;

      /* Remove spaces between > and < to get properly indented result */
      msXMLStripIndentation( (char*) buffer->data );

      pInspireTmpDoc = xmlParseDoc((const xmlChar *)buffer->data);
      pRoot = xmlDocGetRootElement(pInspireTmpDoc);
      xmlReconciliateNs(psDoc, pRoot);

      pOWSExtendedCapabilities = xmlNewNode(psOwsNs, BAD_CAST "ExtendedCapabilities");
      xmlAddChild(psOperationsNode, pOWSExtendedCapabilities);

      pDlsExtendedCapabilities = xmlNewNode(psInspireDlsNs, BAD_CAST "ExtendedCapabilities");
      xmlAddChild(pOWSExtendedCapabilities, pDlsExtendedCapabilities);

      pChild = pRoot->children;
      while(pChild != NULL)
      {
          xmlNodePtr pNext = pChild->next;
          xmlUnlinkNode(pChild);
          xmlAddChild(pDlsExtendedCapabilities, pChild);
          pChild = pNext;
      }

      msWCSAddInspireDSID20(map, psInspireDlsNs, psInspireCommonNs, pDlsExtendedCapabilities);

      msIO_restoreOldStdoutContext(old_context);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Service metadata.                                               */
  /* -------------------------------------------------------------------- */

  if ( MS_WCS_20_CAPABILITIES_INCLUDE_SECTION(params, "ServiceMetadata") ) {
    xmlNodePtr psExtensionNode = NULL;
    psNode = xmlNewChild(psRootNode, psWcsNs, BAD_CAST "ServiceMetadata", NULL);

    /* Apply default formats */
    msApplyDefaultOutputFormats(map);

    /* Add formats list */
    format_list = msWCSGetFormatsList20(map, NULL);
    msLibXml2GenerateList(psNode, psWcsNs, "formatSupported", format_list, ',');
    msFree(format_list);

    psExtensionNode = xmlNewChild(psNode, psWcsNs, BAD_CAST "Extension", NULL);
    /* interpolations supported */
    {
      /* TODO: use URIs/URLs once they are specified */
      const char * const interpolation_methods[] = {"NEAREST", "AVERAGE", "BILINEAR"};
      int i;
      xmlNodePtr imNode = xmlNewChild(psExtensionNode, psIntNs,
                                      BAD_CAST "InterpolationMetadata", NULL);

      for (i=0; i < 3; ++i) {
        xmlNewChild(imNode, psIntNs, BAD_CAST "InterpolationSupported",
                    BAD_CAST interpolation_methods[i]);
      }
    }

    /* Report the supported CRSs */
    {
      char *crs_list = NULL;
      xmlNodePtr crsMetadataNode = xmlNewChild(psExtensionNode, psCrsNs,
                                               BAD_CAST "CrsMetadata", NULL);

      if((crs_list = msOWSGetProjURI(&(map->projection),
                                        &(map->web.metadata),
                                        "CO", MS_FALSE)) != NULL ) {
        msLibXml2GenerateList(crsMetadataNode, psCrsNs, "crsSupported",
                              crs_list, ' ');
        msFree(crs_list);
      } else {
        /* could not determine list of CRSs */
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Contents section.                                               */
  /* -------------------------------------------------------------------- */
  if ( MS_WCS_20_CAPABILITIES_INCLUDE_SECTION(params, "Contents") ) {
    psNode = xmlNewChild( psRootNode, psWcsNs, BAD_CAST "Contents", NULL );

    if(ows_request->numlayers == 0) {
      xmlAddChild(psNode,
                  xmlNewComment(BAD_CAST("WARNING: No WCS layers are enabled. "
                                         "Check wcs/ows_enable_request settings.")));
    } else {
      for(i = 0; i < map->numlayers; ++i) {
        layerObj *layer = map->layers[i];
        int       status;

        if(!msWCSIsLayerSupported(layer))
          continue;

        if (!msIntegerInArray(layer->index, ows_request->enabled_layers, ows_request->numlayers))
          continue;

        status = msWCSGetCapabilities20_CoverageSummary(
                   psDoc, psNode, layer );
        if(status != MS_SUCCESS) {
          msFree(validated_language);
          xmlFreeDoc(psDoc);
          xmlCleanupParser();
          return msWCSException(map, "Internal", "mapserv", params->version);
        }
      }
    }
  }
  /* -------------------------------------------------------------------- */
  /*      Write out the document and clean up.                            */
  /* -------------------------------------------------------------------- */
  msWCSWriteDocument20(psDoc);
  msFree(validated_language);
  xmlFreeDoc(psDoc);
  if( pInspireTmpDoc )
    xmlFreeDoc(pInspireTmpDoc);
  xmlCleanupParser();
  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSDescribeCoverage20_CoverageDescription()      */
/*                                                                      */
/*      Creates a description of a specific coverage with the three     */
/*      major sections: BoundedBy, DomainSet and RangeType.             */
/************************************************************************/

static int msWCSDescribeCoverage20_CoverageDescription(
    layerObj *layer, xmlDocPtr psDoc, xmlNodePtr psRootNode )
{
  int status, swapAxes;
  wcs20coverageMetadataObj cm;
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

  swapAxes = msWCSSwapAxes20(cm.srs_uri);

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
  msWCSCommon20_CreateBoundedBy(&cm, psGmlNs, psCD, &(layer->projection), swapAxes);

  xmlNewChild(psCD, psWcsNs, BAD_CAST "CoverageId", BAD_CAST layer->name);

  /* -------------------------------------------------------------------- */
  /*      gml:domainSet                                                   */
  /* -------------------------------------------------------------------- */
  msWCSCommon20_CreateDomainSet(layer, &cm, psGmlNs, psCD, &(layer->projection), swapAxes);

  /* -------------------------------------------------------------------- */
  /*      gmlcov:rangeType                                                */
  /* -------------------------------------------------------------------- */
  msWCSCommon20_CreateRangeType(&cm, NULL, psGmlcovNs, psSweNs, psCD);

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
    /* for now, WCS 2.0 does not allow per coverage CRS definitions */
    /*{
      xmlNodePtr psSupportedCrss;
      char *owned_value;

      psSupportedCrss = xmlNewChild(psSP, psWcsNs,
                                    BAD_CAST "SupportedCRSs", NULL);

      if ((owned_value = msOWSGetProjURI(&(layer->projection),
                                         &(layer->metadata), "CO", MS_FALSE)) != NULL)
      { }
      else if ((owned_value = msOWSGetProjURI(&(layer->map->projection),
                                              &(layer->map->web.metadata), "CO", MS_FALSE)) != NULL)
      { }
      else {
        msDebug("missing required information, no SRSs defined.\n");
      }

      if (owned_value != NULL && strlen(owned_value) > 0) {
        msLibXml2GenerateList(psSupportedCrss, psWcsNs,
                              "SupportedCRS", owned_value, ' ');
      }

      xmlNewChild(psSupportedCrss, psWcsNs,
                  BAD_CAST "NativeCRS", BAD_CAST cm.srs_uri);

      msFree(owned_value);
    }*/

    /* -------------------------------------------------------------------- */
    /*      SupportedFormats                                                */
    /* -------------------------------------------------------------------- */
    /* for now, WCS 2.0 does not allow per coverage format definitions */
    /*{
      xmlNodePtr psSupportedFormats;
      char *format_list;

      psSupportedFormats =
      xmlNewChild(psSP, psWcsNs, BAD_CAST "SupportedFormats", NULL);

      format_list = msWCSGetFormatsList20(layer->map, layer);

      if (strlen(format_list) > 0) {
        msLibXml2GenerateList(psSupportedFormats, psWcsNs,
                              "SupportedFormat", format_list, ',');

      msFree(format_list);
    }*/


    /* -------------------------------------------------------------------- */
    /*      nativeFormat                                                    */
    /* -------------------------------------------------------------------- */
    xmlNewChild(psSP, psWcsNs,
                BAD_CAST "nativeFormat", BAD_CAST (cm.native_format ?
                    cm.native_format : ""));

    if (!cm.native_format) {
      msDebug("msWCSDescribeCoverage20_CoverageDescription(): "
              "No native format specified.\n");
    }
  }

  msWCSClearCoverageMetadata20(&cm);

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

int msWCSDescribeCoverage20(mapObj *map, wcs20ParamsObjPtr params, owsRequestObj *ows_request)
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
  msWCSPrepareNamespaces20(psDoc, psRootNode, map, MS_FALSE);

  psWcsNs = xmlSearchNs(psDoc, psRootNode,
                        BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX);
  xmlSetNs(psRootNode, psWcsNs);

  /* check if IDs are given */
  if (params->ids) {
    /* for each given ID in the ID-list */
    for (j = 0; params->ids[j]; j++) {
      i = msGetLayerIndex(map, params->ids[j]);
      if (i == -1 || (!msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers)) ) {
        msSetError(MS_WCSERR, "Unknown coverage: (%s)",
                   "msWCSDescribeCoverage20()", params->ids[j]);
        return msWCSException(map, "NoSuchCoverage", "coverage",
                              params->version);
      }
      /* create coverage description for the specified layer */
      if(msWCSDescribeCoverage20_CoverageDescription((GET_LAYER(map, i)),
          psDoc, psRootNode) == MS_FAILURE) {
        msSetError(MS_WCSERR, "Error retrieving coverage description.", "msWCSDescribeCoverage20()");
        return msWCSException(map, "MissingParameterValue", "coverage",
                              params->version);
      }
    }
  } else {
    /* Throw error, since IDs are mandatory */
    msSetError(MS_WCSERR, "Missing COVERAGEID parameter.", "msWCSDescribeCoverage20()");
    return msWCSException(map, "MissingParameterValue", "coverage",
                          params->version);
  }

  /* write out the DOM document to the stream */
  msWCSWriteDocument20(psDoc);

  /* cleanup */
  xmlFreeDoc(psDoc);
  xmlCleanupParser();

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetCoverage_FinalizeParamsObj20()             */
/*                                                                      */
/*      Finalizes a wcs20ParamsObj for a GetCoverage operation. In the  */
/*      process, the params boundig box is adjusted to the subsets,     */
/*      width, height and resolution are determined and the subset crs  */
/*      is found out.                                                   */
/************************************************************************/

static int msWCSGetCoverage20_FinalizeParamsObj(wcs20ParamsObjPtr params, wcs20AxisObjPtr *axes)
{
  char *crs = NULL;
  int have_scale, have_size, have_resolution;
  int have_global_scale = (params->scale != MS_WCS20_UNBOUNDED) ? 1 : 0;

  if (axes[0] != NULL) {
    if(axes[0]->subset != NULL) {
      msDebug("Subset for X-axis found: %s\n", axes[0]->subset->axis);
      if (!axes[0]->subset->min.unbounded)
        params->bbox.minx = axes[0]->subset->min.scalar;
      if (!axes[0]->subset->max.unbounded)
        params->bbox.maxx = axes[0]->subset->max.scalar;
      crs = axes[0]->subset->crs;
    }
    params->width = axes[0]->size;
    params->resolutionX = axes[0]->resolution;
    params->scaleX = axes[0]->scale;

    if(axes[0]->resolutionUOM != NULL) {
      params->resolutionUnits = msStrdup(axes[0]->resolutionUOM);
    }

    have_scale = (params->scaleX != MS_WCS20_UNBOUNDED) ? 1 : 0;
    have_size = (params->width != 0) ? 1 : 0;
    have_resolution = (params->resolutionX != MS_WCS20_UNBOUNDED) ? 1 : 0;

    if ((have_global_scale + have_scale + have_size + have_resolution) > 1) {
      msSetError(MS_WCSERR, "Axis '%s' defines scale, size and/or resolution multiple times.",
                 "msWCSGetCoverage20_FinalizeParamsObj()", axes[0]->name);
      return MS_FAILURE;
    }
  }

  if (axes[1] != NULL) {
    if(axes[1]->subset != NULL) {
      msDebug("Subset for Y-axis found: %s\n", axes[1]->subset->axis);
      if (!axes[1]->subset->min.unbounded)
        params->bbox.miny = axes[1]->subset->min.scalar;
      if (!axes[1]->subset->max.unbounded)
        params->bbox.maxy = axes[1]->subset->max.scalar;
      if(crs != NULL && axes[0] != NULL && axes[0]->subset!= NULL) {
        if(!EQUAL(crs, axes[1]->subset->crs)) {
          msSetError(MS_WCSERR, "CRS for axis %s and axis %s are not the same.",
                     "msWCSGetCoverage20_FinalizeParamsObj()", axes[0]->name, axes[1]->name);
          return MS_FAILURE;
        }
      } else {
        crs = axes[1]->subset->crs;
      }
    }
    params->height = axes[1]->size;
    params->resolutionY = axes[1]->resolution;
    params->scaleY = axes[1]->scale;

    if(params->resolutionUnits == NULL && axes[1]->resolutionUOM != NULL) {
      params->resolutionUnits = msStrdup(axes[1]->resolutionUOM);
    } else if(params->resolutionUnits != NULL && axes[1]->resolutionUOM != NULL
              && !EQUAL(params->resolutionUnits, axes[1]->resolutionUOM)) {
      msSetError(MS_WCSERR, "The units of measure of the resolution for"
                 "axis %s and axis %s are not the same.",
                 "msWCSGetCoverage20_FinalizeParamsObj()", axes[0]->name, axes[1]->name);
      return MS_FAILURE;
    }

    have_scale = (params->scaleY != MS_WCS20_UNBOUNDED) ? 1 : 0;
    have_size = (params->height != 0) ? 1 : 0;
    have_resolution = (params->resolutionY != MS_WCS20_UNBOUNDED) ? 1 : 0;

    if ((have_global_scale + have_scale + have_size + have_resolution) > 1) {
      msSetError(MS_WCSERR, "Axis '%s' defines scale, size and/or resolution multiple times.",
                 "msWCSGetCoverage20_FinalizeParamsObj()", axes[1]->name);
      return MS_FAILURE;
    }
  }

  /* if scale is globally set, use this value instead */
  if (params->scale != MS_WCS20_UNBOUNDED) {
    params->scaleX = params->scaleY = params->scale;
  }

  /* check if projections are equal */
  if(crs != NULL) {
    if (params->subsetcrs && !EQUAL(crs, params->subsetcrs)) {
      /* already set and not equal -> raise exception */
      msSetError(MS_WCSERR, "SubsetCRS does not match the CRS of the axes: "
                 "'%s' != '%s'",
                 "msWCSCreateBoundingBox20()", params->subsetcrs, crs);
      return MS_FAILURE;
    }
    else if (params->subsetcrs) {
      /* equal, and already set -> do nothing */
    }
    else {
      /* not yet globally set -> set it */
      params->subsetcrs = msStrdup(crs);
    }
  } else if (!params->subsetcrs) {
    /* default to CRS of image */
    /* leave params->subsetcrs to null */
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetCoverage20_GetBands()                      */
/*                                                                      */
/*      Returns a string, containing a comma-separated list of band     */
/*      indices.                                                        */
/************************************************************************/

static int msWCSGetCoverage20_GetBands(layerObj *layer,
                                       wcs20ParamsObjPtr params, wcs20coverageMetadataObjPtr cm, char **bandlist)
{
  int maxlen, index;
  char *interval_stop;
  char **band_ids = NULL;

  /* if rangesubset parameter is not given, default to all bands */
  if(NULL == params->range_subset) {
    *bandlist = msStrdup("1");
    for(unsigned i = 1; i < cm->numbands; ++i) {
      char strnumber[12];
      snprintf(strnumber, sizeof(strnumber), ",%d", i + 1);
      *bandlist = msStringConcatenate(*bandlist, strnumber);
    }
    return MS_SUCCESS;
  }

  maxlen = cm->numbands * 4 * sizeof(char);
  *bandlist = static_cast<char*>( msSmallCalloc(sizeof(char), maxlen));

  /* Use WCS 2.0 metadata items in priority */
  {
    char* tmp = msOWSGetEncodeMetadata(&layer->metadata,
                               "CO", "band_names", NULL);

    if( NULL == tmp ) {
      /* Otherwise default to WCS 1.x*/
      tmp = msOWSGetEncodeMetadata(&layer->metadata,
                     "CO", "rangeset_axes", NULL);
      /* "bands" has a special processing in WCS 1.0. See */
      /* msWCSSetDefaultBandsRangeSetInfo */
      if( tmp != NULL && EQUAL(tmp, "bands") )
      {
        int num_band_names = cm->numbands;
        band_ids = (char**) msSmallCalloc( sizeof(char*), (num_band_names + 1) );
        for( int i = 0; i < num_band_names; i++ )
        {
            char szName[30];
            snprintf(szName, sizeof(szName), "Band%d", i+1);
            band_ids[i] = msStrdup(szName);
        }
      }
    }

    if(NULL != tmp && band_ids == NULL) {
      band_ids = CSLTokenizeString2(tmp, " ", 0);
    }
    msFree(tmp);
  }

  /* If we still don't have band names, use the band names from the coverage metadata */
  if (band_ids == NULL) {
    band_ids = (char**) CPLCalloc(sizeof(char*), (cm->numbands + 1));
    for (unsigned i = 0; i < cm->numbands; ++i) {
      band_ids[i] = CPLStrdup(cm->bands[i].name);
    }
  }

  /* Iterate over all supplied range */
  const int count = CSLCount(params->range_subset);
  for(int i = 0; i < count; ++i) {
    /* RangeInterval case: defined as "<start>:<stop>" */
    if ((interval_stop = strchr(params->range_subset[i], ':')) != NULL) {
      int start, stop;
      *interval_stop = '\0';
      ++interval_stop;

      /* check if the string represents an integer */
      if (msStringParseInteger(params->range_subset[i], &start) == MS_SUCCESS) {
      }
      /* get index of start clause, or raise an error if none was found */
      else if ((start = CSLFindString(band_ids, params->range_subset[i])) != -1) {
        start += 1; /* adjust index, since GDAL bands start with index 1 */
      }
      else {
        msSetError(MS_WCSERR, "'%s' is not a valid band identifier.",
               "msWCSGetCoverage20_GetBands()", params->range_subset[i]);
        return MS_FAILURE;
      }

      /* check if the string represents an integer */
      if (msStringParseInteger(interval_stop, &stop) == MS_SUCCESS) {
      }
      /* get index of stop clause, or raise an error if none was found */
      else if ((stop = CSLFindString(band_ids, interval_stop)) != -1) {
        stop += 1; /* adjust index, since GDAL bands start with index 1 */
      }
      else {
        msSetError(MS_WCSERR, "'%s' is not a valid band identifier.",
               "msWCSGetCoverage20_GetBands()", interval_stop);
        return MS_FAILURE;
      }

      /* Check whether or not start and stop are different and stop is higher than start */
      if (stop <= start) {
        msSetError(MS_WCSERR, "Invalid range interval given.",
               "msWCSGetCoverage20_GetBands()");
        return MS_FAILURE;
      } else if (start < 1 || static_cast<unsigned>(stop) > cm->numbands) {
        msSetError(MS_WCSERR, "Band interval is out of the valid range: 1-%d",
               "msWCSGetCoverage20_GetBands()", (int)cm->numbands);
        return MS_FAILURE;
      }

      /* expand the interval to a list of indices and push them on the list */
      for (int j = start; j <= stop; ++j) {
        char* tmp;
        if(i != 0 || j != start) {
          strlcat(*bandlist, ",", maxlen);
        }
        tmp = msIntToString(j);
        strlcat(*bandlist, tmp, maxlen);
        msFree(tmp);
      }
    }
    /* RangeComponent case */
    else {
        if(i != 0) {
        strlcat(*bandlist, ",", maxlen);
      }

      /* check if the string represents an integer */
      if(msStringParseInteger(params->range_subset[i], &index) == MS_SUCCESS) {
        char* tmp;
        if (index < 1 || static_cast<unsigned>(index) > cm->numbands) {
          msSetError(MS_WCSERR, "Band index is out of the valid range: 1-%d",
                 "msWCSGetCoverage20_GetBands()", (int)cm->numbands);
          return MS_FAILURE;
        }

        tmp = msIntToString((int)index);
        strlcat(*bandlist, tmp, maxlen);
        msFree(tmp);
        continue;
      }

      /* check if the string is equal to a band identifier    */
      /* if so, what is the index of the band                 */
      if((index = CSLFindString(band_ids, params->range_subset[i])) != -1) {
        char* tmp = msIntToString((int)index + 1);
        strlcat(*bandlist, tmp, maxlen);
        msFree(tmp);
      }
      else {
        msSetError(MS_WCSERR, "'%s' is not a valid band identifier.",
                   "msWCSGetCoverage20_GetBands()", params->range_subset[i]);
        return MS_FAILURE;
      }
    }
  }

  CSLDestroy(band_ids);
  return MS_SUCCESS;
}

/* Fixes CPLParseNameValue to allow ':' characters for namespaced keys */

static const char * fixedCPLParseNameValue(const char* string, char **key) {
  const char *value;
  size_t size;
  value = strchr(string, '=');

  if (value == NULL) {
    *key = NULL;
    return NULL;
  }

  size = value - string;
  *key = static_cast<char*>(msSmallMalloc(size + 1));
  strncpy(*key, string, size + 1);
  (*key)[size] = '\0';
  return value + 1;
}

/************************************************************************/
/*                   msWCSSetFormatParams20()                           */
/*                                                                      */
/*      Parses the given format options and sets the appropriate        */
/*      output format values.                                           */
/************************************************************************/

static int msWCSSetFormatParams20(outputFormatObj* format, char** format_options) {
  /* currently geotiff only */
  char *format_option;
  int i = 0;
  int is_geotiff = (format->mimetype && EQUAL(format->mimetype, "image/tiff"));

  if (!is_geotiff || !format_options) {
    /* Currently only geotiff available */
    return MS_SUCCESS;
  }

  format_option = format_options[0];
  while (format_option) {
    /* key, value */
    char *key;
    const char *value = fixedCPLParseNameValue(format_option, &key);

    if (!key) {
      msSetError(MS_WCSERR, "Could not deduct the option key.",
                 "msWCSSetFormatParams20()");
      return MS_FAILURE;
    }
    else if (!value) {
      msSetError(MS_WCSERR, "Missing value for parameter '%s'.",
                 "msWCSSetFormatParams20()", key);
      return MS_FAILURE;
    }

    if (EQUAL(key, "geotiff:compression") && is_geotiff) {
      /*COMPRESS=[JPEG/LZW/PACKBITS/DEFLATE/CCITTRLE/CCITTFAX3/CCITTFAX4/NONE]*/
      if (EQUAL(value, "None")) {
        msSetOutputFormatOption(format, "COMPRESS", "NONE");
      }
      else if (EQUAL(value, "PackBits")) {
        msSetOutputFormatOption(format, "COMPRESS", "PACKBITS");
      }
      else if (EQUAL(value, "Deflate")) {
        msSetOutputFormatOption(format, "COMPRESS", "DEFLATE");
      }
      else if (EQUAL(value, "Huffman")) {
        msSetOutputFormatOption(format, "COMPRESS", "CCITTRLE");
      }
      else if (EQUAL(value, "LZW")) {
        msSetOutputFormatOption(format, "COMPRESS", "LZW");
      }
      else if (EQUAL(value, "JPEG")) {
        msSetOutputFormatOption(format, "COMPRESS", "JPEG");
      }
      /* unsupported compression methods: CCITTFAX3/CCITTFAX4 */
      else {
        msSetError(MS_WCSERR, "Compression method '%s' not supported.",
                   "msWCSSetFormatParams20()", value);
        return MS_FAILURE;
      }
    }

    else if (EQUAL(key, "geotiff:jpeg_quality") && is_geotiff) {
      int quality;
      /* JPEG_QUALITY=[1-100] */
      if (msStringParseInteger(value, &quality) != MS_SUCCESS) {
        msSetError(MS_WCSERR, "Could not parse jpeg_quality value.",
                 "msWCSSetFormatParams20()");
        return MS_FAILURE;
      }
      else if (quality < 1 || quality > 100) {
        msSetError(MS_WCSERR, "Invalid jpeg_quality value '%d'.",
                   "msWCSSetFormatParams20()", quality);
        return MS_FAILURE;
      }
      msSetOutputFormatOption(format, "JPEG_QUALITY", value);
    }
    else if (EQUAL(key, "geotiff:predictor") && is_geotiff) {
      /* PREDICTOR=[None/Horizontal/FloatingPoint] */
      const char *predictor;
      if (EQUAL(value, "None") || EQUAL(value, "1")) {
        predictor = "1";
      }
      else if (EQUAL(value, "Horizontal") || EQUAL(value, "2")) {
        predictor = "2";
      }
      else if (EQUAL(value, "FloatingPoint") || EQUAL(value, "3")) {
        predictor = "3";
      }
      else {
        msSetError(MS_WCSERR, "Invalid predictor value '%s'.",
                   "msWCSSetFormatParams20()", value);
        return MS_FAILURE;
      }
      msSetOutputFormatOption(format, "PREDICTOR", predictor);
    }
    else if (EQUAL(key, "geotiff:interleave") && is_geotiff) {
      /* INTERLEAVE=[BAND,PIXEL] */
      if (EQUAL(value, "Band")) {
        msSetOutputFormatOption(format, "INTERLEAVE", "BAND");
      }
      else if (EQUAL(value, "Pixel")) {
        msSetOutputFormatOption(format, "INTERLEAVE", "PIXEL");
      }
      else {
        msSetError(MS_WCSERR, "Interleave method '%s' not supported.",
                   "msWCSSetFormatParams20()", value);
        return MS_FAILURE;
      }
    }
    else if (EQUAL(key, "geotiff:tiling") && is_geotiff) {
      /* TILED=YES */
      if (EQUAL(value, "true")) {
        msSetOutputFormatOption(format, "TILED", "YES");
      }
      else if (EQUAL(value, "false")) {
        msSetOutputFormatOption(format, "TILED", "NO");
      }
      else {
        msSetError(MS_WCSERR, "Invalid boolean value '%s'.",
                   "msWCSSetFormatParams20()", value);
        return MS_FAILURE;
      }
    }
    else if (EQUAL(key, "geotiff:tileheight") && is_geotiff) {
      /* BLOCKXSIZE=n */
      int tileheight;

      if (msStringParseInteger(value, &tileheight) != MS_SUCCESS) {
        msSetError(MS_WCSERR, "Could not parse tileheight value.",
                 "msWCSSetFormatParams20()");
        return MS_FAILURE;
      }
      else if (tileheight < 1 || tileheight % 16) {
        msSetError(MS_WCSERR, "Invalid tileheight value '%d'. "
                   "Must be greater than 0 and dividable by 16.",
                   "msWCSSetFormatParams20()", tileheight);
        return MS_FAILURE;
      }
      msSetOutputFormatOption(format, "BLOCKXSIZE", value);
    }
    else if (EQUAL(key, "geotiff:tilewidth") && is_geotiff) {
      /* BLOCKYSIZE=n */
      int tilewidth;

      if (msStringParseInteger(value, &tilewidth) != MS_SUCCESS) {
        msSetError(MS_WCSERR, "Could not parse tilewidth value.",
                 "msWCSSetFormatParams20()");
        return MS_FAILURE;
      }
      else if (tilewidth < 1 || tilewidth % 16) {
        msSetError(MS_WCSERR, "Invalid tilewidth value '%d'. "
                   "Must be greater than 0 and dividable by 16.",
                   "msWCSSetFormatParams20()", tilewidth);
        return MS_FAILURE;
      }
      msSetOutputFormatOption(format, "BLOCKYSIZE", value);
    }
    else if (EQUALN(key, "geotiff:", 8)) {
      msSetError(MS_WCSERR, "Unrecognized GeoTIFF parameter '%s'.",
                 "msWCSSetFormatParams20()", key);
      return MS_FAILURE;
    }

    msFree(key);

    /* fetch next option */
    format_option = format_options[++i];
  }

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
                       wcs20ParamsObjPtr params, owsRequestObj *ows_request)
{
  (void)request;
  layerObj *layer = NULL;
  wcs20coverageMetadataObj cm;
  imageObj *image = NULL;
  outputFormatObj *format = NULL;

  rectObj subsets, bbox;
  projectionObj imageProj;

  int status, i;
  double x_1, x_2, y_1, y_2;
  char *coverageName, *bandlist=NULL, numbands[12];

  int doDrawRasterLayerDraw = MS_TRUE;
  GDALDatasetH hDS = NULL;

  int widthFromComputationInImageCRS = 0;
  int heightFromComputationInImageCRS = 0;

  /* number of coverage ids should be 1 */
  if (params->ids == NULL || params->ids[0] == NULL) {
    msSetError(MS_WCSERR, "Required parameter CoverageID was not supplied.",
               "msWCSGetCoverage20()");
    return msWCSException(map, "MissingParameterValue", "coverage",
                          params->version);
  }
  if (params->ids[1] != NULL) {
    msSetError(MS_WCSERR, "GetCoverage operation supports only one coverage.",
               "msWCSGetCoverage20()");
    return msWCSException(map, "TooManyParameterValues", "coverage",
                          params->version);
  }

  /* find the right layer */
  layer = NULL;
  for(i = 0; i < map->numlayers; i++) {
    coverageName = msOWSGetEncodeMetadata(&(GET_LAYER(map, i)->metadata),
                                          "CO", "name",
                                          GET_LAYER(map, i)->name);
    if (EQUAL(coverageName, params->ids[0]) &&
        (msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))) {
      layer = GET_LAYER(map, i);
      i = map->numlayers; /* to exit loop don't use break, we want to free resources first */
    }
    msFree(coverageName);
  }

  /* throw exception if no Layer was found */
  if (layer == NULL) {
    msSetError(MS_WCSERR,
               "COVERAGEID=%s not found, not in supported layer list. A layer might be disabled for \
this request. Check wcs/ows_enable_request settings.", "msWCSGetCoverage20()", params->ids[0]);
    return msWCSException(map, "NoSuchCoverage", "coverageid",
                          params->version);
  }
  /* retrieve coverage metadata  */
  status = msWCSGetCoverageMetadata20(layer, &cm);
  if (status != MS_SUCCESS) {
    msWCSClearCoverageMetadata20(&cm);
    return MS_FAILURE;
  }

  /* fill in bands rangeset info, if required.  */
  /* msWCSSetDefaultBandsRangeSetInfo(NULL, &cm, layer ); */

  /* set  resolution, size and maximum extent */
  layer->extent = map->extent = cm.extent;
  map->cellsize = cm.xresolution;
  map->width = cm.xsize;
  map->height = cm.ysize;

  /************************************************************************/
  /*      finalize the params object. determine subset crs and subset     */
  /*      bbox. Also project the image to the subset crs.                 */
  /************************************************************************/

  msInitProjection(&imageProj);
  msProjectionInheritContextFrom(&imageProj, &(layer->projection));
  if (msLoadProjectionString(&imageProj, cm.srs_epsg) == -1) {
    msFreeProjection(&imageProj);
    msWCSClearCoverageMetadata20(&cm);
    msSetError(MS_WCSERR,
               "Error loading CRS %s.",
               "msWCSGetCoverage20()", cm.srs_epsg);
    return msWCSException(map, "InvalidParameterValue",
                          "projection", params->version);
  }

  /* iterate over all subsets and check if they are valid*/
  for(i = 0; i < params->numaxes; ++i) {
    if(params->axes[i]->subset != NULL) {
      if(params->axes[i]->subset->timeOrScalar == MS_WCS20_TIME_VALUE) {
        msFreeProjection(&imageProj);
        msWCSClearCoverageMetadata20(&cm);
        msSetError(MS_WCSERR, "Time values for subsets are not supported. ",
                   "msWCSGetCoverage20()");
        return msWCSException(map, "InvalidSubsetting", "subset", params->version);
      }
      if(params->axes[i]->subset->operation == MS_WCS20_SLICE) {
        msFreeProjection(&imageProj);
        msWCSClearCoverageMetadata20(&cm);
        msSetError(MS_WCSERR, "Subset operation 'slice' is not supported.",
                   "msWCSGetCoverage20()");
        return msWCSException(map, "InvalidSubsetting", "subset", params->version);
      }
    }
  }

  {
    wcs20AxisObjPtr axes[2];
    if(msWCSValidateAndFindAxes20(params, axes) == MS_FAILURE) {
      msFreeProjection(&imageProj);
      msWCSClearCoverageMetadata20(&cm);
      return msWCSException(map, "InvalidAxisLabel", "subset", params->version);
    }
    if(msWCSGetCoverage20_FinalizeParamsObj(params, axes) == MS_FAILURE) {
      msFreeProjection(&imageProj);
      msWCSClearCoverageMetadata20(&cm);
      return msWCSException(map, "InvalidParameterValue", "extent", params->version);
    }
  }

  subsets = params->bbox;

  /* if no subsetCRS was specified use the coverages CRS 
     (Requirement 27 of the WCS 2.0 specification) */
  if (!params->subsetcrs) {
    params->subsetcrs = msStrdup(cm.srs_epsg);
  }

  if(EQUAL(params->subsetcrs, "imageCRS")) {
    /* subsets are in imageCRS; reproject them to real coordinates */
    rectObj orig_bbox = subsets;

    msFreeProjection(&(map->projection));
    map->projection = imageProj;

    if(subsets.minx != -DBL_MAX || subsets.maxx != DBL_MAX) {
      x_1 = cm.geotransform[0]
            + orig_bbox.minx * cm.geotransform[1]
            + orig_bbox.miny * cm.geotransform[2];
      x_2 = cm.geotransform[0]
            + (orig_bbox.maxx+1) * cm.geotransform[1]
            + (orig_bbox.maxy+1) * cm.geotransform[2];

      subsets.minx = MS_MIN(x_1, x_2);
      subsets.maxx = MS_MAX(x_1, x_2);
    }
    if(subsets.miny != -DBL_MAX || subsets.maxy != DBL_MAX) {
      y_1 = cm.geotransform[3]
            + (orig_bbox.maxx+1) * cm.geotransform[4]
            + (orig_bbox.maxy+1) * cm.geotransform[5];
      /*subsets.miny -= cm.geotransform[4]/2 + cm.geotransform[5]/2;*/
      y_2 = cm.geotransform[3]
            + orig_bbox.minx * cm.geotransform[4]
            + orig_bbox.miny * cm.geotransform[5];

      subsets.miny = MS_MIN(y_1, y_2);
      subsets.maxy = MS_MAX(y_1, y_2);
    }
  } else { /* if crs is not the 'imageCRS' */
    projectionObj subsetProj;

    /* if the subsets have a crs given, project the image extent to it */
    msInitProjection(&subsetProj);
    msProjectionInheritContextFrom(&subsetProj, &(layer->projection));
    if(msLoadProjectionString(&subsetProj, params->subsetcrs) != MS_SUCCESS) {
      msFreeProjection(&subsetProj);
      msFreeProjection(&imageProj);
      msWCSClearCoverageMetadata20(&cm);
      msSetError(MS_WCSERR,
                 "Error loading CRS %s.",
                 "msWCSGetCoverage20()", params->subsetcrs);
      return msWCSException(map, "InvalidParameterValue",
                            "projection", params->version);
    }

    if(msProjectionsDiffer(&imageProj, &subsetProj)) {

      /* Reprojection of source raster extent of (-180,-90,180,90) to any */
      /* projected CRS is going to exhibit strong anomalies. So instead */
      /* do the reverse, project the subset extent to the layer CRS, and */
      /* see how much the subset extent takes with respect to the source */
      /* raster extent. This is only used if output width and resolutionX (or */
      /* (height and resolutionY) are unknown. */
      if( ((params->width == 0 && params->resolutionX == MS_WCS20_UNBOUNDED) ||
           (params->height == 0 && params->resolutionY == MS_WCS20_UNBOUNDED)) &&
          (msProjIsGeographicCRS(&imageProj) &&
           !msProjIsGeographicCRS(&subsetProj) &&
           fabs(layer->extent.minx - -180.0) < 1e-5 &&
           fabs(layer->extent.miny - -90.0) < 1e-5 &&
           fabs(layer->extent.maxx - 180.0) < 1e-5 &&
           fabs(layer->extent.maxy - 90.0) < 1e-5) )
      {
          rectObj subsetInImageProj = subsets;
          if( msProjectRect(&subsetProj, &imageProj, &(subsetInImageProj)) == MS_SUCCESS )
          {
            subsetInImageProj.minx = MS_MAX(subsetInImageProj.minx, layer->extent.minx);
            subsetInImageProj.miny = MS_MAX(subsetInImageProj.miny, layer->extent.miny);
            subsetInImageProj.maxx = MS_MIN(subsetInImageProj.maxx, layer->extent.maxx);
            subsetInImageProj.maxy = MS_MIN(subsetInImageProj.maxy, layer->extent.maxy);
            {
                double total = ABS(layer->extent.maxx - layer->extent.minx);
                double part = ABS(subsetInImageProj.maxx - subsetInImageProj.minx);
                widthFromComputationInImageCRS = MS_NINT((part * map->width) / total);
            }
            {
                double total = ABS(layer->extent.maxy - layer->extent.miny);
                double part = ABS(subsetInImageProj.maxy - subsetInImageProj.miny);
                heightFromComputationInImageCRS = MS_NINT((part * map->height) / total);
            }
          }
      }

      msProjectRect(&imageProj, &subsetProj, &(layer->extent));
      map->extent = layer->extent;
      msFreeProjection(&(map->projection));
      map->projection = subsetProj;
      msFreeProjection(&imageProj);
    } else {
      msFreeProjection(&(map->projection));
      map->projection = imageProj;
      msFreeProjection(&subsetProj);
    }
  }

  /* create boundings of params subsets and image extent */
  if(msRectOverlap(&subsets, &(layer->extent)) == MS_FALSE) {
    /* extent and bbox do not overlap -> exit */
    msWCSClearCoverageMetadata20(&cm);
    msSetError(MS_WCSERR, "Image extent does not intersect with desired region.",
               "msWCSGetCoverage20()");
    return msWCSException(map, "ExtentError", "extent", params->version);
  }

  /* write combined bounding box */
  bbox.minx = MS_MAX(subsets.minx, map->extent.minx);
  bbox.miny = MS_MAX(subsets.miny, map->extent.miny);
  bbox.maxx = MS_MIN(subsets.maxx, map->extent.maxx);
  bbox.maxy = MS_MIN(subsets.maxy, map->extent.maxy);

  /* check if we are overspecified  */
  if ((params->width != 0 &&  params->resolutionX != MS_WCS20_UNBOUNDED)
      || (params->height != 0 && params->resolutionY != MS_WCS20_UNBOUNDED)) {
    msWCSClearCoverageMetadata20(&cm);
    msSetError(MS_WCSERR, "GetCoverage operation supports only one of SIZE or RESOLUTION per axis.",
               "msWCSGetCoverage20()");
    return msWCSException(map, "TooManyParameterValues", "coverage",
                          params->version);
  }

  /************************************************************************/
  /* check both axes: see if either size or resolution are given (and     */
  /* calculate the other value). If both are not given, calculate them    */
  /* from the bounding box.                                               */
  /************************************************************************/

  /* check x axis */
  if(params->width != 0) {
    /* TODO Unit Of Measure? */
    params->resolutionX = (bbox.maxx - bbox.minx) / params->width;
  } else if(params->resolutionX != MS_WCS20_UNBOUNDED) {
    params->width = MS_NINT((bbox.maxx - bbox.minx) / params->resolutionX);
  } else {
    if( widthFromComputationInImageCRS != 0 ) {
      params->width = widthFromComputationInImageCRS;
    } else if(ABS(bbox.maxx - bbox.minx) != ABS(map->extent.maxx - map->extent.minx)) {
      double total = ABS(map->extent.maxx - map->extent.minx),
             part = ABS(bbox.maxx - bbox.minx);
      params->width = MS_NINT((part * map->width) / total);
    } else {
      params->width = map->width;
    }

    params->resolutionX = (bbox.maxx - bbox.minx) / params->width;

    if (params->scaleX != MS_WCS20_UNBOUNDED) {
      params->resolutionX /= params->scaleX;
      params->width = (long)(((double) params->width) * params->scaleX);
    }
  }

  /* check y axis */
  if(params->height != 0) {
    params->resolutionY = (bbox.maxy - bbox.miny) / params->height;
  } else if(params->resolutionY != MS_WCS20_UNBOUNDED) {
    params->height = MS_NINT((bbox.maxy - bbox.miny) / params->resolutionY);
  } else {
    if( heightFromComputationInImageCRS != 0 ) {
      params->height = heightFromComputationInImageCRS;
    } else if(ABS(bbox.maxy - bbox.miny) != ABS(map->extent.maxy - map->extent.miny)) {
      double total = ABS(map->extent.maxy - map->extent.miny),
             part = ABS(bbox.maxy - bbox.miny);
      params->height = MS_NINT((part * map->height) / total);
    } else {
      params->height = map->height;
    }

    params->resolutionY = (bbox.maxy - bbox.miny) / params->height;

    if (params->scaleY != MS_WCS20_UNBOUNDED) {
      params->resolutionY /= params->scaleY;
      params->height = (long)(((double) params->height) * params->scaleY);
    }
  }

  /* WCS 2.0 is center of pixel oriented */
  bbox.minx += params->resolutionX * 0.5;
  bbox.maxx -= params->resolutionX * 0.5;
  bbox.miny += params->resolutionY * 0.5;
  bbox.maxy -= params->resolutionY * 0.5;

  /* if parameter 'outputcrs' is given, project the image to this crs */
  if(params->outputcrs != NULL) {
    projectionObj outputProj;

    msInitProjection(&outputProj);
    msProjectionInheritContextFrom(&outputProj, &(layer->projection));
    if(msLoadProjectionString(&outputProj, params->outputcrs) == -1) {
      msFreeProjection(&outputProj);
      msWCSClearCoverageMetadata20(&cm);
      return msWCSException(map, "InvalidParameterValue", "coverage",
                            params->version);
    }
    if(msProjectionsDiffer(&(map->projection), &outputProj)) {

      msDebug("msWCSGetCoverage20(): projecting to outputcrs %s\n", params->outputcrs);

      msProjectRect(&(map->projection), &outputProj, &bbox);
      msFreeProjection(&(map->projection));
      map->projection = outputProj;

      /* recalculate resolutions, needed if UOM changes (e.g: deg -> m) */
      params->resolutionX = (bbox.maxx - bbox.minx) / params->width;
      params->resolutionY = (bbox.maxy - bbox.miny) / params->height;
    }
    else {
      msFreeProjection(&outputProj);
    }
  }

  /* set the bounding box as new map extent */
  map->extent = bbox;
  map->width = params->width;
  map->height = params->height;

  /* Are we exceeding the MAXSIZE limit on result size? */
  if(map->width > map->maxsize || map->height > map->maxsize ) {
    msWCSClearCoverageMetadata20(&cm);
    msSetError(MS_WCSERR, "Raster size out of range, width and height of "
               "resulting coverage must be no more than MAXSIZE=%d.",
               "msWCSGetCoverage20()", map->maxsize);

    return msWCSException(map, "InvalidParameterValue",
                          "size", params->version);
  }

  /* Mapserver only supports square cells */
  if (params->resolutionX <= params->resolutionY)
    map->cellsize = params->resolutionX;
  else
    map->cellsize = params->resolutionY;

  msDebug("msWCSGetCoverage20(): Set parameters from original"
          "data. Width: %d, height: %d, cellsize: %f, extent: %f,%f,%f,%f\n",
          map->width, map->height, map->cellsize, map->extent.minx,
          map->extent.miny, map->extent.maxx, map->extent.maxy);

  /**
   * Which format to use?
   *
   * 1) format parameter
   * 2) native format (from metadata) or GDAL format of the input dataset
   * 3) exception
   **/

  if (!params->format) {
    if (cm.native_format) {
      params->format = msStrdup(cm.native_format);
    }
  }

  if (!params->format) {
    msSetError(MS_WCSERR, "Output format could not be automatically determined. "
               "Use the FORMAT parameter to specify a format.",
               "msWCSGetCoverage20()");
    msWCSClearCoverageMetadata20(&cm);
    return msWCSException(map, "MissingParameterValue", "format",
                          params->version);
  }

  /*    make sure layer is on   */
  layer->status = MS_ON;

  msMapComputeGeotransform(map);

  /*    fill in bands rangeset info, if required.  */
  /* msWCSSetDefaultBandsRangeSetInfo(params, &cm, layer); */
  /* msDebug("Bandcount: %d\n", cm.bandcount); */

  msApplyDefaultOutputFormats(map);

  if (msGetOutputFormatIndex(map, params->format) == -1) {
    msSetError(MS_WCSERR, "Unrecognized value '%s' for the FORMAT parameter.",
               "msWCSGetCoverage20()", params->format);
    msWCSClearCoverageMetadata20(&cm);
    return msWCSException(map, "InvalidParameterValue", "format",
                          params->version);
  }

  /* create a temporary outputformat (we likely will need to tweak parts) */
  format = msCloneOutputFormat(msSelectOutputFormat(map, params->format));
  msApplyOutputFormat(&(map->outputformat), format, MS_NOOVERRIDE,
                      MS_NOOVERRIDE, MS_NOOVERRIDE);

  /* set format specific parameters */
  if (msWCSSetFormatParams20(format, params->format_options) != MS_SUCCESS) {
    msFree(bandlist);
    msWCSClearCoverageMetadata20(&cm);
    return msWCSException(map, "InvalidParameterValue", "format",
                          params->version);
  }

  if(msWCSGetCoverage20_GetBands(layer, params, &cm, &bandlist) != MS_SUCCESS) {
    msFree(bandlist);
    msWCSClearCoverageMetadata20(&cm);
    return msWCSException(map, "InvalidParameterValue", "rangesubset",
                          params->version);
  }
  msLayerSetProcessingKey(layer, "BANDS", bandlist);
  snprintf(numbands, sizeof(numbands), "%d", msCountChars(bandlist, ',')+1);
  msSetOutputFormatOption(map->outputformat, "BAND_COUNT", numbands);

  msWCSApplyLayerCreationOptions(layer, map->outputformat, bandlist);

  /* check for the interpolation */
  /* Defaults to NEAREST */
  if(params->interpolation != NULL) {
    if(EQUALN(params->interpolation,"NEAREST",7)) {
      msLayerSetProcessingKey(layer, "RESAMPLE", "NEAREST");
    } else if(EQUAL(params->interpolation,"BILINEAR")) {
      msLayerSetProcessingKey(layer, "RESAMPLE", "BILINEAR");
    } else if(EQUAL(params->interpolation,"AVERAGE")) {
      msLayerSetProcessingKey(layer, "RESAMPLE", "AVERAGE");
    } else {
      msFree(bandlist);
      msSetError( MS_WCSERR, "'%s' specifies an unsupported interpolation method.",
                  "msWCSGetCoverage20()", params->interpolation );
      msWCSClearCoverageMetadata20(&cm);
      return msWCSException(map, "InvalidParameterValue", "interpolation", params->version);
    }
  } else {
    msLayerSetProcessingKey(layer, "RESAMPLE", "NEAREST");
  }

  /* since the dataset is only used in one layer, set it to be    */
  /* closed after drawing the layer. This normally defaults to    */
  /* DEFER and will produce a memory leak, because the dataset    */
  /* will not be closed.                                          */
  if( msLayerGetProcessingKey(layer, "CLOSE_CONNECTION") == NULL ) {
    msLayerSetProcessingKey(layer, "CLOSE_CONNECTION", "NORMAL");
  }

  if( layer->tileindex == NULL && layer->data != NULL &&
      strlen(layer->data) > 0 &&
      layer->connectiontype != MS_KERNELDENSITY )
  {
      if( msDrawRasterLayerLowCheckIfMustDraw(map, layer) )
      {
          char* decrypted_path = NULL;
          char szPath[MS_MAXPATHLEN];
          hDS = (GDALDatasetH)msDrawRasterLayerLowOpenDataset(
                                    map, layer, layer->data, szPath, &decrypted_path);
          msFree(decrypted_path);
          if( hDS )
            msWCSApplyDatasetMetadataAsCreationOptions(layer, map->outputformat, bandlist, hDS);
      }
      else
      {
          doDrawRasterLayerDraw = MS_FALSE;
      }
  }

  /* create the image object  */
  if (MS_RENDERER_PLUGIN(map->outputformat)) {
    image = msImageCreate(map->width, map->height, map->outputformat,
                          map->web.imagepath, map->web.imageurl, map->resolution,
                          map->defresolution, &map->imagecolor);
  } else if (MS_RENDERER_RAWDATA(map->outputformat)) {
    image = msImageCreate(map->width, map->height, map->outputformat,
                          map->web.imagepath, map->web.imageurl, map->resolution,
                          map->defresolution, &map->imagecolor);
  } else {
    msFree(bandlist);
    msWCSClearCoverageMetadata20(&cm);
    msSetError(MS_WCSERR, "Map outputformat not supported for WCS!",
               "msWCSGetCoverage20()");
    msDrawRasterLayerLowCloseDataset(layer, hDS);
    return msWCSException(map, NULL, NULL, params->version);
  }

  if (image == NULL) {
    msFree(bandlist);
    msWCSClearCoverageMetadata20(&cm);
    msDrawRasterLayerLowCloseDataset(layer, hDS);
    return msWCSException(map, NULL, NULL, params->version);
  }

  if(layer->mask) {
    int maskLayerIdx = msGetLayerIndex(map,layer->mask);
    layerObj *maskLayer;
    outputFormatObj *altFormat;
    if(maskLayerIdx == -1) {
      msSetError(MS_MISCERR, "Layer (%s) references unknown mask layer (%s)", "msDrawLayer()",
                 layer->name,layer->mask);
      msFreeImage(image);
      msFree(bandlist);
      msWCSClearCoverageMetadata20(&cm);
      msDrawRasterLayerLowCloseDataset(layer, hDS);
      return msWCSException(map, NULL, NULL, params->version);
    }
    maskLayer = GET_LAYER(map, maskLayerIdx);
    if(!maskLayer->maskimage) {
      int i,retcode;
      int origstatus, origlabelcache;
      char *origImageType = msStrdup(map->imagetype);
      altFormat =  msSelectOutputFormat(map, "png24");
      msInitializeRendererVTable(altFormat);
      /* TODO: check the png24 format hasn't been tampered with, i.e. it's agg */
      maskLayer->maskimage= msImageCreate(image->width, image->height,altFormat,
                                          image->imagepath, image->imageurl, map->resolution, map->defresolution, NULL);
      if (!maskLayer->maskimage) {
        msSetError(MS_MISCERR, "Unable to initialize mask image.", "msDrawLayer()");
        msFreeImage(image);
        msFree(bandlist);
        msWCSClearCoverageMetadata20(&cm);
        msDrawRasterLayerLowCloseDataset(layer, hDS);
        return msWCSException(map, NULL, NULL, params->version);
      }

      /*
       * force the masked layer to status on, and turn off the labelcache so that
       * eventual labels are added to the temporary image instead of being added
       * to the labelcache
       */
      origstatus = maskLayer->status;
      origlabelcache = maskLayer->labelcache;
      maskLayer->status = MS_ON;
      maskLayer->labelcache = MS_OFF;

      /* draw the mask layer in the temporary image */
      retcode = msDrawLayer(map, maskLayer, maskLayer->maskimage);
      maskLayer->status = origstatus;
      maskLayer->labelcache = origlabelcache;
      if(retcode != MS_SUCCESS) {
        free(origImageType);
        msFreeImage(image);
        msFree(bandlist);
        msWCSClearCoverageMetadata20(&cm);
        msDrawRasterLayerLowCloseDataset(layer, hDS);
        return msWCSException(map, NULL, NULL, params->version);
      }
      /*
       * hack to work around bug #3834: if we have use an alternate renderer, the symbolset may contain
       * symbols that reference it. We want to remove those references before the altFormat is destroyed
       * to avoid a segfault and/or a leak, and so the the main renderer doesn't pick the cache up thinking
       * it's for him.
       */
      for(i=0; i<map->symbolset.numsymbols; i++) {
        if (map->symbolset.symbol[i]!=NULL) {
          symbolObj *s = map->symbolset.symbol[i];
          if(s->renderer == MS_IMAGE_RENDERER(maskLayer->maskimage)) {
            MS_IMAGE_RENDERER(maskLayer->maskimage)->freeSymbol(s);
            s->renderer = NULL;
          }
        }
      }
      /* set the imagetype from the original outputformat back (it was removed by msSelectOutputFormat() */
      msFree(map->imagetype);
      map->imagetype = origImageType;

    }
  }

  /* Actually produce the "grid". */
  if( MS_RENDERER_RAWDATA(map->outputformat) ) {
    if( doDrawRasterLayerDraw ) {
      status = msDrawRasterLayerLowWithDataset( map, layer, image, NULL, hDS );
    } else {
      status = MS_SUCCESS;
    }
  } else {
    rasterBufferObj rb;
    status = MS_IMAGE_RENDERER(image)->getRasterBufferHandle(image,&rb);
    if(MS_LIKELY(status == MS_SUCCESS)) {
      if( doDrawRasterLayerDraw ) {
        status = msDrawRasterLayerLowWithDataset( map, layer, image, &rb, hDS );
      } else {
        status = MS_SUCCESS;
      }
    }
  }

  msDrawRasterLayerLowCloseDataset(layer, hDS);

  if( status != MS_SUCCESS ) {
    msFree(bandlist);
    msFreeImage(image);
    msWCSClearCoverageMetadata20(&cm);
    return msWCSException(map, NULL, NULL, params->version );
  }

  /* GML+Image */
  /* Embed the image into multipart message */
  if(params->multipart == MS_TRUE) {
    xmlDocPtr psDoc = NULL;       /* document pointer */
    xmlNodePtr psRootNode, psRangeSet, psFile, psRangeParameters;
    xmlNsPtr psGmlNs = NULL,
             psGmlcovNs = NULL,
             psSweNs = NULL,
             psXLinkNs = NULL;
    wcs20coverageMetadataObj tmpCm;
    char *srs_uri, *default_filename;
    const char *filename;
    int swapAxes;

    /* Create Document  */
    psDoc = xmlNewDoc(BAD_CAST "1.0");
    psRootNode = xmlNewNode(NULL, BAD_CAST MS_WCS_GML_COVERAGETYPE_RECTIFIED_GRID_COVERAGE);
    xmlDocSetRootElement(psDoc, psRootNode);

    msWCSPrepareNamespaces20(psDoc, psRootNode, map, MS_FALSE);

    psGmlNs    = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX);
    psGmlcovNs = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_GMLCOV_NAMESPACE_PREFIX);
    psSweNs    = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_SWE_NAMESPACE_PREFIX);
    xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_WCS_NAMESPACE_PREFIX);
    psXLinkNs  = xmlSearchNs(psDoc, psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);

    xmlNewNsProp(psRootNode, psGmlNs, BAD_CAST "id", BAD_CAST layer->name);

    xmlSetNs(psRootNode, psGmlcovNs);

    srs_uri = msOWSGetProjURI(&map->projection, NULL, "CO", 1);

    tmpCm = cm;
    tmpCm.extent = map->extent;
    tmpCm.xsize = map->width;
    tmpCm.ysize = map->height;
    strlcpy(tmpCm.srs_uri, srs_uri, sizeof(tmpCm.srs_uri));

    tmpCm.xresolution = map->gt.geotransform[1];
    tmpCm.yresolution = map->gt.geotransform[5];

    tmpCm.extent.minx = MS_MIN(map->gt.geotransform[0], map->gt.geotransform[0] + map->width * tmpCm.xresolution);
    tmpCm.extent.miny = MS_MIN(map->gt.geotransform[3], map->gt.geotransform[3] + map->height * tmpCm.yresolution);
    tmpCm.extent.maxx = MS_MAX(map->gt.geotransform[0], map->gt.geotransform[0] + map->width * tmpCm.xresolution);
    tmpCm.extent.maxy = MS_MAX(map->gt.geotransform[3], map->gt.geotransform[3] + map->height * tmpCm.yresolution);

    swapAxes = msWCSSwapAxes20(srs_uri);
    msFree(srs_uri);

    /* Setup layer information  */
    msWCSCommon20_CreateBoundedBy(&tmpCm, psGmlNs, psRootNode, &(map->projection), swapAxes);
    msWCSCommon20_CreateDomainSet(layer, &tmpCm, psGmlNs, psRootNode, &(map->projection), swapAxes);

    psRangeSet = xmlNewChild(psRootNode, psGmlNs, BAD_CAST "rangeSet", NULL);
    psFile     = xmlNewChild(psRangeSet, psGmlNs, BAD_CAST "File", NULL);

    /* TODO: wait for updated specifications */
    psRangeParameters = xmlNewChild(psFile, psGmlNs, BAD_CAST "rangeParameters", NULL);

    default_filename = msStrdup("out.");
    default_filename = msStringConcatenate(default_filename, MS_IMAGE_EXTENSION(image->format));

    filename = msGetOutputFormatOption(image->format, "FILENAME", default_filename);
    std::string file_ref("cid:coverage/");
    file_ref += filename;

    msFree(default_filename);

    std::string role;
    if(EQUAL(MS_IMAGE_MIME_TYPE(map->outputformat), "image/tiff")) {
      role = MS_WCS_20_PROFILE_GML_GEOTIFF;
    } else {
      role = MS_IMAGE_MIME_TYPE(map->outputformat);
    }

    xmlNewNsProp(psRangeParameters, psXLinkNs, BAD_CAST "href", BAD_CAST file_ref.c_str());
    xmlNewNsProp(psRangeParameters, psXLinkNs, BAD_CAST "role", BAD_CAST role.c_str());
    xmlNewNsProp(psRangeParameters, psXLinkNs, BAD_CAST "arcrole", BAD_CAST "fileReference");

    xmlNewChild(psFile, psGmlNs, BAD_CAST "fileReference", BAD_CAST file_ref.c_str());
    xmlNewChild(psFile, psGmlNs, BAD_CAST "fileStructure", NULL);
    xmlNewChild(psFile, psGmlNs, BAD_CAST "mimeType", BAD_CAST MS_IMAGE_MIME_TYPE(map->outputformat));

    msWCSCommon20_CreateRangeType(&cm, bandlist, psGmlcovNs, psSweNs, psRootNode);

    msIO_setHeader("Content-Type","multipart/related; boundary=wcs");
    msIO_sendHeaders();
    msIO_printf("\r\n--wcs\r\n");

    msWCSWriteDocument20(psDoc);
    msWCSWriteFile20(map, image, params, 1);

    xmlFreeDoc(psDoc);
    xmlCleanupParser();
  /* just print out the file without gml */
  } else {
    msWCSWriteFile20(map, image, params, 0);
  }

  msFree(bandlist);
  msWCSClearCoverageMetadata20(&cm);
  msFreeImage(image);
  return MS_SUCCESS;
}

#endif /* defined(USE_LIBXML2) */

#endif /* defined(USE_WCS_SVR) */
