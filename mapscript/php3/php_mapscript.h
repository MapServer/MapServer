/**********************************************************************
 * $Id$
 *
 * Name:     php_mapscript.c
 * Project:  PHP/MapScript extension for MapServer.
 * Language: ANSI C
 * Purpose:  Header file - prototypes / module definitions
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 2000, 2001, Daniel Morissette, DM Solutions Group
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
 * Revision 1.12  2001/10/03 12:41:05  assefa
 * Add function getLayersIndexByGroup.
 *
 * Revision 1.11  2001/08/01 13:52:59  dan
 * Sync with mapscript.i v1.39: add QueryByAttributes() and take out type arg
 * to getSymbolByName().
 *
 * Revision 1.10  2001/07/26 19:50:08  assefa
 * Add projection class and related functions.
 *
 * Revision 1.9  2001/04/19 15:11:34  dan
 * Sync with mapscript.i v.1.32
 *
 * Revision 1.8  2001/03/30 04:16:15  dan
 * Removed shapepath parameter to layer->getshape()
 *
 * Revision 1.7  2001/03/21 21:55:28  dan
 * Added get/setMetaData() for layerObj and mapObj()
 *
 * Revision 1.6  2001/03/09 19:33:14  dan
 * Updated PHP MapScript... still a few methods missing, and needs testing.
 *
 * Revision 1.5  2001/02/23 21:58:00  dan
 * PHP MapScript working with new 3.5 stuff, but query stuff is disabled
 *
 * Revision 1.4  2000/11/01 16:31:08  dan
 * Add missing functions (in sync with mapscript).
 *
 * Revision 1.3  2000/09/07 20:18:42  dan
 * Sync with mapscript.i version 1.16
 *
 * Revision 1.2  2000/06/28 20:22:02  dan
 * Sync with mapscript.i version 1.9
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

#include "map.h"

/*=====================================================================
 *                   Internal functions from mapscript_i.c
 *====================================================================*/

mapObj         *mapObj_new(char *filename);
void            mapObj_destroy(mapObj* self);
layerObj       *mapObj_getLayer(mapObj* self, int i);
layerObj       *mapObj_getLayerByName(mapObj* self, char *name);
int             *mapObj_getLayersIndexByGroup(mapObj* self, char *groupname, 
                                             int *pnCount);
int             mapObj_addColor(mapObj* self, int r, int g, int b);
int             mapObj_getSymbolByName(mapObj* self, char *name);
void            mapObj_prepareQuery(mapObj* self);
gdImagePtr      mapObj_prepareImage(mapObj* self);
gdImagePtr      mapObj_draw(mapObj* self);
gdImagePtr      mapObj_drawQuery(mapObj* self);
gdImagePtr      mapObj_drawLegend(mapObj* self);
gdImagePtr      mapObj_drawScalebar(mapObj* self);
gdImagePtr      mapObj_drawReferenceMap(mapObj* self);
int             mapObj_embedScalebar(mapObj* self, gdImagePtr img);
int             mapObj_embedLegend(mapObj* self, gdImagePtr img);
int             mapObj_drawLabelCache(mapObj* self, gdImagePtr img);
labelCacheMemberObj *mapObj_nextLabel(mapObj* self);
int             mapObj_queryByPoint(mapObj* self, pointObj *point, 
                                    int mode, double buffer);
int             mapObj_queryByRect(mapObj* self, rectObj rect);
int             mapObj_queryByFeatures(mapObj* self, int slayer);
int             mapObj_queryByShape(mapObj *self, shapeObj *shape);
int             mapObj_setProjection(mapObj* self, char *string);
int             mapObj_save(mapObj* self, char *filename);
char            *mapObj_getMetaData(mapObj *self, char *name);
int             mapObj_setMetaData(mapObj *self, char *name, char *value);


layerObj       *layerObj_new(mapObj *map);
void            layerObj_destroy(layerObj* self);
int             layerObj_open(layerObj *self, char *path);
void            layerObj_close(layerObj *self);
int             layerObj_getShape(layerObj *self, shapeObj *shape,
                                  int tileindex, int shapeindex);
resultCacheMemberObj *layerObj_getResult(layerObj *self, int i);
classObj       *layerObj_getClass(layerObj *self, int i);
int             layerObj_draw(layerObj *self, mapObj *map, gdImagePtr img);
int             layerObj_drawQuery(layerObj *self, mapObj *map,gdImagePtr img);
int             layerObj_queryByAttributes(layerObj *self, mapObj *map, 
                                           int mode);
int             layerObj_queryByPoint(layerObj *self, mapObj *map, 
                          pointObj *point, int mode, double buffer);
int             layerObj_queryByRect(layerObj *self, mapObj *map,rectObj rect);
int             layerObj_queryByFeatures(layerObj *self, mapObj *map, 
                                         int slayer);
int             layerObj_queryByShape(layerObj *self, mapObj *map, 
                                      shapeObj *shape);
int             layerObj_setFilter(layerObj *self, char *string);
int             layerObj_setProjection(layerObj *self, char *string);
int             layerObj_addFeature(layerObj *self, shapeObj *shape);
char            *layerObj_getMetaData(layerObj *self, char *name);
int             layerObj_setMetaData(layerObj *self, char *name, char *value);

classObj       *classObj_new(layerObj *layer);
void            classObj_destroy(classObj* self);
int             classObj_setExpression(classObj *self, char *string);
int             classObj_setText(classObj *self,layerObj *layer,char *string);


pointObj       *pointObj_new();
void            pointObj_destroy(pointObj *self);
int             pointObj_project(pointObj *self, projectionObj *in, 
                                 projectionObj *out);
int             pointObj_draw(pointObj *self, mapObj *map, layerObj *layer, 
                              gdImagePtr img, int class_index, 
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
                              gdImagePtr img);
void            shapeObj_setBounds(shapeObj *self);
int             shapeObj_copy(shapeObj *self, shapeObj *dest);
int             shapeObj_contains(shapeObj *self, pointObj *point);
int             shapeObj_intersects(shapeObj *self, shapeObj *shape);

rectObj        *rectObj_new();
void            rectObj_destroy(rectObj *self);
int             rectObj_project(rectObj *self, projectionObj *in, 
                                projectionObj *out);
double          rectObj_fit(rectObj *self, int width, int height);
int             rectObj_draw(rectObj *self, mapObj *map, layerObj *layer,
                             gdImagePtr img, int classindex, char *text);


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


#endif /* _PHP_MAPSCRIPT_H_INCLUDED_ */
