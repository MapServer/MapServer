#include "map.h"
#include "maperror.h"

/*
** Function to write the initial portion of a GML document.
*/

/*
** comments to Steve
** List of Additional parameters that may be needed
** char *projectElement		<- document root element name
** char *schemaLocation		<- e.g. http://terrasip.gis.umn.edu/projects/egis/avhrr.xsd
** char *prjDescription		<- optional project description text
**
*/

int msGMLStart(FILE *stream, const char *gmlversion, char *projectElement, char *schemaLocation, char *prjDescription) 
{
  if(!stream) return(MS_FAILURE);
  
  fprintf(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  
  fprintf(stream, "<%s xmlns=\"http://mapserver.gis.umn.edu/wmtprojects\" \n", projectElement);
  //fprintf(stream, "<msProject xmlns=\"http://mapserver.gis.umn.edu/wmtprojects\" \n");
  
  fprintf(stream, "\t\t xmlns:gml=\"http://www.opengis.net/gml\" \n" );
  fprintf(stream, "\t\t xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n");
  fprintf(stream, "\t\t xmlns:xsi=\"http://www.w3.org/2000/10/XMLSchema-instance\" \n");
  
  // schema location is needed here
  // fprintf(stream, "\t\t xsi:schemaLocation=\"http://mapserver.gis.umn.edu/wmtprojects %s\"> \n", 
  //										schemaLocation);
  
  fprintf(stream, "\t\t xsi:schemaLocation=\"http://mapserver.gis.umn.edu/wmtprojects test.xsd\"> \n" );
  
  // project description - optional
  // if needed than we need char *prjDescription parameter
  
  // fprintf(stream, "\t<gml:description> %s </gml:description>\n", prjDescription);
  fprintf(stream, "\t<gml:description> Natural Resource Monitoring with AVHRR </gml:description>\n");
  
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

int msGMLLayerStart(FILE *stream)
{
  if(!stream) return(MS_FAILURE);

  return MS_SUCCESS;
}

int msGMLLayerFinish(FILE *stream)
{
  if(!stream) return(MS_FAILURE);

  return MS_SUCCESS;
}

/*
** int msGMLWriteShape(FILE *stream, shapeObj *shape, char *srsName, char **gmlType)
**
**	Additional requirements
** Data Structures :: for mapping shape->type to gmlType
** 	char **gmlType;
** Misc. Parameters
** char *srsName 	<- OGIS equivalent srs name 
*/

int msGMLWriteShape(FILE *stream, shapeObj *shape, char *srsName, char **gmlType) 
{
  int i, j;
  
  // layer description - optional
  // if needed than we need char *layerDescription parameter

  if(!stream) return(MS_FAILURE);

  // fprintf(stream, "\t<gml:name> %s </gml:name>\n", layerDescription);
  fprintf(stream, "\t<gml:name> Major Highways  </gml:name>\n"); 
  
  // Bounding box - map extents (or shape extents)
  
  fprintf(stream, "\t<gml:boundedBy>\n");

 
  // srsName - for now default
  // fprintf(stream, "\t\t<gml:Box srsName=\"%s\">\n", srsName);
  fprintf(stream, "\t\t<gml:Box srsName=\"http://www.opengis.net/gml/srs/epsg.xml#4326\">\n");
  
  fprintf(stream, "\t\t\t<gml:coordinates>\n");
  
  fprintf(stream, "\t\t\t\t%.3f,%.3f %.3f,%.3f\n",
	  shape->bounds.minx,
	  shape->bounds.miny,
	  shape->bounds.maxx,
	  shape->bounds.maxy );
   
  fprintf(stream, "\t\t\t</gml:coordinates>\n");
  fprintf(stream, "\t\t</gml:Box>\n");
  fprintf(stream, "\t</gml:boundedBy>\n");
  
  // End of extents
  
  // generate gemetry features
  // gemetry is partof any database object
  // we need to define schema first for object and then use that definition here 
  // for now just generating geometry stuff only
  
  for (i = 0; i < shape->numlines; i++)
    {
      //fprintf(stream, "\t\t\t<gml:%s srsName=\"EPSG:4326\">\n", gmlType[shape->type]);
      fprintf(stream, "\t\t\t<gml:%s srsName=\"%s\">\n", gmlType[shape->type], srsName);
      
      fprintf(stream, "\t\t\t\t<gml:coordinates>\n");
      fprintf(stream, "\t\t\t\t\t");
      
      for (j = 0; j < shape->line[i].numpoints; j++)
	{
	  // Default: coords are separated by , 
	  // and touples are separted by space
	  fprintf(stream, " %.2f,%.2f", shape->line[i].point[j].x, shape->line[i].point[j].y );
	}
      
      fprintf(stream, "\n");
      fprintf(stream, "\t\t\t\t</gml:coordinates>\n");
      fprintf(stream, "\t\t\t</gml:%s>\n", gmlType[shape->type] );
      
      //fprintf(stream, "\t\t</geometricProperty>\n", psShape->nVertices);
      //fprintf(stream, "\t</featureMember>\n");
    }

  return(MS_SUCCESS);
}

