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
 * Copyright (c) 2000-2002, Daniel Morissette, DM Solutions Group
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
 * Revision 1.39  2003/12/03 18:56:09  assefa
 * Add functions to apply and to generate sld on a layer object.
 *
 * Revision 1.38  2003/12/01 16:12:01  assefa
 * Add applysld and applysldurl on map.
 *
 * Revision 1.37  2003/10/30 22:55:04  assefa
 * getexpression function in Sync with the mapscript.i
 *
 * Revision 1.36  2003/10/30 22:37:27  assefa
 * Add functions executewfsgetfeature on a WFS layer object.
 * Add function getexpression on a class object.
 *
 * Revision 1.35  2003/10/28 16:50:25  assefa
 * Add functions removeMetaData on map and layer.
 *
 * Revision 1.34  2003/07/03 15:31:47  assefa
 * Add the possibility to generate image for function
 * processquerytemplate (bug 341).
 *
 * Revision 1.33  2003/02/24 02:19:44  dan
 * Added map->clone() method
 *
 * Revision 1.32  2003/02/21 16:55:12  assefa
 * Add function querybyindex and freequery.
 *
 * Revision 1.31  2003/02/14 20:17:27  assefa
 * Add savequery and loadquery functions.
 *
 * Revision 1.30  2003/02/11 19:01:08  assefa
 * Distnace points functions have changed names.
 *
 * Revision 1.29  2003/01/11 00:06:40  dan
 * Added setWKTProjection() to mapObj and layerObj
 *
 * Revision 1.28  2003/01/08 15:00:16  assefa
 * Add setsymbolbyname in the style class.
 *
 * Revision 1.27  2002/10/28 20:31:22  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.3  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
 * Revision 1.26  2002/10/23 19:44:08  assefa
 * Add setcolor functions for style and label objects.
 * Add function to select the output format.
 * Correct PrepareImage and PasteImage functions.
 *
 * Revision 1.25  2002/08/09 22:55:38  assefa
 * Update code to be in sync with mapserver addition of Styles.
 *
 * Revision 1.24  2002/07/08 19:07:06  dan
 * Added map->setFontSet() to MapScript
 *
 * Revision 1.23  2002/05/10 19:16:30  dan
 * Added qitem,qstring args to PHP version of layer->queryByAttributes()
 *
 * Revision 1.22  2002/05/02 15:55:51  assefa
 * Adapt code to support imageObj.
 *
 * Revision 1.21  2002/04/22 19:31:57  dan
 * Added optional new_map_path arg to msLoadMap()
 *
 * Revision 1.20  2002/03/14 21:36:12  sacha
 * Add two mapscript function (in PHP and perl)
 * setSymbolSet(filename) that load a symbol file dynanictly
 * getNumSymbols() return the number of symbol in map.
 *
 * Revision 1.19  2002/03/07 22:31:01  assefa
 * Add template processing functions.
 *
 * Revision 1.18  2002/02/08 18:51:11  dan
 * Remove class and layer args to setSymbolByName()
 *
 * Revision 1.17  2002/02/08 18:25:40  sacha
 * let mapserv add a new symbol when we use the classobj setproperty function
 * with "symbolname" and "overlaysymbolname" arg.
 *
 * Revision 1.16  2002/01/22 21:19:02  sacha
 * Add two functions in maplegend.c
 * - msDrawLegendIcon that draw an class legend icon over an existing image.
 * - msCreateLegendIcon that draw an class legend icon and return the newly
 * created image.
 * Also, an php examples in mapscript/php3/examples/test_draw_legend_icon.phtml
 *
 * Revision 1.15  2001/12/19 03:46:02  assefa
 * Support of Measured shpe files.
 *
 * Revision 1.14  2001/11/01 21:10:09  assefa
 * Add getProjection on map and layer object.
 *
 * Revision 1.13  2001/11/01 02:47:06  dan
 * Added layerObj->getWMSFeatureInfoURL()
 *
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

//#include "map.h"
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
int             mapObj_loadMapContext(mapObj *self, char *szFileName);
int             mapObj_selectOutputFormat(mapObj *self,
                                          const char *imagetype);
int             mapObj_applySLD(mapObj *self, char *sld);
int             mapObj_applySLDURL(mapObj *self, char *sld);
char            *mapObj_generateSLD(mapObj *self);

layerObj       *layerObj_new(mapObj *map);
void            layerObj_destroy(layerObj* self);
int             layerObj_open(layerObj *self);
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

#endif /* _PHP_MAPSCRIPT_H_INCLUDED_ */
