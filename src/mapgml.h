/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Headers for mapgml.c.  shapeObj to GML output via MapServer
 *           queries.
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


#ifndef MAPGML_H
#define MAPGML_H

#ifdef USE_LIBXML2
#include<libxml/parser.h>
#include<libxml/tree.h>

xmlNodePtr msGML3BoundedBy(xmlNsPtr psNs, double minx, double miny, double maxx, double maxy, const char *psEpsg);
xmlNodePtr msGML3TimePeriod(xmlNsPtr psNs, char *pszStart, char *pszEnd);
xmlNodePtr msGML3TimeInstant(xmlNsPtr psNs, char *timeInstant);
xmlNodePtr msGML3Point(xmlNsPtr psNs, const char *psSrsName, const char *id, double x, double y);

#endif /* USE_LIBXML2 */

#endif
