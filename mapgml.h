#ifndef MAPGML_H
#define MAP_H

// gml header
int msGMLStart(FILE *gmlOut, 
	       const char *gmlversion, 
	       char *projectElement, 
	       char *schemaLocation,
	       char *prjDescription );

// gml geometry + (attributes)?
int msGMLWriteShape(FILE *stream, 
		    shapeObj *shape, 
		    char *srsName, 
		    char **gmlType);
// gml tailer
int msGMLFinish(FILE *stream, 
		const char *prjElement);

#endif
