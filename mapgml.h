#ifndef MAPGML_H
#define MAPGML_H

#ifdef USE_SOS_SVR
#include<libxml/parser.h>
#include<libxml/tree.h>

#define MS_GML_NAMESPACE_URI      "http://www.opengis.net/gml"
#define MS_GML_NAMESPACE_PREFIX   "gml"

xmlNodePtr msGML3BoundedBy(xmlNsPtr psNs, double minx, double miny, double maxx, double maxy, const char *psEpsg);
xmlNodePtr msGML3TimePeriod(xmlNsPtr psNs, char *pszStart, char *pszEnd);
xmlNodePtr msGML3TimeInstant(xmlNsPtr psNs, char *timeInstant);
xmlNodePtr msGML3Point(xmlNsPtr psNs, const char *psSrsName, const char *id, double x, double y);

#endif /* USE_SOS_SVR */

#endif
