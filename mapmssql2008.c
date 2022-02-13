/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MS SQL Server Layer Connector
 * Author:   Richard Hillman - based on PostGIS and SpatialDB connectors
 *           Tamas Szekeres - maintenance
 *
 ******************************************************************************
 * Copyright (c) 2007 IS Consulting (www.mapdotnet.com)
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
 * Revision 1.0  2007/7/1
 * Created.
 *
 */
#ifndef _WIN32
#define _GNU_SOURCE
#endif

#define _CRT_SECURE_NO_WARNINGS 1

/* $Id$ */
#include <assert.h>
#include "mapserver.h"
#include "maptime.h"
#include "mapows.h"

#ifdef USE_MSSQL2008

#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <string.h>
#include <ctype.h> /* tolower() */

/*   SqlGeometry/SqlGeography serialization format

Simple Point (SerializationProps & IsSinglePoint)
  [SRID][0x01][SerializationProps][Point][z][m]

Simple Line Segment (SerializationProps & IsSingleLineSegment)
  [SRID][0x01][SerializationProps][Point1][Point2][z1][z2][m1][m2]

Complex Geometries
  [SRID][VersionAttribute][SerializationProps][NumPoints][Point1]..[PointN][z1]..[zN][m1]..[mN]
  [NumFigures][Figure]..[Figure][NumShapes][Shape]..[Shape]

Complex Geometries (FigureAttribute == Curve)
  [SRID][VersionAttribute][SerializationProps][NumPoints][Point1]..[PointN][z1]..[zN][m1]..[mN]
  [NumFigures][Figure]..[Figure][NumShapes][Shape]..[Shape][NumSegments][SegmentType]..[SegmentType]

VersionAttribute (1 byte)
  0x01 = Katmai (MSSQL2008+)
  0x02 = Denali (MSSQL2012+)

SRID
  Spatial Reference Id (4 bytes)

SerializationProps (bitmask) 1 byte
  0x01 = HasZValues
  0x02 = HasMValues
  0x04 = IsValid
  0x08 = IsSinglePoint
  0x10 = IsSingleLineSegment
  0x20 = IsLargerThanAHemisphere

Point (2-4)x8 bytes, size depends on SerializationProps & HasZValues & HasMValues
  [x][y]                  - SqlGeometry
  [latitude][longitude]   - SqlGeography

Figure
  [FigureAttribute][PointOffset]

FigureAttribute - Katmai (1 byte)
  0x00 = Interior Ring
  0x01 = Stroke
  0x02 = Exterior Ring

FigureAttribute - Denali (1 byte)
  0x00 = None
  0x01 = Line
  0x02 = Arc
  0x03 = Curve

Shape
  [ParentFigureOffset][FigureOffset][ShapeType]

ShapeType (1 byte)
  0x00 = Unknown
  0x01 = Point
  0x02 = LineString
  0x03 = Polygon
  0x04 = MultiPoint
  0x05 = MultiLineString
  0x06 = MultiPolygon
  0x07 = GeometryCollection
  -- Denali
  0x08 = CircularString
  0x09 = CompoundCurve
  0x0A = CurvePolygon
  0x0B = FullGlobe

SegmentType (1 byte)
  0x00 = Line
  0x01 = Arc
  0x02 = FirstLine
  0x03 = FirstArc

*/

/* Native geometry parser macros */

/* parser error codes */
#define NOERROR 0
#define NOT_ENOUGH_DATA 1
#define CORRUPT_DATA 2
#define UNSUPPORTED_GEOMETRY_TYPE 3

/* geometry format to transfer geometry column */
#define MSSQLGEOMETRY_NATIVE 0
#define MSSQLGEOMETRY_WKB 1
#define MSSQLGEOMETRY_WKT 2

/* geometry column types */
#define MSSQLCOLTYPE_GEOMETRY  0
#define MSSQLCOLTYPE_GEOGRAPHY 1
#define MSSQLCOLTYPE_BINARY 2
#define MSSQLCOLTYPE_TEXT 3

#define SP_NONE 0
#define SP_HASZVALUES 1
#define SP_HASMVALUES 2
#define SP_ISVALID 4
#define SP_ISSINGLEPOINT 8
#define SP_ISSINGLELINESEGMENT 0x10
#define SP_ISLARGERTHANAHEMISPHERE 0x20

#define ST_UNKNOWN 0
#define ST_POINT 1
#define ST_LINESTRING 2
#define ST_POLYGON 3
#define ST_MULTIPOINT 4
#define ST_MULTILINESTRING 5
#define ST_MULTIPOLYGON 6
#define ST_GEOMETRYCOLLECTION 7
#define ST_CIRCULARSTRING 8
#define ST_COMPOUNDCURVE 9
#define ST_CURVEPOLYGON 10
#define ST_FULLGLOBE 11

#define SMT_LINE 0
#define SMT_ARC 1
#define SMT_FIRSTLINE 2
#define SMT_FIRSTARC 3

#define ReadInt32(nPos) (*((unsigned int*)(gpi->pszData + (nPos))))

#define ReadByte(nPos) (gpi->pszData[nPos])

#define ReadDouble(nPos) (*((double*)(gpi->pszData + (nPos))))

#define ParentOffset(iShape) (ReadInt32(gpi->nShapePos + (iShape) * 9 ))
#define FigureOffset(iShape) (ReadInt32(gpi->nShapePos + (iShape) * 9 + 4))
#define ShapeType(iShape) (ReadByte(gpi->nShapePos + (iShape) * 9 + 8))
#define SegmentType(iSegment) (ReadByte(gpi->nSegmentPos + (iSegment)))

#define NextFigureOffset(iShape) (iShape + 1 < gpi->nNumShapes? FigureOffset((iShape) +1) : gpi->nNumFigures)

#define FigureAttribute(iFigure) (ReadByte(gpi->nFigurePos + (iFigure) * 5))
#define PointOffset(iFigure) (ReadInt32(gpi->nFigurePos + (iFigure) * 5 + 1))
#define NextPointOffset(iFigure) (iFigure + 1 < gpi->nNumFigures? PointOffset((iFigure) +1) : (unsigned)gpi->nNumPoints)

#define ReadX(iPoint) (ReadDouble(gpi->nPointPos + 16 * (iPoint)))
#define ReadY(iPoint) (ReadDouble(gpi->nPointPos + 16 * (iPoint) + 8))
#define ReadZ(iPoint) (ReadDouble(gpi->nPointPos + 16 * gpi->nNumPoints + 8 * (iPoint)))
#define ReadM(iPoint) (ReadDouble(gpi->nPointPos + 24 * gpi->nNumPoints + 8 * (iPoint)))

#define FP_EPSILON 1e-12
#define SEGMENT_ANGLE 5.0
#define SEGMENT_MINPOINTS 10

/* Native geometry parser struct */
typedef struct msGeometryParserInfo_t {
  unsigned char* pszData;
  int nLen;
  /* version */
  char chVersion;
  /* serialization properties */
  char chProps;
  /* point array */
  int nPointSize;
  int nPointPos;
  int nNumPoints;
  int nNumPointsRead;
  /* figure array */
  int nFigurePos;
  int nNumFigures;
  /* shape array */
  int nShapePos;
  int nNumShapes;
  int nSRSId;
  /* segment array */
  int nSegmentPos;
  int nNumSegments;
  /* geometry or geography */
  int nColType;
  /* bounds */
  double minx;
  double miny;
  double maxx;
  double maxy;
} msGeometryParserInfo;

/* Structure for connection to an ODBC database (Microsoft preferred way to connect to SQL Server 2005 from c/c++) */
typedef struct msODBCconn_t {
  SQLHENV     henv;               /* ODBC HENV */
  SQLHDBC     hdbc;               /* ODBC HDBC */
  SQLHSTMT    hstmt;              /* ODBC HSTMNT */
  char        errorMessage[1024]; /* Last error message if any */
} msODBCconn;

typedef struct ms_MSSQL2008_layer_info_t {
  char *sql;           /* sql query to send to DB */
  long row_num;        /* what row is the NEXT to be read (for random access) */
  char *geom_column; /* name of the actual geometry column parsed from the LAYER's DATA field */
  char *geom_column_type;  /* the type of the geometry column */
  char *geom_table;  /* the table name or sub-select decalred in the LAYER's DATA field */
  char *urid_name;     /* name of user-specified unique identifier or OID */
  char *user_srid;     /* zero length = calculate, non-zero means using this value! */
  char *index_name;  /* hopefully this isn't necessary - but if the optimizer ain't cuttin' it... */
  char *sort_spec;  /* the sort by specification which should be applied to the generated select statement */
  int mssqlversion_major; /* the sql server major version number */
  int paging;           /* Driver handling of pagination, enabled by default */
  SQLSMALLINT *itemtypes; /* storing the sql field types for further reference */

  msODBCconn * conn;          /* Connection to db */
  msGeometryParserInfo gpi;   /* struct for the geometry parser */
  int geometry_format;        /* Geometry format to be retrieved from the database */
  tokenListNodeObjPtr current_node; /* filter expression translation */
} msMSSQL2008LayerInfo;

#define SQL_COLUMN_NAME_MAX_LENGTH 128
#define SQL_TABLE_NAME_MAX_LENGTH 128

#define DATA_ERROR_MESSAGE \
    "%s" \
    "Error with MSSQL2008 data variable. You specified '%s'.<br>\n" \
    "Standard ways of specifiying are : <br>\n(1) 'geometry_column from geometry_table' <br>\n(2) 'geometry_column from (&lt;sub query&gt;) as foo using unique &lt;column name&gt; using SRID=&lt;srid#&gt;' <br><br>\n\n" \
    "Make sure you utilize the 'using unique  &lt;column name&gt;' and 'using with &lt;index name&gt;' clauses in.\n\n<br><br>" \
    "For more help, please see http://www.mapdotnet.com \n\n<br><br>" \
    "mapmssql2008.c - version of 2007/7/1.\n"

/* Native geometry parser code */
void ReadPoint(msGeometryParserInfo* gpi, pointObj* p, int iPoint)
{
  if (gpi->nColType == MSSQLCOLTYPE_GEOGRAPHY) {
    p->x = ReadY(iPoint);
    p->y = ReadX(iPoint);
  } else {
    p->x = ReadX(iPoint);
    p->y = ReadY(iPoint);
  }
  /* calculate bounds */
  if (gpi->nNumPointsRead++ == 0) {
    gpi->minx = gpi->maxx = p->x;
    gpi->miny = gpi->maxy = p->y;
  } else {
    if (gpi->minx > p->x) gpi->minx = p->x;
    else if (gpi->maxx < p->x) gpi->maxx = p->x;
    if (gpi->miny > p->y) gpi->miny = p->y;
    else if (gpi->maxy < p->y) gpi->maxy = p->y;
  }

  if ((gpi->chProps & SP_HASZVALUES) && (gpi->chProps & SP_HASMVALUES))
  {
    p->z = ReadZ(iPoint);
    p->m = ReadM(iPoint);
  }
  else if (gpi->chProps & SP_HASZVALUES)
  {
      p->z = ReadZ(iPoint);
      p->m = 0.0;
  }
  else if (gpi->chProps & SP_HASMVALUES)
  {
      p->z = 0.0;
      p->m = ReadZ(iPoint);
  }
  else
  {
      p->z = 0.0;
      p->m = 0.0;
  }
}

int StrokeArcToLine(msGeometryParserInfo* gpi, lineObj* line, int index)
{
    if (index > 1) {
        double x, y, x1, y1, x2, y2, x3, y3, dxa, dya, sxa, sya, dxb, dyb;
        double d, sxb, syb, ox, oy, a1, a3, sa, da, a, radius;
        int numpoints;
        double z;
        z = line->point[index].z; /* must be equal for arc segments */
        /* first point */
        x1 = line->point[index - 2].x;
        y1 = line->point[index - 2].y;
        /* second point */
        x2 = line->point[index - 1].x;
        y2 = line->point[index - 1].y;
        /* third point */
        x3 = line->point[index].x;
        y3 = line->point[index].y;

        sxa = (x1 + x2);
        sya = (y1 + y2);
        dxa = x2 - x1;
        dya = y2 - y1;

        sxb = (x2 + x3);
        syb = (y2 + y3);
        dxb = x3 - x2;
        dyb = y3 - y2;

        d = (dxa * dyb - dya * dxb) * 2;

        if (fabs(d) < FP_EPSILON) {
            /* points are colinear, nothing to do here */
            return index;
        }

        /* calculating the center of circle */
        ox = ((sya - syb) * dya * dyb + sxa * dyb * dxa - sxb * dya * dxb) / d;
        oy = ((sxb - sxa) * dxa * dxb + syb * dyb * dxa - sya * dya * dxb) / d;

        radius = sqrt((x1 - ox) * (x1 - ox) + (y1 - oy) * (y1 - oy));

        /* calculating the angle to be used */
        a1 = atan2(y1 - oy, x1 - ox);
        a3 = atan2(y3 - oy, x3 - ox);

        if (d > 0) {
            /* draw counterclockwise */
            if (a3 > a1) /* Wrapping past 180? */
                sa = a3 - a1;
            else
                sa = a3 - a1 + 2.0 * M_PI ;
        }
        else {
            if (a3 > a1) /* Wrapping past 180? */
                sa = a3 - a1 + 2.0 * M_PI;
            else
                sa = a3 - a1;
        }

        numpoints = (int)floor(fabs(sa) * 180 / SEGMENT_ANGLE / M_PI);
        if (numpoints < SEGMENT_MINPOINTS)
            numpoints = SEGMENT_MINPOINTS;

        da = sa / numpoints;

        /* extend the point array */
        line->numpoints += numpoints - 2;
        line->point = msSmallRealloc(line->point, sizeof(pointObj) * line->numpoints);
        --index;

        a = a1 + da;
        while (numpoints > 1) {
            line->point[index].x = x = ox + radius * cos(a);
            line->point[index].y = y = oy + radius * sin(a);
            line->point[index].z = z;

            /* calculate bounds */
            if (gpi->minx > x) gpi->minx = x;
            else if (gpi->maxx < x) gpi->maxx = x;
            if (gpi->miny > y) gpi->miny = y;
            else if (gpi->maxy < y) gpi->maxy = y;

            a += da;
            ++index;
            --numpoints;
        }
        /* set last point */
        line->point[index].x = x3;
        line->point[index].y = y3;
        line->point[index].z = z;
    }
    return index;
}

int ParseSqlGeometry(msMSSQL2008LayerInfo* layerinfo, shapeObj *shape)
{
  msGeometryParserInfo* gpi = &layerinfo->gpi;

  gpi->nNumPointsRead = 0;

  if (gpi->nLen < 10) {
    msDebug("ParseSqlGeometry NOT_ENOUGH_DATA\n");
    return NOT_ENOUGH_DATA;
  }

  /* store the SRS id for further use */
  gpi->nSRSId = ReadInt32(0);

  gpi->chVersion = ReadByte(4);

  if (gpi->chVersion > 2) {
    msDebug("ParseSqlGeometry CORRUPT_DATA\n");
    return CORRUPT_DATA;
  }

  gpi->chProps = ReadByte(5);

  gpi->nPointSize = 16;

  if ( gpi->chProps & SP_HASMVALUES )
    gpi->nPointSize += 8;

  if ( gpi->chProps & SP_HASZVALUES )
    gpi->nPointSize += 8;

  if ( gpi->chProps & SP_ISSINGLEPOINT ) {
    // single point geometry
    gpi->nNumPoints = 1;
    if (gpi->nLen < 6 + gpi->nPointSize) {
      msDebug("ParseSqlGeometry NOT_ENOUGH_DATA\n");
      return NOT_ENOUGH_DATA;
    }

    shape->type = MS_SHAPE_POINT;
    shape->line = (lineObj *) msSmallMalloc(sizeof(lineObj));
    shape->numlines = 1;
    shape->line[0].point = (pointObj *) msSmallMalloc(sizeof(pointObj));
    shape->line[0].numpoints = 1;
    gpi->nPointPos = 6;

    ReadPoint(gpi, &shape->line[0].point[0], 0);
  } else if ( gpi->chProps & SP_ISSINGLELINESEGMENT ) {
    // single line segment with 2 points
    gpi->nNumPoints = 2;
    if (gpi->nLen < 6 + 2 * gpi->nPointSize) {
      msDebug("ParseSqlGeometry NOT_ENOUGH_DATA\n");
      return NOT_ENOUGH_DATA;
    }

    shape->type = MS_SHAPE_LINE;
    shape->line = (lineObj *) msSmallMalloc(sizeof(lineObj));
    shape->numlines = 1;
    shape->line[0].point = (pointObj *) msSmallMalloc(sizeof(pointObj) * 2);
    shape->line[0].numpoints = 2;
    gpi->nPointPos = 6;

    ReadPoint(gpi, &shape->line[0].point[0], 0);
    ReadPoint(gpi, &shape->line[0].point[1], 1);
  } else {
    int iShape, iFigure, iSegment = 0;
    // complex geometries
    gpi->nNumPoints = ReadInt32(6);

    if ( gpi->nNumPoints <= 0 ) {
      return NOERROR;
    }

    // position of the point array
    gpi->nPointPos = 10;

    // position of the figures
    gpi->nFigurePos = gpi->nPointPos + gpi->nPointSize * gpi->nNumPoints + 4;

    if (gpi->nLen < gpi->nFigurePos) {
      msDebug("ParseSqlGeometry NOT_ENOUGH_DATA\n");
      return NOT_ENOUGH_DATA;
    }

    gpi->nNumFigures = ReadInt32(gpi->nFigurePos - 4);

    if ( gpi->nNumFigures <= 0 ) {
      return NOERROR;
    }

    // position of the shapes
    gpi->nShapePos = gpi->nFigurePos + 5 * gpi->nNumFigures + 4;

    if (gpi->nLen < gpi->nShapePos) {
      msDebug("ParseSqlGeometry NOT_ENOUGH_DATA\n");
      return NOT_ENOUGH_DATA;
    }

    gpi->nNumShapes = ReadInt32(gpi->nShapePos - 4);

    if (gpi->nLen < gpi->nShapePos + 9 * gpi->nNumShapes) {
      msDebug("ParseSqlGeometry NOT_ENOUGH_DATA\n");
      return NOT_ENOUGH_DATA;
    }

    if ( gpi->nNumShapes <= 0 ) {
      return NOERROR;
    }

    // pick up the root shape
    if ( ParentOffset(0) != 0xFFFFFFFF) {
      msDebug("ParseSqlGeometry CORRUPT_DATA\n");
      return CORRUPT_DATA;
    }

    // determine the shape type
    for (iShape = 0; iShape < gpi->nNumShapes; iShape++) {
      unsigned char shapeType = ShapeType(iShape);
      if (shapeType == ST_POINT || shapeType == ST_MULTIPOINT) {
        shape->type = MS_SHAPE_POINT;
        break;
      } else if (shapeType == ST_LINESTRING || shapeType == ST_MULTILINESTRING || 
                 shapeType == ST_CIRCULARSTRING || shapeType == ST_COMPOUNDCURVE) {
        shape->type = MS_SHAPE_LINE;
        break;
      } else if (shapeType == ST_POLYGON || shapeType == ST_MULTIPOLYGON || 
                 shapeType == ST_CURVEPOLYGON)
      {
        shape->type = MS_SHAPE_POLYGON;
        break;
      }
    }

    shape->line = (lineObj *) msSmallMalloc(sizeof(lineObj) * gpi->nNumFigures);
    shape->numlines = gpi->nNumFigures;
    gpi->nNumSegments = 0;

    // read figures
    for (iFigure = 0; iFigure < gpi->nNumFigures; iFigure++) {
      int iPoint, iNextPoint, i;
      iPoint = PointOffset(iFigure);
      iNextPoint = NextPointOffset(iFigure);

      shape->line[iFigure].point = (pointObj *) msSmallMalloc(sizeof(pointObj)*(iNextPoint - iPoint));
      shape->line[iFigure].numpoints = iNextPoint - iPoint;
      i = 0;

      if (gpi->chVersion == 0x02 && FigureAttribute(iFigure) >= 0x02) {
          int nPointPrepared = 0;
          lineObj* line = &shape->line[iFigure];
          if (FigureAttribute(iFigure) == 0x03) {
              if (gpi->nNumSegments == 0) {
                  /* position of the segment types */
                  gpi->nSegmentPos = gpi->nShapePos + 9 * gpi->nNumShapes + 4;
                  gpi->nNumSegments = ReadInt32(gpi->nSegmentPos - 4);
                  if (gpi->nLen < gpi->nSegmentPos + gpi->nNumSegments) {
                      msDebug("ParseSqlGeometry NOT_ENOUGH_DATA\n");
                      return NOT_ENOUGH_DATA;
                  }
                  iSegment = 0;
              }             
             
              while (iPoint < iNextPoint && iSegment < gpi->nNumSegments) {
                  ReadPoint(gpi, &line->point[i], iPoint);
                  ++iPoint;
                  ++nPointPrepared;

                  if (nPointPrepared == 2 && (SegmentType(iSegment) == SMT_FIRSTLINE || SegmentType(iSegment) == SMT_LINE)) {
                      ++iSegment;
                      nPointPrepared = 1;
                  }
                  else if (nPointPrepared == 3 && (SegmentType(iSegment) == SMT_FIRSTARC || SegmentType(iSegment) == SMT_ARC)) {
                      i = StrokeArcToLine(gpi, line, i);
                      ++iSegment;
                      nPointPrepared = 1;
                  }
                  ++i;
              }
          }
          else {
              while (iPoint < iNextPoint) {
                  ReadPoint(gpi, &line->point[i], iPoint);
                  ++iPoint;
                  ++nPointPrepared;
                  if (nPointPrepared == 3) {
                      i = StrokeArcToLine(gpi, line, i);
                      nPointPrepared = 1;
                  }
                  ++i;
              }
          }
      }
      else {
          while (iPoint < iNextPoint) {
              ReadPoint(gpi, &shape->line[iFigure].point[i], iPoint);
              ++iPoint;
              ++i;
          }
      }
    }
  }

  /* set bounds */
  shape->bounds.minx = gpi->minx;
  shape->bounds.miny = gpi->miny;
  shape->bounds.maxx = gpi->maxx;
  shape->bounds.maxy = gpi->maxy;

  return NOERROR;
}

/* MS SQL driver code*/

msMSSQL2008LayerInfo *getMSSQL2008LayerInfo(const layerObj *layer)
{
  return layer->layerinfo;
}

void setMSSQL2008LayerInfo(layerObj *layer, msMSSQL2008LayerInfo *MSSQL2008layerinfo)
{
  layer->layerinfo = (void*) MSSQL2008layerinfo;
}

void handleSQLError(layerObj *layer)
{
  SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
  SQLINTEGER     NativeError;
  SQLSMALLINT   i, MsgLen;
  SQLRETURN  rc;
  msMSSQL2008LayerInfo *layerinfo = getMSSQL2008LayerInfo(layer);

  if (layerinfo == NULL)
    return;

  // Get the status records.
  i = 1;
  while ((rc = SQLGetDiagRec(SQL_HANDLE_STMT, layerinfo->conn->hstmt, i, SqlState, &NativeError,
                             Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) {
    if(layer->debug) {
      msDebug("SQLError: %s\n", Msg);
    }
    i++;
  }
}

/* TODO Take a look at glibc's strcasestr */
static char *strstrIgnoreCase(const char *haystack, const char *needle)
{
  char    *hay_lower;
  char    *needle_lower;
  size_t  len_hay,len_need, match;
  int    found = MS_FALSE;
  char    *loc;

  len_hay = strlen(haystack);
  len_need = strlen(needle);

  hay_lower = (char*) msSmallMalloc(len_hay + 1);
  needle_lower =(char*) msSmallMalloc(len_need + 1);

  size_t t;
  for(t = 0; t < len_hay; t++) {
    hay_lower[t] = (char)tolower(haystack[t]);
  }
  hay_lower[t] = 0;

  for(t = 0; t < len_need; t++) {
    needle_lower[t] = (char)tolower(needle[t]);
  }
  needle_lower[t] = 0;

  loc = strstr(hay_lower, needle_lower);
  if(loc) {
    match = loc - hay_lower;
    found = MS_TRUE;
  }

  msFree(hay_lower);
  msFree(needle_lower);

  return (char *) (found == MS_FALSE ? NULL : haystack + match);
}

static int msMSSQL2008LayerParseData(layerObj *layer, char **geom_column_name, char **geom_column_type, char **table_name, char **urid_name, char **user_srid, char **index_name, char **sort_spec, int debug);

/* Close connection and handles */
static void msMSSQL2008CloseConnection(void *conn_handle)
{
  msODBCconn * conn = (msODBCconn *) conn_handle;

  if (!conn) {
    return;
  }

  if (conn->hstmt) {
    SQLFreeHandle(SQL_HANDLE_STMT, conn->hstmt);
  }
  if (conn->hdbc) {
    SQLDisconnect(conn->hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, conn->hdbc);
  }
  if (conn->henv) {
    SQLFreeHandle(SQL_HANDLE_ENV, conn->henv);
  }

  msFree(conn);
}

/* Set the error string for the connection */
static void setConnError(msODBCconn *conn)
{
  SQLSMALLINT len;

  SQLGetDiagField(SQL_HANDLE_DBC, conn->hdbc, 1, SQL_DIAG_MESSAGE_TEXT, (SQLPOINTER) conn->errorMessage, sizeof(conn->errorMessage), &len);

  conn->errorMessage[len] = 0;
}

#ifdef USE_ICONV
static SQLWCHAR* convertCwchartToSQLWCHAR(const wchar_t* inStr)
{
    SQLWCHAR* outStr;
    int i, len;
    for( len = 0; inStr[len] != 0; ++len )
    {
        /* do nothing */
    }
    outStr = (SQLWCHAR*)msSmallMalloc(sizeof(SQLWCHAR) * (len + 1));
    for( i = 0; i <= len; i++ )
    {
        outStr[i] = (SQLWCHAR)inStr[i];
    }
    return outStr;
}
#endif

/* Connect to db */
static msODBCconn * mssql2008Connect(const char * connString)
{
  SQLSMALLINT outConnStringLen;
  SQLRETURN rc;
  msODBCconn * conn = msSmallMalloc(sizeof(msODBCconn));
  char fullConnString[1024];

  memset(conn, 0, sizeof(*conn));

  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &conn->henv);

  SQLSetEnvAttr(conn->henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0);

  SQLAllocHandle(SQL_HANDLE_DBC, conn->henv, &conn->hdbc);

  if (strcasestr(connString, "DRIVER=") == 0)
  {
      snprintf(fullConnString, sizeof(fullConnString), "DRIVER={SQL Server};%s", connString);

      connString = fullConnString;
  }

  {
#ifdef USE_ICONV
      wchar_t *decodedConnString = msConvertWideStringFromUTF8(connString, "UCS-2LE");
      SQLWCHAR outConnString[1024];
      SQLWCHAR* decodedConnStringSQLWCHAR = convertCwchartToSQLWCHAR(decodedConnString);
      rc = SQLDriverConnectW(conn->hdbc, NULL, decodedConnStringSQLWCHAR, SQL_NTS, outConnString, 1024, &outConnStringLen, SQL_DRIVER_NOPROMPT);
      msFree(decodedConnString);
      msFree(decodedConnStringSQLWCHAR);
#else
      SQLCHAR outConnString[1024];
      rc = SQLDriverConnect(conn->hdbc, NULL, (SQLCHAR*)connString, SQL_NTS, outConnString, 1024, &outConnStringLen, SQL_DRIVER_NOPROMPT);
#endif
  }  

  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    setConnError(conn);

    return conn;
  }

  SQLAllocHandle(SQL_HANDLE_STMT, conn->hdbc, &conn->hstmt);

  return conn;
}

/* Set the error string for the statement execution */
static void setStmntError(msODBCconn *conn)
{
  SQLSMALLINT len;

  SQLGetDiagField(SQL_HANDLE_STMT, conn->hstmt, 1, SQL_DIAG_MESSAGE_TEXT, (SQLPOINTER) conn->errorMessage, sizeof(conn->errorMessage), &len);

  conn->errorMessage[len] = 0;
}

/* Execute SQL against connection. Set error string  if failed */
static int executeSQL(msODBCconn *conn, const char * sql)
{
  SQLRETURN rc;

  SQLCloseCursor(conn->hstmt);

#ifdef USE_ICONV
  {
    wchar_t *decodedSql = msConvertWideStringFromUTF8(sql, "UCS-2LE");
    SQLWCHAR* decodedSqlSQLWCHAR = convertCwchartToSQLWCHAR(decodedSql);
    rc = SQLExecDirectW(conn->hstmt, decodedSqlSQLWCHAR, SQL_NTS);
    msFree(decodedSql);
    msFree(decodedSqlSQLWCHAR);
  }
#else
  rc = SQLExecDirect(conn->hstmt, (SQLCHAR *) sql, SQL_NTS);
#endif

  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
    return 1;
  } else {
    setStmntError(conn);
    return 0;
  }
}

/* Get columns name from query results */
static int columnName(msODBCconn *conn, int index, char *buffer, int bufferLength, layerObj *layer, char pass_field_def, SQLSMALLINT *itemType)
{
  SQLRETURN rc;

  SQLCHAR columnName[SQL_COLUMN_NAME_MAX_LENGTH + 1];
  SQLSMALLINT columnNameLen;
  SQLSMALLINT dataType;
  SQLULEN columnSize;
  SQLSMALLINT decimalDigits;
  SQLSMALLINT nullable;

  rc = SQLDescribeCol(
         conn->hstmt,
         (SQLUSMALLINT)index,
         columnName,
         SQL_COLUMN_NAME_MAX_LENGTH,
         &columnNameLen,
         &dataType,
         &columnSize,
         &decimalDigits,
         &nullable);

  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
    if (bufferLength < SQL_COLUMN_NAME_MAX_LENGTH + 1)
      strlcpy(buffer, (const char *)columnName, bufferLength);
    else
      strlcpy(buffer, (const char *)columnName, SQL_COLUMN_NAME_MAX_LENGTH + 1);

    *itemType = dataType;

    if (pass_field_def) {
      char gml_width[32], gml_precision[32];
      const char *gml_type = NULL;

      gml_width[0] = '\0';
      gml_precision[0] = '\0';

      switch( dataType ) {
        case SQL_INTEGER:
        case SQL_SMALLINT:
        case SQL_TINYINT:
          gml_type = "Integer";
          break;

        case SQL_BIGINT:
          gml_type = "Long";
          break;

        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
        case SQL_DECIMAL:
        case SQL_NUMERIC:
          gml_type = "Real";
          if( decimalDigits > 0 )
            sprintf( gml_precision, "%d", decimalDigits );
          break;

        case SQL_TYPE_DATE:
          gml_type = "Date";
          break;
        case SQL_TYPE_TIME:
          gml_type = "Time";
          break;
        case SQL_TYPE_TIMESTAMP:
          gml_type = "DateTime";
          break;

        case SQL_BIT:
          gml_type = "Boolean";
          break;

        default:
          gml_type = "Character";
          break;
      }

      if( columnSize > 0 )
            sprintf( gml_width, "%u", (unsigned int)columnSize );

      msUpdateGMLFieldMetadata(layer, buffer, gml_type, gml_width, gml_precision, (const short) nullable);
    }
    return 1;
  } else {
    setStmntError(conn);
    return 0;
  }
}

/* open up a connection to the MS SQL 2008 database using the connection string in layer->connection */
/* ie. "driver=<driver>;server=<server>;database=<database>;integrated security=?;user id=<username>;password=<password>" */
int msMSSQL2008LayerOpen(layerObj *layer)
{
  msMSSQL2008LayerInfo  *layerinfo;
  char                *index, *maskeddata;
  int                 i, count;
  char *conn_decrypted = NULL;

  if(layer->debug) {
    msDebug("msMSSQL2008LayerOpen called datastatement: %s\n", layer->data);
  }

  if(getMSSQL2008LayerInfo(layer)) {
    if(layer->debug) {
      msDebug("msMSSQL2008LayerOpen :: layer is already open!!\n");
    }
    return MS_SUCCESS;  /* already open */
  }

  if(!layer->data) {
    msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerOpen()", "", "Error parsing MSSQL2008 data variable: nothing specified in DATA statement.<br><br>\n\nMore Help:<br><br>\n\n");

    return MS_FAILURE;
  }

  if(!layer->connection) {
    msSetError( MS_QUERYERR, "MSSQL connection parameter not specified.", "msMSSQL2008LayerOpen()");
    return MS_FAILURE;
  }

  /* have to setup a connection to the database */

  layerinfo = (msMSSQL2008LayerInfo*) msSmallMalloc(sizeof(msMSSQL2008LayerInfo));

  layerinfo->sql = NULL; /* calc later */
  layerinfo->row_num = 0;
  layerinfo->geom_column = NULL;
  layerinfo->geom_column_type = NULL;
  layerinfo->geom_table = NULL;
  layerinfo->urid_name = NULL;
  layerinfo->user_srid = NULL;
  layerinfo->index_name = NULL;
  layerinfo->sort_spec = NULL;
  layerinfo->conn = NULL;
  layerinfo->itemtypes = NULL;
  layerinfo->mssqlversion_major = 0;
  layerinfo->paging = MS_TRUE;

  layerinfo->conn = (msODBCconn *) msConnPoolRequest(layer);

  if(!layerinfo->conn) {
    if(layer->debug) {
      msDebug("MSMSSQL2008LayerOpen -- shared connection not available.\n");
    }

    /* Decrypt any encrypted token in connection and attempt to connect */
    conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);
    if (conn_decrypted == NULL) {
      return(MS_FAILURE);  /* An error should already have been produced */
    }

    layerinfo->conn = mssql2008Connect(conn_decrypted);

    msFree(conn_decrypted);
    conn_decrypted = NULL;

    if(!layerinfo->conn || layerinfo->conn->errorMessage[0]) {
      char *errMess = "Out of memory";

      msDebug("FAILURE!!!");
      maskeddata = (char *)msSmallMalloc(strlen(layer->connection) + 1);
      strcpy(maskeddata, layer->connection);
      index = strstr(maskeddata, "password=");
      if(index != NULL) {
        index = (char *)(index + 9);
        count = (int)(strstr(index, " ") - index);
        for(i = 0; i < count; i++) {
          strlcpy(index, "*", (int)1);
          index++;
        }
      }

      if (layerinfo->conn) {
        errMess = layerinfo->conn->errorMessage;
      }

      msSetError(MS_QUERYERR,
                 "Couldnt make connection to MS SQL Server 2008 with connect string '%s'.\n<br>\n"
                 "Error reported was '%s'.\n<br>\n\n"
                 "This error occured when trying to make a connection to the specified SQL server.  \n"
                 "<br>\nMost commonly this is caused by <br>\n"
                 "(1) incorrect connection string <br>\n"
                 "(2) you didn't specify a 'user id=...' in your connection string <br>\n"
                 "(3) SQL server isnt running <br>\n"
                 "(4) TCPIP not enabled for SQL Client or server <br>\n\n",
                 "msMSSQL2008LayerOpen()", maskeddata, errMess);

      msFree(maskeddata);
      msMSSQL2008CloseConnection(layerinfo->conn);
      msFree(layerinfo);
      return MS_FAILURE;
    }

    msConnPoolRegister(layer, layerinfo->conn, msMSSQL2008CloseConnection);
  }

  setMSSQL2008LayerInfo(layer, layerinfo);

  if (msMSSQL2008LayerParseData(layer, &layerinfo->geom_column, &layerinfo->geom_column_type, &layerinfo->geom_table, &layerinfo->urid_name, &layerinfo->user_srid, &layerinfo->index_name, &layerinfo->sort_spec, layer->debug) != MS_SUCCESS) {
    msSetError( MS_QUERYERR, "Could not parse the layer data", "msMSSQL2008LayerOpen()");
    return MS_FAILURE;
  }

  /* identify the geometry transfer type */
  if (msLayerGetProcessingKey( layer, "MSSQL_READ_WKB" ) != NULL)
    layerinfo->geometry_format = MSSQLGEOMETRY_WKB;
  else {
    layerinfo->geometry_format = MSSQLGEOMETRY_NATIVE;
    if (strcasecmp(layerinfo->geom_column_type, "geography") == 0)
      layerinfo->gpi.nColType = MSSQLCOLTYPE_GEOGRAPHY;
    else
      layerinfo->gpi.nColType = MSSQLCOLTYPE_GEOMETRY;
  }

  return MS_SUCCESS;
}

/* Return MS_TRUE if layer is open, MS_FALSE otherwise. */
int msMSSQL2008LayerIsOpen(layerObj *layer)
{
  return getMSSQL2008LayerInfo(layer) ? MS_TRUE : MS_FALSE;
}


/* Free the itemindexes array in a layer. */
void msMSSQL2008LayerFreeItemInfo(layerObj *layer)
{
  if(layer->debug) {
    msDebug("msMSSQL2008LayerFreeItemInfo called\n");
  }

  if(layer->iteminfo) {
    msFree(layer->iteminfo);
  }

  layer->iteminfo = NULL;
}


/* allocate the iteminfo index array - same order as the item list */
int msMSSQL2008LayerInitItemInfo(layerObj *layer)
{
  int     i;
  int     *itemindexes ;

  if (layer->debug) {
    msDebug("msMSSQL2008LayerInitItemInfo called\n");
  }

  if(layer->numitems == 0) {
    return MS_SUCCESS;
  }

  msFree(layer->iteminfo);

  layer->iteminfo = (int *) msSmallMalloc(sizeof(int) * layer->numitems);

  itemindexes = (int*)layer->iteminfo;

  for(i = 0; i < layer->numitems; i++) {
    itemindexes[i] = i; /* last one is always the geometry one - the rest are non-geom */
  }

  return MS_SUCCESS;
}

static int getMSSQLMajorVersion(layerObj* layer)
{
  msMSSQL2008LayerInfo  *layerinfo = getMSSQL2008LayerInfo(layer);
  if (layerinfo == NULL)
    return 0;

  if (layerinfo->mssqlversion_major == 0) {
    const char* mssqlversion_major = msLayerGetProcessingKey(layer, "MSSQL_VERSION_MAJOR");
    if (mssqlversion_major != NULL) {
      layerinfo->mssqlversion_major = atoi(mssqlversion_major);
    }
    else {
      /* need to query from database */
      if (executeSQL(layerinfo->conn, "SELECT SERVERPROPERTY('ProductVersion')")) {
        SQLRETURN rc = SQLFetch(layerinfo->conn->hstmt);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
          /* process results */
          char result_data[256];
          SQLLEN retLen = 0;

          rc = SQLGetData(layerinfo->conn->hstmt, 1, SQL_C_CHAR, result_data, sizeof(result_data), &retLen);

          if (rc != SQL_ERROR) {
            result_data[retLen] = 0;
            layerinfo->mssqlversion_major = atoi(result_data);
          }
        }
      }
    }
  }

  return layerinfo->mssqlversion_major;
}

static int addFilter(layerObj *layer, char **query)
{
  if (layer->filter.native_string) {
    (*query) = msStringConcatenate(*query, " WHERE (");
    (*query) = msStringConcatenate(*query, layer->filter.native_string);
    (*query) = msStringConcatenate(*query, ")");
    return MS_TRUE;
  }
  else if (msLayerGetProcessingKey(layer, "NATIVE_FILTER") != NULL) {
    (*query) = msStringConcatenate(*query, " WHERE (");
    (*query) = msStringConcatenate(*query, msLayerGetProcessingKey(layer, "NATIVE_FILTER"));
    (*query) = msStringConcatenate(*query, ")");
    return MS_TRUE;
  }

  return MS_FALSE;
}

/* Get the layer extent as specified in the mapfile or a largest area */
/* covering all features */
int msMSSQL2008LayerGetExtent(layerObj *layer, rectObj *extent)
{
    msMSSQL2008LayerInfo *layerinfo;
    char *query = 0;
    char result_data[256];
    SQLLEN retLen;
    SQLRETURN rc;
    
    if(layer->debug) {
      msDebug("msMSSQL2008LayerGetExtent called\n");
    }

    if (!(layer->extent.minx == -1.0 && layer->extent.miny == -1.0 && 
        layer->extent.maxx == -1.0 && layer->extent.maxy == -1.0)) {
      /* extent was already set */
      extent->minx = layer->extent.minx;
      extent->miny = layer->extent.miny;
      extent->maxx = layer->extent.maxx;
      extent->maxy = layer->extent.maxy;
    }

    layerinfo = getMSSQL2008LayerInfo(layer);

    if (!layerinfo) {
        msSetError(MS_QUERYERR, "GetExtent called with layerinfo = NULL", "msMSSQL2008LayerGetExtent()");
        return MS_FAILURE;
    }

    /* set up statement */
    if (getMSSQLMajorVersion(layer) >= 11) {
      if (strcasecmp(layerinfo->geom_column_type, "geography") == 0) {
        query = msStringConcatenate(query, "WITH extent(extentcol) AS (SELECT geometry::EnvelopeAggregate(geometry::STGeomFromWKB(");
        query = msStringConcatenate(query, layerinfo->geom_column);
        query = msStringConcatenate(query, ".STAsBinary(), ");
        query = msStringConcatenate(query, layerinfo->geom_column);
        query = msStringConcatenate(query, ".STSrid)");
      }
      else {
        query = msStringConcatenate(query, "WITH extent(extentcol) AS (SELECT geometry::EnvelopeAggregate(");
        query = msStringConcatenate(query, layerinfo->geom_column);        
      }
      query = msStringConcatenate(query, ") AS extentcol FROM ");
      query = msStringConcatenate(query, layerinfo->geom_table);

      /* adding attribute filter */
      addFilter(layer, &query);

      query = msStringConcatenate(query, ") SELECT extentcol.STPointN(1).STX, extentcol.STPointN(1).STY, extentcol.STPointN(3).STX, extentcol.STPointN(3).STY FROM extent");
    }
    else {
      if (strcasecmp(layerinfo->geom_column_type, "geography") == 0) {
        query = msStringConcatenate(query, "WITH ENVELOPE as (SELECT geometry::STGeomFromWKB(");
        query = msStringConcatenate(query, layerinfo->geom_column);
        query = msStringConcatenate(query, ".STAsBinary(), ");
        query = msStringConcatenate(query, layerinfo->geom_column);
        query = msStringConcatenate(query, ".STSrid)");
      }
      else {
        query = msStringConcatenate(query, "WITH ENVELOPE as (SELECT ");
        query = msStringConcatenate(query, layerinfo->geom_column);      
      }
      query = msStringConcatenate(query, ".STEnvelope() as envelope from ");
      query = msStringConcatenate(query, layerinfo->geom_table);

      /* adding attribute filter */
      addFilter(layer, &query);

      query = msStringConcatenate(query, "), CORNERS as (SELECT envelope.STPointN(1) as point from ENVELOPE UNION ALL select envelope.STPointN(3) from ENVELOPE) SELECT MIN(point.STX), MIN(point.STY), MAX(point.STX), MAX(point.STY) FROM CORNERS");
    }

    if (!executeSQL(layerinfo->conn, query)) {
        msFree(query);
        return MS_FAILURE;
    }

    msFree(query);

    /* process results */
    rc = SQLFetch(layerinfo->conn->hstmt);

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        if (layer->debug) {
            msDebug("msMSSQL2008LayerGetExtent: No results found.\n");
        }
        return MS_FAILURE;
    }

    rc = SQLGetData(layerinfo->conn->hstmt, 1, SQL_C_CHAR, result_data, sizeof(result_data), &retLen);
    if (rc == SQL_ERROR || retLen < 0) {
        msSetError(MS_QUERYERR, "Failed to get MinX value", "msMSSQL2008LayerGetExtent()");
        return MS_FAILURE;
    }

    result_data[retLen] = 0;

    extent->minx = atof(result_data);

    rc = SQLGetData(layerinfo->conn->hstmt, 2, SQL_C_CHAR, result_data, sizeof(result_data), &retLen);
    if (rc == SQL_ERROR || retLen < 0) {
        msSetError(MS_QUERYERR, "Failed to get MinY value", "msMSSQL2008LayerGetExtent()");
        return MS_FAILURE;
    }

    result_data[retLen] = 0;

    extent->miny = atof(result_data);

    rc = SQLGetData(layerinfo->conn->hstmt, 3, SQL_C_CHAR, result_data, sizeof(result_data), &retLen);
    if (rc == SQL_ERROR || retLen < 0) {
        msSetError(MS_QUERYERR, "Failed to get MaxX value", "msMSSQL2008LayerGetExtent()");
        return MS_FAILURE;
    }

    result_data[retLen] = 0;

    extent->maxx = atof(result_data);

    rc = SQLGetData(layerinfo->conn->hstmt, 4, SQL_C_CHAR, result_data, sizeof(result_data), &retLen);
    if (rc == SQL_ERROR || retLen < 0) {
        msSetError(MS_QUERYERR, "Failed to get MaxY value", "msMSSQL2008LayerGetExtent()");
        return MS_FAILURE;
    }

    result_data[retLen] = 0;

    extent->maxy = atof(result_data);

    return MS_SUCCESS;
}

/* Get the layer feature count */
int msMSSQL2008LayerGetNumFeatures(layerObj *layer)
{
    msMSSQL2008LayerInfo *layerinfo;
    char *query = 0;
    char result_data[256];
    SQLLEN retLen;
    SQLRETURN rc;

    if (layer->debug) {
        msDebug("msMSSQL2008LayerGetNumFeatures called\n");
    }

    layerinfo = getMSSQL2008LayerInfo(layer);

    if (!layerinfo) {
        msSetError(MS_QUERYERR, "GetNumFeatures called with layerinfo = NULL", "msMSSQL2008LayerGetNumFeatures()");
        return -1;
    }

    /* set up statement */
    query = msStringConcatenate(query, "SELECT count(*) FROM ");
    query = msStringConcatenate(query, layerinfo->geom_table);

    /* adding attribute filter */
    addFilter(layer, &query);

    if (!executeSQL(layerinfo->conn, query)) {
        msFree(query);
        return -1;
    }

    msFree(query);

    rc = SQLFetch(layerinfo->conn->hstmt);

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        if (layer->debug) {
            msDebug("msMSSQL2008LayerGetNumFeatures: No results found.\n");
        }

        return -1;
    }

    rc = SQLGetData(layerinfo->conn->hstmt, 1, SQL_C_CHAR, result_data, sizeof(result_data), &retLen);

    if (rc == SQL_ERROR) {
        msSetError(MS_QUERYERR, "Failed to get feature count", "msMSSQL2008LayerGetNumFeatures()");
        return -1;
    }

    result_data[retLen] = 0;

    return atoi(result_data);
}

/* Prepare and execute the SQL statement for this layer */
static int prepare_database(layerObj *layer, rectObj rect, char **query_string)
{
  msMSSQL2008LayerInfo *layerinfo;
  char *query = 0;
  char *data_source = 0;
  char *f_table_name = 0;
  char *geom_table = 0;
  char *tmp = 0;
  char *paging_query = 0;
  /*
    "Geometry::STGeomFromText('POLYGON(())',)" + terminator = 40 chars
    Plus 10 formatted doubles (15 digits of precision, a decimal point, a space/comma delimiter each = 17 chars each)
    Plus SRID + comma - if SRID is a long...we'll be safe with 10 chars

  	or for geography columns
	
    "Geography::STGeomFromText('CURVEPOLYGON(())',)" + terminator = 46 chars
    Plus 18 formatted doubles (15 digits of precision, a decimal point, a space/comma delimiter each = 17 chars each)
    Plus SRID + comma - if SRID is a long...we'll be safe with 10 chars
  */
  char        box3d[46 + 18 * 22 + 11];
  int         t;

  char        *pos_from, *pos_ftab, *pos_space, *pos_paren;
  int hasFilter = MS_FALSE;
  const rectObj rectInvalid = MS_INIT_INVALID_RECT;
  const int bIsValidRect = memcmp(&rect, &rectInvalid, sizeof(rect)) != 0;

  layerinfo =  getMSSQL2008LayerInfo(layer);

  /* Extract the proper f_table_name from the geom_table string.
   * We are expecting the geom_table to be either a single word
   * or a sub-select clause that possibly includes a join --
   *
   * (select column[,column[,...]] from ftab[ natural join table2]) as foo
   *
   * We are expecting whitespace or a ')' after the ftab name.
   *
   */

  geom_table = msStrdup(layerinfo->geom_table);
  pos_from = strstrIgnoreCase(geom_table, " from ");

  if(!pos_from) {
    f_table_name = (char *) msSmallMalloc(strlen(geom_table) + 1);
    strcpy(f_table_name, geom_table);
  } else { /* geom_table is a sub-select clause */
    pos_ftab = pos_from + 6; /* This should be the start of the ftab name */
    pos_space = strstr(pos_ftab, " "); /* First space */

    /* TODO strrchr is POSIX and C99, rindex is neither */
#if defined(_WIN32) && !defined(__CYGWIN__)
    pos_paren = strrchr(pos_ftab, ')');
#else
    pos_paren = rindex(pos_ftab, ')');
#endif

    if(!pos_space || !pos_paren) {
      msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "prepare_database()", geom_table, "Error parsing MSSQL2008 data variable: Something is wrong with your subselect statement.<br><br>\n\nMore Help:<br><br>\n\n");

      return MS_FAILURE;
    }

    if (pos_paren < pos_space) { /* closing parenthesis preceeds any space */
      f_table_name = (char *) msSmallMalloc(pos_paren - pos_ftab + 1);
      strlcpy(f_table_name, pos_ftab, pos_paren - pos_ftab + 1);
    } else {
      f_table_name = (char *) msSmallMalloc(pos_space - pos_ftab + 1);
      strlcpy(f_table_name, pos_ftab, pos_space - pos_ftab + 1);
    }
  }

  if (rect.minx == rect.maxx || rect.miny == rect.maxy) {
      /* create point shape for rectangles with zero area */
      sprintf(box3d, "%s::STGeomFromText('POINT(%.15g %.15g)',%s)", /* %s.STSrid)", */
          layerinfo->geom_column_type, rect.minx, rect.miny, layerinfo->user_srid);
  } else if (strcasecmp(layerinfo->geom_column_type, "geography") == 0) {
	  /* SQL Server has a problem when x is -180 or 180 */  
	  double minx = rect.minx <= -180? -179.999: rect.minx;
	  double maxx = rect.maxx >= 180? 179.999: rect.maxx;
	  double miny = rect.miny < -90? -90: rect.miny;
	  double maxy = rect.maxy > 90? 90: rect.maxy;
	  sprintf(box3d, "Geography::STGeomFromText('CURVEPOLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))',%s)", /* %s.STSrid)", */
          minx, miny,
          minx + (maxx - minx) / 2, miny,
          maxx, miny,
          maxx, miny + (maxy - miny) / 2,
          maxx, maxy,
          minx + (maxx - minx) / 2, maxy,
          minx, maxy,
          minx, miny + (maxy - miny) / 2,
          minx, miny,
          layerinfo->user_srid);  
  } else {
      sprintf(box3d, "Geometry::STGeomFromText('POLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))',%s)", /* %s.STSrid)", */
          rect.minx, rect.miny,
          rect.maxx, rect.miny,
          rect.maxx, rect.maxy,
          rect.minx, rect.maxy,
          rect.minx, rect.miny,
          layerinfo->user_srid
      );
  }

  /* substitute token '!BOX!' in geom_table with the box3d - do an unlimited # of subs */
  /* to not undo the work here, we need to make sure that data_source is malloc'd here */

  if (!strstr(geom_table, "!BOX!")) {
      data_source = (char *)msSmallMalloc(strlen(geom_table) + 1);
      strcpy(data_source, geom_table);
  }
  else {
      char* result = NULL;

      while (strstr(geom_table, "!BOX!")) {
          /* need to do a substition */
          char    *start, *end;
          char    *oldresult = result;
          start = strstr(geom_table, "!BOX!");
          end = start + 5;

          result = (char *)msSmallMalloc((start - geom_table) + strlen(box3d) + strlen(end) + 1);

          strlcpy(result, geom_table, start - geom_table + 1);
          strcpy(result + (start - geom_table), box3d);
          strcat(result, end);

          geom_table = result;
          msFree(oldresult);
      }

      /* if we're here, this will be a malloc'd string, so no need to copy it */
      data_source = (char *)geom_table;
  }

  /* start creating the query */

  query = msStringConcatenate(query, "SELECT ");
  if (layerinfo->paging && (layer->maxfeatures >= 0 || layer->startindex > 0))
    paging_query = msStringConcatenate(paging_query, "SELECT ");

  /* adding items to the select list */
  for (t = 0; t < layer->numitems; t++) {
#ifdef USE_ICONV
      query = msStringConcatenate(query, "convert(nvarchar(max), [");     
#else
      query = msStringConcatenate(query, "convert(varchar(max), [");
#endif
      query = msStringConcatenate(query, layer->items[t]);
      query = msStringConcatenate(query, "]) '");
      tmp = msIntToString(t);
      query = msStringConcatenate(query, tmp);
      if (paging_query) {
          paging_query = msStringConcatenate(paging_query, "[");
          paging_query = msStringConcatenate(paging_query, tmp);
          paging_query = msStringConcatenate(paging_query, "], ");
      }
      msFree(tmp);
      query = msStringConcatenate(query, "',");
  }

  /* adding geometry column */
  query = msStringConcatenate(query, "[");
  query = msStringConcatenate(query, layerinfo->geom_column);
  if (layerinfo->geometry_format == MSSQLGEOMETRY_NATIVE)
      query = msStringConcatenate(query, "] 'geom',");
  else {
      query = msStringConcatenate(query, "].STAsBinary() 'geom',");
  }

  /* adding id column */
  query = msStringConcatenate(query, "convert(varchar(36), [");
  query = msStringConcatenate(query, layerinfo->urid_name);
  if (paging_query) {
      paging_query = msStringConcatenate(paging_query, "[geom], [id] FROM (");
      query = msStringConcatenate(query, "]) 'id', row_number() over (");
      if (layerinfo->sort_spec) {
          query = msStringConcatenate(query, layerinfo->sort_spec);
      }

      if (layer->sortBy.nProperties > 0) {
          tmp = msLayerBuildSQLOrderBy(layer);
          if (layerinfo->sort_spec) {
              query = msStringConcatenate(query, ", ");
          }
          else {
              query = msStringConcatenate(query, " ORDER BY ");
          }
          query = msStringConcatenate(query, tmp);
          msFree(tmp);
      }
      else {
          if (!layerinfo->sort_spec) {
              // use the unique Id as the default sort for paging
              query = msStringConcatenate(query, "ORDER BY ");
              query = msStringConcatenate(query, layerinfo->urid_name);
          }
      }
      query = msStringConcatenate(query, ") 'rownum' FROM ");
  }
  else {
      query = msStringConcatenate(query, "]) 'id' FROM ");
  }

  /* adding the source */
  query = msStringConcatenate(query, data_source);
  msFree(data_source);
  msFree(f_table_name);

  /* use the index hint if provided */
  if (layerinfo->index_name) {
      query = msStringConcatenate(query, " WITH (INDEX(");
      query = msStringConcatenate(query, layerinfo->index_name);
      query = msStringConcatenate(query, "))");
  }

  /* adding attribute filter */
  hasFilter = addFilter(layer, &query);

  if( bIsValidRect ) {
    /* adding spatial filter */
    if (hasFilter == MS_FALSE)
        query = msStringConcatenate(query, " WHERE ");
    else
        query = msStringConcatenate(query, " AND ");

    query = msStringConcatenate(query, layerinfo->geom_column);
    query = msStringConcatenate(query, ".STIntersects(");
    query = msStringConcatenate(query, box3d);
    query = msStringConcatenate(query, ") = 1 ");
  }

  if (paging_query) {
      paging_query = msStringConcatenate(paging_query, query);
      paging_query = msStringConcatenate(paging_query, ") tbl where [rownum] ");
      if (layer->startindex > 0) {
          tmp = msIntToString(layer->startindex);
          paging_query = msStringConcatenate(paging_query, ">= ");
          paging_query = msStringConcatenate(paging_query, tmp);
          if (layer->maxfeatures >= 0) {
              msFree(tmp);
              tmp = msIntToString(layer->startindex + layer->maxfeatures);
              paging_query = msStringConcatenate(paging_query, " and [rownum] < ");
              paging_query = msStringConcatenate(paging_query, tmp);
          }
      }
      else {
          tmp = msIntToString(layer->maxfeatures);
          paging_query = msStringConcatenate(paging_query, "< ");
          paging_query = msStringConcatenate(paging_query, tmp);
      }
      msFree(tmp);
      msFree(query);
      query = paging_query;
  } 
  else {
      if (layerinfo->sort_spec) {
          query = msStringConcatenate(query, layerinfo->sort_spec);
      }

      /* Add extra sort by */
      if (layer->sortBy.nProperties > 0) {
          char* pszTmp = msLayerBuildSQLOrderBy(layer);
          if (layerinfo->sort_spec)
              query = msStringConcatenate(query, ", ");
          else
              query = msStringConcatenate(query, " ORDER BY ");
          query = msStringConcatenate(query, pszTmp);
          msFree(pszTmp);
      }
  }


  if (layer->debug) {
      msDebug("query:%s\n", query);
  }

  if (executeSQL(layerinfo->conn, query)) {
      char pass_field_def = 0;
      int t;
      const char *value;
      *query_string = query;
      /* collect result information */
      if ((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
          && strcasecmp(value, "auto") == 0)
          pass_field_def = 1;

      msFree(layerinfo->itemtypes);
      layerinfo->itemtypes = msSmallMalloc(sizeof(SQLSMALLINT) * (layer->numitems + 1));
      for (t = 0; t < layer->numitems; t++) {
          char colBuff[256];
          SQLSMALLINT itemType = 0;

          columnName(layerinfo->conn, t + 1, colBuff, sizeof(colBuff), layer, pass_field_def, &itemType);
          layerinfo->itemtypes[t] = itemType;
      }

      return MS_SUCCESS;
  }
  else {
      msSetError(MS_QUERYERR, "Error executing MSSQL2008 SQL statement: %s\n-%s\n", "msMSSQL2008LayerGetShape()", query, layerinfo->conn->errorMessage);

      msFree(query);

      return MS_FAILURE;
  }
}

/* Execute SQL query for this layer */
int msMSSQL2008LayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
  (void)isQuery;
  msMSSQL2008LayerInfo  *layerinfo = 0;
  char    *query_str = 0;
  int     set_up_result;

  if(layer->debug) {
    msDebug("msMSSQL2008LayerWhichShapes called\n");
  }

  layerinfo = getMSSQL2008LayerInfo(layer);

  if(!layerinfo) {
    /* layer not opened yet */
    msSetError(MS_QUERYERR, "msMSSQL2008LayerWhichShapes called on unopened layer (layerinfo = NULL)", "msMSSQL2008LayerWhichShapes()");

    return MS_FAILURE;
  }

  if(!layer->data) {
    msSetError(MS_QUERYERR, "Missing DATA clause in MSSQL2008 Layer definition.  DATA statement must contain 'geometry_column from table_name' or 'geometry_column from (sub-query) as foo'.", "msMSSQL2008LayerWhichShapes()");

    return MS_FAILURE;
  }

  set_up_result = prepare_database(layer, rect, &query_str);

  if(set_up_result != MS_SUCCESS) {
    msFree(query_str);

    return set_up_result; /* relay error */
  }

  msFree(layerinfo->sql);
  layerinfo->sql = query_str;
  layerinfo->row_num = 0;

  return MS_SUCCESS;
}

/* Close the MSSQL2008 record set and connection */
int msMSSQL2008LayerClose(layerObj *layer)
{
  msMSSQL2008LayerInfo  *layerinfo;

  layerinfo = getMSSQL2008LayerInfo(layer);

  if(layer->debug) {
    char *data = "";

    if (layer->data) {
      data = layer->data;
    }

    msDebug("msMSSQL2008LayerClose datastatement: %s\n", data);
  }

  if(layer->debug && !layerinfo) {
    msDebug("msMSSQL2008LayerClose -- layerinfo is  NULL\n");
  }

  if(layerinfo) {
    msConnPoolRelease(layer, layerinfo->conn);

    layerinfo->conn = NULL;

    if(layerinfo->user_srid) {
      msFree(layerinfo->user_srid);
      layerinfo->user_srid = NULL;
    }

    if(layerinfo->urid_name) {
      msFree(layerinfo->urid_name);
      layerinfo->urid_name = NULL;
    }

    if(layerinfo->index_name) {
      msFree(layerinfo->index_name);
      layerinfo->index_name = NULL;
    }

    if(layerinfo->sort_spec) {
      msFree(layerinfo->sort_spec);
      layerinfo->sort_spec = NULL;
    }

    if(layerinfo->sql) {
      msFree(layerinfo->sql);
      layerinfo->sql = NULL;
    }

    if(layerinfo->geom_column) {
      msFree(layerinfo->geom_column);
      layerinfo->geom_column = NULL;
    }

    if(layerinfo->geom_column_type) {
      msFree(layerinfo->geom_column_type);
      layerinfo->geom_column_type = NULL;
    }

    if(layerinfo->geom_table) {
      msFree(layerinfo->geom_table);
      layerinfo->geom_table = NULL;
    }

    if(layerinfo->itemtypes) {
      msFree(layerinfo->itemtypes);
      layerinfo->itemtypes = NULL;
    }

    setMSSQL2008LayerInfo(layer, NULL);
    msFree(layerinfo);
  }

  return MS_SUCCESS;
}

/* ******************************************************* */
/* wkb is assumed to be 2d (force_2d) */
/* and wkb is a GEOMETRYCOLLECTION (force_collection) */
/* and wkb is in the endian of this computer (asbinary(...,'[XN]DR')) */
/* each of the sub-geom inside the collection are point,linestring, or polygon */
/*  */
/* also, int is 32bits long */
/* double is 64bits long */
/* ******************************************************* */


/* convert the wkb into points */
/* points -> pass through */
/* lines->   constituent points */
/* polys->   treat ring like line and pull out the consituent points */

static int  force_to_points(char *wkb, shapeObj *shape)
{
  /* we're going to make a 'line' for each entity (point, line or ring) in the geom collection */

  int     offset = 0;
  int     ngeoms;
  int     t, u, v;
  int     type, nrings, npoints;
  lineObj line = {0, NULL};

  shape->type = MS_SHAPE_NULL;  /* nothing in it */

  memcpy(&ngeoms, &wkb[5], 4);
  offset = 9;  /* were the first geometry is */
  for(t=0; t < ngeoms; t++) {
    memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

    if(type == 1) {
      /* Point */
      shape->type = MS_SHAPE_POINT;
      line.numpoints = 1;
      line.point = (pointObj *) msSmallMalloc(sizeof(pointObj));

      memcpy(&line.point[0].x, &wkb[offset + 5], 8);
      memcpy(&line.point[0].y, &wkb[offset + 5 + 8], 8);
      offset += 5 + 16;
      msAddLine(shape, &line);
      msFree(line.point);
    } else if(type == 2) {
      /* Linestring */
      shape->type = MS_SHAPE_POINT;

      memcpy(&line.numpoints, &wkb[offset+5], 4); /* num points */
      line.point = (pointObj *) msSmallMalloc(sizeof(pointObj) * line.numpoints);
      for(u = 0; u < line.numpoints; u++) {
        memcpy( &line.point[u].x, &wkb[offset+9 + (16 * u)], 8);
        memcpy( &line.point[u].y, &wkb[offset+9 + (16 * u)+8], 8);
      }
      offset += 9 + 16 * line.numpoints;  /* length of object */
      msAddLine(shape, &line);
      msFree(line.point);
    } else if(type == 3) {
      /* Polygon */
      shape->type = MS_SHAPE_POINT;

      memcpy(&nrings, &wkb[offset+5],4); /* num rings */
      /* add a line for each polygon ring */
      offset += 9; /* now points at 1st linear ring */
      for(u = 0; u < nrings; u++) {
        /* for each ring, make a line */
        memcpy(&npoints, &wkb[offset], 4); /* num points */
        line.numpoints = npoints;
        line.point = (pointObj *) msSmallMalloc(sizeof(pointObj)* npoints);
        /* point struct */
        for(v = 0; v < npoints; v++) {
          memcpy(&line.point[v].x, &wkb[offset + 4 + (16 * v)], 8);
          memcpy(&line.point[v].y, &wkb[offset + 4 + (16 * v) + 8], 8);
        }
        /* make offset point to next linear ring */
        msAddLine(shape, &line);
        msFree(line.point);
        offset += 4 + 16 *npoints;
      }
    }

  }

  return MS_SUCCESS;
}

/* convert the wkb into lines */
/* points-> remove */
/* lines -> pass through */
/* polys -> treat rings as lines */

static int  force_to_lines(char *wkb, shapeObj *shape)
{
  int     offset = 0;
  int     ngeoms;
  int     t, u, v;
  int     type, nrings, npoints;
  lineObj line = {0, NULL};

  shape->type = MS_SHAPE_NULL;  /* nothing in it */

  memcpy(&ngeoms, &wkb[5], 4);
  offset = 9;  /* were the first geometry is */
  for(t=0; t < ngeoms; t++) {
    memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

    /* cannot do anything with a point */

    if(type == 2) {
      /* Linestring */
      shape->type = MS_SHAPE_LINE;

      memcpy(&line.numpoints, &wkb[offset + 5], 4);
      line.point = (pointObj*) msSmallMalloc(sizeof(pointObj) * line.numpoints );
      for(u=0; u < line.numpoints; u++) {
        memcpy(&line.point[u].x, &wkb[offset + 9 + (16 * u)], 8);
        memcpy(&line.point[u].y, &wkb[offset + 9 + (16 * u)+8], 8);
      }
      offset += 9 + 16 * line.numpoints;  /* length of object */
      msAddLine(shape, &line);
      msFree(line.point);
    } else if(type == 3) {
      /* polygon */
      shape->type = MS_SHAPE_LINE;

      memcpy(&nrings, &wkb[offset + 5], 4); /* num rings */
      /* add a line for each polygon ring */
      offset += 9; /* now points at 1st linear ring */
      for(u = 0; u < nrings; u++) {
        /* for each ring, make a line */
        memcpy(&npoints, &wkb[offset], 4);
        line.numpoints = npoints;
        line.point = (pointObj*) msSmallMalloc(sizeof(pointObj) * npoints);
        /* point struct */
        for(v = 0; v < npoints; v++) {
          memcpy(&line.point[v].x, &wkb[offset + 4 + (16 * v)], 8);
          memcpy(&line.point[v].y, &wkb[offset + 4 + (16 * v) + 8], 8);
        }
        /* make offset point to next linear ring */
        msAddLine(shape, &line);
        msFree(line.point);
        offset += 4 + 16 * npoints;
      }
    }
  }

  return MS_SUCCESS;
}

/* point   -> reject */
/* line    -> reject */
/* polygon -> lines of linear rings */
static int  force_to_polygons(char  *wkb, shapeObj *shape)
{
  int     offset = 0;
  int     ngeoms;
  int     t, u, v;
  int     type, nrings, npoints;
  lineObj line = {0, NULL};

  shape->type = MS_SHAPE_NULL;  /* nothing in it */

  memcpy(&ngeoms, &wkb[5], 4);
  offset = 9;  /* were the first geometry is */
  for(t = 0; t < ngeoms; t++) {
    memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

    if(type == 3) {
      /* polygon */
      shape->type = MS_SHAPE_POLYGON;

      memcpy(&nrings, &wkb[offset + 5], 4); /* num rings */
      /* add a line for each polygon ring */
      offset += 9; /* now points at 1st linear ring */
      for(u=0; u < nrings; u++) {
        /* for each ring, make a line */
        memcpy(&npoints, &wkb[offset], 4); /* num points */
        line.numpoints = npoints;
        line.point = (pointObj*) msSmallMalloc(sizeof(pointObj) * npoints);
        for(v=0; v < npoints; v++) {
          memcpy(&line.point[v].x, &wkb[offset + 4 + (16 * v)], 8);
          memcpy(&line.point[v].y, &wkb[offset + 4 + (16 * v) + 8], 8);
        }
        /* make offset point to next linear ring */
        msAddLine(shape, &line);
        msFree(line.point);
        offset += 4 + 16 * npoints;
      }
    }
  }

  return MS_SUCCESS;
}

/* if there is any polygon in wkb, return force_polygon */
/* if there is any line in wkb, return force_line */
/* otherwise return force_point */

static int  dont_force(char *wkb, shapeObj *shape)
{
  int     offset =0;
  int     ngeoms;
  int     type, t;
  int     best_type;

  best_type = MS_SHAPE_NULL;  /* nothing in it */

  memcpy(&ngeoms, &wkb[5], 4);
  offset = 9;  /* were the first geometry is */
  for(t = 0; t < ngeoms; t++) {
    memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

    if(type == 3) {
      best_type = MS_SHAPE_POLYGON;
    } else if(type ==2 && best_type != MS_SHAPE_POLYGON) {
      best_type = MS_SHAPE_LINE;
    } else if(type == 1 && best_type == MS_SHAPE_NULL) {
      best_type = MS_SHAPE_POINT;
    }
  }

  if(best_type == MS_SHAPE_POINT) {
    return force_to_points(wkb, shape);
  }
  if(best_type == MS_SHAPE_LINE) {
    return force_to_lines(wkb, shape);
  }
  if(best_type == MS_SHAPE_POLYGON) {
    return force_to_polygons(wkb, shape);
  }

  return MS_FAILURE; /* unknown type */
}

#if 0
/* ******************************************************* */
/* wkb assumed to be same endian as this machine.  */
/* Should be in little endian (default if created by Microsoft platforms) */
/* ******************************************************* */
/* convert the wkb into points */
/* points -> pass through */
/* lines->   constituent points */
/* polys->   treat ring like line and pull out the consituent points */
static int  force_to_shapeType(char *wkb, shapeObj *shape, int msShapeType)
{
  int     offset = 0;
  int     ngeoms = 1;
  int     u, v;
  int     type, nrings, npoints;
  lineObj line = {0, NULL};

  shape->type = MS_SHAPE_NULL;  /* nothing in it */

  do {
    ngeoms--;

    memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

    if (type == 1) {
      /* Point */
      shape->type = msShapeType;
      line.numpoints = 1;
      line.point = (pointObj *) msSmallMalloc(sizeof(pointObj));

      memcpy(&line.point[0].x, &wkb[offset + 5], 8);
      memcpy(&line.point[0].y, &wkb[offset + 5 + 8], 8);
      offset += 5 + 16;

      if (msShapeType == MS_SHAPE_POINT) {
        msAddLine(shape, &line);
      }

      msFree(line.point);
    } else if(type == 2) {
      /* Linestring */
      shape->type = msShapeType;

      memcpy(&line.numpoints, &wkb[offset+5], 4); /* num points */
      line.point = (pointObj *) msSmallMalloc(sizeof(pointObj) * line.numpoints);
      for(u = 0; u < line.numpoints; u++) {
        memcpy( &line.point[u].x, &wkb[offset+9 + (16 * u)], 8);
        memcpy( &line.point[u].y, &wkb[offset+9 + (16 * u)+8], 8);
      }
      offset += 9 + 16 * line.numpoints;  /* length of object */

      if ((msShapeType == MS_SHAPE_POINT) || (msShapeType == MS_SHAPE_LINE)) {
        msAddLine(shape, &line);
      }

      msFree(line.point);
    } else if(type == 3) {
      /* Polygon */
      shape->type = msShapeType;

      memcpy(&nrings, &wkb[offset+5],4); /* num rings */
      /* add a line for each polygon ring */
      offset += 9; /* now points at 1st linear ring */
      for(u = 0; u < nrings; u++) {
        /* for each ring, make a line */
        memcpy(&npoints, &wkb[offset], 4); /* num points */
        line.numpoints = npoints;
        line.point = (pointObj *) msSmallMalloc(sizeof(pointObj)* npoints);
        /* point struct */
        for(v = 0; v < npoints; v++) {
          memcpy(&line.point[v].x, &wkb[offset + 4 + (16 * v)], 8);
          memcpy(&line.point[v].y, &wkb[offset + 4 + (16 * v) + 8], 8);
        }
        /* make offset point to next linear ring */
        msAddLine(shape, &line);
        msFree(line.point);
        offset += 4 + 16 *npoints;
      }
    } else if(type >= 4 && type <= 7) {
      int cnt = 0;

      offset += 5;

      memcpy(&cnt, &wkb[offset], 4);
      offset += 4;  /* were the first geometry is */

      ngeoms += cnt;
    }
  } while (ngeoms > 0);

  return MS_SUCCESS;
}
#endif

///* if there is any polygon in wkb, return force_polygon */
///* if there is any line in wkb, return force_line */
///* otherwise return force_point */
//static int  dont_force(char *wkb, shapeObj *shape)
//{
//    int     offset =0;
//    int     ngeoms = 1;
//    int     type;
//    int     best_type;
//    int     u;
//    int     nrings, npoints;
//
//    best_type = MS_SHAPE_NULL;  /* nothing in it */
//
//    do
//    {
//        ngeoms--;
//
//        memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */
//
//        if(type == 3) {
//            best_type = MS_SHAPE_POLYGON;
//        } else if(type ==2 && best_type != MS_SHAPE_POLYGON) {
//            best_type = MS_SHAPE_LINE;
//        } else if(type == 1 && best_type == MS_SHAPE_NULL) {
//            best_type = MS_SHAPE_POINT;
//        }
//
//        if (type == 1)
//        {
//            /* Point */
//            offset += 5 + 16;
//        }
//        else if(type == 2)
//        {
//            int numPoints;
//
//            memcpy(&numPoints, &wkb[offset+5], 4); /* num points */
//            /* Linestring */
//            offset += 9 + 16 * numPoints;  /* length of object */
//        }
//        else if(type == 3)
//        {
//            /* Polygon */
//            memcpy(&nrings, &wkb[offset+5],4); /* num rings */
//            offset += 9; /* now points at 1st linear ring */
//            for(u = 0; u < nrings; u++) {
//                /* for each ring, make a line */
//                memcpy(&npoints, &wkb[offset], 4); /* num points */
//                offset += 4 + 16 *npoints;
//            }
//        }
//        else if(type >= 4 && type <= 7)
//        {
//            int cnt = 0;
//
//            offset += 5;
//
//            memcpy(&cnt, &wkb[offset], 4);
//            offset += 4;  /* were the first geometry is */
//
//            ngeoms += cnt;
//        }
//    }
//    while (ngeoms > 0);
//
//    return force_to_shapeType(wkb, shape, best_type);
//}
//
/* find the bounds of the shape */
static void find_bounds(shapeObj *shape)
{
  int     t, u;
  int     first_one = 1;

  for(t = 0; t < shape->numlines; t++) {
    for(u = 0; u < shape->line[t].numpoints; u++) {
      if(first_one) {
        shape->bounds.minx = shape->line[t].point[u].x;
        shape->bounds.maxx = shape->line[t].point[u].x;

        shape->bounds.miny = shape->line[t].point[u].y;
        shape->bounds.maxy = shape->line[t].point[u].y;
        first_one = 0;
      } else {
        if(shape->line[t].point[u].x < shape->bounds.minx) {
          shape->bounds.minx = shape->line[t].point[u].x;
        }
        if(shape->line[t].point[u].x > shape->bounds.maxx) {
          shape->bounds.maxx = shape->line[t].point[u].x;
        }

        if(shape->line[t].point[u].y < shape->bounds.miny) {
          shape->bounds.miny = shape->line[t].point[u].y;
        }
        if(shape->line[t].point[u].y > shape->bounds.maxy) {
          shape->bounds.maxy = shape->line[t].point[u].y;
        }
      }
    }
  }
}

/* Used by NextShape() to access a shape in the query set */
int msMSSQL2008LayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
  msMSSQL2008LayerInfo  *layerinfo;
  SQLLEN needLen = 0;
  SQLLEN retLen = 0;
  char dummyBuffer[1];
  char *wkbBuffer;
  char *valueBuffer;
  char oidBuffer[ 16 ];   /* assuming the OID will always be a long this should be enough */
  long record_oid;
  int t;

  /* for coercing single types into geometry collections */
  char *wkbTemp;
  int geomType;

  layerinfo = getMSSQL2008LayerInfo(layer);

  if(!layerinfo) {
    msSetError(MS_QUERYERR, "GetShape called with layerinfo = NULL", "msMSSQL2008LayerGetShape()");
    return MS_FAILURE;
  }

  if(!layerinfo->conn) {
    msSetError(MS_QUERYERR, "NextShape called on MSSQL2008 layer with no connection to DB.", "msMSSQL2008LayerGetShape()");
    return MS_FAILURE;
  }

  shape->type = MS_SHAPE_NULL;

  while(shape->type == MS_SHAPE_NULL) {
    /* SQLRETURN rc = SQLFetchScroll(layerinfo->conn->hstmt, SQL_FETCH_ABSOLUTE, (SQLLEN) (*record) + 1); */

    /* We only do forward fetches. the parameter 'record' is ignored, but is incremented */
    SQLRETURN rc = SQLFetch(layerinfo->conn->hstmt);

    /* Any error assume out of recordset bounds */
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
      handleSQLError(layer);
      return MS_DONE;
    }

    /* retreive an item */

    {
      /* have to retrieve shape attributes */
      shape->values = (char **) msSmallMalloc(sizeof(char *) * layer->numitems);
      shape->numvalues = layer->numitems;

      for(t=0; t < layer->numitems; t++) {
        /* Startwith a 64 character long buffer. This may need to be increased after calling SQLGetData. */
        SQLLEN emptyLen = 64;
        valueBuffer = (char*) msSmallMalloc(emptyLen);
        if ( valueBuffer == NULL ) {
          msSetError( MS_QUERYERR, "Could not allocate value buffer.", "msMSSQL2008LayerGetShapeRandom()" );
          return MS_FAILURE;
        }

#ifdef USE_ICONV
        SQLSMALLINT targetType = SQL_WCHAR;
#else
        SQLSMALLINT targetType = SQL_CHAR;
#endif
        SQLLEN totalLen = 0;
        char *bufferLocation = valueBuffer;
        int r = 0;
        while (r < 20) {
          rc = SQLGetData(layerinfo->conn->hstmt, (SQLUSMALLINT)(t + 1), targetType, bufferLocation, emptyLen, &retLen);

          if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
            totalLen += retLen > emptyLen || retLen == SQL_NO_TOTAL ? emptyLen : retLen;

          if (rc == SQL_SUCCESS_WITH_INFO && rc != SQL_NO_DATA) {
            /* We must compensate for the last null termination that SQLGetData include */
            /* If we get SQL_NO_TOTAL we do not know how big buffer we need so we increase it with 512. */
#ifdef USE_ICONV
            totalLen -= sizeof(wchar_t);
            emptyLen = retLen != SQL_NO_TOTAL? retLen - emptyLen + 2 * sizeof(wchar_t): 512;
#else
            totalLen -= sizeof(char);
            emptyLen = retLen != SQL_NO_TOTAL? retLen - emptyLen + 2 * sizeof(char): 512;
#endif

            valueBuffer = (char *)msSmallRealloc(valueBuffer, totalLen + emptyLen);
            bufferLocation = valueBuffer + totalLen;
          } else
            break;
          
          r++;
        }

        if (rc == SQL_ERROR)
          handleSQLError(layer);

        if (totalLen > 0) {
          /* Pop the value into the shape's value array */
#ifdef USE_ICONV
          shape->values[t] = msConvertWideStringToUTF8((wchar_t*)valueBuffer, "UCS-2LE");
          msFree(valueBuffer);
#else
          shape->values[t] = valueBuffer;
#endif
        } else {
          /* Copy empty sting for NULL values */
          shape->values[t] = msStrdup("");
          msFree(valueBuffer);
        }
      }

      /* Get shape geometry */
      {
        /* Set up to request the size of the buffer needed */
        rc = SQLGetData(layerinfo->conn->hstmt, (SQLUSMALLINT)(layer->numitems + 1), SQL_C_BINARY, dummyBuffer, 0, &needLen);
        if (rc == SQL_ERROR)
          handleSQLError(layer);

        /* allow space for coercion to geometry collection if needed*/
        wkbTemp = (char*)msSmallMalloc(needLen+9);

        /* write data above space allocated for geometry collection coercion */
        wkbBuffer = wkbTemp + 9;

        if ( wkbBuffer == NULL ) {
          msSetError( MS_QUERYERR, "Could not allocate value buffer.", "msMSSQL2008LayerGetShapeRandom()" );
          return MS_FAILURE;
        }

        /* Grab the WKB */
        rc = SQLGetData(layerinfo->conn->hstmt, (SQLUSMALLINT)(layer->numitems + 1), SQL_C_BINARY, wkbBuffer, needLen, &retLen);
        if (rc == SQL_ERROR || rc == SQL_SUCCESS_WITH_INFO)
          handleSQLError(layer);

        if (layerinfo->geometry_format == MSSQLGEOMETRY_NATIVE) {
          layerinfo->gpi.pszData = (unsigned char*)wkbBuffer;
          layerinfo->gpi.nLen = (int)retLen;

          if (!ParseSqlGeometry(layerinfo, shape)) {
            switch(layer->type) {
              case MS_LAYER_POINT:
                shape->type = MS_SHAPE_POINT;
                break;

              case MS_LAYER_LINE:
                shape->type = MS_SHAPE_LINE;
                break;

              case MS_LAYER_POLYGON:
                shape->type = MS_SHAPE_POLYGON;
                break;

              default:
                break;
            }
          }
        } else {
          memcpy(&geomType, wkbBuffer + 1, 4);

          /* is this a single type? */
          if (geomType < 4) {
            /* copy byte order marker (although we don't check it) */
            wkbTemp[0] = wkbBuffer[0];
            wkbBuffer = wkbTemp;

            /* indicate type is geometry collection (although geomType + 3 would also work) */
            wkbBuffer[1] = (char)7;
            wkbBuffer[2] = (char)0;
            wkbBuffer[3] = (char)0;
            wkbBuffer[4] = (char)0;

            /* indicate 1 shape */
            wkbBuffer[5] = (char)1;
            wkbBuffer[6] = (char)0;
            wkbBuffer[7] = (char)0;
            wkbBuffer[8] = (char)0;
          }

          switch(layer->type) {
            case MS_LAYER_POINT:
              /*result =*/ force_to_points(wkbBuffer, shape);
              break;

            case MS_LAYER_LINE:
              /*result =*/ force_to_lines(wkbBuffer, shape);
              break;

            case MS_LAYER_POLYGON:
              /*result =*/ force_to_polygons(wkbBuffer, shape);
              break;

            case MS_LAYER_QUERY:
            case MS_LAYER_CHART:
              /*result =*/ dont_force(wkbBuffer, shape);
              break;

            case MS_LAYER_RASTER:
              msDebug( "Ignoring MS_LAYER_RASTER in mapMSSQL2008.c\n" );
              break;

            case MS_LAYER_CIRCLE:
              msDebug( "Ignoring MS_LAYER_CIRCLE in mapMSSQL2008.c\n" );
              break;

            default:
              msDebug( "Unsupported layer type in msMSSQL2008LayerNextShape()!" );
              break;
          }
          find_bounds(shape);
        }

        //free(wkbBuffer);
        msFree(wkbTemp);
      }

      /* Next get unique id for row - since the OID shouldn't be larger than a long we'll assume billions as a limit */
      rc = SQLGetData(layerinfo->conn->hstmt, (SQLUSMALLINT)(layer->numitems + 2), SQL_C_BINARY, oidBuffer, sizeof(oidBuffer) - 1, &retLen);
      if (rc == SQL_ERROR || rc == SQL_SUCCESS_WITH_INFO)
        handleSQLError(layer);

      if (retLen < (int)sizeof(oidBuffer))
	  {
		oidBuffer[retLen] = 0;
		record_oid = strtol(oidBuffer, NULL, 10);
		shape->index = record_oid;
	  }
	  else
	  {
		/* non integer fid column, use single pass */
		shape->index = -1;
	  }

      shape->resultindex = (*record);

      (*record)++;        /* move to next shape */

      if(shape->type != MS_SHAPE_NULL) {
        return MS_SUCCESS;
      } else {
        msDebug("msMSSQL2008LayerGetShapeRandom bad shape: %ld\n", *record);
      }
      /* if (layer->type == MS_LAYER_POINT) {return MS_DONE;} */
    }
  }

  msFreeShape(shape);

  return MS_FAILURE;
}

/* find the next shape with the appropriate shape type (convert it if necessary) */
/* also, load in the attribute data */
/* MS_DONE => no more data */
int msMSSQL2008LayerNextShape(layerObj *layer, shapeObj *shape)
{
  int     result;

  msMSSQL2008LayerInfo  *layerinfo;

  layerinfo = getMSSQL2008LayerInfo(layer);

  if(!layerinfo) {
    msSetError(MS_QUERYERR, "NextShape called with layerinfo = NULL", "msMSSQL2008LayerNextShape()");
    return MS_FAILURE;
  }

  result = msMSSQL2008LayerGetShapeRandom(layer, shape, &(layerinfo->row_num));
  /* getshaperandom will increment the row_num */
  /* layerinfo->row_num   ++; */

  return result;
}

/* Execute a query on the DB based on the query result. */
int msMSSQL2008LayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
  char    *query_str;
  char    *columns_wanted = 0;

  msMSSQL2008LayerInfo  *layerinfo;
  int                 t;
  char buffer[32000] = "";
  long shapeindex = record->shapeindex;
  long resultindex = record->resultindex;

  if(layer->debug) {
    msDebug("msMSSQL2008LayerGetShape called for shapeindex = %ld\n", shapeindex);
  }

  layerinfo = getMSSQL2008LayerInfo(layer);

  if(!layerinfo) {
    /* Layer not open */
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShape called on unopened layer (layerinfo = NULL)", "msMSSQL2008LayerGetShape()");

    return MS_FAILURE;
  }

  if (resultindex >= 0 && layerinfo->sql) {
    /* trying to provide the result from the current resultset (single-pass query) */
    if( resultindex < layerinfo->row_num) {
      /* re-issue the query */
      if (!executeSQL(layerinfo->conn, layerinfo->sql)) {
        msSetError(MS_QUERYERR, "Error executing MSSQL2008 SQL statement: %s\n-%s\n", "msMSSQL2008LayerGetShape()", layerinfo->sql, layerinfo->conn->errorMessage);

        return MS_FAILURE;
      }
      layerinfo->row_num = 0;
    }
    while( layerinfo->row_num < resultindex ) {
      /* move forward until we reach the desired index */
      if (msMSSQL2008LayerGetShapeRandom(layer, shape, &(layerinfo->row_num)) != MS_SUCCESS)
        return MS_FAILURE;
    }

    return msMSSQL2008LayerGetShapeRandom(layer, shape, &(layerinfo->row_num));
  }

  /* non single-pass case, fetch the record from the database */

  if(layer->numitems == 0) {
    if (layerinfo->geometry_format == MSSQLGEOMETRY_NATIVE)
      snprintf(buffer, sizeof(buffer), "%s, convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);
    else
      snprintf(buffer, sizeof(buffer), "%s.STAsBinary(), convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);
    columns_wanted = msStrdup(buffer);
  } else {
    for(t = 0; t < layer->numitems; t++) {
      snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "convert(varchar(max), %s),", layer->items[t]);
    }

    if (layerinfo->geometry_format == MSSQLGEOMETRY_NATIVE)
      snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%s, convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);
    else
      snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%s.STAsBinary(), convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);

    columns_wanted = msStrdup(buffer);
  }

  /* index_name is ignored here since the hint should be for the spatial index, not the PK index */
  snprintf(buffer, sizeof(buffer), "select %s from %s where %s = %ld", columns_wanted, layerinfo->geom_table, layerinfo->urid_name, shapeindex);

  query_str = msStrdup(buffer);

  if(layer->debug) {
    msDebug("msMSSQL2008LayerGetShape: %s \n", query_str);
  }

  msFree(columns_wanted);

  if (!executeSQL(layerinfo->conn, query_str)) {
    msSetError(MS_QUERYERR, "Error executing MSSQL2008 SQL statement: %s\n-%s\n", "msMSSQL2008LayerGetShape()",
               query_str, layerinfo->conn->errorMessage);

    msFree(query_str);

    return MS_FAILURE;
  }

  /* we don't preserve the query string in this case (cannot be re-used) */
  msFree(query_str);
  layerinfo->row_num = 0;

  return msMSSQL2008LayerGetShapeRandom(layer, shape, &(layerinfo->row_num));
}

/*
** Returns the number of shapes that match the potential filter and extent.
 * rectProjection is the projection in which rect is expressed, or can be NULL if
 * rect should be considered in the layer projection.
 * This should be equivalent to calling msLayerWhichShapes() and counting the
 * number of shapes returned by msLayerNextShape(), honouring layer->maxfeatures
 * limitation if layer->maxfeatures>=0, and honouring layer->startindex if
 * layer->startindex >= 1 and paging is enabled.
 * Returns -1 in case of failure.
 */
int msMSSQL2008LayerGetShapeCount(layerObj *layer, rectObj rect, projectionObj *rectProjection)
{
    msMSSQL2008LayerInfo *layerinfo;
    char *query = 0;
    char result_data[256];
    char box3d[40 + 10 * 22 + 11];
    SQLLEN retLen;
    SQLRETURN rc;
    int hasFilter = MS_FALSE;
    const rectObj rectInvalid = MS_INIT_INVALID_RECT;
    const int bIsValidRect = memcmp(&rect, &rectInvalid, sizeof(rect)) != 0;

    rectObj searchrectInLayerProj = rect;

    if (layer->debug) {
        msDebug("msMSSQL2008LayerGetShapeCount called.\n");
    }

    layerinfo = getMSSQL2008LayerInfo(layer);

    if (!layerinfo) {
        /* Layer not open */
        msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShapeCount called on unopened layer (layerinfo = NULL)", "msMSSQL2008LayerGetShapeCount()");

        return MS_FAILURE;
    }

    // Special processing if the specified projection for the rect is different from the layer projection
    // We want to issue a WHERE that includes
    // ((the_geom && rect_reprojected_in_layer_SRID) AND NOT ST_Disjoint(ST_Transform(the_geom, rect_SRID), rect))
    if (rectProjection != NULL && layer->project &&
        msProjectionsDiffer(&(layer->projection), rectProjection))
    {
        // If we cannot guess the EPSG code of the rectProjection, fallback on slow implementation
        if (rectProjection->numargs < 1 ||
            strncasecmp(rectProjection->args[0], "init=epsg:", (int)strlen("init=epsg:")) != 0)
        {
            if (layer->debug) {
                msDebug("msMSSQL2008LayerGetShapeCount(): cannot find EPSG code of rectProjection. Falling back on client-side feature count.\n");
            }
            return LayerDefaultGetShapeCount(layer, rect, rectProjection);
        }

        // Reproject the passed rect into the layer projection and get
        // the SRID from the rectProjection
        msProjectRect(rectProjection, &(layer->projection), &searchrectInLayerProj); /* project the searchrect to source coords */
    }

    if (searchrectInLayerProj.minx == searchrectInLayerProj.maxx || 
        searchrectInLayerProj.miny == searchrectInLayerProj.maxy) {
        /* create point shape for rectangles with zero area */
        snprintf(box3d, sizeof(box3d), "%s::STGeomFromText('POINT(%.15g %.15g)',%s)", /* %s.STSrid)", */
            layerinfo->geom_column_type, searchrectInLayerProj.minx, searchrectInLayerProj.miny, layerinfo->user_srid);
    }
    else {
         snprintf(box3d, sizeof(box3d), "%s::STGeomFromText('POLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))',%s)", /* %s.STSrid)", */
            layerinfo->geom_column_type,
            searchrectInLayerProj.minx, searchrectInLayerProj.miny,
            searchrectInLayerProj.maxx, searchrectInLayerProj.miny,
            searchrectInLayerProj.maxx, searchrectInLayerProj.maxy,
            searchrectInLayerProj.minx, searchrectInLayerProj.maxy,
            searchrectInLayerProj.minx, searchrectInLayerProj.miny,
            layerinfo->user_srid
        );
    }

    msLayerTranslateFilter(layer, &layer->filter, layer->filteritem);

    /* set up statement */
    query = msStringConcatenate(query, "SELECT count(*) FROM ");
    query = msStringConcatenate(query, layerinfo->geom_table);

    /* adding attribute filter */
    hasFilter = addFilter(layer, &query);

    if( bIsValidRect ) {
        /* adding spatial filter */
        if (hasFilter == MS_FALSE)
            query = msStringConcatenate(query, " WHERE ");
        else
            query = msStringConcatenate(query, " AND ");

        query = msStringConcatenate(query, layerinfo->geom_column);
        query = msStringConcatenate(query, ".STIntersects(");
        query = msStringConcatenate(query, box3d);
        query = msStringConcatenate(query, ") = 1 ");
    }

    if (layer->debug) {
      msDebug("query:%s\n", query);
    }

    if (!executeSQL(layerinfo->conn, query)) {
        msFree(query);
        return -1;
    }

    msFree(query);

    rc = SQLFetch(layerinfo->conn->hstmt);

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        if (layer->debug) {
            msDebug("msMSSQL2008LayerGetShapeCount: No results found.\n");
        }

        return -1;
    }

    rc = SQLGetData(layerinfo->conn->hstmt, 1, SQL_C_CHAR, result_data, sizeof(result_data), &retLen);

    if (rc == SQL_ERROR) {
        msSetError(MS_QUERYERR, "Failed to get feature count", "msMSSQL2008LayerGetShapeCount()");
        return -1;
    }

    result_data[retLen] = 0;

    return atoi(result_data);
}

/* Query the DB for info about the requested table */
int msMSSQL2008LayerGetItems(layerObj *layer)
{
  msMSSQL2008LayerInfo  *layerinfo;
  char                *sql = NULL;
  size_t              sqlSize;
  char                found_geom = 0;
  int                 t, item_num;
  SQLSMALLINT cols = 0;
  const char *value;
  /*
  * Pass the field definitions through to the layer metadata in the
  * "gml_[item]_{type,width,precision}" set of metadata items for
  * defining fields.
  */
  char                pass_field_def = 0;

  if(layer->debug) {
    msDebug("in msMSSQL2008LayerGetItems  (find column names)\n");
  }

  layerinfo = getMSSQL2008LayerInfo(layer);

  if(!layerinfo) {
    /* layer not opened yet */
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems called on unopened layer", "msMSSQL2008LayerGetItems()");

    return MS_FAILURE;
  }

  if(!layerinfo->conn) {
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems called on MSSQL2008 layer with no connection to DB.", "msMSSQL2008LayerGetItems()");

    return MS_FAILURE;
  }

  sqlSize = strlen(layerinfo->geom_table) + 30;
  sql = msSmallMalloc(sizeof(char *) * sqlSize);

  snprintf(sql, sqlSize, "SELECT top 0 * FROM %s", layerinfo->geom_table);

  if (!executeSQL(layerinfo->conn, sql)) {
    msFree(sql);
    return MS_FAILURE;
  }

  msFree(sql);

  SQLNumResultCols (layerinfo->conn->hstmt, &cols);

  layer->numitems = cols - 1; /* dont include the geometry column */

  layer->items = msSmallMalloc(sizeof(char *) * (layer->numitems + 1)); /* +1 in case there is a problem finding goeometry column */
  layerinfo->itemtypes = msSmallMalloc(sizeof(SQLSMALLINT) * (layer->numitems + 1));
  /* it will return an error if there is no geometry column found, */
  /* so this isnt a problem */

  found_geom = 0; /* havent found the geom field */
  item_num = 0;

  /* consider populating the field definitions in metadata */
  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    pass_field_def = 1;

  for(t = 0; t < cols; t++) {
    char colBuff[256];
    SQLSMALLINT itemType = 0;

    columnName(layerinfo->conn, t + 1, colBuff, sizeof(colBuff), layer, pass_field_def, &itemType);

    if(strcmp(colBuff, layerinfo->geom_column) != 0) {
      /* this isnt the geometry column */
      layer->items[item_num] = (char *) msSmallMalloc(strlen(colBuff) + 1);
      strcpy(layer->items[item_num], colBuff);
      layerinfo->itemtypes[item_num] = itemType;
      item_num++;
    } else {
      found_geom = 1;
    }
  }

  if(!found_geom) {
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems: tried to find the geometry column in the results from the database, but couldnt find it.  Is it miss-capitialized? '%s'", "msMSSQL2008LayerGetItems()", layerinfo->geom_column);
    return MS_FAILURE;
  }

  return msMSSQL2008LayerInitItemInfo(layer);
}

/* Get primary key column of table */
int msMSSQL2008LayerRetrievePK(layerObj *layer, char **urid_name, char* table_name, int debug)
{

  char        sql[1024];
  msMSSQL2008LayerInfo *layerinfo = 0;
  SQLRETURN rc;

  snprintf(sql, sizeof(sql),
           "SELECT     convert(varchar(50), sys.columns.name) AS ColumnName, sys.indexes.name "
           "FROM         sys.columns INNER JOIN "
           "                     sys.indexes INNER JOIN "
           "                     sys.tables ON sys.indexes.object_id = sys.tables.object_id INNER JOIN "
           "                     sys.index_columns ON sys.indexes.object_id = sys.index_columns.object_id AND sys.indexes.index_id = sys.index_columns.index_id ON "
           "                     sys.columns.object_id = sys.index_columns.object_id AND sys.columns.column_id = sys.index_columns.column_id "
           "WHERE     (sys.indexes.is_primary_key = 1) AND (sys.tables.name = N'%s') ",
           table_name);

  if (debug) {
    msDebug("msMSSQL2008LayerRetrievePK: query = %s\n", sql);
  }

  layerinfo = (msMSSQL2008LayerInfo *) layer->layerinfo;

  if(layerinfo->conn == NULL) {

    msSetError(MS_QUERYERR, "Layer does not have a MSSQL2008 connection.", "msMSSQL2008LayerRetrievePK()");

    return(MS_FAILURE);
  }

  /* error somewhere above here in this method */

  if(!executeSQL(layerinfo->conn, sql)) {
    char *tmp1;
    char *tmp2 = NULL;

    tmp1 = "Error executing MSSQL2008 statement (msMSSQL2008LayerRetrievePK():";
    tmp2 = (char *)msSmallMalloc(sizeof(char)*(strlen(tmp1) + strlen(sql) + 1));
    strcpy(tmp2, tmp1);
    strcat(tmp2, sql);
    msSetError(MS_QUERYERR, "%s", "msMSSQL2008LayerRetrievePK()", tmp2);
    msFree(tmp2);
    return(MS_FAILURE);
  }

  rc = SQLFetch(layerinfo->conn->hstmt);

  if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    if(debug) {
      msDebug("msMSSQL2008LayerRetrievePK: No results found.\n");
    }

    return MS_FAILURE;
  }

  {
    char buff[100];
    SQLLEN retLen;
    rc = SQLGetData(layerinfo->conn->hstmt, 1, SQL_C_BINARY, buff, sizeof(buff), &retLen);

    rc = SQLFetch(layerinfo->conn->hstmt);

    if(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
      if(debug) {
        msDebug("msMSSQL2008LayerRetrievePK: Multiple primary key columns are not supported in MapServer\n");
      }

      return MS_FAILURE;
    }

    buff[retLen] = 0;

    *urid_name = msStrdup(buff);
  }

  return MS_SUCCESS;
}

/* Function to parse the Mapserver DATA parameter for geometry
 * column name, table name and name of a column to serve as a
 * unique record id
 */
static int msMSSQL2008LayerParseData(layerObj *layer, char **geom_column_name, char **geom_column_type, char **table_name, char **urid_name, char **user_srid, char **index_name, char **sort_spec, int debug)
{
  char    *pos_opt, *pos_scn, *tmp, *pos_srid, *pos_urid, *pos_geomtype, *pos_geomtype2, *pos_indexHint, *data, *pos_order;
  size_t     slength;

  data = msStrdup(layer->data);
  /* replace tabs with spaces */
  msReplaceChar(data, '\t', ' ');

  /* given a string of the from 'geom from ctivalues' or 'geom from () as foo'
   * return geom_column_name as 'geom'
   * and table name as 'ctivalues' or 'geom from () as foo'
   */

  /* First look for the optional ' using unique ID' string */
  pos_urid = strstrIgnoreCase(data, " using unique ");

  if(pos_urid) {
    /* CHANGE - protect the trailing edge for thing like 'using unique ftab_id using srid=33' */
    tmp = strstr(pos_urid + 14, " ");
    if(!tmp) {
      tmp = pos_urid + strlen(pos_urid);
    }
    *urid_name = (char *) msSmallMalloc((tmp - (pos_urid + 14)) + 1);
    strlcpy(*urid_name, pos_urid + 14, (tmp - (pos_urid + 14)) + 1);
  }

  /* Find the srid */
  pos_srid = strstrIgnoreCase(data, " using SRID=");
  if(!pos_srid) {
    *user_srid = (char *) msSmallMalloc(2);
    (*user_srid)[0] = '0';
    (*user_srid)[1] = 0;
  } else {
    slength = strspn(pos_srid + 12, "-0123456789");
    if(!slength) {
      msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "Error parsing MSSQL2008 data variable: You specified 'using SRID=#' but didn't have any numbers!<br><br>\n\nMore Help:<br><br>\n\n", data);

      msFree(data);
      return MS_FAILURE;
    } else {
      *user_srid = (char *) msSmallMalloc(slength + 1);
      strlcpy(*user_srid, pos_srid + 12, slength+1);
    }
  }

  pos_indexHint = strstrIgnoreCase(data, " using index ");
  if(pos_indexHint) {
    /* CHANGE - protect the trailing edge for thing like 'using unique ftab_id using srid=33' */
    tmp = strstr(pos_indexHint + 13, " ");
    if(!tmp) {
      tmp = pos_indexHint + strlen(pos_indexHint);
    }
    *index_name = (char *) msSmallMalloc((tmp - (pos_indexHint + 13)) + 1);
    strlcpy(*index_name, pos_indexHint + 13, tmp - (pos_indexHint + 13)+1);
  }

  /* this is a little hack so the rest of the code works.  If the ' using SRID=' comes before */
  /* the ' using unique ', then make sure pos_opt points to where the ' using SRID' starts! */
  pos_opt = pos_urid;
  if ( !pos_opt || ( pos_srid && pos_srid < pos_opt ) ) pos_opt = pos_srid;
  if ( !pos_opt || ( pos_indexHint && pos_indexHint < pos_opt ) ) pos_opt = pos_indexHint;
  if ( !pos_opt ) pos_opt = data + strlen(data);

  /* Scan for the table or sub-select clause */
  pos_scn = strstrIgnoreCase(data, " from ");
  if(!pos_scn) {
    msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "Error parsing MSSQL2008 data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldn't find ' from ').  More help: <br><br>\n\n", data);

    msFree(data);
    return MS_FAILURE;
  }

  /* Scanning the geometry column type */
  pos_geomtype = data;
  while (pos_geomtype < pos_scn && *pos_geomtype != '(' && *pos_geomtype != 0)
    ++pos_geomtype;

  if(*pos_geomtype == '(') {
    pos_geomtype2 = pos_geomtype;
    while (pos_geomtype2 < pos_scn && *pos_geomtype2 != ')' && *pos_geomtype2 != 0)
      ++pos_geomtype2;
    if (*pos_geomtype2 != ')' || pos_geomtype2 == pos_geomtype) {
      msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "Error parsing MSSQL2008 data variable.  Invalid syntax near geometry column type.", data);
      msFree(data);
      return MS_FAILURE;
    }

    *geom_column_name = (char *) msSmallMalloc((pos_geomtype - data) + 1);
    strlcpy(*geom_column_name, data, pos_geomtype - data + 1);

    *geom_column_type = (char *) msSmallMalloc(pos_geomtype2 - pos_geomtype);
    strlcpy(*geom_column_type, pos_geomtype + 1, pos_geomtype2 - pos_geomtype);
  } else {
    /* Copy the geometry column name */
    *geom_column_name = (char *) msSmallMalloc((pos_scn - data) + 1);
    strlcpy(*geom_column_name, data, pos_scn - data + 1);
    *geom_column_type = msStrdup("geometry");
  }

  /* Copy out the table name or sub-select clause */
  *table_name = (char *) msSmallMalloc((pos_opt - (pos_scn + 6)) + 1);
  strlcpy(*table_name, pos_scn + 6, pos_opt - (pos_scn + 6) + 1);

  if(strlen(*table_name) < 1 || strlen(*geom_column_name) < 1) {
    msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "Error parsing MSSQL2008 data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find a geometry_column or table/subselect).  More help: <br><br>\n\n", data);

    msFree(data);
    return MS_FAILURE;
  }

  if( !pos_urid ) {
    if( msMSSQL2008LayerRetrievePK(layer, urid_name, *table_name, debug) != MS_SUCCESS ) {
      msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "No primary key defined for table, or primary key contains more that one column\n\n",
                 *table_name);

      msFree(data);
      return MS_FAILURE;
    }
  }

  /* Find the order by */
  pos_order = strstrIgnoreCase(pos_opt, " order by ");
  
  if (pos_order) {
    *sort_spec = msStrdup(pos_order);
  }

  if(debug) {
    msDebug("msMSSQL2008LayerParseData: unique column = %s, srid='%s', geom_column_name = %s, table_name=%s\n", *urid_name, *user_srid, *geom_column_name, *table_name);
  }

  msFree(data);
  return MS_SUCCESS;
}

char *msMSSQL2008LayerEscapePropertyName(layerObj *layer, const char* pszString)
{
  (void)layer;
  char* pszEscapedStr=NULL;
  int j = 0;

  if (pszString && strlen(pszString) > 0) {
    size_t nLength = strlen(pszString);

    pszEscapedStr = (char*) msSmallMalloc( 1 + nLength + 1 + 1);
    pszEscapedStr[j++] = '[';

    for (size_t i=0; i<nLength; i++)
      pszEscapedStr[j++] = pszString[i];

    pszEscapedStr[j++] = ']';
    pszEscapedStr[j++] = 0;
  }
  return pszEscapedStr;
}

char *msMSSQL2008LayerEscapeSQLParam(layerObj *layer, const char *pszString)
{
  (void)layer;
  char *pszEscapedStr=NULL;
  if (pszString) {
    int nSrcLen;
    char c;
    int i=0, j=0;
    nSrcLen = (int)strlen(pszString);
    pszEscapedStr = (char*) msSmallMalloc( 2 * nSrcLen + 1);
    for(i = 0, j = 0; i < nSrcLen; i++) {
      c = pszString[i];
      if (c == '\'') {
        pszEscapedStr[j++] = '\'';
        pszEscapedStr[j++] = '\'';
      } else
        pszEscapedStr[j++] = c;
    }
    pszEscapedStr[j] = 0;
  }
  return pszEscapedStr;
}

int msMSSQL2008GetPaging(layerObj *layer)
{
    msMSSQL2008LayerInfo *layerinfo = NULL;

    if (!msMSSQL2008LayerIsOpen(layer))
        return MS_TRUE;

    assert(layer->layerinfo != NULL);
    layerinfo = (msMSSQL2008LayerInfo *)layer->layerinfo;

    return layerinfo->paging;
}

void msMSSQL2008EnablePaging(layerObj *layer, int value)
{
    msMSSQL2008LayerInfo *layerinfo = NULL;

    if (!msMSSQL2008LayerIsOpen(layer))
    {
      if(msMSSQL2008LayerOpen(layer) != MS_SUCCESS)
      {
        return;
      }
    }

    assert(layer->layerinfo != NULL);
    layerinfo = (msMSSQL2008LayerInfo *)layer->layerinfo;

    layerinfo->paging = value;
}

int process_node(layerObj* layer, expressionObj *filter)
{
  char *snippet = NULL;
  char *strtmpl = NULL;
  char *stresc = NULL;
  msMSSQL2008LayerInfo *layerinfo = (msMSSQL2008LayerInfo *) layer->layerinfo;

  if (!layerinfo->current_node)
  {
    msSetError(MS_MISCERR, "Unecpected end of expression", "msMSSQL2008LayerTranslateFilter()");
    return 0;
  }

  switch(layerinfo->current_node->token) {
    case '(':
       filter->native_string = msStringConcatenate(filter->native_string, "(");
       /* process subexpression */
       layerinfo->current_node = layerinfo->current_node->next;
       while (layerinfo->current_node != NULL) { 
         if (!process_node(layer, filter))
           return 0;
         if (!layerinfo->current_node)
           break;
         if (layerinfo->current_node->token == ')')
           break;
         layerinfo->current_node = layerinfo->current_node->next;
       }
       break;
    case ')':
       filter->native_string = msStringConcatenate(filter->native_string, ")");
       break;
    /* argument separator */
    case ',':
       break;
    /* literal tokens */
    case MS_TOKEN_LITERAL_BOOLEAN:
      if(layerinfo->current_node->tokenval.dblval == MS_TRUE)
        filter->native_string = msStringConcatenate(filter->native_string, "1");
      else
        filter->native_string = msStringConcatenate(filter->native_string, "0");
      break;
    case MS_TOKEN_LITERAL_NUMBER:
    {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "%.18g", layerinfo->current_node->tokenval.dblval);
      filter->native_string = msStringConcatenate(filter->native_string, buffer);
      break;
    }
    case MS_TOKEN_LITERAL_STRING:
      strtmpl = "'%s'";
      stresc = msMSSQL2008LayerEscapeSQLParam(layer, layerinfo->current_node->tokenval.strval);
      snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(stresc));
      sprintf(snippet, strtmpl, stresc);
      filter->native_string = msStringConcatenate(filter->native_string, snippet);
      msFree(snippet);
      msFree(stresc);
      break;
    case MS_TOKEN_LITERAL_TIME:
      {
        int resolution = msTimeGetResolution(layerinfo->current_node->tokensrc);
        snippet = (char *) msSmallMalloc(128);
        switch(resolution) {
          case TIME_RESOLUTION_YEAR:
            sprintf(snippet, "'%d'", (layerinfo->current_node->tokenval.tmval.tm_year+1900));
            break;
          case TIME_RESOLUTION_MONTH:
            sprintf(snippet, "'%d-%02d-01'", (layerinfo->current_node->tokenval.tmval.tm_year+1900), 
                    (layerinfo->current_node->tokenval.tmval.tm_mon+1));
            break;
          case TIME_RESOLUTION_DAY:
            sprintf(snippet, "'%d-%02d-%02d'", (layerinfo->current_node->tokenval.tmval.tm_year+1900), 
                    (layerinfo->current_node->tokenval.tmval.tm_mon+1), 
                    layerinfo->current_node->tokenval.tmval.tm_mday);
            break;
          case TIME_RESOLUTION_HOUR:
            sprintf(snippet, "'%d-%02d-%02d %02d:00'", (layerinfo->current_node->tokenval.tmval.tm_year+1900), 
                (layerinfo->current_node->tokenval.tmval.tm_mon+1), 
                layerinfo->current_node->tokenval.tmval.tm_mday, 
                layerinfo->current_node->tokenval.tmval.tm_hour);
            break;
          case TIME_RESOLUTION_MINUTE:
            sprintf(snippet, "%d-%02d-%02d %02d:%02d'", (layerinfo->current_node->tokenval.tmval.tm_year+1900), 
                (layerinfo->current_node->tokenval.tmval.tm_mon+1), 
                layerinfo->current_node->tokenval.tmval.tm_mday, 
                layerinfo->current_node->tokenval.tmval.tm_hour, 
                layerinfo->current_node->tokenval.tmval.tm_min);
            break;
          case TIME_RESOLUTION_SECOND:
            sprintf(snippet, "'%d-%02d-%02d %02d:%02d:%02d'", (layerinfo->current_node->tokenval.tmval.tm_year+1900),
                (layerinfo->current_node->tokenval.tmval.tm_mon+1), 
                layerinfo->current_node->tokenval.tmval.tm_mday, 
                layerinfo->current_node->tokenval.tmval.tm_hour, 
                layerinfo->current_node->tokenval.tmval.tm_min, 
                layerinfo->current_node->tokenval.tmval.tm_sec);
            break;
        }    
        filter->native_string = msStringConcatenate(filter->native_string, snippet);
        msFree(snippet);
      }
      break;
    case MS_TOKEN_LITERAL_SHAPE:
      if (strcasecmp(layerinfo->geom_column_type, "geography") == 0)
        filter->native_string = msStringConcatenate(filter->native_string, "geography::STGeomFromText('");
      else
        filter->native_string = msStringConcatenate(filter->native_string, "geometry::STGeomFromText('");
      filter->native_string = msStringConcatenate(filter->native_string, msShapeToWKT(layerinfo->current_node->tokenval.shpval));
      filter->native_string = msStringConcatenate(filter->native_string, "'");
      if(layerinfo->user_srid && strcmp(layerinfo->user_srid, "") != 0) {
        filter->native_string = msStringConcatenate(filter->native_string, ", ");
        filter->native_string = msStringConcatenate(filter->native_string, layerinfo->user_srid);
      }
      filter->native_string = msStringConcatenate(filter->native_string, ")");
      break;
    case MS_TOKEN_BINDING_TIME:
    case MS_TOKEN_BINDING_DOUBLE:
    case MS_TOKEN_BINDING_INTEGER:
    case MS_TOKEN_BINDING_STRING:
      if(layerinfo->current_node->next->token == MS_TOKEN_COMPARISON_IRE)
        strtmpl = "LOWER(%s)";
      else
        strtmpl = "%s";

      stresc = msMSSQL2008LayerEscapePropertyName(layer, layerinfo->current_node->tokenval.bindval.item);
      snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(stresc));
      sprintf(snippet, strtmpl, stresc);
      filter->native_string = msStringConcatenate(filter->native_string, snippet);
      msFree(snippet);
      msFree(stresc);
      break;
    case MS_TOKEN_BINDING_SHAPE:
      filter->native_string = msStringConcatenate(filter->native_string, layerinfo->geom_column);
      break;
    case MS_TOKEN_BINDING_MAP_CELLSIZE:
    {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "%.18g", layer->map->cellsize);
      filter->native_string = msStringConcatenate(filter->native_string, buffer);
      break;
    }

    /* comparisons */
    case MS_TOKEN_COMPARISON_IN:
      filter->native_string = msStringConcatenate(filter->native_string, " IN ");
      break;
    case MS_TOKEN_COMPARISON_RE: 
    case MS_TOKEN_COMPARISON_IRE: 
    case MS_TOKEN_COMPARISON_LIKE: {
      /* process regexp */
      size_t i = 0, j = 0;
      char c;
      char c_next;
      int bCaseInsensitive = (layerinfo->current_node->token == MS_TOKEN_COMPARISON_IRE);
      int nEscapeLen = 0;
      if (bCaseInsensitive)
        filter->native_string = msStringConcatenate(filter->native_string, " LIKE LOWER(");
      else
        filter->native_string = msStringConcatenate(filter->native_string, " LIKE ");

      layerinfo->current_node = layerinfo->current_node->next;
      if (layerinfo->current_node->token != MS_TOKEN_LITERAL_STRING) return 0;

      strtmpl = msStrdup(layerinfo->current_node->tokenval.strval);
      if (strtmpl[0] == '/') {
        stresc = strtmpl + 1;
        strtmpl[strlen(strtmpl) - 1] = '\0';
      }
      else if (strtmpl[0] == '^')
        stresc = strtmpl + 1;
      else
        stresc = strtmpl;

      while (*stresc) {
        c = stresc[i];
        if (c == '%' || c == '_' || c == '[' || c == ']' || c == '^') {
           nEscapeLen++;
        }
        stresc++;
      }
      snippet = (char *)msSmallMalloc(strlen(strtmpl) + nEscapeLen + 3);
      snippet[j++] = '\'';
      while (i < strlen(strtmpl)) {
        c = strtmpl[i];
        c_next = strtmpl[i+1];

        if (i == 0 && c == '^') {
          i++;
          continue;
        }
        if( c == '$' && c_next == 0 && strtmpl[0] == '^' )
          break;

        if (c == '\\') {
          i++;
          c = c_next;
        }
         
        if (c == '%' || c == '_' || c == '[' || c == ']' || c == '^') {
           snippet[j++] = '\\';
        }
        
        if (c == '.' && c_next == '*') {
          i++;
          c = '%';
        }
        else if (c == '.')
          c = '_';

        
        snippet[j++] = c;
        i++;          
      }
      snippet[j++] = '\'';
      snippet[j] = '\0';

      filter->native_string = msStringConcatenate(filter->native_string, snippet);
      msFree(strtmpl);
      msFree(snippet);

      if (bCaseInsensitive)
        filter->native_string = msStringConcatenate(filter->native_string, ")");

      if (nEscapeLen > 0)
        filter->native_string = msStringConcatenate(filter->native_string, " ESCAPE '\\'");
    }
    break;
    case MS_TOKEN_COMPARISON_EQ:
      filter->native_string = msStringConcatenate(filter->native_string, " = ");
      break;
    case MS_TOKEN_COMPARISON_NE: 
      filter->native_string = msStringConcatenate(filter->native_string, " != ");
      break;
    case MS_TOKEN_COMPARISON_GT: 
      filter->native_string = msStringConcatenate(filter->native_string, " > ");
      break;
    case MS_TOKEN_COMPARISON_GE:
      filter->native_string = msStringConcatenate(filter->native_string, " >= ");
      break;
    case MS_TOKEN_COMPARISON_LT:
      filter->native_string = msStringConcatenate(filter->native_string, " < ");
      break;
    case MS_TOKEN_COMPARISON_LE:
      filter->native_string = msStringConcatenate(filter->native_string, " <= ");
      break;

    /* logical ops */
    case MS_TOKEN_LOGICAL_AND:
      filter->native_string = msStringConcatenate(filter->native_string, " AND ");
      break;
    case MS_TOKEN_LOGICAL_OR:
      filter->native_string = msStringConcatenate(filter->native_string, " OR ");
      break;
    case MS_TOKEN_LOGICAL_NOT:
      filter->native_string = msStringConcatenate(filter->native_string, " NOT ");
      break;

    /* spatial comparison tokens */
    case MS_TOKEN_COMPARISON_INTERSECTS:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STIntersects(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_DISJOINT:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STDisjoint(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_TOUCHES:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STTouches(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_OVERLAPS:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STOverlaps(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_CROSSES:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STCrosses(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_WITHIN:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STWithin(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_CONTAINS:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STContains(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_EQUALS:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STEquals(");
      if (!process_node(layer, filter))
        return 0;
      break;

    case MS_TOKEN_COMPARISON_BEYOND:
      msSetError(MS_MISCERR, "Beyond operator is unsupported.", "msMSSQL2008LayerTranslateFilter()");
      return 0;

    case MS_TOKEN_COMPARISON_DWITHIN:
      msSetError(MS_MISCERR, "DWithin operator is unsupported.", "msMSSQL2008LayerTranslateFilter()");
      return 0;

    /* spatial functions */
    case MS_TOKEN_FUNCTION_AREA:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STArea()");
      break;

    case MS_TOKEN_FUNCTION_BUFFER:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STBuffer(");
      if (!process_node(layer, filter))
        return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ")");
      break;

    case MS_TOKEN_FUNCTION_DIFFERENCE:
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (!process_node(layer, filter))
        return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      if (layerinfo->current_node->next) layerinfo->current_node = layerinfo->current_node->next;
      else return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ".STDifference(");
      if (!process_node(layer, filter))
        return 0;
      filter->native_string = msStringConcatenate(filter->native_string, ")");
      break;

    case MS_TOKEN_FUNCTION_FROMTEXT:
      if (strcasecmp(layerinfo->geom_column_type, "geography") == 0)
        filter->native_string = msStringConcatenate(filter->native_string, "geography::STGeomFromText");
      else
        filter->native_string = msStringConcatenate(filter->native_string, "geometry::STGeomFromText");          
      break;

    case MS_TOKEN_FUNCTION_LENGTH:
      filter->native_string = msStringConcatenate(filter->native_string, "len");
      break;

    case MS_TOKEN_FUNCTION_ROUND:
      filter->native_string = msStringConcatenate(filter->native_string, "round");
      break;
    
    case MS_TOKEN_FUNCTION_TOSTRING:
      break;

    case MS_TOKEN_FUNCTION_COMMIFY:
      break;

    case MS_TOKEN_FUNCTION_SIMPLIFY:
      filter->native_string = msStringConcatenate(filter->native_string, ".Reduce"); 
      break;
    case MS_TOKEN_FUNCTION_SIMPLIFYPT:
      break;
    case MS_TOKEN_FUNCTION_GENERALIZE:
      filter->native_string = msStringConcatenate(filter->native_string, ".Reduce"); 
      break;

    default:
      msSetError(MS_MISCERR, "Translation to native SQL failed.","msMSSQL2008LayerTranslateFilter()");
      if (layer->debug) {
        msDebug("Token not caught, exiting: Token is %i\n", layerinfo->current_node->token);
      }
      return 0;
  }
  return 1;
}

/* Translate filter expression to native msssql filter */
int msMSSQL2008LayerTranslateFilter(layerObj *layer, expressionObj *filter, char *filteritem)
{
  char *snippet = NULL;
  char *strtmpl = NULL;
  char *stresc = NULL;
  msMSSQL2008LayerInfo *layerinfo = (msMSSQL2008LayerInfo *) layer->layerinfo;

  if(!filter->string) return MS_SUCCESS;

  msFree(filter->native_string);
  filter->native_string = NULL;

  if(filter->type == MS_STRING && filter->string && filteritem) { /* item/value pair */
    stresc = msMSSQL2008LayerEscapePropertyName(layer, filteritem);
    if(filter->flags & MS_EXP_INSENSITIVE) {
      filter->native_string = msStringConcatenate(filter->native_string, "upper(");
      filter->native_string = msStringConcatenate(filter->native_string, stresc);
      filter->native_string = msStringConcatenate(filter->native_string, ") = upper(");
    } else {
      filter->native_string = msStringConcatenate(filter->native_string, stresc);
      filter->native_string = msStringConcatenate(filter->native_string, " = ");
    }
    msFree(stresc);

    strtmpl = "'%s'";  /* don't have a type for the righthand literal so assume it's a string and we quote */
    snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(filter->string));
    sprintf(snippet, strtmpl, filter->string);  // TODO: escape filter->string
    filter->native_string = msStringConcatenate(filter->native_string, snippet);
    free(snippet);

    if(filter->flags & MS_EXP_INSENSITIVE) 
        filter->native_string = msStringConcatenate(filter->native_string, ")");
    
  } else if(filter->type == MS_REGEX && filter->string && filteritem) { /* item/regex pair */
    /* NOTE: regex is not supported by MSSQL natively. We should install it in CLR UDF 
       according to https://msdn.microsoft.com/en-us/magazine/cc163473.aspx*/
    filter->native_string = msStringConcatenate(filter->native_string, "dbo.RegexMatch(");
    if(filter->flags & MS_EXP_INSENSITIVE) {
      filter->native_string = msStringConcatenate(filter->native_string, "'(?i)");
    }
    filter->native_string = msStrdup(filteritem);
    filter->native_string = msStringConcatenate(filter->native_string, ", ");
    strtmpl = "'%s'";
    snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(filter->string));
    sprintf(snippet, strtmpl, filter->string); // TODO: escape filter->string
    filter->native_string = msStringConcatenate(filter->native_string, snippet);

    filter->native_string = msStringConcatenate(filter->native_string, "')");
    free(snippet);
  } else if(filter->type == MS_EXPRESSION) {
    
      if(layer->debug >= 2) 
      msDebug("msMSSQL2008LayerTranslateFilter. String: %s.\n", filter->string);

    if(!filter->tokens) {
      if(layer->debug >= 2) 
        msDebug("msMSSQL2008LayerTranslateFilter. There are tokens to process... \n");
      return MS_SUCCESS;
    }

    /* start processing nodes */
    layerinfo->current_node = filter->tokens;
    while (layerinfo->current_node != NULL) {
      if (!process_node(layer, filter))
      {
        msFree(filter->native_string);
        filter->native_string = 0;
        return MS_FAILURE;
      }

      if (!layerinfo->current_node)
        break;

      layerinfo->current_node = layerinfo->current_node->next;
    }   
  }

  return MS_SUCCESS;
}

#else

/* prototypes if MSSQL2008 isnt supposed to be compiled */

int msMSSQL2008LayerOpen(layerObj *layer)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerOpen called but unimplemented!  (mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerOpen()");
  return MS_FAILURE;
}

int msMSSQL2008LayerIsOpen(layerObj *layer)
{
  msSetError(MS_QUERYERR, "msMSSQL2008IsLayerOpen called but unimplemented!  (mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerIsOpen()");
  return MS_FALSE;
}

void msMSSQL2008LayerFreeItemInfo(layerObj *layer)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerFreeItemInfo called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerFreeItemInfo()");
}

int msMSSQL2008LayerInitItemInfo(layerObj *layer)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerInitItemInfo called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerInitItemInfo()");
  return MS_FAILURE;
}

int msMSSQL2008LayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerWhichShapes called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerWhichShapes()");
  return MS_FAILURE;
}

int msMSSQL2008LayerClose(layerObj *layer)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerClose called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerClose()");
  return MS_FAILURE;
}

int msMSSQL2008LayerNextShape(layerObj *layer, shapeObj *shape)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerNextShape called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerNextShape()");
  return MS_FAILURE;
}

int msMSSQL2008LayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShape called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetShape()");
  return MS_FAILURE;
}

int msMSSQL2008LayerGetShapeCount(layerObj *layer, rectObj rect, projectionObj *rectProjection)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShapeCount called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetShapeCount()");
    return MS_FAILURE;
}

int msMSSQL2008LayerGetExtent(layerObj *layer, rectObj *extent)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerGetExtent called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetExtent()");
  return MS_FAILURE;
}

int msMSSQL2008LayerGetNumFeatures(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetNumFeatures called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetNumFeatures()");
    return -1;
}

int msMSSQL2008LayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShapeRandom called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetShapeRandom()");
  return MS_FAILURE;
}

int msMSSQL2008LayerGetItems(layerObj *layer)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetItems()");
  return MS_FAILURE;
}

void msMSSQL2008EnablePaging(layerObj *layer, int value)
{
    msSetError(MS_QUERYERR, "msMSSQL2008EnablePaging called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008EnablePaging()");
    return;
}

int msMSSQL2008GetPaging(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008GetPaging called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008GetPaging()");
    return MS_FAILURE;
}

int msMSSQL2008LayerTranslateFilter(layerObj *layer, expressionObj *filter, char *filteritem)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerTranslateFilter called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerTranslateFilter()");
  return MS_FAILURE;
}

char *msMSSQL2008LayerEscapeSQLParam(layerObj *layer, const char *pszString)
{
  msSetError(MS_QUERYERR, "msMSSQL2008EscapeSQLParam called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerEscapeSQLParam()");
  return NULL;
}

char *msMSSQL2008LayerEscapePropertyName(layerObj *layer, const char *pszString)
{
  msSetError(MS_QUERYERR, "msMSSQL2008LayerEscapePropertyName called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerEscapePropertyName()");
  return NULL;
}

/* end above's #ifdef USE_MSSQL2008 */
#endif

#ifdef USE_MSSQL2008_PLUGIN

MS_DLL_EXPORT int PluginInitializeVirtualTable(layerVTableObj* vtable, layerObj *layer)
{
  assert(layer != NULL);
  assert(vtable != NULL);

  vtable->LayerEnablePaging = msMSSQL2008EnablePaging;
  vtable->LayerGetPaging = msMSSQL2008GetPaging;
  vtable->LayerTranslateFilter = msMSSQL2008LayerTranslateFilter;
  vtable->LayerEscapeSQLParam = msMSSQL2008LayerEscapeSQLParam;
  vtable->LayerEscapePropertyName = msMSSQL2008LayerEscapePropertyName;

  vtable->LayerInitItemInfo = msMSSQL2008LayerInitItemInfo;
  vtable->LayerFreeItemInfo = msMSSQL2008LayerFreeItemInfo;
  vtable->LayerOpen = msMSSQL2008LayerOpen;
  vtable->LayerIsOpen = msMSSQL2008LayerIsOpen;
  vtable->LayerWhichShapes = msMSSQL2008LayerWhichShapes;
  vtable->LayerNextShape = msMSSQL2008LayerNextShape;
  vtable->LayerGetShape = msMSSQL2008LayerGetShape;
  vtable->LayerGetShapeCount = msMSSQL2008LayerGetShapeCount;

  vtable->LayerClose = msMSSQL2008LayerClose;

  vtable->LayerGetItems = msMSSQL2008LayerGetItems;
  vtable->LayerGetExtent = msMSSQL2008LayerGetExtent;

  vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;

  /* vtable->LayerGetAutoStyle, not supported for this layer */
  vtable->LayerCloseConnection = msMSSQL2008LayerClose;

  vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  /* vtable->LayerCreateItems, use default */
  vtable->LayerGetNumFeatures = msMSSQL2008LayerGetNumFeatures;
  /* layer->vtable->LayerGetAutoProjection, use defaut*/

  return MS_SUCCESS;
}

#endif

int
msMSSQL2008LayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerEnablePaging = msMSSQL2008EnablePaging;
  layer->vtable->LayerGetPaging = msMSSQL2008GetPaging;
  layer->vtable->LayerTranslateFilter = msMSSQL2008LayerTranslateFilter;
  layer->vtable->LayerEscapeSQLParam = msMSSQL2008LayerEscapeSQLParam;
  layer->vtable->LayerEscapePropertyName = msMSSQL2008LayerEscapePropertyName;

  layer->vtable->LayerInitItemInfo = msMSSQL2008LayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msMSSQL2008LayerFreeItemInfo;
  layer->vtable->LayerOpen = msMSSQL2008LayerOpen;
  layer->vtable->LayerIsOpen = msMSSQL2008LayerIsOpen;
  layer->vtable->LayerWhichShapes = msMSSQL2008LayerWhichShapes;
  layer->vtable->LayerNextShape = msMSSQL2008LayerNextShape;
  layer->vtable->LayerGetShape = msMSSQL2008LayerGetShape;
  layer->vtable->LayerGetShapeCount = msMSSQL2008LayerGetShapeCount;

  layer->vtable->LayerClose = msMSSQL2008LayerClose;

  layer->vtable->LayerGetItems = msMSSQL2008LayerGetItems;
  layer->vtable->LayerGetExtent = msMSSQL2008LayerGetExtent;

  layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;

  /* layer->vtable->LayerGetAutoStyle, not supported for this layer */
  layer->vtable->LayerCloseConnection = msMSSQL2008LayerClose;

  layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  layer->vtable->LayerGetNumFeatures = msMSSQL2008LayerGetNumFeatures;


  return MS_SUCCESS;
}
