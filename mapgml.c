#include "map.h"
#include "maperror.h"

/*
** Function to write the initial portion of a GML document.
*/

int msGMLWriteQuery(mapObj *map, char *filename)
{
  int i,j;
  layerObj *lp;
  FILE *stream=stdout; // defaults to stdout

  if(filename && strlen(filename) > 0) { // deal with the filename if present
    stream = fopen(filename, "w");
    if(!stream) {
       sprintf(ms_error.message, "(%s)", filename);
      msSetError(MS_IOERR, ms_error.message, "msGMLWriteQuery()");
      return(MS_FAILURE);
    }
  }

  // msGMLStart(stream);

  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    if(lp->resultcache && lp->resultcache->numresults > 0) {
      // msGMLCollectionStart(stream);

      // for(j=0; j<lp->resultcache->numresults; j++)
	// msGMLWriteShape(stream);

      // msGMLCollectionFinish(stream);
    }
  }

  // msGMLFinish(stream);

  if(filename && strlen(filename) > 0) fclose(stream);

  return(MS_SUCCESS);
}

int msGMLStart(FILE *stream, const char *prjElement, const char *prjURI, const char *schemaLocation, const char *prjDescription)
{
  if(!stream) return(MS_FAILURE);
  if(!prjElement) return(MS_FAILURE);
  if(!prjURI) return(MS_FAILURE);
  if(!schemaLocation) return(MS_FAILURE);
	
  fprintf(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  
  fprintf(stream, "<%s xmlns=\"%s\"\n", prjElement, prjURI);
  fprintf(stream, "\t xmlns:gml=\"http://www.opengis.net/gml\"\n" );
  fprintf(stream, "\t xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
  fprintf(stream, "\t xmlns:xsi=\"http://www.w3.org/2000/10/XMLSchema-instance\"\n");  
  fprintf(stream, "\t xsi:schemaLocation=\"%s\">\n", schemaLocation);

  if(prjDescription) fprintf(stream, "\t<gml:description>%s</gml:description>\n", prjDescription);
    
  return(MS_SUCCESS);
}

/*
** Function to write the final portion of a GML document.
*/
int msGMLFinish(FILE *stream, const char *prjElement) 
{
  if(!stream) return(MS_FAILURE);
	
  fprintf(stream, "</%s>\n", prjElement);
  return(MS_SUCCESS);
}

/*
** Function that essentially starts a feature collection. This can be
** a collection of a bunch of layers, or the features of a single layer.
*/
int msGMLCollectionStart(FILE *stream, const char *colName,  const char *colFID)
{
  if(!stream) return(MS_FAILURE);
  if(!colName) return(MS_FAILURE);

  if(colFID) 
    fprintf(stream, "\t<%s fid=\"%s\">", colName, colFID);
  else
    fprintf(stream, "\t<%s>", colName);

  return(MS_SUCCESS);
}

int msGMLCollectionFinish(FILE *stream, const char *colName)
{
  if(!stream) return(MS_FAILURE);
  if(!colName) return(MS_FAILURE);

  fprintf(stream, "\t</%s>", colName);

  return(MS_SUCCESS);
}

int msGMLWriteShape(FILE *stream, shapeObj *shape, char *name, char *srsName, char **items, char *tab) 
{
  int i, j;
   
  if(!stream) return(MS_FAILURE);
  if(!shape) return(MS_FAILURE);
 
  // start of feature
  if(name)
    fprintf(stream, "%s<%s>\n", tab, name);
  else
    fprintf(stream, "%s<gml:feature>\n", tab);
 
  // feature attributes
  if(items && shape->values) {
    for(i=0; i<shape->numvalues; i++)
      fprintf(stream, "%s<%s>%s</%s>\n", tab, items[i], shape->values[i], items[i]);
  }

  // feature bounding box
  fprintf(stream, "%s<gml:boundedBy>\n", tab);
  if(srsName) 
    fprintf(stream, "%s\t<gml:Box srsName=\"%s\">\n", tab, srsName);
  else
    fprintf(stream, "%s\t<gml:Box>\n", tab);
  fprintf(stream, "%s\t\t<gml:coordinates>\n", tab);  
  fprintf(stream, "%s\t\t\t%.6f,%.6f %.6f,%.6f\n", tab, shape->bounds.minx, shape->bounds.miny, shape->bounds.maxx, shape->bounds.maxy );   
  fprintf(stream, "%s\t\t</gml:coordinates>\n", tab);
  fprintf(stream, "%s\t</gml:Box>\n", tab);
  fprintf(stream, "%s</gml:boundedBy>\n", tab);

  // feature geometry
  switch(shape->type) {
  case(MS_SHAPE_POINT):
    break;
  case(MS_SHAPE_LINE):
    break;
  case(MS_SHAPE_POLYGON):
    break;
  default:
    break;
  }

  // end of feature
  if(name)
    fprintf(stream, "%s</%s>\n", tab, name);
  else
    fprintf(stream, "%s</gml:feature>\n", tab);

  return(MS_SUCCESS);
}
