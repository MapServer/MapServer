#include "map.h"
#include "maperror.h"

// checks to see if ring r is an outer ring of shape
static int isOuterRing(shapeObj *shape, int r) {
  pointObj point; // a point in the ring
  shapeObj ring;

  if(shape->numlines == 1) return(MS_TRUE);

  msInitShape(&ring); // convert the ring of interest into its own shape
  msAddLine(&ring, &(shape->line[r]));

  msPolygonLabelPoint(&ring, &point, -1); // generate a point in that ring
  msFreeShape(&ring); // done with it

  return(msIntersectPointPolygon(&point, shape)); // test the point against the main shape
}

// returns a list of outer rings for shape (the list has one entry for each ring, MS_TRUE for outer rings)
static int *getOuterList(shapeObj *shape) {
  int i;
  int *list;

  list = (int *)malloc(sizeof(int)*shape->numlines);
  if(!list) return(NULL);

  for(i=0; i<shape->numlines; i++)
    list[i] = isOuterRing(shape, i);

  return(list);
}

// returns a list of inner rings for ring r in shape (given a list of outer rings)
static int *getInnerList(shapeObj *shape, int r, int *outerlist) {
  int i;
  int *list;

  list = (int *)malloc(sizeof(int)*shape->numlines);
  if(!list) return(NULL);

  for(i=0; i<shape->numlines; i++) { // test all rings against the ring

    if(outerlist[i] == MS_TRUE) { // ring is an outer and can't be an inner
      list[i] = MS_FALSE;
      continue;
    }

    list[i] = msPointInPolygon(&(shape->line[i].point[0]), &(shape->line[r]));
  }

  return(list);
}

// function that writes the feature boundary geometry (i.e. a rectObj)
static int gmlWriteBounds(FILE *stream, rectObj *rect, const char *srsname, char *tab)
{ 

  if(!stream) return(MS_FAILURE);
  if(!rect) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  fprintf(stream, "%s<gml:boundedBy>\n", tab);
  if(srsname)
    fprintf(stream, "%s\t<gml:Box srsName=\"%s\">\n", tab, srsname);
  else
    fprintf(stream, "%s\t<gml:Box>\n", tab);
  fprintf(stream, "%s\t\t<gml:coordinates>\n", tab);  
  fprintf(stream, "%s\t\t\t%.6f,%.6f %.6f,%.6f\n", tab, rect->minx, rect->miny, rect->maxx, rect->maxy );   
  fprintf(stream, "%s\t\t</gml:coordinates>\n", tab);
  fprintf(stream, "%s\t</gml:Box>\n", tab);
  fprintf(stream, "%s</gml:boundedBy>\n", tab);
  return MS_SUCCESS;
}

// function only writes the feature geometry for a shapeObj
static int gmlWriteGeometry(FILE *stream, shapeObj *shape, const char *srsname, char *tab) 
{
  int i, j, k;
  int *innerlist, *outerlist, numouters;

  if(!stream) return(MS_FAILURE);
  if(!shape) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  if(shape->numlines <= 0) return(MS_SUCCESS); // empty shape, nothing to output

  // feature geometry
  switch(shape->type) {
  case(MS_SHAPE_POINT):
    if(shape->line[0].numpoints > 1) { // write a MultiPoint
      if(srsname)
        fprintf(stream, "%s<gml:MultiPoint srsName=\"%s\">\n", tab, srsname);
      else
        fprintf(stream, "%s<gml:MultiPoint>\n", tab);

      for(i=0; i<shape->line[0].numpoints; i++) {
        fprintf(stream, "%s\t<gml:Point>\n", tab);
        fprintf(stream, "%s\t\t<gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[0].point[i].x, shape->line[0].point[i].y);
        fprintf(stream, "%s\t</gml:Point>\n", tab);
      }

      fprintf(stream, "%s</gml:MultiPoint>\n", tab);
    } else { // write a Point
      if(srsname)
        fprintf(stream, "%s<gml:Point srsName=\"%s\">\n", tab, srsname);
      else
        fprintf(stream, "%s<gml:Point>\n", tab);

      fprintf(stream, "%s\t<gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[0].point[0].x, shape->line[0].point[0].y);

      fprintf(stream, "%s</gml:Point>\n", tab);
    }
    break;
  case(MS_SHAPE_LINE):
    if(shape->numlines == 1) { // write a LineString
      if(srsname)
        fprintf(stream, "%s<gml:LineString srsName=\"%s\">\n", tab, srsname);
      else
        fprintf(stream, "%s<gml:LineString>\n", tab);

      fprintf(stream, "%s\t<gml:coordinates>", tab);
      for(j=0; j<shape->line[0].numpoints-1; j++)
        fprintf(stream, "%f,%f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
/* -------------------------------------------------------------------- */
/*      Adding a tab at the end of the coordinates string does          */
/*      create a bug when reading the the coordinates using ogr         */
/*      (function ParseGMLCoordinates in gml2ogrgeometry.cpp).          */
/* -------------------------------------------------------------------- */
      fprintf(stream, "</gml:coordinates>\n");

      fprintf(stream, "%s</gml:LineString>\n", tab);
    } else { // write a MultiLineString      
      if(srsname)
        fprintf(stream, "%s<gml:MultiLineString srsName=\"%s\">\n", tab, srsname);
      else
        fprintf(stream, "%s<gml:MultiLineString>\n", tab);

      for(i=0; i<shape->numlines; i++) {
        fprintf(stream, "%s\t<gml:LineString>\n", tab); // no srsname at this point

        fprintf(stream, "%s\t\t<gml:coordinates>", tab);
        for(j=0; j<shape->line[0].numpoints; j++)
	  fprintf(stream, "%f,%f", shape->line[i].point[j].x, shape->line[i].point[j].y);
        fprintf(stream, "</gml:coordinates>\n");
        fprintf(stream, "%s\t</gml:LineString>\n", tab);
      }

      fprintf(stream, "%s</gml:MultiLineString>\n", tab);
    }
    break;
  case(MS_SHAPE_POLYGON): // this gets nasty, since our shapes are so flexible
    if(shape->numlines == 1) { // write a Polygon (no interior rings)
      if(srsname)
        fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, srsname);
      else
        fprintf(stream, "%s<gml:Polygon>\n", tab);

      fprintf(stream, "%s\t<gml:outerBoundaryIs>\n", tab);
      fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

      fprintf(stream, "%s\t\t\t<gml:coordinates>\n", tab);
      fprintf(stream, "%s\t\t\t\t", tab);
      for(j=0; j<shape->line[0].numpoints; j++)
	fprintf(stream, "%f,%f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
      fprintf(stream, "\n");
      fprintf(stream, "%s\t\t\t</gml:coordinates>\n", tab);

      fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
      fprintf(stream, "%s\t</gml:outerBoundaryIs>\n", tab);

      fprintf(stream, "%s</gml:Polygon>\n", tab);
    } else { // need to test for inner and outer rings
      outerlist = getOuterList(shape);

      numouters = 0;
      for(i=0; i<shape->numlines; i++)
        if(outerlist[i] == MS_TRUE) numouters++;

      if(numouters == 1) { // write a Polygon (with interior rings)
	for(i=0; i<shape->numlines; i++) // find the outer ring
          if(outerlist[i] == MS_TRUE) break;

	innerlist = getInnerList(shape, i, outerlist);

	if(srsname)
          fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, srsname);
        else
          fprintf(stream, "%s<gml:Polygon>\n", tab);

        fprintf(stream, "%s\t<gml:outerBoundaryIs>\n", tab);
        fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

        fprintf(stream, "%s\t\t\t<gml:coordinates>\n", tab);
        fprintf(stream, "%s\t\t\t\t", tab);
        for(j=0; j<shape->line[i].numpoints; j++)
	  fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
        fprintf(stream, "\n");
        fprintf(stream, "%s\t\t\t</gml:coordinates>\n", tab);

        fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
        fprintf(stream, "%s\t</gml:outerBoundaryIs>\n", tab);

	for(k=0; k<shape->numlines; k++) { // now step through all the inner rings
	  if(innerlist[k] == MS_TRUE) {
	    fprintf(stream, "%s\t<gml:innerBoundaryIs>\n", tab);
            fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

            fprintf(stream, "%s\t\t\t<gml:coordinates>\n", tab);
            fprintf(stream, "%s\t\t\t\t", tab);
            for(j=0; j<shape->line[k].numpoints; j++)
	      fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
            fprintf(stream, "\n");
            fprintf(stream, "%s\t\t\t</gml:coordinates>\n", tab);

            fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
            fprintf(stream, "%s\t</gml:innerBoundaryIs>\n", tab);
          }
        }

        fprintf(stream, "%s</gml:Polygon>\n", tab);
	free(innerlist);
      } else {  // write a MultiPolygon	
	if(srsname)
          fprintf(stream, "%s<gml:MultiPolygon srsName=\"%s\">\n", tab, srsname);
        else
          fprintf(stream, "%s<gml:MultiPolygon>\n", tab);

	for(i=0; i<shape->numlines; i++) { // step through the outer rings
          if(outerlist[i] == MS_TRUE) {
  	    innerlist = getInnerList(shape, i, outerlist);

            fprintf(stream, "%s\t<gml:Polygon>\n", tab);

            fprintf(stream, "%s\t\t<gml:outerBoundaryIs>\n", tab);
            fprintf(stream, "%s\t\t\t<gml:LinearRing>\n", tab);

            fprintf(stream, "%s\t\t\t\t<gml:coordinates>\n", tab);
            fprintf(stream, "%s\t\t\t\t\t", tab);
            for(j=0; j<shape->line[i].numpoints; j++)
	      fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
            fprintf(stream, "\n");
            fprintf(stream, "%s\t\t\t\t</gml:coordinates>\n", tab);

            fprintf(stream, "%s\t\t\t</gml:LinearRing>\n", tab);
            fprintf(stream, "%s\t\t</gml:outerBoundaryIs>\n", tab);

	    for(k=0; k<shape->numlines; k++) { // now step through all the inner rings
	      if(innerlist[k] == MS_TRUE) {
	        fprintf(stream, "%s\t\t<gml:innerBoundaryIs>\n", tab);
                fprintf(stream, "%s\t\t\t<gml:LinearRing>\n", tab);

                fprintf(stream, "%s\t\t\t\t<gml:coordinates>\n", tab);
                fprintf(stream, "%s\t\t\t\t\t", tab);
                for(j=0; j<shape->line[k].numpoints; j++)
	          fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
                fprintf(stream, "\n");
                fprintf(stream, "%s\t\t\t\t</gml:coordinates>\n", tab);

                fprintf(stream, "%s\t\t\t</gml:LinearRing>\n", tab);
                fprintf(stream, "%s\t\t</gml:innerBoundaryIs>\n", tab);
              }
            }

            fprintf(stream, "%s\t</gml:Polygon>\n", tab);
	    free(innerlist);
          }
        }

	fprintf(stream, "%s</gml:MultiPolygon>\n", tab);
      }
      free(outerlist);
    }
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
}

int msGMLWriteQuery(mapObj *map, char *filename)
{
  int status;
  int i,j,k;
  layerObj *lp=NULL;
  shapeObj shape;
  FILE *stream=stdout; // defaults to stdout
  char szPath[MS_MAXPATHLEN];

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
  if(msLookupHashTable(map->web.metadata, "gml_encoding")) 
      fprintf(stream, "<?xml version=\"1.0\" encoding=\"%s\"?>\n\n",
              msLookupHashTable(map->web.metadata, "gml_encoding"));
  else if(msLookupHashTable(map->web.metadata, "wms_encoding")) 
      fprintf(stream, "<?xml version=\"1.0\" encoding=\"%s\"?>\n\n",
              msLookupHashTable(map->web.metadata, "wms_encoding"));
  else
      fprintf(stream, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\n");
  
  if(msLookupHashTable(map->web.metadata, "gml_rootname")) 
    fprintf(stream, "<%s ", msLookupHashTable(map->web.metadata, "gml_rootname"));
  else
    fprintf(stream, "<msGMLOutput "); // default root element name
  
  if(msLookupHashTable(map->web.metadata, "gml_uri"))  fprintf(stream, "xmlns=\"%s\"", msLookupHashTable(map->web.metadata, "gml_uri"));
  fprintf(stream, "\n\t xmlns:gml=\"http://www.opengis.net/gml\"" );
  fprintf(stream, "\n\t xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
  fprintf(stream, "\n\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");  
  if(msLookupHashTable(map->web.metadata, "gml_schema")) fprintf(stream, "\n\t xsi:schemaLocation=\"%s\"", msLookupHashTable(map->web.metadata, "gml_schema"));
  fprintf(stream, ">\n");

  // a schema *should* be required
  if(msLookupHashTable(map->web.metadata, "gml_description")) fprintf(stream, "\t<gml:description>%s</gml:description>\n", msLookupHashTable(map->web.metadata, "gml_description"));

  // step through the layers looking for query results
  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    if(lp->dump == MS_TRUE && lp->resultcache && lp->resultcache->numresults > 0) { // found results

      // start this collection (layer)
      if(msLookupHashTable(lp->metadata, "gml_layername")) // specify a collection name
	fprintf(stream, "\t<%s>\n", msLookupHashTable(lp->metadata, "gml_layername"));
      else
	fprintf(stream, "\t<%s_layer>\n", lp->name); // fall back on the layer name + "Layer"

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
	if(msLookupHashTable(lp->metadata, "gml_featurename")) // specify a feature name
	  fprintf(stream, "\t\t<%s>\n", msLookupHashTable(lp->metadata, "gml_featurename"));
        else
	  fprintf(stream, "\t\t<%s_feature>\n", lp->name); // fall back on the layer name + "Feature"

	// write the item/values
	for(k=0; k<lp->numitems; k++)	
        {
          char *encoded_val;
          encoded_val = msEncodeHTMLEntities(shape.values[k]);
	  fprintf(stream, "\t\t\t<%s>%s</%s>\n", lp->items[k], encoded_val, lp->items[k]);
          free(encoded_val);
        }

	// write the bounding box
#ifdef USE_PROJ
	if(msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE)) // use the map projection first
	  gmlWriteBounds(stream, &(shape.bounds), msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE), "\t\t\t");
	else // then use the layer projection and/or metadata
	  gmlWriteBounds(stream, &(shape.bounds), msGetEPSGProj(&(lp->projection), lp->metadata, MS_TRUE), "\t\t\t");	
#else
	gmlWriteBounds(stream, &(shape.bounds), NULL, "\t\t\t"); // no projection information
#endif

	// write the feature geometry
#ifdef USE_PROJ
	if(msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE)) // use the map projection first
	  gmlWriteGeometry(stream, &(shape), msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE), "\t\t\t");
        else // then use the layer projection and/or metadata
	  gmlWriteGeometry(stream, &(shape), msGetEPSGProj(&(lp->projection), lp->metadata, MS_TRUE), "\t\t\t");      
#else
	gmlWriteGeometry(stream, &(shape), NULL, "\t\t\t");
#endif

	// end this feature
        if(msLookupHashTable(lp->metadata, "gml_featurename")) // specify a feature name
	  fprintf(stream, "\t\t</%s>\n", msLookupHashTable(lp->metadata, "gml_featurename"));
        else
	  fprintf(stream, "\t\t</%s_feature>\n", lp->name); // fall back on the layer name + "Feature"

	msFreeShape(&shape); // init too
      }

      // end this collection (layer)
      if(msLookupHashTable(lp->metadata, "gml_layername")) // specify a collection name
        fprintf(stream, "\t</%s>\n", msLookupHashTable(lp->metadata, "gml_layername"));
      else
        fprintf(stream, "\t</%s_layer>\n", lp->name); // fall back on the layer name + "Layer"

      msLayerClose(lp);
    }
  } // next layer

  // end this document
  if(msLookupHashTable(map->web.metadata, "gml_rootname")) 
    fprintf(stream, "</%s>\n", msLookupHashTable(map->web.metadata, "gml_rootname"));
  else
    fprintf(stream, "</msGMLOutput>\n"); // default

  if(filename && strlen(filename) > 0) fclose(stream);

  return(MS_SUCCESS);
}


#ifdef USE_WFS_SVR

/*
** msGMLWriteWFSQuery()
**
** Similar to msGMLWriteQuery() but tuned for use with WFS 1.0.0
*/

int msGMLWriteWFSQuery(mapObj *map, FILE *stream, int maxfeatures, 
                       char *namespace)
{
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
                     msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE), 
                     "      ");
  }

  // step through the layers looking for query results
  for(i=0; i<map->numlayers; i++) 
  {
    lp = &(map->layers[i]);

    if(lp->dump == MS_TRUE && 
       lp->resultcache && lp->resultcache->numresults > 0) 
    { // found results
      const char *geom_name;
      geom_name = msWFSGetGeomElementName(map, lp);

      // actually open the layer
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(status);

      // retrieve all the item names. (Note : there might be no attributs)
      status = msLayerGetItems(lp);
      //if(status != MS_SUCCESS) return(status);

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
	fprintf(stream, "    <gml:featureMember>\n");
        if (namespace)
          fprintf(stream, "      <%s:%s>\n", namespace,lp->name);

        else
           fprintf(stream, "      <%s>\n", lp->name);

        //write name and description attributs if specified in the map file
        
        description_gml = msLookupHashTable(lp->metadata, "wfs_gml_description_item");
        if (description_gml)
        {
            for(k=0; k<lp->numitems; k++)	
            {
                if (strcasecmp(lp->items[k], description_gml) == 0)
                {
                    fprintf(stream, "      <gml:description>%s</gml:description>\n",  
                            msEncodeHTMLEntities(shape.values[k]));
                    break;
                 }
             }
         }
        name_gml = msLookupHashTable(lp->metadata, "wfs_gml_name_item");
        if (name_gml)
         {
             for(k=0; k<lp->numitems; k++)	
             {
                 if (strcasecmp(lp->items[k], name_gml) == 0)
                 {
                     fprintf(stream, "      <gml:name>%s</gml:name>\n",  
                             msEncodeHTMLEntities(shape.values[k]));
                     break;
                 }
             }
         }
         
	// write the bounding box
	if(msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE)) // use the map projection first
#ifdef USE_PROJ
	  gmlWriteBounds(stream, &(shape.bounds), msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE), "        ");
	else // then use the layer projection and/or metadata
	  gmlWriteBounds(stream, &(shape.bounds), msGetEPSGProj(&(lp->projection), lp->metadata, MS_TRUE), "        ");	
#else
	gmlWriteBounds(stream, &(shape.bounds), NULL, "        "); // no projection information
#endif

        
        fprintf(stream, "        <gml:%s>\n", geom_name); 

                    
	// write the feature geometry
#ifdef USE_PROJ
	if(msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE)) // use the map projection first
	  gmlWriteGeometry(stream, &(shape), msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE), "          ");
        else // then use the layer projection and/or metadata
	  gmlWriteGeometry(stream, &(shape), msGetEPSGProj(&(lp->projection), lp->metadata, MS_TRUE), "          ");      
#else
	gmlWriteGeometry(stream, &(shape), NULL, "          ");
#endif

        fprintf(stream, "        </gml:%s>\n", geom_name); 

	// write the item/values
	for(k=0; k<lp->numitems; k++)	
        {
          char *encoded_val;
          encoded_val = msEncodeHTMLEntities(shape.values[k]);
         
          if (name_gml && strcmp(name_gml,  lp->items[k]) == 0)
            continue;
          if (description_gml && strcmp(description_gml,  lp->items[k]) == 0)
            continue;
          
          if (namespace)
            fprintf(stream, "        <%s:%s>%s</%s:%s>\n", 
                    namespace, lp->items[k], encoded_val, namespace, lp->items[k]);
          else      
            fprintf(stream, "        <%s>%s</%s>\n", 
                    lp->items[k], encoded_val, lp->items[k]);
          free(encoded_val);
        }

	// end this feature
         if (namespace)
           fprintf(stream, "      </%s:%s>\n", namespace, lp->name);
         else
           fprintf(stream, "      </%s>\n", lp->name);

	fprintf(stream, "    </gml:featureMember>\n");

	msFreeShape(&shape); // init too

         features++;
         if (maxfeatures > 0 && features == maxfeatures)
           break;
      }

      // end this layer

      msLayerClose(lp);
    }

    if (maxfeatures > 0 && features == maxfeatures)
      break;

  } // next layer


  return(MS_SUCCESS);
}

#endif /* USE_WFS_SVR */
