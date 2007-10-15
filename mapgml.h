#ifndef MAPGML_H
#define MAPGML_H

#ifdef USE_SOS_SVR
#include<libxml/parser.h>
#include<libxml/tree.h>

#define MS_GML_NAMESPACE_URI      "http://www.opengis.net/gml"
#define MS_GML_NAMESPACE_PREFIX   "gml"

xmlNodePtr msGML3BoundedBy(double minx, double miny, double maxx, double maxy, const char *psEpsg);
xmlNodePtr msGML3TimePeriod(char *pszStart, char *pszEnd);
xmlNodePtr msGML3TimeInstant(char *timeInstant);
xmlNodePtr msGML3Point(const char *psSrsName, const char *id, double x, double y);

#endif /* USE_SOS_SVR */

#endif
