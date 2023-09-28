/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  libxml2 convenience wrapper functions
 * Author:   Tom Kralidis (tomkralidis@gmail.com)
 *
 ******************************************************************************
 * Copyright (c) 2007, Tom Kralidis
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

#ifdef USE_LIBXML2

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

/**
 * msLibXml2GenerateList()
 *
 * Convenience function to produce a series of XML elements from a delimited
 * list
 *
 * @param xmlNodePtr psParent the encompassing node
 * @param xmlNsPtr psNs the namespace object
 * @param const char *elname the list member element name
 * @param const char *values the list member element values
 * @param char delim the delimiter
 *
 */

void msLibXml2GenerateList(xmlNodePtr psParent, xmlNsPtr psNs,
                           const char *elname, const char *values, char delim) {
  char **tokens = NULL;
  int n = 0;
  int i = 0;
  tokens = msStringSplit(values, delim, &n);
  for (i = 0; i < n; i++) {
    // Not sure we really need to distinguish empty vs non-empty case, but
    // this does change the result of
    // msautotest/wxs/expected/wcs_empty_cap111.xml otherwise
    if (tokens[i] && tokens[i][0] != '\0')
      xmlNewTextChild(psParent, psNs, BAD_CAST elname, BAD_CAST tokens[i]);
    else
      xmlNewChild(psParent, psNs, BAD_CAST elname, BAD_CAST tokens[i]);
  }
  msFreeCharArray(tokens, n);
}

/**
 * msLibXml2GetXPath()
 *
 * Convenience function to fetch an XPath
 *
 * @param xmlDocPtr doc the XML doc pointer
 * @param xmlXPathContextPtr the context pointer
 * @param xmlChar *xpath the xpath value
 *
 * @return result xmlXPathObjectPtr pointer
 *
 */

xmlXPathObjectPtr msLibXml2GetXPath(xmlDocPtr doc, xmlXPathContextPtr context,
                                    xmlChar *xpath) {
  (void)doc;
  xmlXPathObjectPtr result;
  result = xmlXPathEval(xpath, context);
  if (result == NULL) {
    return NULL;
  }
  if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
    xmlXPathFreeObject(result);
    return NULL;
  }
  return result;
}

/**
 * msLibXml2GetXPathTree
 *
 * Convenience function to fetch an XPath and children
 *
 * @param xmlDocPtr doc the XML doc pointer
 * @param xmlXPathObjectPtr the xpath object
 *
 * @return result string
 *
 */

char *msLibXml2GetXPathTree(xmlDocPtr doc, xmlXPathObjectPtr xpath) {
  xmlBufferPtr xbuf;
  char *result = NULL;

  xbuf = xmlBufferCreate();

  if (xpath) {
    if (xmlNodeDump(xbuf, doc, xpath->nodesetval->nodeTab[0], 0, 0) == -1) {
      return NULL;
    }
    result = msStrdup((char *)xbuf->content);
  }
  xmlBufferFree(xbuf);
  return result;
}

#endif /* defined(USE_LIBXML2) */
