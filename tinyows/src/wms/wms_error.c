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


static char *wms_error_code_string(enum wms_error_code code)
{
	switch (code)
	{
	case WMS_ERROR_INVALID_FORMAT:
		return "InvalidFormat";
	case WMS_ERROR_INVALID_CRS:
		return "InvalidCRS";
	case WMS_ERROR_LAYER_NOT_DEFINED:
		return "LayerNotDefined";
	case WMS_ERROR_STYLE_NOT_DEFINED:
		return "StyleNotDefined";
	case WMS_ERROR_LAYER_NOT_QUERYABLE:
		return "LayerNotQueryable";
	case WMS_ERROR_INVALID_POINT:
		return "InvalidPoint";
	case WMS_ERROR_CURRENT_UPDATE_SEQUENCE:
		return "CurrentUpdateSequence";
	case WMS_ERROR_INVALID_UPDATE_SEQUENCE:
		return "InvalidUpdateSequence";
	case WMS_ERROR_MISSING_DIMENSION_VALUE:
		return "MissingDimensionValue";
	case WMS_ERROR_INVALID_DIMENSION_VALUE:
		return "InvalidDimensionValue";
	case WMS_ERROR_OPERATION_NOT_SUPPORTED:
		return "OperationNotSupported";
	case WMS_ERROR_INVALID_VERSION:
		return "InvalidVersion";
	case WMS_ERROR_INVALID_BBOX:
		return "InvalidBbox";
	case WMS_ERROR_INVALID_WIDTH:
		return "InvalidWidth";
	case WMS_ERROR_INVALID_HEIGHT:
		return "InvalidHeight";
	}

	return "";					/* Only to avoid gcc warning */
}


/* TODO what about 1.1.1 exception ? */
static void wms_error_xml(ows * o, enum wms_error_code code, char *message)
{

	fprintf(o->output, "<?xml version='1.0' encoding='UTF-8'?>\n");
	fprintf(o->output, "<ServiceExceptionReport version='1.3.0'\n");
	fprintf(o->output, " xmlns='http://www.opengis.net/ogc'\n");
	fprintf(o->output,
	   " xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n");
	fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/ogc");
	printf
	   (" http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd'>\n");
	fprintf(o->output,
	   " <ServiceException code='%s'>%s</ServiceException>\n",
	   wms_error_code_string(code), message);
	fprintf(o->output, "</ServiceExceptionReport>\n");
}


static void wms_error_inimage(ows * o, wms_request * wr, char *message)
{
	/* TODO */
}


static void wms_error_blank(ows * o, wms_request * wr)
{
	/* TODO */
}


void wms_error(ows * o, wms_request * wr, enum wms_error_code code,
   char *message)
{
	switch (wr->exception)
	{

	case WMS_EXCEPTION_UNKNOWN:
	case WMS_EXCEPTION_XML:
		wms_error_xml(o, code, message);
		break;
	case WMS_EXCEPTION_INIMAGE:
		wms_error_inimage(o, wr, message);
		break;
	case WMS_EXCEPTION_BLANK:
		wms_error_blank(o, wr);
		break;
	}

	wms_request_free(wr);
	ows_free(o);

	exit(EXIT_SUCCESS);
}


/*
 * vim: expandtab sw=4 ts=4 
 */
