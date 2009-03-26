/******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.35  2006/02/22 05:04:34  sdlime
 * Applied patch for bug 1660 to hide certain structures from Swig-based MapScript.
 *
 * Revision 1.34  2005/06/14 16:03:34  dan
 * Updated copyright date to 2005
 *
 * Revision 1.33  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.32  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef MAPSHAPE_H
#define MAPSHAPE_H

#include <stdio.h>
#include "mapprimitive.h"

#ifdef __cplusplus
extern "C" {
#endif

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

typedef	struct {
    FILE	*fpSHP;
    FILE	*fpSHX;

    int		nShapeType;				/* SHPT_* */
    int		nFileSize;				/* SHP file */

    int		nRecords;
    int		nMaxRecords;
    int		*panRecOffset;
    int		*panRecSize;

    double	adBoundsMin[4];
    double	adBoundsMax[4];

    int		bUpdated;

    int		nBufSize; /* these used static vars in shape readers, moved to be thread-safe */
    uchar   *pabyRec;
    int		nPartMax;
    int		*panParts;

} SHPInfo;
typedef SHPInfo * SHPHandle;
#endif

typedef	struct
{
#ifdef SWIG
%immutable;
#endif
    FILE	*fp;

    int		nRecords;

    int		nRecordLength;
    int		nHeaderLength;
    int		nFields;
    int		*panFieldOffset;
    int		*panFieldSize;
    int		*panFieldDecimals;
    char	*pachFieldType;

    char	*pszHeader;

    int		nCurrentRecord;
    int		bCurrentRecordModified;
    char	*pszCurrentRecord;
    
    int		bNoHeader;
    int		bUpdated;

    char 	*pszStringField;
    int		nStringFieldLen;    
#ifdef SWIG
%mutable;
#endif
} DBFInfo;
typedef DBFInfo * DBFHandle;

typedef enum {FTString, FTInteger, FTDouble, FTInvalid} DBFFieldType;

/* Shapefile object, no write access via scripts */
typedef struct {
#ifdef SWIG
%immutable;
#endif
  char source[MS_PATH_LENGTH]; /* full path to this file data */

#ifndef SWIG
  SHPHandle hSHP; /* SHP/SHX file pointer */
#endif

  int type; /* shapefile type */
  int numshapes; /* number of shapes */
  rectObj bounds; /* shape extent */

#ifndef SWIG
  DBFHandle hDBF; /* DBF file pointer */
#endif

  int lastshape;

  char *status;
  rectObj statusbounds; /* holds extent associated with the status vector */

  int isopen;
#ifdef SWIG
%mutable;
#endif
} shapefileObj;

#ifndef SWIG
/* layerInfo structure for tiled shapefiles */
typedef struct { 
  shapefileObj *shpfile;
  shapefileObj *tileshpfile;
  int tilelayerindex;
} msTiledSHPLayerInfo;


/* shapefileObj function prototypes  */
MS_DLL_EXPORT int msSHPOpenFile(shapefileObj *shpfile, char *mode, char *filename);
MS_DLL_EXPORT int msSHPCreateFile(shapefileObj *shpfile, char *filename, int type);
MS_DLL_EXPORT void msSHPCloseFile(shapefileObj *shpfile);
MS_DLL_EXPORT int msSHPWhichShapes(shapefileObj *shpfile, rectObj rect, int debug);

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

/* tiledShapefileObj function prototypes are in map.h */

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
