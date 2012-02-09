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
 * Transform an error code into an error message
 */
static char *wfs_error_code_string(enum wfs_error_code code)
{
    switch (code) {
        case WFS_ERROR_INVALID_VERSION:
            return "InvalidVersion";
        case WFS_ERROR_OUTPUT_FORMAT_NOT_SUPPORTED:
            return "OutputFormatNotSupported";
        case WFS_ERROR_LAYER_NOT_DEFINED:
            return "LayerNotDefined";
        case WFS_ERROR_EXCLUSIVE_PARAMETERS:
            return "ExclusiveParameters";
        case WFS_ERROR_LAYER_NOT_RETRIEVABLE:
            return "LayerNotRetrievable";
        case WFS_ERROR_LAYER_NOT_WRITABLE:
            return "LayerNotWritable";
        case WFS_ERROR_INCORRECT_SIZE_PARAMETER:
            return "IncorrectSizeParameter";
        case WFS_ERROR_NO_MATCHING:
            return "NoMatching";
        case WFS_ERROR_INVALID_PARAMETER:
            return "InvalidParameterValue";
        case WFS_ERROR_MISSING_PARAMETER:
            return "MissingParameterValue";
    }

    assert(0); /* Should not happen */
}


/*
 * Return a ServiceExceptionReport as specified in WFS 1.0.0 specification
 */
static void wfs_error_100(ows * o, wfs_request * wf, enum wfs_error_code code, char *message, char *locator)
{
    assert(o);
    assert(wf);
    assert(message);
    assert(locator);

    assert(!o->exit);
    o->exit = true;

    ows_log(o, 1, message);

    fprintf(o->output, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", o->encoding->buf);
    fprintf(o->output, "<ServiceExceptionReport\n");
    fprintf(o->output, " xmlns=\"http://www.opengis.net/ogc\"\n");
    fprintf(o->output, " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(o->output, " xsi:schemaLocation=\"http://www.opengis.net/ogc");
    fprintf(o->output, " http://schemas.opengis.net/wms/1.1.1/OGC-exception.xsd\"\n");
    fprintf(o->output, "version=\"1.2.0\">\n");
    fprintf(o->output, "<ServiceException code=\"%s\"", wfs_error_code_string(code));
    fprintf(o->output, " locator=\"%s\">\n%s", locator, message);
    fprintf(o->output, "</ServiceException>\n");
    fprintf(o->output, "</ServiceExceptionReport>\n");
}


/*
 * Return an ExceptionReport as specified in WFS 1.1.0 specification
 */
static void wfs_error_110(ows * o, wfs_request * wf, enum wfs_error_code code, char *message, char *locator)
{
    assert(o);
    assert(wf);
    assert(message);
    assert(locator);

    assert(!o->exit);
    o->exit = true;

    ows_log(o, 1, message);

    fprintf(o->output, "<?xml version='1.0' encoding='UTF-8'?>\n");
    fprintf(o->output, "<ExceptionReport\n");
    fprintf(o->output, " xmlns='http://www.opengis.net/ows'\n");
    fprintf(o->output, " xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n");
    fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/ows");
    fprintf(o->output, " http://schemas.opengis.net/ows/1.0.0/owsExceptionReport.xsd'\n");
    fprintf(o->output, " version='1.0.0' language='en'>\n");
    fprintf(o->output, " <Exception exceptionCode='%s' locator='%s'>\n", wfs_error_code_string(code), locator);
    fprintf(o->output, "  <ExceptionText>%s</ExceptionText>\n", message);
    fprintf(o->output, " </Exception>\n");
    fprintf(o->output, "</ExceptionReport>\n");
}


/*
 * Call the right function according to version
 */
void wfs_error(ows * o, wfs_request * wf, enum wfs_error_code code, char *message, char *locator)
{
    int version;

    assert(o);
    assert(wf);
    assert(message);
    assert(locator);

    version = ows_version_get(o->request->version);
    fprintf(o->output, "Content-Type: application/xml\n\n");

    switch (version) {
        case 100:
            wfs_error_100(o, wf, code, message, locator);
            break;
        case 110:
            wfs_error_110(o, wf, code, message, locator);
            break;
    }
}


/*
 * vim: expandtab sw=4 ts=4
 */
