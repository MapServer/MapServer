/**********************************************************************
 * $Id$
 *
 * Name:     php_mapscript.c
 * Project:  PHP/MapScript extension for MapServer.
 * Language: ANSI C
 * Purpose:  Header file - prototypes / module definitions
 * Author:   Daniel Morissette, morissette@dmsolutions.ca
 *
 **********************************************************************
 * Copyright (c) 2000-2005, Daniel Morissette, DM Solutions Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log$
 * Revision 1.57  2006/08/22 15:55:03  assefa
 * Adding geos functions to php mapscript (Bug 1327)
 *
 * Revision 1.56  2006/05/29 19:02:01  assefa
 * Update PHP mapscript to support addition of MapScript WxS Services
 * (RFC 16, Bug 1790)
 *
 * Revision 1.55  2006/05/17 16:04:55  assefa
 * Add geos functions union, difference and intersection (Bug 1778)
 *
 * Revision 1.54  2006/01/20 15:03:35  assefa
 * Add containsshape function using uderlying GEOS function (Bug 1623).
 *
 * Revision 1.53  2005/10/13 16:00:36  assefa
 * Add support for whichshape and nextshape (Bug 1491)
 * Adsd support for config setting/getting at the map level (Bug 1487).
 *
 * Revision 1.52  2005/09/08 19:24:50  assefa
 * Expose GEOS operations through PHP Mapscript (Bug 1327).
 * Initially only functions buffer and convexhull on a shape object
 * are available.
 *
 * Revision 1.51  2005/06/14 16:03:36  dan
 * Updated copyright date to 2005
 *
 * Revision 1.50  2005/06/06 05:48:20  dan
 * Added $layerObj->removeClass() (was already in SWIG MapScript)
 * Added $layerObj->removeClass() to PHP MapScript (was already in SWIG
 * MapScript, bug 1373)
 *
 * Revision 1.49  2005/05/27 15:00:12  dan
 * New regex wrappers to solve issues with previous version (bug 1354)
 *
 * Revision 1.48  2005/05/25 21:02:06  dan
 * Added a regex wrapper, allowing MapServer to build with PHP compiled
 * with its builtin regex (bug 1354)
 *
 * Revision 1.47  2004/11/02 21:03:36  assefa
 * Add a 2nd optional argument to LoadMapContext function (Bug 1023).
 *
 * Revision 1.46  2004/07/28 22:03:50  dan
 * Added layer->getFilter() to PHP MapScript (bug 787)
 *
 * Revision 1.45  2004/07/26 14:12:37  dan
 * Created hashtable_i.c for new hashTableObj
 *
 * Revision 1.44  2004/05/31 15:35:39  dan
 * Added setRotation() (bug 702)
 *
 * Revision 1.43  2004/01/30 17:01:12  assefa
 * Add function deletestyle on a class object.
 *
 * Revision 1.42  2004/01/13 23:52:38  assefa
 * Add functions to move styles up and down.
 * Add function to clone style.
 *
 * Revision 1.41  2004/01/12 19:56:19  assefa
 * Add moveclassup and moveclassdown on a layer object.
 * Add clone function for the class object.
 * Add a 2nd optional argument for function ms_newclassobj to be able
 * to pass a class as argument.
 *
 * Revision 1.40  2004/01/05 21:27:14  assefa
 * applySLDURL and applySLD on a layer object can now take an optional
 * argument which is the name of the NamedLayer to use to style the layer.
 *
 * ...
 *
 * Revision 1.1  2000/05/09 21:06:11  dan
 * Initial Import
 *
 * Revision 1.4  2000/04/26 16:10:02  daniel
 * Changes to build with ms_3.3.010
 *
 * Revision 1.3  2000/03/15 00:42:24  daniel
 * Finished conversion of all cover functions to real C
 *
 * Revision 1.2  2000/02/07 05:19:38  daniel
 * Added layerObj and classObj
 *
 * Revision 1.1  2000/01/31 08:38:43  daniel
 * First working version - only mapObj implemented with read-only properties
 *
 **********************************************************************/

#ifndef _PHP_MAPSCRIPT_H_INCLUDED_
#define _PHP_MAPSCRIPT_H_INCLUDED_

#ifdef USE_PHP_REGEX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <direct.h>
#include <memory.h>
#include <malloc.h>
#else
#include <unistd.h>
#endif

#include "map.h"
#include "mapregex.h"
#endif /* USE_PHP_REGEX */


#include "maptemplate.h"
#include "mapogcsld.h"

/*=====================================================================
 *                   Internal functions from mapscript_i.c
 *====================================================================*/

mapObj         *mapObj_new(char *filename, char *new_path);
void            mapObj_destroy(mapObj* self);
mapObj         *mapObj_clone(mapObj* self);
int             mapObj_setRotation(mapObj* self, double rotation_angle );
layerObj       *mapObj_getLayer(mapObj* self, int i);
layerObj       *mapObj_getLayerByName(mapObj* self, char *name);
int             *mapObj_getLayersIndexByGroup(mapObj* self, char *groupname, 
                                             int *pnCount);
int             mapObj_getSymbolByName(mapObj* self, char *name);
void            mapObj_prepareQuery(mapObj* self);
imageObj        *mapObj_prepareImage(mapObj* self);
imageObj        *mapObj_draw(mapObj* self);
imageObj        *mapObj_drawQuery(mapObj* self);
imageObj        *mapObj_drawLegend(mapObj* self);
imageObj        *mapObj_drawScalebar(mapObj* self);
imageObj        *mapObj_drawReferenceMap(mapObj* self);
int             mapObj_embedScalebar(mapObj* self, imageObj *img);
int             mapObj_embedLegend(mapObj* self, imageObj *img);
int             mapObj_drawLabelCache(mapObj* self, imageObj *img);
labelCacheMemberObj *mapObj_nextLabel(mapObj* self);
int             mapObj_queryByPoint(mapObj* self, pointObj *point, 
                                    int mode, double buffer);
int             mapObj_queryByRect(mapObj* self, rectObj rect);
int             mapObj_queryByFeatures(mapObj* self, int slayer);
int             mapObj_queryByShape(mapObj *self, shapeObj *shape);
int              mapObj_queryByIndex(mapObj *self, int qlayer, 
                                     int tileindex, int shapeindex,
                                     int bAddToQuery);
int             mapObj_saveQuery(mapObj *self, char *filename);
int             mapObj_loadQuery(mapObj *self, char *filename);

int             mapObj_setWKTProjection(mapObj *self, char *string);
char*           mapObj_getProjection(mapObj* self);
int             mapObj_setProjection(mapObj* self, char *string);
int             mapObj_save(mapObj* self, char *filename);
char            *mapObj_getMetaData(mapObj *self, char *name);
int             mapObj_setMetaData(mapObj *self, char *name, char *value);
int             mapObj_removeMetaData(mapObj *self, char *name);
void            mapObj_freeQuery(mapObj *self, int qlayer);
int             mapObj_moveLayerup(mapObj *self, int layerindex);
int             mapObj_moveLayerdown(mapObj *self, int layerindex);
int             *mapObj_getLayersdrawingOrder(mapObj *self);
int             mapObj_setLayersdrawingOrder(mapObj *self, int *panIndexes);

char            *mapObj_processTemplate(mapObj *self, int bGenerateImages, 
                                        char **names, char **values, 
                                        int numentries);
char            *mapObj_processLegendTemplate(mapObj *self,
                                              char **names, char **values, 
                                              int numentries);
char            *mapObj_processQueryTemplate(mapObj *self,
                                             int bGenerateImages,
                                             char **names, char **values, 
                                             int numentries);
int             mapObj_setSymbolSet(mapObj *self, char *szFileName);
int             mapObj_getNumSymbols(mapObj *self);
int             mapObj_setFontSet(mapObj *self, char *szFileName);
int             mapObj_saveMapContext(mapObj *self, char *szFileName);
int             mapObj_loadMapContext(mapObj *self, char *szFileName,
                                      int bUniqueLayerName);
int             mapObj_selectOutputFormat(mapObj *self,
                                          const char *imagetype);
int             mapObj_applySLD(mapObj *self, char *sld);
int             mapObj_applySLDURL(mapObj *self, char *sld);
char            *mapObj_generateSLD(mapObj *self);
int             mapObj_loadOWSParameters(mapObj *self, cgiRequestObj *request, 
                                          char *wmtver_string);
int             mapObj_OWSDispatch(mapObj *self, cgiRequestObj *req );

layerObj       *layerObj_new(mapObj *map);
void            layerObj_destroy(layerObj* self);
int             layerObj_open(layerObj *self);
int             layerObj_whichShapes(layerObj *self, rectObj *poRect);
shapeObj        *layerObj_nextShape(layerObj *self);
void            layerObj_close(layerObj *self);
int             layerObj_getShape(layerObj *self, shapeObj *shape,
                                  int tileindex, int shapeindex);
resultCacheMemberObj *layerObj_getResult(layerObj *self, int i);
classObj       *layerObj_getClass(layerObj *self, int i);
int             layerObj_draw(layerObj *self, mapObj *map, imageObj *img);
int             layerObj_drawQuery(layerObj *self, mapObj *map, imageObj *img);
int             layerObj_queryByAttributes(layerObj *self, mapObj *map, 
                                           char *qitem, char *qstring, 
                                           int mode);
int             layerObj_queryByPoint(layerObj *self, mapObj *map, 
                          pointObj *point, int mode, double buffer);
int             layerObj_queryByRect(layerObj *self, mapObj *map,rectObj rect);
int             layerObj_queryByFeatures(layerObj *self, mapObj *map, 
                                         int slayer);
int             layerObj_queryByShape(layerObj *self, mapObj *map, 
                                      shapeObj *shape);
int             layerObj_setFilter(layerObj *self, char *string);
char*           layerObj_getFilter(layerObj *self);
int             layerObj_setWKTProjection(layerObj *self, char *string);
char*           layerObj_getProjection(layerObj *self);
int             layerObj_setProjection(layerObj *self, char *string);
int             layerObj_addFeature(layerObj *self, shapeObj *shape);
char            *layerObj_getMetaData(layerObj *self, char *name);
int             layerObj_setMetaData(layerObj *self, char *name, char *value);
int             layerObj_removeMetaData(layerObj *self, char *name);
char            *layerObj_getWMSFeatureInfoURL(layerObj *self, mapObj *map, 
                                               int click_x, int click_y,     
                                               int feature_count, 
                                               char *info_format);
char            *layerObj_executeWFSGetFeature(layerObj *self);
int             layerObj_applySLD(layerObj *self, char *sld, char *stylelayer);
int             layerObj_applySLDURL(layerObj *self, char *sld, char *stylelayer);
char            *layerObj_generateSLD(layerObj *self);
int             layerObj_moveClassUp(layerObj *self, int index);
int             layerObj_moveClassDown(layerObj *self, int index);
classObj        *layerObj_removeClass(layerObj *self, int index);


classObj       *classObj_new(layerObj *layer, classObj *class);
void            classObj_destroy(classObj* self);
int             classObj_setExpression(classObj *self, char *string);
char            *classObj_getExpressionString(classObj *self);
int             classObj_setText(classObj *self,layerObj *layer,char *string);
int             classObj_drawLegendIcon(classObj *self,
                                        mapObj *map,
                                        layerObj *layer, 
                                        int width, int height, 
                                        gdImagePtr im, 
                                        int dstX, int dstY);
imageObj       *classObj_createLegendIcon(classObj *self, 
                                          mapObj *map, 
                                          layerObj *layer, 
                                          int width, int height);
int             classObj_setSymbolByName(classObj *self,
                                         mapObj *map,
                                         char *pszSymbolName);
int             classObj_setOverlaySymbolByName(classObj *self,
                                                mapObj *map,
                                                char *pszOverlaySymbolName);
classObj        *classObj_clone(classObj *class, layerObj *layer);
int             classObj_moveStyleUp(classObj *self, int index);
int             classObj_moveStyleDown(classObj *self, int index);
int             classObj_deleteStyle(classObj *self, int index);

pointObj       *pointObj_new();
void            pointObj_destroy(pointObj *self);
int             pointObj_project(pointObj *self, projectionObj *in, 
                                 projectionObj *out);
int             pointObj_draw(pointObj *self, mapObj *map, layerObj *layer, 
                              imageObj *img, int class_index, 
                              char *label_string);
double          pointObj_distanceToPoint(pointObj *self, pointObj *point);
double          pointObj_distanceToLine(pointObj *self, pointObj *a, 
                                        pointObj *b);
double          pointObj_distanceToShape(pointObj *self, shapeObj *shape);


lineObj        *lineObj_new();
void            lineObj_destroy(lineObj *self);
int             lineObj_project(lineObj *self, projectionObj *in, 
                                projectionObj *out);
pointObj       *lineObj_get(lineObj *self, int i);
int             lineObj_add(lineObj *self, pointObj *p);


shapeObj       *shapeObj_new(int type);
void            shapeObj_destroy(shapeObj *self);
int             shapeObj_project(shapeObj *self, projectionObj *in, 
                                 projectionObj *out);
lineObj        *shapeObj_get(shapeObj *self, int i);
int             shapeObj_add(shapeObj *self, lineObj *line);
int             shapeObj_draw(shapeObj *self, mapObj *map, layerObj *layer, 
                              imageObj *img);
void            shapeObj_setBounds(shapeObj *self);
int             shapeObj_copy(shapeObj *self, shapeObj *dest);
int             shapeObj_contains(shapeObj *self, pointObj *point);
int             shapeObj_intersects(shapeObj *self, shapeObj *shape);
pointObj        *shapeObj_getpointusingmeasure(shapeObj *self, double m);
pointObj        *shapeObj_getmeasureusingpoint(shapeObj *self, pointObj *point);

shapeObj        *shapeObj_buffer(shapeObj *self, double width);
shapeObj        *shapeObj_convexHull(shapeObj *self);
shapeObj        *shapeObj_boundary(shapeObj *self);
int             shapeObj_contains_geos(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_Union(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_intersection(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_difference(shapeObj *self, shapeObj *poshape);
shapeObj        *shapeObj_symdifference(shapeObj *self, shapeObj *poshape);
int             shapeObj_overlaps(shapeObj *self, shapeObj *shape);
int             shapeObj_within(shapeObj *self, shapeObj *shape);
int             shapeObj_crosses(shapeObj *self, shapeObj *shape);
int             shapeObj_touches(shapeObj *self, shapeObj *shape);
int             shapeObj_equals(shapeObj *self, shapeObj *shape);
int             shapeObj_disjoint(shapeObj *self, shapeObj *shape);
pointObj        *shapeObj_getcentroid(shapeObj *self);
double          shapeObj_getarea(shapeObj *self);
double          shapeObj_getlength(shapeObj *self);

rectObj        *rectObj_new();
void            rectObj_destroy(rectObj *self);
int             rectObj_project(rectObj *self, projectionObj *in, 
                                projectionObj *out);
double          rectObj_fit(rectObj *self, int width, int height);
int             rectObj_draw(rectObj *self, mapObj *map, layerObj *layer,
                             imageObj *img, int classindex, char *text);


shapefileObj   *shapefileObj_new(char *filename, int type);
void            shapefileObj_destroy(shapefileObj *self);
int             shapefileObj_get(shapefileObj *self, int i, shapeObj *shape);
int             shapefileObj_getPoint(shapefileObj *self, int i, pointObj *point);
int             shapefileObj_getTransformed(shapefileObj *self, mapObj *map, 
                                            int i, shapeObj *shape);
void            shapefileObj_getExtent(shapefileObj *self, int i, 
                                       rectObj *rect);
int             shapefileObj_add(shapefileObj *self, shapeObj *shape);
int             shapefileObj_addPoint(shapefileObj *self, pointObj *point);

projectionObj   *projectionObj_new(char *string);
void            projectionObj_destroy(projectionObj *self);

void            labelCacheObj_freeCache(labelCacheObj *self);

char           *DBFInfo_getFieldName(DBFInfo *self, int iField);
int             DBFInfo_getFieldWidth(DBFInfo *self, int iField);
int             DBFInfo_getFieldDecimals(DBFInfo *self, int iField);
DBFFieldType    DBFInfo_getFieldType(DBFInfo *self, int iField);

styleObj       *styleObj_new(classObj *class, styleObj *style);
void            styleObj_destroy(styleObj* self);
int             styleObj_setSymbolByName(styleObj *self, mapObj *map, 
                                         char* pszSymbolName);
styleObj       *styleObj_clone(styleObj *style);

/*=====================================================================
 *                   Internal functions from hashtable_i.c
 *====================================================================*/
hashTableObj   *hashTableObj_new();
int             hashTableObj_set(hashTableObj *self, const char *key, 
                                 const char *value);
const char     *hashTableObj_get(hashTableObj *self, const char *key);
int            *hashTableObj_remove(hashTableObj *self, const char *key);
void           *hashTableObj_clear(hashTableObj *self);
char           *hashTableObj_nextKey(hashTableObj *self, const char *prevkey);


cgiRequestObj *cgirequestObj_new();
int cgirequestObj_loadParams(cgiRequestObj *self);
void cgirequestObj_setParameter(cgiRequestObj *self, char *name, char *value);
char *cgirequestObj_getName(cgiRequestObj *self, int index);
char *cgirequestObj_getValue(cgiRequestObj *self, int index);
char *cgirequestObj_getValueByName(cgiRequestObj *self, const char *name);
void cgirequestObj_destroy(cgiRequestObj *self);

#endif /* _PHP_MAPSCRIPT_H_INCLUDED_ */
