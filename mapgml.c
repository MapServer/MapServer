/******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.96  2006/08/22 23:14:13  hobu
 * throw away the const qualifier of msOWSLookupMetadata when we set
 * the namespace_prefix which is not defined as constant
 *
 * Revision 1.95  2006/08/22 04:45:06  sdlime
 * Fixed a bug that did not allow for seperate metadata namespaces to be used for WMS vs. WFS for GML transformations (e.g. WFS_GEOMETRIES, WMS_GEOMETRIES).
 *
 * Revision 1.94  2006/08/14 20:06:32  dan
 * Produce warning in WFS GetFeature output if ???_featureid is specified
 * but corresponding item is not found in layer (bug 1781)
 *
 * Revision 1.93  2006/08/14 18:47:28  sdlime
 * Some more implementation of external application schema support.
 *
 * Revision 1.92  2006/08/11 21:32:22  sdlime
 * The GML writer for WFS now allows a layer to override the default namespace via OWS_NAMESPACE_PREFIX at the layer level.
 *
 * Revision 1.91  2006/08/11 18:56:40  sdlime
 * Initial versions of namespace read/free functions.
 *
 * Revision 1.90  2006/08/01 13:48:31  sdlime
 * Added missing / in closing tag for multipoint (GML v2.1.2).
 *
 * Revision 1.89  2006/08/01 03:09:26  sdlime
 * Added missing pointMember tag to multipoint output for GML 2.1.2. (bug 1847)
 *
 * Revision 1.88  2006/05/02 20:52:06  dan
 * Output feature id as @fid instead of @gml:id in WFS 1.0.0 / GML 2.1.2
 * GetFeature requests (bug 1759)
 *
 * Revision 1.87  2006/05/02 19:38:39  dan
 * Allow use of wms/ows_include_items and wms/ows_exclude_items to control
 * which items to output in text/plain GetFeatureInfo. (bug 1761)
 *
 * Revision 1.86  2006/04/08 05:24:41  frank
 * Fixed itemList->items memory leak.
 *
 * Revision 1.85  2006/04/08 05:16:55  frank
 * Fixed memory leak of encoded entities in msGMLWriteItem().
 *
 * Revision 1.84  2006/03/22 21:40:47  sdlime
 * Added support to allow a service provider (WFS or WMS) tonot expose feature geometries. (bug 1718)
 *
 * Revision 1.83  2006/02/16 07:49:10  sdlime
 * Removed duplicate call to create outer ring list in writeGeometry_GML2().
 *
 * Revision 1.82  2006/02/07 17:40:23  sdlime
 * Reverting to Dan's original solution for the template member.
 *
 * Revision 1.81  2006/02/07 16:13:37  sdlime
 * Renamed gmlItemObj template to tmplate to avoid c++ compile errors.
 *
 * Revision 1.80  2006/02/06 19:50:41  sdlime
 * Added ability to define a item template for GML output, best for use with application schema. A templated attribute is: 1) not queryable and 2) not output in the server produced schema. The layer namespace and item value can be accessed via the template: e.g. gml_area_template '<$namespace:area>$value</$namespace:area>
 *
 * Revision 1.79  2006/01/23 22:42:53  julien
 * Add gml:lineStringMember in GML2 MultiLineString geometry (bug 1569)
 *
 * Revision 1.78  2005/11/21 23:22:15  sdlime
 * Trivial comment fixes.
 *
 * Revision 1.77  2005/11/01 16:55:18  sdlime
 * Updated the layer loops in the GML writers to repsect layer order.
 *
 * Revision 1.76  2005/10/26 21:22:04  sdlime
 * I changed my mind and removed the option to use the shape index as an ID. I think it is better practice to use a more stable key item. This should also make supporting featureid requests easier since it's just a shortcut for a filter. Normal filter processing does not support index queries.
 *
 * Revision 1.75  2005/10/26 21:07:35  sdlime
 * WFS GML output includes an option to set a gml:id for each feature. The default is no id (current behavior), but this can be overidden using the metadata value gml_featureid, which takes an item name to use to create the id or the special value '_index' to use the shape->index. This partially addresses bug 1413.
 *
 * Revision 1.74  2005/10/25 20:29:52  sdlime
 * Completed work to add constants to GML output. For example gml_constants 'aConstant'  gml_aConstant_value 'this is a constant', which results in output like <aConstant>this is a constant</aConstant>. Constants can appear in groups and can haves pecific types (default is string). Constants are NOT queryable so their use should be limited until some extensions to wfs 1.1 appear that will allow us to mark certain elements as queryable or not in capabilities output.
 *
 * Revision 1.73  2005/10/24 21:03:03  sdlime
 * Removed unused variable geom_name from WFS GML writer.
 *
 * Revision 1.72  2005/10/24 20:52:46  sdlime
 * Moved code to generate the layer_name outside of the feature loop. It was being executed for each feature when it only needed to be run once per layer.
 *
 * Revision 1.71  2005/10/24 20:46:46  sdlime
 * Updated GML transformation metadata handling to use the msOWSLookupMetadata functions with prefixes OWS_, WFS_ and GML_. Previously only the GML_ prefix was supported.
 *
 * Revision 1.70  2005/10/12 15:34:27  sdlime
 * Updated the gmlGroupObj to allow you to set the group type. This impacts schema location. If not set the complex type written by the schema generator is 'groupnameType' and via metadata you can override that (e.g. gml_groupname_type 'MynameType').
 *
 * Revision 1.69  2005/09/04 20:20:28  sdlime
 * GML writes now support multiple ways of defining a multipoint. 1) one line string with multiple points and 2) multiplle line strings with one point each. (bug 1490)
 *
 * Revision 1.68  2005/08/01 16:34:28  sdlime
 * Fixed a problem with the GML writer not properly closing the geometry container for MultiPoints. (bug 1409).
 *
 * Revision 1.67  2005/07/06 15:18:17  dan
 * Fixed missing space in GML MultiLineString output (bug 1408)
 *
 * Revision 1.66  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.65  2005/06/07 03:34:34  sdlime
 * Ported GML 2.1.2 writer to use geometry metadata like the GML 3.1 writer.
 *
 * Revision 1.64  2005/06/01 19:51:12  sdlime
 * Updated GML output to produce features in a default container (msGeometry) if no geometry metadata is supplied.
 *
 * Revision 1.63  2005/05/31 18:24:49  sdlime
 * Updated GML3 writer to use the new gmlGeometryListObj. This allows you to package geometries from WFS in a pretty flexible manner. Will port GML2 writer once testing on GML3 code is complete.
 *
 * Revision 1.62  2005/05/31 05:49:31  sdlime
 * Added geometry metadata processing functions to mapgml.c.
 *
 * Revision 1.61  2005/05/27 17:49:14  sdlime
 * Changed dimension to srsDimension for posList in GML3 writer.
 *
 * Revision 1.60  2005/05/26 21:19:05  sdlime
 * Fixed problem with GML outout when gml_geometry_name is not set.
 *
 * Revision 1.59  2005/05/26 16:09:15  sdlime
 * Updated mapwfs.c to produce schema compliant with the GML for Simple Feature Exchange proposed standard. Changes are relatively minor with the exception of naming the geometry container and handling of mixed geometry types. The previous version defaulted to a GML type. We need more control for application specific schema. We now package the GML geometry in an element named by default geometry, users can override using gml_geometry_name metadata. We also advertise a *very* generic GMLPropertyType by default again which can be overriden using gml_geometry_type. That metadata *can* contain a list of valid types which are offered as a xsd:choice.
 *
 * Revision 1.58  2005/05/23 17:31:27  sdlime
 * Move GML metadata structures and functions prototypes to mapows.h since they will be needed by mapwfs.c as well.
 *
 * Revision 1.57  2005/04/25 06:41:56  sdlime
 * Applied Bill's newest gradient patch, more concise in the mapfile and potential to use via MapScript.
 *
 * Revision 1.56  2005/04/21 21:10:38  sdlime
 * Adjusted WFS support to allow for a new output format (GML3).
 *
 * Revision 1.55  2005/04/21 04:46:35  sdlime
 * Fixed a small problem with last commit.
 *
 * Revision 1.54  2005/04/20 21:40:44  sdlime
 * Bug 950 transformations now apply to WMS output as well. You must explicitly request that items be exposed or set gml_include_items 'all'.
 *
 * Revision 1.53  2005/04/14 16:51:07  frank
 * Removed unused tab in printf.
 *
 * Revision 1.52  2005/04/01 23:02:45  frank
 * Avoid emitting extra whitespace in the gml:coordinates for the gml:Box.
 * Related to bug 1304.
 *
 * Revision 1.51  2005/03/25 20:39:24  sdlime
 * Fixed problem in WFS GML writer. Memory was being free'd in the wrong place. Should've been inside the if-then statement that tests if a layer is valid for WFS output.
 *
 * Revision 1.50  2005/03/16 17:19:30  sdlime
 * Updated GML group reading code to explicitly initialize all structure members.
 *
 * Revision 1.49  2005/02/18 19:15:23  sdlime
 * Added GML item-level transformation support for WFS. Includes inclusion/exclusion, column aliases and simple groups. (bug 950)
 *
 * Revision 1.48  2005/02/18 03:06:45  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.47  2004/12/29 22:49:57  sdlime
 * Added GML3 writing capabilities to mapgml.c. Not hooked up to anything yet.
 *
 * Revision 1.46  2004/12/14 21:30:43  sdlime
 * Moved functions to build lists of inner and outer rings to mapprimitive.c from mapgml.c. They are needed to covert between MapServer polygons and GEOS gemometries (bug 771).
 *
 * Revision 1.45  2004/11/16 21:57:49  dan
 * Final pass at updating WMS/WFS client/server interfaces to lookup "ows_*"
 * metadata in addition to default "wms_*"/"wfs_*" metadata (bug 568)
 *
 * Revision 1.44  2004/11/09 16:00:34  sean
 * move conditional compilation inside definition of msGMLWriteQuery and
 * msGMLWriteWFSQuery.  if OWS macros aren't defined, these functions set an
 * error and return MS_FAILURE (bug 1046).
 *
 * Revision 1.43  2004/10/28 18:54:05  dan
 * USe OWS_NOERR instead of MS_NOERR in calls to msOWSPrint*()
 *
 * Revision 1.42  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"
#include "maperror.h"

MS_CVSID("$Id$")

/* Use only mapgml.c if WMS or WFS is available */

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

  msIO_fprintf(stream, "%s\t\t<gml:pos>%.6f %.6f</gml:pos>\n", tab, rect->minx, rect->miny);
  msIO_fprintf(stream, "%s\t\t<gml:pos>%.6f %.6f</gml:pos>\n", tab, rect->maxx, rect->maxy);
  
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
  geometryList->geometries = NULL;
  geometryList->numgeometries = 0;

  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "geometries")) != NULL) {
    names = split(value, ',', &numnames);

    /* allocation an array of gmlGeometryObj's */
    geometryList->numgeometries = numnames;
    geometryList->geometries = (gmlGeometryObj *) malloc(sizeof(gmlGeometryObj)*geometryList->numgeometries);

    for(i=0; i<geometryList->numgeometries; i++) {
      geometry = &(geometryList->geometries[i]);

      geometry->name = strdup(names[i]); /* initialize a few things */
      geometry->type = NULL;
      geometry->occurmin = 0;
      geometry->occurmax = 1;      

      snprintf(tag, 64, "%s_type", names[i]);
      if((value =  msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
        geometry->type = strdup(value); /* TODO: validate input value */

      snprintf(tag, 64, "%s_occurances", names[i]);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) {
        char **occur;
        int numoccur;

        occur = split(value, ',', &numoccur);
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

  free(geometryList);
}

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
    incitems = split(value, ',', &numincitems);

  /* get a list of items that should be excluded in output */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "exclude_items")) != NULL)  
    excitems = split(value, ',', &numexcitems);

  /* get a list of items that need don't get encoded */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "xml_items")) != NULL)  
    xmlitems = split(value, ',', &numxmlitems);

  /* allocate memory and iinitialize the item collection */
  itemList = (gmlItemListObj *) malloc(sizeof(gmlItemListObj));
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

    item->name = strdup(layer->items[i]);  /* initialize the item */
    item->alias = NULL;
    item->type = NULL;
    item->template = NULL;
    item->encode = MS_TRUE;
    item->visible = MS_FALSE;

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

    snprintf(tag, 64, "%s_alias", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) 
      item->alias = strdup(value);

    snprintf(tag, 64, "%s_type", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) 
      item->type = strdup(value);

    snprintf(tag, 64, "%s_template", layer->items[i]);
    if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) 
      item->template = strdup(value);
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
    encoded_value = strdup(value);  
  
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

    tag = strdup(item->template);
    tag = gsub(tag, "$value", encoded_value);
    if(namespace) tag = gsub(tag, "$namespace", namespace);
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
  namespaceList->namespaces = NULL;
  namespaceList->numnamespaces = 0; 

  /* list of constants (TODO: make this automatic by parsing metadata) */
  if((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces, "external_namespace_prefixes")) != NULL) {
    prefixes = split(value, ',', &numprefixes);

    /* allocation an array of gmlNamespaceObj's */
    namespaceList->numnamespaces = numprefixes;
    namespaceList->namespaces = (gmlNamespaceObj *) malloc(sizeof(gmlNamespaceObj)*namespaceList->numnamespaces);

    for(i=0; i<namespaceList->numnamespaces; i++) {
      namespace = &(namespaceList->namespaces[i]);

      namespace->prefix = strdup(prefixes[i]); /* initialize a few things */      
      namespace->uri = NULL;
      namespace->schemalocation = NULL;

      snprintf(tag, 64, "%s_uri", namespace->prefix);
      if((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces, tag)) != NULL) 
      namespace->uri = strdup(value);

      snprintf(tag, 64, "%s_schema_location", namespace->prefix);
      if((value = msOWSLookupMetadata(&(web->metadata), metadata_namespaces, tag)) != NULL) 
      namespace->schemalocation = strdup(value);
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
  constantList->constants = NULL;
  constantList->numconstants = 0;

  /* list of constants (TODO: make this automatic by parsing metadata) */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "constants")) != NULL) {
    names = split(value, ',', &numnames);

    /* allocation an array of gmlConstantObj's */
    constantList->numconstants = numnames;
    constantList->constants = (gmlConstantObj *) malloc(sizeof(gmlConstantObj)*constantList->numconstants);

    for(i=0; i<constantList->numconstants; i++) {
      constant = &(constantList->constants[i]);

      constant->name = strdup(names[i]); /* initialize a few things */      
      constant->value = NULL;
      constant->type = NULL;

      snprintf(tag, 64, "%s_value", constant->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) 
      constant->value = strdup(value);

      snprintf(tag, 64, "%s_type", constant->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) 
      constant->type = strdup(value);
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
  groupList->groups = NULL;
  groupList->numgroups = 0;

  /* list of groups (TODO: make this automatic by parsing metadata) */
  if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, "groups")) != NULL) {
    names = split(value, ',', &numnames);

    /* allocation an array of gmlGroupObj's */
    groupList->numgroups = numnames;
    groupList->groups = (gmlGroupObj *) malloc(sizeof(gmlGroupObj)*groupList->numgroups);

    for(i=0; i<groupList->numgroups; i++) {
      group = &(groupList->groups[i]);

      group->name = strdup(names[i]); /* initialize a few things */
      group->items = NULL;
      group->numitems = 0;
      group->type = NULL;

      snprintf(tag, 64, "%s_group", group->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL)
        group->items = split(value, ',', &group->numitems);

      snprintf(tag, 64, "%s_type", group->name);
      if((value = msOWSLookupMetadata(&(layer->metadata), metadata_namespaces, tag)) != NULL) 
      group->type = strdup(value);
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
  itemtab = (char *) malloc(sizeof(char)*strlen(tab)+3);
  if(!itemtab) return;
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

  /* step through the layers looking for query results */
  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[map->layerorder[i]]);

    if(lp->dump == MS_TRUE && lp->resultcache && lp->resultcache->numresults > 0) { /* found results */

      /* start this collection (layer) */
      /* if no layer name provided fall back on the layer name + "_layer" */
      value = (char*) malloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "layername", OWS_NOERR, "\t<%s>\n", value);
      msFree(value);

      /* actually open the layer */
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(status);

      /* retrieve all the item names */
      status = msLayerGetItems(lp);
      if(status != MS_SUCCESS) return(status);

      /* populate item and group metadata structures (TODO: test for NULLs here, shouldn't happen) */
      itemList = msGMLGetItems(lp, namespaces);
      constantList = msGMLGetConstants(lp, namespaces);
      groupList = msGMLGetGroups(lp, namespaces);      
      geometryList = msGMLGetGeometries(lp, namespaces);

      for(j=0; j<lp->resultcache->numresults; j++) {
        status = msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex);
        if(status != MS_SUCCESS) return(status);

#ifdef USE_PROJ
        /* project the shape into the map projection (if necessary), note that this projects the bounds as well */
        if(msProjectionsDiffer(&(lp->projection), &(map->projection)))
          msProjectShape(&lp->projection, &map->projection, &shape);
#endif

        /* start this feature */
        /* specify a feature name, if nothing provided fall back on the layer name + "_feature" */
        value = (char*) malloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "featurename", OWS_NOERR, "\t\t<%s>\n", value);
        msFree(value);

        /* write the feature geometry and bounding box */
        if(!(geometryList && geometryList->numgeometries == 1 && strcasecmp(geometryList->geometries[0].name, "none") == 0)) {

#ifdef USE_PROJ
          if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE)) { /* use the map projection first */
            gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE), "\t\t\t");
            gmlWriteGeometry(stream, NULL, OWS_GML2, &(shape), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE), NULL, "\t\t\t");
          } else { /* then use the layer projection and/or metadata */
            gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), namespaces, MS_TRUE), "\t\t\t");
            gmlWriteGeometry(stream, NULL, OWS_GML2, &(shape), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), namespaces, MS_TRUE), NULL, "\t\t\t");
          }
#else
          gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), NULL, "\t\t\t"); /* no projection information */
          gmlWriteGeometry(stream, NULL, OWS_GML2, &(shape), NULL, NULL, "\t\t\t");
#endif

        }

        /* write the item/values */
        for(k=0; k<itemList->numitems; k++) {
          item = &(itemList->items[k]);  
          if(msItemInGroups(item->name, groupList) == MS_FALSE) 
            msGMLWriteItem(stream, item, shape.values[k], NULL, "\t\t\t");
        }

        /* write the constants */
        for(k=0; k<constantList->numconstants; k++) {
          constant = &(constantList->constants[k]);  
          if(msItemInGroups(constant->name, groupList) == MS_FALSE) 
            msGMLWriteConstant(stream, constant, NULL, "\t\t\t");
        }

        /* write the groups */
        for(k=0; k<groupList->numgroups; k++)
          msGMLWriteGroup(stream, &(groupList->groups[k]), &shape, itemList, constantList, NULL, "\t\t\t");

        /* end this feature */
        /* specify a feature name if nothing provided */
        /* fall back on the layer name + "Feature" */
        value = (char*) malloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "featurename", OWS_NOERR, "\t\t</%s>\n", value);
        msFree(value);

        msFreeShape(&shape); /* init too */
      }

      /* end this collection (layer) */
      /* if no layer name provided fall back on the layer name + "_layer" */
      value = (char*) malloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, "layername", OWS_NOERR, "\t</%s>\n", value);
      msFree(value);

      msGMLFreeGroups(groupList);
      msGMLFreeConstants(constantList);
      msGMLFreeItems(itemList);
      msGMLFreeGeometries(geometryList);

      msLayerClose(lp);
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

/*
** msGMLWriteWFSQuery()
**
** Similar to msGMLWriteQuery() but tuned for use with WFS 1.0.0
*/
int msGMLWriteWFSQuery(mapObj *map, FILE *stream, int maxfeatures, char *default_namespace_prefix, int outputformat)
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

  msInitShape(&shape);

  /* Need to start with BBOX of the whole resultset */
  if (msGetQueryResultBounds(map, &resultBounds) > 0)
    gmlWriteBounds(stream, outputformat, &resultBounds, msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), "      ");

  /* step through the layers looking for query results */
  for(i=0; i<map->numlayers; i++) {

    lp = &(map->layers[map->layerorder[i]]);

    if(lp->dump == MS_TRUE && lp->resultcache && lp->resultcache->numresults > 0)  { /* found results */
      char *layerName;      
      const char *value;
      int featureIdIndex=-1; /* no feature id */

      /* actually open the layer */
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(status);

      /* retrieve all the item names. (Note : there might be no attributes) */
      status = msLayerGetItems(lp);
      /* if(status != MS_SUCCESS) return(status); */

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

        /* Produce a warning if a featureid was set but the corresponding
         * item is not found. 
         */
        if (featureIdIndex == -1) {
            msIO_fprintf(stream, "<!-- WARNING: FeatureId item '%s' not found in typename '%s'. -->\n", value, lp->name);
        }
      }

      /* populate item and group metadata structures (TODO: test for NULLs here, shouldn't happen) */
      itemList = msGMLGetItems(lp, "OFG");
      constantList = msGMLGetConstants(lp, "OFG");
      groupList = msGMLGetGroups(lp, "OFG");
      geometryList = msGMLGetGeometries(lp, "OFG");

      /* set the layer name */
      /* value = msOWSLookupMetadata(&(lp->metadata), "OFG", "layername");
         if(!value) value = lp->name; */
      if (namespace_prefix) {
        layerName = (char *) malloc(strlen(namespace_prefix)+strlen(lp->name)+2);
        sprintf(layerName, "%s:%s", namespace_prefix, lp->name);
      } else {
        layerName = strdup(lp->name);
      }

      for(j=0; j<lp->resultcache->numresults; j++) {
        status = msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex);
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
        if(featureIdIndex != -1) 
        {
            if (outputformat == OWS_GML2)
                msIO_fprintf(stream, "      <%s fid=\"%s\">\n", layerName, shape.values[featureIdIndex]);
            else  /* OWS_GML3 */
                msIO_fprintf(stream, "      <%s gml:id=\"%s\">\n", layerName, shape.values[featureIdIndex]);
        }
        else
          msIO_fprintf(stream, "      <%s>\n", layerName);
                    
        /* write the feature geometry and bounding box */
        if(!(geometryList && geometryList->numgeometries == 1 && strcasecmp(geometryList->geometries[0].name, "none") == 0)) {

#ifdef USE_PROJ
          if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE)) { /* use the map projection first*/
            gmlWriteBounds(stream, outputformat, &(shape.bounds), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), "        ");
            gmlWriteGeometry(stream, geometryList, outputformat, &(shape), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), namespace_prefix, "        "); 
          } else { /* then use the layer projection and/or metadata */
            gmlWriteBounds(stream, outputformat, &(shape.bounds), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), "        ");
            gmlWriteGeometry(stream, geometryList, outputformat, &(shape), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), namespace_prefix, "        ");
          }
#else
          gmlWriteBounds(stream, outputformat, &(shape.bounds), NULL, "        "); /* no projection information */
          gmlWriteGeometry(stream, geometryList, outputformat, &(shape), NULL, namespace_prefix, "        ");
#endif
        }

        /* write the item/values */
        for(k=0; k<itemList->numitems; k++) {
          item = &(itemList->items[k]);  
          if(msItemInGroups(item->name, groupList) == MS_FALSE) 
            msGMLWriteItem(stream, item, shape.values[k], namespace_prefix, "        ");
        }

        /* write the constants */
        for(k=0; k<constantList->numconstants; k++) {
          constant = &(constantList->constants[k]);  
          if(msItemInGroups(constant->name, groupList) == MS_FALSE) 
            msGMLWriteConstant(stream, constant, namespace_prefix, "        ");
        }

        /* write the groups */
        for(k=0; k<groupList->numgroups; k++)
          msGMLWriteGroup(stream, &(groupList->groups[k]), &shape, itemList, constantList, namespace_prefix, "        ");

        /* end this feature */
        msIO_fprintf(stream, "      </%s>\n", layerName);
        msIO_fprintf(stream, "    </gml:featureMember>\n");

        msFreeShape(&shape); /* init too */

        features++;
        if (maxfeatures > 0 && features == maxfeatures)
          break; 
      }      

      /* done with this layer, do a little clean-up */      
      msFree(layerName);

      msGMLFreeGroups(groupList);
      msGMLFreeConstants(constantList);
      msGMLFreeItems(itemList);
      msGMLFreeGeometries(geometryList);

      msLayerClose(lp);
    }

    if (maxfeatures > 0 && features == maxfeatures)
      break;

  } /* next layer */

  return(MS_SUCCESS);

#else /* Stub for mapscript */
  msSetError(MS_MISCERR, "WMS server support not enabled", "msGMLWriteWFSQuery()");
  return MS_FAILURE;
#endif /* USE_WFS_SVR */
}
