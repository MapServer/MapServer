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
static int gmlWriteBounds(FILE *stream, rectObj *rect, char *srsname, char *tab)
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
}

// function only writes the feature geometry for a shapeObj
static int gmlWriteGeometry(FILE *stream, shapeObj *shape, char *srsname, char *tab) 
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

      fprintf(stream, "%s\t<gml:coordinates>\n", tab);
      fprintf(stream, "%s\t\t", tab);
      for(j=0; j<shape->line[0].numpoints; j++)
	fprintf(stream, "%f,%f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
      fprintf(stream, "\n");
      fprintf(stream, "%s\t</gml:coordinates>\n", tab);

      fprintf(stream, "%s</gml:LineString>\n", tab);
    } else { // write a MultiLineString      
      if(srsname)
        fprintf(stream, "%s<gml:MultiLineString srsName=\"%s\">\n", tab, srsname);
      else
        fprintf(stream, "%s<gml:MultiLineString>\n", tab);

      for(i=0; i<shape->numlines; i++) {
        fprintf(stream, "%s\t<gml:LineString>\n", tab); // no srsname at this point

        fprintf(stream, "%s\t\t<gml:coordinates>\n", tab);
        fprintf(stream, "%s\t\t\t", tab);
        for(j=0; j<shape->line[0].numpoints; j++)
	  fprintf(stream, "%f,%f", shape->line[i].point[j].x, shape->line[i].point[j].y);
        fprintf(stream, "\n");
        fprintf(stream, "%s\t\t</gml:coordinates>\n", tab);
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

      fprintf(stream, "%s\t<gml:outerBoundayIs>\n", tab);
      fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

      fprintf(stream, "%s\t\t\t<gml:coordinates>\n", tab);
      fprintf(stream, "%s\t\t\t\t", tab);
      for(j=0; j<shape->line[0].numpoints; j++)
	fprintf(stream, "%f,%f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
      fprintf(stream, "\n");
      fprintf(stream, "%s\t\t\t</gml:coordinates>\n", tab);

      fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
      fprintf(stream, "%s\t</gml:outerBoundayIs>\n", tab);

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

  if(filename && strlen(filename) > 0) { // deal with the filename if present
    stream = fopen(filename, "w");
    if(!stream) {
       sprintf(ms_error.message, "(%s)", filename);
      msSetError(MS_IOERR, ms_error.message, "msGMLWriteQuery()");
      return(MS_FAILURE);
    }
  }

  fprintf(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  
  if(msLookupHashTable(map->web.metadata, "gml_rootname")) 
    fprintf(stream, "<%s ", msLookupHashTable(map->web.metadata, "gml_rootname"));
  else
    fprintf(stream, "<msGMLOutput "); // default root element name
  
  if(msLookupHashTable(map->web.metadata, "gml_uri"))  fprintf(stream, "xmlns=\"%s\"\n", msLookupHashTable(map->web.metadata, "gml_uri"));
  fprintf(stream, "\t xmlns:gml=\"http://www.opengis.net/gml\"\n" );
  fprintf(stream, "\t xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
  fprintf(stream, "\t xmlns:xsi=\"http://www.w3.org/2000/10/XMLSchema-instance\"\n");  
  if(msLookupHashTable(map->web.metadata, "gml_schema")) fprintf(stream, "\t xsi:schemaLocation=\"%s\">\n", msLookupHashTable(map->web.metadata, "gml_schema"));

  // a schema *should* be required
  if(msLookupHashTable(map->web.metadata, "gml_description")) fprintf(stream, "\t<gml:description>%s</gml:description>\n", msLookupHashTable(map->web.metadata, "gml_description"));

  // step through the layers looking for query results
  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    if(lp->resultcache && lp->resultcache->numresults > 0) { // found results

      // start this collection (layer)
      if(msLookupHashTable(lp->metadata, "gml_layername")) // specify a collection name
	fprintf(stream, "\t<%s>\n", msLookupHashTable(lp->metadata, "gml_layername"));
      else
	fprintf(stream, "\t<%s_layer>\n", lp->name); // fall back on the layer name + "Layer"

      // actually open the layer
      status = msLayerOpen(lp, map->shapepath);
      if(status != MS_SUCCESS) return(status);

      // retrieve all the item names
      status = msLayerGetItems(lp);
      if(status != MS_SUCCESS) return(status);

      for(j=0; j<lp->resultcache->numresults; j++) {
	status = msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex);
        if(status != MS_SUCCESS) return(status);

	// start this feature
	if(msLookupHashTable(lp->metadata, "gml_featurename")) // specify a feature name
	  fprintf(stream, "\t\t<%s>\n", msLookupHashTable(lp->metadata, "gml_featurename"));
        else
	  fprintf(stream, "\t\t<%s_feature>\n", lp->name); // fall back on the layer name + "Feature"

	// write the item/values
	for(k=0; k<lp->numitems; k++)	
	  fprintf(stream, "\t\t\t<%s>%s</%s>\n", lp->items[k], shape.values[k], lp->items[k]);

	// write the bounding box
	gmlWriteBounds(stream, &(shape.bounds), NULL, "\t\t\t");

	// write the feature geometry
	gmlWriteGeometry(stream, &(shape), NULL, "\t\t\t");

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
