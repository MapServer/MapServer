#ifndef MAPSHAPE_H
#define MAPSHAPE_H

#include <stdio.h>
#include "mapprimitive.h"
#include "mapproject.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SWIG
#define MS_PATH_LENGTH 1024

#ifdef USE_OGR
// Since OGR comes with shapelib as well, we have to rename the public
// shapelib functions to avoid conflicts

#define SHPOpen                 msSHPOpen
#define SHPClose                msSHPClose
#define SHPCreate               msSHPCreate
#define SHPGetInfo              msSHPGetInfo
#define SHPInfo                 msSHPInfo
#define SHPHandle               msSHPHandle

#define DBFOpen                 msDBFOpen
#define DBFClose                msDBFClose
#define DBFCreate               msDBFCreate
#define DBFGetFieldCount        msDBFGetFieldCount
#define DBFGetRecordCount       msDBFGetRecordCount
#define DBFAddField             msDBFAddField
#define DBFGetFieldInfo         msDBFGetFieldInfo
#define DBFReadIntegerAttribute msDBFReadIntegerAttribute
#define DBFReadDoubleAttribute  msDBFReadDoubleAttribute
#define DBFReadStringAttribute  msDBFReadStringAttribute
#define DBFWriteIntegerAttribute msDBFWriteIntegerAttribute
#define DBFWriteDoubleAttribute msDBFWriteDoubleAttribute
#define DBFWriteStringAttribute msDBFWriteStringAttribute
#define DBFInfo                 msDBFInfo
#define DBFHandle               msDBFHandle
#endif /* USE_OGR */

#endif

// Shapefile types
#define MS_SHP_POINT 1
#define MS_SHP_ARC 3
#define MS_SHP_POLYGON 5
#define MS_SHP_MULTIPOINT 8

#ifndef SWIG
typedef	struct {
    FILE        *fpSHP;
    FILE	*fpSHX;

    int		nShapeType;				/* SHPT_* */
    int		nFileSize;				/* SHP file */

    int         nRecords;
    int		nMaxRecords;
    int		*panRecOffset;
    int		*panRecSize;

    double	adBoundsMin[2];
    double	adBoundsMax[2];

    int		bUpdated;
} SHPInfo;
typedef SHPInfo * SHPHandle;

typedef	struct
{
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
} DBFInfo;
typedef DBFInfo * DBFHandle;

typedef enum {FTString, FTInteger, FTDouble, FTInvalid} DBFFieldType;
#endif

// Shapefile object, no write access via scripts                               
typedef struct {
#ifdef SWIG
%readonly
#endif
  char source[MS_PATH_LENGTH]; /* full path to the shapefile data */
  SHPHandle hSHP; /* .SHP/.SHX file pointer */
  int type; /* shape type */
  int numshapes; /* number of shapes */
  rectObj bounds; /* shape extent */
  DBFHandle hDBF; /* .DBF file pointer */
#ifdef SWIG
%readwrite
#endif
} shapefileObj;

#ifndef SWIG
// shapefileObj functions
int msOpenSHPFile(shapefileObj *shpfile, char *mode, char *path, char *tile, char *filename);
int msCreateSHPFile(shapefileObj *shpfile, char *filename, int type);
void msCloseSHPFile(shapefileObj *shpfile);

// SHP/SHX function prototypes
SHPHandle SHPOpen( const char * pszShapeFile, const char * pszAccess );
SHPHandle SHPCreate( const char * pszShapeFile, int nShapeType );
void SHPClose( SHPHandle hSHP );

void SHPGetInfo( SHPHandle hSHP, int * pnEntities, int * pnShapeType );

#ifdef USE_PROJ
void SHPReadShapeProj( SHPHandle psSHP, int hEntity, shapeObj *shape, projectionObj *in, projectionObj *out );
#endif

int SHPReadBounds( SHPHandle psSHP, int hEntity, rectObj *padBounds );
void SHPReadShape( SHPHandle psSHP, int hEntity, shapeObj *shape );

int SHPWriteShape( SHPHandle psSHP, shapeObj *shape );

// XBase function prototypes
DBFHandle DBFOpen( const char * pszDBFFile, const char * pszAccess );
DBFHandle DBFCreate( const char * pszDBFFile );

int DBFGetFieldCount( DBFHandle psDBF );
int DBFGetRecordCount( DBFHandle psDBF );
int DBFAddField( DBFHandle hDBF, const char * pszFieldName, DBFFieldType eType, int nWidth, int nDecimals );

DBFFieldType DBFGetFieldInfo( DBFHandle psDBF, int iField, char * pszFieldName, int * pnWidth, int * pnDecimals );

int DBFReadIntegerAttribute( DBFHandle hDBF, int iShape, int iField );
double DBFReadDoubleAttribute( DBFHandle hDBF, int iShape, int iField );
const char *DBFReadStringAttribute( DBFHandle hDBF, int iShape, int iField );

int DBFWriteIntegerAttribute( DBFHandle hDBF, int iShape, int iField, int nFieldValue );
int DBFWriteDoubleAttribute( DBFHandle hDBF, int iShape, int iField, double dFieldValue );
int DBFWriteStringAttribute( DBFHandle hDBF, int iShape, int iField, const char * pszFieldValue );

void DBFClose( DBFHandle hDBF );
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPSHAPE_H */
