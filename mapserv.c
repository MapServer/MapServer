#include "mapserv.h"

#ifdef USE_EGIS 
#include <errLog.h>  	// egis - includes
#include "egis.h"
#include "globalStruct.h"
#include "egisHTML.h"

// template stuff
//#include "maptemplate.c"

//OV -egis- for local debugging, if set various message will be 
// printed to /tmp/egisError.log

#define myDebug 0 
#define myDebugHtml 0

char errLogMsg[80];	// egis - for storing and printing error messages
#endif

extern double inchesPerUnit[6];

mapservObj* msObj;

int writeLog(int show_error)
{
  FILE *stream;
  int i;
  time_t t;

  if(!msObj->Map) return(0);
  if(!msObj->Map->web.log) return(0);
  
  if((stream = fopen(msObj->Map->web.log,"a")) == NULL) {
    msSetError(MS_IOERR, msObj->Map->web.log, "writeLog()");
    return(-1);
  }

  t = time(NULL);
  fprintf(stream,"%s,",chop(ctime(&t)));
  fprintf(stream,"%d,",(int)getpid());
  
  if(getenv("REMOTE_ADDR") != NULL)
    fprintf(stream,"%s,",getenv("REMOTE_ADDR"));
  else
    fprintf(stream,"NULL,");
 
  fprintf(stream,"%s,",msObj->Map->name);
  fprintf(stream,"%d,",msObj->Mode);

  fprintf(stream,"%f %f %f %f,", msObj->Map->extent.minx, msObj->Map->extent.miny, msObj->Map->extent.maxx, msObj->Map->extent.maxy);

  fprintf(stream,"%f %f,", msObj->MapPnt.x, msObj->MapPnt.y);

  for(i=0;i<msObj->NumLayers;i++)
    fprintf(stream, "%s ", msObj->Layers[i]);
  fprintf(stream,",");

  if(show_error == MS_TRUE)
    msWriteError(stream);
  else
    fprintf(stream, "normal execution");

  fprintf(stream,"\n");

  fclose(stream);
  return(0);
}

void writeError()
{
  errorObj *ms_error = msGetErrorObj();

  writeLog(MS_TRUE);

  if(!msObj->Map) {
    printf("Content-type: text/html%c%c",10,10);
    printf("<HTML>\n");
    printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
    printf("<!-- %s -->\n", msGetVersion());
    printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
    msWriteError(stdout);
    printf("</BODY></HTML>");
    exit(0);
  }

  if((ms_error->code == MS_NOTFOUND) && (msObj->Map->web.empty)) {
    msRedirect(msObj->Map->web.empty);
  } else {
    if(msObj->Map->web.error) {      
      msRedirect(msObj->Map->web.error);
    } else {
      printf("Content-type: text/html%c%c",10,10);
      printf("<HTML>\n");
      printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
      printf("<!-- %s -->\n", msGetVersion());
      printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
      msWriteError(stdout);
      printf("</BODY></HTML>");
    }
  }

  /*
  ** Clean Up
  */
  free(Item);
  free(Value);      
  free(QueryFile);
  free(QueryLayer);      
  free(SelectLayer);

  msFreeMapServObj(msObj);

  exit(0); // bail
}

/*
** Converts a string (e.g. form parameter) to a double, first checking the format against
** a regular expression. Dumps an error immediately if the format test fails.
*/
static double getNumeric(regex_t re, char *s)
{
  regmatch_t *match;
  
  if((match = (regmatch_t *)malloc(sizeof(regmatch_t))) == NULL) {
    msSetError(MS_MEMERR, NULL, "getNumeric()");
    writeError();
  }
  
  if(regexec(&re, s, (size_t)1, match, 0) == REG_NOMATCH) {
    free(match);
    msSetError(MS_TYPEERR, NULL, "getNumeric()"); 
    writeError();
  }
  
  if(strlen(s) != (match->rm_eo - match->rm_so)) { /* whole string must match */    
    free(match);
    msSetError(MS_TYPEERR, NULL, "getNumeric()"); 
    writeError();
  }
  
  free(match);
  return(atof(s));
}

/*
** Extract Map File name from params and load it.  
** Returns map object or NULL on error.
*/
mapObj *loadMap()
{
  int i;
  mapObj *map = NULL;

  for(i=0;i<msObj->NumParams;i++) // find the mapfile parameter first
    if(strcasecmp(msObj->ParamNames[i], "map") == 0) break;
  
  if(i == msObj->NumParams) {
    if(getenv("MS_MAPFILE")) // has a default file has not been set
      map = msLoadMap(getenv("MS_MAPFILE"), NULL);
    else {
      msSetError(MS_WEBERR, "CGI variable \"map\" is not set.", "loadMap()"); // no default, outta here
      writeError();
    }
  } else {
    if(getenv(msObj->ParamValues[i])) // an environment references the actual file to use
      map = msLoadMap(getenv(msObj->ParamValues[i]), NULL);
    else
      map = msLoadMap(msObj->ParamValues[i], NULL);
  }

  if(!map) writeError();

  return map;
}

/*
** Process CGI parameters.
*/
void loadForm()
{
  int i,j,k,n;
  char **tokens, *tmpstr;
  regex_t re;
  int rosa_type=0;

  if(regcomp(&re, NUMEXP, REG_EXTENDED) != 0) { // what is a number
    msSetError(MS_REGEXERR, NULL, "loadForm()"); 
    writeError();
  }

  for(i=0;i<msObj->NumParams;i++) { // now process the rest of the form variables
    if(strlen(msObj->ParamValues[i]) == 0)
      continue;
    
    if(strcasecmp(msObj->ParamNames[i],"queryfile") == 0) {      
      QueryFile = strdup(msObj->ParamValues[i]);
      continue;
    }
    
    if(strcasecmp(msObj->ParamNames[i],"savequery") == 0) {
      msObj->SaveQuery = MS_TRUE;
      continue;
    }
    
    if(strcasecmp(msObj->ParamNames[i],"savemap") == 0) {      
      msObj->SaveMap = MS_TRUE;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"zoom") == 0) {
      msObj->Zoom = getNumeric(re, msObj->ParamValues[i]);      
      if((msObj->Zoom > MAXZOOM) || (msObj->Zoom < MINZOOM)) {
	msSetError(MS_WEBERR, "Zoom value out of range.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"zoomdir") == 0) {
      msObj->ZoomDirection = getNumeric(re, msObj->ParamValues[i]);
      if((msObj->ZoomDirection != -1) && (msObj->ZoomDirection != 1) && (msObj->ZoomDirection != 0)) {
	msSetError(MS_WEBERR, "Zoom direction must be 1, 0 or -1.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"zoomsize") == 0) { // absolute zoom magnitude
      ZoomSize = getNumeric(re, msObj->ParamValues[i]);      
      if((ZoomSize > MAXZOOM) || (ZoomSize < 1)) {
	msSetError(MS_WEBERR, "Invalid zoom size.", "loadForm()");
	writeError();
      }	
      continue;
    }
    
    if(strcasecmp(msObj->ParamNames[i],"imgext") == 0) { // extent of an existing image in a web application
      tokens = split(msObj->ParamValues[i], ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 4) {
	msSetError(MS_WEBERR, "Not enough arguments for imgext.", "loadForm()");
	writeError();
      }

      msObj->ImgExt.minx = getNumeric(re, tokens[0]);
      msObj->ImgExt.miny = getNumeric(re, tokens[1]);
      msObj->ImgExt.maxx = getNumeric(re, tokens[2]);
      msObj->ImgExt.maxy = getNumeric(re, tokens[3]);

      msFreeCharArray(tokens, 4);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"searchmap") == 0) {      
      SearchMap = MS_TRUE;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"id") == 0) {
      strncpy(msObj->Id, msObj->ParamValues[i], IDSIZE);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"mapext") == 0) { // extent of the new map or query

      if(strncasecmp(msObj->ParamValues[i],"shape",5) == 0)
        msObj->UseShapes = MS_TRUE;
      else {
	tokens = split(msObj->ParamValues[i], ' ', &n);
	
	if(!tokens) {
	  msSetError(MS_MEMERR, NULL, "loadForm()");
	  writeError();
	}
	
	if(n != 4) {
	  msSetError(MS_WEBERR, "Not enough arguments for mapext.", "loadForm()");
	  writeError();
	}
	
	msObj->Map->extent.minx = getNumeric(re, tokens[0]);
	msObj->Map->extent.miny = getNumeric(re, tokens[1]);
	msObj->Map->extent.maxx = getNumeric(re, tokens[2]);
	msObj->Map->extent.maxy = getNumeric(re, tokens[3]);	
	
	msFreeCharArray(tokens, 4);
	
#ifdef USE_PROJ
	if(msObj->Map->projection.proj && !pj_is_latlong(msObj->Map->projection.proj)
           && (msObj->Map->extent.minx >= -180.0 && msObj->Map->extent.minx <= 180.0) 
           && (msObj->Map->extent.miny >= -90.0 && msObj->Map->extent.miny <= 90.0))
	  msProjectRect(&(msObj->Map->latlon), 
                        &(msObj->Map->projection), 
                        &(msObj->Map->extent)); // extent is a in lat/lon
#endif

	if((msObj->Map->extent.minx != msObj->Map->extent.maxx) && (msObj->Map->extent.miny != msObj->Map->extent.maxy)) { // extent seems ok
	  msObj->CoordSource = FROMUSERBOX;
	  QueryCoordSource = FROMUSERBOX;
	}
      }

      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"minx") == 0) { // extent of the new map, in pieces
      msObj->Map->extent.minx = getNumeric(re, msObj->ParamValues[i]);      
      continue;
    }
    if(strcasecmp(msObj->ParamNames[i],"maxx") == 0) {      
      msObj->Map->extent.maxx = getNumeric(re, msObj->ParamValues[i]);
      continue;
    }
    if(strcasecmp(msObj->ParamNames[i],"miny") == 0) {
      msObj->Map->extent.miny = getNumeric(re, msObj->ParamValues[i]);
      continue;
    }
    if(strcasecmp(msObj->ParamNames[i],"maxy") == 0) {
      msObj->Map->extent.maxy = getNumeric(re, msObj->ParamValues[i]);
      msObj->CoordSource = FROMUSERBOX;
      QueryCoordSource = FROMUSERBOX;
      continue;
    } 

    if(strcasecmp(msObj->ParamNames[i],"mapxy") == 0) { // user map coordinate
      
      if(strncasecmp(msObj->ParamValues[i],"shape",5) == 0) {
        msObj->UseShapes = MS_TRUE;	
      } else {
	tokens = split(msObj->ParamValues[i], ' ', &n);

	if(!tokens) {
	  msSetError(MS_MEMERR, NULL, "loadForm()");
	  writeError();
	}
	
	if(n != 2) {
	  msSetError(MS_WEBERR, "Not enough arguments for mapxy.", "loadForm()");
	  writeError();
	}
	
	msObj->MapPnt.x = getNumeric(re, tokens[0]);
	msObj->MapPnt.y = getNumeric(re, tokens[1]);
	
	msFreeCharArray(tokens, 2);

#ifdef USE_PROJ
	if(msObj->Map->projection.proj && !pj_is_latlong(msObj->Map->projection.proj)
           && (msObj->MapPnt.x >= -180.0 && msObj->MapPnt.x <= 180.0) 
           && (msObj->MapPnt.y >= -90.0 && msObj->MapPnt.y <= 90.0))
	  msProjectPoint(&(msObj->Map->projection), 
                         &(msObj->Map->projection), 
                         &msObj->MapPnt); // point is a in lat/lon
#endif

	if(msObj->CoordSource == NONE) { // don't override previous settings (i.e. buffer or scale )
	  msObj->CoordSource = FROMUSERPNT;
	  QueryCoordSource = FROMUSERPNT;
	}
      }
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"mapshape") == 0) { // query shape
      lineObj line={0,NULL};
      char **tmp=NULL;
      int n, j;
      
      tmp = split(msObj->ParamValues[i], ' ', &n);

      if((line.point = (pointObj *)malloc(sizeof(pointObj)*(n/2))) == NULL) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }
      line.numpoints = n/2;

      msInitShape(&(msObj->SelectShape));
      msObj->SelectShape.type = MS_SHAPE_POLYGON;

      for(j=0; j<n/2; j++) {
	line.point[j].x = atof(tmp[2*j]);
	line.point[j].y = atof(tmp[2*j+1]);

#ifdef USE_PROJ
	if(msObj->Map->projection.proj && !pj_is_latlong(msObj->Map->projection.proj)
           && (line.point[j].x >= -180.0 && line.point[j].x <= 180.0) 
           && (line.point[j].y >= -90.0 && line.point[j].y <= 90.0))
	  msProjectPoint(&(msObj->Map->latlon), 
                         &(msObj->Map->projection), 
                         &line.point[j]); // point is a in lat/lon
#endif
      }

      if(msAddLine(&msObj->SelectShape, &line) == -1) writeError();

      msFree(line.point);	
      msFreeCharArray(tmp, n);

      QueryCoordSource = FROMUSERSHAPE;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"img.x") == 0) { // mouse click, in pieces
      msObj->ImgPnt.x = getNumeric(re, msObj->ParamValues[i]);
      if((msObj->ImgPnt.x > (2*MS_MAXIMGSIZE)) || (msObj->ImgPnt.x < (-2*MS_MAXIMGSIZE))) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      msObj->CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }
    if(strcasecmp(msObj->ParamNames[i],"img.y") == 0) {
      msObj->ImgPnt.y = getNumeric(re, msObj->ParamValues[i]);      
      if((msObj->ImgPnt.y > (2*MS_MAXIMGSIZE)) || (msObj->ImgPnt.y < (-2*MS_MAXIMGSIZE))) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      msObj->CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"imgxy") == 0) { // mouse click, single variable
      if(msObj->CoordSource == FROMIMGPNT)
	continue;

      tokens = split(msObj->ParamValues[i], ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
	writeError();
      }

      msObj->ImgPnt.x = getNumeric(re, tokens[0]);
      msObj->ImgPnt.y = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);

      if((msObj->ImgPnt.x > (2*MS_MAXIMGSIZE)) || (msObj->ImgPnt.x < (-2*MS_MAXIMGSIZE)) || (msObj->ImgPnt.y > (2*MS_MAXIMGSIZE)) || (msObj->ImgPnt.y < (-2*MS_MAXIMGSIZE))) {
	msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
	writeError();
      }

      if(msObj->CoordSource == NONE) { // override nothing since this parameter is usually used to hold a default value
	msObj->CoordSource = FROMIMGPNT;
	QueryCoordSource = FROMIMGPNT;
      }
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"imgbox") == 0) { // selection box (eg. mouse drag)
      tokens = split(msObj->ParamValues[i], ' ', &n);
      
      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }
      
      if(n != 4) {
	msSetError(MS_WEBERR, "Not enough arguments for imgbox.", "loadForm()");
	writeError();
      }
      
      msObj->ImgBox.minx = getNumeric(re, tokens[0]);
      msObj->ImgBox.miny = getNumeric(re, tokens[1]);
      msObj->ImgBox.maxx = getNumeric(re, tokens[2]);
      msObj->ImgBox.maxy = getNumeric(re, tokens[3]);
      
      msFreeCharArray(tokens, 4);

      if((msObj->ImgBox.minx != msObj->ImgBox.maxx) && (msObj->ImgBox.miny != msObj->ImgBox.maxy)) { // must not degenerate into a point
	msObj->CoordSource = FROMIMGBOX;
	QueryCoordSource = FROMIMGBOX;
      }
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"imgshape") == 0) { // shape given in image coordinates
      lineObj line={0,NULL};
      char **tmp=NULL;
      int n, j;
      
      tmp = split(msObj->ParamValues[i], ' ', &n);

      if((line.point = (pointObj *)malloc(sizeof(pointObj)*(n/2))) == NULL) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }
      line.numpoints = n/2;

      msInitShape(&msObj->SelectShape);
      msObj->SelectShape.type = MS_SHAPE_POLYGON;

      for(j=0; j<n/2; j++) {
	line.point[j].x = atof(tmp[2*j]);
	line.point[j].y = atof(tmp[2*j+1]);
      }

      if(msAddLine(&msObj->SelectShape, &line) == -1) writeError();

      msFree(line.point);
      msFreeCharArray(tmp, n);

      QueryCoordSource = FROMIMGSHAPE;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"ref.x") == 0) { // mouse click in reference image, in pieces
      msObj->RefPnt.x = getNumeric(re, msObj->ParamValues[i]);      
      if((msObj->RefPnt.x > (2*MS_MAXIMGSIZE)) || (msObj->RefPnt.x < (-2*MS_MAXIMGSIZE))) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      msObj->CoordSource = FROMREFPNT;
      continue;
    }
    if(strcasecmp(msObj->ParamNames[i],"ref.y") == 0) {
      msObj->RefPnt.y = getNumeric(re, msObj->ParamValues[i]); 
      if((msObj->RefPnt.y > (2*MS_MAXIMGSIZE)) || (msObj->RefPnt.y < (-2*MS_MAXIMGSIZE))) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      msObj->CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"refxy") == 0) { /* mouse click in reference image, single variable */
      tokens = split(msObj->ParamValues[i], ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
	writeError();
      }

      msObj->RefPnt.x = getNumeric(re, tokens[0]);
      msObj->RefPnt.y = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if((msObj->RefPnt.x > (2*MS_MAXIMGSIZE)) || (msObj->RefPnt.x < (-2*MS_MAXIMGSIZE)) || (msObj->RefPnt.y > (2*MS_MAXIMGSIZE)) || (msObj->RefPnt.y < (-2*MS_MAXIMGSIZE))) {
	msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
	writeError();
      }
      
      msObj->CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"buffer") == 0) { // radius (map units), actually 1/2 square side
      msObj->Buffer = getNumeric(re, msObj->ParamValues[i]);      
      msObj->CoordSource = FROMBUF;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"scale") == 0) { // scale for new map
      msObj->Scale = getNumeric(re, msObj->ParamValues[i]);      
      if(msObj->Scale <= 0) {
	msSetError(MS_WEBERR, "Scale out of range.", "loadForm()");
	writeError();
      }
      msObj->CoordSource = FROMSCALE;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }
    
    if(strcasecmp(msObj->ParamNames[i],"imgsize") == 0) { // size of existing image (pixels)
      tokens = split(msObj->ParamValues[i], ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for imgsize.", "loadForm()");
	writeError();
      }

      msObj->ImgCols = getNumeric(re, tokens[0]);
      msObj->ImgRows = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(msObj->ImgCols > MS_MAXIMGSIZE || msObj->ImgRows > MS_MAXIMGSIZE || msObj->ImgCols < 0 || msObj->ImgRows < 0) {
	msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
	writeError();
      }
 
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"mapsize") == 0) { // size of new map (pixels)
      tokens = split(msObj->ParamValues[i], ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for mapsize.", "loadForm()");
	writeError();
      }

      msObj->Map->width = getNumeric(re, tokens[0]);
      msObj->Map->height = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(msObj->Map->width > MS_MAXIMGSIZE || msObj->Map->height > MS_MAXIMGSIZE || msObj->Map->width < 0 || msObj->Map->height < 0) {
	msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strncasecmp(msObj->ParamNames[i],"layers", 6) == 0) { // turn a set of layers, delimited by spaces, on
      int num_layers=0, l;
      char **layers=NULL;

      layers = split(msObj->ParamValues[i], ' ', &(num_layers));
      for(l=0; l<num_layers; l++)
	msObj->Layers[msObj->NumLayers+l] = strdup(layers[l]);
      msObj->NumLayers += l;

      msFreeCharArray(layers, num_layers);
      num_layers = 0;
      continue;
    }

    if(strncasecmp(msObj->ParamNames[i],"layer", 5) == 0) { // turn a single layer/group on
      msObj->Layers[msObj->NumLayers] = strdup(msObj->ParamValues[i]);
      msObj->NumLayers++;
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"qlayer") == 0) { // layer to query (i.e search)
      QueryLayer = strdup(msObj->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"qitem") == 0) { // attribute to query on (optional)
      QueryItem = strdup(msObj->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"qstring") == 0) { // attribute query string
      QueryString = strdup(msObj->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"slayer") == 0) { // layer to select (for feature based search)
      SelectLayer = strdup(msObj->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"item") == 0) { // search item
      Item = strdup(msObj->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"value") == 0) { // search expression
      if(!Value)
	Value = strdup(msObj->ParamValues[i]);
      else { /* need to append */
	tmpstr = strdup(Value);
	free(Value);
	Value = (char *)malloc(strlen(tmpstr)+strlen(msObj->ParamValues[i])+2);
	sprintf(Value, "%s|%s", tmpstr, msObj->ParamValues[i]);
	free(tmpstr);
      }
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"template") == 0) { // template file, common change hence the simple parameter
      free(msObj->Map->web.template);
      msObj->Map->web.template = strdup(msObj->ParamValues[i]);      
      continue;
    }

#ifdef USE_EGIS

    // OV - egis - additional token "none" is defined to create somewhat
    // mutual exculsiveness between mapserver and egis
 
    if(strcasecmp(msObj->ParamNames[i],"egis") == 0)
    {
        if(strcasecmp(msObj->ParamValues[i],"none") != 0)
        {
                msObj->Mode = PROCESSING;
        }
        continue;
    }
#endif

    if(strcasecmp(msObj->ParamNames[i],"shapeindex") == 0) { // used for index queries
      ShapeIndex = getNumeric(re, msObj->ParamValues[i]);
      continue;
    }
    if(strcasecmp(msObj->ParamNames[i],"tileindex") == 0) {
      TileIndex = getNumeric(re, msObj->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->ParamNames[i],"mode") == 0) { // set operation mode
      for(j=0; j<numModes; j++) {
	if(strcasecmp(msObj->ParamValues[i], modeStrings[j]) == 0) {
	  msObj->Mode = j;

	  if(msObj->Mode == ZOOMIN) {
	    msObj->ZoomDirection = 1;
	    msObj->Mode = BROWSE;
	  }
	  if(msObj->Mode == ZOOMOUT) {
	    msObj->ZoomDirection = -1;
	    msObj->Mode = BROWSE;
	  }

	  break;
	}
      }

      if(j == numModes) {
	msSetError(MS_WEBERR, "Invalid mode.", "loadForm()");
	writeError();
      }

      continue;
    }

    if(strncasecmp(msObj->ParamNames[i],"map_",4) == 0) { // check to see if there are any additions to the mapfile
      if(msLoadMapString(msObj->Map, msObj->ParamNames[i], msObj->ParamValues[i]) == -1)
	writeError();
      continue;
    }
/* -------------------------------------------------------------------- */
/*      The following code is used to support the rosa applet (for      */
/*      more information on Rosa, please consult :                      */
/*      http://www2.dmsolutions.ca/webtools/rosa/index.html) .          */
/*      This code was provided by Tim.Mackey@agso.gov.au.               */
/*                                                                      */
/*      For Application using it can be seen at :                       */
/*http://mapserver.gis.umn.edu/wilma/mapserver-users/0011/msg00077.html */
/*   http://www.agso.gov.au/map/pilbara/                                */
/*                                                                      */
/* -------------------------------------------------------------------- */

    if(strcasecmp(msObj->ParamNames[i],"INPUT_TYPE") == 0)
    { /* Rosa input type */
        if(strcasecmp(msObj->ParamValues[i],"auto_rect") == 0) 
        {
            rosa_type=1; /* rectangle */
            continue;
        }
            
        if(strcasecmp(msObj->ParamValues[i],"auto_point") == 0) 
        {
            rosa_type=2; /* point */
            continue;
        }
    }
    if(strcasecmp(msObj->ParamNames[i],"INPUT_COORD") == 0) 
    { /* Rosa coordinates */
 
       switch(rosa_type)
       {
         case 1:
             sscanf(msObj->ParamValues[i],"%lf,%lf;%lf,%lf",
                    &msObj->ImgBox.minx,&msObj->ImgBox.miny,&msObj->ImgBox.maxx,
                    &msObj->ImgBox.maxy);
             if((msObj->ImgBox.minx != msObj->ImgBox.maxx) && 
                (msObj->ImgBox.miny != msObj->ImgBox.maxy)) 
             {
                 msObj->CoordSource = FROMIMGBOX;
                 QueryCoordSource = FROMIMGBOX;
             }
             else 
             {
                 msObj->CoordSource = FROMIMGPNT;
                 QueryCoordSource = FROMIMGPNT;
                 msObj->ImgPnt.x=msObj->ImgBox.minx;
                 msObj->ImgPnt.y=msObj->ImgBox.miny;
	   }
           break;
         case 2:
           sscanf(msObj->ParamValues[i],"%lf,%lf",&msObj->ImgPnt.x,
                   &msObj->ImgPnt.y);
           msObj->CoordSource = FROMIMGPNT;
           QueryCoordSource = FROMIMGPNT;
           break;
         }
       continue;
    }    
/* -------------------------------------------------------------------- */
/*      end of code for Rosa support.                                   */
/* -------------------------------------------------------------------- */


  }

  regfree(&re);

  if(ZoomSize != 0) { // use direction and magnitude to calculate zoom
    if(msObj->ZoomDirection == 0) {
      msObj->fZoom = 1;
    } else {
      msObj->fZoom = ZoomSize*msObj->ZoomDirection;
      if(msObj->fZoom < 0)
	msObj->fZoom = 1.0/MS_ABS(msObj->fZoom);
    }
  } else { // use single value for zoom
    if((msObj->Zoom >= -1) && (msObj->Zoom <= 1)) {
      msObj->fZoom = 1; // pan
    } else {
      if(msObj->Zoom < 0)
	msObj->fZoom = 1.0/MS_ABS(msObj->Zoom);
      else
	msObj->fZoom = msObj->Zoom;
    }
  }

  if(msObj->ImgRows == -1) msObj->ImgRows = msObj->Map->height;
  if(msObj->ImgCols == -1) msObj->ImgCols = msObj->Map->width;  
  if(msObj->Map->height == -1) msObj->Map->height = msObj->ImgRows;
  if(msObj->Map->width == -1) msObj->Map->width = msObj->ImgCols;

  for(i=0;i<msObj->NumParams;i++) {
    tmpstr = (char *)malloc(sizeof(char)*strlen(msObj->ParamNames[i]) + 3);
    sprintf(tmpstr,"%%%s%%", msObj->ParamNames[i]);
    
    for(j=0; j<msObj->Map->numlayers; j++) {
      if(msObj->Map->layers[j].filter.string && (strstr(msObj->Map->layers[j].filter.string, tmpstr) != NULL)) msObj->Map->layers[j].filter.string = gsub(msObj->Map->layers[j].filter.string, tmpstr, msObj->ParamValues[i]);
      for(k=0; k<msObj->Map->layers[j].numclasses; k++)
	if(msObj->Map->layers[j].class[k].expression.string && (strstr(msObj->Map->layers[j].class[k].expression.string, tmpstr) != NULL)) msObj->Map->layers[j].class[k].expression.string = gsub(msObj->Map->layers[j].class[k].expression.string, tmpstr, msObj->ParamValues[i]);
    }
    
    free(tmpstr);
  }
}

void setExtentFromShapes() {
  int i;
  int found=0;
  double dx, dy, cellsize;
  layerObj *lp;

  rectObj tmpext={-1.0,-1.0,-1.0,-1.0};
  pointObj tmppnt={-1.0,-1.0};

  for(i=0; i<msObj->Map->numlayers; i++) {
    lp = &(msObj->Map->layers[i]);

    if(!lp->resultcache) continue;
    if(lp->resultcache->numresults <= 0) continue;

    if(!found) {
      tmpext = lp->resultcache->bounds;
      found = 1;
    } else
      msMergeRect(&(tmpext), &(lp->resultcache->bounds));
  }

  dx = tmpext.maxx - tmpext.minx;
  dy = tmpext.maxy - tmpext.miny;
 
  tmppnt.x = (tmpext.maxx + tmpext.minx)/2;
  tmppnt.y = (tmpext.maxy + tmpext.miny)/2;
  tmpext.minx -= dx*EXTENT_PADDING/2.0;
  tmpext.maxx += dx*EXTENT_PADDING/2.0;
  tmpext.miny -= dy*EXTENT_PADDING/2.0;
  tmpext.maxy += dy*EXTENT_PADDING/2.0;

  if(msObj->Scale != 0) { // apply the scale around the center point (tmppnt)
    cellsize = (msObj->Scale/msObj->Map->resolution)/inchesPerUnit[msObj->Map->units]; // user supplied a point and a scale
    tmpext.minx = tmppnt.x - cellsize*msObj->Map->width/2.0;
    tmpext.miny = tmppnt.y - cellsize*msObj->Map->height/2.0;
    tmpext.maxx = tmppnt.x + cellsize*msObj->Map->width/2.0;
    tmpext.maxy = tmppnt.y + cellsize*msObj->Map->height/2.0;
  } else if(msObj->Buffer != 0) { // apply the buffer around the center point (tmppnt)
    tmpext.minx = tmppnt.x - msObj->Buffer;
    tmpext.miny = tmppnt.y - msObj->Buffer;
    tmpext.maxx = tmppnt.x + msObj->Buffer;
    tmpext.maxy = tmppnt.y + msObj->Buffer;
  }

  // in case we don't get  usable extent at this point (i.e. single point result)
  if(!MS_VALID_EXTENT(tmpext.minx, tmpext.miny, tmpext.maxx, tmpext.maxy)) {
    if(msObj->Map->web.minscale > 0) { // try web object minscale first
      cellsize = (msObj->Map->web.minscale/msObj->Map->resolution)/inchesPerUnit[msObj->Map->units]; // user supplied a point and a scale
      tmpext.minx = tmppnt.x - cellsize*msObj->Map->width/2.0;
      tmpext.miny = tmppnt.y - cellsize*msObj->Map->height/2.0;
      tmpext.maxx = tmppnt.x + cellsize*msObj->Map->width/2.0;
      tmpext.maxy = tmppnt.y + cellsize*msObj->Map->height/2.0;
    } else {
      msSetError(MS_WEBERR, "No way to generate a valid map extent from selected shapes.", "mapserv()");
      writeError();
    }
  }

  msObj->MapPnt = tmppnt;
  msObj->Map->extent = msObj->RawExt = tmpext; // save unadjusted extent

  return;
}


/* FIX: NEED ERROR CHECKING HERE FOR IMGPNT or MAPPNT */
void setCoordinate()
{
  double cellx,celly;

  cellx = MS_CELLSIZE(msObj->ImgExt.minx, msObj->ImgExt.maxx, msObj->ImgCols);
  celly = MS_CELLSIZE(msObj->ImgExt.miny, msObj->ImgExt.maxy, msObj->ImgRows);

  msObj->MapPnt.x = MS_IMAGE2MAP_X(msObj->ImgPnt.x, msObj->ImgExt.minx, cellx);
  msObj->MapPnt.y = MS_IMAGE2MAP_Y(msObj->ImgPnt.y, msObj->ImgExt.maxy, celly);

  return;
}

void returnCoordinate()
{
  msSetError(MS_NOERR, 
             "Your \"<i>click</i>\" corresponds to (approximately): (%g, %g).",
             NULL,
             msObj->MapPnt.x, msObj->MapPnt.y);

#ifdef USE_PROJ
  if(msObj->Map->projection.proj != NULL && !pj_is_latlong(msObj->Map->projection.proj) ) {
    pointObj p=msObj->MapPnt;
    msProjectPoint(&(msObj->Map->projection), &(msObj->Map->latlon), &p);
    msSetError( MS_NOERR, 
                "%s Computed lat/lon value is (%g, %g).\n", NULL, p.x, p.y);
  }
#endif

  writeError();
}


// FIX: need to consider JOINS
// FIX: need to consider 5% shape extent expansion


/*
**
** Start of main program
**
*/
int main(int argc, char *argv[]) {
    int i,j;
    char buffer[1024];
    imageObj *img=NULL;
    int status;

    msObj = msAllocMapServObj();

#ifdef USE_EGIS
    // OV -egis- Initialize egis error log file here...
    initErrLog("/export/home/tmp/msError.log");
    writeErrLog("ErrorLogfile is initialized\n");
    fflush(fpErrLog);
#endif

    if(argc > 1 && strcmp(argv[1], "-v") == 0) {
      printf("%s\n", msGetVersion());
      fflush(stdout);
      exit(0);
    }
    else if (argc > 1 && strncmp(argv[1], "QUERY_STRING=", 13) == 0) {
      /* Debugging hook... pass "QUERY_STRING=..." on the command-line */
      char *buf;
      buf = strdup("REQUEST_METHOD=GET");
      putenv(buf);
      buf = strdup(argv[1]);
      putenv(buf);
    }

    sprintf(msObj->Id, "%ld%d",(long)time(NULL),(int)getpid()); // asign now so it can be overridden

    msObj->ParamNames = (char **) malloc(MAX_PARAMS*sizeof(char*));
    msObj->ParamValues = (char **) malloc(MAX_PARAMS*sizeof(char*));
    if (msObj->ParamNames==NULL || msObj->ParamValues==NULL) {
	msSetError(MS_MEMERR, NULL, "mapserv()");
	writeError();
    }

    msObj->NumParams = loadParams(msObj->ParamNames, msObj->ParamValues);
    msObj->Map = loadMap();

    /*
    ** Start by calling the WMS Dispatcher.  If it fails then we'll process
    ** this as a regular MapServer request.
    */
#ifdef USE_WMS

    if ((status = msWMSDispatch(msObj->Map, msObj->ParamNames, msObj->ParamValues, msObj->NumParams)) != MS_DONE) {
       
       if (status != MS_SUCCESS)
         writeError();
      /* This was a WMS request... cleanup and exit */
      msFreeMap(msObj->Map);
      msFreeCharArray(msObj->ParamNames, msObj->NumParams);
      msFreeCharArray(msObj->ParamValues, msObj->NumParams);
       
      exit(0);
    }
#endif

    loadForm();
 
    if(msObj->SaveMap) {
      sprintf(buffer, "%s%s%s.map", msObj->Map->web.imagepath, msObj->Map->name, msObj->Id);
      if(msSaveMap(msObj->Map, buffer) == -1) writeError();
    }

    if((msObj->CoordSource == FROMIMGPNT) || (msObj->CoordSource == FROMIMGBOX)) /* make sure extent of existing image matches shape of image */
      msObj->Map->cellsize = msAdjustExtent(&msObj->ImgExt, msObj->ImgCols, msObj->ImgRows);

    /*
    ** For each layer lets set layer status
    */
    for(i=0;i<msObj->Map->numlayers;i++) {
      if((msObj->Map->layers[i].status != MS_DEFAULT)) {
	if(isOn(msObj, msObj->Map->layers[i].name, msObj->Map->layers[i].group) == MS_TRUE) /* Set layer status */
	  msObj->Map->layers[i].status = MS_ON;
	else
	  msObj->Map->layers[i].status = MS_OFF;
      }     
    }

    if(msObj->CoordSource == FROMREFPNT) /* force browse mode if the reference coords are set */
      msObj->Mode = BROWSE;

    if(msObj->Mode == BROWSE) {

      if(!msObj->Map->web.template) {
	msSetError(MS_WEBERR, "No template provided.", "mapserv()");
	writeError();
      }

      if(QueryFile) {
	status = msLoadQuery(msObj->Map, QueryFile);
	if(status != MS_SUCCESS) writeError();
      }
      
      setExtent(msObj);
      checkWebScale(msObj);
       
/* -------------------------------------------------------------------- */
/*      generate map, legend, scalebar and refernce images.             */
/* -------------------------------------------------------------------- */
      if (msGenerateImages(msObj, QueryFile, MS_TRUE) == MS_FALSE)
        writeError();
      
      if(QueryFile) {
          if (msReturnQuery(msObj, "text/html", NULL) != MS_SUCCESS)
           writeError();
      } else {
	if(TEMPLATE_TYPE(msObj->Map->web.template) == MS_FILE) { /* if thers's an html template, then use it */
	  printf("Content-type: text/html%c%c", 10, 10); /* write MIME header */
	  printf("<!-- %s -->\n", msGetVersion());
	  fflush(stdout);
	  if (msReturnPage(msObj, msObj->Map->web.template, BROWSE, NULL) != MS_SUCCESS)
         writeError();
	} else {	
	  if (msReturnURL(msObj, msObj->Map->web.template, BROWSE) != MS_SUCCESS)
         writeError();
	}
      }

    } else if(msObj->Mode == MAP || msObj->Mode == SCALEBAR || msObj->Mode == REFERENCE) { // "image" only modes
      setExtent(msObj);
      checkWebScale(msObj);
      
      switch(msObj->Mode) {
      case MAP:
	if(QueryFile) {
	  status = msLoadQuery(msObj->Map, QueryFile);
	  if(status != MS_SUCCESS) writeError();
	  img = msDrawQueryMap(msObj->Map);
	} else
	  img = msDrawMap(msObj->Map);
	break;
      case REFERENCE:
	msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	img = msDrawReferenceMap(msObj->Map);
	break;      
      case SCALEBAR:
	img = msDrawScalebar(msObj->Map);
	break;
      }
      
      if(!img) writeError();

      printf("Content-type: %s%c%c",MS_IMAGE_MIME_TYPE(msObj->Map->imagetype), 10,10);
      status = msSaveImage(img, NULL, msObj->Map->transparent, msObj->Map->interlace, msObj->Map->imagequality);
      if(status != MS_SUCCESS) writeError();
      
      msFreeImage(img);
    } else if (msObj->Mode == LEGEND) {
       if (msObj->Map->legend.template) 
       {
          char *legendTemplate;

          legendTemplate = generateLegendTemplate(msObj);
          if (legendTemplate) {
             printf("Content-type: text/html\n\n%s", legendTemplate);

             free(legendTemplate);
          }
          else // error already generated by (generateLegendTemplate())
            writeError();;
       }         
       else
       {
          img = msDrawLegend(msObj->Map);
          if(!img) writeError();

          printf("Content-type: %s%c%c",MS_IMAGE_MIME_TYPE(msObj->Map->imagetype), 10,10);
          status = msSaveImage(img, NULL, msObj->Map->transparent, msObj->Map->interlace, msObj->Map->imagequality);
          if(status != MS_SUCCESS) writeError();
          
          msFreeImage(img);            
       }
    } else if(msObj->Mode >= QUERY) { // query modes

      if(QueryFile) { // already got a completed query
	status = msLoadQuery(msObj->Map, QueryFile);
	if(status != MS_SUCCESS) writeError();
      } else {

	if((QueryLayerIndex = msGetLayerIndex(msObj->Map, QueryLayer)) != -1) /* force the query layer on */
	  msObj->Map->layers[QueryLayerIndex].status = MS_ON;

        switch(msObj->Mode) {
	case ITEMFEATUREQUERY:
        case ITEMFEATURENQUERY:
	case ITEMFEATUREQUERYMAP:
        case ITEMFEATURENQUERYMAP:
	  if((SelectLayerIndex = msGetLayerIndex(msObj->Map, SelectLayer)) == -1) { /* force the selection layer on */
	    msSetError(MS_WEBERR, "Selection layer not set or references an invalid layer.", "mapserv()"); 
	    writeError();
	  }
	  msObj->Map->layers[SelectLayerIndex].status = MS_ON;

	  if(QueryCoordSource != NONE && !msObj->UseShapes)
	    setExtent(msObj); /* set user area of interest */

	  if(msObj->Mode == ITEMFEATUREQUERY || msObj->Mode == ITEMFEATUREQUERYMAP) {
	    if((status = msQueryByAttributes(msObj->Map, SelectLayerIndex, QueryItem, QueryString, MS_SINGLE)) != MS_SUCCESS) writeError();
	  } else {
	    if((status = msQueryByAttributes(msObj->Map, SelectLayerIndex, QueryItem, QueryString, MS_MULTIPLE)) != MS_SUCCESS) writeError();
	  }

	  if(msQueryByFeatures(msObj->Map, QueryLayerIndex, SelectLayerIndex) != MS_SUCCESS) writeError();

	  break;
        case FEATUREQUERY:
        case FEATURENQUERY:
	case FEATUREQUERYMAP:
        case FEATURENQUERYMAP:
	  if((SelectLayerIndex = msGetLayerIndex(msObj->Map, SelectLayer)) == -1) { /* force the selection layer on */
	    msSetError(MS_WEBERR, "Selection layer not set or references an invalid layer.", "mapserv()"); 
	    writeError();
	  }
	  msObj->Map->layers[SelectLayerIndex].status = MS_ON;
	  
	  if(msObj->Mode == FEATUREQUERY || msObj->Mode == FEATUREQUERYMAP) {
	    switch(QueryCoordSource) {
	    case FROMIMGPNT:
	      msObj->Map->extent = msObj->ImgExt; /* use the existing map extent */	
	      setCoordinate();
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_SINGLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	      break;
	    case FROMUSERPNT: /* only a buffer makes sense */
	      if(msObj->Buffer == -1) {
		msSetError(MS_WEBERR, "Point given but no search buffer specified.", "mapserv()");
		writeError();
	      }
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_SINGLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	      break;
	    default:
	      msSetError(MS_WEBERR, "No way to the initial search, not enough information.", "mapserv()");
	      writeError();
	      break;
	    }	  
	  } else { /* FEATURENQUERY/FEATURENQUERYMAP */
	    switch(QueryCoordSource) {
	    case FROMIMGPNT:
	      msObj->Map->extent = msObj->ImgExt; /* use the existing map extent */	
	      setCoordinate();
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_MULTIPLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	      break;	 
	    case FROMIMGBOX:
	      break;
	    case FROMUSERPNT: /* only a buffer makes sense */
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_MULTIPLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	    default:
	      setExtent(msObj);
	      if((status = msQueryByRect(msObj->Map, SelectLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	      break;
	    }
	  } /* end switch */
	
	  if(msQueryByFeatures(msObj->Map, QueryLayerIndex, SelectLayerIndex) != MS_SUCCESS) writeError();
      
	  break;
        case ITEMQUERY:
	case ITEMNQUERY:
        case ITEMQUERYMAP:
        case ITEMNQUERYMAP:

	  if(QueryCoordSource != NONE && !msObj->UseShapes)
	    setExtent(msObj); /* set user area of interest */

	  if(msObj->Mode == ITEMQUERY || msObj->Mode == ITEMQUERYMAP) {
	    if((status = msQueryByAttributes(msObj->Map, QueryLayerIndex, QueryItem, QueryString, MS_SINGLE)) != MS_SUCCESS) writeError();
	  } else {
	    if((status = msQueryByAttributes(msObj->Map, QueryLayerIndex, QueryItem, QueryString, MS_MULTIPLE)) != MS_SUCCESS) writeError();
          }

	  break;
        case NQUERY:
        case NQUERYMAP:
	  switch(QueryCoordSource) {
	  case FROMIMGPNT:	  
	    setCoordinate();
	  
	    if(SearchMap) { // compute new extent, pan etc then search that extent
	      setExtent(msObj);
	      msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    } else {
	      msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	      msObj->Map->width = msObj->ImgCols;
	      msObj->Map->height = msObj->ImgRows;
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();	 
	      if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_MULTIPLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	    }
	    break;	  
	  case FROMIMGBOX:	  
	    if(SearchMap) { // compute new extent, pan etc then search that extent
	      setExtent(msObj);
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
	      msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    } else {
	      double cellx, celly;
	    
	      msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	      msObj->Map->width = msObj->ImgCols;
	      msObj->Map->height = msObj->ImgRows;
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();	  
	    
	      cellx = MS_CELLSIZE(msObj->ImgExt.minx, msObj->ImgExt.maxx, msObj->ImgCols); // calculate the new search extent
	      celly = MS_CELLSIZE(msObj->ImgExt.miny, msObj->ImgExt.maxy, msObj->ImgRows);
	      msObj->RawExt.minx = MS_IMAGE2MAP_X(msObj->ImgBox.minx, msObj->ImgExt.minx, cellx);	      
	      msObj->RawExt.maxx = MS_IMAGE2MAP_X(msObj->ImgBox.maxx, msObj->ImgExt.minx, cellx);
	      msObj->RawExt.maxy = MS_IMAGE2MAP_Y(msObj->ImgBox.miny, msObj->ImgExt.maxy, celly); // y's are flip flopped because img/map coordinate systems are
	      msObj->RawExt.miny = MS_IMAGE2MAP_Y(msObj->ImgBox.maxy, msObj->ImgExt.maxy, celly);

	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->RawExt)) != MS_SUCCESS) writeError();
	    }
	    break;
	  case FROMIMGSHAPE:
	    msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	    msObj->Map->width = msObj->ImgCols;
	    msObj->Map->height = msObj->ImgRows;
	    msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	    if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
	  
	    // convert from image to map coordinates here (see setCoordinate)
	    for(i=0; i<msObj->SelectShape.numlines; i++) {
	      for(j=0; j<msObj->SelectShape.line[i].numpoints; j++) {
	        msObj->SelectShape.line[i].point[j].x = MS_IMAGE2MAP_X(msObj->SelectShape.line[i].point[j].x, msObj->Map->extent.minx, msObj->Map->cellsize);
	        msObj->SelectShape.line[i].point[j].y = MS_IMAGE2MAP_Y(msObj->SelectShape.line[i].point[j].y, msObj->Map->extent.maxy, msObj->Map->cellsize);
	      }
	    }
	  
	    if((status = msQueryByShape(msObj->Map, QueryLayerIndex, &msObj->SelectShape)) != MS_SUCCESS) writeError();
	    break;	  
	  case FROMUSERPNT:
	    if(msObj->Buffer == 0) {
	      if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_MULTIPLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	      setExtent(msObj);
	    } else {
	      setExtent(msObj);
	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    }
	    break;
	  case FROMUSERSHAPE:
	    setExtent(msObj);
	    if((status = msQueryByShape(msObj->Map, QueryLayerIndex, &msObj->SelectShape)) != MS_SUCCESS) writeError();
	    break;	  
	  default: // from an extent of some sort
	    setExtent(msObj);
	    if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    break;
	  }      
	  break;
        case QUERY:
        case QUERYMAP:
	  switch(QueryCoordSource) {
	  case FROMIMGPNT:
	    setCoordinate();
	    msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	    msObj->Map->width = msObj->ImgCols;
	    msObj->Map->height = msObj->ImgRows;
	    if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();	 	  
	    if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_SINGLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	    break;
	  
	  case FROMUSERPNT: /* only a buffer makes sense, DOES IT? */	
	    setExtent(msObj);	
	    if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_SINGLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	    break;
	  
	  default:
	    msSetError(MS_WEBERR, "Query mode needs a point, imgxy and mapxy are not set.", "mapserv()");
	    writeError();
	    break;
	  }
	  break;
        case INDEXQUERY:
        case INDEXQUERYMAP:
	  if((status = msQueryByIndex(msObj->Map, QueryLayerIndex, TileIndex, ShapeIndex)) != MS_SUCCESS) writeError();
	  break;
        } // end mode switch
      }
	  
      if(msObj->Map->querymap.width != -1) msObj->Map->width = msObj->Map->querymap.width; // make sure we use the right size
      if(msObj->Map->querymap.height != -1) msObj->Map->height = msObj->Map->querymap.height;

      if(msObj->UseShapes)
	setExtentFromShapes();

      // just return the image
      if(msObj->Mode == QUERYMAP || msObj->Mode == NQUERYMAP || msObj->Mode == ITEMQUERYMAP || msObj->Mode == ITEMNQUERYMAP || msObj->Mode == FEATUREQUERYMAP || msObj->Mode == FEATURENQUERYMAP || msObj->Mode == ITEMFEATUREQUERYMAP || msObj->Mode == ITEMFEATURENQUERYMAP || msObj->Mode == INDEXQUERYMAP) {

	checkWebScale(msObj);

	img = msDrawQueryMap(msObj->Map);
	if(!img) writeError();

	printf("Content-type: %s%c%c",MS_IMAGE_MIME_TYPE(msObj->Map->imagetype), 10,10);
	status = msSaveImage(img, NULL, msObj->Map->transparent, msObj->Map->interlace, msObj->Map->imagequality);
	if(status != MS_SUCCESS) writeError();
	msFreeImage(img);

      } 
       else { // process the query through templates
          if (msReturnTemplateQuery(msObj, "text/html") != MS_SUCCESS) writeError();
          
          if(msObj->SaveQuery) {
             sprintf(buffer, "%s%s%s%s", msObj->Map->web.imagepath, msObj->Map->name, msObj->Id, MS_QUERY_EXTENSION);
             if((status = msSaveQuery(msObj->Map, buffer)) != MS_SUCCESS) return status;
          }
       }

    } else if(msObj->Mode == COORDINATE) {
      setCoordinate(); // mouse click => map coord
      returnCoordinate(msObj->MapPnt);
    } else if(msObj->Mode == PROCESSING) {
#ifdef USE_EGIS
      setExtent(msObj);
      errLogMsg[0] = '\0';
      sprintf(errLogMsg, "Map Coordinates: x %f, y %f\n", msObj->MapPnt.x, msObj->MapPnt.y);
      writeErrLog(errLogMsg);
      
      status = egisl(msObj->Map, Entries, msObj->NumParams, msObj->CoordSource);
      // printf("Numerical Window - %f %f\n", msObj->ImgPnt.x, msObj->ImgPnt.y);
      fflush(stdout);
      
      if (status == 1) {
	// write MIME header
	printf("Content-type: text/html%c%c", 10, 10);
	returnStatistics("numwin.html");
      } else if(status == 2) {
	writeErrLog("Calling returnSplot()\n");
	fflush(fpErrLog);
	
	// write MIME header
	printf("Content-type: text/html%c%c", 10, 10);
	returnSplot("splot.html");
      } else if(status == 3) {
	writeErrLog("Calling returnHisto()\n");
	fflush(fpErrLog);
	
	// write MIME header
	printf("Content-type: text/html%c%c", 10, 10);
	returnHisto("histo.html");
      } else {
	writeErrLog("Error status from egisl()\n");
	fflush(fpErrLog);
	
	errLogMsg[0] = '\0';
	sprintf(errLogMsg, "Select Image Layer\n");
	
	msSetError(MS_IOERR, "<br>eGIS Error: Choose Stack Image<br>", errLogMsg);
	writeError();
      }
#else
      msSetError(MS_MISCERR, "EGIS Support is not available.", "mapserv()");
      writeError();
#endif
    }

    writeLog(MS_FALSE);
   
    /*
    ** Clean-up
    */
    free(Item); // the following are not stored as part of the msObj
    free(Value);      
    free(QueryLayer);
    free(SelectLayer);
    free(QueryFile);    
   
    msFreeMapServObj(msObj);

    exit(0); // end MapServer
} 
