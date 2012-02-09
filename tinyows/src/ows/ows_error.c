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

#include "ows.h"


/*
 * Transform an error code into an error message 
 */
static char *ows_error_code_string(enum ows_error_code code)
{
	switch (code)
	{
	case OWS_ERROR_OPERATION_NOT_SUPPORTED:
		return "OperationNotSupported";
	case OWS_ERROR_MISSING_PARAMETER_VALUE:
		return "MissingParameterValue";
	case OWS_ERROR_INVALID_PARAMETER_VALUE:
		return "InvalidParameterValue";
	case OWS_ERROR_VERSION_NEGOTIATION_FAILED:
		return "VersionNegotiationFailed";
	case OWS_ERROR_INVALID_UPDATE_SEQUENCE:
		return "InvalidUpdateSequence";
	case OWS_ERROR_NO_APPLICABLE_CODE:
		return "NoApplicableCode";
	case OWS_ERROR_CONNECTION_FAILED:
		return "ConnectionFailed";
	case OWS_ERROR_CONFIG_FILE:
		return "ErrorConfigFile";
	case OWS_ERROR_REQUEST_SQL_FAILED:
		return "RequestSqlFailed";
	case OWS_ERROR_REQUEST_HTTP:
		return "RequestHTTPNotValid";
	case OWS_ERROR_FORBIDDEN_CHARACTER:
		return "ForbiddenCharacter";
	case OWS_ERROR_MISSING_METADATA:
		return "MissingMetadata";
	case OWS_ERROR_NO_SRS_DEFINED:
		return "NoSrsDefined";
	}

	assert(0); /* Should not happen */
}


/* 
 * Return an ExceptionReport as specified in OWS 1.1.0 specification
 */
void ows_error(ows * o, enum ows_error_code code, char *message,
   char *locator)
{
	assert(o != NULL);
	assert(message != NULL);
	assert(locator != NULL);

	fprintf(o->output, "Content-Type: application/xml\n\n");
	fprintf(o->output, "<?xml version='1.0' encoding='UTF-8'?>\n");
	fprintf(o->output, "<ExceptionReport\n");
	fprintf(o->output, " xmlns='http://www.opengis.net/ows'\n");
	fprintf(o->output, " xmlns:ows='http://www.opengis.net/ows'\n");
	fprintf(o->output,
	   " xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n");
	fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/ows");
	fprintf(o->output,
	   " http://schemas.opengis.net/ows/1.0.0/owsExceptionReport.xsd'\n");
	fprintf(o->output, " version='1.1.0' language='en'>\n");
	fprintf(o->output,
	   " <Exception exceptionCode='%s' locator='%s'>\n",
	   ows_error_code_string(code), locator);
	fprintf(o->output, "  <ExceptionText>%s</ExceptionText>\n", message);
	fprintf(o->output, " </Exception>\n");
	fprintf(o->output, "</ExceptionReport>\n");

	ows_free(o);

	exit(EXIT_SUCCESS);
}


/*
 * vim: expandtab sw=4 ts=4
 */
