/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  libxml2 convenience wrapper functions include file
 * Author:   Tom Kralidis (tomkralidis@hotmail.com)
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

#ifndef MAPLIBXML2_H
#define MAPLIBXML2_H

#ifdef USE_LIBXML2

#include<libxml/parser.h>
#include<libxml/tree.h>
#include<libxml/xpath.h>
#include<libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>

xmlXPathObjectPtr msLibXml2GetXPath(xmlDocPtr doc, xmlXPathContextPtr context, xmlChar *xpath);

void msLibXml2GenerateList(xmlNodePtr psParent, xmlNsPtr psNs, const char *elname, const char *values, char delim);

char *msLibXml2GetXPathTree(xmlDocPtr doc, xmlXPathObjectPtr xpath);

#endif /* defined(USE_LIBXML2) */

#endif /* MAPLIBXML2_H */
