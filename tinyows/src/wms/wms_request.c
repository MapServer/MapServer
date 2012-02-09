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
#include <string.h>
#include <ctype.h>

#include "../ows/ows.h"


/*
 * FIXME version and service --> ows_request !!!
 */


/*
 * Initialize wms_request struct 
 */
wms_request *wms_request_init()
{
	wms_request *wr;

	wr = malloc(sizeof(wms_request));
	assert(wr != NULL);

	wr->request = WMS_REQUEST_UNKNOWN;
	wr->width = 0;
	wr->height = 0;
	wr->format = WMS_FORMAT_UNKNOWN;
	wr->exception = WMS_EXCEPTION_XML;
	wr->layers = NULL;
	wr->styles = NULL;
	wr->bbox = NULL;
	wr->srs = NULL;
	wr->request_i = 0;
	wr->request_j = 0;
	wr->feature_count = 0;
	wr->transparent = false;
	wr->bgcolor = NULL;

	return wr;
}


#ifdef OWS_DEBUG
void wms_request_flush(wms_request * wr, FILE * output)
{
	fprintf(output, "[\n");

	fprintf(output, " request -> %i\n", wr->request);
	fprintf(output, " width -> %i\n", wr->width);
	fprintf(output, " height -> %i\n", wr->height);
	fprintf(output, " format -> %i\n", wr->format);
	fprintf(output, " exception -> %i\n", wr->exception);

	if (wr->layers != NULL)
	{
		fprintf(output, " layers -> ");
		list_flush(wr->layers, output);
		fprintf(output, "\n");
	}
	if (wr->styles != NULL)
	{
		fprintf(output, "styles -> ");
		list_flush(wr->styles, output);
		fprintf(output, "\n");
	}
	if (wr->bbox != NULL)
	{
		fprintf(output, "bbox -> ");
		ows_bbox_flush(wr->bbox, output);
		fprintf(output, "\n");
	}
	if (wr->srs != NULL)
	{
		fprintf(output, "srs -> ");
		ows_srs_flush(wr->srs, output);
		fprintf(output, "\n");
	}
	fprintf(output, " request_i -> %i\n", wr->request_i);
	fprintf(output, " request_j -> %i\n", wr->request_j);
	fprintf(output, " feature_count -> %i\n", wr->feature_count);
	fprintf(output, " transparent -> %i\n", wr->transparent ? 1 : 0);
	if (wr->bgcolor != NULL)
	{
		fprintf(output, " bgcolor -> ");
		buffer_flush(wr->bgcolor, output);
		fprintf(output, "\n");
	}
	fprintf(output, "]\n");
}
#endif


/*
 * Release wms_request struct 
 */
void wms_request_free(wms_request * wr)
{
	assert(wr != NULL);

	if (wr->layers != NULL)
		list_free(wr->layers);

	if (wr->styles != NULL)
		list_free(wr->styles);

	if (wr->bbox != NULL)
		ows_bbox_free(wr->bbox);

	if (wr->srs != NULL)
		ows_srs_free(wr->srs);

	if (wr->bgcolor != NULL)
		buffer_free(wr->bgcolor);

	free(wr);
	wr = NULL;
}


/*
 * Check version is 1.3.0 or 1.1.0
 */
static ows_version *wms_request_check_version(ows * o, wms_request * wr,
   const array * cgi)
{
	if (ows_version_get(wr->version) != 111
	   && ows_version_get(wr->version) != 130)
		wms_error(o, wr, WMS_ERROR_INVALID_VERSION,
		   "VERSION parameter is not valid (use 1.3.0 or 1.1.1)");

	return wr->version;
}


/*
 * Check and fill all WMS get_capabilities parameter
 */
static void wms_request_check_get_capabilities(ows * o, wms_request * wr,
   const array * cgi)
{
	/*if key version is not set, version = higher version */
	if (!array_is_key(cgi, "version"))
	{
		ows_version_set(wr->version, 1, 3, 0);
	}
	else
	{
		if (ows_version_get(wr->version) < 130)
			ows_version_set(wr->version, 1, 1, 1);
		if (ows_version_get(wr->version) > 130)
			ows_version_set(wr->version, 1, 3, 0);
	}

	/* FIXME handle UPDATESEQUENCE */
}


/*
 * Check and fill all WMS get_map parameter
 * (See 7.3.2 in WMS 1.3.0)
 * (Assume SERVICE, VERSION and REQUEST already rightly set)
 */
static void wms_request_check_get_map(ows * o, wms_request * wr,
   const array * cgi)
{
	buffer *b;
	list *l;
	int i;

	assert(o != NULL);
	assert(wr != NULL);
	assert(cgi != NULL);


	/* exceptions (optional) (default is WMS_EXCEPTION_XML) */
	/* We check it first, so if something wrong later exception
	 * format asked by client could be used... */
	if (array_is_key(cgi, "exceptions"))
	{
		b = array_get(cgi, "exceptions");

		if (buffer_cmp(b, "application/vnd.ogc.se_inimage"))
			wr->exception = WMS_EXCEPTION_INIMAGE;
		else if (buffer_cmp(b, "application/vnd.ogc.se_blank"))
			wr->exception = WMS_EXCEPTION_BLANK;
	}


	/* layers */
	if (!array_is_key(cgi, "layers"))
		wms_error(o, wr, WMS_ERROR_LAYER_NOT_DEFINED,
		   "LAYERS parameter is not set");

	b = array_get(cgi, "layers");
	l = list_explode(',', b);
	if (l->size == 0)
	{
		list_free(l);
		wms_error(o, wr, WMS_ERROR_LAYER_NOT_DEFINED,
		   "LAYERS parameter is empty");
	}

	if (!ows_layer_list_in_list(o->layers, l))
	{
		list_free(l);
		wms_error(o, wr, WMS_ERROR_LAYER_NOT_DEFINED,
		   "LAYERS parameter contains unknown(s) layer(s)");
	}

	wr->layers = l;


	/* styles */
	if (!array_is_key(cgi, "styles"))
		wms_error(o, wr, WMS_ERROR_STYLE_NOT_DEFINED,
		   "STYLES parameter is not set");

	b = array_get(cgi, "styles");
	l = list_explode(',', b);

	if (l->size > 0 && l->size != wr->layers->size)
	{
		list_free(l);
		wms_error(o, wr, WMS_ERROR_STYLE_NOT_DEFINED,
		   "STYLES parameter not follow LAYERS setting");
	}

	/* FIXME add a check to valid all styles against conf setting */

	wr->styles = l;


	/* crs | srs */
	if (ows_version_get(wr->version) == 130)
	{
		if (!array_is_key(cgi, "crs"))
			wms_error(o, wr, WMS_ERROR_INVALID_CRS,
			   "CRS parameter is not set");

		b = array_get(cgi, "crs");
		l = list_explode(':', b);

		if (l->size != 2)
		{
			list_free(l);
			wms_error(o, wr, WMS_ERROR_INVALID_CRS,
			   "CRS parameter value is not valid");
		}
		/* FIXME add a check to valid if first is alpha and second is int */
		wr->srs = ows_srs_init();
		if (!ows_srs_set(o, wr->srs, l->first->value,
			  atoi(l->last->value->buf)))
		{
			list_free(l);
			wms_error(o, wr, WMS_ERROR_INVALID_CRS,
			   "CRS parameter value is not valid");
		}

		list_free(l);

	}
	else if (ows_version_get(wr->version) == 111)
	{
		if (!array_is_key(cgi, "srs"))
			wms_error(o, wr, WMS_ERROR_INVALID_CRS,
			   "SRS parameter is not set");

		b = array_get(cgi, "srs");
		l = list_explode(':', b);

		if (l->size != 2)
		{
			list_free(l);
			wms_error(o, wr, WMS_ERROR_INVALID_CRS,
			   "SRS parameter value is not valid");
		}
		/* FIXME add a check to valid if first is alpha and second is int */
		wr->srs = ows_srs_init();
		if (!ows_srs_set(o, wr->srs, l->first->value,
			  atoi(l->last->value->buf)))
		{
			list_free(l);
			wms_error(o, wr, WMS_ERROR_INVALID_CRS,
			   "SRS parameter value is not valid");
		}
		list_free(l);
	}


	/* bbox */
	if (!array_is_key(cgi, "bbox"))
		wms_error(o, wr, WMS_ERROR_INVALID_BBOX,
		   "BBOX parameter is not set");

	b = array_get(cgi, "bbox");
	l = list_explode(',', b);

	if (l->size != 4)
	{
		list_free(l);
		wms_error(o, wr, WMS_ERROR_INVALID_BBOX,
		   "BBOX parameter is not valid, must be : xmin, ymin, xmax, ymax");
	}

	/* FIXME Add a check to be sure values are all float */

	wr->bbox = ows_bbox_init();
	if (!ows_bbox_set(o, wr->bbox,
		  atof(l->first->value->buf),
		  atof(l->first->next->value->buf),
		  atof(l->first->next->next->value->buf),
		  atof(l->first->next->next->next->value->buf), wr->srs->srid))
	{
		list_free(l);
		wms_request_free(wr);
		ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
		   "Bad parameters for Bbox, must be Xmin,Ymin,Xmax,Ymax", "NULL");
	}

	list_free(l);


	/* width */
	if (!array_is_key(cgi, "width"))
		wms_error(o, wr, WMS_ERROR_INVALID_WIDTH,
		   "WIDTH parameter is not set");

	/* FIXME Add a check to be sure value is an int */

	b = array_get(cgi, "width");
	i = atoi(b->buf);
	if (i <= 0)
		wms_error(o, wr, WMS_ERROR_INVALID_WIDTH,
		   "WIDTH parameter is not valid");

	wr->width = i;


	/* height */
	if (!array_is_key(cgi, "height"))
		wms_error(o, wr, WMS_ERROR_INVALID_HEIGHT,
		   "HEIGHT parameter is not set");

	/* FIXME Add a check to be sure value is an int */

	b = array_get(cgi, "height");
	i = atoi(b->buf);
	if (i <= 0)
		wms_error(o, wr, WMS_ERROR_INVALID_HEIGHT,
		   "HEIGHT parameter is not valid");

	wr->height = i;


	/* format */
	if (!array_is_key(cgi, "format"))
		wms_error(o, wr, WMS_ERROR_INVALID_FORMAT,
		   "FORMAT parameter is not set");

	b = array_get(cgi, "format");
	if (buffer_cmp(b, "image/svg+xml"))
		wr->format = WMS_FORMAT_SVG;
	else
		wms_error(o, wr, WMS_ERROR_INVALID_FORMAT,
		   "Format parameter is not supported");


	/* transparent (optional) (default is false) */
	if (array_is_key(cgi, "transparent"))
	{
		b = array_get(cgi, "transparent");

		if (buffer_case_cmp(b, "true"))
			wr->transparent = true;

	}


	/* bgcolor (optional) (default is ffffff) */
	if (array_is_key(cgi, "bgcolor"))
	{
		b = array_get(cgi, "bgcolor");

		/* FIXME add a check to rightful pattern 0x[0-9a-f]{6} */
		if (b->use == 8)
		{
			wr->bgcolor = buffer_init();
			buffer_copy(wr->bgcolor, b);
			buffer_shift(wr->bgcolor, 2);	/* remove 0x prefix */
		}
	}


	/* TODO time (optional) */
	/* TODO elevation (optional) */
	/* TODO other sample dimensions (optional) */

}


void wms_request_check(ows * o, wms_request * wr, const array * cgi)
{
	buffer *b;

	assert(o != NULL);
	assert(cgi != NULL);

	b = array_get(cgi, "request");

	if (buffer_cmp(b, "GetCapabilities"))
	{
		wr->request = WMS_GET_CAPABILITIES;
		wms_request_check_get_capabilities(o, wr, cgi);

	}
	else if (buffer_cmp(b, "GetMap"))
	{
		wr->request = WMS_GET_MAP;
		wr->version = wms_request_check_version(o, wr, cgi);
		wms_request_check_get_map(o, wr, cgi);

	}
	else
	{
		wms_error(o, wr, WMS_ERROR_OPERATION_NOT_SUPPORTED,
		   "REQUEST is not supported");
	}

}


void wms(ows * o, wms_request * wr)
{
	assert(o != NULL);
	assert(wr != NULL);

	switch (wr->request)
	{
	case WMS_GET_MAP:
		wms_get_map(o, wr);
		break;
	case WMS_GET_CAPABILITIES:
		wms_get_capabilities(o, wr);
		break;
	default:
		assert(0);				/* Should not happen */
	}
}


/*
 * vim: expandtab sw=4 ts=4 
 */
