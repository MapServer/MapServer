/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  shapeObj to GML output.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2004 Regents of the University of Minnesota.
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

// Use only mapgml.c if WMS or WFS is available

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)

// function that writes the feature boundary geometry (i.e. a rectObj)
static int gmlWriteBounds(FILE *stream, rectObj *rect, const char *srsname, char *tab)
{ 
  char *srsname_encoded;

  if(!stream) return(MS_FAILURE);
  if(!rect) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  msIO_fprintf(stream, "%s<gml:boundedBy>\n", tab);
  if(srsname)
  {
    srsname_encoded = msEncodeHTMLEntities(srsname);
    msIO_fprintf(stream, "%s\t<gml:Box srsName=\"%s\">\n", tab, srsname_encoded);
    msFree(srsname_encoded);
  }
  else
    msIO_fprintf(stream, "%s\t<gml:Box>\n", tab);
  msIO_fprintf(stream, "%s\t\t<gml:coordinates>\n", tab);  
  msIO_fprintf(stream, "%s\t\t\t%.6f,%.6f %.6f,%.6f\n", tab, rect->minx, rect->miny, rect->maxx, rect->maxy );   
  msIO_fprintf(stream, "%s\t\t</gml:coordinates>\n", tab);
  msIO_fprintf(stream, "%s\t</gml:Box>\n", tab);
  msIO_fprintf(stream, "%s</gml:boundedBy>\n", tab);
  return MS_SUCCESS;
}

// function only writes the feature geometry for a shapeObj
static int gmlWriteGeometry(FILE *stream, shapeObj *shape, const char *srsname, char *tab) 
{
  int i, j, k;
  int *innerlist, *outerlist, numouters;
  char *srsname_encoded = NULL;

  if(!stream) return(MS_FAILURE);
  if(!shape) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  if(shape->numlines <= 0) return(MS_SUCCESS); // empty shape, nothing to output

  if(srsname)
      srsname_encoded = msEncodeHTMLEntities( srsname );

  // feature geometry
  switch(shape->type) {
  case(MS_SHAPE_POINT):
    if(shape->line[0].numpoints > 1) { // write a MultiPoint
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiPoint srsName=\"%s\">\n", tab, 
                srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiPoint>\n", tab);

      for(i=0; i<shape->line[0].numpoints; i++) {
        msIO_fprintf(stream, "%s\t<gml:Point>\n", tab);
        msIO_fprintf(stream, "%s\t\t<gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[0].point[i].x, shape->line[0].point[i].y);
        msIO_fprintf(stream, "%s\t</gml:Point>\n", tab);
      }

      msIO_fprintf(stream, "%s</gml:MultiPoint>\n", tab);
    } else { // write a Point
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:Point srsName=\"%s\">\n", tab, 
                srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:Point>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[0].point[0].x, shape->line[0].point[0].y);

      msIO_fprintf(stream, "%s</gml:Point>\n", tab);
    }
    break;
  case(MS_SHAPE_LINE):
    if(shape->numlines == 1) { // write a LineString
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:LineString srsName=\"%s\">\n", tab, 
                srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:LineString>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:coordinates>", tab);
      for(j=0; j<shape->line[0].numpoints-1; j++)
        msIO_fprintf(stream, "%f,%f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
/* -------------------------------------------------------------------- */
/*      Adding a tab at the end of the coordinates string does          */
/*      create a bug when reading the the coordinates using ogr         */
/*      (function ParseGMLCoordinates in gml2ogrgeometry.cpp).          */
/* -------------------------------------------------------------------- */
      msIO_fprintf(stream, "</gml:coordinates>\n");

      msIO_fprintf(stream, "%s</gml:LineString>\n", tab);
    } else { // write a MultiLineString      
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiLineString srsName=\"%s\">\n", tab, 
                srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiLineString>\n", tab);

      for(i=0; i<shape->numlines; i++) {
        msIO_fprintf(stream, "%s\t<gml:LineString>\n", tab); // no srsname at this point

        msIO_fprintf(stream, "%s\t\t<gml:coordinates>", tab);
        for(j=0; j<shape->line[i].numpoints; j++)
	  msIO_fprintf(stream, "%f,%f", shape->line[i].point[j].x, shape->line[i].point[j].y);
        msIO_fprintf(stream, "</gml:coordinates>\n");
        msIO_fprintf(stream, "%s\t</gml:LineString>\n", tab);
      }

      msIO_fprintf(stream, "%s</gml:MultiLineString>\n", tab);
    }
    break;
  case(MS_SHAPE_POLYGON): // this gets nasty, since our shapes are so flexible
    if(shape->numlines == 1) { // write a Polygon (no interior rings)
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, 
                srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:outerBoundaryIs>\n", tab);
      msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

      msIO_fprintf(stream, "%s\t\t\t<gml:coordinates>\n", tab);
      msIO_fprintf(stream, "%s\t\t\t\t", tab);
      for(j=0; j<shape->line[0].numpoints; j++)
	msIO_fprintf(stream, "%f,%f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
      //msIO_fprintf(stream, "\n");
      // msIO_fprintf(stream, "%s\t\t\t</gml:coordinates>\n", tab);
      msIO_fprintf(stream, "</gml:coordinates>\n");

      msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
      msIO_fprintf(stream, "%s\t</gml:outerBoundaryIs>\n", tab);

      msIO_fprintf(stream, "%s</gml:Polygon>\n", tab);
    } else { // need to test for inner and outer rings
      outerlist = msGetOuterList(shape);

      numouters = 0;
      for(i=0; i<shape->numlines; i++)
        if(outerlist[i] == MS_TRUE) numouters++;

      if(numouters == 1) { // write a Polygon (with interior rings)
	for(i=0; i<shape->numlines; i++) // find the outer ring
          if(outerlist[i] == MS_TRUE) break;

	innerlist = msGetInnerList(shape, i, outerlist);

	if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, 
                  srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

        msIO_fprintf(stream, "%s\t<gml:outerBoundaryIs>\n", tab);
        msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

        msIO_fprintf(stream, "%s\t\t\t<gml:coordinates>\n", tab);
        msIO_fprintf(stream, "%s\t\t\t\t", tab);
        for(j=0; j<shape->line[i].numpoints; j++)
	  msIO_fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
        //msIO_fprintf(stream, "\n");
        //msIO_fprintf(stream, "%s\t\t\t</gml:coordinates>\n", tab);
        msIO_fprintf(stream, "</gml:coordinates>\n");

        msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
        msIO_fprintf(stream, "%s\t</gml:outerBoundaryIs>\n", tab);

	for(k=0; k<shape->numlines; k++) { // now step through all the inner rings
	  if(innerlist[k] == MS_TRUE) {
	    msIO_fprintf(stream, "%s\t<gml:innerBoundaryIs>\n", tab);
            msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s\t\t\t<gml:coordinates>\n", tab);
            msIO_fprintf(stream, "%s\t\t\t\t", tab);
            for(j=0; j<shape->line[k].numpoints; j++)
	      msIO_fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
            //msIO_fprintf(stream, "\n");
            //msIO_fprintf(stream, "%s\t\t\t</gml:coordinates>\n", tab);
            msIO_fprintf(stream, "</gml:coordinates>\n");

            msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s\t</gml:innerBoundaryIs>\n", tab);
          }
        }

        msIO_fprintf(stream, "%s</gml:Polygon>\n", tab);
	free(innerlist);
      } else {  // write a MultiPolygon	
	if(srsname_encoded)
        {
          msIO_fprintf(stream, "%s<gml:MultiPolygon srsName=\"%s\">\n", tab, 
                  srsname_encoded);
        }
        else
          msIO_fprintf(stream, "%s<gml:MultiPolygon>\n", tab);
        
	for(i=0; i<shape->numlines; i++) { // step through the outer rings
          if(outerlist[i] == MS_TRUE) {
  	    innerlist = msGetInnerList(shape, i, outerlist);

            msIO_fprintf(stream, "%s<gml:polygonMember>\n", tab);

            msIO_fprintf(stream, "%s\t<gml:Polygon>\n", tab);

            msIO_fprintf(stream, "%s\t\t<gml:outerBoundaryIs>\n", tab);
            msIO_fprintf(stream, "%s\t\t\t<gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s\t\t\t\t<gml:coordinates>\n", tab);
            msIO_fprintf(stream, "%s\t\t\t\t\t", tab);
            for(j=0; j<shape->line[i].numpoints; j++)
	      msIO_fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
            //msIO_fprintf(stream, "\n");
            //msIO_fprintf(stream, "%s\t\t\t\t</gml:coordinates>\n", tab);
            msIO_fprintf(stream, "</gml:coordinates>\n");

            msIO_fprintf(stream, "%s\t\t\t</gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s\t\t</gml:outerBoundaryIs>\n", tab);

	    for(k=0; k<shape->numlines; k++) { // now step through all the inner rings
	      if(innerlist[k] == MS_TRUE) {
	        msIO_fprintf(stream, "%s\t\t<gml:innerBoundaryIs>\n", tab);
                msIO_fprintf(stream, "%s\t\t\t<gml:LinearRing>\n", tab);

                msIO_fprintf(stream, "%s\t\t\t\t<gml:coordinates>\n", tab);
                msIO_fprintf(stream, "%s\t\t\t\t\t", tab);
                for(j=0; j<shape->line[k].numpoints; j++)
	          msIO_fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
                //msIO_fprintf(stream, "\n");
                //msIO_fprintf(stream, "%s\t\t\t\t</gml:coordinates>\n", tab);
                msIO_fprintf(stream, "</gml:coordinates>\n");


                msIO_fprintf(stream, "%s\t\t\t</gml:LinearRing>\n", tab);
                msIO_fprintf(stream, "%s\t\t</gml:innerBoundaryIs>\n", tab);
              }
            }

            msIO_fprintf(stream, "%s\t</gml:Polygon>\n", tab);
            msIO_fprintf(stream, "%s</gml:polygonMember>\n", tab);
	
	    free(innerlist);
          }
        }

        msIO_fprintf(stream, "%s</gml:MultiPolygon>\n", tab);
      }
      free(outerlist);
    }
    break;
  default:
    break;
  }

  msFree(srsname_encoded);

  return(MS_SUCCESS);
}

#endif

int msGMLWriteQuery(mapObj *map, char *filename, const char *namespaces)
{
#if defined(USE_WMS_SVR)
  int status;
  int i,j,k;
  layerObj *lp=NULL;
  shapeObj shape;
  FILE *stream=stdout; // defaults to stdout
  char szPath[MS_MAXPATHLEN];
  char *value;

  msInitShape(&shape);

  if(filename && strlen(filename) > 0) { // deal with the filename if present
    stream = fopen(msBuildPath(szPath, map->mappath, filename), "w");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msGMLWriteQuery()", filename);
      return(MS_FAILURE);
    }
  }

  // charset encoding: lookup "gml_encoding" metadata first, then 
  // "wms_encoding", and if not found then use "ISO-8859-1" as default.
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "encoding", 
                      OWS_NOERR, "<?xml version=\"1.0\" encoding=\"%s\"?>\n\n", 
                           "ISO-8859-1");

  msOWSPrintValidateMetadata(stream, &(map->web.metadata),namespaces, "rootname",
                      OWS_NOERR, "<%s ", "msGMLOutput");

  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "uri", 
                      OWS_NOERR, "xmlns=\"%s\"", NULL);
  msIO_fprintf(stream, "\n\t xmlns:gml=\"http://www.opengis.net/gml\"" );
  msIO_fprintf(stream, "\n\t xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
  msIO_fprintf(stream, "\n\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "schema", 
                      OWS_NOERR, "\n\t xsi:schemaLocation=\"%s\"", NULL);
  msIO_fprintf(stream, ">\n");

  // a schema *should* be required
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, 
                           "description", OWS_NOERR, 
                           "\t<gml:description>%s</gml:description>\n", NULL);

  // step through the layers looking for query results
  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    if(lp->dump == MS_TRUE && lp->resultcache && lp->resultcache->numresults > 0) { // found results

      // start this collection (layer)
      // if no layer name provided 
      // fall back on the layer name + "Layer"
      value = (char*) malloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                 "layername", OWS_NOERR, "\t<%s>\n", value);
      msFree(value);

      // actually open the layer
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(status);

      // retrieve all the item names
      status = msLayerGetItems(lp);
      if(status != MS_SUCCESS) return(status);

      for(j=0; j<lp->resultcache->numresults; j++) {
	status = msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex);
        if(status != MS_SUCCESS) return(status);

#ifdef USE_PROJ
	// project the shape into the map projection (if necessary), note that this projects the bounds as well
        if(msProjectionsDiffer(&(lp->projection), &(map->projection)))
          msProjectShape(&lp->projection, &map->projection, &shape);
#endif

	// start this feature
        // specify a feature name, if nothing provided 
        // fall back on the layer name + "Feature"
        value = (char*) malloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                   "featurename", OWS_NOERR, 
                                   "\t\t<%s>\n", value);
        msFree(value);

	// write the item/values
	for(k=0; k<lp->numitems; k++)	
        {
          char *encoded_val;

          if(msIsXMLTagValid(lp->items[k]) == MS_FALSE)
              msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid in a "
                      "XML tag context. -->\n", lp->items[k]);

          encoded_val = msEncodeHTMLEntities(shape.values[k]);
	  msIO_fprintf(stream, "\t\t\t<%s>%s</%s>\n", lp->items[k], encoded_val, 
                  lp->items[k]);
          free(encoded_val);
        }

	// write the bounding box
#ifdef USE_PROJ
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE)) // use the map projection first
	  gmlWriteBounds(stream, &(shape.bounds), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE), "\t\t\t");
	else // then use the layer projection and/or metadata
	  gmlWriteBounds(stream, &(shape.bounds), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), namespaces, MS_TRUE), "\t\t\t");	
#else
	gmlWriteBounds(stream, &(shape.bounds), NULL, "\t\t\t"); // no projection information
#endif

	// write the feature geometry
#ifdef USE_PROJ
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE)) // use the map projection first
	  gmlWriteGeometry(stream, &(shape), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE), "\t\t\t");
        else // then use the layer projection and/or metadata
	  gmlWriteGeometry(stream, &(shape), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), namespaces, MS_TRUE), "\t\t\t");      
#else
	gmlWriteGeometry(stream, &(shape), NULL, "\t\t\t");
#endif

	// end this feature
        // specify a feature name if nothing provided
        // fall back on the layer name + "Feature"
        value = (char*) malloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                   "featurename", OWS_NOERR, 
                                   "\t\t</%s>\n", value);
        msFree(value);

	msFreeShape(&shape); // init too
      }

      // end this collection (layer)
      // if no layer name provided 
      // fall back on the layer name + "Layer"
      value = (char*) malloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                 "layername", OWS_NOERR, "\t</%s>\n", value);
      msFree(value);

      msLayerClose(lp);
    }
  } // next layer

  // end this document
  msOWSPrintValidateMetadata(stream, &(map->web.metadata), namespaces, "rootname",
                             OWS_NOERR, "</%s>\n", "msGMLOutput");

  if(filename && strlen(filename) > 0) fclose(stream);

  return(MS_SUCCESS);

#else /* Stub for mapscript */
    msSetError(MS_MISCERR, "WMS server support not enabled",
               "msGMLWriteQuery()");
    return MS_FAILURE;
#endif
}

/*
** msGMLWriteWFSQuery()
**
** Similar to msGMLWriteQuery() but tuned for use with WFS 1.0.0
*/

int msGMLWriteWFSQuery(mapObj *map, FILE *stream, int maxfeatures, 
                       char *wfs_namespace)
{
#ifdef USE_WFS_SVR
  int status;
  int i,j,k;
  layerObj *lp=NULL;
  shapeObj shape;
  rectObj  resultBounds = {-1.0,-1.0,-1.0,-1.0};
  int features = 0;
  char *name_gml = NULL;
  char *description_gml = NULL;

  msInitShape(&shape);

  // Need to start with BBOX of the whole resultset
  if (msGetQueryResultBounds(map, &resultBounds) > 0)
  {
      gmlWriteBounds(stream, &resultBounds, 
                     msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), 
                     "      ");
  }

  // step through the layers looking for query results
  for(i=0; i<map->numlayers; i++) 
  {
    lp = &(map->layers[i]);

    if(lp->dump == MS_TRUE && 
       lp->resultcache && lp->resultcache->numresults > 0) 
    { // found results
      int   *item_is_xml = NULL;
      const char *geom_name, *xml_items;
      char *layer_name;
      geom_name = msWFSGetGeomElementName(map, lp);

      // actually open the layer
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(status);

      // retrieve all the item names. (Note : there might be no attributs)
      status = msLayerGetItems(lp);
      //if(status != MS_SUCCESS) return(status);
      
      /* 
      ** Determine which items, if any, should be treated as raw XML and not 
      ** escaped.
      */
      item_is_xml = (int *) calloc(sizeof(int),lp->numitems);
      xml_items = msLookupHashTable(&(lp->metadata), "wfs_gml_xml_items");
      if( xml_items != NULL )
      {
          int xml_items_count = 0;
          char **xml_item_list = split( xml_items, ',', &xml_items_count );
          
          for( k = 0; k < lp->numitems; k++ )
          {
              int xi;

              for( xi = 0; xi < xml_items_count; xi++ )
              {
                  if( strcmp(lp->items[k],xml_item_list[xi]) == 0 )
                      item_is_xml[k] = MS_TRUE;
              }
          }
          
          msFreeCharArray( xml_item_list, xml_items_count );
      }

      for(j=0; j<lp->resultcache->numresults; j++) 
      {
	status = msLayerGetShape(lp, &shape, 
                                 lp->resultcache->results[j].tileindex, 
                                 lp->resultcache->results[j].shapeindex);
        if(status != MS_SUCCESS) return(status);

#ifdef USE_PROJ
	// project the shape into the map projection (if necessary), note that this projects the bounds as well
        if(msProjectionsDiffer(&(lp->projection), &(map->projection)))
          msProjectShape(&lp->projection, &map->projection, &shape);
#endif
        
	// start this feature
        if (wfs_namespace)
        {
            layer_name = (char*) malloc(strlen(wfs_namespace)+strlen(lp->name)+2);
            sprintf(layer_name, "%s:%s", wfs_namespace, lp->name);
        }
        else
            layer_name = strdup(lp->name);

	msIO_fprintf(stream, "    <gml:featureMember>\n");
        if(msIsXMLTagValid(layer_name) == MS_FALSE)
            msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid "
                    "in a XML tag context. -->\n", layer_name);
        msIO_fprintf(stream, "      <%s>\n", layer_name);

        //write name and description attributs if specified in the map file
        
        description_gml = msLookupHashTable(&(lp->metadata), "wfs_gml_description_item");
        if (description_gml)
        {
            for(k=0; k<lp->numitems; k++)	
            {
                if (strcasecmp(lp->items[k], description_gml) == 0)
                {
                    msIO_fprintf(stream, "      <gml:description>%s</gml:description>\n",  
                            msEncodeHTMLEntities(shape.values[k]));
                    break;
                 }
             }
         }
        name_gml = msLookupHashTable(&(lp->metadata), "wfs_gml_name_item");
        if (name_gml)
         {
             for(k=0; k<lp->numitems; k++)	
             {
                 if (strcasecmp(lp->items[k], name_gml) == 0)
                 {
                     msIO_fprintf(stream, "      <gml:name>%s</gml:name>\n",  
                             msEncodeHTMLEntities(shape.values[k]));
                     break;
                 }
             }
         }
         
	// write the bounding box
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE)) // use the map projection first
#ifdef USE_PROJ
	  gmlWriteBounds(stream, &(shape.bounds), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), "        ");
	else // then use the layer projection and/or metadata
	  gmlWriteBounds(stream, &(shape.bounds), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), "        ");	
#else
	gmlWriteBounds(stream, &(shape.bounds), NULL, "        "); // no projection information
#endif

        
        msIO_fprintf(stream, "        <gml:%s>\n", geom_name); 

                    
	// write the feature geometry
#ifdef USE_PROJ
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE)) // use the map projection first
	  gmlWriteGeometry(stream, &(shape), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), "          ");
        else // then use the layer projection and/or metadata
	  gmlWriteGeometry(stream, &(shape), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), "          ");      
#else
	gmlWriteGeometry(stream, &(shape), NULL, "          ");
#endif

        msIO_fprintf(stream, "        </gml:%s>\n", geom_name); 

	// write the item/values
	for(k=0; k<lp->numitems; k++)	
        {
          char *encoded_val;
          
          if( item_is_xml[k] == MS_TRUE )
              encoded_val = strdup(shape.values[k]);
          else
              encoded_val = msEncodeHTMLEntities(shape.values[k]);
         
          if (name_gml && strcmp(name_gml,  lp->items[k]) == 0)
            continue;
          if (description_gml && strcmp(description_gml,  lp->items[k]) == 0)
            continue;

          if(msIsXMLTagValid(lp->items[k]) == MS_FALSE)
              msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid "
                      "in a XML tag context. -->\n", lp->items[k]);
          
          if (wfs_namespace)
          {
            if(msIsXMLTagValid(wfs_namespace) == MS_FALSE)
              msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid "
                      "in a XML tag context. -->\n", wfs_namespace);
            msIO_fprintf(stream, "        <%s:%s>%s</%s:%s>\n", 
                    wfs_namespace, lp->items[k], encoded_val, 
                    wfs_namespace, lp->items[k]);
          }
          else      
            msIO_fprintf(stream, "        <%s>%s</%s>\n", 
                    lp->items[k], encoded_val, lp->items[k]);
        }

	// end this feature
        msIO_fprintf(stream, "      </%s>\n", layer_name);

	msIO_fprintf(stream, "    </gml:featureMember>\n");

        msFree(layer_name);

	msFreeShape(&shape); // init too

         features++;
         if (maxfeatures > 0 && features == maxfeatures)
           break;
         
         // end this layer
      }

      free( item_is_xml );

      msLayerClose(lp);
    }

    if (maxfeatures > 0 && features == maxfeatures)
      break;

  } // next layer


  return(MS_SUCCESS);

#else /* Stub for mapscript */
    msSetError(MS_MISCERR, "WMS server support not enabled",
               "msGMLWriteWFSQuery()");
    return MS_FAILURE;
#endif /* USE_WFS_SVR */
}

