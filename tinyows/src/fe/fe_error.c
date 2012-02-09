/* 
  Copyright (c) <2007> <Barbara Philippot - Olivier Courtin>

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
	assert(o != NULL);
	assert(fe != NULL);

	if (fe->error_code == FE_ERROR_FEATUREID)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "featureid must match layer.id", "FILTER");
	}
	else if (fe->error_code == FE_ERROR_FILTER)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "filter parameter doesn't validate the filter.xsd schema. Check your xml",
		   "FILTER");
	}
	else if (fe->error_code == FE_ERROR_BBOX)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "bbox must match xmin,ymin,xmax,ymax", "FILTER");
	}
	else if (fe->error_code == FE_ERROR_PROPERTYNAME)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "PropertyName not available, execute a DescribeFeaturetype request to see the available properties",
		   "FILTER");
	}
	else if (fe->error_code == FE_ERROR_UNITS)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "units not supported, use 'meters' or 'kilometers'", "FILTER");
	}
	else if (fe->error_code == FE_ERROR_GEOMETRY)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "bad geometry", "FILTER");
	}
	else if (fe->error_code == FE_ERROR_FID)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "only one type of identifiers is allowed (FeatureId or GmlObjectId)",
		   "FILTER");
	}
	else if (fe->error_code == FE_ERROR_SRS)
	{
		filter_encoding_free(fe);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "srsName isn't valid", "srsName");
	}
}


/* 
 * Fill a buffer with a human understable message
 */
buffer *fill_fe_error(ows * o, filter_encoding * fe)
{
	buffer *result;

	assert(o != NULL);
	assert(fe != NULL);

	result = buffer_init();

	if (fe->error_code == FE_ERROR_FEATUREID)
	{
		buffer_add_str(result, "featureid must match layer.id");
	}
	else if (fe->error_code == FE_ERROR_FILTER)
	{
		buffer_add_str(result,
		   "filter parameter doesn't validate the filter.xsd schema. Check your xml");
	}
	else if (fe->error_code == FE_ERROR_BBOX)
	{
		buffer_add_str(result, "bbox must match xmin,ymin,xmax,ymax");
	}
	else if (fe->error_code == FE_ERROR_PROPERTYNAME)
	{
		buffer_add_str(result,
		   "PropertyName not available, execute a DescribeFeaturetype request to see the available properties");
	}
	else if (fe->error_code == FE_ERROR_UNITS)
	{
		buffer_add_str(result,
		   "units not supported, use 'meters' or 'kilometers'");
	}
	else if (fe->error_code == FE_ERROR_GEOMETRY)
	{
		buffer_add_str(result, "bad geometry");
	}
	else if (fe->error_code == FE_ERROR_FID)
	{
		buffer_add_str(result,
		   "only one type of identifiers is allowed (FeatureId or GmlObjectId)");
	}
	else if (fe->error_code == FE_ERROR_SRS)
	{
		buffer_add_str(result, "srsName isn't valid");
	}

	return result;
}


/*
 * vim: expandtab sw=4 ts=4
 */
