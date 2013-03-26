/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  shapeObj to GML output via MapServer queries.
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

#include "mapserver.h"
#include "maperror.h"
#include "mapgml.h"
#include "iconv.h"


/* Use only mapgml.c if WMS or WFS is available (with minor exceptions at end)*/

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)

static int msGMLGeometryLookup(gmlGeometryListObj *geometryList, char *type);

/*
** Functions that write the feature boundary geometry (i.e. a rectObj).
*/

/* GML 2.1.2 */
static int gmlWriteBounds_GML2(FILE *stream, rectObj *rect, const char *srsname, char *tab)
{
  char *srsname_encoded;

  if(!stream) return(MS_FAILURE);
  if(!rect) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  msIO_fprintf(stream, "%s<gml:boundedBy>\n", tab);
  if(srsname) {
    srsname_encoded = msEncodeHTMLEntities(srsname);
    msIO_fprintf(stream, "%s\t<gml:Box srsName=\"%s\">\n", tab, srsname_encoded);
    msFree(srsname_encoded);
  } else
    msIO_fprintf(stream, "%s\t<gml:Box>\n", tab);

  msIO_fprintf(stream, "%s\t\t<gml:coordinates>", tab);
  msIO_fprintf(stream, "%.6f,%.6f %.6f,%.6f", rect->minx, rect->miny, rect->maxx, rect->maxy );
  msIO_fprintf(stream, "</gml:coordinates>\n");
  msIO_fprintf(stream, "%s\t</gml:Box>\n", tab);
  msIO_fprintf(stream, "%s</gml:boundedBy>\n", tab);

  return MS_SUCCESS;
}

/* GML 3.1 (MapServer limits GML encoding to the level 0 profile) */
static int gmlWriteBounds_GML3(FILE *stream, rectObj *rect, const char *srsname, char *tab)
{
  char *srsname_encoded;

  if(!stream) return(MS_FAILURE);
  if(!rect) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  msIO_fprintf(stream, "%s<gml:boundedBy>\n", tab);
  if(srsname) {
    srsname_encoded = msEncodeHTMLEntities(srsname);
    msIO_fprintf(stream, "%s\t<gml:Envelope srsName=\"%s\">\n", tab, srsname_encoded);
    msFree(srsname_encoded);
  } else
    msIO_fprintf(stream, "%s\t<gml:Envelope>\n", tab);

  msIO_fprintf(stream, "%s\t\t<gml:lowerCorner>%.6f %.6f</gml:lowerCorner>\n", tab, rect->minx, rect->miny);
  msIO_fprintf(stream, "%s\t\t<gml:upperCorner>%.6f %.6f</gml:upperCorner>\n", tab, rect->maxx, rect->maxy);

  msIO_fprintf(stream, "%s\t</gml:Envelope>\n", tab);
  msIO_fprintf(stream, "%s</gml:boundedBy>\n", tab);

  return MS_SUCCESS;
}

static void gmlStartGeometryContainer(FILE *stream, char *name, char *namespace, const char *tab)
{
  const char *tag_name=OWS_GML_DEFAULT_GEOMETRY_NAME;

  if(name) tag_name = name;

  if(namespace)
    msIO_fprintf(stream, "%s<%s:%s>\n", tab, namespace, tag_name);
  else
    msIO_fprintf(stream, "%s<%s>\n", tab, tag_name);
}

static void gmlEndGeometryContainer(FILE *stream, char *name, char *namespace, const char *tab)
{
  const char *tag_name=OWS_GML_DEFAULT_GEOMETRY_NAME;

  if(name) tag_name = name;

  if(namespace)
    msIO_fprintf(stream, "%s</%s:%s>\n", tab, namespace, tag_name);
  else
    msIO_fprintf(stream, "%s</%s>\n", tab, tag_name);
}

/* GML 2.1.2 */
static int gmlWriteGeometry_GML2(FILE *stream, gmlGeometryListObj *geometryList, shapeObj *shape, const char *srsname, char *namespace, char *tab)
{
  int i, j, k;
  int *innerlist, *outerlist, numouters;
  char *srsname_encoded = NULL;

  int geometry_aggregate_index, geometry_simple_index;
  char *geometry_aggregate_name = NULL, *geometry_simple_name = NULL;

  if(!stream) return(MS_FAILURE);
  if(!shape) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);
  if(!geometryList) return(MS_FAILURE);

  if(shape->numlines <= 0) return(MS_SUCCESS); /* empty shape, nothing to output */

  if(srsname)
    srsname_encoded = msEncodeHTMLEntities(srsname);

  /* feature geometry */
  switch(shape->type) {
    case(MS_SHAPE_POINT):
      geometry_simple_index = msGMLGeometryLookup(geometryList, "point");
      geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multipoint");
      if(geometry_simple_index >= 0) geometry_simple_name = geometryList->geometries[geometry_simple_index].name;
      if(geometry_aggregate_index >= 0) geometry_aggregate_name = geometryList->geometries[geometry_aggregate_index].name;

      if((geometry_simple_index != -1 && shape->line[0].numpoints == 1 && shape->numlines == 1) ||
          (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
          (geometryList->numgeometries == 0 && shape->line[0].numpoints == 1 && shape->numlines == 1)) { /* write a Point(s) */

        for(i=0; i<shape->numlines; i++) {
          for(j=0; j<shape->line[i].numpoints; j++) {
            gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

            /* Point */
            if(srsname_encoded)
              msIO_fprintf(stream, "%s<gml:Point srsName=\"%s\">\n", tab, srsname_encoded);
            else
              msIO_fprintf(stream, "%s<gml:Point>\n", tab);
            msIO_fprintf(stream, "%s  <gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "%s</gml:Point>\n", tab);

            gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
          }
        }
      } else if((geometry_aggregate_index != -1) || (geometryList->numgeometries == 0)) { /* write a MultiPoint */
        gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace, tab);

        /* MultiPoint */
        if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:MultiPoint srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:MultiPoint>\n", tab);

        for(i=0; i<shape->numlines; i++) {
          for(j=0; j<shape->line[i].numpoints; j++) {
            msIO_fprintf(stream, "%s  <gml:pointMember>\n", tab);
            msIO_fprintf(stream, "%s    <gml:Point>\n", tab);
            msIO_fprintf(stream, "%s      <gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "%s    </gml:Point>\n", tab);
            msIO_fprintf(stream, "%s  </gml:pointMember>\n", tab);
          }
        }

        msIO_fprintf(stream, "%s</gml:MultiPoint>\n", tab);

        gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
      } else {
        msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no point/multipoint geometry defined. -->\n");
      }

      break;
    case(MS_SHAPE_LINE):
      geometry_simple_index = msGMLGeometryLookup(geometryList, "line");
      geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multiline");
      if(geometry_simple_index >= 0) geometry_simple_name = geometryList->geometries[geometry_simple_index].name;
      if(geometry_aggregate_index >= 0) geometry_aggregate_name = geometryList->geometries[geometry_aggregate_index].name;

      if((geometry_simple_index != -1 && shape->numlines == 1) ||
          (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
          (geometryList->numgeometries == 0 && shape->numlines == 1)) { /* write a LineStrings(s) */
        for(i=0; i<shape->numlines; i++) {
          gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

          /* LineString */
          if(srsname_encoded)
            msIO_fprintf(stream, "%s<gml:LineString srsName=\"%s\">\n", tab, srsname_encoded);
          else
            msIO_fprintf(stream, "%s<gml:LineString>\n", tab);

          msIO_fprintf(stream, "%s  <gml:coordinates>", tab);
          for(j=0; j<shape->line[i].numpoints; j++)
            msIO_fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
          msIO_fprintf(stream, "</gml:coordinates>\n");

          msIO_fprintf(stream, "%s</gml:LineString>\n", tab);

          gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
        }
      } else if(geometry_aggregate_index != -1 || (geometryList->numgeometries == 0)) { /* write a MultiCurve */
        gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace, tab);

        /* MultiLineString */
        if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:MultiLineString srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:MultiLineString>\n", tab);

        for(j=0; j<shape->numlines; j++) {
          msIO_fprintf(stream, "%s  <gml:lineStringMember>\n", tab); /* no srsname at this point */
          msIO_fprintf(stream, "%s    <gml:LineString>\n", tab); /* no srsname at this point */

          msIO_fprintf(stream, "%s      <gml:coordinates>", tab);
          for(i=0; i<shape->line[j].numpoints; i++)
            msIO_fprintf(stream, "%f,%f ", shape->line[j].point[i].x, shape->line[j].point[i].y);
          msIO_fprintf(stream, "</gml:coordinates>\n");
          msIO_fprintf(stream, "%s    </gml:LineString>\n", tab);
          msIO_fprintf(stream, "%s  </gml:lineStringMember>\n", tab);
        }

        msIO_fprintf(stream, "%s</gml:MultiLineString>\n", tab);

        gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
      } else {
        msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no line/multiline geometry defined. -->\n");
      }

      break;
    case(MS_SHAPE_POLYGON): /* this gets nasty, since our shapes are so flexible */
      geometry_simple_index = msGMLGeometryLookup(geometryList, "polygon");
      geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multipolygon");
      if(geometry_simple_index >= 0) geometry_simple_name = geometryList->geometries[geometry_simple_index].name;
      if(geometry_aggregate_index >= 0) geometry_aggregate_name = geometryList->geometries[geometry_aggregate_index].name;

      /* get a list of outter rings for this polygon */
      outerlist = msGetOuterList(shape);

      numouters = 0;
      for(i=0; i<shape->numlines; i++)
        if(outerlist[i] == MS_TRUE) numouters++;

      if((geometry_simple_index != -1 && numouters == 1) ||
          (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
          (geometryList->numgeometries == 0 && shape->numlines == 1)) { /* write a Polygon(s) */
        for(i=0; i<shape->numlines; i++) {
          if(outerlist[i] == MS_FALSE) break; /* skip non-outer rings, each outer ring is a new polygon */

          /* get a list of inner rings for this polygon */
          innerlist = msGetInnerList(shape, i, outerlist);

          gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

          /* Polygon */
          if(srsname_encoded)
            msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, srsname_encoded);
          else
            msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

          msIO_fprintf(stream, "%s  <gml:outerBoundaryIs>\n", tab);
          msIO_fprintf(stream, "%s    <gml:LinearRing>\n", tab);

          msIO_fprintf(stream, "%s      <gml:coordinates>", tab);
          for(j=0; j<shape->line[i].numpoints; j++)
            msIO_fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
          msIO_fprintf(stream, "</gml:coordinates>\n");

          msIO_fprintf(stream, "%s    </gml:LinearRing>\n", tab);
          msIO_fprintf(stream, "%s  </gml:outerBoundaryIs>\n", tab);

          for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
            if(innerlist[k] == MS_TRUE) {
              msIO_fprintf(stream, "%s  <gml:innerBoundaryIs>\n", tab);
              msIO_fprintf(stream, "%s    <gml:LinearRing>\n", tab);

              msIO_fprintf(stream, "%s      <gml:coordinates>", tab);
              for(j=0; j<shape->line[k].numpoints; j++)
                msIO_fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
              msIO_fprintf(stream, "</gml:coordinates>\n");

              msIO_fprintf(stream, "%s    </gml:LinearRing>\n", tab);
              msIO_fprintf(stream, "%s  </gml:innerBoundaryIs>\n", tab);
            }
          }

          msIO_fprintf(stream, "%s</gml:Polygon>\n", tab);
          free(innerlist);

          gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
        }
        free(outerlist);
      } else if(geometry_aggregate_index != -1 || (geometryList->numgeometries == 0)) { /* write a MultiPolygon */
        gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace, tab);

        /* MultiPolygon */
        if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:MultiPolygon srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:MultiPolygon>\n", tab);

        for(i=0; i<shape->numlines; i++) { /* step through the outer rings */
          if(outerlist[i] == MS_TRUE) {
            innerlist = msGetInnerList(shape, i, outerlist);

            msIO_fprintf(stream, "%s<gml:polygonMember>\n", tab);
            msIO_fprintf(stream, "%s  <gml:Polygon>\n", tab);

            msIO_fprintf(stream, "%s    <gml:outerBoundaryIs>\n", tab);
            msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s        <gml:coordinates>", tab);
            for(j=0; j<shape->line[i].numpoints; j++)
              msIO_fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "</gml:coordinates>\n");

            msIO_fprintf(stream, "%s      </gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s    </gml:outerBoundaryIs>\n", tab);

            for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
              if(innerlist[k] == MS_TRUE) {
                msIO_fprintf(stream, "%s    <gml:innerBoundaryIs>\n", tab);
                msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

                msIO_fprintf(stream, "%s        <gml:coordinates>", tab);
                for(j=0; j<shape->line[k].numpoints; j++)
                  msIO_fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
                msIO_fprintf(stream, "</gml:coordinates>\n");

                msIO_fprintf(stream, "%s      </gml:LinearRing>\n", tab);
                msIO_fprintf(stream, "%s    </gml:innerBoundaryIs>\n", tab);
              }
            }

            msIO_fprintf(stream, "%s  </gml:Polygon>\n", tab);
            msIO_fprintf(stream, "%s</gml:polygonMember>\n", tab);

            free(innerlist);
          }
        }
        msIO_fprintf(stream, "%s</gml:MultiPolygon>\n", tab);

        free(outerlist);

        gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
      } else {
        msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no polygon/multipolygon geometry defined. -->\n");
      }

      break;
    default:
      break;
  }

  /* clean-up */
  msFree(srsname_encoded);

  return(MS_SUCCESS);
}

/* GML 3.1 (MapServer limits GML encoding to the level 0 profile) */
static int gmlWriteGeometry_GML3(FILE *stream, gmlGeometryListObj *geometryList, shapeObj *shape, const char *srsname, char *namespace, char *tab)
{
  int i, j, k;
  int *innerlist, *outerlist, numouters;
  char *srsname_encoded = NULL;

  int geometry_aggregate_index, geometry_simple_index;
  char *geometry_aggregate_name = NULL, *geometry_simple_name = NULL;

  if(!stream) return(MS_FAILURE);
  if(!shape) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);
  if(!geometryList) return(MS_FAILURE);

  if(shape->numlines <= 0) return(MS_SUCCESS); /* empty shape, nothing to output */

  if(srsname)
    srsname_encoded = msEncodeHTMLEntities(srsname);

  /* feature geometry */
  switch(shape->type) {
    case(MS_SHAPE_POINT):
      geometry_simple_index = msGMLGeometryLookup(geometryList, "point");
      geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multipoint");
      if(geometry_simple_index >= 0) geometry_simple_name = geometryList->geometries[geometry_simple_index].name;
      if(geometry_aggregate_index >= 0) geometry_aggregate_name = geometryList->geometries[geometry_aggregate_index].name;

      if((geometry_simple_index != -1 && shape->line[0].numpoints == 1 && shape->numlines == 1) ||
          (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
          (geometryList->numgeometries == 0 && shape->line[0].numpoints == 1 && shape->numlines == 1)) { /* write a Point(s) */

        for(i=0; i<shape->numlines; i++) {
          for(j=0; j<shape->line[i].numpoints; j++) {
            gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

            /* Point */
            if(srsname_encoded)
              msIO_fprintf(stream, "%s  <gml:Point srsName=\"%s\">\n", tab, srsname_encoded);
            else
              msIO_fprintf(stream, "%s  <gml:Point>\n", tab);
            msIO_fprintf(stream, "%s    <gml:pos>%f %f</gml:pos>\n", tab, shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "%s  </gml:Point>\n", tab);

            gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
          }
        }
      } else if((geometry_aggregate_index != -1) || (geometryList->numgeometries == 0)) { /* write a MultiPoint */
        gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace, tab);

        /* MultiPoint */
        if(srsname_encoded)
          msIO_fprintf(stream, "%s  <gml:MultiPoint srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s  <gml:MultiPoint>\n", tab);

        msIO_fprintf(stream, "%s    <gml:pointMembers>\n", tab);
        for(i=0; i<shape->numlines; i++) {
          for(j=0; j<shape->line[i].numpoints; j++) {
            msIO_fprintf(stream, "%s      <gml:Point>\n", tab);
            msIO_fprintf(stream, "%s        <gml:pos>%f %f</gml:pos>\n", tab, shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "%s      </gml:Point>\n", tab);
          }
        }
        msIO_fprintf(stream, "%s    </gml:pointMembers>\n", tab);

        msIO_fprintf(stream, "%s  </gml:MultiPoint>\n", tab);

        gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
      } else {
        msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no point/multipoint geometry defined. -->\n");
      }

      break;
    case(MS_SHAPE_LINE):
      geometry_simple_index = msGMLGeometryLookup(geometryList, "line");
      geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multiline");
      if(geometry_simple_index >= 0) geometry_simple_name = geometryList->geometries[geometry_simple_index].name;
      if(geometry_aggregate_index >= 0) geometry_aggregate_name = geometryList->geometries[geometry_aggregate_index].name;

      if((geometry_simple_index != -1 && shape->numlines == 1) ||
          (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
          (geometryList->numgeometries == 0 && shape->numlines == 1)) { /* write a LineStrings(s) */
        for(i=0; i<shape->numlines; i++) {
          gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

          /* LineString (should be Curve?) */
          if(srsname_encoded)
            msIO_fprintf(stream, "%s  <gml:LineString srsName=\"%s\">\n", tab, srsname_encoded);
          else
            msIO_fprintf(stream, "%s  <gml:LineString>\n", tab);

          msIO_fprintf(stream, "%s    <gml:posList srsDimension=\"2\">", tab);
          for(j=0; j<shape->line[i].numpoints; j++)
            msIO_fprintf(stream, "%f %f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
          msIO_fprintf(stream, "</gml:posList>\n");

          msIO_fprintf(stream, "%s  </gml:LineString>\n", tab);

          gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
        }
      } else if(geometry_aggregate_index != -1 || (geometryList->numgeometries == 0)) { /* write a MultiCurve */
        gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace, tab);

        /* MultiCurve */
        if(srsname_encoded)
          msIO_fprintf(stream, "%s  <gml:MultiCurve srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s  <gml:MultiCurve>\n", tab);

        msIO_fprintf(stream, "%s    <gml:curveMembers>\n", tab);
        for(i=0; i<shape->numlines; i++) {
          msIO_fprintf(stream, "%s      <gml:LineString>\n", tab); /* no srsname at this point */

          msIO_fprintf(stream, "%s        <gml:posList srsDimension=\"2\">", tab);
          for(j=0; j<shape->line[i].numpoints; j++)
            msIO_fprintf(stream, "%f %f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
          msIO_fprintf(stream, "</gml:posList>\n");
          msIO_fprintf(stream, "%s      </gml:LineString>\n", tab);
        }
        msIO_fprintf(stream, "%s    </gml:curveMembers>\n", tab);

        msIO_fprintf(stream, "%s  </gml:MultiCurve>\n", tab);

        gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
      } else {
        msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no line/multiline geometry defined. -->\n");
      }

      break;
    case(MS_SHAPE_POLYGON): /* this gets nasty, since our shapes are so flexible */
      geometry_simple_index = msGMLGeometryLookup(geometryList, "polygon");
      geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multipolygon");
      if(geometry_simple_index >= 0) geometry_simple_name = geometryList->geometries[geometry_simple_index].name;
      if(geometry_aggregate_index >= 0) geometry_aggregate_name = geometryList->geometries[geometry_aggregate_index].name;

      /* get a list of outter rings for this polygon */
      outerlist = msGetOuterList(shape);

      numouters = 0;
      for(i=0; i<shape->numlines; i++)
        if(outerlist[i] == MS_TRUE) numouters++;

      if((geometry_simple_index != -1 && numouters == 1) ||
          (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
          (geometryList->numgeometries == 0 && shape->numlines == 1)) { /* write a Polygon(s) */
        for(i=0; i<shape->numlines; i++) {
          if(outerlist[i] == MS_FALSE) break; /* skip non-outer rings, each outer ring is a new polygon */

          /* get a list of inner rings for this polygon */
          innerlist = msGetInnerList(shape, i, outerlist);

          gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

          /* Polygon (should be Surface?) */
          if(srsname_encoded)
            msIO_fprintf(stream, "%s  <gml:Polygon srsName=\"%s\">\n", tab, srsname_encoded);
          else
            msIO_fprintf(stream, "%s  <gml:Polygon>\n", tab);

          msIO_fprintf(stream, "%s    <gml:exterior>\n", tab);
          msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

          msIO_fprintf(stream, "%s        <gml:posList srsDimension=\"2\">", tab);
          for(j=0; j<shape->line[i].numpoints; j++)
            msIO_fprintf(stream, "%f %f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
          msIO_fprintf(stream, "</gml:posList>\n");

          msIO_fprintf(stream, "%s      </gml:LinearRing>\n", tab);
          msIO_fprintf(stream, "%s    </gml:exterior>\n", tab);

          for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
            if(innerlist[k] == MS_TRUE) {
              msIO_fprintf(stream, "%s    <gml:interior>\n", tab);
              msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

              msIO_fprintf(stream, "%s        <gml:posList srsDimension=\"2\">", tab);
              for(j=0; j<shape->line[k].numpoints; j++)
                msIO_fprintf(stream, "%f %f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
              msIO_fprintf(stream, "</gml:posList>\n");

              msIO_fprintf(stream, "%s      </gml:LinearRing>\n", tab);
              msIO_fprintf(stream, "%s    </gml:interior>\n", tab);
            }
          }

          msIO_fprintf(stream, "%s  </gml:Polygon>\n", tab);
          free(innerlist);

          gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
        }
        free(outerlist);
      } else if(geometry_aggregate_index != -1 || (geometryList->numgeometries == 0)) { /* write a MultiSurface */
        gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace, tab);

        /* MultiSurface */
        if(srsname_encoded)
          msIO_fprintf(stream, "%s  <gml:MultiSurface srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s  <gml:MultiSurface>\n", tab);

        msIO_fprintf(stream, "%s    <gml:surfaceMembers>\n", tab);
        for(i=0; i<shape->numlines; i++) { /* step through the outer rings */
          if(outerlist[i] == MS_TRUE) {

            /* get a list of inner rings for this polygon */
            innerlist = msGetInnerList(shape, i, outerlist);

            msIO_fprintf(stream, "%s      <gml:Polygon>\n", tab);

            msIO_fprintf(stream, "%s        <gml:exterior>\n", tab);
            msIO_fprintf(stream, "%s          <gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s            <gml:posList srsDimension=\"2\">", tab);
            for(j=0; j<shape->line[i].numpoints; j++)
              msIO_fprintf(stream, "%f %f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "</gml:posList>\n");

            msIO_fprintf(stream, "%s          </gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s        </gml:exterior>\n", tab);

            for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
              if(innerlist[k] == MS_TRUE) {
                msIO_fprintf(stream, "%s        <gml:interior>\n", tab);
                msIO_fprintf(stream, "%s          <gml:LinearRing>\n", tab);

                msIO_fprintf(stream, "%s            <gml:posList srsDimension=\"2\">", tab);
                for(j=0; j<shape->line[k].numpoints; j++)
                  msIO_fprintf(stream, "%f %f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
                msIO_fprintf(stream, "</gml:posList>\n");

                msIO_fprintf(stream, "%s          </gml:LinearRing>\n", tab);
                msIO_fprintf(stream, "%s        </gml:interior>\n", tab);
              }
            }

            msIO_fprintf(stream, "%s      </gml:Polygon>\n", tab);

            free(innerlist);
          }
        }
        msIO_fprintf(stream, "%s    </gml:surfaceMembers>\n", tab);
        msIO_fprintf(stream, "%s  </gml:MultiSurface>\n", tab);

        free(outerlist);

        gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
      } else {
        msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no polygon/multipolygon geometry defined. -->\n");
      }

      break;
    default:
      break;
  }

  /* clean-up */
  msFree(srsname_encoded);

  return(MS_SUCCESS);
}

/*
** Wrappers for the format specific encoding functions.
*/
static int gmlWriteBounds(FILE *stream, int format, rectObj *rect, const char *srsname, char *tab)
{
  switch(format) {
    case(OWS_GML2):
      return gmlWriteBounds_GML2(stream, rect, srsname, tab);
      break;
    case(OWS_GML3):
      return gmlWriteBounds_GML3(stream, rect, srsname, tab);
      break;
    default:
      msSetError(MS_IOERR, "Unsupported GML format.", "gmlWriteBounds()");
  }

  return(MS_FAILURE);
}

static int gmlWriteGeometry(FILE *stream, gmlGeometryListObj *geometryList, int format, shapeObj *shape, const char *srsname, char *namespace, char *tab)
{
  switch(format) {
    case(OWS_GML2):
      return gmlWriteGeometry_GML2(stream, geometryList, shape, srsname, namespace, tab);
      break;
    case(OWS_GML3):
      return gmlWriteGeometry_GML3(stream, geometryList, shape, srsname, namespace, tab);
      break;
    default:
      msSetError(MS_IOERR, "Unsupported GML format.", "gmlWriteGeometry()");
  }

  return(MS_FAILURE);
}

/*
** GML specific metadata handling functions.
*/

int msItemInGroups(char *name, gmlGroupListObj *groupList)
{
  int i, j;
  gmlGroupObj *group;

  if(!groupList) return MS_FALSE; /* no groups */

  for(i=0; i<groupList->numgroups; i++) {
    group = &(groupList->groups[i]);
    for(j=0; j<group->numitems; j++) {
      if(strcasecmp(name, group->items[j]) == 0) return MS_TRUE;
    }
  }

  return MS_FALSE;
}

static int msGMLGeometryLookup(gmlGeometryListObj *geometryList, char *type)
{
  int i;

  if(!geometryList || !type) return -1; /* nothing to look for */

  for(i=0; i<geometryList->numgeometries; i++)
    if(geometryList->geometries[i].type && (strcasecmp(geometryList->geometries[i].type, type) == 0))
      return i;

  return -1; /* not found */
}

gmlGeometryListObj *msGMLGetGeometries(layerObj *layer, const char *metadata_namespaces)
{
  int i;

  const char *value=NULL;
  char tag[64];

  char **names=NULL;
  int numnames=0;

  gmlGeometryListObj *geometryList=NULL;
  gmlGeometryObj *geometry=NULL;

  /* allocate memory and initialize the item collection */
  geometryList = (gmlGeometryListObj *) malloc(sizeof(gmlGeometryListObj));
  MS_CHECK_ALLOC(geometryList, sizeof(gmlGeometryListObj), NULL) ;
  geometryList->geometries = NULL;
  geometryList->numgeometries = 0;

  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "geometries")) != NULL) {
    names = msStringSplit(value, ',', &numnames);

    /* allocation an array of gmlGeometryObj's */
    geometryList->numgeometries = numnames;
    geometryList->geometries = (gmlGeometryObj *) malloc(sizeof(gmlGeometryObj)*geometryList->numgeometries);
    if (geometryList->geometries ==  NULL) {
      msSetError(MS_MEMERR, "Out of memory allocating %u bytes.\n", "msGMLGetGeometries()",
                 sizeof(gmlGeometryObj)*geometryList->numgeometries);
      free(geometryList);
      return NULL;
    }

    for(i=0; i<geometryList->numgeometries; i++) {
      geometry = &(geometryList->geometries[i]);

      geometry->name = msStrdup(names[i]); /* initialize a few things */
      geometry->type = NULL;
      geometry->occurmin = 0;
      geometry->occurmax = 1;

      snprintf(tag, 64, "%s_type", names[i]);
      if((value =  msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
        geometry->type = msStrdup(value); /* TODO: validate input value */

      snprintf(tag, 64, "%s_occurances", names[i]);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) {
        char **occur;
        int numoccur;

        occur = msStringSplit(value, ',', &numoccur);
        if(numoccur == 2) { /* continue (TODO: throw an error if != 2) */
          geometry->occurmin = atof(occur[0]);
          if(strcasecmp(occur[1], "UNBOUNDED") == 0)
            geometry->occurmax = OWS_GML_OCCUR_UNBOUNDED;
          else
            geometry->occurmax = atof(occur[1]);
        }
      }
    }

    msFreeCharArray(names, numnames);
  }

  return geometryList;
}

void msGMLFreeGeometries(gmlGeometryListObj *geometryList)
{
  int i;

  if(!geometryList) return;

  for(i=0; i<geometryList->numgeometries; i++) {
    msFree(geometryList->geometries[i].name);
    msFree(geometryList->geometries[i].type);
  }
  free(geometryList->geometries);

  free(geometryList);
}

static void msGMLWriteItem(FILE *stream, gmlItemObj *item, char *value, const char *namespace, const char *tab)
{
  char *encoded_value, *tag_name;
  int add_namespace = MS_TRUE;

  if(!stream || !item) return;
  if(!item->visible) return;

  if(!namespace) add_namespace = MS_FALSE;

  if(item->encode == MS_TRUE)
    encoded_value = msEncodeHTMLEntities(value);
  else
    encoded_value = msStrdup(value);

  if(!item->template) { /* build the tag from pieces */
    if(item->alias) {
      tag_name = item->alias;
      if(strchr(item->alias, ':') != NULL) add_namespace = MS_FALSE;
    } else {
      tag_name = item->name;
      if(strchr(item->name, ':') != NULL) add_namespace = MS_FALSE;
    }

    if(add_namespace == MS_TRUE && msIsXMLTagValid(tag_name) == MS_FALSE)
      msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid in a XML tag context. -->\n", tag_name);

    if(add_namespace == MS_TRUE)
      msIO_fprintf(stream, "%s<%s:%s>%s</%s:%s>\n", tab, namespace, tag_name, encoded_value, namespace, tag_name);
    else
      msIO_fprintf(stream, "%s<%s>%s</%s>\n", tab, tag_name, encoded_value, tag_name);
  } else {
    char *tag = NULL;

    tag = msStrdup(item->template);
    tag = msReplaceSubstring(tag, "$value", encoded_value);
    if(namespace) tag = msReplaceSubstring(tag, "$namespace", namespace);
    msIO_fprintf(stream, "%s%s\n", tab, tag);
    free(tag);
  }

  free( encoded_value );

  return;
}

gmlNamespaceListObj *msGMLGetNamespaces(webObj *web, const char *metadata_namespaces)
{
  int i;
  const char *value=NULL;
  char tag[64];

  char **prefixes=NULL;
  int numprefixes=0;

  gmlNamespaceListObj *namespaceList=NULL;
  gmlNamespaceObj *namespace=NULL;

  /* allocate the collection */
  namespaceList = (gmlNamespaceListObj *) malloc(sizeof(gmlNamespaceListObj));
  MS_CHECK_ALLOC(namespaceList, sizeof(gmlNamespaceListObj), NULL) ;
  namespaceList->namespaces = NULL;
  namespaceList->numnamespaces = 0;

  /* list of namespaces (TODO: make this automatic by parsing metadata) */
  if((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces, "external_namespace_prefixes")) != NULL) {
    prefixes = msStringSplit(value, ',', &numprefixes);

    /* allocation an array of gmlNamespaceObj's */
    namespaceList->numnamespaces = numprefixes;
    namespaceList->namespaces = (gmlNamespaceObj *) malloc(sizeof(gmlNamespaceObj)*namespaceList->numnamespaces);
    if (namespaceList->namespaces == NULL) {
      msSetError(MS_MEMERR, "Out of memory allocating %u bytes.\n", "msGMLGetNamespaces()",
                 sizeof(gmlNamespaceObj)*namespaceList->numnamespaces);
      free(namespaceList);
      return NULL;
    }

    for(i=0; i<namespaceList->numnamespaces; i++) {
      namespace = &(namespaceList->namespaces[i]);

      namespace->prefix = msStrdup(prefixes[i]); /* initialize a few things */
      namespace->uri = NULL;
      namespace->schemalocation = NULL;

      snprintf(tag, 64, "%s_uri", namespace->prefix);
      if((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces, tag)) != NULL)
        namespace->uri = msStrdup(value);

      snprintf(tag, 64, "%s_schema_location", namespace->prefix);
      if((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces, tag)) != NULL)
        namespace->schemalocation = msStrdup(value);
    }

    msFreeCharArray(prefixes, numprefixes);
  }

  return namespaceList;
}

void msGMLFreeNamespaces(gmlNamespaceListObj *namespaceList)
{
  int i;

  if(!namespaceList) return;

  for(i=0; i<namespaceList->numnamespaces; i++) {
    msFree(namespaceList->namespaces[i].prefix);
    msFree(namespaceList->namespaces[i].uri);
    msFree(namespaceList->namespaces[i].schemalocation);
  }

  free(namespaceList);
}

gmlConstantListObj *msGMLGetConstants(layerObj *layer, const char *metadata_namespaces)
{
  int i;
  const char *value=NULL;
  char tag[64];

  char **names=NULL;
  int numnames=0;

  gmlConstantListObj *constantList=NULL;
  gmlConstantObj *constant=NULL;

  /* allocate the collection */
  constantList = (gmlConstantListObj *) malloc(sizeof(gmlConstantListObj));
  MS_CHECK_ALLOC(constantList, sizeof(gmlConstantListObj), NULL);
  constantList->constants = NULL;
  constantList->numconstants = 0;

  /* list of constants (TODO: make this automatic by parsing metadata) */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "constants")) != NULL) {
    names = msStringSplit(value, ',', &numnames);

    /* allocation an array of gmlConstantObj's */
    constantList->numconstants = numnames;
    constantList->constants = (gmlConstantObj *) malloc(sizeof(gmlConstantObj)*constantList->numconstants);
    if (constantList->constants == NULL) {
      msSetError(MS_MEMERR, "Out of memory allocating %u bytes.\n", "msGMLGetConstants()",
                 sizeof(gmlConstantObj)*constantList->numconstants);
      free(constantList);
      return NULL;
    }


    for(i=0; i<constantList->numconstants; i++) {
      constant = &(constantList->constants[i]);

      constant->name = msStrdup(names[i]); /* initialize a few things */
      constant->value = NULL;
      constant->type = NULL;

      snprintf(tag, 64, "%s_value", constant->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
        constant->value = msStrdup(value);

      snprintf(tag, 64, "%s_type", constant->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
        constant->type = msStrdup(value);
    }

    msFreeCharArray(names, numnames);
  }

  return constantList;
}

void msGMLFreeConstants(gmlConstantListObj *constantList)
{
  int i;

  if(!constantList) return;

  for(i=0; i<constantList->numconstants; i++) {
    msFree(constantList->constants[i].name);
    msFree(constantList->constants[i].value);
    msFree(constantList->constants[i].type);
  }

  free(constantList);
}

static void msGMLWriteConstant(FILE *stream, gmlConstantObj *constant, const char *namespace, const char *tab)
{
  int add_namespace = MS_TRUE;

  if(!stream || !constant) return;
  if(!constant->value) return;

  if(!namespace) add_namespace = MS_FALSE;
  if(strchr(constant->name, ':') != NULL) add_namespace = MS_FALSE;

  if(add_namespace == MS_TRUE && msIsXMLTagValid(constant->name) == MS_FALSE)
    msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid in a XML tag context. -->\n", constant->name);

  if(add_namespace == MS_TRUE)
    msIO_fprintf(stream, "%s<%s:%s>%s</%s:%s>\n", tab, namespace, constant->name, constant->value, namespace, constant->name);
  else
    msIO_fprintf(stream, "%s<%s>%s</%s>\n", tab, constant->name, constant->value, constant->name);

  return;
}

gmlGroupListObj *msGMLGetGroups(layerObj *layer, const char *metadata_namespaces)
{
  int i;
  const char *value=NULL;
  char tag[64];

  char **names=NULL;
  int numnames=0;

  gmlGroupListObj *groupList=NULL;
  gmlGroupObj *group=NULL;

  /* allocate the collection */
  groupList = (gmlGroupListObj *) malloc(sizeof(gmlGroupListObj));
  MS_CHECK_ALLOC(groupList, sizeof(gmlGroupListObj), NULL) ;
  groupList->groups = NULL;
  groupList->numgroups = 0;

  /* list of groups (TODO: make this automatic by parsing metadata) */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "groups")) != NULL) {
    names = msStringSplit(value, ',', &numnames);

    /* allocation an array of gmlGroupObj's */
    groupList->numgroups = numnames;
    groupList->groups = (gmlGroupObj *) malloc(sizeof(gmlGroupObj)*groupList->numgroups);
    if (groupList->groups == NULL) {
      msSetError(MS_MEMERR, "Out of memory allocating %u bytes.\n", "msGMLGetGroups()",
                 sizeof(gmlGroupObj)*groupList->numgroups);
      free(groupList);
      return NULL;
    }

    for(i=0; i<groupList->numgroups; i++) {
      group = &(groupList->groups[i]);

      group->name = msStrdup(names[i]); /* initialize a few things */
      group->items = NULL;
      group->numitems = 0;
      group->type = NULL;

      snprintf(tag, 64, "%s_group", group->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
        group->items = msStringSplit(value, ',', &group->numitems);

      snprintf(tag, 64, "%s_type", group->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
        group->type = msStrdup(value);
    }

    msFreeCharArray(names, numnames);
  }

  return groupList;
}

void msGMLFreeGroups(gmlGroupListObj *groupList)
{
  int i;

  if(!groupList) return;

  for(i=0; i<groupList->numgroups; i++) {
    msFree(groupList->groups[i].name);
    msFreeCharArray(groupList->groups[i].items, groupList->groups[i].numitems);
    msFree(groupList->groups[i].type);
  }

  free(groupList);
}

static void msGMLWriteGroup(FILE *stream, gmlGroupObj *group, shapeObj *shape, gmlItemListObj *itemList, gmlConstantListObj *constantList, const char *namespace, const char *tab)
{
  int i,j;
  int add_namespace = MS_TRUE;
  char *itemtab;

  gmlItemObj *item=NULL;
  gmlConstantObj *constant=NULL;

  if(!stream || !group) return;

  /* setup the item/constant tab */
  itemtab = (char *) msSmallMalloc(sizeof(char)*strlen(tab)+3);

  sprintf(itemtab, "%s  ", tab);

  if(!namespace || strchr(group->name, ':') != NULL) add_namespace = MS_FALSE;

  /* start the group */
  if(add_namespace == MS_TRUE)
    msIO_fprintf(stream, "%s<%s:%s>\n", tab, namespace, group->name);
  else
    msIO_fprintf(stream, "%s<%s>\n", tab, group->name);

  /* now the items/constants in the group */
  for(i=0; i<group->numitems; i++) {
    for(j=0; j<constantList->numconstants; j++) {
      constant = &(constantList->constants[j]);
      if(strcasecmp(constant->name, group->items[i]) == 0) {
        msGMLWriteConstant(stream, constant, namespace, itemtab);
        break;
      }
    }
    if(j != constantList->numconstants) continue; /* found this one */
    for(j=0; j<itemList->numitems; j++) {
      item = &(itemList->items[j]);
      if(strcasecmp(item->name, group->items[i]) == 0) {
        /* the number of items matches the number of values exactly */
        msGMLWriteItem(stream, item, shape->values[j], namespace, itemtab);
        break;
      }
    }
  }

  /* end the group */
  if(add_namespace == MS_TRUE)
    msIO_fprintf(stream, "%s</%s:%s>\n", tab, namespace, group->name);
  else
    msIO_fprintf(stream, "%s</%s>\n", tab, group->name);

  return;
}
#endif

/* Dump GML query results for WMS GetFeatureInfo */
int msGMLWriteQuery(mapObj *map, char *filename, const char *namespaces)
{
#if defined(USE_WMS_SVR)
  int status;
  int i,j,k;
  layerObj *lp=NULL;
  shapeObj shape;
  FILE *stream=stdout; /* defaults to stdout */
  char szPath[MS_MAXPATHLEN];
  char *value;
  const char *pszMapSRS = NULL;

  gmlGroupListObj *groupList=NULL;
  gmlItemListObj *itemList=NULL;
  gmlConstantListObj *constantList=NULL;
  gmlGeometryListObj *geometryList=NULL;
  gmlItemObj *item=NULL;
  gmlConstantObj *constant=NULL;

  msInitShape(&shape);

  if(filename && strlen(filename) > 0) { /* deal with the filename if present */
    stream = fopen(msBuildPath(szPath, map->mappath, filename), "w");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msGMLWriteQuery()", filename);
      return(MS_FAILURE);
    }
  }

  /* charset encoding: lookup "gml_encoding" metadata first, then  */
  /* "wms_encoding", and if not found then use "ISO-8859-1" as default. */
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "encoding", OWS_NOERR, "<?xml version=\"1.0\" encoding=\"%s\"?>\n\n", "ISO-8859-1");
  msOWSPrintValidateMetadata(stream, &(map->web.metadata), namespaces, "rootname", OWS_NOERR, "<%s ", "msGMLOutput");

  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "uri", OWS_NOERR, "xmlns=\"%s\"", NULL);
  msIO_fprintf(stream, "\n\t xmlns:gml=\"http://www.opengis.net/gml\"" );
  msIO_fprintf(stream, "\n\t xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
  msIO_fprintf(stream, "\n\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "schema", OWS_NOERR, "\n\t xsi:schemaLocation=\"%s\"", NULL);
  msIO_fprintf(stream, ">\n");

  /* a schema *should* be required */
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "description", OWS_NOERR, "\t<gml:description>%s</gml:description>\n", NULL);

  /* Look up map SRS. We need an EPSG code for GML, if not then we get null and we'll fall back on the layer's SRS */
  pszMapSRS = msOWSGetEPSGProj(&(map->projection), NULL, namespaces, MS_TRUE);

  /* step through the layers looking for query results */
  for(i=0; i<map->numlayers; i++) {
    const char *pszOutputSRS = NULL;
    lp = (GET_LAYER(map, map->layerorder[i]));

    if(lp->resultcache && lp->resultcache->numresults > 0) { /* found results */

#ifdef USE_PROJ
      /* Determine output SRS, if map has none, then try using layer's native SRS */
      if ((pszOutputSRS = pszMapSRS) == NULL) {
        pszOutputSRS = msOWSGetEPSGProj(&(lp->projection), NULL, namespaces, MS_TRUE);
        if (pszOutputSRS == NULL) {
          msSetError(MS_WMSERR, "No valid EPSG code in map or layer projection for GML output", "msGMLWriteQuery()");
          continue;  /* No EPSG code, cannot output this layer */
        }
      }
#endif

      /* start this collection (layer) */
      /* if no layer name provided fall back on the layer name + "_layer" */
      value = (char*) msSmallMalloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "layername", OWS_NOERR, "\t<%s>\n", value);
      msFree(value);

      value = (char *) msOWSLookupMetadata(&(lp->metadata), "OM", "title");
      if (value) {
        msOWSPrintMetadata(stream, &(lp->metadata), namespaces, "title", OWS_NOERR, "\t<gml:name>%s</gml:name>\n", value);
      }

      /* populate item and group metadata structures */
      itemList = msGMLGetItems(lp, namespaces);
      constantList = msGMLGetConstants(lp, namespaces);
      groupList = msGMLGetGroups(lp, namespaces);
      geometryList = msGMLGetGeometries(lp, namespaces);
      if (itemList == NULL || constantList == NULL || groupList == NULL || geometryList == NULL) {
        msSetError(MS_MISCERR, "Unable to populate item and group metadata structures", "msGMLWriteQuery()");
        return MS_FAILURE;
      }

      for(j=0; j<lp->resultcache->numresults; j++) {
        status = msLayerGetShape(lp, &shape, &(lp->resultcache->results[j]));
        if(status != MS_SUCCESS) return(status);

#ifdef USE_PROJ
        /* project the shape into the map projection (if necessary), note that this projects the bounds as well */
        if(pszOutputSRS == pszMapSRS && msProjectionsDiffer(&(lp->projection), &(map->projection))) {
          status = msProjectShape(&lp->projection, &map->projection, &shape);
          if(status != MS_SUCCESS) {
            msIO_fprintf(stream, "<!-- Warning: Failed to reproject shape: %s -->\n",msGetErrorString(","));
            continue;
          }
        }
#endif

        /* start this feature */
        /* specify a feature name, if nothing provided fall back on the layer name + "_feature" */
        value = (char*) msSmallMalloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "featurename", OWS_NOERR, "\t\t<%s>\n", value);
        msFree(value);

        /* Write the feature geometry and bounding box unless 'none' was requested. */
        /* Default to bbox only if nothing specified and output full geometry only if explicitly requested */
        if(!(geometryList && geometryList->numgeometries == 1 && strcasecmp(geometryList->geometries[0].name, "none") == 0)) {

#ifdef USE_PROJ
          gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), pszOutputSRS, "\t\t\t");
          if (geometryList && geometryList->numgeometries > 0 )
            gmlWriteGeometry(stream, geometryList, OWS_GML2, &(shape), pszOutputSRS, NULL, "\t\t\t");
#else
          gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), NULL, "\t\t\t"); /* no projection information */
          if (geometryList && geometryList->numgeometries > 0 )
            gmlWriteGeometry(stream, geometryList, OWS_GML2, &(shape), NULL, NULL, "\t\t\t");
#endif

        }

        /* write any item/values */
        for(k=0; k<itemList->numitems; k++) {
          item = &(itemList->items[k]);
          if(msItemInGroups(item->name, groupList) == MS_FALSE)
            msGMLWriteItem(stream, item, shape.values[k], NULL, "\t\t\t");
        }

        /* write any constants */
        for(k=0; k<constantList->numconstants; k++) {
          constant = &(constantList->constants[k]);
          if(msItemInGroups(constant->name, groupList) == MS_FALSE)
            msGMLWriteConstant(stream, constant, NULL, "\t\t\t");
        }

        /* write any groups */
        for(k=0; k<groupList->numgroups; k++)
          msGMLWriteGroup(stream, &(groupList->groups[k]), &shape, itemList, constantList, NULL, "\t\t\t");

        /* end this feature */
        /* specify a feature name if nothing provided */
        /* fall back on the layer name + "Feature" */
        value = (char*) msSmallMalloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "featurename", OWS_NOERR, "\t\t</%s>\n", value);
        msFree(value);

        msFreeShape(&shape); /* init too */
      }

      /* end this collection (layer) */
      /* if no layer name provided fall back on the layer name + "_layer" */
      value = (char*) msSmallMalloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "layername", OWS_NOERR, "\t</%s>\n", value);
      msFree(value);

      msGMLFreeGroups(groupList);
      msGMLFreeConstants(constantList);
      msGMLFreeItems(itemList);
      msGMLFreeGeometries(geometryList);

      /* msLayerClose(lp); */
    }
  } /* next layer */

  /* end this document */
  msOWSPrintValidateMetadata(stream, &(map->web.metadata), namespaces, "rootname", OWS_NOERR, "</%s>\n", "msGMLOutput");

  if(filename && strlen(filename) > 0) fclose(stream);

  return(MS_SUCCESS);

#else /* Stub for mapscript */
  msSetError(MS_MISCERR, "WMS server support not enabled", "msGMLWriteQuery()");
  return MS_FAILURE;
#endif
}


/************************************************************************/
/*                             msAxisSwapShape                          */
/*                                                                      */
/*      Utility function to swap x and y coordiatesn Use for now for    */
/*      WFS 1.1.x                                                       */
/************************************************************************/
void msAxisSwapShape(shapeObj *shape)
{
  double tmp;
  int i,j;

  if (shape) {
    for(i=0; i<shape->numlines; i++) {
      for( j=0; j<shape->line[i].numpoints; j++ ) {
        tmp = shape->line[i].point[j].x;
        shape->line[i].point[j].x = shape->line[i].point[j].y;
        shape->line[i].point[j].y = tmp;
      }
    }

    /*swap bounds*/
    tmp = shape->bounds.minx;
    shape->bounds.minx = shape->bounds.miny;
    shape->bounds.miny = tmp;

    tmp = shape->bounds.maxx;
    shape->bounds.maxx = shape->bounds.maxy;
    shape->bounds.maxy = tmp;
  }
}
/*
** msGMLWriteWFSQuery()
**
** Similar to msGMLWriteQuery() but tuned for use with WFS 1.0.0
*/
int msGMLWriteWFSQuery(mapObj *map, FILE *stream, char *default_namespace_prefix, int outputformat)
{
#ifdef USE_WFS_SVR
  int status;
  int i,j,k;
  layerObj *lp=NULL;
  shapeObj shape;
  rectObj  resultBounds = {-1.0,-1.0,-1.0,-1.0};
  int features = 0;

  gmlGroupListObj *groupList=NULL;
  gmlItemListObj *itemList=NULL;
  gmlConstantListObj *constantList=NULL;
  gmlGeometryListObj *geometryList=NULL;
  gmlItemObj *item=NULL;
  gmlConstantObj *constant=NULL;

  char *namespace_prefix=NULL;
  const char *axis = NULL;
  int bSwapAxis = 0;
  double tmp;
  const char *srsMap =  NULL;

  msInitShape(&shape);

  /*add a check to see if the map projection is set to be north-east*/
  for( i = 0; i < map->projection.numargs; i++ ) {
    if( strstr(map->projection.args[i],"epsgaxis=") != NULL ) {
      axis = strstr(map->projection.args[i],"=") + 1;
      break;
    }
  }

  if (axis && strcasecmp(axis,"ne") == 0 )
    bSwapAxis = 1;


  /* Need to start with BBOX of the whole resultset */
  if (msGetQueryResultBounds(map, &resultBounds) > 0) {
    if (bSwapAxis) {
      tmp = resultBounds.minx;
      resultBounds.minx =  resultBounds.miny;
      resultBounds.miny = tmp;

      tmp = resultBounds.maxx;
      resultBounds.maxx =  resultBounds.maxy;
      resultBounds.maxy = tmp;

    }
    srsMap = msOWSGetEPSGProj(&(map->projection), NULL, "FGO", MS_TRUE);
    if (!srsMap)
      msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE);

    gmlWriteBounds(stream, outputformat, &resultBounds, srsMap, "      ");
  }
  /* step through the layers looking for query results */
  for(i=0; i<map->numlayers; i++) {

    lp = GET_LAYER(map, map->layerorder[i]);

    if(lp->resultcache && lp->resultcache->numresults > 0)  { /* found results */
      char *layerName;
      const char *value;
      int featureIdIndex=-1; /* no feature id */


      /* setup namespace, a layer can override the default */
      namespace_prefix = (char*) msOWSLookupMetadata(&(lp->metadata), "OFG", "namespace_prefix");
      if(!namespace_prefix) namespace_prefix = default_namespace_prefix;

      value = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
      if(value) { /* find the featureid amongst the items for this layer */
        for(j=0; j<lp->numitems; j++) {
          if(strcasecmp(lp->items[j], value) == 0) { /* found it */
            featureIdIndex = j;
            break;
          }
        }

        /* Produce a warning if a featureid was set but the corresponding item is not found. */
        if (featureIdIndex == -1)
          msIO_fprintf(stream, "<!-- WARNING: FeatureId item '%s' not found in typename '%s'. -->\n", value, lp->name);
      }

      /* populate item and group metadata structures */
      itemList = msGMLGetItems(lp, "G");
      constantList = msGMLGetConstants(lp, "G");
      groupList = msGMLGetGroups(lp, "G");
      geometryList = msGMLGetGeometries(lp, "GFO");
      if (itemList == NULL || constantList == NULL || groupList == NULL || geometryList == NULL) {
        msSetError(MS_MISCERR, "Unable to populate item and group metadata structures", "msGMLWriteWFSQuery()");
        return MS_FAILURE;
      }


      if (namespace_prefix) {
        layerName = (char *) msSmallMalloc(strlen(namespace_prefix)+strlen(lp->name)+2);
        sprintf(layerName, "%s:%s", namespace_prefix, lp->name);
      } else {
        layerName = msStrdup(lp->name);
      }

      for(j=0; j<lp->resultcache->numresults; j++) {

        status = msLayerGetShape(lp, &shape, &(lp->resultcache->results[j]));
        if(status != MS_SUCCESS)
          return(status);

#ifdef USE_PROJ
        /* project the shape into the map projection (if necessary), note that this projects the bounds as well */
        if(msProjectionsDiffer(&(lp->projection), &(map->projection)))
          msProjectShape(&lp->projection, &map->projection, &shape);
#endif

        /*
        ** start this feature
        */
        msIO_fprintf(stream, "    <gml:featureMember>\n");
        if(msIsXMLTagValid(layerName) == MS_FALSE)
          msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid in a XML tag context. -->\n", layerName);
        if(featureIdIndex != -1) {
	  if(lp->encoding!=NULL && strncasecmp("utf-8",lp->encoding,5)!=0){
	    char* tmp=msGetEncodedString(shape.values[featureIdIndex],lp->encoding);
	    if(outputformat == OWS_GML2)
	      msIO_fprintf(stream, "      <%s fid=\"%s.%s\">\n", layerName, lp->name, tmp);
	    else  /* OWS_GML3 */
	      msIO_fprintf(stream, "      <%s gml:id=\"%s.%s\">\n", layerName, lp->name, tmp);
	    free(tmp);
	  } else
	    msIO_fprintf(stream, "      <%s gml:id=\"%s.%s\">\n", layerName, lp->name, shape.values[featureIdIndex]);
	}else
	  msIO_fprintf(stream, "      <%s>\n", layerName);
        if (bSwapAxis)
          msAxisSwapShape(&shape);

        /* write the feature geometry and bounding box */
        if(!(geometryList && geometryList->numgeometries == 1 && strcasecmp(geometryList->geometries[0].name, "none") == 0)) {
#ifdef USE_PROJ
          srsMap = msOWSGetEPSGProj(&(map->projection), NULL, "FGO", MS_TRUE);
          if (!srsMap)
            msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE);
          if(srsMap) { /* use the map projection first*/
            gmlWriteBounds(stream, outputformat, &(shape.bounds), srsMap, "        ");
            gmlWriteGeometry(stream, geometryList, outputformat, &(shape), srsMap, namespace_prefix, "        ");
          } else { /* then use the layer projection and/or metadata */
            gmlWriteBounds(stream, outputformat, &(shape.bounds), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), "        ");
            gmlWriteGeometry(stream, geometryList, outputformat, &(shape), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), namespace_prefix, "        ");
          }
#else
          gmlWriteBounds(stream, outputformat, &(shape.bounds), NULL, "        "); /* no projection information */
          gmlWriteGeometry(stream, geometryList, outputformat, &(shape), NULL, namespace_prefix, "        ");
#endif
        }

        /* write any item/values */
        for(k=0; k<itemList->numitems; k++) {
          item = &(itemList->items[k]);
          if(msItemInGroups(item->name, groupList) == MS_FALSE){
	    if(lp->encoding!=NULL && strncasecmp("utf-8",lp->encoding,5)!=0){
	      char* tmp=msGetEncodedString(shape.values[k],lp->encoding);
	      msGMLWriteItem(stream, item, tmp, namespace_prefix, "        ");
	      free(tmp);
	    }else
	      msGMLWriteItem(stream, item, shape.values[k], namespace_prefix, "        ");
	  }
        }

        /* write any constants */
        for(k=0; k<constantList->numconstants; k++) {
          constant = &(constantList->constants[k]);
          if(msItemInGroups(constant->name, groupList) == MS_FALSE)
            msGMLWriteConstant(stream, constant, namespace_prefix, "        ");
        }

        /* write any groups */
        for(k=0; k<groupList->numgroups; k++)
          msGMLWriteGroup(stream, &(groupList->groups[k]), &shape, itemList, constantList, namespace_prefix, "        ");

        /* end this feature */
        msIO_fprintf(stream, "      </%s>\n", layerName);
        msIO_fprintf(stream, "    </gml:featureMember>\n");

        msFreeShape(&shape); /* init too */

        features++;
      }

      /* done with this layer, do a little clean-up */
      msFree(layerName);

      msGMLFreeGroups(groupList);
      msGMLFreeConstants(constantList);
      msGMLFreeItems(itemList);
      msGMLFreeGeometries(geometryList);

      /* msLayerClose(lp); */
    }

  } /* next layer */

  return(MS_SUCCESS);

#else /* Stub for mapscript */
  msSetError(MS_MISCERR, "WMS server support not enabled", "msGMLWriteWFSQuery()");
  return MS_FAILURE;
#endif /* USE_WFS_SVR */
}


#ifdef USE_LIBXML2

/**
 * msGML3BoundedBy()
 *
 * returns an object of BoundedBy as per GML 3
 *
 * @param xmlNsPtr psNs the namespace object
 * @param minx minx
 * @param miny miny
 * @param maxx maxx
 * @param mayy mayy
 * @param psEpsg epsg code
 * @param dimension number of dimensions
 *
 * @return psNode xmlNodePtr of XML construct
 *
 */

xmlNodePtr msGML3BoundedBy(xmlNsPtr psNs, double minx, double miny, double maxx, double maxy, const char *psEpsg)
{
  xmlNodePtr psNode = NULL, psSubNode = NULL;
  char *pszTmp = NULL;
  char *pszTmp2 = NULL;
  char *pszEpsg = NULL;
  size_t bufferSize = 0;

  psNode = xmlNewNode(psNs, BAD_CAST "boundedBy");
  psSubNode = xmlNewChild(psNode, NULL, BAD_CAST "Envelope", NULL);

  if (psEpsg) {
    bufferSize = strlen(psEpsg)+1;
    pszEpsg = (char*) msSmallMalloc(bufferSize);
    snprintf(pszEpsg, bufferSize, "%s", psEpsg);
    msStringToLower(pszEpsg);
    pszTmp = msStringConcatenate(pszTmp, "urn:ogc:crs:");
    pszTmp = msStringConcatenate(pszTmp, pszEpsg);
    xmlNewProp(psSubNode, BAD_CAST "srsName", BAD_CAST pszTmp);
    free(pszEpsg);
    free(pszTmp);
    pszTmp = msIntToString(2);
    xmlNewProp(psSubNode, BAD_CAST "srsDimension", BAD_CAST pszTmp);
    free(pszTmp);
  }

  pszTmp = msDoubleToString(minx, MS_TRUE);
  pszTmp = msStringConcatenate(pszTmp, " ");
  pszTmp2 = msDoubleToString(miny, MS_TRUE);
  pszTmp = msStringConcatenate(pszTmp, pszTmp2);
  xmlNewChild(psSubNode, NULL, BAD_CAST "lowerCorner", BAD_CAST pszTmp);
  free(pszTmp);
  free(pszTmp2);

  pszTmp = msDoubleToString(maxx, MS_TRUE);
  pszTmp = msStringConcatenate(pszTmp, " ");
  pszTmp2 = msDoubleToString(maxy,MS_TRUE);
  pszTmp = msStringConcatenate(pszTmp, pszTmp2);
  xmlNewChild(psSubNode, NULL, BAD_CAST "upperCorner", BAD_CAST pszTmp);
  free(pszTmp);
  free(pszTmp2);
  return psNode;
}

/**
 *msGML3Point()
 *
 * returns an object of Point as per GML 3
 *
 * @param xmlNsPtr psNs the gml namespace object
 * @param pszSrsName EPSG code of geometry
 * @param id
 * @param x x coordinate
 * @param y y coordinate
 * @param z z coordinate
 *
 * @return psNode xmlNodePtr of XML construct
 *
 */

xmlNodePtr msGML3Point(xmlNsPtr psNs, const char *psSrsName, const char *id, double x, double y)
{
  xmlNodePtr psNode = NULL;
  char *pszTmp = NULL;
  int dimension = 2;
  char *pszSrsName = NULL;
  char *pszTmp2 = NULL;
  size_t bufferSize = 0;

  psNode = xmlNewNode(psNs, BAD_CAST "Point");

  if (id) {
    xmlNewNsProp(psNode, psNs, BAD_CAST "id", BAD_CAST id);
  }

  if (psSrsName) {
    bufferSize = strlen(psSrsName)+1;
    pszSrsName = (char *) msSmallMalloc(bufferSize);
    snprintf(pszSrsName, bufferSize, "%s", psSrsName);
    msStringToLower(pszSrsName);
    pszTmp = msStringConcatenate(pszTmp, "urn:ogc:crs:");
    pszTmp = msStringConcatenate(pszTmp, pszSrsName);
    xmlNewProp(psNode, BAD_CAST "srsName", BAD_CAST pszTmp);
    free(pszSrsName);
    free(pszTmp);
    pszTmp = msIntToString(dimension);
    xmlNewProp(psNode, BAD_CAST "srsDimension", BAD_CAST pszTmp);
    free(pszTmp);
  }

  pszTmp = msDoubleToString(x, MS_TRUE);
  pszTmp = msStringConcatenate(pszTmp, " ");
  pszTmp2 = msDoubleToString(y, MS_TRUE);
  pszTmp = msStringConcatenate(pszTmp, pszTmp2);
  xmlNewChild(psNode, NULL, BAD_CAST "pos", BAD_CAST pszTmp);

  free(pszTmp);
  free(pszTmp2);
  return psNode;
}

/**
 * msGML3TimePeriod()
 *
 * returns an object of TimePeriod as per GML 3
 *
 * @param xmlNsPtr psNs the gml namespace object
 * @param pszStart start time
 * @param pszEnd end time
 *
 * @return psNode xmlNodePtr of XML construct
 *
 */

xmlNodePtr msGML3TimePeriod(xmlNsPtr psNs, char *pszStart, char *pszEnd)
{
  xmlNodePtr psNode=NULL;

  psNode = xmlNewNode(psNs, BAD_CAST "TimePeriod");
  xmlNewChild(psNode, NULL, BAD_CAST "beginPosition", BAD_CAST pszStart);
  if (pszEnd)
    xmlNewChild(psNode, NULL, BAD_CAST "endPosition", BAD_CAST pszEnd);
  else {
    xmlNewChild(psNode, NULL, BAD_CAST "endPosition", NULL);
    xmlNewProp(psNode, BAD_CAST "indeterminatePosition", BAD_CAST "now");
  }
  return psNode;
}

/**
 * msGML3TimeInstant()
 *
 * returns an object of TimeInstant as per GML 3
 *
 * @param xmlNsPtr psNs the gml namespace object
 * @param timeInstant time instant
 *
 * @return psNode xmlNodePtr of XML construct
 *
 */

xmlNodePtr msGML3TimeInstant(xmlNsPtr psNs, char *pszTime)
{
  xmlNodePtr psNode=NULL;

  psNode = xmlNewNode(psNs, BAD_CAST "TimeInstant");
  xmlNewChild(psNode, NULL, BAD_CAST "timePosition", BAD_CAST pszTime);
  return psNode;
}

#endif /* USE_LIBXML2 */


/************************************************************************/
/*      The following functions are enabled in all cases, even if       */
/*      WMS and WFS are not available.                                  */
/************************************************************************/

gmlItemListObj *msGMLGetItems(layerObj *layer, const char *metadata_namespaces)
{
  int i,j;

  char **xmlitems=NULL;
  int numxmlitems=0;

  char **incitems=NULL;
  int numincitems=0;

  char **excitems=NULL;
  int numexcitems=0;

  const char *value=NULL;
  char tag[64];

  gmlItemListObj *itemList=NULL;
  gmlItemObj *item=NULL;

  /* get a list of items that should be included in output */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "include_items")) != NULL)
    incitems = msStringSplit(value, ',', &numincitems);

  /* get a list of items that should be excluded in output */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "exclude_items")) != NULL)
    excitems = msStringSplit(value, ',', &numexcitems);

  /* get a list of items that need don't get encoded */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "xml_items")) != NULL)
    xmlitems = msStringSplit(value, ',', &numxmlitems);

  /* allocate memory and iinitialize the item collection */
  itemList = (gmlItemListObj *) malloc(sizeof(gmlItemListObj));
  if(itemList == NULL) {
    msSetError(MS_MEMERR, "Error allocating a collection GML item structures.", "msGMLGetItems()");
    return NULL;
  }

  itemList->items = NULL;
  itemList->numitems = 0;

  itemList->numitems = layer->numitems;
  itemList->items = (gmlItemObj *) malloc(sizeof(gmlItemObj)*itemList->numitems);
  if(!itemList->items) {
    msSetError(MS_MEMERR, "Error allocating a collection GML item structures.", "msGMLGetItems()");
    return NULL;
  }

  for(i=0; i<layer->numitems; i++) {
    item = &(itemList->items[i]);

    item->name = msStrdup(layer->items[i]);  /* initialize the item */
    item->alias = NULL;
    item->type = NULL;
    item->template = NULL;
    item->encode = MS_TRUE;
    item->visible = MS_FALSE;
    item->width = 0;
    item->precision = 0;

    /* check visibility, included items first... */
    if(numincitems == 1 && strcasecmp("all", incitems[0]) == 0) {
      item->visible = MS_TRUE;
    } else {
      for(j=0; j<numincitems; j++) {
        if(strcasecmp(layer->items[i], incitems[j]) == 0)
          item->visible = MS_TRUE;
      }
    }

    /* ...and now excluded items */
    for(j=0; j<numexcitems; j++) {
      if(strcasecmp(layer->items[i], excitems[j]) == 0)
        item->visible = MS_FALSE;
    }

    /* check encoding */
    for(j=0; j<numxmlitems; j++) {
      if(strcasecmp(layer->items[i], xmlitems[j]) == 0)
        item->encode = MS_FALSE;
    }

    snprintf(tag, sizeof(tag), "%s_alias", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
      item->alias = msStrdup(value);

    snprintf(tag, sizeof(tag), "%s_type", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
      item->type = msStrdup(value);

    snprintf(tag, sizeof(tag), "%s_template", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
      item->template = msStrdup(value);

    snprintf(tag, sizeof(tag), "%s_width", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
      item->width = atoi(value);

    snprintf(tag, sizeof(tag), "%s_precision", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
      item->precision = atoi(value);
  }

  msFreeCharArray(incitems, numincitems);
  msFreeCharArray(excitems, numexcitems);
  msFreeCharArray(xmlitems, numxmlitems);

  return itemList;
}

void msGMLFreeItems(gmlItemListObj *itemList)
{
  int i;

  if(!itemList) return;

  for(i=0; i<itemList->numitems; i++) {
    msFree(itemList->items[i].name);
    msFree(itemList->items[i].alias);
    msFree(itemList->items[i].type);
    msFree(itemList->items[i].template);
  }

  if( itemList->items != NULL )
    free(itemList->items);

  free(itemList);
}

