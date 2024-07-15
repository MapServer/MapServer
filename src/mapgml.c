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
#include "mapows.h"
#include "mapgml.h"
#include "maptime.h"

/* Use only mapgml.c if WMS or WFS is available (with minor exceptions at end)
 */

#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR)

static int msGMLGeometryLookup(gmlGeometryListObj *geometryList,
                               const char *type);

/*
** Functions that write the feature boundary geometry (i.e. a rectObj).
*/

/* GML 2.1.2 */
static int gmlWriteBounds_GML2(FILE *stream, rectObj *rect, const char *srsname,
                               const char *tab, const char *pszTopPrefix) {
  char *srsname_encoded;

  if (!stream)
    return (MS_FAILURE);
  if (!rect)
    return (MS_FAILURE);
  if (!tab)
    return (MS_FAILURE);

  msIO_fprintf(stream, "%s<%s:boundedBy>\n", tab, pszTopPrefix);
  if (srsname) {
    srsname_encoded = msEncodeHTMLEntities(srsname);
    msIO_fprintf(stream, "%s\t<gml:Box srsName=\"%s\">\n", tab,
                 srsname_encoded);
    msFree(srsname_encoded);
  } else
    msIO_fprintf(stream, "%s\t<gml:Box>\n", tab);

  msIO_fprintf(stream, "%s\t\t<gml:coordinates>", tab);
  msIO_fprintf(stream, "%.6f,%.6f %.6f,%.6f", rect->minx, rect->miny,
               rect->maxx, rect->maxy);
  msIO_fprintf(stream, "</gml:coordinates>\n");
  msIO_fprintf(stream, "%s\t</gml:Box>\n", tab);
  msIO_fprintf(stream, "%s</%s:boundedBy>\n", tab, pszTopPrefix);

  return MS_SUCCESS;
}

/* GML 3.1 or GML 3.2 (MapServer limits GML encoding to the level 0 profile) */
static int gmlWriteBounds_GML3(FILE *stream, rectObj *rect, const char *srsname,
                               const char *tab, const char *pszTopPrefix) {
  char *srsname_encoded;

  if (!stream)
    return (MS_FAILURE);
  if (!rect)
    return (MS_FAILURE);
  if (!tab)
    return (MS_FAILURE);

  msIO_fprintf(stream, "%s<%s:boundedBy>\n", tab, pszTopPrefix);
  if (srsname) {
    srsname_encoded = msEncodeHTMLEntities(srsname);
    msIO_fprintf(stream, "%s\t<gml:Envelope srsName=\"%s\">\n", tab,
                 srsname_encoded);
    msFree(srsname_encoded);
  } else
    msIO_fprintf(stream, "%s\t<gml:Envelope>\n", tab);

  msIO_fprintf(stream, "%s\t\t<gml:lowerCorner>%.6f %.6f</gml:lowerCorner>\n",
               tab, rect->minx, rect->miny);
  msIO_fprintf(stream, "%s\t\t<gml:upperCorner>%.6f %.6f</gml:upperCorner>\n",
               tab, rect->maxx, rect->maxy);

  msIO_fprintf(stream, "%s\t</gml:Envelope>\n", tab);
  msIO_fprintf(stream, "%s</%s:boundedBy>\n", tab, pszTopPrefix);

  return MS_SUCCESS;
}

static void gmlStartGeometryContainer(FILE *stream, const char *name,
                                      const char *namespace, const char *tab) {
  const char *tag_name = OWS_GML_DEFAULT_GEOMETRY_NAME;

  if (name)
    tag_name = name;

  if (namespace)
    msIO_fprintf(stream, "%s<%s:%s>\n", tab, namespace, tag_name);
  else
    msIO_fprintf(stream, "%s<%s>\n", tab, tag_name);
}

static void gmlEndGeometryContainer(FILE *stream, const char *name,
                                    const char *namespace, const char *tab) {
  const char *tag_name = OWS_GML_DEFAULT_GEOMETRY_NAME;

  if (name)
    tag_name = name;

  if (namespace)
    msIO_fprintf(stream, "%s</%s:%s>\n", tab, namespace, tag_name);
  else
    msIO_fprintf(stream, "%s</%s>\n", tab, tag_name);
}

/* GML 2.1.2 */
static int gmlWriteGeometry_GML2(FILE *stream, gmlGeometryListObj *geometryList,
                                 shapeObj *shape, const char *srsname,
                                 const char *namespace, const char *tab,
                                 int nSRSDimension, int geometry_precision) {
  int i, j, k;
  int *innerlist, *outerlist = NULL, numouters;
  char *srsname_encoded = NULL;

  int geometry_aggregate_index, geometry_simple_index;
  char *geometry_aggregate_name = NULL, *geometry_simple_name = NULL;

  if (!stream)
    return (MS_FAILURE);
  if (!shape)
    return (MS_FAILURE);
  if (!tab)
    return (MS_FAILURE);
  if (!geometryList)
    return (MS_FAILURE);

  if (shape->numlines <= 0)
    return (MS_SUCCESS); /* empty shape, nothing to output */

  if (srsname)
    srsname_encoded = msEncodeHTMLEntities(srsname);

  /* feature geometry */
  switch (shape->type) {
  case (MS_SHAPE_POINT):
    geometry_simple_index = msGMLGeometryLookup(geometryList, "point");
    geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multipoint");
    if (geometry_simple_index >= 0)
      geometry_simple_name =
          geometryList->geometries[geometry_simple_index].name;
    if (geometry_aggregate_index >= 0)
      geometry_aggregate_name =
          geometryList->geometries[geometry_aggregate_index].name;

    if ((geometry_simple_index != -1 && shape->line[0].numpoints == 1 &&
         shape->numlines == 1) ||
        (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
        (geometryList->numgeometries == 0 && shape->line[0].numpoints == 1 &&
         shape->numlines == 1)) { /* write a Point(s) */

      for (i = 0; i < shape->numlines; i++) {
        for (j = 0; j < shape->line[i].numpoints; j++) {
          gmlStartGeometryContainer(stream, geometry_simple_name, namespace,
                                    tab);

          /* Point */
          if (srsname_encoded)
            msIO_fprintf(stream, "%s<gml:Point srsName=\"%s\">\n", tab,
                         srsname_encoded);
          else
            msIO_fprintf(stream, "%s<gml:Point>\n", tab);

          if (nSRSDimension == 3)
            msIO_fprintf(
                stream,
                "%s  <gml:coordinates>%.*f,%.*f,%.*f</gml:coordinates>\n", tab,
                geometry_precision, shape->line[i].point[j].x,
                geometry_precision, shape->line[i].point[j].y,
                geometry_precision, shape->line[i].point[j].z);
          else
            /* fall-through */

            msIO_fprintf(stream,
                         "%s  <gml:coordinates>%.*f,%.*f</gml:coordinates>\n",
                         tab, geometry_precision, shape->line[i].point[j].x,
                         geometry_precision, shape->line[i].point[j].y);

          msIO_fprintf(stream, "%s</gml:Point>\n", tab);

          gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
        }
      }
    } else if ((geometry_aggregate_index != -1) ||
               (geometryList->numgeometries == 0)) { /* write a MultiPoint */
      gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace,
                                tab);

      /* MultiPoint */
      if (srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiPoint srsName=\"%s\">\n", tab,
                     srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiPoint>\n", tab);

      for (i = 0; i < shape->numlines; i++) {
        for (j = 0; j < shape->line[i].numpoints; j++) {
          msIO_fprintf(stream, "%s  <gml:pointMember>\n", tab);
          msIO_fprintf(stream, "%s    <gml:Point>\n", tab);
          if (nSRSDimension == 3)
            msIO_fprintf(
                stream,
                "%s      <gml:coordinates>%.*f,%.*f,%.*f</gml:coordinates>\n",
                tab, geometry_precision, shape->line[i].point[j].x,
                geometry_precision, shape->line[i].point[j].y,
                geometry_precision, shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(
                stream, "%s      <gml:coordinates>%f,%f</gml:coordinates>\n",
                tab, shape->line[i].point[j].x, shape->line[i].point[j].y);
          msIO_fprintf(stream, "%s    </gml:Point>\n", tab);
          msIO_fprintf(stream, "%s  </gml:pointMember>\n", tab);
        }
      }

      msIO_fprintf(stream, "%s</gml:MultiPoint>\n", tab);

      gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
    } else {
      msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no "
                           "point/multipoint geometry defined. -->\n");
    }

    break;
  case (MS_SHAPE_LINE):
    geometry_simple_index = msGMLGeometryLookup(geometryList, "line");
    geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multiline");
    if (geometry_simple_index >= 0)
      geometry_simple_name =
          geometryList->geometries[geometry_simple_index].name;
    if (geometry_aggregate_index >= 0)
      geometry_aggregate_name =
          geometryList->geometries[geometry_aggregate_index].name;

    if ((geometry_simple_index != -1 && shape->numlines == 1) ||
        (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
        (geometryList->numgeometries == 0 &&
         shape->numlines == 1)) { /* write a LineStrings(s) */
      for (i = 0; i < shape->numlines; i++) {
        gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

        /* LineString */
        if (srsname_encoded)
          msIO_fprintf(stream, "%s<gml:LineString srsName=\"%s\">\n", tab,
                       srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:LineString>\n", tab);

        msIO_fprintf(stream, "%s  <gml:coordinates>", tab);
        for (j = 0; j < shape->line[i].numpoints; j++) {
          if (nSRSDimension == 3)
            msIO_fprintf(stream, "%.*f,%.*f,%.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y, geometry_precision,
                         shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%.*f,%.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y);
        }
        msIO_fprintf(stream, "</gml:coordinates>\n");

        msIO_fprintf(stream, "%s</gml:LineString>\n", tab);

        gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
      }
    } else if (geometry_aggregate_index != -1 ||
               (geometryList->numgeometries == 0)) { /* write a MultiCurve */
      gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace,
                                tab);

      /* MultiLineString */
      if (srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiLineString srsName=\"%s\">\n", tab,
                     srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiLineString>\n", tab);

      for (j = 0; j < shape->numlines; j++) {
        msIO_fprintf(stream, "%s  <gml:lineStringMember>\n",
                     tab); /* no srsname at this point */
        msIO_fprintf(stream, "%s    <gml:LineString>\n",
                     tab); /* no srsname at this point */

        msIO_fprintf(stream, "%s      <gml:coordinates>", tab);
        for (i = 0; i < shape->line[j].numpoints; i++) {
          if (nSRSDimension == 3)
            msIO_fprintf(stream, "%.*f,%.*f,%.*f ", geometry_precision,
                         shape->line[j].point[i].x, geometry_precision,
                         shape->line[j].point[i].y, geometry_precision,
                         shape->line[j].point[i].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%.*f,%.*f ", geometry_precision,
                         shape->line[j].point[i].x, geometry_precision,
                         shape->line[j].point[i].y);
        }
        msIO_fprintf(stream, "</gml:coordinates>\n");
        msIO_fprintf(stream, "%s    </gml:LineString>\n", tab);
        msIO_fprintf(stream, "%s  </gml:lineStringMember>\n", tab);
      }

      msIO_fprintf(stream, "%s</gml:MultiLineString>\n", tab);

      gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
    } else {
      msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no "
                           "line/multiline geometry defined. -->\n");
    }

    break;
  case (
      MS_SHAPE_POLYGON): /* this gets nasty, since our shapes are so flexible */
    geometry_simple_index = msGMLGeometryLookup(geometryList, "polygon");
    geometry_aggregate_index =
        msGMLGeometryLookup(geometryList, "multipolygon");
    if (geometry_simple_index >= 0)
      geometry_simple_name =
          geometryList->geometries[geometry_simple_index].name;
    if (geometry_aggregate_index >= 0)
      geometry_aggregate_name =
          geometryList->geometries[geometry_aggregate_index].name;

    /* get a list of outter rings for this polygon */
    outerlist = msGetOuterList(shape);

    numouters = 0;
    for (i = 0; i < shape->numlines; i++)
      if (outerlist[i] == MS_TRUE)
        numouters++;

    if ((geometry_simple_index != -1 && numouters == 1) ||
        (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
        (geometryList->numgeometries == 0 &&
         shape->numlines == 1)) { /* write a Polygon(s) */
      for (i = 0; i < shape->numlines; i++) {
        if (outerlist[i] == MS_FALSE)
          break; /* skip non-outer rings, each outer ring is a new polygon */

        /* get a list of inner rings for this polygon */
        innerlist = msGetInnerList(shape, i, outerlist);

        gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

        /* Polygon */
        if (srsname_encoded)
          msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab,
                       srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

        msIO_fprintf(stream, "%s  <gml:outerBoundaryIs>\n", tab);
        msIO_fprintf(stream, "%s    <gml:LinearRing>\n", tab);

        msIO_fprintf(stream, "%s      <gml:coordinates>", tab);
        for (j = 0; j < shape->line[i].numpoints; j++) {
          if (nSRSDimension == 3)
            msIO_fprintf(stream, "%.*f,%.*f,%.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y, geometry_precision,
                         shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%.*f,%.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y);
        }
        msIO_fprintf(stream, "</gml:coordinates>\n");

        msIO_fprintf(stream, "%s    </gml:LinearRing>\n", tab);
        msIO_fprintf(stream, "%s  </gml:outerBoundaryIs>\n", tab);

        for (k = 0; k < shape->numlines;
             k++) { /* now step through all the inner rings */
          if (innerlist[k] == MS_TRUE) {
            msIO_fprintf(stream, "%s  <gml:innerBoundaryIs>\n", tab);
            msIO_fprintf(stream, "%s    <gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s      <gml:coordinates>", tab);
            for (j = 0; j < shape->line[k].numpoints; j++) {
              if (nSRSDimension == 3)
                msIO_fprintf(stream, "%.*f,%.*f,%.*f ", geometry_precision,
                             shape->line[k].point[j].x, geometry_precision,
                             shape->line[k].point[j].y, geometry_precision,
                             shape->line[k].point[j].z);
              else
                /* fall-through */
                msIO_fprintf(stream, "%.*f,%.*f ", geometry_precision,
                             shape->line[k].point[j].x, geometry_precision,
                             shape->line[k].point[j].y);
            }
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
      outerlist = NULL;
    } else if (geometry_aggregate_index != -1 ||
               (geometryList->numgeometries == 0)) { /* write a MultiPolygon */
      gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace,
                                tab);

      /* MultiPolygon */
      if (srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiPolygon srsName=\"%s\">\n", tab,
                     srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiPolygon>\n", tab);

      for (i = 0; i < shape->numlines; i++) { /* step through the outer rings */
        if (outerlist[i] == MS_TRUE) {
          innerlist = msGetInnerList(shape, i, outerlist);

          msIO_fprintf(stream, "%s<gml:polygonMember>\n", tab);
          msIO_fprintf(stream, "%s  <gml:Polygon>\n", tab);

          msIO_fprintf(stream, "%s    <gml:outerBoundaryIs>\n", tab);
          msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

          msIO_fprintf(stream, "%s        <gml:coordinates>", tab);
          for (j = 0; j < shape->line[i].numpoints; j++) {
            if (nSRSDimension == 3)
              msIO_fprintf(stream, "%.*f,%.*f,%.*f ", geometry_precision,
                           shape->line[i].point[j].x, geometry_precision,
                           shape->line[i].point[j].y, geometry_precision,
                           shape->line[i].point[j].z);
            else
              /* fall-through */
              msIO_fprintf(stream, "%.*f,%.*f ", geometry_precision,
                           shape->line[i].point[j].x, geometry_precision,
                           shape->line[i].point[j].y);
          }
          msIO_fprintf(stream, "</gml:coordinates>\n");

          msIO_fprintf(stream, "%s      </gml:LinearRing>\n", tab);
          msIO_fprintf(stream, "%s    </gml:outerBoundaryIs>\n", tab);

          for (k = 0; k < shape->numlines;
               k++) { /* now step through all the inner rings */
            if (innerlist[k] == MS_TRUE) {
              msIO_fprintf(stream, "%s    <gml:innerBoundaryIs>\n", tab);
              msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

              msIO_fprintf(stream, "%s        <gml:coordinates>", tab);
              for (j = 0; j < shape->line[k].numpoints; j++) {
                if (nSRSDimension == 3)
                  msIO_fprintf(stream, "%.*f,%.*f,%.*f ", geometry_precision,
                               shape->line[k].point[j].x, geometry_precision,
                               shape->line[k].point[j].y, geometry_precision,
                               shape->line[k].point[j].z);
                else
                  /* fall-through */
                  msIO_fprintf(stream, "%.*f,%.*f ", geometry_precision,
                               shape->line[k].point[j].x, geometry_precision,
                               shape->line[k].point[j].y);
              }
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
      outerlist = NULL;

      gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
    } else {
      msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no "
                           "polygon/multipolygon geometry defined. -->\n");
    }

    break;
  default:
    break;
  }

  /* clean-up */
  msFree(srsname_encoded);
  msFree(outerlist);

  return (MS_SUCCESS);
}

static char *gmlCreateGeomId(OWSGMLVersion nGMLVersion, const char *pszFID,
                             int *p_id) {
  char *pszGMLId;
  if (nGMLVersion == OWS_GML32) {
    pszGMLId =
        (char *)msSmallMalloc(strlen(pszFID) + 1 + strlen(" gml:id=\"\"") + 10);
    sprintf(pszGMLId, " gml:id=\"%s.%d\"", pszFID, *p_id);
    (*p_id)++;
  } else
    pszGMLId = msStrdup("");
  return pszGMLId;
}

/* GML 3.1 or GML 3.2 (MapServer limits GML encoding to the level 0 profile) */
static int gmlWriteGeometry_GML3(FILE *stream, gmlGeometryListObj *geometryList,
                                 shapeObj *shape, const char *srsname,
                                 const char *namespace, const char *tab,
                                 const char *pszFID, OWSGMLVersion nGMLVersion,
                                 int nSRSDimension, int geometry_precision) {
  int i, j, k, id = 1;
  int *innerlist, *outerlist = NULL, numouters;
  char *srsname_encoded = NULL;
  char *pszGMLId;

  int geometry_aggregate_index, geometry_simple_index;
  char *geometry_aggregate_name = NULL, *geometry_simple_name = NULL;

  if (!stream)
    return (MS_FAILURE);
  if (!shape)
    return (MS_FAILURE);
  if (!tab)
    return (MS_FAILURE);
  if (!geometryList)
    return (MS_FAILURE);

  if (shape->numlines <= 0)
    return (MS_SUCCESS); /* empty shape, nothing to output */

  if (srsname)
    srsname_encoded = msEncodeHTMLEntities(srsname);

  /* feature geometry */
  switch (shape->type) {
  case (MS_SHAPE_POINT):
    geometry_simple_index = msGMLGeometryLookup(geometryList, "point");
    geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multipoint");
    if (geometry_simple_index >= 0)
      geometry_simple_name =
          geometryList->geometries[geometry_simple_index].name;
    if (geometry_aggregate_index >= 0)
      geometry_aggregate_name =
          geometryList->geometries[geometry_aggregate_index].name;

    if ((geometry_simple_index != -1 && shape->line[0].numpoints == 1 &&
         shape->numlines == 1) ||
        (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
        (geometryList->numgeometries == 0 && shape->line[0].numpoints == 1 &&
         shape->numlines == 1)) { /* write a Point(s) */

      for (i = 0; i < shape->numlines; i++) {
        for (j = 0; j < shape->line[i].numpoints; j++) {
          gmlStartGeometryContainer(stream, geometry_simple_name, namespace,
                                    tab);

          /* Point */
          pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);
          if (srsname_encoded)
            msIO_fprintf(stream, "%s  <gml:Point%s srsName=\"%s\">\n", tab,
                         pszGMLId, srsname_encoded);
          else
            msIO_fprintf(stream, "%s  <gml:Point%s>\n", tab, pszGMLId);

          if (nSRSDimension == 3)
            msIO_fprintf(
                stream,
                "%s    <gml:pos srsDimension=\"3\">%.*f %.*f %.*f</gml:pos>\n",
                tab, geometry_precision, shape->line[i].point[j].x,
                geometry_precision, shape->line[i].point[j].y,
                geometry_precision, shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%s    <gml:pos>%.*f %.*f</gml:pos>\n", tab,
                         geometry_precision, shape->line[i].point[j].x,
                         geometry_precision, shape->line[i].point[j].y);

          msIO_fprintf(stream, "%s  </gml:Point>\n", tab);

          gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
          msFree(pszGMLId);
        }
      }
    } else if ((geometry_aggregate_index != -1) ||
               (geometryList->numgeometries == 0)) { /* write a MultiPoint */
      pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);
      gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace,
                                tab);

      /* MultiPoint */
      if (srsname_encoded)
        msIO_fprintf(stream, "%s  <gml:MultiPoint%s srsName=\"%s\">\n", tab,
                     pszGMLId, srsname_encoded);
      else
        msIO_fprintf(stream, "%s  <gml:MultiPoint%s>\n", tab, pszGMLId);

      msFree(pszGMLId);

      for (i = 0; i < shape->numlines; i++) {
        for (j = 0; j < shape->line[i].numpoints; j++) {
          msIO_fprintf(stream, "%s    <gml:pointMember>\n", tab);
          pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);
          msIO_fprintf(stream, "%s      <gml:Point%s>\n", tab, pszGMLId);
          if (nSRSDimension == 3)
            msIO_fprintf(stream,
                         "%s        <gml:pos srsDimension=\"3\">%.*f %.*f "
                         "%.*f</gml:pos>\n",
                         tab, geometry_precision, shape->line[i].point[j].x,
                         geometry_precision, shape->line[i].point[j].y,
                         geometry_precision, shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%s        <gml:pos>%.*f %.*f</gml:pos>\n",
                         tab, geometry_precision, shape->line[i].point[j].x,
                         geometry_precision, shape->line[i].point[j].y);
          msIO_fprintf(stream, "%s      </gml:Point>\n", tab);
          msFree(pszGMLId);
          msIO_fprintf(stream, "%s    </gml:pointMember>\n", tab);
        }
      }

      msIO_fprintf(stream, "%s  </gml:MultiPoint>\n", tab);

      gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
    } else {
      msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no "
                           "point/multipoint geometry defined. -->\n");
    }

    break;
  case (MS_SHAPE_LINE):
    geometry_simple_index = msGMLGeometryLookup(geometryList, "line");
    geometry_aggregate_index = msGMLGeometryLookup(geometryList, "multiline");
    if (geometry_simple_index >= 0)
      geometry_simple_name =
          geometryList->geometries[geometry_simple_index].name;
    if (geometry_aggregate_index >= 0)
      geometry_aggregate_name =
          geometryList->geometries[geometry_aggregate_index].name;

    if ((geometry_simple_index != -1 && shape->numlines == 1) ||
        (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
        (geometryList->numgeometries == 0 &&
         shape->numlines == 1)) { /* write a LineStrings(s) */
      for (i = 0; i < shape->numlines; i++) {
        gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

        /* LineString (should be Curve?) */
        pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);
        if (srsname_encoded)
          msIO_fprintf(stream, "%s  <gml:LineString%s srsName=\"%s\">\n", tab,
                       pszGMLId, srsname_encoded);
        else
          msIO_fprintf(stream, "%s  <gml:LineString%s>\n", tab, pszGMLId);
        msFree(pszGMLId);

        msIO_fprintf(stream, "%s    <gml:posList srsDimension=\"%d\">", tab,
                     nSRSDimension);
        for (j = 0; j < shape->line[i].numpoints; j++) {
          if (nSRSDimension == 3)
            msIO_fprintf(stream, "%.*f %.*f %.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y, geometry_precision,
                         shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%.*f %.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y);
        }
        msIO_fprintf(stream, "</gml:posList>\n");

        msIO_fprintf(stream, "%s  </gml:LineString>\n", tab);

        gmlEndGeometryContainer(stream, geometry_simple_name, namespace, tab);
      }
    } else if (geometry_aggregate_index != -1 ||
               (geometryList->numgeometries == 0)) { /* write a MultiCurve */
      gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace,
                                tab);

      /* MultiCurve */
      pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);
      if (srsname_encoded)
        msIO_fprintf(stream, "%s  <gml:MultiCurve%s srsName=\"%s\">\n", tab,
                     pszGMLId, srsname_encoded);
      else
        msIO_fprintf(stream, "%s  <gml:MultiCurve%s>\n", tab, pszGMLId);
      msFree(pszGMLId);

      for (i = 0; i < shape->numlines; i++) {
        msIO_fprintf(stream, "%s    <gml:curveMember>\n", tab);
        pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);
        msIO_fprintf(stream, "%s      <gml:LineString%s>\n", tab,
                     pszGMLId); /* no srsname at this point */
        msFree(pszGMLId);

        msIO_fprintf(stream, "%s        <gml:posList srsDimension=\"%d\">", tab,
                     nSRSDimension);
        for (j = 0; j < shape->line[i].numpoints; j++) {
          if (nSRSDimension == 3)
            msIO_fprintf(stream, "%.*f %.*f %.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y, geometry_precision,
                         shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%.*f %.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y);
        }

        msIO_fprintf(stream, "</gml:posList>\n");
        msIO_fprintf(stream, "%s      </gml:LineString>\n", tab);
        msIO_fprintf(stream, "%s    </gml:curveMember>\n", tab);
      }

      msIO_fprintf(stream, "%s  </gml:MultiCurve>\n", tab);

      gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
    } else {
      msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no "
                           "line/multiline geometry defined. -->\n");
    }

    break;
  case (
      MS_SHAPE_POLYGON): /* this gets nasty, since our shapes are so flexible */
    geometry_simple_index = msGMLGeometryLookup(geometryList, "polygon");
    geometry_aggregate_index =
        msGMLGeometryLookup(geometryList, "multipolygon");
    if (geometry_simple_index >= 0)
      geometry_simple_name =
          geometryList->geometries[geometry_simple_index].name;
    if (geometry_aggregate_index >= 0)
      geometry_aggregate_name =
          geometryList->geometries[geometry_aggregate_index].name;

    /* get a list of outter rings for this polygon */
    outerlist = msGetOuterList(shape);

    numouters = 0;
    for (i = 0; i < shape->numlines; i++)
      if (outerlist[i] == MS_TRUE)
        numouters++;

    if ((geometry_simple_index != -1 && numouters == 1) ||
        (geometry_simple_index != -1 && geometry_aggregate_index == -1) ||
        (geometryList->numgeometries == 0 &&
         shape->numlines == 1)) { /* write a Polygon(s) */
      for (i = 0; i < shape->numlines; i++) {
        if (outerlist[i] == MS_FALSE)
          break; /* skip non-outer rings, each outer ring is a new polygon */

        /* get a list of inner rings for this polygon */
        innerlist = msGetInnerList(shape, i, outerlist);

        gmlStartGeometryContainer(stream, geometry_simple_name, namespace, tab);

        pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);

        /* Polygon (should be Surface?) */
        if (srsname_encoded)
          msIO_fprintf(stream, "%s  <gml:Polygon%s srsName=\"%s\">\n", tab,
                       pszGMLId, srsname_encoded);
        else
          msIO_fprintf(stream, "%s  <gml:Polygon%s>\n", tab, pszGMLId);
        msFree(pszGMLId);

        msIO_fprintf(stream, "%s    <gml:exterior>\n", tab);
        msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

        msIO_fprintf(stream, "%s        <gml:posList srsDimension=\"%d\">", tab,
                     nSRSDimension);
        for (j = 0; j < shape->line[i].numpoints; j++) {
          if (nSRSDimension == 3)
            msIO_fprintf(stream, "%.*f %.*f %.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y, geometry_precision,
                         shape->line[i].point[j].z);
          else
            /* fall-through */
            msIO_fprintf(stream, "%.*f %.*f ", geometry_precision,
                         shape->line[i].point[j].x, geometry_precision,
                         shape->line[i].point[j].y);
        }

        msIO_fprintf(stream, "</gml:posList>\n");

        msIO_fprintf(stream, "%s      </gml:LinearRing>\n", tab);
        msIO_fprintf(stream, "%s    </gml:exterior>\n", tab);

        for (k = 0; k < shape->numlines;
             k++) { /* now step through all the inner rings */
          if (innerlist[k] == MS_TRUE) {
            msIO_fprintf(stream, "%s    <gml:interior>\n", tab);
            msIO_fprintf(stream, "%s      <gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s        <gml:posList srsDimension=\"%d\">",
                         tab, nSRSDimension);
            for (j = 0; j < shape->line[k].numpoints; j++) {
              if (nSRSDimension == 3)
                msIO_fprintf(stream, "%.*f %.*f %.*f ", geometry_precision,
                             shape->line[k].point[j].x, geometry_precision,
                             shape->line[k].point[j].y, geometry_precision,
                             shape->line[k].point[j].z);
              else
                /* fall-through */
                msIO_fprintf(stream, "%.*f %.*f ", geometry_precision,
                             shape->line[k].point[j].x, geometry_precision,
                             shape->line[k].point[j].y);
            }

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
      outerlist = NULL;
    } else if (geometry_aggregate_index != -1 ||
               (geometryList->numgeometries == 0)) { /* write a MultiSurface */
      gmlStartGeometryContainer(stream, geometry_aggregate_name, namespace,
                                tab);

      pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);

      /* MultiSurface */
      if (srsname_encoded)
        msIO_fprintf(stream, "%s  <gml:MultiSurface%s srsName=\"%s\">\n", tab,
                     pszGMLId, srsname_encoded);
      else
        msIO_fprintf(stream, "%s  <gml:MultiSurface%s>\n", tab, pszGMLId);
      msFree(pszGMLId);

      for (i = 0; i < shape->numlines; i++) { /* step through the outer rings */
        if (outerlist[i] == MS_TRUE) {
          msIO_fprintf(stream, "%s    <gml:surfaceMember>\n", tab);

          /* get a list of inner rings for this polygon */
          innerlist = msGetInnerList(shape, i, outerlist);

          pszGMLId = gmlCreateGeomId(nGMLVersion, pszFID, &id);

          msIO_fprintf(stream, "%s      <gml:Polygon%s>\n", tab, pszGMLId);
          msFree(pszGMLId);

          msIO_fprintf(stream, "%s        <gml:exterior>\n", tab);
          msIO_fprintf(stream, "%s          <gml:LinearRing>\n", tab);

          msIO_fprintf(stream,
                       "%s            <gml:posList srsDimension=\"%d\">", tab,
                       nSRSDimension);
          for (j = 0; j < shape->line[i].numpoints; j++) {
            if (nSRSDimension == 3)
              msIO_fprintf(stream, "%.*f %.*f %.*f ", geometry_precision,
                           shape->line[i].point[j].x, geometry_precision,
                           shape->line[i].point[j].y, geometry_precision,
                           shape->line[i].point[j].z);
            else
              /* fall-through */
              msIO_fprintf(stream, "%.*f %.*f ", geometry_precision,
                           shape->line[i].point[j].x, geometry_precision,
                           shape->line[i].point[j].y);
          }

          msIO_fprintf(stream, "</gml:posList>\n");

          msIO_fprintf(stream, "%s          </gml:LinearRing>\n", tab);
          msIO_fprintf(stream, "%s        </gml:exterior>\n", tab);

          for (k = 0; k < shape->numlines;
               k++) { /* now step through all the inner rings */
            if (innerlist[k] == MS_TRUE) {
              msIO_fprintf(stream, "%s        <gml:interior>\n", tab);
              msIO_fprintf(stream, "%s          <gml:LinearRing>\n", tab);

              msIO_fprintf(stream,
                           "%s            <gml:posList srsDimension=\"%d\">",
                           tab, nSRSDimension);
              for (j = 0; j < shape->line[k].numpoints; j++) {
                if (nSRSDimension == 3)
                  msIO_fprintf(stream, "%.*f %.*f %.*f ", geometry_precision,
                               shape->line[k].point[j].x, geometry_precision,
                               shape->line[k].point[j].y, geometry_precision,
                               shape->line[k].point[j].z);
                else
                  /* fall-through */
                  msIO_fprintf(stream, "%.*f %.*f ", geometry_precision,
                               shape->line[k].point[j].x, geometry_precision,
                               shape->line[k].point[j].y);
              }
              msIO_fprintf(stream, "</gml:posList>\n");

              msIO_fprintf(stream, "%s          </gml:LinearRing>\n", tab);
              msIO_fprintf(stream, "%s        </gml:interior>\n", tab);
            }
          }

          msIO_fprintf(stream, "%s      </gml:Polygon>\n", tab);

          free(innerlist);
          msIO_fprintf(stream, "%s    </gml:surfaceMember>\n", tab);
        }
      }
      msIO_fprintf(stream, "%s  </gml:MultiSurface>\n", tab);

      free(outerlist);
      outerlist = NULL;

      gmlEndGeometryContainer(stream, geometry_aggregate_name, namespace, tab);
    } else {
      msIO_fprintf(stream, "<!-- Warning: Cannot write geometry- no "
                           "polygon/multipolygon geometry defined. -->\n");
    }

    break;
  default:
    break;
  }

  /* clean-up */
  msFree(outerlist);
  msFree(srsname_encoded);

  return (MS_SUCCESS);
}

/*
** Wrappers for the format specific encoding functions.
*/
static int gmlWriteBounds(FILE *stream, OWSGMLVersion format, rectObj *rect,
                          const char *srsname, const char *tab,
                          const char *pszTopPrefix) {
  switch (format) {
  case (OWS_GML2):
    return gmlWriteBounds_GML2(stream, rect, srsname, tab, pszTopPrefix);
    break;
  case (OWS_GML3):
  case (OWS_GML32):
    return gmlWriteBounds_GML3(stream, rect, srsname, tab, pszTopPrefix);
    break;
  default:
    msSetError(MS_IOERR, "Unsupported GML format.", "gmlWriteBounds()");
  }

  return (MS_FAILURE);
}

static int gmlWriteGeometry(FILE *stream, gmlGeometryListObj *geometryList,
                            OWSGMLVersion format, shapeObj *shape,
                            const char *srsname, const char *namespace,
                            const char *tab, const char *pszFID,
                            int nSRSDimension, int geometry_precision) {
  switch (format) {
  case (OWS_GML2):
    return gmlWriteGeometry_GML2(stream, geometryList, shape, srsname,
                                 namespace, tab, nSRSDimension,
                                 geometry_precision);
    break;
  case (OWS_GML3):
  case (OWS_GML32):
    return gmlWriteGeometry_GML3(stream, geometryList, shape, srsname,
                                 namespace, tab, pszFID, format, nSRSDimension,
                                 geometry_precision);
    break;
  default:
    msSetError(MS_IOERR, "Unsupported GML format.", "gmlWriteGeometry()");
  }

  return (MS_FAILURE);
}

/*
** GML specific metadata handling functions.
*/

int msItemInGroups(const char *name, gmlGroupListObj *groupList) {
  int i, j;
  gmlGroupObj *group;

  if (!groupList)
    return MS_FALSE; /* no groups */

  for (i = 0; i < groupList->numgroups; i++) {
    group = &(groupList->groups[i]);
    for (j = 0; j < group->numitems; j++) {
      if (strcasecmp(name, group->items[j]) == 0)
        return MS_TRUE;
    }
  }

  return MS_FALSE;
}

static int msGMLGeometryLookup(gmlGeometryListObj *geometryList,
                               const char *type) {
  int i;

  if (!geometryList || !type)
    return -1; /* nothing to look for */

  for (i = 0; i < geometryList->numgeometries; i++)
    if (geometryList->geometries[i].type &&
        (strcasecmp(geometryList->geometries[i].type, type) == 0))
      return i;

  if (geometryList->numgeometries == 1 &&
      geometryList->geometries[0].type == NULL)
    return 0;

  return -1; /* not found */
}

gmlGeometryListObj *msGMLGetGeometries(layerObj *layer,
                                       const char *metadata_namespaces,
                                       int bWithDefaultGeom) {
  int i;

  const char *value = NULL;
  char tag[64];

  char **names = NULL;
  int numnames = 0;

  gmlGeometryListObj *geometryList = NULL;
  gmlGeometryObj *geometry = NULL;

  /* allocate memory and initialize the item collection */
  geometryList = (gmlGeometryListObj *)malloc(sizeof(gmlGeometryListObj));
  MS_CHECK_ALLOC(geometryList, sizeof(gmlGeometryListObj), NULL);
  geometryList->geometries = NULL;
  geometryList->numgeometries = 0;

  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "geometries")) != NULL) {
    names = msStringSplit(value, ',', &numnames);

    /* allocation an array of gmlGeometryObj's */
    geometryList->numgeometries = numnames;
    geometryList->geometries = (gmlGeometryObj *)malloc(
        sizeof(gmlGeometryObj) * geometryList->numgeometries);
    if (geometryList->geometries == NULL) {
      msSetError(
          MS_MEMERR, "Out of memory allocating %u bytes.\n",
          "msGMLGetGeometries()",
          (unsigned int)(sizeof(gmlGeometryObj) * geometryList->numgeometries));
      free(geometryList);
      return NULL;
    }

    for (i = 0; i < geometryList->numgeometries; i++) {
      geometry = &(geometryList->geometries[i]);

      geometry->name = msStrdup(names[i]); /* initialize a few things */
      geometry->type = NULL;
      geometry->occurmin = 0;
      geometry->occurmax = 1;

      snprintf(tag, 64, "%s_type", names[i]);
      if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                       tag)) != NULL)
        geometry->type = msStrdup(value); /* TODO: validate input value */

      snprintf(tag, 64, "%s_occurances", names[i]);
      if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                       tag)) != NULL) {
        char **occur;
        int numoccur;

        occur = msStringSplit(value, ',', &numoccur);
        if (numoccur == 2) { /* continue (TODO: throw an error if != 2) */
          geometry->occurmin = atof(occur[0]);
          if (strcasecmp(occur[1], "UNBOUNDED") == 0)
            geometry->occurmax = OWS_GML_OCCUR_UNBOUNDED;
          else
            geometry->occurmax = atof(occur[1]);
        }
        msFreeCharArray(occur, numoccur);
      }
    }

    msFreeCharArray(names, numnames);
  } else if (bWithDefaultGeom) {
    geometryList->numgeometries = 1;
    geometryList->geometries =
        (gmlGeometryObj *)calloc(1, sizeof(gmlGeometryObj));
    geometryList->geometries[0].name = msStrdup(OWS_GML_DEFAULT_GEOMETRY_NAME);
    geometryList->geometries[0].type = NULL;
    geometryList->geometries[0].occurmin = 0;
    geometryList->geometries[0].occurmax = 1;
  }

  return geometryList;
}

void msGMLFreeGeometries(gmlGeometryListObj *geometryList) {
  int i;

  if (!geometryList)
    return;

  for (i = 0; i < geometryList->numgeometries; i++) {
    msFree(geometryList->geometries[i].name);
    msFree(geometryList->geometries[i].type);
  }
  free(geometryList->geometries);

  free(geometryList);
}

static void msGMLWriteItem(FILE *stream, gmlItemObj *item, const char *value,
                           const char *namespace, const char *tab,
                           OWSGMLVersion outputformat, const char *pszFID) {
  char *encoded_value = NULL, *tag_name;
  int add_namespace = MS_TRUE;
  char gmlid[256];
  gmlid[0] = 0;

  if (!stream || !item)
    return;
  if (!item->visible)
    return;

  if (!namespace)
    add_namespace = MS_FALSE;

  if (item->alias)
    tag_name = item->alias;
  else
    tag_name = item->name;
  if (strchr(tag_name, ':') != NULL)
    add_namespace = MS_FALSE;

  if (item->type &&
      (EQUAL(item->type, "Date") || EQUAL(item->type, "DateTime") ||
       EQUAL(item->type, "Time"))) {
    struct tm tm;
    if (msParseTime(value, &tm) == MS_TRUE) {
      const char *pszStartTag = "";
      const char *pszEndTag = "";

      encoded_value = (char *)msSmallMalloc(256);
      if (outputformat == OWS_GML32) {
        if (pszFID != NULL)
          snprintf(gmlid, 256, " gml:id=\"%s.%s\"", pszFID, tag_name);
        pszStartTag = "<gml:timePosition>";
        pszEndTag = "</gml:timePosition>";
      }

      if (EQUAL(item->type, "Date"))
        snprintf(encoded_value, 256, "%s%04d-%02d-%02d%s", pszStartTag,
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, pszEndTag);
      else if (EQUAL(item->type, "Time"))
        snprintf(encoded_value, 256, "%s%02d:%02d:%02dZ%s", pszStartTag,
                 tm.tm_hour, tm.tm_min, tm.tm_sec, pszEndTag);
      else {
        // Detected date time already formatted as YYYY-MM-DDTHH:mm:SS+ab:cd
        // because msParseTime() can't handle time zones.
        if (strlen(value) ==
                4 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 &&
            value[4] == '-' && value[7] == '-' && value[10] == 'T' &&
            value[13] == ':' && value[16] == ':' &&
            (value[19] == '+' || value[19] == '-') && value[22] == ':') {
          snprintf(encoded_value, 256, "%s%s%s", pszStartTag, value, pszEndTag);
        } else {
          snprintf(encoded_value, 256, "%s%04d-%02d-%02dT%02d:%02d:%02dZ%s",
                   pszStartTag, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min, tm.tm_sec, pszEndTag);
        }
      }
    }
  }

  if (encoded_value == NULL) {
    if (item->encode == MS_TRUE)
      encoded_value = msEncodeHTMLEntities(value);
    else
      encoded_value = msStrdup(value);
  }

  if (!item->template) { /* build the tag from pieces */

    if (add_namespace == MS_TRUE && msIsXMLTagValid(tag_name) == MS_FALSE)
      msIO_fprintf(stream,
                   "<!-- WARNING: The value '%s' is not valid in a XML tag "
                   "context. -->\n",
                   tag_name);

    if (add_namespace == MS_TRUE)
      msIO_fprintf(stream, "%s<%s:%s%s>%s</%s:%s>\n", tab, namespace, tag_name,
                   gmlid, encoded_value, namespace, tag_name);
    else
      msIO_fprintf(stream, "%s<%s%s>%s</%s>\n", tab, tag_name, gmlid,
                   encoded_value, tag_name);
  } else {
    char *tag = NULL;

    tag = msStrdup(item->template);
    tag = msReplaceSubstring(tag, "$value", encoded_value);
    if (namespace)
      tag = msReplaceSubstring(tag, "$namespace", namespace);
    msIO_fprintf(stream, "%s%s\n", tab, tag);
    free(tag);
  }

  free(encoded_value);

  return;
}

gmlNamespaceListObj *msGMLGetNamespaces(webObj *web,
                                        const char *metadata_namespaces) {
  int i;
  const char *value = NULL;
  char tag[64];

  char **prefixes = NULL;
  int numprefixes = 0;

  gmlNamespaceListObj *namespaceList = NULL;
  gmlNamespaceObj *namespace = NULL;

  /* allocate the collection */
  namespaceList = (gmlNamespaceListObj *)malloc(sizeof(gmlNamespaceListObj));
  MS_CHECK_ALLOC(namespaceList, sizeof(gmlNamespaceListObj), NULL);
  namespaceList->namespaces = NULL;
  namespaceList->numnamespaces = 0;

  /* list of namespaces (TODO: make this automatic by parsing metadata) */
  if ((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces,
                                   "external_namespace_prefixes")) != NULL) {
    prefixes = msStringSplit(value, ',', &numprefixes);

    /* allocation an array of gmlNamespaceObj's */
    namespaceList->numnamespaces = numprefixes;
    namespaceList->namespaces = (gmlNamespaceObj *)malloc(
        sizeof(gmlNamespaceObj) * namespaceList->numnamespaces);
    if (namespaceList->namespaces == NULL) {
      msSetError(MS_MEMERR, "Out of memory allocating %u bytes.\n",
                 "msGMLGetNamespaces()",
                 (unsigned int)(sizeof(gmlNamespaceObj) *
                                namespaceList->numnamespaces));
      free(namespaceList);
      return NULL;
    }

    for (i = 0; i < namespaceList->numnamespaces; i++) {
      namespace = &(namespaceList->namespaces[i]);

      namespace->prefix = msStrdup(prefixes[i]); /* initialize a few things */
      namespace->uri = NULL;
      namespace->schemalocation = NULL;

      snprintf(tag, 64, "%s_uri", namespace->prefix);
      if ((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces,
                                       tag)) != NULL)
      namespace->uri = msStrdup(value);

      snprintf(tag, 64, "%s_schema_location", namespace->prefix);
      if ((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces,
                                       tag)) != NULL)
      namespace->schemalocation = msStrdup(value);
    }

    msFreeCharArray(prefixes, numprefixes);
  }

  return namespaceList;
}

void msGMLFreeNamespaces(gmlNamespaceListObj *namespaceList) {
  int i;

  if (!namespaceList)
    return;

  for (i = 0; i < namespaceList->numnamespaces; i++) {
    msFree(namespaceList->namespaces[i].prefix);
    msFree(namespaceList->namespaces[i].uri);
    msFree(namespaceList->namespaces[i].schemalocation);
  }

  free(namespaceList);
}

gmlConstantListObj *msGMLGetConstants(layerObj *layer,
                                      const char *metadata_namespaces) {
  int i;
  const char *value = NULL;
  char tag[64];

  char **names = NULL;
  int numnames = 0;

  gmlConstantListObj *constantList = NULL;
  gmlConstantObj *constant = NULL;

  /* allocate the collection */
  constantList = (gmlConstantListObj *)malloc(sizeof(gmlConstantListObj));
  MS_CHECK_ALLOC(constantList, sizeof(gmlConstantListObj), NULL);
  constantList->constants = NULL;
  constantList->numconstants = 0;

  /* list of constants (TODO: make this automatic by parsing metadata) */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "constants")) != NULL) {
    names = msStringSplit(value, ',', &numnames);

    /* allocation an array of gmlConstantObj's */
    constantList->numconstants = numnames;
    constantList->constants = (gmlConstantObj *)malloc(
        sizeof(gmlConstantObj) * constantList->numconstants);
    if (constantList->constants == NULL) {
      msSetError(
          MS_MEMERR, "Out of memory allocating %u bytes.\n",
          "msGMLGetConstants()",
          (unsigned int)(sizeof(gmlConstantObj) * constantList->numconstants));
      free(constantList);
      return NULL;
    }

    for (i = 0; i < constantList->numconstants; i++) {
      constant = &(constantList->constants[i]);

      constant->name = msStrdup(names[i]); /* initialize a few things */
      constant->value = NULL;
      constant->type = NULL;

      snprintf(tag, 64, "%s_value", constant->name);
      if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                       tag)) != NULL)
        constant->value = msStrdup(value);

      snprintf(tag, 64, "%s_type", constant->name);
      if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                       tag)) != NULL)
        constant->type = msStrdup(value);
    }

    msFreeCharArray(names, numnames);
  }

  return constantList;
}

void msGMLFreeConstants(gmlConstantListObj *constantList) {
  int i;

  if (!constantList)
    return;

  for (i = 0; i < constantList->numconstants; i++) {
    msFree(constantList->constants[i].name);
    msFree(constantList->constants[i].value);
    msFree(constantList->constants[i].type);
  }

  free(constantList);
}

static void msGMLWriteConstant(FILE *stream, gmlConstantObj *constant,
                               const char *namespace, const char *tab) {
  int add_namespace = MS_TRUE;

  if (!stream || !constant)
    return;
  if (!constant->value)
    return;

  if (!namespace)
    add_namespace = MS_FALSE;
  if (strchr(constant->name, ':') != NULL)
    add_namespace = MS_FALSE;

  if (add_namespace == MS_TRUE && msIsXMLTagValid(constant->name) == MS_FALSE)
    msIO_fprintf(
        stream,
        "<!-- WARNING: The value '%s' is not valid in a XML tag context. -->\n",
        constant->name);

  if (add_namespace == MS_TRUE)
    msIO_fprintf(stream, "%s<%s:%s>%s</%s:%s>\n", tab, namespace,
                 constant->name, constant->value, namespace, constant->name);
  else
    msIO_fprintf(stream, "%s<%s>%s</%s>\n", tab, constant->name,
                 constant->value, constant->name);

  return;
}

static void msGMLWriteGroup(FILE *stream, gmlGroupObj *group, shapeObj *shape,
                            gmlItemListObj *itemList,
                            gmlConstantListObj *constantList,
                            const char *namespace, const char *tab,
                            OWSGMLVersion outputformat, const char *pszFID) {
  int i, j;
  int add_namespace = MS_TRUE;
  char *itemtab;

  gmlItemObj *item = NULL;
  gmlConstantObj *constant = NULL;

  if (!stream || !group)
    return;

  /* setup the item/constant tab */
  itemtab = (char *)msSmallMalloc(sizeof(char) * strlen(tab) + 3);

  sprintf(itemtab, "%s  ", tab);

  if (!namespace || strchr(group->name, ':') != NULL)
    add_namespace = MS_FALSE;

  /* start the group */
  if (add_namespace == MS_TRUE)
    msIO_fprintf(stream, "%s<%s:%s>\n", tab, namespace, group->name);
  else
    msIO_fprintf(stream, "%s<%s>\n", tab, group->name);

  /* now the items/constants in the group */
  for (i = 0; i < group->numitems; i++) {
    for (j = 0; j < constantList->numconstants; j++) {
      constant = &(constantList->constants[j]);
      if (strcasecmp(constant->name, group->items[i]) == 0) {
        msGMLWriteConstant(stream, constant, namespace, itemtab);
        break;
      }
    }
    if (j != constantList->numconstants)
      continue; /* found this one */
    for (j = 0; j < itemList->numitems; j++) {
      item = &(itemList->items[j]);
      if (strcasecmp(item->name, group->items[i]) == 0) {
        /* the number of items matches the number of values exactly */
        msGMLWriteItem(stream, item, shape->values[j], namespace, itemtab,
                       outputformat, pszFID);
        break;
      }
    }
  }

  /* end the group */
  if (add_namespace == MS_TRUE)
    msIO_fprintf(stream, "%s</%s:%s>\n", tab, namespace, group->name);
  else
    msIO_fprintf(stream, "%s</%s>\n", tab, group->name);

  msFree(itemtab);

  return;
}
#endif

gmlGroupListObj *msGMLGetGroups(layerObj *layer,
                                const char *metadata_namespaces) {
  int i;
  const char *value = NULL;
  char tag[64];

  char **names = NULL;
  int numnames = 0;

  gmlGroupListObj *groupList = NULL;
  gmlGroupObj *group = NULL;

  /* allocate the collection */
  groupList = (gmlGroupListObj *)malloc(sizeof(gmlGroupListObj));
  MS_CHECK_ALLOC(groupList, sizeof(gmlGroupListObj), NULL);
  groupList->groups = NULL;
  groupList->numgroups = 0;

  /* list of groups (TODO: make this automatic by parsing metadata) */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "groups")) != NULL &&
      value[0] != '\0') {
    names = msStringSplit(value, ',', &numnames);

    /* allocation an array of gmlGroupObj's */
    groupList->numgroups = numnames;
    groupList->groups =
        (gmlGroupObj *)malloc(sizeof(gmlGroupObj) * groupList->numgroups);
    if (groupList->groups == NULL) {
      msSetError(MS_MEMERR, "Out of memory allocating %u bytes.\n",
                 "msGMLGetGroups()",
                 (unsigned int)(sizeof(gmlGroupObj) * groupList->numgroups));
      free(groupList);
      return NULL;
    }

    for (i = 0; i < groupList->numgroups; i++) {
      group = &(groupList->groups[i]);

      group->name = msStrdup(names[i]); /* initialize a few things */
      group->items = NULL;
      group->numitems = 0;
      group->type = NULL;

      snprintf(tag, 64, "%s_group", group->name);
      if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                       tag)) != NULL)
        group->items = msStringSplit(value, ',', &group->numitems);

      snprintf(tag, 64, "%s_type", group->name);
      if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                       tag)) != NULL)
        group->type = msStrdup(value);
    }

    msFreeCharArray(names, numnames);
  }

  return groupList;
}

void msGMLFreeGroups(gmlGroupListObj *groupList) {
  int i;

  if (!groupList)
    return;

  for (i = 0; i < groupList->numgroups; i++) {
    msFree(groupList->groups[i].name);
    msFreeCharArray(groupList->groups[i].items, groupList->groups[i].numitems);
    msFree(groupList->groups[i].type);
  }
  msFree(groupList->groups);

  free(groupList);
}

/* Dump GML query results for WMS GetFeatureInfo */
int msGMLWriteQuery(mapObj *map, char *filename, const char *namespaces) {
#if defined(USE_WMS_SVR)
  int status;
  int i, j, k;
  layerObj *lp = NULL;
  shapeObj shape;
  FILE *stream_to_free = NULL;
  FILE *stream = stdout; /* defaults to stdout */
  char szPath[MS_MAXPATHLEN];
  char *value;
  char *pszMapSRS = NULL;

  gmlGroupListObj *groupList = NULL;
  gmlItemListObj *itemList = NULL;
  gmlConstantListObj *constantList = NULL;
  gmlGeometryListObj *geometryList = NULL;
  gmlItemObj *item = NULL;
  gmlConstantObj *constant = NULL;

  msInitShape(&shape);

  if (filename &&
      strlen(filename) > 0) { /* deal with the filename if present */
    stream_to_free = fopen(msBuildPath(szPath, map->mappath, filename), "w");
    if (!stream_to_free) {
      msSetError(MS_IOERR, "(%s)", "msGMLWriteQuery()", filename);
      return (MS_FAILURE);
    }
    stream = stream_to_free;
  }

  msIO_fprintf(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  msOWSPrintValidateMetadata(stream, &(map->web.metadata), namespaces,
                             "rootname", OWS_NOERR, "<%s ", "msGMLOutput");

  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "uri",
                           OWS_NOERR, "xmlns=\"%s\"", NULL);
  msIO_fprintf(stream, "\n\t xmlns:gml=\"http://www.opengis.net/gml\"");
  msIO_fprintf(stream, "\n\t xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
  msIO_fprintf(stream,
               "\n\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "schema",
                           OWS_NOERR, "\n\t xsi:schemaLocation=\"%s\"", NULL);
  msIO_fprintf(stream, ">\n");

  /* a schema *should* be required */
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces,
                           "description", OWS_NOERR,
                           "\t<gml:description>%s</gml:description>\n", NULL);

  /* Look up map SRS. We need an EPSG code for GML, if not then we get null and
   * we'll fall back on the layer's SRS */
  msOWSGetEPSGProj(&(map->projection), NULL, namespaces, MS_TRUE, &pszMapSRS);

  /* step through the layers looking for query results */
  for (i = 0; i < map->numlayers; i++) {
    char *pszOutputSRS = NULL;
    int nSRSDimension = 2;
    const char *geomtype;
    lp = (GET_LAYER(map, map->layerorder[i]));

    if (lp->resultcache &&
        lp->resultcache->numresults > 0) { /* found results */

      reprojectionObj *reprojector = NULL;

      /* Determine output SRS, if map has none, then try using layer's native
       * SRS */
      if ((pszOutputSRS = pszMapSRS) == NULL) {
        msOWSGetEPSGProj(&(lp->projection), NULL, namespaces, MS_TRUE,
                         &pszOutputSRS);
        if (pszOutputSRS == NULL) {
          msSetError(
              MS_WMSERR,
              "No valid EPSG code in map or layer projection for GML output",
              "msGMLWriteQuery()");
          continue; /* No EPSG code, cannot output this layer */
        }
      }

      /* start this collection (layer) */
      /* if no layer name provided fall back on the layer name + "_layer" */
      value = (char *)msSmallMalloc(strlen(lp->name) + 7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces,
                                 "layername", OWS_NOERR, "\t<%s>\n", value);
      msFree(value);

      value = (char *)msOWSLookupMetadata(&(lp->metadata), "OM", "title");
      if (value) {
        msOWSPrintEncodeMetadata(stream, &(lp->metadata), namespaces, "title",
                                 OWS_NOERR, "\t<gml:name>%s</gml:name>\n",
                                 value);
      }

      geomtype = msOWSLookupMetadata(&(lp->metadata), "OFG", "geomtype");
      if (geomtype != NULL && (strstr(geomtype, "25d") != NULL ||
                               strstr(geomtype, "25D") != NULL)) {
        nSRSDimension = 3;
      }

      /* populate item and group metadata structures */
      itemList = msGMLGetItems(lp, namespaces);
      constantList = msGMLGetConstants(lp, namespaces);
      groupList = msGMLGetGroups(lp, namespaces);
      geometryList = msGMLGetGeometries(lp, namespaces, MS_FALSE);
      if (itemList == NULL || constantList == NULL || groupList == NULL ||
          geometryList == NULL) {
        msSetError(MS_MISCERR,
                   "Unable to populate item and group metadata structures",
                   "msGMLWriteQuery()");
        if (stream_to_free != NULL)
          fclose(stream_to_free);
        return MS_FAILURE;
      }

      if (pszOutputSRS == pszMapSRS &&
          msProjectionsDiffer(&(lp->projection), &(map->projection))) {
        reprojector =
            msProjectCreateReprojector(&(lp->projection), &(map->projection));
        if (reprojector == NULL) {
          msGMLFreeGroups(groupList);
          msGMLFreeConstants(constantList);
          msGMLFreeItems(itemList);
          msGMLFreeGeometries(geometryList);
          msFree(pszOutputSRS);
          if (stream_to_free != NULL)
            fclose(stream_to_free);
          return MS_FAILURE;
        }
      }

      for (j = 0; j < lp->resultcache->numresults; j++) {
        status = msLayerGetShape(lp, &shape, &(lp->resultcache->results[j]));
        if (status != MS_SUCCESS) {
          msGMLFreeGroups(groupList);
          msGMLFreeConstants(constantList);
          msGMLFreeItems(itemList);
          msGMLFreeGeometries(geometryList);
          msProjectDestroyReprojector(reprojector);
          msFree(pszOutputSRS);
          if (stream_to_free != NULL)
            fclose(stream_to_free);
          return MS_FAILURE;
        }

        /* project the shape into the map projection (if necessary), note that
         * this projects the bounds as well */
        if (reprojector) {
          status = msProjectShapeEx(reprojector, &shape);
          if (status != MS_SUCCESS) {
            msIO_fprintf(stream,
                         "<!-- Warning: Failed to reproject shape: %s -->\n",
                         msGetErrorString(","));
            msFreeShape(&shape);
            continue;
          }
        }

        /* start this feature */
        /* specify a feature name, if nothing provided fall back on the layer
         * name + "_feature" */
        value = (char *)msSmallMalloc(strlen(lp->name) + 9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces,
                                   "featurename", OWS_NOERR, "\t\t<%s>\n",
                                   value);
        msFree(value);

        /* Write the feature geometry and bounding box unless 'none' was
         * requested. */
        /* Default to bbox only if nothing specified and output full geometry
         * only if explicitly requested */
        if (!(geometryList && geometryList->numgeometries == 1 &&
              strcasecmp(geometryList->geometries[0].name, "none") == 0)) {
          gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), pszOutputSRS,
                         "\t\t\t", "gml");
          if (geometryList && geometryList->numgeometries > 0)
            gmlWriteGeometry(stream, geometryList, OWS_GML2, &(shape),
                             pszOutputSRS, NULL, "\t\t\t", "", nSRSDimension,
                             6);
        }

        /* write any item/values */
        for (k = 0; k < itemList->numitems; k++) {
          item = &(itemList->items[k]);
          if (msItemInGroups(item->name, groupList) == MS_FALSE)
            msGMLWriteItem(stream, item, shape.values[k], NULL, "\t\t\t",
                           OWS_GML2, NULL);
        }

        /* write any constants */
        for (k = 0; k < constantList->numconstants; k++) {
          constant = &(constantList->constants[k]);
          if (msItemInGroups(constant->name, groupList) == MS_FALSE)
            msGMLWriteConstant(stream, constant, NULL, "\t\t\t");
        }

        /* write any groups */
        for (k = 0; k < groupList->numgroups; k++)
          msGMLWriteGroup(stream, &(groupList->groups[k]), &shape, itemList,
                          constantList, NULL, "\t\t\t", OWS_GML2, NULL);

        /* end this feature */
        /* specify a feature name if nothing provided */
        /* fall back on the layer name + "Feature" */
        value = (char *)msSmallMalloc(strlen(lp->name) + 9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces,
                                   "featurename", OWS_NOERR, "\t\t</%s>\n",
                                   value);
        msFree(value);

        msFreeShape(&shape); /* init too */
      }

      msProjectDestroyReprojector(reprojector);

      /* end this collection (layer) */
      /* if no layer name provided fall back on the layer name + "_layer" */
      value = (char *)msSmallMalloc(strlen(lp->name) + 7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces,
                                 "layername", OWS_NOERR, "\t</%s>\n", value);
      msFree(value);

      msGMLFreeGroups(groupList);
      msGMLFreeConstants(constantList);
      msGMLFreeItems(itemList);
      msGMLFreeGeometries(geometryList);

      /* msLayerClose(lp); */
    }
    if (pszOutputSRS != pszMapSRS) {
      msFree(pszOutputSRS);
    }
  } /* next layer */

  /* end this document */
  msOWSPrintValidateMetadata(stream, &(map->web.metadata), namespaces,
                             "rootname", OWS_NOERR, "</%s>\n", "msGMLOutput");

  if (stream_to_free != NULL)
    fclose(stream_to_free);
  msFree(pszMapSRS);

  return (MS_SUCCESS);

#else /* Stub for mapscript */
  msSetError(MS_MISCERR, "WMS server support not enabled", "msGMLWriteQuery()");
  return MS_FAILURE;
#endif
}

#ifdef USE_WFS_SVR
void msGMLWriteWFSBounds(mapObj *map, FILE *stream, const char *tab,
                         OWSGMLVersion outputformat, int nWFSVersion,
                         int bUseURN) {
  rectObj resultBounds = {-1.0, -1.0, -1.0, -1.0};

  /*add a check to see if the map projection is set to be north-east*/
  int bSwapAxis = msIsAxisInvertedProj(&(map->projection));

  /* Need to start with BBOX of the whole resultset */
  if (msGetQueryResultBounds(map, &resultBounds) > 0) {
    char *srs = NULL;
    if (bSwapAxis) {
      double tmp;

      tmp = resultBounds.minx;
      resultBounds.minx = resultBounds.miny;
      resultBounds.miny = tmp;

      tmp = resultBounds.maxx;
      resultBounds.maxx = resultBounds.maxy;
      resultBounds.maxy = tmp;
    }
    if (bUseURN) {
      srs = msOWSGetProjURN(&(map->projection), NULL, "FGO", MS_TRUE);
      if (!srs)
        srs = msOWSGetProjURN(&(map->projection), &(map->web.metadata), "FGO",
                              MS_TRUE);
    } else {
      msOWSGetEPSGProj(&(map->projection), NULL, "FGO", MS_TRUE, &srs);
      if (!srs)
        msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO",
                         MS_TRUE, &srs);
    }

    gmlWriteBounds(stream, outputformat, &resultBounds, srs, tab,
                   (nWFSVersion == OWS_2_0_0) ? "wfs" : "gml");
    msFree(srs);
  }
}

#endif

/*
** msGMLWriteWFSQuery()
**
** Similar to msGMLWriteQuery() but tuned for use with WFS
*/
int msGMLWriteWFSQuery(mapObj *map, FILE *stream,
                       const char *default_namespace_prefix,
                       OWSGMLVersion outputformat, int nWFSVersion, int bUseURN,
                       int bGetPropertyValueRequest) {
#ifdef USE_WFS_SVR
  int status;
  int i, j, k;
  layerObj *lp = NULL;
  shapeObj shape;

  gmlGroupListObj *groupList = NULL;
  gmlItemListObj *itemList = NULL;
  gmlConstantListObj *constantList = NULL;
  gmlGeometryListObj *geometryList = NULL;
  gmlItemObj *item = NULL;
  gmlConstantObj *constant = NULL;

  const char *namespace_prefix = NULL;
  int bSwapAxis;

  msInitShape(&shape);

  /*add a check to see if the map projection is set to be north-east*/
  bSwapAxis = msIsAxisInvertedProj(&(map->projection));

  /* Need to start with BBOX of the whole resultset */
  if (!bGetPropertyValueRequest) {
    msGMLWriteWFSBounds(map, stream, "      ", outputformat, nWFSVersion,
                        bUseURN);
  }
  /* step through the layers looking for query results */
  for (i = 0; i < map->numlayers; i++) {

    lp = GET_LAYER(map, map->layerorder[i]);

    if (lp->resultcache &&
        lp->resultcache->numresults > 0) { /* found results */
      char *layerName;
      const char *value;
      int featureIdIndex = -1; /* no feature id */
      char *srs = NULL;
      int bOutputGMLIdOnly = MS_FALSE;
      int nSRSDimension = 2;
      const char *geomtype;
      reprojectionObj *reprojector = NULL;
      int geometry_precision = 6;

      /* setup namespace, a layer can override the default */
      namespace_prefix =
          msOWSLookupMetadata(&(lp->metadata), "OFG", "namespace_prefix");
      if (!namespace_prefix)
        namespace_prefix = default_namespace_prefix;

      geomtype = msOWSLookupMetadata(&(lp->metadata), "OFG", "geomtype");
      if (geomtype != NULL && (strstr(geomtype, "25d") != NULL ||
                               strstr(geomtype, "25D") != NULL)) {
        nSRSDimension = 3;
      }

      value = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
      if (value) { /* find the featureid amongst the items for this layer */
        for (j = 0; j < lp->numitems; j++) {
          if (strcasecmp(lp->items[j], value) == 0) { /* found it */
            featureIdIndex = j;
            break;
          }
        }

        /* Produce a warning if a featureid was set but the corresponding item
         * is not found. */
        if (featureIdIndex == -1)
          msIO_fprintf(stream,
                       "<!-- WARNING: FeatureId item '%s' not found in "
                       "typename '%s'. -->\n",
                       value, lp->name);
      } else if (outputformat == OWS_GML32)
        msIO_fprintf(stream,
                     "<!-- WARNING: No featureid defined for typename '%s'. "
                     "Output will not validate. -->\n",
                     lp->name);

      /* populate item and group metadata structures */
      itemList = msGMLGetItems(lp, "G");
      constantList = msGMLGetConstants(lp, "G");
      groupList = msGMLGetGroups(lp, "G");
      geometryList = msGMLGetGeometries(lp, "GFO", MS_FALSE);
      if (itemList == NULL || constantList == NULL || groupList == NULL ||
          geometryList == NULL) {
        msSetError(MS_MISCERR,
                   "Unable to populate item and group metadata structures",
                   "msGMLWriteWFSQuery()");
        return MS_FAILURE;
      }

      if (bGetPropertyValueRequest) {
        const char *value =
            msOWSLookupMetadata(&(lp->metadata), "G", "include_items");
        if (value != NULL && strcmp(value, "@gml:id") == 0)
          bOutputGMLIdOnly = MS_TRUE;
      }

      if (namespace_prefix) {
        layerName = (char *)msSmallMalloc(strlen(namespace_prefix) +
                                          strlen(lp->name) + 2);
        sprintf(layerName, "%s:%s", namespace_prefix, lp->name);
      } else {
        layerName = msStrdup(lp->name);
      }

      if (bUseURN) {
        srs = msOWSGetProjURN(&(map->projection), NULL, "FGO", MS_TRUE);
        if (!srs)
          srs = msOWSGetProjURN(&(map->projection), &(map->web.metadata), "FGO",
                                MS_TRUE);
        if (!srs)
          srs = msOWSGetProjURN(&(lp->projection), &(lp->metadata), "FGO",
                                MS_TRUE);
      } else {
        msOWSGetEPSGProj(&(map->projection), NULL, "FGO", MS_TRUE, &srs);
        if (!srs)
          msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO",
                           MS_TRUE, &srs);
        if (!srs)
          msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE,
                           &srs);
      }

      if (msProjectionsDiffer(&(lp->projection), &(map->projection))) {
        reprojector =
            msProjectCreateReprojector(&(lp->projection), &(map->projection));
        if (reprojector == NULL) {
          msGMLFreeGroups(groupList);
          msGMLFreeConstants(constantList);
          msGMLFreeItems(itemList);
          msGMLFreeGeometries(geometryList);
          msFree(layerName);
          msFree(srs);
          return MS_FAILURE;
        }
      }

      for (j = 0; j < lp->resultcache->numresults; j++) {
        char *pszFID;

        if (lp->resultcache->results[j].shape) {
          /* msDebug("Using cached shape %ld\n",
           * lp->resultcache->results[j].shapeindex); */
          msCopyShape(lp->resultcache->results[j].shape, &shape);
        } else {
          status = msLayerGetShape(lp, &shape, &(lp->resultcache->results[j]));
          if (status != MS_SUCCESS) {
            msGMLFreeGroups(groupList);
            msGMLFreeConstants(constantList);
            msGMLFreeItems(itemList);
            msGMLFreeGeometries(geometryList);
            msFree(layerName);
            msProjectDestroyReprojector(reprojector);
            msFree(srs);
            return (status);
          }
        }

        /* project the shape into the map projection (if necessary), note that
         * this projects the bounds as well */
        if (reprojector)
          msProjectShapeEx(reprojector, &shape);

        if (featureIdIndex != -1) {
          pszFID = (char *)msSmallMalloc(
              strlen(lp->name) + 1 + strlen(shape.values[featureIdIndex]) + 1);
          sprintf(pszFID, "%s.%s", lp->name, shape.values[featureIdIndex]);
        } else
          pszFID = msStrdup("");

        if (bOutputGMLIdOnly) {
          msIO_fprintf(stream, "    <wfs:member>%s</wfs:member>\n", pszFID);
          msFree(pszFID);
          msFreeShape(&shape); /* init too */
          continue;
        }

        /*
        ** start this feature
        */
        if (nWFSVersion == OWS_2_0_0)
          msIO_fprintf(stream, "    <wfs:member>\n");
        else
          msIO_fprintf(stream, "    <gml:featureMember>\n");
        if (msIsXMLTagValid(layerName) == MS_FALSE)
          msIO_fprintf(stream,
                       "<!-- WARNING: The value '%s' is not valid in a XML tag "
                       "context. -->\n",
                       layerName);
        if (featureIdIndex != -1) {
          if (!bGetPropertyValueRequest) {
            if (outputformat == OWS_GML2)
              msIO_fprintf(stream, "      <%s fid=\"%s\">\n", layerName,
                           pszFID);
            else /* OWS_GML3 or OWS_GML32 */
              msIO_fprintf(stream, "      <%s gml:id=\"%s\">\n", layerName,
                           pszFID);
          }
        } else {
          if (!bGetPropertyValueRequest)
            msIO_fprintf(stream, "      <%s>\n", layerName);
        }

        if (bSwapAxis)
          msAxisSwapShape(&shape);

        /* write the feature geometry and bounding box */
        if (!(geometryList && geometryList->numgeometries == 1 &&
              strcasecmp(geometryList->geometries[0].name, "none") == 0)) {
          if (!bGetPropertyValueRequest)
            gmlWriteBounds(stream, outputformat, &(shape.bounds), srs,
                           "        ", "gml");

          if (msOWSLookupMetadata(&(lp->metadata), "F", "geometry_precision")) {
            geometry_precision = atoi(msOWSLookupMetadata(
                &(lp->metadata), "F", "geometry_precision"));
          } else if (msOWSLookupMetadata(&map->web.metadata, "F",
                                         "geometry_precision")) {
            geometry_precision = atoi(msOWSLookupMetadata(
                &map->web.metadata, "F", "geometry_precision"));
          }

          gmlWriteGeometry(stream, geometryList, outputformat, &(shape), srs,
                           namespace_prefix, "        ", pszFID, nSRSDimension,
                           geometry_precision);
        }

        /* write any item/values */
        for (k = 0; k < itemList->numitems; k++) {
          item = &(itemList->items[k]);
          if (msItemInGroups(item->name, groupList) == MS_FALSE)
            msGMLWriteItem(stream, item, shape.values[k], namespace_prefix,
                           "        ", outputformat, pszFID);
        }

        /* write any constants */
        for (k = 0; k < constantList->numconstants; k++) {
          constant = &(constantList->constants[k]);
          if (msItemInGroups(constant->name, groupList) == MS_FALSE)
            msGMLWriteConstant(stream, constant, namespace_prefix, "        ");
        }

        /* write any groups */
        for (k = 0; k < groupList->numgroups; k++)
          msGMLWriteGroup(stream, &(groupList->groups[k]), &shape, itemList,
                          constantList, namespace_prefix, "        ",
                          outputformat, pszFID);

        if (!bGetPropertyValueRequest)
          /* end this feature */
          msIO_fprintf(stream, "      </%s>\n", layerName);

        if (nWFSVersion == OWS_2_0_0)
          msIO_fprintf(stream, "    </wfs:member>\n");
        else
          msIO_fprintf(stream, "    </gml:featureMember>\n");

        msFree(pszFID);
        pszFID = NULL;
        msFreeShape(&shape); /* init too */
      }

      msProjectDestroyReprojector(reprojector);

      msFree(srs);

      /* done with this layer, do a little clean-up */
      msFree(layerName);

      msGMLFreeGroups(groupList);
      msGMLFreeConstants(constantList);
      msGMLFreeItems(itemList);
      msGMLFreeGeometries(geometryList);

      /* msLayerClose(lp); */
    }

  } /* next layer */

  return (MS_SUCCESS);

#else  /* Stub for mapscript */
  (void)map;
  (void)stream;
  (void)default_namespace_prefix;
  (void)outputformat;
  (void)nWFSVersion;
  (void)bUseURN;
  (void)bGetPropertyValueRequest;
  msSetError(MS_MISCERR, "WFS server support not enabled",
             "msGMLWriteWFSQuery()");
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

xmlNodePtr msGML3BoundedBy(xmlNsPtr psNs, double minx, double miny, double maxx,
                           double maxy, const char *psEpsg) {
  xmlNodePtr psNode = NULL, psSubNode = NULL;
  char *pszTmp = NULL;
  char *pszTmp2 = NULL;
  char *pszEpsg = NULL;

  psNode = xmlNewNode(psNs, BAD_CAST "boundedBy");
  psSubNode = xmlNewChild(psNode, NULL, BAD_CAST "Envelope", NULL);

  if (psEpsg) {
    const size_t bufferSize = strlen(psEpsg) + 1;
    pszEpsg = (char *)msSmallMalloc(bufferSize);
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
  pszTmp2 = msDoubleToString(maxy, MS_TRUE);
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

xmlNodePtr msGML3Point(xmlNsPtr psNs, const char *psSrsName, const char *id,
                       double x, double y) {

  xmlNodePtr psNode = xmlNewNode(psNs, BAD_CAST "Point");

  if (id) {
    xmlNewNsProp(psNode, psNs, BAD_CAST "id", BAD_CAST id);
  }

  if (psSrsName) {
    const size_t bufferSize = strlen(psSrsName) + 1;
    char *pszSrsName = (char *)msSmallMalloc(bufferSize);
    snprintf(pszSrsName, bufferSize, "%s", psSrsName);
    msStringToLower(pszSrsName);
    char *pszTmp = NULL;
    pszTmp = msStringConcatenate(pszTmp, "urn:ogc:crs:");
    pszTmp = msStringConcatenate(pszTmp, pszSrsName);
    xmlNewProp(psNode, BAD_CAST "srsName", BAD_CAST pszTmp);
    free(pszSrsName);
    free(pszTmp);
    const int dimension = 2;
    pszTmp = msIntToString(dimension);
    xmlNewProp(psNode, BAD_CAST "srsDimension", BAD_CAST pszTmp);
    free(pszTmp);
  }

  char *pszTmp = msDoubleToString(x, MS_TRUE);
  pszTmp = msStringConcatenate(pszTmp, " ");
  char *pszTmp2 = msDoubleToString(y, MS_TRUE);
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

xmlNodePtr msGML3TimePeriod(xmlNsPtr psNs, char *pszStart, char *pszEnd) {
  xmlNodePtr psNode = NULL;

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

xmlNodePtr msGML3TimeInstant(xmlNsPtr psNs, char *pszTime) {
  xmlNodePtr psNode = NULL;

  psNode = xmlNewNode(psNs, BAD_CAST "TimeInstant");
  xmlNewChild(psNode, NULL, BAD_CAST "timePosition", BAD_CAST pszTime);
  return psNode;
}

#endif /* USE_LIBXML2 */

/************************************************************************/
/*      The following functions are enabled in all cases, even if       */
/*      WMS and WFS are not available.                                  */
/************************************************************************/

gmlItemListObj *msGMLGetItems(layerObj *layer,
                              const char *metadata_namespaces) {
  int i, j;

  char **xmlitems = NULL;
  int numxmlitems = 0;

  char **incitems = NULL;
  int numincitems = 0;

  char **excitems = NULL;
  int numexcitems = 0;

  char **optionalitems = NULL;
  int numoptionalitems = 0;

  char **mandatoryitems = NULL;
  int nummandatoryitems = 0;

  char **defaultitems = NULL;
  int numdefaultitems = 0;

  const char *value = NULL;
  char tag[64];

  gmlItemListObj *itemList = NULL;
  gmlItemObj *item = NULL;

  /* get a list of items that might be included in output */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "include_items")) != NULL)
    incitems = msStringSplit(value, ',', &numincitems);

  /* get a list of items that should be excluded in output */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "exclude_items")) != NULL)
    excitems = msStringSplit(value, ',', &numexcitems);

  /* get a list of items that should not be XML encoded */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "xml_items")) != NULL)
    xmlitems = msStringSplit(value, ',', &numxmlitems);

  /* get a list of items that should be indicated as optional in
   * DescribeFeatureType */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "optional_items")) != NULL)
    optionalitems = msStringSplit(value, ',', &numoptionalitems);

  /* get a list of items that should be indicated as mandatory in
   * DescribeFeatureType */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "mandatory_items")) != NULL)
    mandatoryitems = msStringSplit(value, ',', &nummandatoryitems);

  /* get a list of items that should be presented by default in GetFeature */
  if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                   "default_items")) != NULL)
    defaultitems = msStringSplit(value, ',', &numdefaultitems);

  /* allocate memory and iinitialize the item collection */
  itemList = (gmlItemListObj *)malloc(sizeof(gmlItemListObj));
  if (itemList == NULL) {
    msSetError(MS_MEMERR, "Error allocating a collection GML item structures.",
               "msGMLGetItems()");
    return NULL;
  }

  itemList->numitems = layer->numitems;
  itemList->items =
      (gmlItemObj *)malloc(sizeof(gmlItemObj) * itemList->numitems);
  if (!itemList->items) {
    msSetError(MS_MEMERR, "Error allocating a collection GML item structures.",
               "msGMLGetItems()");
    free(itemList);
    return NULL;
  }

  for (i = 0; i < layer->numitems; i++) {
    item = &(itemList->items[i]);

    item->name = msStrdup(layer->items[i]); /* initialize the item */
    item->alias = NULL;
    item->type = NULL;
    item->template = NULL;
    item->encode = MS_TRUE;
    item->visible = MS_FALSE;
    item->width = 0;
    item->precision = 0;
    item->outputByDefault = (numdefaultitems == 0);
    item->minOccurs = 0;

    /* check visibility, included items first... */
    if (numincitems == 1 && strcasecmp("all", incitems[0]) == 0) {
      item->visible = MS_TRUE;
    } else {
      for (j = 0; j < numincitems; j++) {
        if (strcasecmp(layer->items[i], incitems[j]) == 0)
          item->visible = MS_TRUE;
      }
    }

    /* ...and now excluded items */
    for (j = 0; j < numexcitems; j++) {
      if (strcasecmp(layer->items[i], excitems[j]) == 0)
        item->visible = MS_FALSE;
    }

    /* check encoding */
    for (j = 0; j < numxmlitems; j++) {
      if (strcasecmp(layer->items[i], xmlitems[j]) == 0)
        item->encode = MS_FALSE;
    }

    /* check optional */
    if (numoptionalitems == 1 && strcasecmp("all", optionalitems[0]) == 0) {
      item->minOccurs = 0;
    } else if (numoptionalitems > 0) {
      item->minOccurs = 1;
      for (j = 0; j < numoptionalitems; j++) {
        if (strcasecmp(layer->items[i], optionalitems[j]) == 0)
          item->minOccurs = 0;
      }
    }

    /* check mandatory */
    if (nummandatoryitems == 1 && strcasecmp("all", mandatoryitems[0]) == 0) {
      item->minOccurs = 1;
    } else if (nummandatoryitems > 0) {
      item->minOccurs = 0;
      for (j = 0; j < nummandatoryitems; j++) {
        if (strcasecmp(layer->items[i], mandatoryitems[j]) == 0)
          item->minOccurs = 1;
      }
    }

    /* check default */
    for (j = 0; j < numdefaultitems; j++) {
      if (strcasecmp(layer->items[i], defaultitems[j]) == 0)
        item->outputByDefault = 1;
    }

    snprintf(tag, sizeof(tag), "%s_alias", layer->items[i]);
    if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                     tag)) != NULL)
      item->alias = msStrdup(value);

    snprintf(tag, sizeof(tag), "%s_type", layer->items[i]);
    if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                     tag)) != NULL)
      item->type = msStrdup(value);

    snprintf(tag, sizeof(tag), "%s_template", layer->items[i]);
    if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                     tag)) != NULL)
      item->template = msStrdup(value);

    snprintf(tag, sizeof(tag), "%s_width", layer->items[i]);
    if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                     tag)) != NULL)
      item->width = atoi(value);

    snprintf(tag, sizeof(tag), "%s_precision", layer->items[i]);
    if ((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces,
                                     tag)) != NULL)
      item->precision = atoi(value);
  }

  msFreeCharArray(incitems, numincitems);
  msFreeCharArray(excitems, numexcitems);
  msFreeCharArray(xmlitems, numxmlitems);
  msFreeCharArray(optionalitems, numoptionalitems);
  msFreeCharArray(mandatoryitems, nummandatoryitems);
  msFreeCharArray(defaultitems, numdefaultitems);

  return itemList;
}

void msGMLFreeItems(gmlItemListObj *itemList) {
  int i;

  if (!itemList)
    return;

  for (i = 0; i < itemList->numitems; i++) {
    msFree(itemList->items[i].name);
    msFree(itemList->items[i].alias);
    msFree(itemList->items[i].type);
    msFree(itemList->items[i].template);
  }

  if (itemList->items != NULL)
    free(itemList->items);

  free(itemList);
}
