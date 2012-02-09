/*
  Copyright (c) <2007-2012> <Barbara Philippot - Olivier Courtin>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "../ows/ows.h"


/*
 * Call ows_error with the matching message
 */
void fe_error(ows * o, filter_encoding * fe)
{
    assert(o);
    assert(fe);

    if (fe->error_code == FE_ERROR_FEATUREID) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Featureid must match layer.id", "FILTER");
    } else if (fe->error_code == FE_ERROR_FILTER) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Filter parameter doesn't validate WFS Schema", "FILTER");
    } else if (fe->error_code == FE_ERROR_BBOX) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Bbox must match xmin,ymin,xmax,ymax", "FILTER");
    } else if (fe->error_code == FE_ERROR_PROPERTYNAME) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "PropertyName not available", "FILTER");
    } else if (fe->error_code == FE_ERROR_GEOM_PROPERTYNAME) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Geometry PropertyName not available", "FILTER");
    } else if (fe->error_code == FE_ERROR_UNITS) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Units not supported, use 'meters' or 'kilometers'", "FILTER");
    } else if (fe->error_code == FE_ERROR_GEOMETRY) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Bad geometry", "FILTER");
    } else if (fe->error_code == FE_ERROR_FID) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Only one allowed at once among FeatureId and GmlObjectId)", "FILTER");
    } else if (fe->error_code == FE_ERROR_SRS) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "SrsName isn't valid", "FILTER");
    } else if (fe->error_code == FE_ERROR_FUNCTION) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Unknown Function Name used in Filter", "FILTER");
    } else if (fe->error_code == FE_ERROR_NAMESPACE) {
        filter_encoding_free(fe);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Filter Element contains incoherent XML Namespaces", "FILTER");
    }
}


/*
 * Fill a buffer with a human understable message
 */
buffer *fill_fe_error(ows * o, filter_encoding * fe)
{
    buffer *result;

    assert(o);
    assert(fe);

    result = buffer_init();

    if (fe->error_code == FE_ERROR_FEATUREID)
        buffer_add_str(result, "Featureid must match layer.id");
    else if (fe->error_code == FE_ERROR_FILTER)
        buffer_add_str(result, "Filter parameter doesn't validate the filter.xsd schema. Check your xml");
    else if (fe->error_code == FE_ERROR_BBOX)
        buffer_add_str(result, "Bbox must match xmin,ymin,xmax,ymax");
    else if (fe->error_code == FE_ERROR_PROPERTYNAME)
        buffer_add_str(result, "PropertyName not available");
    else if (fe->error_code == FE_ERROR_GEOM_PROPERTYNAME)
        buffer_add_str(result, "Geometry PropertyName not available");
    else if (fe->error_code == FE_ERROR_UNITS)
        buffer_add_str(result, "Units not supported, use 'meters' or 'kilometers'");
    else if (fe->error_code == FE_ERROR_GEOMETRY)
        buffer_add_str(result, "Bad geometry");
    else if (fe->error_code == FE_ERROR_FID)
        buffer_add_str(result, "Only one type of identifier allowed (FeatureId or GmlObjectId)");
    else if (fe->error_code == FE_ERROR_SRS)
        buffer_add_str(result, "SrsName isn't valid");

    return result;
}


/*
 * vim: expandtab sw=4 ts=4
 */
