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
  char source[MS_PATH_LENGTH]; /* full path to the shapefile data */
  SHPHandle hSHP; /* .SHP/.SHX file pointer */
  int type; /* shape type */
  int numshapes; /* number of shapes */
  rectObj bounds; /* shape extent */
  DBFHandle hDBF; /* .DBF file pointer */
  int lastshape;
  char *status;
#ifdef SWIG
%readwrite
#endif
} shapefileObj;

#ifndef SWIG

// shapefileObj function prototypes 
int msSHPOpenFile(shapefileObj *shpfile, char *mode, char *shapepath, char *filename);
int msSHPCreateFile(shapefileObj *shpfile, char *filename, int type);
void msSHPCloseFile(shapefileObj *shpfile);
char *msSHPWhichShapes(shapefileObj *shpfile, rectObj rect, projectionObj *in, projectionObj *out);

// SHP/SHX function prototypes
SHPHandle msSHPOpen( const char * pszShapeFile, const char * pszAccess );
SHPHandle msSHPCreate( const char * pszShapeFile, int nShapeType );
void msSHPClose( SHPHandle hSHP );
void msSHPGetInfo( SHPHandle hSHP, int * pnEntities, int * pnShapeType );
int msSHPReadBounds( SHPHandle psSHP, int hEntity, rectObj *padBounds );
void msSHPReadShape( SHPHandle psSHP, int hEntity, shapeObj *shape );
int msSHPWriteShape( SHPHandle psSHP, shapeObj *shape );

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
char **msDBFGetValueList(DBFHandle dbffile, int record, char **items, int **itemindexes, int numitems);
int msDBFGetItemIndex(DBFHandle dbffile, char *name);

#ifdef __cplusplus
}
#endif

#endif /* MAPSHAPE_H */
