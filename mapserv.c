#include "mapserv.h"

#ifdef USE_EGIS 
#include <errLog.h>  	// egis - includes
#include "egis.h"
#include "globalStruct.h"
#include "egisHTML.h"
 
//OV -egis- for local debugging, if set various message will be 
// printed to /tmp/egisError.log

#define myDebug 0 
#define myDebugHtml 0

char errLogMsg[80];	// egis - for storing and printing error messages
#endif

static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};

/*
** Various function prototypes
*/
void returnHTML(char *);
void returnURL(char *);

void writeVersion() {
  printf("<!-- MapServer Version %s", MS_VERSION);
#ifdef USE_PROJ
  printf(" -PROJ.4");
#endif
#ifdef USE_TTF
  printf(" -FreeType");
#endif
#ifdef USE_TIFF
  printf(" -TIFF");
#endif
#ifdef USE_EPPL
  printf(" -EPPL7");
#endif
#ifdef USE_JPEG
  printf(" -JPEG");
#endif
#ifdef USE_EGIS
  printf(" -EGIS");
#endif
  printf(" -->\n");
}

int writeLog(int show_error)
{
  FILE *stream;
  int i;
  time_t t;

  if(!Map)
    return(0); /* errors in the initial reading of map file cannot be logged */

  if(!Map->web.log)
    return(0);
  
  if((stream = fopen(Map->web.log,"a")) == NULL) {
    msSetError(MS_IOERR, Map->web.log, "writeLog()");
    return(-1);
  }

  t = time(NULL);
  fprintf(stream,"%s,",chop(ctime(&t)));
  fprintf(stream,"%d,",(int)getpid());
  
  if(getenv("REMOTE_ADDR") != NULL)
    fprintf(stream,"%s,",getenv("REMOTE_ADDR"));
  else
    fprintf(stream,"NULL,");
 
  fprintf(stream,"%s,",Map->name);
  fprintf(stream,"%d,",Mode);

  fprintf(stream,"%f %f %f %f,", Map->extent.minx, Map->extent.miny,Map->extent.maxx,Map->extent.maxy);

  fprintf(stream,"%f %f,", MapPnt.x, MapPnt.y);

  for(i=0;i<NumLayers;i++)
    fprintf(stream, "%s ", Layers[i]);
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
  int i;

  writeLog(MS_TRUE);

  if(!Map) {
    printf("Content-type: text/html%c%c",10,10);
    printf("<HTML>\n");
    printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
    writeVersion();
    printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
    msWriteError(stdout);
    printf("</BODY></HTML>");
    exit(0);
  }

  if((ms_error.code == MS_NOTFOUND) && (Map->web.empty)) {
    redirect(Map->web.empty);
  } else {
    if(Map->web.error) {      
      redirect(Map->web.error);
    } else {
      printf("Content-type: text/html%c%c",10,10);
      printf("<HTML>\n");
      printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
      writeVersion();
      printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
      msWriteError(stdout);
      printf("</BODY></HTML>");
    }
  }

  msFreeQueryResults(QueryResults);

  msFreeMap(Map);
  for(i=0;i<NumEntries;i++) {
    free(Entries[i].val);
    free(Entries[i].name);
  }
    
  free(Item);
  free(Value);      
  free(QueryFile);
  free(QueryLayer);      
  free(SelectLayer);

  for(i=0;i<NumLayers;i++)
    free(Layers[i]);

  exit(0); /* bail */
}

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

void loadForm() /* set variables as input from the user form */
{
  int i, n;
  char **tokens, *tmpstr;
  regex_t re;
  
  /* FIND and LOAD mapfile first */
  for(i=0;i<NumEntries;i++)
    if(strcasecmp(Entries[i].name, "map") == 0)
      break;
  
  if(i == NumEntries) {
    msSetError(MS_WEBERR, "CGI variable \"map\" is not set.", "loadForm()"); 
    writeError();
  }

  Map = msLoadMap(Entries[i].val);
  if(!Map)
    writeError();

  if(regcomp(&re, NUMEXP, REG_EXTENDED) != 0) {
    msSetError(MS_REGEXERR, NULL, "loadForm()"); 
    writeError();
  }

  for(i=0;i<NumEntries;i++) { /* now process the rest of the form variables */

    if(strlen(Entries[i].val) == 0)
      continue;
    
    if(strcasecmp(Entries[i].name,"queryfile") == 0) {      
      QueryFile = strdup(Entries[i].val);
      continue;
    }
    
    if(strcasecmp(Entries[i].name,"savequery") == 0) {
      SaveQuery = MS_TRUE;
      continue;
    }
    
    if(strcasecmp(Entries[i].name,"savemap") == 0) {      
      SaveMap = MS_TRUE;
      continue;
    }

    if(strcasecmp(Entries[i].name,"zoom") == 0) { /* zoom factor */
      Zoom = getNumeric(re, Entries[i].val);      
      if((Zoom > MAXZOOM) || (Zoom < MINZOOM)) {
	msSetError(MS_WEBERR, "Zoom value out of range.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strcasecmp(Entries[i].name,"zoomdir") == 0) { /* zoom direction */
      ZoomDirection = getNumeric(re, Entries[i].val);
      if((ZoomDirection != -1) && (ZoomDirection != 1) && (ZoomDirection != 0)) {
	msSetError(MS_WEBERR, "Zoom direction must be 1, 0 or -1.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strcasecmp(Entries[i].name,"zoomsize") == 0) { /* absolute zoom magnitude */
      ZoomSize = getNumeric(re, Entries[i].val);      
      if((ZoomSize > MAXZOOM) || (ZoomSize < 1)) {
	msSetError(MS_WEBERR, "Invalid zoom size.", "loadForm()");
	writeError();
      }	
      continue;
    }
    
    if(strcasecmp(Entries[i].name,"imgext") == 0) { /* extent of existing image */
      tokens = split(Entries[i].val, ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 4) {
	msSetError(MS_WEBERR, "Not enough arguments for imgext.", "loadForm()");
	writeError();
      }

      ImgExt.minx = getNumeric(re, tokens[0]);
      ImgExt.miny = getNumeric(re, tokens[1]);
      ImgExt.maxx = getNumeric(re, tokens[2]);
      ImgExt.maxy = getNumeric(re, tokens[3]);

      msFreeCharArray(tokens, 4);
      continue;
    }

    if(strcasecmp(Entries[i].name,"searchmap") == 0) {      
      SearchMap = MS_TRUE;
      continue;
    }

    if(strcasecmp(Entries[i].name,"id") == 0) {      
      strncpy(Id, Entries[i].val, IDSIZE);
      continue;
    }

    if(strcasecmp(Entries[i].name,"mapext") == 0) { /* extent of the new map or query */

      if(strncasecmp(Entries[i].val,"shape",5) == 0)
        UseShapes = MS_TRUE;
      else {
	tokens = split(Entries[i].val, ' ', &n);
	
	if(!tokens) {
	  msSetError(MS_MEMERR, NULL, "loadForm()");
	  writeError();
	}
	
	if(n != 4) {
	  msSetError(MS_WEBERR, "Not enough arguments for mapext.", "loadForm()");
	  writeError();
	}
	
	Map->extent.minx = getNumeric(re, tokens[0]);
	Map->extent.miny = getNumeric(re, tokens[1]);
	Map->extent.maxx = getNumeric(re, tokens[2]);
	Map->extent.maxy = getNumeric(re, tokens[3]);	
	
	msFreeCharArray(tokens, 4);
	
#ifdef USE_PROJ
	if(Map->projection.proj && (Map->extent.minx >= -180.0 && Map->extent.minx <= 180.0) && (Map->extent.miny >= -90.0 && Map->extent.miny <= 90.0))
	  msProjectRect(NULL, Map->projection.proj, &(Map->extent)); // extent is a in lat/lon
#endif

	if((Map->extent.minx != Map->extent.maxx) && (Map->extent.miny != Map->extent.maxy)) {
	  CoordSource = FROMUSERBOX; /* extent seems valid */
	  QueryCoordSource = FROMUSERBOX;
	}
      }

      continue;
    }

    if(strcasecmp(Entries[i].name,"minx") == 0) { /* extent of the new map */
      Map->extent.minx = getNumeric(re, Entries[i].val);      
      continue;
    }
    if(strcasecmp(Entries[i].name,"maxx") == 0) {      
      Map->extent.maxx = getNumeric(re, Entries[i].val);
      continue;
    }
    if(strcasecmp(Entries[i].name,"miny") == 0) {
      Map->extent.miny = getNumeric(re, Entries[i].val);
      continue;
    }
    if(strcasecmp(Entries[i].name,"maxy") == 0) {
      Map->extent.maxy = getNumeric(re, Entries[i].val);
      CoordSource = FROMUSERBOX;
      QueryCoordSource = FROMUSERBOX;
      continue;
    } 

    if(strcasecmp(Entries[i].name,"mapxy") == 0) { /* user map coordinate */
      
      if(strncasecmp(Entries[i].val,"shape",5) == 0) {
        UseShapes = MS_TRUE;	
      } else {
	tokens = split(Entries[i].val, ' ', &n);

	if(!tokens) {
	  msSetError(MS_MEMERR, NULL, "loadForm()");
	  writeError();
	}
	
	if(n != 2) {
	  msSetError(MS_WEBERR, "Not enough arguments for mapxy.", "loadForm()");
	  writeError();
	}
	
	MapPnt.x = getNumeric(re, tokens[0]);
	MapPnt.y = getNumeric(re, tokens[1]);
	
	msFreeCharArray(tokens, 2);

#ifdef USE_PROJ
	if(Map->projection.proj && (MapPnt.x >= -180.0 && MapPnt.x <= 180.0) && (MapPnt.y >= -90.0 && MapPnt.y <= 90.0))
	  msProjectPoint(NULL, Map->projection.proj, &MapPnt); // point is a in lat/lon
#endif

	if(CoordSource == NONE) { /* don't override previous settings (i.e. buffer or scale ) */
	  CoordSource = FROMUSERPNT;
	  QueryCoordSource = FROMUSERPNT;
	}
      }
      continue;
    }

    if(strcasecmp(Entries[i].name,"mapshape") == 0) { /* query shape */     
      lineObj line={0,NULL};
      char **tmp=NULL;
      int n, j;
      
      tmp = split(Entries[i].val, ' ', &n);

      if((line.point = (pointObj *)malloc(sizeof(pointObj)*(n/2))) == NULL) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }
      line.numpoints = n/2;

      for(j=0; j<n/2; j++) {
	line.point[j].x = atof(tmp[2*j]);
	line.point[j].y = atof(tmp[2*j+1]);

#ifdef USE_PROJ
	if(Map->projection.proj && (line.point[j].x >= -180.0 && line.point[j].x <= 180.0) && (line.point[j].y >= -90.0 && line.point[j].y <= 90.0))
	  msProjectPoint(NULL, Map->projection.proj, &line.point[j]); // point is a in lat/lon
#endif
      }

      if(msAddLine(&SelectShape, &line) == -1) writeError();
      msFreeCharArray(tmp, n);

      QueryCoordSource = FROMUSERSHAPE;
      continue;
    }

    if(strcasecmp(Entries[i].name,"img.x") == 0) { /* mouse click */
      ImgPnt.x = getNumeric(re, Entries[i].val);
      if((ImgPnt.x > MAXCLICK) || (ImgPnt.x < MINCLICK)) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }
    if(strcasecmp(Entries[i].name,"img.y") == 0) {
      ImgPnt.y = getNumeric(re, Entries[i].val);      
      if((ImgPnt.y > MAXCLICK) || (ImgPnt.y < MINCLICK)) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }

    if(strcasecmp(Entries[i].name,"imgxy") == 0) { /* mouse click, single variable */
      if(CoordSource == FROMIMGPNT)
	continue;

      tokens = split(Entries[i].val, ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
	writeError();
      }

      ImgPnt.x = getNumeric(re, tokens[0]);
      ImgPnt.y = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);

      if((ImgPnt.x > MAXCLICK) || (ImgPnt.x < MINCLICK) || (ImgPnt.y > MAXCLICK) || (ImgPnt.y < MINCLICK)) {
	msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
	writeError();
      }

      if(CoordSource == NONE) { /* override nothing */
	CoordSource = FROMIMGPNT;
	QueryCoordSource = FROMIMGPNT;
      }
      continue;
    }

    if(strcasecmp(Entries[i].name,"imgbox") == 0) { /* mouse drag */
      tokens = split(Entries[i].val, ' ', &n);
      
      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }
      
      if(n != 4) {
	msSetError(MS_WEBERR, "Not enough arguments for imgbox.", "loadForm()");
	writeError();
      }
      
      ImgBox.minx = getNumeric(re, tokens[0]);
      ImgBox.miny = getNumeric(re, tokens[1]);
      ImgBox.maxx = getNumeric(re, tokens[2]);
      ImgBox.maxy = getNumeric(re, tokens[3]);
      
      msFreeCharArray(tokens, 4);

      if((ImgBox.minx != ImgBox.maxx) && (ImgBox.miny != ImgBox.maxy)) {
	CoordSource = FROMIMGBOX;
	QueryCoordSource = FROMIMGBOX;
      }
      continue;
    }

    if(strcasecmp(Entries[i].name,"imgshape") == 0) {
      lineObj line={0,NULL};
      char **tmp=NULL;
      int n, j;
      
      tmp = split(Entries[i].val, ' ', &n);

      if((line.point = (pointObj *)malloc(sizeof(pointObj)*(n/2))) == NULL) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }
      line.numpoints = n/2;

      for(j=0; j<n/2; j++) {
	line.point[j].x = atof(tmp[2*j]);
	line.point[j].y = atof(tmp[2*j+1]);
      }

      if(msAddLine(&SelectShape, &line) == -1) writeError();
      msFreeCharArray(tmp, n);

      QueryCoordSource = FROMIMGSHAPE;
      continue;
    }

    if(strcasecmp(Entries[i].name,"ref.x") == 0) { /* mouse click: reference image */
      RefPnt.x = getNumeric(re, Entries[i].val);      
      if((RefPnt.x > MAXCLICK) || (RefPnt.x < MINCLICK)) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      CoordSource = FROMREFPNT;
      continue;
    }
    if(strcasecmp(Entries[i].name,"ref.y") == 0) {
      RefPnt.y = getNumeric(re, Entries[i].val); 
      if((RefPnt.y > MAXCLICK) || (RefPnt.y < MINCLICK)) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(Entries[i].name,"refxy") == 0) { /* mouse click: reference image, single variable */
      tokens = split(Entries[i].val, ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
	writeError();
      }

      RefPnt.x = getNumeric(re, tokens[0]);
      RefPnt.y = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if((RefPnt.x > MAXCLICK) || (RefPnt.x < MINCLICK) || (RefPnt.y > MAXCLICK) || (RefPnt.y < MINCLICK)) {
	msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
	writeError();
      }
      
      CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(Entries[i].name,"buffer") == 0) { /* radius (map units), actually 1/2 rectangle side */
      Buffer = getNumeric(re, Entries[i].val);      
      CoordSource = FROMBUF;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }

    if(strcasecmp(Entries[i].name,"scale") == 0) { /* scale for new map */
      Map->scale = getNumeric(re, Entries[i].val);      
      if(Map->scale <= 0) {
	msSetError(MS_WEBERR, "Scale out of range.", "loadForm()");
	writeError();
      }
      CoordSource = FROMSCALE;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }
    
    if(strcasecmp(Entries[i].name,"imgsize") == 0) { /* size of existing image (pixels) */
      tokens = split(Entries[i].val, ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for imgsize.", "loadForm()");
	writeError();
      }

      ImgCols = getNumeric(re, tokens[0]);
      ImgRows = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(ImgCols > MAXIMGSIZE || ImgRows > MAXIMGSIZE || ImgCols < 0 || ImgRows < 0) {
	msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
	writeError();
      }
 
      continue;
    }

    if(strcasecmp(Entries[i].name,"mapsize") == 0) { /* size of new map (pixels) */
      tokens = split(Entries[i].val, ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 2) {
	msSetError(MS_WEBERR, "Not enough arguments for mapsize.", "loadForm()");
	writeError();
      }

      Map->width = getNumeric(re, tokens[0]);
      Map->height = getNumeric(re, tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(Map->width > MAXIMGSIZE || Map->height > MAXIMGSIZE || Map->width < 0 || Map->height < 0) {
	msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strncasecmp(Entries[i].name,"layers", 6) == 0) { /* turn a group of layers on */
      int num_layers=0, l;
      char **layers=NULL;

      layers = split(Entries[i].val, ' ', &(num_layers));
      for(l=0; l<num_layers; l++)
	Layers[NumLayers+l] = strdup(layers[l]);
      NumLayers += l;

      msFreeCharArray(layers, num_layers);
      num_layers = 0;
      continue;
    }

    if(strncasecmp(Entries[i].name,"layer", 5) == 0) { /* turn a layer on */
      Layers[NumLayers] = strdup(Entries[i].val);
      NumLayers++;
      continue;
    }

    if(strcasecmp(Entries[i].name,"qlayer") == 0) { /* layer to query (i.e search) */     
      QueryLayer = strdup(Entries[i].val);
      continue;
    }

    if(strcasecmp(Entries[i].name,"slayer") == 0) { /* layer to select (for feature based search) */     
      SelectLayer = strdup(Entries[i].val);
      continue;
    }

    if(strcasecmp(Entries[i].name,"item") == 0) { /* search item */
      Item = strdup(Entries[i].val);
      continue;
    }

    if(strcasecmp(Entries[i].name,"value") == 0) { /* search expression */
      if(!Value)
	Value = strdup(Entries[i].val);
      else { /* need to append */
	tmpstr = strdup(Value);
	free(Value);
	Value = (char *)malloc(strlen(tmpstr)+strlen(Entries[i].val)+2);
	sprintf(Value, "%s|%s", tmpstr, Entries[i].val);
	free(tmpstr);
      }
      continue;
    }

    if(strcasecmp(Entries[i].name,"tile") == 0) { /* shapefile tile directory */
      free(Map->tile);
      Map->tile = strdup(Entries[i].val);
      continue;
    }

    if(strcasecmp(Entries[i].name,"template") == 0) { /* template file */
      free(Map->web.template);
      Map->web.template = strdup(Entries[i].val);      
      continue;
    }

#ifdef USE_EGIS

    // OV - egis - additional token "none" is defined to create somewhat
    // mutual exculsiveness between mapserver and egis
 
    if(strcasecmp(Entries[i].name,"egis") == 0)
    {
        if(strcasecmp(Entries[i].val,"none") != 0)
        {
                Mode = PROCESSING;
        }
        continue;
    }
#endif

    if(strcasecmp(Entries[i].name,"mode") == 0) { /* set operation mode */
      if(strcasecmp(Entries[i].val,"browse") == 0) {
        Mode = BROWSE;
        continue;
      }
      if(strcasecmp(Entries[i].val,"zoomin") == 0) {
	ZoomDirection = 1;
        Mode = BROWSE;
        continue;
      }
      if(strcasecmp(Entries[i].val,"zoomout") == 0) {
	ZoomDirection = -1;
        Mode = BROWSE;
        continue;
      }
      if(strcasecmp(Entries[i].val,"featurequery") == 0) {
        Mode = FEATUREQUERY;
        continue;
      }
      if(strcasecmp(Entries[i].val,"featurenquery") == 0) {
        Mode = FEATURENQUERY;
        continue;
      }
      if(strcasecmp(Entries[i].val,"itemquery") == 0) {
        Mode = ITEMQUERY;
        continue;
      }
      if(strcasecmp(Entries[i].val,"query") == 0) {
        Mode = QUERY;
        continue;
      }
      if(strcasecmp(Entries[i].val,"itemnquery") == 0) {
        Mode = ITEMNQUERY;
        continue;
      }
      if(strcasecmp(Entries[i].val,"nquery") == 0) {
        Mode = NQUERY;
        continue;
      }
      if(strcasecmp(Entries[i].val,"featurequerymap") == 0) {
        Mode = FEATUREQUERYMAP;
        continue;
      }
      if(strcasecmp(Entries[i].val,"featurenquerymap") == 0) {
        Mode = FEATURENQUERYMAP;
        continue;
      }
      if(strcasecmp(Entries[i].val,"itemquerymap") == 0) {
        Mode = ITEMQUERYMAP;
        continue;
      }
      if(strcasecmp(Entries[i].val,"querymap") == 0) {
        Mode = QUERYMAP;
        continue;
      }
      if(strcasecmp(Entries[i].val,"itemnquerymap") == 0) {
        Mode = ITEMNQUERYMAP;
        continue;
      }
      if(strcasecmp(Entries[i].val,"nquerymap") == 0) {
        Mode = NQUERYMAP;
        continue;
      }
      if(strcasecmp(Entries[i].val,"map") == 0) {
        Mode = MAP;
        continue;
      }
      if(strcasecmp(Entries[i].val,"legend") == 0) {
        Mode = LEGEND;
        continue;
      }
      if(strcasecmp(Entries[i].val,"scalebar") == 0) {
        Mode = SCALEBAR;
        continue;
      }
      if(strcasecmp(Entries[i].val,"reference") == 0) {
        Mode = REFERENCE;
        continue;
      }
      if(strcasecmp(Entries[i].val,"coordinate") == 0) {
        Mode = COORDINATE;
        continue;
      }

#ifdef USE_EGIS
      //OV -egis- may be unsafe - test it properly for side effects- raju

      if(strcasecmp(Entries[i].val,"none") == 0) {
        continue;
      }

      // OV - Defined any new modes?
      errLogMsg[0] = '\0';
      sprintf(errLogMsg, "Invalid mode :: %s", Entries[i].val);

#endif

      msSetError(MS_WEBERR, "Invalid mode.", "loadForm()");
      writeError();
    }

    if(strncasecmp(Entries[i].name,"map_",4) == 0) {
      if(msLoadMapString(Map, Entries[i].name, Entries[i].val) == -1)
	writeError();
      continue;
    }
  }

  regfree(&re);

  if(ZoomSize != 0) { /* use direction and magnitude to calculate zoom */
    if(ZoomDirection == 0) {
      fZoom = 1;
    } else {
      fZoom = ZoomSize*ZoomDirection;
      if(fZoom < 0)
	fZoom = 1.0/MS_ABS(fZoom);
    }
  } else { /* use single value for zoom */
    if((Zoom >= -1) && (Zoom <= 1)) {
      fZoom = 1; /* pan */
    } else {
      if(Zoom < 0)
	fZoom = 1.0/MS_ABS(Zoom);
      else
	fZoom = Zoom;      
    }
  }

  if(ImgRows == -1) ImgRows = Map->height;
  if(ImgCols == -1) ImgCols = Map->width;  
  if(Map->height == -1) Map->height = ImgRows;
  if(Map->width == -1) Map->width = ImgCols;
}

/*
** is a particular layer on
*/
int isOn(char *name, char *group)
{
  int i;

  for(i=0;i<NumLayers;i++) {
    if(name && strcmp(Layers[i], name) == 0)  return(MS_TRUE);
    if(group && strcmp(Layers[i], group) == 0) return(MS_TRUE);
  }

  return(MS_FALSE);
}

/*
** sets the map extent under a variety of scenarios
*/
void setExtent() 
{
  double cellx,celly,cellsize;

  switch(CoordSource) {
  case FROMUSERBOX: /* user passed in a map extent */
    break;
  case FROMIMGBOX: /* fully interactive web, most likely with java front end */
    cellx = (ImgExt.maxx-ImgExt.minx)/(ImgCols-1);
    celly = (ImgExt.maxy-ImgExt.miny)/(ImgRows-1);
    Map->extent.minx = ImgExt.minx + cellx*ImgBox.minx;
    Map->extent.maxx = ImgExt.minx + cellx*ImgBox.maxx;
    Map->extent.miny = ImgExt.maxy - celly*ImgBox.maxy;
    Map->extent.maxy = ImgExt.maxy - celly*ImgBox.miny;
    break;
  case FROMIMGPNT:
    cellx = (ImgExt.maxx-ImgExt.minx)/(ImgCols-1);
    celly = (ImgExt.maxy-ImgExt.miny)/(ImgRows-1);
    MapPnt.x = ImgExt.minx + cellx*ImgPnt.x;
    MapPnt.y = ImgExt.maxy - celly*ImgPnt.y;
    Map->extent.minx = MapPnt.x - .5*((ImgExt.maxx - ImgExt.minx)/fZoom);
    Map->extent.miny = MapPnt.y - .5*((ImgExt.maxy - ImgExt.miny)/fZoom);
    Map->extent.maxx = MapPnt.x + .5*((ImgExt.maxx - ImgExt.minx)/fZoom);
    Map->extent.maxy = MapPnt.y + .5*((ImgExt.maxy - ImgExt.miny)/fZoom);
    break;
  case FROMREFPNT:
    cellx = (Map->reference.extent.maxx -  Map->reference.extent.minx)/(Map->reference.width-1);
    celly = (Map->reference.extent.maxy -  Map->reference.extent.miny)/(Map->reference.height-1);
    MapPnt.x = Map->reference.extent.minx + cellx*RefPnt.x;
    MapPnt.y = Map->reference.extent.maxy - celly*RefPnt.y;    
    Map->extent.minx = MapPnt.x - .5*(ImgExt.maxx - ImgExt.minx);
    Map->extent.miny = MapPnt.y - .5*(ImgExt.maxy - ImgExt.miny);
    Map->extent.maxx = MapPnt.x + .5*(ImgExt.maxx - ImgExt.minx);
    Map->extent.maxy = MapPnt.y + .5*(ImgExt.maxy - ImgExt.miny);
    break;
  case FROMBUF: /* user supplied a point and a buffer */
    Map->extent.minx = MapPnt.x - Buffer;
    Map->extent.miny = MapPnt.y - Buffer;
    Map->extent.maxx = MapPnt.x + Buffer;
    Map->extent.maxy = MapPnt.y + Buffer;
    break;
  case FROMSCALE: /* user supplied a point and a scale */ 
    cellsize = (Map->scale/MS_PPI)/inchesPerUnit[Map->units];
    Map->extent.minx = MapPnt.x - cellsize*Map->width/2.0;
    Map->extent.miny = MapPnt.y - cellsize*Map->height/2.0;
    Map->extent.maxx = MapPnt.x + cellsize*Map->width/2.0;
    Map->extent.maxy = MapPnt.y + cellsize*Map->height/2.0;
    break;
  default: /* use the default in the mapfile if it exists */
    if((Map->extent.minx == Map->extent.maxx) && (Map->extent.miny == Map->extent.maxy)) {
      msSetError(MS_WEBERR, "No way to generate map extent.", "mapserv()");
      writeError();
    }    
  }

  RawExt = Map->extent; /* save unaltered extent */
}

void setExtentFromShapes() {
  int i,j;
  shapefileObj shp;
  shapeObj shape = {0,NULL};
  int found=0;
  double dx, dy;

  for(i=0; i<Map->numlayers; i++) {    
    if(QueryResults->layers[i].numresults <= 0) /* next query */
      continue;

    if(msOpenSHPFile(&shp, "rb", Map->shapepath, Map->tile, Map->layers[i].data) == -1)
      writeError();

    for(j=0; j<shp.numshapes; j++) {

      if(!msGetBit(QueryResults->layers[i].status,j))
	continue; /* next shape */

#ifdef USE_PROJ
      SHPReadShapeProj(shp.hSHP, j, &shape, &(Map->layers[i].projection), &(Map->projection));
#else
      SHPReadShape(shp.hSHP, j, &shape);
#endif

      if(!found) {
	Map->extent = shape.bounds;
	found = 1;
      } else {
	msMergeRect(&Map->extent, &shape.bounds);
      }

      msFreeShape(&shape);
    }
  }

  dx = Map->extent.maxx - Map->extent.minx;
  dy = Map->extent.maxy - Map->extent.miny;
 
  MapPnt.x = dx/2;
  MapPnt.y = dy/2;
  Map->extent.minx -= dx*EXTENT_PADDING/2.0;
  Map->extent.maxx += dx*EXTENT_PADDING/2.0;
  Map->extent.miny -= dy*EXTENT_PADDING/2.0;
  Map->extent.maxy += dy*EXTENT_PADDING/2.0;

  RawExt = Map->extent; /* save unaltered extent */

  /* NEED TO ADD A COUPLE OF POINT BASED METHODS */

  return;
}

/* NEED ERROR CHECKING HERE FOR IMGPNT or MAPPNT */
void setCoordinate()
{
  double cellx,celly;

  cellx = (ImgExt.maxx - ImgExt.minx)/(ImgCols-1);
  celly = (ImgExt.maxy - ImgExt.miny)/(ImgRows-1);

  MapPnt.x = ImgExt.minx + cellx*ImgPnt.x;
  MapPnt.y = ImgExt.maxy - celly*ImgPnt.y;

  return;
}

void returnCoordinate()
{
  msSetError(MS_NOERR, NULL, NULL);
  sprintf(ms_error.message, "Your \"<i>click</i>\" corresponds to (approximately): (%g, %g).\n", MapPnt.x, MapPnt.y);

#ifdef USE_PROJ
  if(Map->projection.proj != NULL) {
    pointObj p=MapPnt;
    msProjectPoint(Map->projection.proj, NULL, &p);
    sprintf(ms_error.message, "%s Computed lat/lon value is (%g, %g).\n", ms_error.message, p.x, p.y);
  }
#endif

  writeError();
}

void returnOneToManyJoin(int j)
{
  int k,l;
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH], substr[MS_BUFFER_LENGTH], *outstr;

  if(Query->joins[j].header != NULL) {
    if((stream = fopen(Query->joins[j].header, "r")) == NULL) {
      msSetError(MS_IOERR, Query->joins[j].header, "returnOneToManyJoin()");
      writeError();
    }

    while(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) /* echo file, no substitutions */
    printf("%s", buffer);

    fclose(stream);
  }

  if((stream = fopen(Query->joins[j].template, "r")) == NULL) { /* open main template file */
    msSetError(MS_IOERR, Query->joins[j].template, "returnOneToManyJoin()");
    writeError();
  }

  for(k=0; k<Query->joins[j].numrecords; k++) {
    while(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) { /* now on to the end of the file */
      outstr = strdup(buffer);
      
      for(l=0; l<Query->joins[j].numitems; l++) {	  
	sprintf(substr, "[%s]", Query->joins[j].items[l]);
	if(strstr(outstr, substr) != NULL) { /* do substitution */
	  outstr = gsub(outstr, substr, Query->joins[j].data[k][l]);
	}
      } /* next item */
      
      printf("%s", outstr);
      fflush(stdout);	
      free(outstr);
    }
      
    rewind(stream);
  }

  if(Query->joins[j].footer != NULL) {
    if((stream = fopen(Query->joins[j].footer, "r")) == NULL) {
      msSetError(MS_IOERR, Query->joins[j].footer, "returnOneToManyJoin()");
      writeError();
    }

    while(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) /* echo file, no substitutions */
      printf("%s", buffer);

    fclose(stream);
  }

}

void returnHTML(char *html)
{
  int i,j;
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH], substr[1024], *outstr;

  regex_t re; /* compiled regular expression to be matched */ 

#ifdef USE_PROJ
  rectObj llextent;
  pointObj llpoint;
#endif

  if(regcomp(&re, MS_TEMPLATE_EXPR, REG_EXTENDED|REG_NOSUB) != 0) {   
    msSetError(MS_REGEXERR, NULL, "returnHTML()");
    writeError();
  }

  if(regexec(&re, html, 0, NULL, 0) != 0) { /* no match */
    regfree(&re);
    msSetError(MS_WEBERR, "Malformed template name.", "returnHTML()");
    writeError();
  }
  regfree(&re);

  if((stream = fopen(html, "r")) == NULL) {
    msSetError(MS_IOERR, html, "returnHTML()");
    writeError();
  } 

#ifdef USE_PROJ
  if(Map->projection.proj != NULL) {
    llextent=Map->extent;
    llpoint=MapPnt;
    msProjectRect(Map->projection.proj, NULL, &llextent);
    msProjectPoint(Map->projection.proj, NULL, &llpoint);
  }
#endif

  while(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) { /* now on to the end of the file */

    outstr = strdup(buffer);

    if(strchr(outstr, '[') != NULL) { /* this line probably needs a replacement */    
      
      sprintf(substr, "%s", getenv("HTTP_HOST")); 
      outstr = gsub(outstr, "[host]", substr);
      sprintf(substr, "%s", getenv("SERVER_PORT"));
      outstr = gsub(outstr, "[port]", substr);

      sprintf(substr, "%s", Id);
      outstr = gsub(outstr, "[id]", substr);

      for(i=0;i<Map->numlayers;i++) { /* Layer legend and description strings */
	sprintf(substr, "[%s_desc]", Map->layers[i].name);
        if(Map->layers[i].status == MS_ON)
	  outstr = gsub(outstr, substr, Map->layers[i].description);
	else
	  outstr = gsub(outstr, substr, "");
	
	sprintf(substr, "[%s_leg]", Map->layers[i].name);
        if(Map->layers[i].status == MS_ON)
	  outstr = gsub(outstr, substr, Map->layers[i].legend);
	else
	  outstr = gsub(outstr, substr, "");
      }
      
      strcpy(substr, ""); /* Layer list for a "GET" request */
      for(i=0;i<NumLayers;i++)    
	sprintf(substr, "%s&layer=%s", substr, Layers[i]);
      outstr = gsub(outstr, "[get_layers]", substr);

      strcpy(substr, ""); /* Layer list for a  request */
      for(i=0;i<NumLayers;i++)
	sprintf(substr, "%s%s ", substr, Layers[i]);
      trimBlanks(substr);
      outstr = gsub(outstr, "[layers]", substr);
      
      for(i=0;i<Map->numlayers;i++) { /* Set form widgets (i.e. checkboxes, radio and select lists), note that default layers don't show up here */ 
        if(isOn(Map->layers[i].name, Map->layers[i].group) == MS_TRUE) {
	  sprintf(substr, "[%s_select]", Map->layers[i].name);
	  outstr = gsub(outstr, substr, "selected");
	  sprintf(substr, "[%s_check]", Map->layers[i].name);
	  outstr = gsub(outstr, substr, "checked");
        } else {
	  sprintf(substr, "[%s_select]", Map->layers[i].name);
	  outstr = gsub(outstr, substr, "");
	  sprintf(substr, "[%s_check]", Map->layers[i].name);
	  outstr = gsub(outstr, substr, "");
        }
      }

      sprintf(substr, "%f", MapPnt.x);
      outstr = gsub(outstr, "[mapx]", substr);
      sprintf(substr, "%f", MapPnt.y);
      outstr = gsub(outstr, "[mapy]", substr);

      sprintf(substr, "%f", Map->extent.minx); /* Individual mapextent elements for spatial query building */
      outstr = gsub(outstr, "[minx]", substr);
      sprintf(substr, "%f", Map->extent.maxx);
      outstr = gsub(outstr, "[maxx]", substr);
      sprintf(substr, "%f", Map->extent.miny);
      outstr = gsub(outstr, "[miny]", substr);
      sprintf(substr, "%f", Map->extent.maxy);
      outstr = gsub(outstr, "[maxy]", substr);
      sprintf(substr, "%f %f %f %f", Map->extent.minx, Map->extent.miny,  Map->extent.maxx, Map->extent.maxy);
      outstr = gsub(outstr, "[mapext]", substr);

      sprintf(substr, "%f", RawExt.minx); /* Individual raw extent elements for spatial query building */
      outstr = gsub(outstr, "[rawminx]", substr);
      sprintf(substr, "%f", RawExt.maxx);
      outstr = gsub(outstr, "[rawmaxx]", substr);
      sprintf(substr, "%f", RawExt.miny);
      outstr = gsub(outstr, "[rawminy]", substr);
      sprintf(substr, "%f", RawExt.maxy);
      outstr = gsub(outstr, "[rawmaxy]", substr);
      sprintf(substr, "%f %f %f %f", RawExt.minx, RawExt.miny,  RawExt.maxx, RawExt.maxy);
      outstr = gsub(outstr, "[rawext]", substr);

#ifdef USE_PROJ
      sprintf(substr, "%f", llpoint.x);
      outstr = gsub(outstr, "[maplon]", substr);
      sprintf(substr, "%f", llpoint.y);
      outstr = gsub(outstr, "[maplat]", substr);

      sprintf(substr, "%f", llextent.minx); /* map extent as lat/lon */
      outstr = gsub(outstr, "[minlon]", substr);
      sprintf(substr, "%f", llextent.maxx);
      outstr = gsub(outstr, "[maxlon]", substr);
      sprintf(substr, "%f", llextent.miny);
      outstr = gsub(outstr, "[minlat]", substr);
      sprintf(substr, "%f", llextent.maxy);
      outstr = gsub(outstr, "[maxlat]", substr);
      sprintf(substr, "%f %f %f %f", llextent.minx, llextent.miny,  llextent.maxx, llextent.maxy);
      outstr = gsub(outstr, "[mapext_latlon]", substr);
#endif
          
      sprintf(substr, "%d %d", Map->width, Map->height);
      outstr = gsub(outstr, "[mapsize]", substr);
      sprintf(substr, "%d", Map->width);
      outstr = gsub(outstr, "[mapwidth]", substr);
      sprintf(substr, "%d", Map->height);
      outstr = gsub(outstr, "[mapheight]", substr);

      sprintf(substr, "%f", Map->scale);
      outstr = gsub(outstr, "[scale]", substr);

      sprintf(substr, "%.1f %.1f", (Map->width-1)/2.0, (Map->height-1)/2.0);
      outstr = gsub(outstr, "[center]", substr);
      sprintf(substr, "%.1f", (Map->width-1)/2.0);
      outstr = gsub(outstr, "[center_x]", substr);
      sprintf(substr, "%.1f", (Map->height-1)/2.0);
      outstr = gsub(outstr, "[center_y]", substr);

      for(i=-1;i<=1;i++) { /* make zoom direction persistant */
	if(ZoomDirection == i) {
	  sprintf(substr, "[zoomdir_%d_select]", i);
	  outstr = gsub(outstr, substr, "selected");
	  sprintf(substr, "[zoomdir_%d_check]", i);
	  outstr = gsub(outstr, substr, "checked");
	} else {
	  sprintf(substr, "[zoomdir_%d_select]", i);
	  outstr = gsub(outstr, substr, "");
	  sprintf(substr, "[zoomdir_%d_check]", i);
	  outstr = gsub(outstr, substr, "");
	}
      }
      
      for(i=MINZOOM;i<=MAXZOOM;i++) { /* make zoom persistant */
	if(Zoom == i) {
	  sprintf(substr, "[zoom_%d_select]", i);
	  outstr = gsub(outstr, substr, "selected");
	  sprintf(substr, "[zoom_%d_check]", i);
	  outstr = gsub(outstr, substr, "checked");
	} else {
	  sprintf(substr, "[zoom_%d_select]", i);
	  outstr = gsub(outstr, substr, "");
	  sprintf(substr, "[zoom_%d_check]", i);
	  outstr = gsub(outstr, substr, "");
	}
      }
 
      if(SaveQuery) {
        sprintf(substr, "%s%s%s.qf", Map->web.imageurl, Map->name, Id);
        outstr = gsub(outstr, "[queryfile]", substr);
      }

      if(SaveMap) {
        sprintf(substr, "%s%s%s.map", Map->web.imageurl, Map->name, Id);
        outstr = gsub(outstr, "[map]", substr);
      }

      sprintf(substr, "%s%s%s.%s", Map->web.imageurl, Map->name, Id, outputImageType[Map->imagetype]);
      outstr = gsub(outstr, "[img]", substr);
      sprintf(substr, "%s%sref%s.%s", Map->web.imageurl, Map->name, Id, outputImageType[Map->imagetype]);
      outstr = gsub(outstr, "[ref]", substr);
      sprintf(substr, "%s%sleg%s.%s", Map->web.imageurl, Map->name, Id, outputImageType[Map->imagetype]);
      outstr = gsub(outstr, "[legend]", substr);
      sprintf(substr, "%s%ssb%s.%s", Map->web.imageurl, Map->name, Id, outputImageType[Map->imagetype]);
      outstr = gsub(outstr, "[scalebar]", substr);

      /*
      ** These are really for situations with multiple result sets only, but often used in templates
      */
      sprintf(substr, "%d", NR); /* total number of results */
      outstr = gsub(outstr, "[nr]", substr);
      sprintf(substr, "%d", NLR); /* total number of results within a layer */
      outstr = gsub(outstr, "[nlr]", substr);
      sprintf(substr, "%d", NL); /* total number of layers with results */
      outstr = gsub(outstr, "[nl]", substr);
      sprintf(substr, "%d", RN); /* result number */
      outstr = gsub(outstr, "[rn]", substr);
      sprintf(substr, "%d", LRN); /* result number within a layer */
      outstr = gsub(outstr, "[lrn]", substr);      
      if(CL) /* current layer */
	outstr = gsub(outstr, "[cl]", CL);      
      if(CD) /* current description */
	outstr = gsub(outstr, "[cd]", CD);

      /*
      ** If query mode, need to do a bit more
      */
      if(Mode == QUERY) {	

	sprintf(substr, "%f %f", (ShpExt.maxx+ShpExt.minx)/2, (ShpExt.maxy+ShpExt.miny)/2); 
	outstr = gsub(outstr, "[shpmid]", substr);
	sprintf(substr, "%f", (ShpExt.maxx+ShpExt.minx)/2);
        outstr = gsub(outstr, "[shpmidx]", substr);
	sprintf(substr, "%f", (ShpExt.maxy+ShpExt.miny)/2);
        outstr = gsub(outstr, "[shpmidy]", substr);

	sprintf(substr, "%f %f %f %f", ShpExt.minx, ShpExt.miny,  ShpExt.maxx, ShpExt.maxy);
	outstr = gsub(outstr, "[shpext]", substr);
	sprintf(substr, "%f", ShpExt.minx);
	outstr = gsub(outstr, "[shpminx]", substr);
	sprintf(substr, "%f", ShpExt.miny);
	outstr = gsub(outstr, "[shpminy]", substr);
	sprintf(substr, "%f", ShpExt.maxx);
	outstr = gsub(outstr, "[shpmaxx]", substr);
	sprintf(substr, "%f", ShpExt.maxy);
	outstr = gsub(outstr, "[shpmaxy]", substr);

	sprintf(substr, "%d", ShpIdx);
	outstr = gsub(outstr, "[shpidx]", substr);

	for(i=0;i<Query->numitems;i++) {	 
	  sprintf(substr, "[%s]", Query->items[i]);
	  if(strstr(outstr, substr) != NULL) { /* do substitution */
	    outstr = gsub(outstr, substr, Query->data[i]);
	  }
	} /* next item */

	for(i=0; i<Query->numjoins; i++) {
	  if(Query->joins[i].type == MS_MULTIPLE) {
	    sprintf(substr, "[%s]", Query->joins[i].name);
	    if(strstr(outstr, substr)) {
	      outstr = gsub(outstr, substr, "");
	      returnOneToManyJoin(i);
	    }
	  }

	  // some common data may exist for all matches so multiples are allowed here
	  for(j=0;j<Query->joins[i].numitems; j++) { 
	    sprintf(substr, "[%s_%s]", Query->joins[i].name, Query->joins[i].items[j]);
	    if(strstr(outstr, substr))
	      outstr = gsub(outstr, substr, Query->joins[i].data[0][j]);
	    sprintf(substr, "[%s]", Query->joins[i].items[j]);
	    if(strstr(outstr, substr))
	      outstr = gsub(outstr, substr, Query->joins[i].data[0][j]);
	  }
	} /* next join */
      }

      for(i=0;i<NumEntries;i++) { 
        sprintf(substr, "[%s]", Entries[i].name);
        outstr = gsub(outstr, substr, Entries[i].val); /* do substitution */
      }

    } /* end substitution */

    printf("%s", outstr);
    fflush(stdout);

    free(outstr);
  } /* next template line */

  fclose(stream);
}

void returnURL(char *url)
{
  char *outstr;
  char substr[256]; /* should be large enough */
  int i,j;

#ifdef USE_PROJ
  rectObj llextent;
  pointObj llpoint;
#endif

  if(url == NULL) {
    msSetError(MS_WEBERR, "Empty URL.", "returnURL()");
    writeError();
  }

#ifdef USE_PROJ
  if(Map->projection.proj != NULL) {
    llextent=Map->extent;
    llpoint=MapPnt;
    msProjectRect(Map->projection.proj, NULL, &llextent);
    msProjectPoint(Map->projection.proj, NULL, &llpoint);
  }  
#endif

  outstr = strdup(url);

  sprintf(substr, "%s", getenv("HTTP_HOST")); 
  outstr = gsub(outstr, "[host]", substr);
  sprintf(substr, "%s", getenv("SERVER_PORT"));
  outstr = gsub(outstr, "[port]", substr);

  sprintf(substr, "%s", Id);
  outstr = gsub(outstr, "[id]", substr);

  sprintf(substr, "%f", Map->scale);
  outstr = gsub(outstr, "[scale]", substr);
  
  sprintf(substr, "%f", MapPnt.x);
  outstr = gsub(outstr, "[mapx]", substr);
  sprintf(substr, "%f", MapPnt.y);
  outstr = gsub(outstr, "[mapy]", substr);

  sprintf(substr, "%d %d", Map->width, Map->height);
  outstr = gsub(outstr, "[mapsize]", (char *)encode_url(substr));
  sprintf(substr, "%d", Map->width);
  outstr = gsub(outstr, "[mapwidth]", substr);
  sprintf(substr, "%d", Map->height);
  outstr = gsub(outstr, "[mapheight]", substr);
  
  sprintf(substr, "%f", Map->extent.minx); /* Individual mapextent elements for spatial query building */
  outstr = gsub(outstr, "[minx]", substr);
  sprintf(substr, "%f", Map->extent.maxx);
  outstr = gsub(outstr, "[maxx]", substr);
  sprintf(substr, "%f", Map->extent.miny);
  outstr = gsub(outstr, "[miny]", substr);
  sprintf(substr, "%f", Map->extent.maxy);
  outstr = gsub(outstr, "[maxy]", substr);
  sprintf(substr, "%f %f %f %f", Map->extent.minx, Map->extent.miny,  Map->extent.maxx, Map->extent.maxy);
  outstr = gsub(outstr, "[mapext]", (char *)encode_url(substr));

  sprintf(substr, "%f", RawExt.minx); /* Individual raw extent elements for spatial query building */
  outstr = gsub(outstr, "[rawminx]", substr);
  sprintf(substr, "%f", RawExt.maxx);
  outstr = gsub(outstr, "[rawmaxx]", substr);
  sprintf(substr, "%f", RawExt.miny);
  outstr = gsub(outstr, "[rawminy]", substr);
  sprintf(substr, "%f", RawExt.maxy);
  outstr = gsub(outstr, "[rawmaxy]", substr);
  sprintf(substr, "%f %f %f %f", RawExt.minx, RawExt.miny,  RawExt.maxx, RawExt.maxy);
  outstr = gsub(outstr, "[rawext]", (char *)encode_url(substr));
  
  strcpy(substr, ""); /* Layer list for a "GET" request */
  for(i=0;i<NumLayers;i++)    
    sprintf(substr, "%s&layer=%s", substr, Layers[i]);
  outstr = gsub(outstr, "[get_layers]", substr);
  
  strcpy(substr, ""); /* Layer list for a request */
  for(i=0;i<NumLayers;i++)
    sprintf(substr, "%s%s ", substr, Layers[i]);
  trimBlanks(substr);
  outstr = gsub(outstr, "[layers]",  (char *)encode_url(substr));

#ifdef USE_PROJ
  sprintf(substr, "%f", llpoint.x);
  outstr = gsub(outstr, "[maplon]", substr);
  sprintf(substr, "%f", llpoint.y);
  outstr = gsub(outstr, "[maplat]", substr);
  
  sprintf(substr, "%f", llextent.minx); /* map extent as lat/lon */
  outstr = gsub(outstr, "[minlon]", substr);
  sprintf(substr, "%f", llextent.maxx);
  outstr = gsub(outstr, "[maxlon]", substr);
  sprintf(substr, "%f", llextent.miny);
  outstr = gsub(outstr, "[minlat]", substr);
  sprintf(substr, "%f", llextent.maxy);
  outstr = gsub(outstr, "[maxlat]", substr);
  sprintf(substr, "%f %f %f %f", llextent.minx, llextent.miny,  llextent.maxx, llextent.maxy);
  outstr = gsub(outstr, "[mapext_latlon]", (char *)encode_url(substr));
#endif
  
  if(Mode == QUERY) { /* need to do a bit more with query results */
      
    sprintf(substr, "%f %f", (ShpExt.maxx+ShpExt.minx)/2, (ShpExt.maxy+ShpExt.miny)/2); 
    outstr = gsub(outstr, "[shpmid]", (char *)encode_url(substr));
    sprintf(substr, "%f", (ShpExt.maxx+ShpExt.minx)/2);
    outstr = gsub(outstr, "[shpmidx]", substr);
    sprintf(substr, "%f", (ShpExt.maxy+ShpExt.miny)/2);
    outstr = gsub(outstr, "[shpmidy]", substr);
    
    sprintf(substr, "%f %f %f %f", ShpExt.minx, ShpExt.miny,  ShpExt.maxx, ShpExt.maxy);
    outstr = gsub(outstr, "[shpext]", (char *)encode_url(substr));
    sprintf(substr, "%f", ShpExt.minx);
    outstr = gsub(outstr, "[shpminx]", substr);
    sprintf(substr, "%f", ShpExt.miny);
    outstr = gsub(outstr, "[shpminy]", substr);
    sprintf(substr, "%f", ShpExt.maxx);
    outstr = gsub(outstr, "[shpmaxx]", substr);
    sprintf(substr, "%f", ShpExt.maxy);
    outstr = gsub(outstr, "[shpmaxy]", substr);

    sprintf(substr, "%d", ShpIdx);
    outstr = gsub(outstr, "[shpidx]", substr);
    
    for(i=0;i<Query->numitems;i++) {	  
      sprintf(substr, "[%s]", Query->items[i]);
      if(strstr(outstr, substr) != NULL) { /* do substitution */
	outstr = gsub(outstr, substr, (char *)encode_url(Query->data[i]));
      }
    } /* next item */
    
    for(i=0; i<Query->numjoins; i++) {
      for(j=0;j<Query->joins[i].numitems; j++) {
	if(Query->joins[i].type == MS_SINGLE) {
	  sprintf(substr, "[%s_%s]", Query->joins[i].name, Query->joins[i].items[j]);
	  if(strstr(outstr, substr))
	    outstr = gsub(outstr, substr, (char *)encode_url(Query->joins[i].data[0][j]));
	  sprintf(substr, "[%s]", Query->joins[i].items[j]);
	  if(strstr(outstr, substr))
	    outstr = gsub(outstr, substr, (char *)encode_url(Query->joins[i].data[0][j]));
	}
      }
    } /* next join */
  }
  
  for(i=0;i<NumEntries;i++) { /* allow for echoing of user Entries */
    sprintf(substr, "[%s]", Entries[i].name);
    outstr = gsub(outstr, substr, (char *)encode_url(Entries[i].val));
  }

  redirect(outstr); /* send the user to the new URL */

  /*
  ** Now some cleanup
  */  
  free(outstr);

  return; /* all done */
}

void returnQuery()
{
  int i,j,k;
  double dx,dy;

  shapefileObj shp;
  shapeObj shape={0,NULL};
  int item;

  if((Mode == ITEMQUERY) || (Mode == QUERY)) { // may need to handle a URL result set

    Mode = QUERY; /* simplifies life */

    for(i=0;i<QueryResults->numlayers;i++) { /* find the first result */      
      if(QueryResults->layers[i].numresults > 0)
	break;
    }
	  
    for(j=0; j<QueryResults->layers[i].numshapes; j++) {
      if(msGetBit(QueryResults->layers[i].status,j)) // found it
	break;
    }

    Query = &(Map->layers[i].query[(int)QueryResults->layers[i].index[j]]);

    if(TEMPLATE_TYPE(Query->template) == MS_URL) { 

      if(msOpenSHPFile(&shp, "rb", Map->shapepath, Map->tile, Map->layers[i].data) == -1) writeError();

#ifdef USE_PROJ
      SHPReadShapeProj(shp.hSHP, j, &shape, &(Map->layers[i].projection), &(Map->projection));
#else
      SHPReadShape(shp.hSHP, j, &shape); /* get feature extent */
#endif

      dx = shape.bounds.maxx - shape.bounds.minx;
      dy = shape.bounds.maxy - shape.bounds.miny;
      ShpExt.minx = shape.bounds.minx - dx*EXTENT_PADDING/2.0;
      ShpExt.maxx = shape.bounds.maxx + dx*EXTENT_PADDING/2.0;
      ShpExt.miny = shape.bounds.miny - dy*EXTENT_PADDING/2.0;
      ShpExt.maxy = shape.bounds.maxy + dy*EXTENT_PADDING/2.0;
      ShpIdx = j;

      Query->numitems = DBFGetFieldCount(shp.hDBF);
      if((Query->items = msGetDBFItems(shp.hDBF)) == NULL) /* save items names */
	writeError();
      
      if((Query->data = msGetDBFValues(shp.hDBF, j)) == NULL)
	writeError();
      
      for(i=0; i<Query->numjoins; i++) {
	if((item = msGetItemIndex(shp.hDBF, Query->joins[i].from)) == -1)
	  writeError();
	Query->joins[i].match = strdup(DBFReadStringAttribute(shp.hDBF, j, item));
	if(msJoinDBFTables(&(Query->joins[i]), Map->shapepath, Map->tile) == -1)
	  writeError();
      }
    
      returnURL(Query->template);
      
      
      msFreeShape(&shape);
      msCloseSHPFile(&shp);

      return;
    }
  }
   
  Mode = QUERY; /* simplifies life */
  
  NR = QueryResults->numresults;
  NL = QueryResults->numquerylayers;

  printf("Content-type: text/html%c%c", 10, 10); /* write MIME header */
  writeVersion();
  fflush(stdout);
  
  if(Map->web.header != NULL) {
    Mode = BROWSE; returnHTML(Map->web.header); Mode = QUERY;
  }
  
  for(i=(QueryResults->numlayers-1); i>=0; i--) {

    if(QueryResults->layers[i].numresults == 0) /* next query */
      continue;

    NLR = QueryResults->layers[i].numresults;
    CL = Map->layers[i].name;
    CD = Map->layers[i].description;

    if(Map->layers[i].header != NULL) {
      Mode = BROWSE; returnHTML(Map->layers[i].header); Mode = QUERY;
    }
    
    if(msOpenSHPFile(&shp, "rb", Map->shapepath, Map->tile, Map->layers[i].data) == -1) 
      writeError();

    for(j=0; j<shp.numshapes; j++) {      
      
      if(!msGetBit(QueryResults->layers[i].status,j))
	continue;
      
#ifdef USE_PROJ
      SHPReadShapeProj(shp.hSHP, j, &shape, &(Map->layers[i].projection), &(Map->projection));
#else       
      SHPReadShape(shp.hSHP, j, &shape); /* get feature extent */
#endif
      
      dx = shape.bounds.maxx - shape.bounds.minx;
      dy = shape.bounds.maxy - shape.bounds.miny;
      ShpExt.minx = shape.bounds.minx - dx*EXTENT_PADDING/2.0;
      ShpExt.maxx = shape.bounds.maxx + dx*EXTENT_PADDING/2.0;
      ShpExt.miny = shape.bounds.miny - dy*EXTENT_PADDING/2.0;
      ShpExt.maxy = shape.bounds.maxy + dy*EXTENT_PADDING/2.0;
      ShpIdx = j;
      
      RN++;
      LRN = j+1;
      
      Query = &(Map->layers[i].query[(int)QueryResults->layers[i].index[j]]);
      
      if(!Query->items) { /* only load once */
	Query->numitems = DBFGetFieldCount(shp.hDBF);
	Query->items = msGetDBFItems(shp.hDBF);
	if(!Query->items)
	  writeError();
      }

      if(Query->data) /* free old data */
	msFreeCharArray(Query->data, Query->numitems);
      Query->data = msGetDBFValues(shp.hDBF, j);
      if(!Query->data)
	writeError();
      
      for(k=0; k<Query->numjoins; k++) {
	if((item = msGetItemIndex(shp.hDBF, Query->joins[k].from)) == -1)
	  writeError();
	Query->joins[k].match = strdup(DBFReadStringAttribute(shp.hDBF, j, item));
	if(msJoinDBFTables(&(Query->joins[k]), Map->shapepath, Map->tile) == -1)
	  writeError();
      }
      
      returnHTML(Query->template);
      msFreeShape(&shape);
    }

    if(Map->layers[i].footer) {
      Mode = BROWSE; returnHTML(Map->layers[i].footer); Mode = QUERY;
    }

    msCloseSHPFile(&shp);
  }

  if(Map->web.footer) {      
    Mode = BROWSE; returnHTML(Map->web.footer); Mode = QUERY;
  }
  
  return;
}

/*
**
** Start of main program
**
*/
int main(int argc, char *argv[]) {
    int i,j;
    char buffer[1024];
    gdImagePtr img=NULL; /* intialize the point to NULL */

#ifdef USE_EGIS
    int status;		//OV -egis-

    // OV -egis- Initialize egis error log file here...
    initErrLog("/export/home/tmp/msError.log");
    writeErrLog("ErrorLogfile is initialized\n");
    fflush(fpErrLog);
#endif

    NumEntries = loadEntries(Entries);

    loadForm();

    sprintf(Id, "%ld%d",(long)time(NULL),(int)getpid());

    if(SaveMap) {
      sprintf(buffer, "%s%s%s.map", Map->web.imagepath, Map->name, Id);
      if(msSaveMap(Map, buffer) == -1) writeError();
    }

    if((CoordSource == FROMIMGPNT) || (CoordSource == FROMIMGBOX)) /* make sure extent of existing image matches shape of image */
      Map->cellsize = msAdjustExtent(&ImgExt, ImgCols, ImgRows);

    /*
    ** For each layer lets set layer status
    */
    for(i=0;i<Map->numlayers;i++) {
      if((Map->layers[i].status != MS_DEFAULT) && (Map->layers[i].status != MS_QUERY)) {
	if(isOn(Map->layers[i].name, Map->layers[i].group) == MS_TRUE) /* Set layer status */
	  Map->layers[i].status = MS_ON;
	else
	  Map->layers[i].status = MS_OFF;
      }     
    }

    if(CoordSource == FROMREFPNT) /* force browse mode if the reference coords are set */
      Mode = BROWSE;

    switch(Mode) {

#ifdef USE_EGIS
        // OV - Added this new section for processing capabilities
        case PROCESSING:
        {
                setExtent();
                errLogMsg[0] = '\0';
                sprintf(errLogMsg, "Map Coordinates: x %f, y %f\n",
                                                        MapPnt.x, MapPnt.y);
                writeErrLog(errLogMsg);
 
                status = egisl(Map, Entries, NumEntries, CoordSource);
                // printf("Numerical Window - %f %f\n", ImgPnt.x, ImgPnt.y);
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
        }
        break; 
#endif

    case COORDINATE:
      setCoordinate(); /* mouse click => map coord */      
      returnCoordinate(MapPnt);
      break;
    case FEATUREQUERY:
    case FEATURENQUERY:

      if((i = msGetLayerIndex(Map, SelectLayer)) == -1) { /* force the selection layer on */
	msSetError(MS_WEBERR, "Selection layer not set or references an invalid layer.", "mapserv()"); 
	writeError();
      } else {
	Map->layers[i].status = MS_ON;
      }
 
      if((i = msGetLayerIndex(Map, QueryLayer)) != -1) /* force the query layer on */
	Map->layers[i].status = MS_ON;

      if(QueryCoordSource == NONE) { /* use item/value */

        if((Item == NULL) || (Value == NULL)) {
	  msSetError(MS_WEBERR, "Missing \"item\"\\\"value\" pairing.", "mapserv()");
	  writeError();
	}

        if(Mode == FEATUREQUERY) {
          if((QueryResults = msQueryUsingItem(Map, SelectLayer, MS_SINGLE, Item, Value)) == NULL) writeError();
        } else {
          if((QueryResults = msQueryUsingItem(Map, SelectLayer, MS_MULTIPLE, Item, Value)) == NULL) writeError();
	}

      } else { /* use coordinates */

	if(Mode == FEATUREQUERY) {
	  switch(QueryCoordSource) {
	  case FROMIMGPNT:
	    Map->extent = ImgExt; /* use the existing map extent */	
	    setCoordinate();
	    if((QueryResults = msQueryUsingPoint(Map, SelectLayer, MS_SINGLE, MapPnt, 0)) == NULL) writeError();
	    break;
	  case FROMUSERPNT: /* only a buffer makes sense */
	    if(Buffer == -1) {
	      msSetError(MS_WEBERR, "Point given but no search buffer specified.", "mapserv()");
	      writeError();
	    }
	    if((QueryResults = msQueryUsingPoint(Map, SelectLayer, MS_SINGLE, MapPnt, Buffer)) == NULL) writeError();
	    break;
	  default:
	    msSetError(MS_WEBERR, "No way to the initial search, not enough information.", "mapserv()");
	    writeError();
	    break;
	  }	  
	} else { /* FEATURENQUERY */
	  switch(QueryCoordSource) {
	  case FROMIMGPNT:
	    Map->extent = ImgExt; /* use the existing map extent */	
	    setCoordinate();
	    if((QueryResults = msQueryUsingPoint(Map, SelectLayer, MS_MULTIPLE, MapPnt, 0)) == NULL) writeError();
	    break;	 
	  case FROMIMGBOX:
	    break;
	  case FROMUSERPNT: /* only a buffer makes sense */
	    if((QueryResults = msQueryUsingPoint(Map, SelectLayer, MS_MULTIPLE, MapPnt, Buffer)) == NULL) writeError();
	  default:
	    setExtent();
	    if((QueryResults = msQueryUsingRect(Map, SelectLayer, &Map->extent)) == NULL) writeError();
	    break;
	  }
	} /* end switch */
      } 
         
      if(msQueryUsingFeatures(Map, QueryLayer, QueryResults) == -1) /* now feature query */
	writeError();
      
      returnQuery();

      if(SaveQuery) {
	sprintf(buffer, "%s%s%s%s", Map->web.imagepath, Map->name, Id, MS_QUERY_EXTENSION);
	if(msSaveQuery(QueryResults, buffer) == -1) 
	  writeError();
      }

      break; 

    case ITEMQUERY:
    case ITEMNQUERY:
    case ITEMQUERYMAP:
    case ITEMNQUERYMAP:
      
      if(!Item || !Value) {
	msSetError(MS_WEBERR, "Missing \"item\"\\\"value\" pairing.", "mapserv()");
	writeError();
      }

      if((i = msGetLayerIndex(Map, QueryLayer)) != -1) /* force the query layer on */
	Map->layers[i].status = MS_ON;

      if(QueryCoordSource != NONE && !UseShapes)
	setExtent(); /* set user area of interest */

      if(Mode == ITEMQUERY) {
	if((QueryResults = msQueryUsingItem(Map, QueryLayer, MS_SINGLE, Item, Value)) == NULL) writeError();
      } else {
	if((QueryResults = msQueryUsingItem(Map, QueryLayer, MS_MULTIPLE, Item, Value)) == NULL) writeError();
      }

      if(UseShapes)
	setExtentFromShapes();

      if(Map->querymap.status) {
	
	img = msDrawQueryMap(Map, QueryResults);
	if(!img) writeError();

	if((Mode == ITEMQUERYMAP) || (Mode == ITEMNQUERYMAP)) { /* just return the image */
	  printf("Content-type: image/%s%c%c", outputImageType[Map->imagetype], 10,10);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, NULL, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, NULL, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	  break;
	} else {
	  sprintf(buffer, "%s%s%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}

	if(Map->legend.status == MS_ON) {
	  img = msDrawLegend(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%sleg%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->legend.transparent, Map->legend.interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->legend.transparent, Map->legend.interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->scalebar.status == MS_ON) {
	  img = msDrawScalebar(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%ssb%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->scalebar.transparent, Map->scalebar.interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->scalebar.transparent, Map->scalebar.interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->reference.status == MS_ON) {
	  img = msDrawReferenceMap(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%sref%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
      }

      returnQuery();

      if(SaveQuery) {
	sprintf(buffer, "%s%s%s%s", Map->web.imagepath, Map->name, Id, MS_QUERY_EXTENSION);
	if(msSaveQuery(QueryResults, buffer) == -1) 
	  writeError();
      }

      break;
      
    case QUERY:
    case QUERYMAP:

      if((i = msGetLayerIndex(Map, QueryLayer)) != -1) /* force the query layer on */
	Map->layers[i].status = MS_ON;      

      switch(QueryCoordSource) {
      case FROMIMGPNT:	
	setCoordinate();
	Map->extent = ImgExt; // use the existing image parameters
	Map->width = ImgCols;
	Map->height = ImgRows;
	Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);	 

	if((QueryResults = msQueryUsingPoint(Map, QueryLayer, MS_SINGLE, MapPnt, 0)) == NULL) writeError();
	break;

      case FROMUSERPNT: /* only a buffer makes sense, DOES IT? */	
	setExtent();	
	if((QueryResults = msQueryUsingPoint(Map, QueryLayer, MS_SINGLE, MapPnt, Buffer)) == NULL) writeError();
	break;

      default:
	msSetError(MS_WEBERR, "Query mode needs a point, imgxy and mapxy are not set.", "mapserv()");
	writeError();
	break;
      }

      if(UseShapes)
	setExtentFromShapes();      
      
      if(Map->querymap.status) {
	img = msDrawQueryMap(Map, QueryResults);
	if(!img) writeError();
	
	if(Mode == QUERYMAP) { /* just return the image */
	  printf("Content-type: image/%s%c%c", outputImageType[Map->imagetype], 10,10);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, NULL, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	  break;
	} else {
	  sprintf(buffer, "%s%s%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->legend.status == MS_ON) {
	  img = msDrawLegend(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%sleg%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->legend.transparent, Map->legend.interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->legend.transparent, Map->legend.interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->scalebar.status == MS_ON) {
	  img = msDrawScalebar(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%ssb%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->scalebar.transparent, Map->scalebar.interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->scalebar.transparent, Map->scalebar.interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->reference.status == MS_ON) {
	  img = msDrawReferenceMap(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%sref%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif	  
	  gdImageDestroy(img);
	}
      }      
       
      returnQuery();

      if(SaveQuery) {
	sprintf(buffer, "%s%s%s%s", Map->web.imagepath, Map->name, Id, MS_QUERY_EXTENSION);
	if(msSaveQuery(QueryResults, buffer) == -1) 
	  writeError();
      }

      break; /* end switch */

    case NQUERY:
    case NQUERYMAP:

      if((i = msGetLayerIndex(Map, QueryLayer)) != -1) /* force the query layer on */
	Map->layers[i].status = MS_ON;

      switch(QueryCoordSource) {
      case FROMIMGPNT:
	
	setCoordinate();

	if(SearchMap) { // compute new extent, pan etc then search that extent
	  setExtent();
	  Map->cellsize = msAdjustExtent(&(Map->extent), Map->width, Map->height);
	  Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);
	  if((QueryResults = msQueryUsingRect(Map, QueryLayer, &Map->extent)) == NULL) writeError();
	} else {
	  Map->extent = ImgExt; // use the existing image parameters
	  Map->width = ImgCols;
	  Map->height = ImgRows;
	  Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);	 
	  if((QueryResults = msQueryUsingPoint(Map, QueryLayer, MS_MULTIPLE, MapPnt, 0)) == NULL) writeError();
	}
	break;

      case FROMIMGBOX:	

	if(SearchMap) { // compute new extent, pan etc then search that extent
	  setExtent();
	  Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);
	  Map->cellsize = msAdjustExtent(&(Map->extent), Map->width, Map->height);
	  if((QueryResults = msQueryUsingRect(Map, QueryLayer, &Map->extent)) == NULL) writeError();
	} else {
	  double cellx, celly;

	  Map->extent = ImgExt; // use the existing image parameters
	  Map->width = ImgCols;
	  Map->height = ImgRows;
	  Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);	  

	  cellx = (ImgExt.maxx-ImgExt.minx)/(ImgCols-1); // calculate the new search extent
	  celly = (ImgExt.maxy-ImgExt.miny)/(ImgRows-1);
	  RawExt.minx = ImgExt.minx + cellx*ImgBox.minx;
	  RawExt.maxx = ImgExt.minx + cellx*ImgBox.maxx;
	  RawExt.miny = ImgExt.maxy - celly*ImgBox.maxy;
	  RawExt.maxy = ImgExt.maxy - celly*ImgBox.miny;

	  if((QueryResults = msQueryUsingRect(Map, QueryLayer, &RawExt)) == NULL) writeError();
	}
	break;

      case FROMIMGSHAPE:
	Map->extent = ImgExt; // use the existing image parameters
        Map->width = ImgCols;
	Map->height = ImgRows;
	Map->cellsize = msAdjustExtent(&(Map->extent), Map->width, Map->height);
	Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);

	// convert from image to map coordinates here (see setCoordinate)
	for(i=0; i<SelectShape.numlines; i++) {
	  for(j=0; j<SelectShape.line[i].numpoints; j++) {
	    SelectShape.line[i].point[j].x = Map->extent.minx + Map->cellsize*SelectShape.line[i].point[j].x;
	    SelectShape.line[i].point[j].y = Map->extent.maxy - Map->cellsize*SelectShape.line[i].point[j].y;
	  }
	}

	if((QueryResults = msQueryUsingShape(Map, QueryLayer, &SelectShape)) == NULL) writeError();
	break;

      case FROMUSERPNT:
	if(Buffer == 0) {
	  if((QueryResults = msQueryUsingPoint(Map, QueryLayer, MS_MULTIPLE, MapPnt, Buffer)) == NULL) writeError();
	  setExtent();
	} else {
	  setExtent();
	  if((QueryResults = msQueryUsingRect(Map, QueryLayer, &Map->extent)) == NULL) writeError();
	}
	break;

      case FROMUSERSHAPE:
	setExtent();
	if((QueryResults = msQueryUsingShape(Map, QueryLayer, &SelectShape)) == NULL) writeError();
	break;

      default: /* from an extent sort */	
	setExtent();
	if((QueryResults = msQueryUsingRect(Map, QueryLayer, &Map->extent)) == NULL) writeError();
	break;
      }      

      if(UseShapes)
	setExtentFromShapes();      

      if(Map->querymap.status) {
	img = msDrawQueryMap(Map, QueryResults);
	if(!img) writeError();
	
	if(Mode == QUERYMAP) { /* just return the image */
	  printf("Content-type: image/%s%c%c", outputImageType[Map->imagetype], 10,10);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, NULL, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, NULL, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif	  
	  gdImageDestroy(img);
	  break;
	} else {
	  sprintf(buffer, "%s%s%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->legend.status == MS_ON || UseShapes) {
	  img = msDrawLegend(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%sleg%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->legend.transparent, Map->legend.interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->legend.transparent, Map->legend.interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->scalebar.status == MS_ON) {
	  img = msDrawScalebar(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%ssb%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->scalebar.transparent, Map->scalebar.interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->scalebar.transparent, Map->scalebar.interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
	
	if(Map->reference.status == MS_ON) {
	  img = msDrawReferenceMap(Map);
	  if(!img) writeError();
	  sprintf(buffer, "%s%sref%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	  if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	  if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	  gdImageDestroy(img);
	}
      }
      
      returnQuery();

      if(SaveQuery) {
	sprintf(buffer, "%s%s%s%s", Map->web.imagepath, Map->name, Id, MS_QUERY_EXTENSION);
	if(msSaveQuery(QueryResults, buffer) == -1) 
	  writeError();
      }

      break; /* end switch */
      
    case REFERENCE:
      
      setExtent();

      Map->cellsize = msAdjustExtent(&(Map->extent), Map->width, Map->height);

      img = msDrawReferenceMap(Map);
      if(!img) writeError();
      
      printf("Content-type: image/%s%c%c", outputImageType[Map->imagetype], 10,10);
#ifndef USE_GD_1_8
      if(msSaveImage(img, NULL, Map->transparent, Map->interlace) == -1) writeError();
#else
      if(msSaveImage(img, NULL, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
      gdImageDestroy(img);
      
      break;
      
    case SCALEBAR: // can't embed

      setExtent();
      
      img = msDrawScalebar(Map);
      if(!img) writeError();
      
      printf("Content-type: image/%s%c%c", outputImageType[Map->imagetype], 10,10);
#ifndef USE_GD_1_8
      if(msSaveImage(img, NULL, Map->scalebar.transparent, Map->scalebar.interlace) == -1) writeError();
#else
      if(msSaveImage(img, NULL, Map->imagetype, Map->scalebar.transparent, Map->scalebar.interlace, Map->imagequality) == -1) writeError();
#endif
      gdImageDestroy(img);
      
      break;

    case LEGEND:

      setExtent();

      img = msDrawLegend(Map);
      if(!img) writeError();
      
      printf("Content-type: image/%s%c%c", outputImageType[Map->imagetype], 10,10);
#ifndef USE_GD_1_8
      if(msSaveImage(img, NULL, Map->legend.transparent, Map->legend.interlace) == -1) writeError();
#else
      if(msSaveImage(img, NULL, Map->imagetype, Map->legend.transparent, Map->legend.interlace, Map->imagequality) == -1) writeError();
#endif
      gdImageDestroy(img);
      
      break;
      
    case MAP:

      if(QueryFile) {
	QueryResults = msLoadQuery(QueryFile);
	if(!QueryResults)
	  writeError();
      }

      setExtent();
      
      if(QueryResults)
	img = msDrawQueryMap(Map, QueryResults);
      else
	img = msDrawMap(Map);
      if(!img) writeError();
      
      printf("Content-type: image/%s%c%c", outputImageType[Map->imagetype], 10,10);
#ifndef USE_GD_1_8
      if(msSaveImage(img, NULL, Map->transparent, Map->interlace) == -1) writeError();
#else
      if(msSaveImage(img, NULL, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
      gdImageDestroy(img);
      
      break;
      
    case BROWSE:

      if(!Map->web.template) {
	msSetError(MS_WEBERR, "No template provided.", "mapserv()");
	writeError();
      }

      if(QueryFile) {
	QueryResults = msLoadQuery(QueryFile);
	if(!QueryResults)
	  writeError();
      }
 
      setExtent();
      
      Map->cellsize = msAdjustExtent(&(Map->extent), Map->width, Map->height);      
      Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);

      if((Map->scale < Map->web.minscale) && (Map->web.minscale > 0)) {
	if(Map->web.mintemplate != NULL) { /* use the template provided */
	  if(TEMPLATE_TYPE(Map->web.mintemplate) == MS_FILE)
	    returnHTML(Map->web.mintemplate);
	  else
	    returnURL(Map->web.mintemplate);
	  break;
	} else { /* force zoom = 1 (i.e. pan) */
	  fZoom = Zoom = 1;
	  ZoomDirection = 0;
	  CoordSource = FROMSCALE;
	  Map->scale = Map->web.minscale;
	  MapPnt.x = (Map->extent.maxx + Map->extent.minx)/2; // use center of bad extent
	  MapPnt.y = (Map->extent.maxy + Map->extent.miny)/2;
	  setExtent();
	  Map->cellsize = msAdjustExtent(&(Map->extent), Map->width, Map->height);      
	  Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);
	}
      }
      if((Map->scale > Map->web.maxscale) && (Map->web.maxscale > 0)) {
	if(Map->web.maxtemplate != NULL) { /* use the template provided */
	  if(TEMPLATE_TYPE(Map->web.maxtemplate) == MS_FILE)
	    returnHTML(Map->web.maxtemplate);
	  else
	    returnURL(Map->web.maxtemplate);
	} else { /* force zoom = 1 (i.e. pan) */
	  fZoom = Zoom = 1;
	  ZoomDirection = 0;
	  CoordSource = FROMSCALE;
	  Map->scale = Map->web.maxscale;
	  MapPnt.x = (Map->extent.maxx + Map->extent.minx)/2; // use center of bad extent
	  MapPnt.y = (Map->extent.maxy + Map->extent.miny)/2;
	  setExtent();
	  Map->cellsize = msAdjustExtent(&(Map->extent), Map->width, Map->height);
	  Map->scale = msCalculateScale(Map->extent, Map->units, Map->width, Map->height);
	}
      }
   
      if(Map->status == MS_ON) {
	if(QueryResults)
	  img = msDrawQueryMap(Map, QueryResults);
	else
	  img = msDrawMap(Map);
	if(!img) writeError();
	sprintf(buffer, "%s%s%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);	
#ifndef USE_GD_1_8
	if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	gdImageDestroy(img);
      }
      
      if(Map->legend.status == MS_ON) {
	img = msDrawLegend(Map);
	if(!img) writeError();
	sprintf(buffer, "%s%sleg%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	if(msSaveImage(img, buffer, Map->legend.transparent, Map->legend.interlace) == -1) writeError();
#else
	if(msSaveImage(img, buffer, Map->imagetype, Map->legend.transparent, Map->legend.interlace, Map->imagequality) == -1) writeError();
#endif
	gdImageDestroy(img);
      }
      
      if(Map->scalebar.status == MS_ON) {
	img = msDrawScalebar(Map);
	if(!img) writeError();
	sprintf(buffer, "%s%ssb%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	if(msSaveImage(img, buffer, Map->scalebar.transparent, Map->scalebar.interlace) == -1) writeError();
#else
	if(msSaveImage(img, buffer, Map->imagetype, Map->scalebar.transparent, Map->scalebar.interlace, Map->imagequality) == -1) writeError();
#endif
	gdImageDestroy(img);
      }

      if(Map->reference.status == MS_ON) {
	img = msDrawReferenceMap(Map);
	if(!img) writeError();
	sprintf(buffer, "%s%sref%s.%s", Map->web.imagepath, Map->name, Id, outputImageType[Map->imagetype]);
#ifndef USE_GD_1_8
	if(msSaveImage(img, buffer, Map->transparent, Map->interlace) == -1) writeError();
#else
	if(msSaveImage(img, buffer, Map->imagetype, Map->transparent, Map->interlace, Map->imagequality) == -1) writeError();
#endif
	gdImageDestroy(img);
      }

      if(QueryFile) {
	returnQuery();
      } else {
	if(TEMPLATE_TYPE(Map->web.template) == MS_FILE) { /* if thers's an html template, then use it */
	  printf("Content-type: text/html%c%c", 10, 10); /* write MIME header */
	  writeVersion();
	  fflush(stdout);
	  returnHTML(Map->web.template);
	} else {	
	  returnURL(Map->web.template);
	} 
      }
     
      break;
      
    default:
      break;
    } /* end switch mode */

    writeLog(MS_FALSE);
   
    /*
    ** Clean-up
    */
    msFreeQueryResults(QueryResults);
    msFreeMap(Map);

    for(i=0;i<NumEntries;i++) {
      free(Entries[i].val);
      free(Entries[i].name);
    }

    free(Item);
    free(Value);      
    free(QueryLayer);
    free(QueryFile);

    for(i=0;i<NumLayers;i++)
      free(Layers[i]);

    exit(0);
} /* End main routine */
