/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Shapefile access API
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

#ifndef MAPSHAPE_H
#define MAPSHAPE_H

#include <stdio.h>
#include "mapprimitive.h"
#include "mapproject.h"

#include "cpl_vsi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHX_BUFFER_PAGE 1024

#ifndef SWIG
#define MS_PATH_LENGTH 1024

  /* Shapefile types */
#define SHP_POINT 1
#define SHP_ARC 3
#define SHP_POLYGON 5
#define SHP_MULTIPOINT 8

#define SHP_POINTZ 11
#define SHP_ARCZ  13
#define SHP_POLYGONZ 15
#define SHP_MULTIPOINTZ 18

#define SHP_POINTM 21
#define SHP_ARCM  23
#define SHP_POLYGONM 25
#define SHP_MULTIPOINTM 28
#endif

#define MS_SHAPEFILE_POINT 1
#define MS_SHAPEFILE_ARC 3
#define MS_SHAPEFILE_POLYGON 5
#define MS_SHAPEFILE_MULTIPOINT 8

#define MS_SHP_POINTZ 11
#define MS_SHP_ARCZ  13
#define MS_SHP_POLYGONZ 15
#define MS_SHP_MULTIPOINTZ 18

#define MS_SHP_POINTM 21
#define MS_SHP_ARCM  23
#define MS_SHP_POLYGONM 25
#define MS_SHP_MULTIPOINTM 28

#ifndef SWIG
  typedef unsigned char uchar;

  typedef struct {
    VSILFILE  *fpSHP;
    VSILFILE  *fpSHX;

    int   nShapeType;       /* SHPT_* */
    int   nFileSize;        /* SHP file */

    int   nRecords;
    int   nMaxRecords;

    int   *panRecOffset;
    int   *panRecSize;
    ms_bitarray panRecLoaded;
    int   panRecAllLoaded;

    double  adBoundsMin[4];
    double  adBoundsMax[4];

    int   bUpdated;

    int   nBufSize; /* these used static vars in shape readers, moved to be thread-safe */
    uchar   *pabyRec;
    int   nPartMax;
    int   *panParts;

  } SHPInfo;
  typedef SHPInfo * SHPHandle;
#endif

  /************************************************************************/
  /*                          DBFInfo                                     */
  /************************************************************************/

/**
 * An object containing information about a DBF file
 *
 */
  typedef struct {
#ifdef SWIG
    %immutable;
#endif

    int   nRecords; ///< Number of records in the DBF
    int   nFields; ///< Number of fields in the DBF


#ifndef SWIG
    VSILFILE  *fp;
    unsigned int nRecordLength;
    int   nHeaderLength;

    int   *panFieldOffset;
    int   *panFieldSize;
    int   *panFieldDecimals;
    char  *pachFieldType;

    char  *pszHeader;

    int   nCurrentRecord;
    int   bCurrentRecordModified;
    char  *pszCurrentRecord;

    int   bNoHeader;
    int   bUpdated;

    char  *pszStringField;
    int   nStringFieldLen;
#endif /* not SWIG */
  } DBFInfo;

  typedef DBFInfo * DBFHandle;

  typedef enum {FTString, FTInteger, FTDouble, FTInvalid} DBFFieldType;

  /************************************************************************/
  /*                          shapefileObj                                */
  /************************************************************************/

/**
 * An object representing a Shapefile. There is no write access to this object
 * using MapScript.
 */
  typedef struct {
#ifdef SWIG
    %immutable;
#endif

    int type; ///< Shapefile type - see mapshape.h for values of type
    int numshapes; ///< Number of shapes
    rectObj bounds; ///< Extent of shapes

#ifndef SWIG
    char source[MS_PATH_LENGTH]; /* full path to this file data */
    int lastshape;
    ms_bitarray status;
    int isopen;
    SHPHandle hSHP; /* SHP/SHX file pointer */
    DBFHandle hDBF; /* DBF file pointer */
#endif

  } shapefileObj;

#ifndef SWIG
  /* layerInfo structure for tiled shapefiles */
  typedef struct {
    shapefileObj *shpfile;
    shapefileObj *tileshpfile;
    int tilelayerindex;
    projectionObj sTileProj;
    rectObj searchrect;
    reprojectionObj* reprojectorFromTileProjToLayerProj;
  } msTiledSHPLayerInfo;

  /* shapefileObj function prototypes  */
  MS_DLL_EXPORT int msShapefileOpen(shapefileObj *shpfile, const char *mode, const char *filename, int log_failures);
  MS_DLL_EXPORT int msShapefileCreate(shapefileObj *shpfile, char *filename, int type);
  MS_DLL_EXPORT void msShapefileClose(shapefileObj *shpfile);
  MS_DLL_EXPORT int msShapefileWhichShapes(shapefileObj *shpfile, rectObj rect, int debug);

  /* SHP/SHX function prototypes */
  MS_DLL_EXPORT SHPHandle msSHPOpen( const char * pszShapeFile, const char * pszAccess );
  MS_DLL_EXPORT SHPHandle msSHPCreate( const char * pszShapeFile, int nShapeType );
  MS_DLL_EXPORT void msSHPClose( SHPHandle hSHP );
  MS_DLL_EXPORT void msSHPGetInfo( SHPHandle hSHP, int * pnEntities, int * pnShapeType );
  MS_DLL_EXPORT int msSHPReadBounds( SHPHandle psSHP, int hEntity, rectObj *padBounds );
  MS_DLL_EXPORT void msSHPReadShape( SHPHandle psSHP, int hEntity, shapeObj *shape );
  MS_DLL_EXPORT int msSHPReadPoint(SHPHandle psSHP, int hEntity, pointObj *point );
  MS_DLL_EXPORT int msSHPWriteShape( SHPHandle psSHP, shapeObj *shape );
  MS_DLL_EXPORT int msSHPWritePoint(SHPHandle psSHP, pointObj *point );

  /* tiledShapefileObj function prototypes are in mapserver.h */

  /* XBase function prototypes */
  MS_DLL_EXPORT DBFHandle msDBFOpen( const char * pszDBFFile, const char * pszAccess );
  MS_DLL_EXPORT void msDBFClose( DBFHandle hDBF );
  MS_DLL_EXPORT DBFHandle msDBFCreate( const char * pszDBFFile );

  MS_DLL_EXPORT int msDBFGetFieldCount( DBFHandle psDBF );
  MS_DLL_EXPORT int msDBFGetRecordCount( DBFHandle psDBF );
  MS_DLL_EXPORT int msDBFAddField( DBFHandle hDBF, const char * pszFieldName, DBFFieldType eType, int nWidth, int nDecimals );

  MS_DLL_EXPORT DBFFieldType msDBFGetFieldInfo( DBFHandle psDBF, int iField, char * pszFieldName, int * pnWidth, int * pnDecimals );

  MS_DLL_EXPORT int msDBFReadIntegerAttribute( DBFHandle hDBF, int iShape, int iField );
  MS_DLL_EXPORT double msDBFReadDoubleAttribute( DBFHandle hDBF, int iShape, int iField );
  MS_DLL_EXPORT const char *msDBFReadStringAttribute( DBFHandle hDBF, int iShape, int iField );

  MS_DLL_EXPORT int msDBFWriteIntegerAttribute( DBFHandle hDBF, int iShape, int iField, int nFieldValue );
  MS_DLL_EXPORT int msDBFWriteDoubleAttribute( DBFHandle hDBF, int iShape, int iField, double dFieldValue );
  MS_DLL_EXPORT int msDBFWriteStringAttribute( DBFHandle hDBF, int iShape, int iField, const char * pszFieldValue );

  MS_DLL_EXPORT char **msDBFGetItems(DBFHandle dbffile);
  MS_DLL_EXPORT char **msDBFGetValues(DBFHandle dbffile, int record);
  MS_DLL_EXPORT char **msDBFGetValueList(DBFHandle dbffile, int record, int *itemindexes, int numitems);
  MS_DLL_EXPORT int *msDBFGetItemIndexes(DBFHandle dbffile, char **items, int numitems);
  MS_DLL_EXPORT int msDBFGetItemIndex(DBFHandle dbffile, char *name);

#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPSHAPE_H */
