/**********************************************************************
 * $Id$
 *
 * Name:     php_mapscript.c
 * Project:  PHP/MapScript extension for MapServer.
 * Language: ANSI C
 * Purpose:  External interface functions
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 2000, Daniel Morissette
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
 * Revision 1.7  2000/08/16 21:43:18  dan
 * Removed obsolete symbol type consts (MS_MARKERSET, etc) + added MS_QUERY
 *
 * Revision 1.6  2000/07/14 19:09:43  dan
 * OOpps... same problem with pIndex in getshape() and gettransformed()
 *
 * Revision 1.5  2000/07/14 19:04:36  dan
 * Fix in shapefile_getextent(): convert pIndex to long and not double
 *
 * Revision 1.4  2000/07/12 20:20:50  dan
 * Added labelObj background and shadow members
 *
 * Revision 1.3  2000/06/30 14:27:23  dan
 * Add a possibility to zoom in as much as possible using the minscale.
 *
 * Revision 1.2  2000/06/28 20:25:16  dan
 * Fixed colorObj.setRGB(), added map.save() + sync with mapscript.i v1.9
 *
 * Revision 1.1  2000/05/09 21:06:11  dan
 * Initial Import
 *
 * Revision 1.20  2000/05/09 20:38:32  daniel
 * Added classObj->setExpression()
 *
 * Revision 1.19  2000/05/05 20:59:41  daniel
 * Added shapefileObj.free() and fixed free() method on other classes
 *
 * Revision 1.18  2000/04/27 20:10:13  daniel
 * Added missing return value in map->zoomRectangle()
 *
 * Revision 1.17  2000/04/27 18:48:29  daniel
 * Fixed test of Max. extents in map->zoomPoint()
 *
 * Revision 1.16  2000/04/26 16:10:02  daniel
 * Changes to build with ms_3.3.010
 *
 * Revision 1.15  2000/04/03 22:37:41  daniel
 * Added include errno.h for Unix
 *
 * Revision 1.14  2000/04/03 15:03:08  assefa
 * Add ms_getcwd function to get current working directory.
 *
 * Revision 1.13  2000/03/27 22:39:46  daniel
 * Fixed compile warnings
 *
 * Revision 1.12  2000/03/27 21:57:46  assefa
 * Add rectange query functions on map and layer objects.
 *
 * Revision 1.11  2000/03/23 23:32:31  assefa
 * Change the way to test for min/max scale when zooming.
 *
 * Revision 1.10  2000/03/20 22:49:00  daniel
 * Fixed shapeResultObj constructor and added missing members to layerObj
 *
 * Revision 1.9  2000/03/20 21:15:43  assefa
 * Add scale and maximum range in zooming functions.
 *
 * Revision 1.8  2000/03/17 23:08:30  daniel
 * Added shapeFileObj, queryUsingPoint() methods, and all MS_* constants
 *
 * Revision 1.7  2000/03/16 20:39:59  daniel
 * Added lineObj, shapeObj, featureObj
 *
 * Revision 1.6  2000/03/16 20:27:23  assefa
 * Add zooming functions. Add getColorByIndex function.
 *
 * Revision 1.5  2000/03/15 23:03:04  assefa
 * Add return values in function getLayerByName.
 *
 * Revision 1.4  2000/03/15 06:20:18  daniel
 * Added pointObj, colorObj, shapeResultObj, queryResultObj, etc.
 *
 * Revision 1.3  2000/03/15 05:27:31  assefa
 * Add Web object, refernece map object, rect object.
 *
 * Revision 1.2  2000/02/07 05:18:02  daniel
 * Added layerObj, classObj, and labelObj
 *
 * Revision 1.1  2000/01/31 08:38:43  daniel
 * First working version - only mapObj implemented with read-only properties
 *
 **********************************************************************/

#include "php_mapscript.h"

#include "phpdl.h"
#include "php3_list.h"
#include "maperror.h"

#include <time.h>

#ifdef _WIN32
#include <process.h>
#else
#include <errno.h>
#endif

#define PHP3_MS_VERSION "1.0.011 (Jul 14, 2000)"

/*=====================================================================
 *                         Prototypes
 *====================================================================*/

DLEXPORT void php3_info_mapscript(void);
DLEXPORT int  php3_init_mapscript(INIT_FUNC_ARGS);
DLEXPORT int  php3_end_mapscript(void);
DLEXPORT void php3_ms_free_mapObj(mapObj *pMap);
DLEXPORT void php3_ms_free_image(gdImagePtr im);
DLEXPORT void php3_ms_free_queryResult(queryResultObj *pQueryResult);
DLEXPORT void php3_ms_free_point(pointObj *pPoint);
DLEXPORT void php3_ms_free_line(lineObj *pLine);
DLEXPORT void php3_ms_free_shape(shapeObj *pShape);
DLEXPORT void php3_ms_free_shapefile(shapefileObj *pShapefile);
DLEXPORT void php3_ms_free_rect(rectObj *pRect);
DLEXPORT void php3_ms_free_stub(void *ptr) ;

DLEXPORT void php3_ms_map_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_addColor(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawLabelCache(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawLegend(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawReferenceMap(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_drawScaleBar(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getLayer(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getLayerByName(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getSymbolByName(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_prepareImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_nextLabel(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_getColorByIndex(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_queryUsingPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_queryUsingRect(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_save(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_map_setExtent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_map_zoomRectangle(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_img_saveImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_img_saveWebImage(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_img_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_lyr_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_getClass(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_firstFeature(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_queryUsingPoint(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_lyr_queryUsingRect(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_class_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_class_setExpression(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_label_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_queryresult_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_queryresult_free(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_queryresult_next(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_queryresult_rewind(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_color_setRGB(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_rect_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_draw(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_point_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_setXY(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_point_draw(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_line_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_add(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_addXY(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_line_point(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_shape_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_add(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_line(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shape_draw(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_shapefile_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_addshape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getshape(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_gettransformed(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_getextent(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_shapefile_free(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_feature_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_feature_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_feature_next(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_web_setProperty(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_rect_new(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_setProperty(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_draw(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_rect_setExtent(INTERNAL_FUNCTION_PARAMETERS);

DLEXPORT void php3_ms_referenceMap_setProperty(INTERNAL_FUNCTION_PARAMETERS);


static long _phpms_build_img_object(gdImagePtr im, webObj *pweb,
                                    HashTable *list, pval *return_value);
static long _phpms_build_layer_object(layerObj *player, int parent_map_id,
                                      HashTable *list, pval *return_value);
static long _phpms_build_class_object(classObj *pclass, int parent_layer_id,
                                      HashTable *list, pval *return_value);
static long _phpms_build_label_object(labelObj *plabel,
                                      HashTable *list, pval *return_value);
static long _phpms_build_queryresult_object(queryResultObj *pquery, 
                                            HashTable *list, 
                                            pval *return_value);
static long _phpms_build_shapeResult_object(shapeResultObj *pShapeResult,
                                            HashTable *list, 
                                            pval *return_value);
static long _phpms_build_color_object(colorObj *pcolor,
                                      HashTable *list, pval *return_value);
static long _phpms_build_point_object(pointObj *ppoint, int handle_type,
                                      HashTable *list, pval *return_value);
static long _phpms_build_feature_object(struct featureObj *pfeature, 
                                        int parent_layer_id,
                                        HashTable *list, pval *return_value);
static long _phpms_build_web_object(webObj *pweb,
                                    HashTable *list, pval *return_value);
static long _phpms_build_rect_object(rectObj *prect, int handle_type,
                                    HashTable *list, pval *return_value);
static long _phpms_build_referenceMap_object(referenceMapObj *preferenceMap,
                                          HashTable *list, pval *return_value);

/* ==================================================================== */
/*      utility functions prototypes.                                   */
/* ==================================================================== */
DLEXPORT void php3_ms_getcwd(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php3_ms_getpid(INTERNAL_FUNCTION_PARAMETERS);


static double GetDeltaExtentsUsingScale(double dfMinscale, int nUnits, 
                                        int nWidth);

/*=====================================================================
 *               PHP Dynamically Loadable Library stuff
 *====================================================================*/

#define PHPMS_GLOBAL(a) a
static int le_msmap;
static int le_mslayer;
static int le_msimg;
static int le_msclass;
static int le_mslabel;
static int le_msqueryresult;
static int le_mscolor;
static int le_msrect_new;
static int le_msrect_ref;
static int le_mspoint_new;
static int le_mspoint_ref;
static int le_msline_new;
static int le_msline_ref;
static int le_msshape_new;
static int le_msshape_ref;
static int le_msshapefile;
static int le_msfeature;
static int le_msweb;
static int ls_msrefmap;

static char tmpId[128]; /* big enough for time + pid */
static int  tmpCount;

function_entry php3_ms_functions[] = {
    {"ms_newmapobj",    php3_ms_map_new,   NULL},
    {"ms_newlayerobj",  php3_ms_lyr_new,   NULL},
    {"ms_newclassobj",  php3_ms_class_new, NULL},
    {"ms_newpointobj",  php3_ms_point_new, NULL},
    {"ms_newlineobj",   php3_ms_line_new,  NULL},
    {"ms_newshapeobj",  php3_ms_shape_new,  NULL},
    {"ms_newshapefileobj", php3_ms_shapefile_new,  NULL},
    {"ms_newrectobj",      php3_ms_rect_new,  NULL},
    {"ms_newfeatureobj",   php3_ms_feature_new,  NULL},
    {"ms_getcwd",    php3_ms_getcwd,   NULL},
    {"ms_getpid",    php3_ms_getpid,   NULL},
    {NULL, NULL, NULL}
};


php3_module_entry php3_ms_module_entry = {
    "MapScript", php3_ms_functions, php3_init_mapscript, php3_end_mapscript,
    NULL, NULL, php3_info_mapscript, STANDARD_MODULE_PROPERTIES 
};


#if COMPILE_DL
DLEXPORT php3_module_entry *get_module(void) { return &php3_ms_module_entry; }
#endif


DLEXPORT void php3_info_mapscript(void) 
{
    php3_printf("MapScript Version %s<br>\n", PHP3_MS_VERSION);

    php3_printf("MapServer Version %s ", MS_VERSION);
#ifdef USE_PROJ
    php3_printf(" -PROJ.4");
#endif
#ifdef USE_TTF
    php3_printf(" -FreeType");
#endif
#ifdef USE_TIFF
    php3_printf(" -TIFF");
#endif
#ifdef USE_EPPL
    php3_printf(" -EPPL7");
#endif
#ifdef USE_JPEG
    php3_printf(" -JPEG");
#endif
#ifdef USE_EGIS
    php3_printf(" -EGIS");
#endif
    php3_printf("<BR>\n");
}

DLEXPORT int php3_init_mapscript(INIT_FUNC_ARGS)
{
    int const_flag = CONST_CS|CONST_PERSISTENT;

    PHPMS_GLOBAL(le_msmap)  = register_list_destructors(php3_ms_free_mapObj,
                                                        NULL);
    PHPMS_GLOBAL(le_msimg)  = register_list_destructors(php3_ms_free_image,
                                                        NULL);
    PHPMS_GLOBAL(le_mslayer)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msclass)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_mslabel)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msqueryresult)=
                          register_list_destructors(php3_ms_free_queryResult,
                                                    NULL);
    PHPMS_GLOBAL(le_mscolor)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_mspoint_new)= register_list_destructors(php3_ms_free_point,
                                                        NULL);
    PHPMS_GLOBAL(le_mspoint_ref)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msline_new)= register_list_destructors(php3_ms_free_line,
                                                        NULL);
    PHPMS_GLOBAL(le_msline_ref)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msshape_new)= register_list_destructors(php3_ms_free_shape,
                                                        NULL);
    PHPMS_GLOBAL(le_msshape_ref)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msshapefile)= 
                              register_list_destructors(php3_ms_free_shapefile,
                                                        NULL);
    PHPMS_GLOBAL(le_msfeature)= register_list_destructors(php3_ms_free_stub,
                                                        NULL);
    PHPMS_GLOBAL(le_msweb)= register_list_destructors(php3_ms_free_stub,
                                                      NULL);
    PHPMS_GLOBAL(ls_msrefmap)= register_list_destructors(php3_ms_free_stub,
                                                             NULL);
    PHPMS_GLOBAL(le_msrect_new)= register_list_destructors(php3_ms_free_rect,
                                                           NULL);
    PHPMS_GLOBAL(le_msrect_ref)= register_list_destructors(php3_ms_free_stub,
                                                           NULL);

    /* boolean constants*/
    REGISTER_LONG_CONSTANT("MS_TRUE",       MS_TRUE,        const_flag);
    REGISTER_LONG_CONSTANT("MS_FALSE",      MS_FALSE,       const_flag);
    REGISTER_LONG_CONSTANT("MS_ON",         MS_ON,          const_flag);
    REGISTER_LONG_CONSTANT("MS_OFF",        MS_OFF,         const_flag);
    REGISTER_LONG_CONSTANT("MS_YES",        MS_YES,         const_flag);
    REGISTER_LONG_CONSTANT("MS_NO",         MS_NO,          const_flag);

    /* map units constants*/
    REGISTER_LONG_CONSTANT("MS_INCHES",     MS_INCHES,      const_flag);
    REGISTER_LONG_CONSTANT("MS_FEET",       MS_FEET,        const_flag);
    REGISTER_LONG_CONSTANT("MS_MILES",      MS_MILES,       const_flag);
    REGISTER_LONG_CONSTANT("MS_METERS",     MS_METERS,      const_flag);
    REGISTER_LONG_CONSTANT("MS_KILOMETERS", MS_KILOMETERS,  const_flag);
    REGISTER_LONG_CONSTANT("MS_DD",         MS_DD,          const_flag);
    REGISTER_LONG_CONSTANT("MS_PIXELS",     MS_PIXELS,      const_flag);

    /* layer type constants*/
    REGISTER_LONG_CONSTANT("MS_POINT",      MS_POINT,       const_flag);
    REGISTER_LONG_CONSTANT("MS_LINE",       MS_LINE,        const_flag);
    REGISTER_LONG_CONSTANT("MS_POLYGON",    MS_POLYGON,     const_flag);
    REGISTER_LONG_CONSTANT("MS_POLYLINE",   MS_POLYLINE,    const_flag);
    REGISTER_LONG_CONSTANT("MS_RASTER",     MS_RASTER,      const_flag);
    REGISTER_LONG_CONSTANT("MS_ANNOTATION", MS_ANNOTATION,  const_flag);
    REGISTER_LONG_CONSTANT("MS_NULL",       MS_NULL,        const_flag);

    /* layer status constants (see also MS_ON, MS_OFF) */
    REGISTER_LONG_CONSTANT("MS_QUERY",      MS_QUERY,       const_flag);

    /* font type constants*/
    REGISTER_LONG_CONSTANT("MS_TRUETYPE",   MS_TRUETYPE,    const_flag);
    REGISTER_LONG_CONSTANT("MS_BITMAP",     MS_BITMAP,      const_flag);

    /* label position constants*/
    REGISTER_LONG_CONSTANT("MS_UL",         MS_UL,          const_flag);
    REGISTER_LONG_CONSTANT("MS_LR",         MS_LR,          const_flag);
    REGISTER_LONG_CONSTANT("MS_UR",         MS_UR,          const_flag);
    REGISTER_LONG_CONSTANT("MS_LL",         MS_LL,          const_flag);
    REGISTER_LONG_CONSTANT("MS_CR",         MS_CR,          const_flag);
    REGISTER_LONG_CONSTANT("MS_CL",         MS_CL,          const_flag);
    REGISTER_LONG_CONSTANT("MS_UC",         MS_UC,          const_flag);
    REGISTER_LONG_CONSTANT("MS_LC",         MS_LC,          const_flag);
    REGISTER_LONG_CONSTANT("MS_AUTO",       MS_AUTO,        const_flag);

    /* shapefile type constants*/
    REGISTER_LONG_CONSTANT("MS_SHP_POINT",  MS_SHP_POINT,   const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_ARC",    MS_SHP_ARC,     const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_POLYGON",MS_SHP_POLYGON, const_flag);
    REGISTER_LONG_CONSTANT("MS_SHP_MULTIPOINT", MS_SHP_MULTIPOINT,const_flag);

    /* query/join type constants*/
    REGISTER_LONG_CONSTANT("MS_SINGLE",     MS_SINGLE,      const_flag);
    REGISTER_LONG_CONSTANT("MS_MULTIPLE",   MS_MULTIPLE,    const_flag);

    /* We'll use tmpId and tmpCount to generate unique filenames */
    sprintf(GLOBAL(tmpId), "%ld%d",(long)time(NULL),(int)getpid());
    tmpCount = 0;

    return SUCCESS;
}

DLEXPORT int php3_end_mapscript(void)
{
    return SUCCESS;
}


/**********************************************************************
 *                     resource list destructors
 **********************************************************************/
DLEXPORT void php3_ms_free_mapObj(mapObj *pMap) 
{
    mapObj_destroy(pMap);
}

DLEXPORT void php3_ms_free_image(gdImagePtr im) 
{
    msFreeImage(im);
}

DLEXPORT void php3_ms_free_queryResult(queryResultObj *pQueryResult) 
{
    queryResultObj_free(pQueryResult);
}

DLEXPORT void php3_ms_free_rect(rectObj *pRect) 
{
    rectObj_destroy(pRect);
}

DLEXPORT void php3_ms_free_point(pointObj *pPoint) 
{
    pointObj_destroy(pPoint);
}

DLEXPORT void php3_ms_free_line(lineObj *pLine) 
{
    lineObj_destroy(pLine);
}

DLEXPORT void php3_ms_free_shape(shapeObj *pShape) 
{
    shapeObj_destroy(pShape);
}

DLEXPORT void php3_ms_free_shapefile(shapefileObj *pShapefile) 
{
    shapefileObj_destroy(pShapefile);
}

DLEXPORT void php3_ms_free_stub(void *ptr)
{
    /* nothing to do... map destructor takes care of all objects. */
}

/*=====================================================================
 *                       Misc support functions
 *====================================================================*/

/**********************************************************************
 *                  macros for setting object properties
 **********************************************************************/
#define IF_SET_LONG(php_name, internal_var)                     \
  if (strcmp(pPropertyName->value.str.val, php_name) == 0)      \
  {                                                             \
    convert_to_long(pNewValue);                                 \
    _phpms_set_property_long(pThis,php_name, pNewValue->value.lval, E_ERROR); \
    internal_var = pNewValue->value.lval;                       \
  }

#define IF_SET_DOUBLE(php_name, internal_var)                   \
  if (strcmp(pPropertyName->value.str.val, php_name) == 0)      \
  {                                                             \
    _phpms_set_property_double(pThis,php_name,pNewValue->value.dval,E_ERROR); \
    internal_var = pNewValue->value.dval;                       \
  }

#define IF_SET_STRING(php_name, internal_var)                   \
  if (strcmp(pPropertyName->value.str.val, php_name) == 0)      \
  {                                                             \
    _phpms_set_property_string(pThis,php_name,pNewValue->value.str.val,E_ERROR); \
    if (internal_var) free(internal_var);                       \
    internal_var = NULL;                                        \
    if (pNewValue->value.str.val)                               \
      internal_var = strdup(pNewValue->value.str.val);          \
  }

#define IF_SET_BYTE(php_name, internal_var)                     \
  if (strcmp(pPropertyName->value.str.val, php_name) == 0)      \
  {                                                             \
    convert_to_long(pNewValue);                                 \
    _phpms_set_property_long(pThis,php_name, pNewValue->value.lval, E_ERROR); \
    internal_var = (unsigned char)pNewValue->value.lval;                       \
  }

#define PHPMS_ADD_PROP_STR(ret_val, name, value) \
  add_property_string(ret_val, name, (value)?(value):"", 1)


/**********************************************************************
 *                     _phpms_report_mapserver_error()
 **********************************************************************/
static void _phpms_report_mapserver_error(int php_err_type)
{
    if (ms_error.code != MS_NOERR)
    {
        php3_error(php_err_type, 
                   "MapServer Error in %s: %s\n", 
                   ms_error.routine, ms_error.message);
    }
}


/**********************************************************************
 *                     _phpms_fetch_handle2()
 **********************************************************************/
static void *_phpms_fetch_handle2(pval *pObj, 
                                 int handle_type1, int handle_type2, 
                                 HashTable *list)
{
    pval *phandle;
    void *retVal = NULL;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(E_ERROR, "Object expected as argument.");
        retVal = NULL;
    }
    else if (_php3_hash_find(pObj->value.ht, "_handle_", sizeof("_handle_"), 
                             (void **)&phandle) == FAILURE)
    {
        php3_error(E_ERROR, 
                   "Unable to find _handle_ property");
        retVal = NULL;
    }
    else
    {
        int type;
        retVal = (void *)php3_list_find(phandle->value.lval, &type);
        if (retVal == NULL || (type != handle_type1 && type != handle_type2))
        {
            php3_error(E_ERROR, "Object has an invalid _handle_ property");
            retVal = NULL;
        }
    }

    /* Note: because of the php3_error() calls above, this function
     *       will probably never return a NULL value.
     */
    return retVal;    
}

/**********************************************************************
 *                     _phpms_fetch_handle()
 **********************************************************************/
static void *_phpms_fetch_handle(pval *pObj, int handle_type, 
                                 HashTable *list)
{
    return _phpms_fetch_handle2(pObj, handle_type, handle_type, list);
}


/**********************************************************************
 *                     _phpms_fetch_property_handle2()
 **********************************************************************/
static char *_phpms_fetch_property_handle2(pval *pObj, char *property_name, 
                                           int handle_type1, int handle_type2,
                                           HashTable *list, int err_type)
{
    pval *phandle;
    void *retVal = NULL;
    int type;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return NULL;
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return NULL;
    }

    if (phandle->type != IS_LONG ||
        (retVal = (void *)php3_list_find(phandle->value.lval, &type)) ==NULL ||
        (type != handle_type1 && type != handle_type2))
    {
        if (err_type != 0)
            php3_error(err_type, "Object has an invalid '%s' property", 
                       property_name);
        retVal = NULL;
    }

    return retVal;
}

/**********************************************************************
 *                     _phpms_fetch_property_handle()
 **********************************************************************/
static char *_phpms_fetch_property_handle(pval *pObj, char *property_name, 
                                          int handle_type, HashTable *list,
                                          int err_type)
{
    return _phpms_fetch_property_handle2(pObj, property_name, 
                                          handle_type, handle_type, list,
                                          err_type);
}

/**********************************************************************
 *                     _phpms_fetch_property_string()
 **********************************************************************/
static char *_phpms_fetch_property_string(pval *pObj, char *property_name, 
                                          int err_type)
{
    pval *phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return "";
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return "";
    }

    convert_to_string(phandle);
    return phandle->value.str.val;
}

/**********************************************************************
 *                     _phpms_fetch_property_long()
 **********************************************************************/
static long _phpms_fetch_property_long(pval *pObj, char *property_name, 
                                       int err_type)
{
    pval *phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0;
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0;
    }

    convert_to_long(phandle);
    return phandle->value.lval;
}

/**********************************************************************
 *                     _phpms_fetch_property_double()
 **********************************************************************/
static double _phpms_fetch_property_double(pval *pObj, char *property_name,
                                           int err_type)
{
    pval *phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0.0;
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1,
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0.0;
    }

    convert_to_double(phandle);
    return phandle->value.dval;
}

/**********************************************************************
 *                     _phpms_set_property_string()
 **********************************************************************/
static int _phpms_set_property_string(pval *pObj, char *property_name, 
                                      char *szNewValue, int err_type)
{
    pval *phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

    convert_to_string(phandle);
    if (phandle->value.str.val)
        efree(phandle->value.str.val);
    phandle->value.str.val = estrdup(szNewValue);
    phandle->value.str.len = strlen(szNewValue);

    return 0;
}

/**********************************************************************
 *                     _phpms_set_property_long()
 **********************************************************************/
static int _phpms_set_property_long(pval *pObj, char *property_name, 
                                    long lNewValue, int err_type)
{
    pval *phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

    convert_to_long(phandle);
    phandle->value.lval = lNewValue;

    return 0;
}

/**********************************************************************
 *                     _phpms_set_property_double()
 **********************************************************************/
static int _phpms_set_property_double(pval *pObj, char *property_name, 
                                      double dNewValue, int err_type)
{
    pval *phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

    convert_to_double(phandle);
    phandle->value.dval = dNewValue;

    return 0;
}

/**********************************************************************
 *                     _phpms_add_property_object()
 **********************************************************************/
static int _phpms_add_property_object(pval *pObj,
                                      char *property_name, pval *pObjToAdd,
                                      int err_type)
{
    pval *phandle;

    /* This is kind of a hack...
     * We will add a 'long' property, and then we'll replace its contents 
     * with the object that was passed.
     */

    if (pObj->type != IS_OBJECT || pObjToAdd->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (add_property_long(pObj, property_name, 0) == FAILURE ||
             _php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to add %s property", property_name);
        return -1;
    }

    phandle->type = pObjToAdd->type;
    phandle->value.ht = pObjToAdd->value.ht;

    return 0;
}



/*=====================================================================
 *                 PHP function wrappers - mapObj class
 *====================================================================*/

/**********************************************************************
 *                        ms_newMapObj()
 **********************************************************************/

/* {{{ proto mapObj ms_newMapObj(string filename)
   Returns a new object to deal with a MapServer map file. */

DLEXPORT void php3_ms_map_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pFname;
    mapObj      *pNewObj = NULL;
    int         map_id;
    pval        new_obj_param;  /* No, it's not a pval * !!! */

    if (getParameters(ht, 1, &pFname) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    /* Attempt to open the MAP file 
     */
    convert_to_string(pFname);
    pNewObj = mapObj_new(pFname->value.str.val);
    if (pNewObj == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed to open map file %s", 
                            pFname->value.str.val);
        RETURN_FALSE;
    }

    /* Create a PHP object, add all mapObj methods, etc.
     */
    object_init(return_value);

    map_id = php3_list_insert(pNewObj, PHPMS_GLOBAL(le_msmap));
    add_property_long(return_value, "_handle_", map_id);

    /* read-only properties */
    add_property_long(return_value, "numlayers", pNewObj->numlayers);

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value, "name",      pNewObj->name);
    add_property_long(return_value,  "status",    pNewObj->status);
    add_property_long(return_value,  "width",     pNewObj->width);
    add_property_long(return_value,  "height",    pNewObj->height);
    add_property_long(return_value,  "transparent", pNewObj->transparent);
    add_property_long(return_value,  "interlace", pNewObj->interlace);

    _phpms_build_rect_object(&(pNewObj->extent), PHPMS_GLOBAL(le_msrect_ref),
                             list, &new_obj_param);
    _phpms_add_property_object(return_value, "extent", &new_obj_param,E_ERROR);
    
    add_property_double(return_value,"cellsize",  pNewObj->cellsize);
    add_property_double(return_value,"scale",     pNewObj->scale);
    PHPMS_ADD_PROP_STR(return_value, "shapepath", pNewObj->shapepath);
    PHPMS_ADD_PROP_STR(return_value, "tile",      pNewObj->tile);
    add_property_long(return_value,  "keysizex",  pNewObj->legend.keysizex);
    add_property_long(return_value,  "keysizey",  pNewObj->legend.keysizey);
    add_property_long(return_value, "keyspacingx",pNewObj->legend.keyspacingx);
    add_property_long(return_value, "keyspacingy",pNewObj->legend.keyspacingy);

    _phpms_build_color_object(&(pNewObj->imagecolor),list, &new_obj_param);
    _phpms_add_property_object(return_value, 
                               "imagecolor", &new_obj_param,E_ERROR);

    _phpms_build_web_object(&(pNewObj->web), list, &new_obj_param);
    _phpms_add_property_object(return_value, "web", &new_obj_param,E_ERROR);

    _phpms_build_referenceMap_object(&(pNewObj->reference), list, 
                                     &new_obj_param);
    _phpms_add_property_object(return_value, 
                               "reference", &new_obj_param, E_ERROR);

    /* methods */
    add_method(return_value, "set",             php3_ms_map_setProperty);
    add_method(return_value, "addcolor",        php3_ms_map_addColor);
    add_method(return_value, "prepareimage",    php3_ms_map_prepareImage);
    add_method(return_value, "draw",            php3_ms_map_draw);
    add_method(return_value, "drawlabelcache",  php3_ms_map_drawLabelCache);
    add_method(return_value, "drawlegend",      php3_ms_map_drawLegend);
    add_method(return_value, "drawreferencemap",php3_ms_map_drawReferenceMap);
    add_method(return_value, "drawscalebar",    php3_ms_map_drawScaleBar);
    add_method(return_value, "getlayer",        php3_ms_map_getLayer);
    add_method(return_value, "getlayerbyname",  php3_ms_map_getLayerByName);
#ifdef __TODO__
    add_method(return_value, "getsymbolbyname", php3_ms_map_getSymbolByName);
    add_method(return_value, "nextlabel",       php3_ms_map_nextLabel);
#endif
    add_method(return_value, "getcolorbyindex",  php3_ms_map_getColorByIndex);

    add_method(return_value, "setextent",       php3_ms_map_setExtent);
    add_method(return_value, "zoompoint",       php3_ms_map_zoomPoint);
    add_method(return_value, "zoomrectangle",   php3_ms_map_zoomRectangle);
    
    add_method(return_value, "queryusingpoint", php3_ms_map_queryUsingPoint);
    add_method(return_value, "queryusingrect",  php3_ms_map_queryUsingRect);

    add_method(return_value, "save",            php3_ms_map_save);

    return;
}
/* }}} */


/**********************************************************************
 *                        map->set()
 **********************************************************************/

/* {{{ proto int map.set(string property_name, new_value)
   Set map object property to new value. Returns -1 on error. */

DLEXPORT void php3_ms_map_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);


    IF_SET_STRING(     "name",        self->name)
    else IF_SET_LONG(  "status",      self->status)
    else IF_SET_LONG(  "width",       self->width)
    else IF_SET_LONG(  "height",      self->height)
    else IF_SET_LONG(  "transparent", self->transparent)
    else IF_SET_LONG(  "interlace",   self->interlace)
    else IF_SET_DOUBLE("cellsize",    self->cellsize)
    else IF_SET_DOUBLE("scale",       self->scale)
    else IF_SET_STRING("shapepath",   self->shapepath)
    else IF_SET_STRING("tile",        self->tile)
    else IF_SET_LONG(  "keysizex",    self->legend.keysizex)
    else IF_SET_LONG(  "keysizey",    self->legend.keysizey)
    else IF_SET_LONG(  "keyspacingx", self->legend.keyspacingx)
    else IF_SET_LONG(  "keyspacingy", self->legend.keyspacingy)
    else if (strcmp( "numlayers", pPropertyName->value.str.val) == 0 ||
             strcmp( "extent", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        map->setExtent()
 **********************************************************************/

/* {{{ proto int map.setextent(string property_name, new_value)
   Set map extent property to new value. Returns -1 on error. */

DLEXPORT void php3_ms_map_setExtent(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pThis, *pExtent;
    pval   *pMinX, *pMinY, *pMaxX, *pMaxY;
    

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 4, &pMinX, &pMinY, &pMaxX, &pMaxY) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pMinX);
    convert_to_double(pMinY);
    convert_to_double(pMaxX);
    convert_to_double(pMaxY);

    self->extent.minx = pMinX->value.dval;
    self->extent.miny = pMinY->value.dval;
    self->extent.maxx = pMaxX->value.dval;
    self->extent.maxy = pMaxY->value.dval;
    
    self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                    self->height);      
    self->scale = msCalculateScale(*(&(self->extent)), self->units, 
                                   self->width, self->height);

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR); 
    _phpms_set_property_double(pThis,"scale", self->scale, E_ERROR); 

    if (_php3_hash_find(pThis->value.ht, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double(pExtent,"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxy", self->extent.maxy, 
                                   E_ERROR);
    }
   

}

/* ==================================================================== */
/*      Zoom related functions.                                         */
/* ==================================================================== */

/************************************************************************/
/*    static double Pix2Georef(int nPixPos, int nPixMin, double nPixMax,*/
/*                              double dfGeoMin, double dfGeoMax,       */
/*                              bool bULisYOrig)                        */
/*                                                                      */
/*        Utility function to convert a pixel pos to georef pos. If     */
/*      bULisYOrig parameter is set to true then the upper left is      */
/*      considered to be the Y origin.                                  */
/*                                                                      */
/************************************************************************/
static double Pix2Georef(int nPixPos, int nPixMin, int nPixMax, 
                         double dfGeoMin, double dfGeoMax, int bULisYOrig)

{
    double      dfWidthGeo = 0.0;
    int         nWidthPix = 0;
    double      dfPixToGeo = 0.0;
    double      dfPosGeo = 0.0;
    double      dfDeltaGeo = 0.0;
    int         nDeltaPix = 0;

    dfWidthGeo = dfGeoMax - dfGeoMin;
    nWidthPix = nPixMax - nPixMin;
   
    if (dfWidthGeo > 0.0 && nWidthPix > 0)
    {
        dfPixToGeo = dfWidthGeo / (double)nWidthPix;

        if (!bULisYOrig)
            nDeltaPix = nPixPos - nPixMin;
        else
            nDeltaPix = nPixMax - nPixPos;
        
        dfDeltaGeo = nDeltaPix * dfPixToGeo;

        dfPosGeo = dfGeoMin + dfDeltaGeo;
    }
    return (dfPosGeo);
}

/************************************************************************/
/*                           map->zoomPoint()                           */
/*                                                                      */
/*      Parmeters are :                                                 */
/*                                                                      */
/*       - Zoom factor : positive values do zoom in, negative values    */
/*                      zoom out. Factor of 1 will recenter.            */
/*                                                                      */
/*       - Pixel position (pointObj) : x, y coordinates of the click.   */
/*       - Width : width in pixel of the current image.                 */
/*       - Height : Height in pixel of the current image.               */
/*       - Georef extent (rectObj) : current georef extents.            */
/*       - MaxGeoref extent (rectObj) : (optional) maximum georef       */
/*                                      extents.                        */
/*                                                                      */
/************************************************************************/
DLEXPORT void php3_ms_map_zoomPoint(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj      *self;
    pval        *pThis, *pExtent;
    pval        *pZoomFactor;
    pval        *pPixelPos;
    pval        *pWidth, *pHeight;
    pval        *pGeorefExt;
    pval        *pMaxGeorefExt;
    rectObj     *poGeorefExt = NULL;
    rectObj     *poMaxGeorefExt = NULL;
    pointObj    *poPixPos = NULL;
    double      dfGeoPosX, dfGeoPosY;
    double      dfDeltaX, dfDeltaY;
    rectObj     oNewGeorefExt;    
    double      dfNewScale = 0.0;
    int         bMaxExtSet = 0;
    int         nArgs = ARG_COUNT(ht);
    double      dfDeltaExt = -1.0;

    if (getThis(&pThis) == FAILURE ||   
        (nArgs != 5 && nArgs != 6))
    {
        WRONG_PARAM_COUNT;
    }
    
    if (getParameters(ht, nArgs, 
                      &pZoomFactor, &pPixelPos, &pWidth, &pHeight,
                      &pGeorefExt, &pMaxGeorefExt) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    if (nArgs == 6)
        bMaxExtSet =1;

    convert_to_long(pZoomFactor);
    convert_to_long(pWidth);
    convert_to_long(pHeight);
         
    poGeorefExt = (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                                  PHPMS_GLOBAL(le_msrect_ref),
                                                  PHPMS_GLOBAL(le_msrect_new),
                                                  list);
    poPixPos = (pointObj *)_phpms_fetch_handle2(pPixelPos, 
                                                PHPMS_GLOBAL(le_mspoint_new), 
                                                PHPMS_GLOBAL(le_mspoint_ref),
                                                list);
    
    if (bMaxExtSet)
        poMaxGeorefExt = 
            (rectObj *)_phpms_fetch_handle2(pMaxGeorefExt, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list);

/* -------------------------------------------------------------------- */
/*      check the validity of the parameters.                           */
/* -------------------------------------------------------------------- */
    if (pZoomFactor->value.lval == 0 || 
        pWidth->value.lval <= 0 ||
        pHeight->value.lval <= 0 ||
        poGeorefExt == NULL ||
        poPixPos == NULL ||
        (bMaxExtSet && poMaxGeorefExt == NULL))
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomPoint failed : incorrect parameters\n");
        RETURN_FALSE;
    }
    
    dfGeoPosX = Pix2Georef((int)poPixPos->x, 0, (int)pWidth->value.lval, 
                           poGeorefExt->minx, poGeorefExt->maxx, 0); 
    dfGeoPosY = Pix2Georef((int)poPixPos->y, 0, (int)pHeight->value.lval, 
                           poGeorefExt->miny, poGeorefExt->maxy, 1); 
    dfDeltaX = poGeorefExt->maxx - poGeorefExt->minx;
    dfDeltaY = poGeorefExt->maxy - poGeorefExt->miny;

   
/* -------------------------------------------------------------------- */
/*      zoom in                                                         */
/* -------------------------------------------------------------------- */
    if (pZoomFactor->value.lval > 1)
    {
        oNewGeorefExt.minx = 
            dfGeoPosX - (dfDeltaX/(2*pZoomFactor->value.lval));        
         oNewGeorefExt.miny = 
             dfGeoPosY - (dfDeltaY/(2*pZoomFactor->value.lval));        
         oNewGeorefExt.maxx = 
             dfGeoPosX + (dfDeltaX/(2*pZoomFactor->value.lval));        
         oNewGeorefExt.maxy = 
             dfGeoPosY + (dfDeltaY/(2*pZoomFactor->value.lval));
    }

    if (pZoomFactor->value.lval < 0)
    {
        oNewGeorefExt.minx = 
            dfGeoPosX - (dfDeltaX/2)*(abs(pZoomFactor->value.lval));    
        oNewGeorefExt.miny = 
            dfGeoPosY - (dfDeltaY/2)*(abs(pZoomFactor->value.lval));    
        oNewGeorefExt.maxx = 
            dfGeoPosX + (dfDeltaX/2)*(abs(pZoomFactor->value.lval));    
        oNewGeorefExt.maxy = 
            dfGeoPosY + (dfDeltaY/2)*(abs(pZoomFactor->value.lval));
    }
    if (pZoomFactor->value.lval == 1)
    {
        oNewGeorefExt.minx = dfGeoPosX - (dfDeltaX/2);
        oNewGeorefExt.miny = dfGeoPosY - (dfDeltaY/2);
        oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaX/2);
        oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaY/2);
    }

/* -------------------------------------------------------------------- */
/*      if the min and max scale are set in the map file, we will       */
/*      use them to test before zooming.                                */
/* -------------------------------------------------------------------- */
    dfNewScale =  msCalculateScale(oNewGeorefExt, self->units, 
                                   self->width, self->height);

    if (self->web.maxscale > 0)
    {
        if (pZoomFactor->value.lval < 0 && dfNewScale >  self->web.maxscale)
        {
            RETURN_FALSE;
        }
    }

/* ==================================================================== */
/*      we do a spcial case for zoom in : we try to zoom as much as     */
/*      possible using the mincale set in the .map.                     */
/* ==================================================================== */
    if (self->web.minscale > 0 && dfNewScale <  self->web.minscale &&
        pZoomFactor->value.lval > 1)
    {
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscale, self->units, 
                                      self->width);
        if (dfDeltaExt > 0.0)
        {
            oNewGeorefExt.minx = dfGeoPosX - (dfDeltaExt/2);
            oNewGeorefExt.miny = dfGeoPosY - (dfDeltaExt/2);
            oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaExt/2);
            oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaExt/2);
        }
        else
          RETURN_FALSE;
    }
/* -------------------------------------------------------------------- */
/*      If the buffer is set, make sure that the extents do not go      */
/*      beyond the buffer.                                              */
/* -------------------------------------------------------------------- */
    if (bMaxExtSet)
    {
        dfDeltaX = oNewGeorefExt.maxx - oNewGeorefExt.minx;
        dfDeltaY = oNewGeorefExt.maxy - oNewGeorefExt.miny;
        
        /* Make sure Current georef extents is not bigger than max extents */
        if (dfDeltaX > (poMaxGeorefExt->maxx-poMaxGeorefExt->minx))
            dfDeltaX = poMaxGeorefExt->maxx-poMaxGeorefExt->minx;
        if (dfDeltaY > (poMaxGeorefExt->maxy-poMaxGeorefExt->miny))
            dfDeltaY = poMaxGeorefExt->maxy-poMaxGeorefExt->miny;

        if (oNewGeorefExt.minx < poMaxGeorefExt->minx)
        {
            oNewGeorefExt.minx = poMaxGeorefExt->minx;
            oNewGeorefExt.maxx =  oNewGeorefExt.minx + dfDeltaX;
        }
        if (oNewGeorefExt.maxx > poMaxGeorefExt->maxx)
        {
            oNewGeorefExt.maxx = poMaxGeorefExt->maxx;
            oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
        }
        if (oNewGeorefExt.miny < poMaxGeorefExt->miny)
        {
            oNewGeorefExt.miny = poMaxGeorefExt->miny;
            oNewGeorefExt.maxy =  oNewGeorefExt.miny + dfDeltaY;
        }
        if (oNewGeorefExt.maxy > poMaxGeorefExt->maxy)
        {
            oNewGeorefExt.maxy = poMaxGeorefExt->maxy;
            oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
        }
    }
    
/* -------------------------------------------------------------------- */
/*      set the map extents with new values.                            */
/* -------------------------------------------------------------------- */
    self->extent.minx = oNewGeorefExt.minx;
    self->extent.miny = oNewGeorefExt.miny;
    self->extent.maxx = oNewGeorefExt.maxx;
    self->extent.maxy = oNewGeorefExt.maxy;
    
    self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                    self->height);      
    self->scale = msCalculateScale(*(&(self->extent)), self->units, 
                                   self->width, self->height);

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR); 
    _phpms_set_property_double(pThis,"scale", self->scale, E_ERROR); 

    if (_php3_hash_find(pThis->value.ht, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double(pExtent,"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxy", self->extent.maxy, 
                                   E_ERROR);
    }

     RETURN_TRUE;

}


/************************************************************************/
/*                           map->zoomRectange()                        */
/*                                                                      */
/*      Parmeters are :                                                 */
/*                                                                      */
/*       - Pixel position (rectObj) : extents in pixels.                */
/*       - Width : width in pixel of the current image.                 */
/*       - Height : Height in pixel of the current image.               */
/*       - Georef extent (rectObj) : current georef extents.            */
/*                                                                      */
/************************************************************************/
DLEXPORT void php3_ms_map_zoomRectangle(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj      *self;
    pval        *pThis, *pExtent;
    pval        *pPixelExt;
    pval        *pWidth, *pHeight;
    pval        *pGeorefExt;
    rectObj     *poGeorefExt = NULL;
    rectObj     *poPixExt = NULL;
    double      dfNewScale = 0.0;
    rectObj     oNewGeorefExt;    
    double      dfMiddleX =0.0;
    double      dfMiddleY =0.0;
    double      dfDeltaExt =0.0;
    
    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 4, &pPixelExt, &pWidth, &pHeight,
                      &pGeorefExt) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL)
    {
        RETURN_FALSE;
    }

    convert_to_long(pWidth);
    convert_to_long(pHeight);
    
    poGeorefExt = (rectObj *)_phpms_fetch_handle2(pGeorefExt, 
                                                  PHPMS_GLOBAL(le_msrect_ref),
                                                  PHPMS_GLOBAL(le_msrect_new),
                                                  list);
    poPixExt = (rectObj *)_phpms_fetch_handle2(pPixelExt, 
                                               PHPMS_GLOBAL(le_msrect_ref), 
                                               PHPMS_GLOBAL(le_msrect_new),
                                               list);

/* -------------------------------------------------------------------- */
/*      check the validity of the parameters.                           */
/* -------------------------------------------------------------------- */
    if (pWidth->value.lval <= 0 ||
        pHeight->value.lval <= 0 ||
        poGeorefExt == NULL ||
        poPixExt == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "zoomRectangle failed : incorrect parameters\n");
            /*RETURN_FALSE;*/
    }
    

    oNewGeorefExt.minx = Pix2Georef((int)poPixExt->minx, 0, 
                                    (int)pWidth->value.lval, 
                                    poGeorefExt->minx, poGeorefExt->maxx, 0); 
    oNewGeorefExt.maxx = Pix2Georef((int)poPixExt->maxx, 0, 
                                    (int)pWidth->value.lval, 
                                    poGeorefExt->minx, poGeorefExt->maxx, 0); 
    oNewGeorefExt.miny = Pix2Georef((int)poPixExt->miny, 0, 
                                    (int)pHeight->value.lval, 
                                    poGeorefExt->miny, poGeorefExt->maxy, 1); 
    oNewGeorefExt.maxy = Pix2Georef((int)poPixExt->maxy, 0, 
                                    (int)pHeight->value.lval, 
                                    poGeorefExt->miny, poGeorefExt->maxy, 1); 
    
 
    dfNewScale =  msCalculateScale(oNewGeorefExt, self->units, 
                                   self->width, self->height);

/* -------------------------------------------------------------------- */
/*      if the min and max scale are set in the map file, we will       */
/*      use them to test before settting extents.                       */
/* -------------------------------------------------------------------- */
    dfNewScale =  msCalculateScale(oNewGeorefExt, self->units, 
                                   self->width, self->height);

    if (self->web.maxscale > 0 &&  dfNewScale > self->web.maxscale)
    {
        RETURN_FALSE;
    }

    if (self->web.minscale > 0 && dfNewScale <  self->web.minscale)
    {
        dfDeltaExt = 
            GetDeltaExtentsUsingScale(self->web.minscale, self->units, 
                                      self->width);
        dfMiddleX = oNewGeorefExt.minx + 
            ((oNewGeorefExt.maxx - oNewGeorefExt.minx)/2);
        dfMiddleY = oNewGeorefExt.miny + 
            ((oNewGeorefExt.maxy - oNewGeorefExt.miny)/2);
        
        if (dfDeltaExt > 0.0)
        {
            oNewGeorefExt.minx = dfMiddleX - (dfDeltaExt/2);
            oNewGeorefExt.miny = dfMiddleY - (dfDeltaExt/2);
            oNewGeorefExt.maxx = dfMiddleX + (dfDeltaExt/2);
            oNewGeorefExt.maxy = dfMiddleY + (dfDeltaExt/2);
        }
        else
          RETURN_FALSE;
    }
    self->extent.minx = oNewGeorefExt.minx;
    self->extent.miny = oNewGeorefExt.miny;
    self->extent.maxx = oNewGeorefExt.maxx;
    self->extent.maxy = oNewGeorefExt.maxy;
    
    self->cellsize = msAdjustExtent(&(self->extent), self->width, 
                                    self->height);      
    self->scale = msCalculateScale(*(&(self->extent)), self->units, 
                                   self->width, self->height);

    _phpms_set_property_double(pThis,"cellsize", self->cellsize, E_ERROR); 
    _phpms_set_property_double(pThis,"scale", self->scale, E_ERROR); 

    if (_php3_hash_find(pThis->value.ht, "extent", sizeof("extent"), 
                        (void **)&pExtent) == SUCCESS)
    {
        _phpms_set_property_double(pExtent,"minx", self->extent.minx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"miny", self->extent.miny, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxx", self->extent.maxx, 
                                   E_ERROR);
        _phpms_set_property_double(pExtent,"maxy", self->extent.maxy, 
                                   E_ERROR);
    }

    RETURN_TRUE;
}

/**********************************************************************
 *                        map->addColor()
 **********************************************************************/

/* {{{ proto int map.addColor(int r, int g, int b)
   Add a color to map's palette.  Returns color index.*/

DLEXPORT void php3_ms_map_addColor(INTERNAL_FUNCTION_PARAMETERS)
{
    mapObj *self;
    pval   *pR, *pG, *pB, *pThis;
    int     nColorId = 0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 3, &pR, &pG, &pB) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || 
        (nColorId = mapObj_addColor(self, pR->value.lval, 
                                    pG->value.lval, pB->value.lval)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(nColorId);
}
/* }}} */

/**********************************************************************
 *                        map->prepareImage()
 **********************************************************************/

/* {{{ proto int map.prepareImage()
   Return handle on blank image object. */

DLEXPORT void php3_ms_map_prepareImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_prepareImage(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->draw()
 **********************************************************************/

/* {{{ proto int map.draw()
   Render map and return handle on image object. */

DLEXPORT void php3_ms_map_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_draw(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->drawLegend()
 **********************************************************************/

/* {{{ proto int map.drawLegend()
   Render legend and return handle on image object. */

DLEXPORT void php3_ms_map_drawLegend(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_drawLegend(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->drawReferenceMap()
 **********************************************************************/

/* {{{ proto int map.drawReferenceMap()
   Render reference map and return handle on image object. */

DLEXPORT void php3_ms_map_drawReferenceMap(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_drawReferenceMap(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->drawScaleBar()
 **********************************************************************/

/* {{{ proto int map.drawScaleBar()
   Render scale bar and return handle on image object. */

DLEXPORT void php3_ms_map_drawScaleBar(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    mapObj *self;
    gdImagePtr im = NULL;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (im = mapObj_drawScalebar(self)) == NULL)
        _phpms_report_mapserver_error(E_ERROR);

    /* Return an image object */
    _phpms_build_img_object(im, &(self->web), list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->drawLabelCache()
 **********************************************************************/

/* {{{ proto int map.drawLabelCache(image img)
   Renders the labels for a map. Returns -1 on error. */

DLEXPORT void php3_ms_map_drawLabelCache(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *imgObj, *pThis;
    mapObj *self;
    gdImagePtr im = NULL;
    int    retVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &imgObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }
        
    im = (gdImagePtr)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || (retVal = mapObj_drawLabelCache(self, im)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        map->getLayer()
 *
 * Note: Multiple calls to getlayer() will return multiple instances
 * of PHP objects pointing to the same layerObj... this is safe but is a
 * waste of resources.
 **********************************************************************/

/* {{{ proto int map.getLayer(int i)
   Returns a layerObj from the map given an index value (0=first layer, etc.) */

DLEXPORT void php3_ms_map_getLayer(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pLyrIndex, *pThis;
    mapObj *self=NULL;
    layerObj *newLayer=NULL;
    int map_id;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pLyrIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    /* pLyrIndex is the 0-based index of the requested layer */
    convert_to_long(pLyrIndex);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self == NULL || 
        (newLayer = mapObj_getLayer(self, pLyrIndex->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* We will store a reference to the parent object id (this) inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_long(pThis, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_layer_object(newLayer, map_id, list, return_value);
}
/* }}} */

/**********************************************************************
 *                        map->getLayerByName()
 *
 * Note: Multiple calls to getlayer() will return multiple instances
 * of PHP objects pointing to the same layerObj... this is safe but is a
 * waste of resources.
 **********************************************************************/

/* {{{ proto int map.getLayerByName(string layer_name)
   Returns a layerObj from the map given a layer name */

DLEXPORT void php3_ms_map_getLayerByName(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pLyrName, *pThis;
    mapObj *self=NULL;
    layerObj *newLayer=NULL;
    int map_id;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pLyrName) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pLyrName);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        newLayer = mapObj_getLayerByName(self, pLyrName->value.str.val);
        if (newLayer == NULL)
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "getLayerByName failed for : %s\n",
                       pLyrName->value.str.val);
        }
    }
    else
    {
        RETURN_FALSE;
    }

    /* We will store a reference to the parent object id (this) inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_long(pThis, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_layer_object(newLayer, map_id, list, return_value);
}
/* }}} */


/************************************************************************/
/*                         map->getColorByIndex                         */
/*                                                                      */
/*      Returns a color object (r,g,b values) using the color index     */
/*      given in argument.                                              */
/*      The color index corresponds to the index in the internal        */
/*      palette array used by the map to store the colors.              */
/************************************************************************/
/* {{{ proto int map.getColorByIndex(int nColorIndex)
   Returns a colorObj from the map given a color index */

DLEXPORT void php3_ms_map_getColorByIndex(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval        *pColorIndex, *pThis;
    mapObj      *self=NULL;
    paletteObj  palette;
    colorObj    oColor;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pColorIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pColorIndex);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    if (self != NULL)
    {
        palette = self->palette;

        if (pColorIndex->value.lval < palette.numcolors)
        {
            oColor.red = palette.colors[(int)pColorIndex->value.lval].red;
            oColor.green = palette.colors[(int)pColorIndex->value.lval].green;
            oColor.blue = palette.colors[(int)pColorIndex->value.lval].blue;
        }
        else
        {
            _phpms_report_mapserver_error(E_WARNING);
            php3_error(E_ERROR, "getColorByIndex failed"\
                       "Index shoud not be higher than %d\n",
                       palette.numcolors-1);
        }
    }
    else
    {
        RETURN_FALSE;
    }

    /* Return color object */
    _phpms_build_color_object(&oColor, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        map->queryUsingPoint()
 *
 * Type is MS_SINGLE or MS_MULTIPLE depending on number of results
 * you want. Passing buffer <=0 defaults to tolerances set in the map 
 * file (in pixels) but you can use a constant buffer (specified in 
 * ground units) instead.
 **********************************************************************/

/* {{{ proto queryResultObj map.queryUsingPoint(pointObj point, int type, double buffer)
   Query at point location. */

DLEXPORT void php3_ms_map_queryUsingPoint(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pPoint, *pType, *pBuffer;
    mapObj *self=NULL;
    pointObj *poPoint=NULL;
    queryResultObj *poResult=NULL;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 3, &pPoint, &pType, &pBuffer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);
    convert_to_double(pBuffer);

    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list);

    if (self && poPoint && 
        (poResult=mapObj_queryUsingPoint(self, poPoint, pType->value.lval, 
                                         pBuffer->value.dval)) != NULL)
    {
        /* Found something... return queryResult object */
        _phpms_build_queryresult_object(poResult, list, return_value);
        return;
    }

    /* Nothing found... we should produce no error and just return 0 */
    RETURN_LONG(0);
}
/* }}} */


/************************************************************************/
/*                        map->queryUsingRect()                         */
/*                                                                      */
/*      Parmeters :                                                     */
/*                                                                      */
/*       rectObj : poRect : extents used to make the query.             */
/*                                                                      */
/************************************************************************/


/* {{{ proto queryResultObj map.queryUsingRect(rectObj poRect) */
 

DLEXPORT void php3_ms_map_queryUsingRect(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pRect;
    mapObj *self=NULL;
    rectObj *poRect=NULL;
    queryResultObj *poResult=NULL;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pRect) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }


    self = (mapObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msmap), list);
    poRect = (rectObj *)_phpms_fetch_handle2(pRect,
                                             PHPMS_GLOBAL(le_msrect_ref),
                                             PHPMS_GLOBAL(le_msrect_new),
                                             list);

    if (self && poRect && 
        (poResult=mapObj_queryUsingRect(self, poRect)) != NULL)
    {
        /* Found something... return queryResult object */
        _phpms_build_queryresult_object(poResult, list, return_value);
        return;
    }

    /* Nothing found... we should produce no error and just return 0 */
    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        map->save()
 **********************************************************************/

/* {{{ proto int map.save(string filename)
   Write map object to a file. */

DLEXPORT void php3_ms_map_save(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pThis, *pFname;
    mapObj *self;
    int    retVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pFname) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFname);

    self = (mapObj *)_phpms_fetch_handle(pThis, le_msmap, list);
    if (self == NULL || 
        (retVal = mapObj_save(self, pFname->value.str.val)) != 0)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(retVal);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - image class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_img_object()
 **********************************************************************/
static long _phpms_build_img_object(gdImagePtr im, webObj *pweb,
                                    HashTable *list, pval *return_value)
{
    int img_id;

    if (im == NULL)
        return 0;

    img_id = php3_list_insert(im, PHPMS_GLOBAL(le_msimg));

    object_init(return_value);
    add_property_long(return_value, "_handle_", img_id);
    add_method(return_value, "saveimage", php3_ms_img_saveImage);
    add_method(return_value, "savewebimage", php3_ms_img_saveWebImage);
    add_method(return_value, "free", php3_ms_img_free);

    /* width/height params are read-only */
    add_property_long(return_value, "width", gdImageSX(im));
    add_property_long(return_value, "height", gdImageSY(im));
    
    if (pweb)
    {
        PHPMS_ADD_PROP_STR(return_value, "imagepath", pweb->imagepath);
        PHPMS_ADD_PROP_STR(return_value, "imageurl", pweb->imageurl);
    }

/* php3_printf("Create image: id=%d, ptr=0x%x<P>\n", img_id, im);*/

    return img_id;
}

/**********************************************************************
 *                        image->saveImage()
 **********************************************************************/

/* {{{ proto int img.saveImage(string filename, int transparent, int interlace)
   Writes image object to specifed filename. Returns -1 on error. */

DLEXPORT void php3_ms_img_saveImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval   *pFname, *pTransparent, *pInterlace, *pThis;
    gdImagePtr im = NULL;
    int retVal = 0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 3, &pFname, &pTransparent, &pInterlace) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pFname);
    convert_to_long(pTransparent);
    convert_to_long(pInterlace);

    im = (gdImagePtr)_phpms_fetch_handle(pThis, le_msimg, list);

    if (im == NULL ||
        (retVal = msSaveImage(im, pFname->value.str.val, 
                              pTransparent->value.lval, 
                              pInterlace->value.lval) ) != 0)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed writing image to %s", 
                   pFname->value.str.val);
    }

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        image->saveWebImage()
 **********************************************************************/

/* {{{ proto int img.saveWebImage(int transparent, int interlace)
   Writes image to temp directory.  Returns image URL. */

DLEXPORT void php3_ms_img_saveWebImage(INTERNAL_FUNCTION_PARAMETERS)
{
    pval   *pTransparent, *pInterlace, *pThis;
    gdImagePtr im = NULL;
    char *pImagepath, *pImageurl, *pBuf;
    int nBufSize, nLen1, nLen2;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pTransparent, &pInterlace) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pTransparent);
    convert_to_long(pInterlace);

    im = (gdImagePtr)_phpms_fetch_handle(pThis, le_msimg, list);
    pImagepath = _phpms_fetch_property_string(pThis, "imagepath", E_ERROR);
    pImageurl = _phpms_fetch_property_string(pThis, "imageurl", E_ERROR);

    /* Build a unique filename in the IMAGEPATH directory 
     */
    nLen1 = strlen(pImagepath);
    nLen2 = strlen(pImageurl);
    nBufSize = (nLen1>nLen2 ? nLen1:nLen2) + strlen(tmpId) + 30;
    pBuf = (char*)emalloc(nBufSize);
    tmpCount++;
    sprintf(pBuf, "%s%s%d.%s", pImagepath, tmpId, tmpCount, MS_IMAGE_TYPE);

    /* Save the image... 
     */
    if (im == NULL || msSaveImage(im, pBuf, pTransparent->value.lval, 
                                  pInterlace->value.lval) != 0)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed writing image to %s", 
                   pBuf);
    }

    /* ... and return the corresponding URL
     */
    sprintf(pBuf, "%s%s%d.%s", pImageurl, tmpId, tmpCount, MS_IMAGE_TYPE);
    RETURN_STRING(pBuf, 0);
}
/* }}} */


/**********************************************************************
 *                        image->free()
 **********************************************************************/

/* {{{ proto int img.free()
   Destroys resources used by an image object */

DLEXPORT void php3_ms_img_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    gdImagePtr self;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (gdImagePtr)_phpms_fetch_handle(pThis, le_msimg, list);
    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)
        {
            php3_list_delete(phandle->value.lval);
        }
    }

}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - layerObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_layer_object()
 **********************************************************************/
static long _phpms_build_layer_object(layerObj *player, int parent_map_id,
                                      HashTable *list, pval *return_value)
{
    int layer_id;

    if (player == NULL)
    {
        return 0;
    }
    
    layer_id = php3_list_insert(player, PHPMS_GLOBAL(le_mslayer));

    object_init(return_value);
    add_property_long(return_value, "_handle_", layer_id);
    add_property_long(return_value, "_map_handle_", parent_map_id);

    /* read-only properties */
    add_property_long(return_value,   "numclasses", player->numclasses);
    add_property_long(return_value,   "index",      player->index);

    /* editable properties */
    add_property_long(return_value,   "status",     player->status);
    PHPMS_ADD_PROP_STR(return_value,  "classitem",  player->classitem);
    PHPMS_ADD_PROP_STR(return_value,  "name",       player->name);
    PHPMS_ADD_PROP_STR(return_value,  "group",      player->group);
    PHPMS_ADD_PROP_STR(return_value,  "description",player->description);
    PHPMS_ADD_PROP_STR(return_value,  "legend",     player->legend);
    PHPMS_ADD_PROP_STR(return_value,  "data",       player->data);
    add_property_long(return_value,   "type",       player->type);
    add_property_double(return_value, "tolerance",  player->tolerance);
    add_property_long(return_value,   "toleranceunits",player->toleranceunits);
    add_property_double(return_value, "symbolscale",player->symbolscale);
    add_property_double(return_value, "minscale",   player->minscale);
    add_property_double(return_value, "maxscale",   player->maxscale);
    add_property_double(return_value, "labelminscale",player->labelminscale);
    add_property_double(return_value, "labelmaxscale",player->labelmaxscale);
    add_property_long(return_value,   "maxfeatures",player->maxfeatures);
    add_property_long(return_value,   "offsite",    player->offsite);
    add_property_long(return_value,   "transform",  player->transform);
    add_property_long(return_value,   "labelcache", player->labelcache);
    PHPMS_ADD_PROP_STR(return_value,  "labelitem",  player->labelitem);
    PHPMS_ADD_PROP_STR(return_value,  "labelsizeitem",player->labelsizeitem);
    PHPMS_ADD_PROP_STR(return_value,  "labelangleitem",player->labelangleitem);
    PHPMS_ADD_PROP_STR(return_value,  "tileitem",   player->tileitem);
    PHPMS_ADD_PROP_STR(return_value,  "tileindex",  player->tileindex);
    PHPMS_ADD_PROP_STR(return_value,  "header",     player->header);
    PHPMS_ADD_PROP_STR(return_value,  "footer",     player->footer);


    add_method(return_value, "set",      php3_ms_lyr_setProperty);
    add_method(return_value, "draw",     php3_ms_lyr_draw);
    add_method(return_value, "getclass", php3_ms_lyr_getClass);
    add_method(return_value, "firstfeature", php3_ms_lyr_firstFeature);
    add_method(return_value, "queryusingpoint", php3_ms_lyr_queryUsingPoint);
    add_method(return_value, "queryusingrect", php3_ms_lyr_queryUsingRect);

    return layer_id;
}


/**********************************************************************
 *                        ms_newLayerObj()
 **********************************************************************/

/* {{{ proto layerObj ms_newLayerObj(mapObj map)
   Create a new layer in the specified map. */

DLEXPORT void php3_ms_lyr_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pMapObj;
    mapObj *parent_map;
    layerObj *pNewLayer;
    int map_id;

    if (getParameters(ht, 1, &pMapObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_map = (mapObj*)_phpms_fetch_handle(pMapObj, 
                                              PHPMS_GLOBAL(le_msmap),
                                              list);

    if (parent_map == NULL ||
        (pNewLayer = layerObj_new(parent_map)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* We will store a reference to the parent_map object id inside
     * the layer obj.
     */
    map_id = _phpms_fetch_property_long(pMapObj, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_layer_object(pNewLayer, map_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        layer->set()
 **********************************************************************/

/* {{{ proto int layer.set(string property_name, new_value)
   Set layer object property to new value. Returns -1 on error. */

DLEXPORT void php3_ms_lyr_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    layerObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, le_mslayer, list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_LONG(       "status",     self->status)
    else IF_SET_STRING("classitem",  self->classitem)
    else IF_SET_STRING("name",       self->name)
    else IF_SET_STRING("group",      self->group)
    else IF_SET_STRING("description",self->description)
    else IF_SET_STRING("legend",     self->legend)
    else IF_SET_STRING("data",       self->data)
    else IF_SET_LONG(  "type",       self->type)
    else IF_SET_DOUBLE("tolerance",  self->tolerance)
    else IF_SET_LONG(  "toleranceunits",self->toleranceunits)
    else IF_SET_DOUBLE("symbolscale",self->symbolscale)
    else IF_SET_DOUBLE("minscale",   self->minscale)
    else IF_SET_DOUBLE("maxscale",   self->maxscale)
    else IF_SET_DOUBLE("labelminscale",self->labelminscale)
    else IF_SET_DOUBLE("labelmaxscale",self->labelmaxscale)
    else IF_SET_LONG(  "maxfeatures",self->maxfeatures)
    else IF_SET_LONG(  "offsite",    self->offsite)
    else IF_SET_LONG(  "transform",  self->transform)
    else IF_SET_LONG(  "labelcache", self->labelcache)
    else IF_SET_STRING("labelitem",  self->labelitem)
    else IF_SET_STRING("labelsizeitem",self->labelsizeitem)
    else IF_SET_STRING("labelangleitem",self->labelangleitem)
    else IF_SET_STRING("tileitem",   self->tileitem)
    else IF_SET_STRING("tileindex",  self->tileindex)
    else IF_SET_STRING("header",     self->header)
    else IF_SET_STRING("footer",     self->footer)
    else if (strcmp( "numclasses", pPropertyName->value.str.val) == 0 ||
             strcmp( "index",      pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        layer->draw()
 **********************************************************************/

/* {{{ proto int layer.draw(image img)
   Draw a single layer, add labels to cache if required. Returns -1 on error. */

DLEXPORT void php3_ms_lyr_draw(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *imgObj, *pThis;
    mapObj *parent_map;
    layerObj *self;
    gdImagePtr im = NULL;
    int    retVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &imgObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    im = (gdImagePtr)_phpms_fetch_handle(imgObj, 
                                         PHPMS_GLOBAL(le_msimg), list);
   
    self = (layerObj *)_phpms_fetch_handle(pThis, 
                                           PHPMS_GLOBAL(le_mslayer),list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (im == NULL || self == NULL || parent_map == NULL ||
        (retVal = layerObj_draw(self, parent_map, im)) == -1)
        _phpms_report_mapserver_error(E_ERROR);

    RETURN_LONG(retVal);
}
/* }}} */


/**********************************************************************
 *                        map->getClass()
 *
 * Note: Multiple calls to getClass() will return multiple instances
 * of PHP objects pointing to the same classObj... this is safe but is a
 * waste of resources.
 **********************************************************************/

/* {{{ proto int layer.getClass(int i)
   Returns a classObj from the layer given an index value (0=first class) */

DLEXPORT void php3_ms_lyr_getClass(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval  *pClassIndex, *pThis;
    layerObj *self=NULL;
    classObj *newClass=NULL;
    int layer_id;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pClassIndex) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    /* pClassIndex is the 0-based index of the requested class */
    convert_to_long(pClassIndex);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    if (self == NULL || 
        (newClass = layerObj_getClass(self, pClassIndex->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
    }

    /* We will store a reference to the parent object id (this) inside
     * the class obj.
     */
    layer_id = _phpms_fetch_property_long(pThis, "_handle_", E_ERROR);

    /* Return layer object */
    _phpms_build_class_object(newClass, layer_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        layer->firstFeature()
 **********************************************************************/

/* {{{ proto int layer.firstFeature()
   Returns a reference to the first featureObj in this layer, or 0 if the layer contains no featureObj yet. */

DLEXPORT void php3_ms_lyr_firstFeature(INTERNAL_FUNCTION_PARAMETERS)
{
    pval       *pThis;
    layerObj   *self;
    int         layer_id;

    if (getThis(&pThis) == FAILURE || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj*)_phpms_fetch_handle(pThis,PHPMS_GLOBAL(le_mslayer),list);

    if (self==NULL || self->features == NULL)
    {
        /* Return 0 when we reach the last feature of the list. */
        RETURN_LONG(0);
    }

    layer_id = _phpms_fetch_property_long(pThis, "_handle_", E_ERROR);

    /* Return reference to the feature.  Reference is valid only
     * during the life of this layerObj.
     */
    _phpms_build_feature_object(self->features, layer_id, list, return_value);
}
/* }}} */



/**********************************************************************
 *                        layer->queryUsingPoint()
 *
 * Type is MS_SINGLE or MS_MULTIPLE depending on number of results
 * you want. Passing buffer <=0 defaults to tolerances set in the map 
 * file (in pixels) but you can use a constant buffer (specified in 
 * ground units) instead.
 **********************************************************************/

/* {{{ proto queryResultObj layer.queryUsingPoint(pointObj point, int type, double buffer)
   Query at point location. */

DLEXPORT void php3_ms_lyr_queryUsingPoint(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pPoint, *pType, *pBuffer;
    layerObj *self=NULL;
    mapObj   *parent_map;
    pointObj *poPoint=NULL;
    queryResultObj *poResult=NULL;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 3, &pPoint, &pType, &pBuffer) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);
    convert_to_double(pBuffer);

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    poPoint = (pointObj *)_phpms_fetch_handle2(pPoint,
                                               PHPMS_GLOBAL(le_mspoint_ref),
                                               PHPMS_GLOBAL(le_mspoint_new),
                                               list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (self && poPoint && parent_map &&
        (poResult=layerObj_queryUsingPoint(self, parent_map, poPoint, 
                                           pType->value.lval, 
                                           pBuffer->value.dval)) != NULL)
    {
        /* Found something... return queryResult object */
        _phpms_build_queryresult_object(poResult, list, return_value);
        return;
    }

    /* Nothing found... we should produce no error and just return 0 */
    RETURN_LONG(0);
}
/* }}} */

/************************************************************************/
/*                        layer->queryUsingRect                         */
/*                                                                      */
/*      Query on a layer using rectangular extents.                     */
/************************************************************************/

/* {{{ proto queryResultObj layer.queryUsingRect(rectObj poRect)
   Query using rectangular extent. */

DLEXPORT void php3_ms_lyr_queryUsingRect(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pRect;
    layerObj *self=NULL;
    mapObj   *parent_map;
    rectObj *poRect=NULL;
    queryResultObj *poResult=NULL;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pRect) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    self = (layerObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslayer),
                                           list);
    poRect = (rectObj *)_phpms_fetch_handle2(pRect,
                                             PHPMS_GLOBAL(le_msrect_ref),
                                             PHPMS_GLOBAL(le_msrect_new),
                                             list);
    parent_map = (mapObj*)_phpms_fetch_property_handle(pThis, "_map_handle_", 
                                                       PHPMS_GLOBAL(le_msmap),
                                                       list, E_ERROR);

    if (self && poRect && parent_map &&
        (poResult=layerObj_queryUsingRect(self, parent_map, poRect)) != NULL)
    {
        /* Found something... return queryResult object */
        _phpms_build_queryresult_object(poResult, list, return_value);
        return;
    }

    /* Nothing found... we should produce no error and just return 0 */
    RETURN_LONG(0);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - labelObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_label_object()
 **********************************************************************/
static long _phpms_build_label_object(labelObj *plabel,
                                      HashTable *list, pval *return_value)
{
    int label_id;

    if (plabel == NULL)
        return 0;

    label_id = php3_list_insert(plabel, PHPMS_GLOBAL(le_mslabel));

    object_init(return_value);
    add_property_long(return_value, "_handle_", label_id);

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value,  "font",       plabel->font);
    add_property_long(return_value,   "type",       plabel->type);
    add_property_long(return_value,   "color",      plabel->color);
    add_property_long(return_value,   "outlinecolor", plabel->outlinecolor);
    add_property_long(return_value,   "shadowcolor",plabel->shadowcolor);
    add_property_long(return_value,   "shadowsizex",plabel->shadowsizex);
    add_property_long(return_value,   "shadowsizey",plabel->shadowsizey);
    add_property_long(return_value,   "backgroundcolor",
                                                plabel->backgroundcolor);
    add_property_long(return_value,   "backgroundshadowcolor",
                                                plabel->backgroundshadowcolor);
    add_property_long(return_value,   "backgroundshadowsizex",
                                                plabel->backgroundshadowsizex);
    add_property_long(return_value,   "backgroundshadowsizey",
                                                plabel->backgroundshadowsizey);
    add_property_long(return_value,   "size",       plabel->size);
    add_property_long(return_value,   "minsize",    plabel->minsize);
    add_property_long(return_value,   "maxsize",    plabel->maxsize);
    add_property_long(return_value,   "position",   plabel->position);
    add_property_long(return_value,   "offsetx",    plabel->offsetx);
    add_property_long(return_value,   "offsety",    plabel->offsety);
    add_property_double(return_value, "angle",      plabel->angle);
    add_property_long(return_value,   "autoangle",  plabel->autoangle);
    add_property_long(return_value,   "buffer",     plabel->buffer);
    add_property_long(return_value,   "antialias",  plabel->antialias);
    add_property_long(return_value,   "wrap",       plabel->wrap);
    add_property_long(return_value,   "minfeaturesize",plabel->minfeaturesize);
    add_property_long(return_value,   "autominfeaturesize",plabel->autominfeaturesize);
    add_property_long(return_value,   "mindistance",plabel->mindistance);
    add_property_long(return_value,   "partials",   plabel->partials);
    add_property_long(return_value,   "force",      plabel->force);

    add_method(return_value, "set",    php3_ms_label_setProperty);

    return label_id;
}


/**********************************************************************
 *                        label->set()
 **********************************************************************/

/* {{{ proto int label.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_label_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    labelObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (labelObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mslabel),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "font",         self->font)
    else IF_SET_LONG(  "type",         self->type)
    else IF_SET_LONG(  "color",        self->color)
    else IF_SET_LONG(  "outlinecolor", self->outlinecolor)
    else IF_SET_LONG(  "shadowcolor",  self->shadowcolor)
    else IF_SET_LONG(  "shadowsizex",  self->shadowsizex)
    else IF_SET_LONG(  "shadowsizey",  self->shadowsizey)
    else IF_SET_LONG(  "size",         self->size)
    else IF_SET_LONG(  "minsize",      self->minsize)
    else IF_SET_LONG(  "maxsize",      self->maxsize)
    else IF_SET_LONG(  "position",     self->position)
    else IF_SET_LONG(  "offsetx",      self->offsetx)
    else IF_SET_LONG(  "offsety",      self->offsety)
    else IF_SET_DOUBLE("angle",        self->angle)
    else IF_SET_LONG(  "autoangle",    self->autoangle)
    else IF_SET_LONG(  "buffer",       self->buffer)
    else IF_SET_LONG(  "antialias",    self->antialias)
    else IF_SET_BYTE(  "wrap",         self->wrap)
    else IF_SET_LONG(  "minfeaturesize", self->minfeaturesize)
    else IF_SET_LONG(  "autominfeaturesize", self->autominfeaturesize)
    else IF_SET_LONG(  "mindistance",  self->mindistance)
    else IF_SET_LONG(  "partials",     self->partials)
    else IF_SET_LONG(  "force",        self->force)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/*=====================================================================
 *                 PHP function wrappers - classObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_class_object()
 **********************************************************************/
static long _phpms_build_class_object(classObj *pclass, int parent_layer_id,
                                      HashTable *list, pval *return_value)
{
    int class_id;
    pval new_obj_param;  /* No, it's not a pval * !!! */

    if (pclass == NULL)
        return 0;

    class_id = php3_list_insert(pclass, PHPMS_GLOBAL(le_msclass));

    object_init(return_value);
    add_property_long(return_value, "_handle_", class_id);
    add_property_long(return_value, "_layer_handle_", parent_layer_id);

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value,  "name",       pclass->name);
    add_property_long(return_value,   "color",      pclass->color);
    add_property_long(return_value, "backgroundcolor",pclass->backgroundcolor);
    add_property_long(return_value,   "outlinecolor", pclass->outlinecolor);
    add_property_long(return_value,   "symbol",     pclass->symbol);
    add_property_long(return_value,   "size",       pclass->size);
    add_property_long(return_value,   "minsize",    pclass->minsize);
    add_property_long(return_value,   "maxsize",    pclass->maxsize);
    PHPMS_ADD_PROP_STR(return_value,  "symbolname", pclass->symbolname);

    _phpms_build_label_object(&(pclass->label), list, &new_obj_param);
    _phpms_add_property_object(return_value, "label", &new_obj_param,E_ERROR);

    add_method(return_value, "set",    php3_ms_class_setProperty);
    add_method(return_value, "setexpression",php3_ms_class_setExpression);

    return class_id;
}


/**********************************************************************
 *                        ms_newClassObj()
 **********************************************************************/

/* {{{ proto layerObj ms_newClassObj(layerObj layer)
   Create a new class in the specified layer. */

DLEXPORT void php3_ms_class_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pLayerObj;
    layerObj *parent_layer;
    classObj *pNewClass;
    int layer_id;

    if (getParameters(ht, 1, &pLayerObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_layer = (layerObj*)_phpms_fetch_handle(pLayerObj, 
                                                  PHPMS_GLOBAL(le_mslayer),
                                                  list);

    if (parent_layer == NULL ||
        (pNewClass = classObj_new(parent_layer)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* We will store a reference to the parent_layer object id inside
     * the class obj.
     */
    layer_id = _phpms_fetch_property_long(pLayerObj, "_handle_", E_ERROR);

    /* Return class object */
    _phpms_build_class_object(pNewClass, layer_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        class->set()
 **********************************************************************/

/* {{{ proto int class.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_class_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    classObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(     "name",         self->name)
    else IF_SET_LONG(  "color",        self->color)
    else IF_SET_LONG(  "backgroundcolor",self->backgroundcolor)
    else IF_SET_LONG(  "outlinecolor", self->outlinecolor)
    else IF_SET_LONG(  "symbol",       self->symbol)
    else IF_SET_STRING("symbolname",   self->symbolname)
    else IF_SET_LONG(  "size",         self->size)
    else IF_SET_LONG(  "minsize",      self->minsize)
    else IF_SET_LONG(  "maxsize",      self->maxsize)
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/************************************************************************/
/*                          class->setExpression()                      */
/*                                                                      */
/*      Set the expression string for a class object.                   */
/*                                                                      */
/*      Returns 0 on success, -1 on error.                              */
/************************************************************************/

/* {{{ proto int class.setExpression(string exression)
   Set the expression string for a class object. */

DLEXPORT void php3_ms_class_setExpression(INTERNAL_FUNCTION_PARAMETERS)
{ 
    pval   *pThis, *pString;
    classObj *self=NULL;
    int     nStatus=-1;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pString) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pString);

    self = (classObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msclass),
                                           list);

    if (self == NULL || 
        (nStatus = classObj_setExpression(self, pString->value.str.val)) != 0)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_LONG(nStatus);
    }

    RETURN_LONG(0);
}
/* }}} */



/*=====================================================================
 *                 PHP function wrappers - colorObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_color_object()
 **********************************************************************/
static long _phpms_build_color_object(colorObj *pcolor,
                                      HashTable *list, pval *return_value)
{
    int color_id;

    if (pcolor == NULL)
        return 0;

    color_id = php3_list_insert(pcolor, PHPMS_GLOBAL(le_mscolor));

    object_init(return_value);
    add_property_long(return_value, "_handle_", color_id);

    /* editable properties */
    add_property_long(return_value,   "red",   pcolor->red);
    add_property_long(return_value,   "green", pcolor->green);
    add_property_long(return_value,   "blue",  pcolor->blue);

    add_method(return_value, "setrgb", php3_ms_color_setRGB);

    return color_id;
}


/**********************************************************************
 *                        color->setRGB()
 **********************************************************************/

/* {{{ proto int color.setRGB(int R, int G, int B)
   Set new RGB color. Returns -1 on error. */

DLEXPORT void php3_ms_color_setRGB(INTERNAL_FUNCTION_PARAMETERS)
{
    colorObj *self;
    pval   *pR, *pG, *pB, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 3, &pR, &pG, &pB) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (colorObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_mscolor),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_long(pR);
    convert_to_long(pG);
    convert_to_long(pB);

    self->red =   pR->value.lval;
    self->green = pG->value.lval;
    self->blue =  pB->value.lval;

    _phpms_set_property_long(pThis, "red",   self->red, E_ERROR);
    _phpms_set_property_long(pThis, "green", self->green, E_ERROR);
    _phpms_set_property_long(pThis, "blue",  self->blue, E_ERROR);

    RETURN_LONG(0);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - queryResultObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_queryresult_object()
 **********************************************************************/
static long _phpms_build_queryresult_object(queryResultObj *pquery, 
                                            HashTable *list, 
                                            pval *return_value)
{
    int queryresult_id;

    if (pquery == NULL)
        return 0;

    queryresult_id = php3_list_insert(pquery, PHPMS_GLOBAL(le_msqueryresult));

    object_init(return_value);
    add_property_long(return_value, "_handle_", queryresult_id);

    /* read-only properties */
    add_property_long(return_value, "numresults",     pquery->numresults);
    add_property_long(return_value, "numquerylayers", pquery->numquerylayers);
    add_property_long(return_value, "numlayers",      pquery->numlayers);
    add_property_long(return_value, "currentlayer",   pquery->currentlayer);
    add_property_long(return_value, "currenttile",    pquery->currenttile);
    add_property_long(return_value, "currentshape",   pquery->currentshape);

    add_method(return_value, "free", php3_ms_queryresult_free);
    add_method(return_value, "next", php3_ms_queryresult_next);
    add_method(return_value, "rewind", php3_ms_queryresult_rewind);

    add_method(return_value, "set",  php3_ms_queryresult_setProperty);

    return queryresult_id;
}

/**********************************************************************
 *                        queryResult->set()
 **********************************************************************/

/* {{{ proto int queryResult.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_queryresult_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    queryResultObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (queryResultObj*)_phpms_fetch_handle(pThis, 
                                                PHPMS_GLOBAL(le_msqueryresult),
                                                list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_LONG(       "currentlayer", self->currentlayer)
    else IF_SET_LONG(  "currentshape", self->currentshape)
    else IF_SET_LONG(  "currenttile",  self->currenttile)
    else if (strcmp( "numresults",     pPropertyName->value.str.val) == 0 ||
             strcmp( "numquerylayers", pPropertyName->value.str.val) == 0 ||
             strcmp( "numlayers",      pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */

/**********************************************************************
 *                        queryResult->free()
 **********************************************************************/

/* {{{ proto int queryResult.free()
   Destroys resources used by a queryResult object */

DLEXPORT void php3_ms_queryresult_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    queryResultObj *self;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (queryResultObj*)_phpms_fetch_handle(pThis, le_msqueryresult, list);
    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)
        {
            php3_list_delete(phandle->value.lval);
        }
    }
}
/* }}} */

/**********************************************************************
 *                        queryResult->next()
 **********************************************************************/

/* {{{ proto int queryResult.next()
   Returns the next shapeResultObj from a queryResult object */

DLEXPORT void php3_ms_queryresult_next(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    queryResultObj *self;
    shapeResultObj sShapeResult;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (queryResultObj*)_phpms_fetch_handle(pThis, le_msqueryresult, list);
    if (self)
        sShapeResult = queryResultObj_next(self);

    /* Return a shapeResultObj object */
    _phpms_build_shapeResult_object(&sShapeResult, list, return_value);
}
/* }}} */

/**********************************************************************
 *                        queryResult->rewind()
 **********************************************************************/

/* {{{ proto void queryResult.rewind()
   Moves the queryResult object to the first result record */

DLEXPORT void php3_ms_queryresult_rewind(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    queryResultObj *self;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (queryResultObj*)_phpms_fetch_handle(pThis, le_msqueryresult, list);
    if (self)
       queryResultObj_rewind(self);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - shapeResultObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_shapeResult_object()
 **********************************************************************/
static long _phpms_build_shapeResult_object(shapeResultObj *pShapeResult,
                                            HashTable *list, 
                                            pval *return_value)
{
    if (pShapeResult == NULL)
        return 0;

    object_init(return_value);

    /* Note: Contrary to most other object classes, this one does not
     *       need to keep a handle on the internal structure since all
     *       members are read-only and thus there is no set() method.
     */

    /* read-only properties */
    add_property_long(return_value,   "layer",  pShapeResult->layer);
    add_property_long(return_value,   "tile",   pShapeResult->tile);
    add_property_long(return_value,   "shape",  pShapeResult->shape);
    add_property_long(return_value,   "query",  pShapeResult->query);

    return 0;
}


/*=====================================================================
 *                 PHP function wrappers - pointObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_point_object()
 *
 * handle_type is either le_mspoint_ref for an embedded object or
 * le_mspoint_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_point_object(pointObj *ppoint, int handle_type,
                                      HashTable *list, pval *return_value)
{
    int point_id;

    if (ppoint == NULL)
        return 0;

    point_id = php3_list_insert(ppoint, handle_type);

    object_init(return_value);
    add_property_long(return_value, "_handle_", point_id);

    /* editable properties */
    add_property_double(return_value,   "x",   ppoint->x);
    add_property_double(return_value,   "y",   ppoint->y);

    add_method(return_value, "setxy", php3_ms_point_setXY);

    return point_id;
}


/**********************************************************************
 *                        ms_newPointObj()
 **********************************************************************/

/* {{{ proto pointObj ms_newPointObj()
   Create a new pointObj instance. */

DLEXPORT void php3_ms_point_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pointObj *pNewPoint;

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pNewPoint = pointObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return point object */
    _phpms_build_point_object(pNewPoint, PHPMS_GLOBAL(le_mspoint_new), 
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        point->setXY()
 **********************************************************************/

/* {{{ proto int point.setXY(double x, double y)
   Set new RGB point. Returns -1 on error. */

DLEXPORT void php3_ms_point_setXY(INTERNAL_FUNCTION_PARAMETERS)
{
    pointObj *self;
    pval   *pX, *pY, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pX, &pY) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pX);
    convert_to_double(pY);

    self->x = pX->value.dval;
    self->y = pY->value.dval;

    _phpms_set_property_double(pThis, "x", self->x, E_ERROR);
    _phpms_set_property_double(pThis, "y", self->y, E_ERROR);

    RETURN_LONG(0);
}
/* }}} */


/**********************************************************************
 *                        point->draw()
 **********************************************************************/

/* {{{ proto int point.draw(mapObj map, layerObj layer, imageObj img, string class_name, string text)
   Draws the individual point using layer. */

DLEXPORT void php3_ms_point_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pMap, *pLayer, *pImg, *pClass, *pText;
    pointObj    *self;
    mapObj      *poMap;
    layerObj    *poLayer;
    gdImagePtr  im;
    int         nRetVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 5, &pMap, &pLayer, &pImg, &pClass, &pText) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pClass);
    convert_to_string(pText);

    self = (pointObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_mspoint_ref),
                                            PHPMS_GLOBAL(le_mspoint_new),
                                            list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list);
    im = (gdImagePtr)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list);

    if (self)
        nRetVal = pointObj_draw(self, poMap, poLayer, im, 
                                pClass->value.str.val, pText->value.str.val);

    /* Draw() seems to return 1 on success !?!?!? */
    RETURN_LONG(nRetVal)
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - lineObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_line_object()
 *
 * handle_type is either le_msline_ref for an embedded object or
 * le_msline_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_line_object(lineObj *pline, int handle_type,
                                      HashTable *list, pval *return_value)
{
    int line_id;

    if (pline == NULL)
        return 0;

    line_id = php3_list_insert(pline, handle_type);

    object_init(return_value);
    add_property_long(return_value, "_handle_", line_id);

    /* read-only properties */
    add_property_long(return_value, "numpoints", pline->numpoints);

    add_method(return_value, "add", php3_ms_line_add);
    add_method(return_value, "addxy", php3_ms_line_addXY);
    add_method(return_value, "point", php3_ms_line_point);

    return line_id;
}


/**********************************************************************
 *                        ms_newLineObj()
 **********************************************************************/

/* {{{ proto lineObj ms_newLineObj()
   Create a new lineObj instance. */

DLEXPORT void php3_ms_line_new(INTERNAL_FUNCTION_PARAMETERS)
{
    lineObj *pNewLine;

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pNewLine = lineObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return line object */
    _phpms_build_line_object(pNewLine, PHPMS_GLOBAL(le_msline_new), 
                              list, return_value);
}
/* }}} */


/**********************************************************************
 *                        line->add()
 **********************************************************************/

/* {{{ proto int line.add(pointObj point)
   Adds a point to the end of a line */

DLEXPORT void php3_ms_line_add(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pPoint;
    lineObj     *self;
    pointObj    *poPoint;
    int         nRetVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pPoint) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list);

    poPoint = (pointObj*)_phpms_fetch_handle2(pPoint,
                                              PHPMS_GLOBAL(le_mspoint_ref),
                                              PHPMS_GLOBAL(le_mspoint_new),
                                              list);

    if (self && poPoint)
        nRetVal = lineObj_add(self, poPoint);

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        line->addXY()
 **********************************************************************/

/* {{{ proto int line.addXY(double x, double y)
   Adds a point to the end of a line */

DLEXPORT void php3_ms_line_addXY(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pX, *pY;
    lineObj     *self;
    pointObj    oPoint;
    int         nRetVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pX, &pY) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_double(pX);
    convert_to_double(pY);

    oPoint.x = pX->value.dval;
    oPoint.y = pY->value.dval;

    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list);

    if (self)
        nRetVal = lineObj_add(self, &oPoint);

    RETURN_LONG(nRetVal)
}
/* }}} */



/**********************************************************************
 *                        line->point()
 **********************************************************************/

/* {{{ proto int line.point(int i)
   Returns point number i.  First point is number 0. */

DLEXPORT void php3_ms_line_point(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    lineObj     *self;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (lineObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msline_ref),
                                           PHPMS_GLOBAL(le_msline_new),
                                           list);

    if (self==NULL || 
        pIndex->value.lval < 0 || pIndex->value.lval >= self->numpoints)
    {
        RETURN_FALSE;
    }

    /* Return reference to the specified point.  Reference is valid only
     * during the life of the lineObj that contains the point.
     */
    _phpms_build_point_object(&(self->point[pIndex->value.lval]), 
                                PHPMS_GLOBAL(le_mspoint_ref), 
                                list, return_value);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - shapeObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_shape_object()
 *
 * handle_type is either le_msshape_ref for an embedded object or
 * le_msshape_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_shape_object(shapeObj *pshape, int handle_type,
                                      HashTable *list, pval *return_value)
{
    int     shape_id;
    pval    new_obj_param;  /* No, it's not a pval * !!! */

    if (pshape == NULL)
        return 0;

    shape_id = php3_list_insert(pshape, handle_type);

    object_init(return_value);
    add_property_long(return_value, "_handle_", shape_id);

    /* read-only properties */
    add_property_long(return_value, "numlines", pshape->numlines);
    add_property_long(return_value, "type",     pshape->type);

    _phpms_build_rect_object(&(pshape->bounds), PHPMS_GLOBAL(le_msrect_ref), 
                             list, &new_obj_param);
    _phpms_add_property_object(return_value, "bounds", &new_obj_param,E_ERROR);

    add_method(return_value, "add", php3_ms_shape_add);
    add_method(return_value, "point", php3_ms_shape_line);
    add_method(return_value, "draw", php3_ms_shape_draw);

    return shape_id;
}


/**********************************************************************
 *                        ms_newShapeObj()
 **********************************************************************/

/* {{{ proto shapeObj ms_newShapeObj()
   Create a new shapeObj instance. */

DLEXPORT void php3_ms_shape_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pType;
    shapeObj *pNewShape;

    if (getParameters(ht, 1, &pType) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pType);

    if ((pNewShape = shapeObj_new(pType->value.lval)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(pNewShape, PHPMS_GLOBAL(le_msshape_new), 
                              list, return_value);
}
/* }}} */


/**********************************************************************
 *                        shape->add()
 **********************************************************************/

/* {{{ proto int shape.add(lineObj line)
   Adds a line (i.e. a part) to a shape */

DLEXPORT void php3_ms_shape_add(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pLine;
    shapeObj     *self;
    lineObj    *poLine;
    int         nRetVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pLine) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msshape_ref),
                                           PHPMS_GLOBAL(le_msshape_new),
                                           list);

    poLine = (lineObj*)_phpms_fetch_handle2(pLine,
                                            PHPMS_GLOBAL(le_msline_ref),
                                            PHPMS_GLOBAL(le_msline_new),
                                            list);

    if (self && poLine)
        nRetVal = shapeObj_add(self, poLine);

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        shape->line()
 **********************************************************************/

/* {{{ proto int shape.line(int i)
   Returns line (part) number i.  First line is number 0. */

DLEXPORT void php3_ms_shape_line(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    shapeObj     *self;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msshape_ref),
                                           PHPMS_GLOBAL(le_msshape_new),
                                           list);

    if (self==NULL || 
        pIndex->value.lval < 0 || pIndex->value.lval >= self->numlines)
    {
        RETURN_FALSE;
    }

    /* Return reference to the specified line.  Reference is valid only
     * during the life of the shapeObj that contains the line.
     */
    _phpms_build_line_object(&(self->line[pIndex->value.lval]), 
                                PHPMS_GLOBAL(le_msline_ref), 
                                list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shape->draw()
 **********************************************************************/

/* {{{ proto int shape.draw(mapObj map, layerObj layer, imageObj img, string class_name, string text)
   Draws the individual shape using layer. */

DLEXPORT void php3_ms_shape_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pMap, *pLayer, *pImg, *pClass, *pText;
    shapeObj    *self;
    mapObj      *poMap;
    layerObj    *poLayer;
    gdImagePtr  im;
    int         nRetVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 5, &pMap, &pLayer, &pImg, &pClass, &pText) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pClass);
    convert_to_string(pText);

    self = (shapeObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msshape_ref),
                                            PHPMS_GLOBAL(le_msshape_new),
                                            list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list);
    im = (gdImagePtr)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list);

    if (self)
        nRetVal = shapeObj_draw(self, poMap, poLayer, im, 
                                pClass->value.str.val, pText->value.str.val);

    /* Draw() seems to return 1 on success !?!?!? */
    RETURN_LONG(nRetVal)
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - featureObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_feature_object()
 **********************************************************************/
static long _phpms_build_feature_object(struct featureObj *pfeature, 
                                        int parent_layer_id,
                                        HashTable *list, pval *return_value)
{
    int feature_id;
    pval new_obj_param;  /* No, it's not a pval * !!! */

    if (pfeature == NULL)
        return 0;

    feature_id = php3_list_insert(pfeature, PHPMS_GLOBAL(le_msfeature));

    object_init(return_value);
    add_property_long(return_value, "_handle_", feature_id);
    add_property_long(return_value, "_layer_handle_", parent_layer_id);

    /* editable properties */
    PHPMS_ADD_PROP_STR(return_value,  "class",      pfeature->class);
    PHPMS_ADD_PROP_STR(return_value,  "text",       pfeature->text);

    _phpms_build_shape_object(&(pfeature->shape), PHPMS_GLOBAL(le_msshape_ref),
                              list, &new_obj_param);
    _phpms_add_property_object(return_value, "shape", &new_obj_param, E_ERROR);

    add_method(return_value, "set",    php3_ms_feature_setProperty);
    add_method(return_value, "next",   php3_ms_feature_next);

    return feature_id;
}


/**********************************************************************
 *                        ms_newFeatureObj()
 **********************************************************************/

/* {{{ proto layerObj ms_newFeatureObj(layerObj layer)
   Create a new inline feature in the specified layer. */

DLEXPORT void php3_ms_feature_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval  *pLayerObj;
    layerObj *parent_layer;
    struct featureObj *pNewFeature;
    int layer_id;

    if (getParameters(ht, 1, &pLayerObj) == FAILURE) 
    {
        WRONG_PARAM_COUNT;
    }

    parent_layer = (layerObj*)_phpms_fetch_handle(pLayerObj, 
                                                  PHPMS_GLOBAL(le_mslayer),
                                                  list);

    if (parent_layer == NULL ||
        (pNewFeature = featureObj_new(parent_layer)) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* We will store a reference to the parent_layer object id inside
     * the feature obj.
     */
    layer_id = _phpms_fetch_property_long(pLayerObj, "_handle_", E_ERROR);

    /* Return feature object */
    _phpms_build_feature_object(pNewFeature, layer_id, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        feature->set()
 **********************************************************************/

/* {{{ proto int feature.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_feature_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    struct featureObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (struct featureObj *)_phpms_fetch_handle(pThis, 
                                                    PHPMS_GLOBAL(le_msfeature),
                                                    list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_STRING(      "class",   self->class)
    else IF_SET_STRING( "text",    self->text)
    else if (strcmp( "shape", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }         
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}
/* }}} */


/**********************************************************************
 *                        feature->next()
 **********************************************************************/

/* {{{ proto int feature.next()
   Returns the next featureObj in the list for this layer. Returns 0 when the end of the list is reached. */

DLEXPORT void php3_ms_feature_next(INTERNAL_FUNCTION_PARAMETERS)
{
    pval       *pThis;
    struct featureObj *self;
    int         layer_id;

    if (getThis(&pThis) == FAILURE || ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (struct featureObj *)_phpms_fetch_handle(pThis, 
                                                    PHPMS_GLOBAL(le_msfeature),
                                                    list);

    if (self==NULL || self->next == NULL)
    {
        /* Return 0 when we reach the last feature of the list. */
        RETURN_LONG(0);
    }

    layer_id = _phpms_fetch_property_long(pThis, "_layer_handle_", E_ERROR);

    /* Return reference to the next feature.  Reference is valid only
     * during the life of the layerObj that contains the feature.
     */
    _phpms_build_feature_object(self->next, layer_id, list, return_value);
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - webObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_web_object()
 **********************************************************************/
static long _phpms_build_web_object(webObj *pweb, 
                                    HashTable *list, pval *return_value)
{
    int         web_id;
    pval        new_obj_param;  /* No, it's not a pval * !!! */

    if (pweb == NULL)
        return 0;

    web_id = php3_list_insert(pweb, PHPMS_GLOBAL(le_msweb));

    object_init(return_value);
    add_property_long(return_value, "_handle_", web_id);

    PHPMS_ADD_PROP_STR(return_value,  "log",            pweb->log);
    PHPMS_ADD_PROP_STR(return_value,  "imagepath",      pweb->imagepath);
    PHPMS_ADD_PROP_STR(return_value,  "template",       pweb->template);
    PHPMS_ADD_PROP_STR(return_value,  "imageurl",       pweb->imageurl);
    PHPMS_ADD_PROP_STR(return_value,  "header",         pweb->header);
    PHPMS_ADD_PROP_STR(return_value,  "footer",         pweb->footer);
    PHPMS_ADD_PROP_STR(return_value,  "empty",          pweb->empty);
    PHPMS_ADD_PROP_STR(return_value,  "error",          pweb->error);
    PHPMS_ADD_PROP_STR(return_value,  "mintemplate",    pweb->mintemplate);
    PHPMS_ADD_PROP_STR(return_value,  "maxtemplate",    pweb->maxtemplate);
    add_property_double(return_value,   "minscale",       pweb->minscale);
    add_property_double(return_value,   "maxscale",       pweb->maxscale);
    
    _phpms_build_rect_object(&(pweb->extent), PHPMS_GLOBAL(le_msrect_ref), 
                             list, &new_obj_param);
    _phpms_add_property_object(return_value, "extent", &new_obj_param,E_ERROR);

    add_method(return_value, "set",    php3_ms_web_setProperty);

    return web_id;
}

/**********************************************************************
 *                        web->set()
 **********************************************************************/

/* {{{ proto int web.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */


DLEXPORT void php3_ms_web_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    webObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (webObj *)_phpms_fetch_handle(pThis, PHPMS_GLOBAL(le_msweb),
                                         list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    
    IF_SET_STRING(       "log",         self->log)
    else IF_SET_STRING(  "imagepath",   self->imagepath)
    else IF_SET_STRING(  "template",   self->template) 
    else IF_SET_STRING(  "imageurl",   self->imageurl)
    else IF_SET_STRING(  "header",   self->header)
    else IF_SET_STRING(  "footer",   self->footer)
    else IF_SET_STRING(  "mintemplate",   self->mintemplate) 
    else IF_SET_STRING(  "maxtemplate",   self->maxtemplate) 
    else IF_SET_LONG(    "minscale",   self->minscale)
    else IF_SET_LONG(    "maxscale",   self->maxscale)  
    else if (strcmp( "empty", pPropertyName->value.str.val) == 0 ||
             strcmp( "error",  pPropertyName->value.str.val) == 0 ||
             strcmp( "extent", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }         
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}           



/*=====================================================================
 *                 PHP function wrappers - rectObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_rect_object()
 * handle_type is either le_msrect_ref for an embedded object or
 * le_msrect_new for a newly allocated object
 **********************************************************************/
static long _phpms_build_rect_object(rectObj *prect, int handle_type, 
                                    HashTable *list, pval *return_value)
{
    int rect_id;

    if (prect == NULL)
        return 0;

    rect_id = php3_list_insert(prect, handle_type);

    object_init(return_value);
    add_property_long(return_value, "_handle_", rect_id);

    add_property_double(return_value,   "minx",       prect->minx);
    add_property_double(return_value,   "miny",       prect->miny);
    add_property_double(return_value,   "maxx",       prect->maxx);
    add_property_double(return_value,   "maxy",       prect->maxy);

    add_method(return_value, "set",    php3_ms_rect_setProperty);
    add_method(return_value, "setextent",    php3_ms_rect_setExtent);

    return rect_id;
}


/**********************************************************************
 *                        ms_newRectObj()
 **********************************************************************/

/* {{{ proto rectObj ms_newRectObj()
   Create a new rectObj instance. */

DLEXPORT void php3_ms_rect_new(INTERNAL_FUNCTION_PARAMETERS)
{
    rectObj *pNewRect;

    if (ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    if ((pNewRect = rectObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_ERROR);
        RETURN_FALSE;
    }

    /* Return rect object */
    _phpms_build_rect_object(pNewRect, PHPMS_GLOBAL(le_msrect_new), 
                              list, return_value);
}
/* }}} */


/**********************************************************************
 *                        rect->set()
 **********************************************************************/

/* {{{ proto int rect.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_rect_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    rectObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (rectObj *)_phpms_fetch_handle2(pThis, PHPMS_GLOBAL(le_msrect_ref),
                                           PHPMS_GLOBAL(le_msrect_new), list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    IF_SET_LONG(        "minx",   self->minx)
    else IF_SET_LONG(   "miny",   self->miny)  
    else IF_SET_LONG(   "maxx",   self->maxx) 
    else IF_SET_LONG(   "maxy",   self->maxy)           
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }
         
    RETURN_LONG(0);
}           

/**********************************************************************
 *                        rect->setextent()
 **********************************************************************/

/* {{{ proto int rect.setextent(xmin, ymin, xmax, ymax)
   Set object property using four values.
*/
DLEXPORT void php3_ms_rect_setExtent(INTERNAL_FUNCTION_PARAMETERS)
{
    rectObj *self;
    pval   *pXMin, *pYMin, *pXMax, *pYMax, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 4, &pXMin, &pYMin, &pXMax, &pYMax) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (rectObj *)_phpms_fetch_handle2(pThis, 
                                           PHPMS_GLOBAL(le_msrect_ref),
                                           PHPMS_GLOBAL(le_msrect_new),
                                           list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_double(pXMin);
    convert_to_double(pYMin);
    convert_to_double(pXMax);
    convert_to_double(pYMax);
    
    self->minx = pXMin->value.dval;
    self->miny = pYMin->value.dval;
    self->maxx = pXMax->value.dval;
    self->maxy = pYMax->value.dval;

    _phpms_set_property_double(pThis, "minx", self->minx, E_ERROR);
    _phpms_set_property_double(pThis, "miny", self->miny, E_ERROR);
    _phpms_set_property_double(pThis, "maxx", self->maxx, E_ERROR);
    _phpms_set_property_double(pThis, "maxy", self->maxy, E_ERROR);

    RETURN_LONG(0);
}
/* }}} */


/**********************************************************************
 *                        rect->draw()
 **********************************************************************/

/* {{{ proto int rect.draw(mapObj map, layerObj layer, imageObj img, string class_name, string text)
   Draws the individual rect using layer. */

DLEXPORT void php3_ms_rect_draw(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pMap, *pLayer, *pImg, *pClass, *pText;
    rectObj    *self;
    mapObj      *poMap;
    layerObj    *poLayer;
    gdImagePtr  im;
    int         nRetVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 5, &pMap, &pLayer, &pImg, &pClass, &pText) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_string(pClass);
    convert_to_string(pText);

    self = (rectObj *)_phpms_fetch_handle2(pThis, 
                                            PHPMS_GLOBAL(le_msrect_ref),
                                            PHPMS_GLOBAL(le_msrect_new),
                                            list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);
    poLayer = (layerObj*)_phpms_fetch_handle(pLayer, PHPMS_GLOBAL(le_mslayer),
                                             list);
    im = (gdImagePtr)_phpms_fetch_handle(pImg, PHPMS_GLOBAL(le_msimg), list);

    if (self)
        nRetVal = rectObj_draw(self, poMap, poLayer, im, 
                                pClass->value.str.val, pText->value.str.val);

    /* Draw() seems to return 1 on success !?!?!? */
    RETURN_LONG(nRetVal)
}
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - referneceMapObj class
 *====================================================================*/
/**********************************************************************
 *                     _phpms_build_referenceMap_object()
 **********************************************************************/
static long _phpms_build_referenceMap_object(referenceMapObj *preference, 
                                    HashTable *list, pval *return_value)
{
    int         reference_id;
    pval        new_obj_param;  /* No, it's not a pval * !!! */

    if (preference == NULL)
        return 0;

    reference_id = php3_list_insert(preference, PHPMS_GLOBAL(ls_msrefmap));

    object_init(return_value);
    add_property_long(return_value, "_handle_", reference_id);

    PHPMS_ADD_PROP_STR(return_value,  "image",   preference->image);
    add_property_long(return_value,   "width",  preference->width);
    add_property_long(return_value,   "height",  preference->height);
    add_property_long(return_value,   "status",  preference->status);
    
    _phpms_build_rect_object(&(preference->extent), 
                             PHPMS_GLOBAL(le_msrect_ref),list, &new_obj_param);
    _phpms_add_property_object(return_value, "extent", &new_obj_param,E_ERROR);

    _phpms_build_color_object(&(preference->color),list, &new_obj_param);
    _phpms_add_property_object(return_value, "color", &new_obj_param,E_ERROR);

    _phpms_build_color_object(&(preference->outlinecolor),list,&new_obj_param);
    _phpms_add_property_object(return_value, "outlinecolor", 
                               &new_obj_param, E_ERROR);

    add_method(return_value, "set",    php3_ms_referenceMap_setProperty);

    return reference_id;
}


/**********************************************************************
 *                        referenceMap->set()
 **********************************************************************/

/* {{{ proto int web.set(string property_name, new_value)
   Set object property to a new value. Returns -1 on error. */

DLEXPORT void php3_ms_referenceMap_setProperty(INTERNAL_FUNCTION_PARAMETERS)
{
    referenceMapObj *self;
    pval   *pPropertyName, *pNewValue, *pThis;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pPropertyName, &pNewValue) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (referenceMapObj *)_phpms_fetch_handle(pThis, 
                                                  PHPMS_GLOBAL(ls_msrefmap),
                                                  list);
    if (self == NULL)
    {
        RETURN_LONG(-1);
    }

    convert_to_string(pPropertyName);

    
    IF_SET_STRING(       "image",    self->image)
    else IF_SET_LONG(    "width",    self->width)
    else IF_SET_LONG(    "height",   self->height)
    else IF_SET_LONG(    "status",   self->status) 
    else if (strcmp( "extent", pPropertyName->value.str.val) == 0 ||
             strcmp( "color",  pPropertyName->value.str.val) == 0 ||
             strcmp( "outlinecolor", pPropertyName->value.str.val) == 0)
    {
        php3_error(E_ERROR,"Property '%s' is read-only and cannot be set.", 
                            pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }                  
    else
    {
        php3_error(E_ERROR,"Property '%s' does not exist in this object.", 
                   pPropertyName->value.str.val);
        RETURN_LONG(-1);
    }

    RETURN_LONG(0);
}           
/* }}} */


/*=====================================================================
 *                 PHP function wrappers - shapefileObj class
 *====================================================================*/

/**********************************************************************
 *                     _phpms_build_shapefile_object()
 **********************************************************************/
static long _phpms_build_shapefile_object(shapefileObj *pshapefile,
                                          HashTable *list, pval *return_value)
{
    int shapefile_id;
    pval    new_obj_param;  /* No, it's not a pval * !!! */

    if (pshapefile == NULL)
        return 0;

    object_init(return_value);

    shapefile_id = php3_list_insert(pshapefile, PHPMS_GLOBAL(le_msshapefile));
    add_property_long(return_value, "_handle_", shapefile_id);

    /* read-only properties */
    add_property_long(return_value, "numshapes",  pshapefile->numshapes);
    add_property_long(return_value, "type",       pshapefile->numshapes);
    PHPMS_ADD_PROP_STR(return_value,"source",     pshapefile->source);

    _phpms_build_rect_object(&(pshapefile->bounds), 
                             PHPMS_GLOBAL(le_msrect_ref), 
                             list, &new_obj_param);
    _phpms_add_property_object(return_value, "bounds", &new_obj_param,E_ERROR);


    /* methods */
    add_method(return_value, "getshape",        php3_ms_shapefile_getshape);
    add_method(return_value,"gettransformed",php3_ms_shapefile_gettransformed);
    add_method(return_value, "getextent",       php3_ms_shapefile_getextent);
    add_method(return_value, "addshape",        php3_ms_shapefile_addshape);
    add_method(return_value, "free",            php3_ms_shapefile_free);

    return shapefile_id;
}

/**********************************************************************
 *                        ms_newShapefileObj()
 **********************************************************************/

/* {{{ proto shapefileObj ms_newShapefileObj(string filename, int type)
   Opens a shapefile and returns a new object to deal with it. Filename should be passed with no extension. */

DLEXPORT void php3_ms_shapefile_new(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pFname, *pType;
    shapefileObj *pNewObj = NULL;

    if (getParameters(ht, 2, &pFname, &pType) != SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    /* Attempt to open the file 
     */
    convert_to_string(pFname);
    convert_to_long(pType);
    pNewObj = shapefileObj_new(pFname->value.str.val, pType->value.lval);
    if (pNewObj == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed to open shapefile %s", 
                            pFname->value.str.val);
        RETURN_FALSE;
    }

    /* Create a PHP object, add all shapefileObj methods, etc.
     */
    _phpms_build_shapefile_object(pNewObj, list, return_value);
}
/* }}} */


/**********************************************************************
 *                        shapefile->addShape()
 **********************************************************************/

/* {{{ proto int shapefile.addShape(shapeObj shape)
   Appends a shape to an open shapefile. */

DLEXPORT void php3_ms_shapefile_addshape(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pShape;
    shapefileObj *self;
    shapeObj    *poShape;
    int         nRetVal=0;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pShape) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    poShape = (shapeObj*)_phpms_fetch_handle2(pShape,
                                              PHPMS_GLOBAL(le_msshape_ref),
                                              PHPMS_GLOBAL(le_msshape_new),
                                              list);

    if (self && poShape)
        nRetVal = shapefileObj_add(self, poShape);

    RETURN_LONG(nRetVal)
}
/* }}} */


/**********************************************************************
 *                        shapefile->getShape()
 **********************************************************************/

/* {{{ proto int shapefile.getShape(int i)
   Retrieve shape by index. */

DLEXPORT void php3_ms_shapefile_getshape(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    shapefileObj *self;
    shapeObj    *poShape;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    /* Create a new shapeObj to hold the result 
     * Note that the type used to create the shape (MS_NULL) does not matter
     * at this point since it will be set by SHPReadShape().
     */
    if ((poShape = shapeObj_new(MS_NULL)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new shape (out of memory?)");
        RETURN_FALSE;
    }

    /* Read from the file */
    if (self == NULL || 
        shapefileObj_get(self, pIndex->value.lval, poShape) != 0)
    {
        shapeObj_destroy(poShape);
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed reading shape %d.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), 
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shapefile->gettransformed()
 **********************************************************************/

/* {{{ proto int shapefile.getTransformed(mapObj map, int i)
   Retrieve shape by index. */

DLEXPORT void php3_ms_shapefile_gettransformed(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex, *pMap;
    shapefileObj *self;
    shapeObj    *poShape;
    mapObj      *poMap;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 2, &pMap, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    poMap = (mapObj*)_phpms_fetch_handle(pMap, PHPMS_GLOBAL(le_msmap), list);

    /* Create a new shapeObj to hold the result 
     * Note that the type used to create the shape (MS_NULL) does not matter
     * at this point since it will be set by SHPReadShape().
     */
    if ((poShape = shapeObj_new(MS_NULL)) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new shape (out of memory?)");
        RETURN_FALSE;
    }

    /* Read from the file */
    if (self == NULL || 
        shapefileObj_getTransformed(self, poMap, 
                                    pIndex->value.lval, poShape) != 0)
    {
        shapeObj_destroy(poShape);
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed reading shape %d.", pIndex->value.lval);
        RETURN_FALSE;
    }

    /* Return shape object */
    _phpms_build_shape_object(poShape, PHPMS_GLOBAL(le_msshape_new), 
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shapefile->getExtent()
 **********************************************************************/

/* {{{ proto int shapefile.getExtent(int i)
   Retrieve a shape's bounding box by index. */

DLEXPORT void php3_ms_shapefile_getextent(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis, *pIndex;
    shapefileObj *self;
    rectObj      *poRect;

    if (getThis(&pThis) == FAILURE ||
        getParameters(ht, 1, &pIndex) !=SUCCESS)
    {
        WRONG_PARAM_COUNT;
    }

    convert_to_long(pIndex);

    self = (shapefileObj *)_phpms_fetch_handle(pThis, 
                                               PHPMS_GLOBAL(le_msshapefile),
                                               list);

    if (self == NULL)
    {
        RETURN_FALSE;
    }

    /* Create a new rectObj to hold the result */
    if ((poRect = rectObj_new()) == NULL)
    {
        _phpms_report_mapserver_error(E_WARNING);
        php3_error(E_ERROR, "Failed creating new rectObj (out of memory?)");
        RETURN_FALSE;
    }

    /* Read from the file 
     * shapefileObj_getExtent() has no return value!  How do we catch errors?
     */
    shapefileObj_getExtent(self, pIndex->value.lval, poRect);

    /* Return rectObj */
    _phpms_build_rect_object(poRect, PHPMS_GLOBAL(le_msrect_new), 
                              list, return_value);
}
/* }}} */

/**********************************************************************
 *                        shapeFileObj->free()
 **********************************************************************/

/* {{{ proto int shapefile.free()
   Destroys resources used by a shapeFileObj object */

DLEXPORT void php3_ms_shapefile_free(INTERNAL_FUNCTION_PARAMETERS)
{
    pval *pThis;
    shapefileObj *self;

    if (getThis(&pThis) == FAILURE ||
        ARG_COUNT(ht) > 0)
    {
        WRONG_PARAM_COUNT;
    }

    self = (shapefileObj*)_phpms_fetch_handle(pThis, le_msshapefile, list);
    if (self)
    {
        /* Note: we do not have to call the object destructor...
         * removing the object from the resource list using php3_list_delete()
         * will also call the object destructor through the list destructor.
         */
        pval *phandle;
        if (_php3_hash_find(pThis->value.ht, "_handle_", sizeof("_handle_"), 
                            (void **)&phandle) == SUCCESS)
        {
            php3_list_delete(phandle->value.lval);
        }
    }
}
/* }}} */


/* ==================================================================== */
/*      utility functions                                               */
/* ==================================================================== */


/************************************************************************/
/*      DLEXPORT void php3_ms_getcwd(INTERNAL_FUNCTION_PARAMETERS)      */
/*                                                                      */
/*      This function is a copy of the                                  */
/*      vod php3_posix_getcwd(INTERNAL_FUNCTION_PARAMETERS)             */
/*      (./php/functions/posix.c).                                      */
/*                                                                      */
/*       Since posix functions seems to not be standard in php, It has been*/
/*      added here.                                                     */
/************************************************************************/
/* OS/2's gcc defines _POSIX_PATH_MAX but not PATH_MAX */
#if !defined(PATH_MAX) && defined(_POSIX_PATH_MAX)
#define PATH_MAX _POSIX_PATH_MAX
#endif

#if !defined(PATH_MAX) && defined(MAX_PATH)
#define PATH_MAX MAX_PATH
#endif

#if !defined(PATH_MAX)
#define PATH_MAX 512
#endif

DLEXPORT void php3_ms_getcwd(INTERNAL_FUNCTION_PARAMETERS)
{
    char  buffer[PATH_MAX];
    char *p;
   
    p = getcwd(buffer, PATH_MAX);
    if (!p) {
        php3_error(E_WARNING, "posix_getcwd() failed with '%s'",
                   strerror(errno));
        RETURN_FALSE;
    }

    RETURN_STRING(buffer, 1);
}

/************************************************************************/
/*                           php3_ms_getpid()                           */
/************************************************************************/
DLEXPORT void php3_ms_getpid(INTERNAL_FUNCTION_PARAMETERS)
{
    RETURN_LONG(getpid());
}



/************************************************************************/
/*  static double GetDeltaExtentsUsingScale(double dfMinscale, int nUnits,*/
/*                                              int nWidth)             */
/*                                                                      */
/*      Utility function to return the maximum extent using the         */
/*      scale and the width of the image.                               */
/*                                                                      */
/*      Base on the function msCalculateScale (mapscale.c)              */
/************************************************************************/
static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};
static double GetDeltaExtentsUsingScale(double dfScale, int nUnits, 
                                        int nWidth)
{
    double md = 0.0;
    double dfDelta = -1.0;

    if (dfScale <= 0 || nWidth <=0)
      return -1;

    switch (nUnits) 
    {
      case(MS_DD):
      case(MS_METERS):    
      case(MS_KILOMETERS):
      case(MS_MILES):
      case(MS_INCHES):  
      case(MS_FEET):
        md = (nWidth-1)/(MS_PPI*inchesPerUnit[nUnits]);
        dfDelta = md * dfScale;
        break;
          
      default:
        break;
    }

    return dfDelta;
}

