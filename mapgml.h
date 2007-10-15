#ifndef MAPGML_H
#define MAPGML_H

#ifdef USE_SOS_SVR
#include<libxml/parser.h>
#include<libxml/tree.h>

xmlNodePtr msGML3BoundedBy(xmlNodePtr psParent, double minx, double miny, double maxx, double maxy, const char *psEpsg, int dimemsion);
xmlNodePtr msGML3TimePeriod(xmlNodePtr psNode, char *pszStart, char *pszEnd);

#endif /* USE_SOS_SVR */

#endif
