/******************************************************************************
 * $id: mapfile.c 7854 2008-08-14 19:22:48Z dmorissette $
 *
 * Project:  MapServer
 * Purpose:  High level Map file parsing code.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/
#include "mapserver.h"

#ifdef USE_XMLMAPFILE

#include <libxslt/xslt.h>
#include "libexslt/exslt.h"
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>
#include <libxslt/xsltutils.h>

extern int xmlLoadExtDtdDefaultValue;

int msTransformXmlMapfile(const char *stylesheet, const char *xmlMapfile, FILE *tmpfile)
{
  xsltStylesheetPtr cur = NULL;
  int status = MS_FAILURE;
  xmlDocPtr doc = NULL, res = NULL;

  exsltRegisterAll();
  xsltRegisterTestModule();

  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 1;

  cur = xsltParseStylesheetFile((const xmlChar *)stylesheet);
  if (cur == NULL) {
    msSetError(MS_MISCERR, "Failed to load xslt stylesheet", "msTransformXmlMapfile()");
    goto done;
  }

  doc = xmlParseFile(xmlMapfile);
  if (doc == NULL) {
    msSetError(MS_MISCERR, "Failed to load xml mapfile", "msTransformXmlMapfile()");
    goto done;
  }

  res = xsltApplyStylesheet(cur, doc, NULL);
  if (res == NULL) {
    msSetError(MS_MISCERR, "Failed to apply style sheet to %s", "msTransformXmlMapfile()", xmlMapfile);
    goto done;
  }

  if ( xsltSaveResultToFile(tmpfile, res, cur) != -1 )
    status =  MS_SUCCESS;

done:
  if (cur)
    xsltFreeStylesheet(cur);
  if (res)
    xmlFreeDoc(res);
  if (doc)
    xmlFreeDoc(doc);

  xsltCleanupGlobals();
  xmlCleanupParser();

  return status;
}

#endif
