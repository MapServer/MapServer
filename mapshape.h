#ifndef MAPSHAPE_H
#define MAPSHAPE_H

#include <stdio.h>
#include "mapprimitive.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SWIG
#define MS_PATH_LENGTH 1024

// Shapefile types
#define SHP_POINT 1
#define SHP_ARC 3
#define SHP_POLYGON 5
#define SHP_MULTIPOINT 8

#define SHP_POINTM 21
#define SHP_ARCM  23
#define SHP_POLYGONM 25
#define SHP_MULTIPOINTM 28
#endif

#define MS_SHAPEFILE_POINT 1
#define MS_SHAPEFILE_ARC 3
#define MS_SHAPEFILE_POLYGON 5
#define MS_SHAPEFILE_MULTIPOINT 8

#define MS_SHP_POINTM 21
#define MS_SHP_ARCM  23
#define MS_SHP_POLYGONM 25
#define MS_SHP_MULTIPOINTM 28

#ifndef SWIG
typedef unsigned char uchar;

typedef	struct {
    FILE        *fpSHP;
    FILE	*fpSHX;

    int		nShapeType;				/* SHPT_* */
    int		nFileSize;				/* SHP file */

    int         nRecords;
    int		nMaxRecords;
    int		*panRecOffset;
    int		*panRecSize;

    double	adBoundsMin[4];
    double	adBoundsMax[4];

    int		bUpdated;

    int         nBufSize; // these used static vars in shape readers, moved to be thread-safe
    uchar       *pabyRec;
    int         nPartMax;
    int		*panParts;

} SHPInfo;
typedef SHPInfo * SHPHandle;
#endif

typedef	struct
{
#ifdef SWIG
%readonly
#endif
    FILE	*fp;

    int         nRecords;

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
%readwrite
#endif
} DBFInfo;
typedef DBFInfo * DBFHandle;

typedef enum {FTString, FTInteger, FTDouble, FTInvalid} DBFFieldType;

// Shapefile object, no write access via scripts                               
typedef struct {
#ifdef SWIG
%readonly
#endif
  char source[MS_PATH_LENGTH]; // full path to this file data

#ifndef SWIG
  SHPHandle hSHP; // SHP/SHX file pointer
#endif

  int type; // shapefile type
  int numshapes; // number of shapes
  rectObj bounds; // shape extent

#ifndef SWIG
  DBFHandle hDBF; // DBF file pointer 
#endif

  int *indexes; // what items do we need to retrieve from this shapefile
  int numindexes;
  
  int lastshape;

  char *status;
  rectObj statusbounds; // holds extent associated with the status vector

#ifdef SWIG
%readwrite
#endif
} shapefileObj;

#ifndef SWIG

// shapefileObj function prototypes 
int msSHPOpenFile(shapefileObj *shpfile, char *mode, char *filename);
int msSHPCreateFile(shapefileObj *shpfile, char *filename, int type);
void msSHPCloseFile(shapefileObj *shpfile);
int msSHPWhichShapes(shapefileObj *shpfile, rectObj rect);

// SHP/SHX function prototypes
SHPHandle msSHPOpen( const char * pszShapeFile, const char * pszAccess );
SHPHandle msSHPCreate( const char * pszShapeFile, int nShapeType );
void msSHPClose( SHPHandle hSHP );
void msSHPGetInfo( SHPHandle hSHP, int * pnEntities, int * pnShapeType );
int msSHPReadBounds( SHPHandle psSHP, int hEntity, rectObj *padBounds );
void msSHPReadShape( SHPHandle psSHP, int hEntity, shapeObj *shape );
int msSHPReadPoint(SHPHandle psSHP, int hEntity, pointObj *point );
int msSHPWriteShape( SHPHandle psSHP, shapeObj *shape );
int msSHPWritePoint(SHPHandle psSHP, pointObj *point );

// tiledShapefileObj function prototypes are in map.h
#endif

// XBase function prototypes
DBFHandle msDBFOpen( const char * pszDBFFile, const char * pszAccess );
void msDBFClose( DBFHandle hDBF );
DBFHandle msDBFCreate( const char * pszDBFFile );

int msDBFGetFieldCount( DBFHandle psDBF );
int msDBFGetRecordCount( DBFHandle psDBF );
int msDBFAddField( DBFHandle hDBF, const char * pszFieldName, DBFFieldType eType, int nWidth, int nDecimals );

DBFFieldType msDBFGetFieldInfo( DBFHandle psDBF, int iField, char * pszFieldName, int * pnWidth, int * pnDecimals );

int msDBFReadIntegerAttribute( DBFHandle hDBF, int iShape, int iField );
double msDBFReadDoubleAttribute( DBFHandle hDBF, int iShape, int iField );
const char *msDBFReadStringAttribute( DBFHandle hDBF, int iShape, int iField );

int msDBFWriteIntegerAttribute( DBFHandle hDBF, int iShape, int iField, int nFieldValue );
int msDBFWriteDoubleAttribute( DBFHandle hDBF, int iShape, int iField, double dFieldValue );
int msDBFWriteStringAttribute( DBFHandle hDBF, int iShape, int iField, const char * pszFieldValue );

char **msDBFGetItems(DBFHandle dbffile);
char **msDBFGetValues(DBFHandle dbffile, int record);
char **msDBFGetValueList(DBFHandle dbffile, int record, int *itemindexes, int numitems);
int *msDBFGetItemIndexes(DBFHandle dbffile, char **items, int numitems);
int msDBFGetItemIndex(DBFHandle dbffile, char *name);

#ifdef __cplusplus
}
#endif

#endif /* MAPSHAPE_H */
