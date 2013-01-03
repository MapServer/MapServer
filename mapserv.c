/******************************************************************************
 * $id: mapserv.c 9470 2009-10-16 16:09:31Z sdlime $
 *
 * Project:  MapServer
 * Purpose:  MapServer CGI mainline.
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

#ifdef USE_FASTCGI
#define NO_FCGI_DEFINES
#include "fcgi_stdio.h"
#endif

#include "mapserv.h"
#include "maptime.h"

#ifndef WIN32
#include <signal.h>
#endif

MS_CVSID("$Id$")

mapservObj* mapserv;

int writeLog(int show_error)
{
  FILE *stream;
  int i;
  time_t t;
  char szPath[MS_MAXPATHLEN];

  if(!mapserv) return(0);
  if(!mapserv->map) return(0);
  if(!mapserv->map->web.log) return(0);
  
  if((stream = fopen(msBuildPath(szPath, mapserv->map->mappath, 
                                   mapserv->map->web.log),"a")) == NULL) {
    msSetError(MS_IOERR, mapserv->map->web.log, "writeLog()");
    return(-1);
  }

  t = time(NULL);
  fprintf(stream,"%s,",msStringChop(ctime(&t)));
  fprintf(stream,"%d,",(int)getpid());
  
  if(getenv("REMOTE_ADDR") != NULL)
    fprintf(stream,"%s,",getenv("REMOTE_ADDR"));
  else
    fprintf(stream,"NULL,");
 
  fprintf(stream,"%s,",mapserv->map->name);
  fprintf(stream,"%d,",mapserv->Mode);

  fprintf(stream,"%f %f %f %f,", mapserv->map->extent.minx, mapserv->map->extent.miny, mapserv->map->extent.maxx, mapserv->map->extent.maxy);

  fprintf(stream,"%f %f,", mapserv->mappnt.x, mapserv->mappnt.y);

  for(i=0;i<mapserv->NumLayers;i++)
    fprintf(stream, "%s ", mapserv->Layers[i]);
  fprintf(stream,",");

  if(show_error == MS_TRUE)
    msWriteError(stream);
  else
    fprintf(stream, "normal execution");

  fprintf(stream,"\n");

  fclose(stream);
  return(0);
}

void writeError(void)
{
  errorObj *ms_error = msGetErrorObj();

  writeLog(MS_TRUE);

  if(!mapserv || !mapserv->map) {
    msIO_printf("Content-type: text/html%c%c",10,10);
    msIO_printf("<HTML>\n");
    msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
    msIO_printf("<!-- %s -->\n", msGetVersion());
    msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
    msWriteErrorXML(stdout);
    msIO_printf("</BODY></HTML>");
    if(mapserv) 
      msFreeMapServObj(mapserv);
    msCleanup();
    exit(0);
  }

  if((ms_error->code == MS_NOTFOUND) && (mapserv->map->web.empty)) {
    /* msRedirect(mapserv->map->web.empty); */
    if(msReturnURL(mapserv, mapserv->map->web.empty, BROWSE) != MS_SUCCESS) {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("<HTML>\n");
      msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
      msIO_printf("<!-- %s -->\n", msGetVersion());
      msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
      msWriteErrorXML(stdout);
      msIO_printf("</BODY></HTML>");
    }
  } else {
    if(mapserv->map->web.error) {      
      /* msRedirect(mapserv->map->web.error); */
      if(msReturnURL(mapserv, mapserv->map->web.error, BROWSE) != MS_SUCCESS) {
        msIO_printf("Content-type: text/html%c%c",10,10);
        msIO_printf("<HTML>\n");
        msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
        msIO_printf("<!-- %s -->\n", msGetVersion());
        msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
        msWriteErrorXML(stdout);
        msIO_printf("</BODY></HTML>");
      }
    } else {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("<HTML>\n");
      msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
      msIO_printf("<!-- %s -->\n", msGetVersion());
      msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
      msWriteErrorXML(stdout);
      msIO_printf("</BODY></HTML>");
    }
  }

  /* Clean-up (the following are not stored as part of the mapserv) */
  if(QueryItem) free(QueryItem);
  if(QueryString) free(QueryString);
  if(QueryLayer) free(QueryLayer);
  if(SelectLayer) free(SelectLayer);
  if(QueryFile) free(QueryFile);

  msFreeMapServObj(mapserv);
  msCleanup();

  exit(0); /* bail */
}

/*
** Converts a string (e.g. form parameter) to a double, first checking the format against
** a regular expression. Dumps an error immediately if the format test fails.
*/
static double getNumeric(char *s)
{
  char *err;
  double rv; 

  rv = strtod(s, &err);
  if (*err) {
    msSetError(MS_TYPEERR, NULL, "getNumeric()");
    writeError();
  }
  return rv;
}



/*
** Extract Map File name from params and load it.  
** Returns map object or NULL on error.
*/
mapObj *loadMap(void)
{
  int i;
  mapObj *map = NULL;

  for(i=0;i<mapserv->request->NumParams;i++) /* find the mapfile parameter first */
    if(strcasecmp(mapserv->request->ParamNames[i], "map") == 0) break;
  
  if(i == mapserv->request->NumParams) {
    if(getenv("MS_MAPFILE")) /* has a default file has not been set */
      map = msLoadMap(getenv("MS_MAPFILE"), NULL);
    else {
      msSetError(MS_WEBERR, "CGI variable \"map\" is not set.", "loadMap()"); /* no default, outta here */
      writeError();
    }
  } else {
    if(getenv(mapserv->request->ParamValues[i])) /* an environment variable references the actual file to use */
      map = msLoadMap(getenv(mapserv->request->ParamValues[i]), NULL);
    else {
      /* by here we know the request isn't for something in an environment variable */
      if(getenv("MS_MAP_NO_PATH")) {
        msSetError(MS_WEBERR, "Mapfile not found in environment variables and this server is not configured for full paths.", "loadMap()");
	writeError();
      }

      if(getenv("MS_MAP_PATTERN") && msEvalRegex(getenv("MS_MAP_PATTERN"), mapserv->request->ParamValues[i]) != MS_TRUE) {
        msSetError(MS_WEBERR, "Parameter 'map' value fails to validate.", "loadMap()");
        writeError();
      }

      /* ok to try to load now */
      map = msLoadMap(mapserv->request->ParamValues[i], NULL);
    }
  }

  if(!map) writeError();

  /* check for any %variable% substitutions here, also do any map_ changes, we do this here so WMS/WFS  */
  /* services can take advantage of these "vendor specific" extensions */
  for(i=0;i<mapserv->request->NumParams;i++) {
    /*
    ** a few CGI variables should be skipped altogether
    **
    ** qstring: there is separate per layer validation for attribute queries and the substitution checks
    **          below conflict with that so we avoid it here
    */
    if(strncasecmp(mapserv->request->ParamNames[i],"qstring",7) == 0) continue;

    if(strncasecmp(mapserv->request->ParamNames[i],"map_",4) == 0 || strncasecmp(mapserv->request->ParamNames[i],"map.",4) == 0) { /* check to see if there are any additions to the mapfile */
      if(msUpdateMapFromURL(map, mapserv->request->ParamNames[i], mapserv->request->ParamValues[i]) != MS_SUCCESS) writeError();
      continue;
    }
  }

  msApplySubstitutions(map, mapserv->request->ParamNames, mapserv->request->ParamValues, mapserv->request->NumParams);
  msApplyDefaultSubstitutions(map);

  /* check to see if a ogc map context is passed as argument. if there */
  /* is one load it */

  for(i=0;i<mapserv->request->NumParams;i++) {
    if(strcasecmp(mapserv->request->ParamNames[i],"context") == 0) {
      if(mapserv->request->ParamValues[i] && strlen(mapserv->request->ParamValues[i]) > 0) {
        if(strncasecmp(mapserv->request->ParamValues[i],"http",4) == 0) {
          if(msGetConfigOption(map, "CGI_CONTEXT_URL"))
            msLoadMapContextURL(map, mapserv->request->ParamValues[i], MS_FALSE);
        } else
            msLoadMapContext(map, mapserv->request->ParamValues[i], MS_FALSE); 
      }
    }
  } 

  return map;
}


/*
** Set operation mode. First look in MS_MODE env. var. as a
** default value that can be overridden by the mode=... CGI param.
** Returns silently, leaving mapserv->Mode unchanged if mode param not set.
*/
static int setMode(void)
{
    const char *mode = NULL;
    int i, j;


    mode = getenv("MS_MODE");
    for( i=0; i<mapserv->request->NumParams; i++ ) 
    {
        if(strcasecmp(mapserv->request->ParamNames[i], "mode") == 0)
        {
            mode = mapserv->request->ParamValues[i];
            break;
        }
    }

    if (mode) {
      for(j=0; j<numModes; j++) {
        if(strcasecmp(mode, modeStrings[j]) == 0) {
          mapserv->Mode = j;
          break;
        }
      }

      if(j == numModes) {
        msSetError(MS_WEBERR, "Invalid mode.", "setMode()");
        return MS_FAILURE;
      }
    }

    return MS_SUCCESS;
}

/*
** Process CGI parameters.
*/
void loadForm(void)
{
  int i,n;
  char **tokens=NULL;
  int rosa_type=0;

  for(i=0;i<mapserv->request->NumParams;i++) { /* now process the rest of the form variables */
    if(strlen(mapserv->request->ParamValues[i]) == 0)
      continue;
    
    
    if(strcasecmp(mapserv->request->ParamNames[i],"icon") == 0) {      
      mapserv->icon = msStrdup(mapserv->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"queryfile") == 0) {      
      QueryFile = msStrdup(mapserv->request->ParamValues[i]);
      continue;
    }
    
    if(strcasecmp(mapserv->request->ParamNames[i],"savequery") == 0) {
      mapserv->savequery = MS_TRUE;
      continue;
    }
    
    /* Insecure as implemented, need to save someplace non accessible by everyone in the universe
        if(strcasecmp(mapserv->request->ParamNames[i],"savemap") == 0) {      
         mapserv->savemap = MS_TRUE;
         continue;
        }
    */

    if(strcasecmp(mapserv->request->ParamNames[i],"zoom") == 0) {
      mapserv->Zoom = getNumeric(mapserv->request->ParamValues[i]);      
      if((mapserv->Zoom > MAXZOOM) || (mapserv->Zoom < MINZOOM)) {
        msSetError(MS_WEBERR, "Zoom value out of range.", "loadForm()");
        writeError();
      }
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"zoomdir") == 0) {
      mapserv->ZoomDirection = (int)getNumeric(mapserv->request->ParamValues[i]);
      if((mapserv->ZoomDirection != -1) && (mapserv->ZoomDirection != 1) && (mapserv->ZoomDirection != 0)) {
        msSetError(MS_WEBERR, "Zoom direction must be 1, 0 or -1.", "loadForm()");
        writeError();
      }
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"zoomsize") == 0) { /* absolute zoom magnitude */
      ZoomSize = (int) getNumeric(mapserv->request->ParamValues[i]);      
      if((ZoomSize > MAXZOOM) || (ZoomSize < 1)) {
        msSetError(MS_WEBERR, "Invalid zoom size.", "loadForm()");
        writeError();
      }    
      continue;
    }
    
    if(strcasecmp(mapserv->request->ParamNames[i],"imgext") == 0) { /* extent of an existing image in a web application */
      tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);

      if(!tokens) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
        writeError();
      }

      if(n != 4) {
        msSetError(MS_WEBERR, "Not enough arguments for imgext.", "loadForm()");
        writeError();
      }

      mapserv->ImgExt.minx = getNumeric(tokens[0]);
      mapserv->ImgExt.miny = getNumeric(tokens[1]);
      mapserv->ImgExt.maxx = getNumeric(tokens[2]);
      mapserv->ImgExt.maxy = getNumeric(tokens[3]);

      msFreeCharArray(tokens, 4);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"searchmap") == 0) {      
      SearchMap = MS_TRUE;
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"id") == 0) {
      if(msEvalRegex(IDPATTERN, mapserv->request->ParamValues[i]) == MS_FALSE) { 
	msSetError(MS_WEBERR, "Parameter 'id' value fails to validate.", "loadForm()"); 
	writeError(); 
      }
      strlcpy(mapserv->Id, mapserv->request->ParamValues[i], IDSIZE);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"mapext") == 0) { /* extent of the new map or query */

      if(strncasecmp(mapserv->request->ParamValues[i],"shape",5) == 0)
        mapserv->UseShapes = MS_TRUE;
      else {
        tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);
    
        if(!tokens) {
          msSetError(MS_MEMERR, NULL, "loadForm()");
          writeError();
        }
    
        if(n != 4) {
          msSetError(MS_WEBERR, "Not enough arguments for mapext.", "loadForm()");
          writeError();
        }
    
        mapserv->map->extent.minx = getNumeric(tokens[0]);
        mapserv->map->extent.miny = getNumeric(tokens[1]);
        mapserv->map->extent.maxx = getNumeric(tokens[2]);
        mapserv->map->extent.maxy = getNumeric(tokens[3]);    
    
        msFreeCharArray(tokens, 4);
    
#ifdef USE_PROJ
        /* 
         * If there is a projection in the map file, and it is not lon/lat, and the 
         * extents "look like" they *are* lon/lat, based on their size,
         * then convert the extents to the map file projection.
         *
         * DANGER: If the extents are legitimately in the mapfile projection
         *         and coincidentally fall in the lon/lat range, bad things
         *         will ensue.
         */
        if(mapserv->map->projection.proj && !pj_is_latlong(mapserv->map->projection.proj)
           && (mapserv->map->extent.minx >= -180.0 && mapserv->map->extent.minx <= 180.0) 
           && (mapserv->map->extent.miny >= -90.0 && mapserv->map->extent.miny <= 90.0)
           && (mapserv->map->extent.maxx >= -180.0 && mapserv->map->extent.maxx <= 180.0) 
           && (mapserv->map->extent.maxy >= -90.0 && mapserv->map->extent.maxy <= 90.0)) {
          msProjectRect(&(mapserv->map->latlon), &(mapserv->map->projection), &(mapserv->map->extent)); /* extent is a in lat/lon */
        }
#endif

        if((mapserv->map->extent.minx != mapserv->map->extent.maxx) && (mapserv->map->extent.miny != mapserv->map->extent.maxy)) { /* extent seems ok */
          mapserv->CoordSource = FROMUSERBOX;
          QueryCoordSource = FROMUSERBOX;
        }
      }

      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"minx") == 0) { /* extent of the new map, in pieces */
      mapserv->map->extent.minx = getNumeric(mapserv->request->ParamValues[i]);      
      continue;
    }
    if(strcasecmp(mapserv->request->ParamNames[i],"maxx") == 0) {      
      mapserv->map->extent.maxx = getNumeric(mapserv->request->ParamValues[i]);
      continue;
    }
    if(strcasecmp(mapserv->request->ParamNames[i],"miny") == 0) {
      mapserv->map->extent.miny = getNumeric(mapserv->request->ParamValues[i]);
      continue;
    }
    if(strcasecmp(mapserv->request->ParamNames[i],"maxy") == 0) {
      mapserv->map->extent.maxy = getNumeric(mapserv->request->ParamValues[i]);
      mapserv->CoordSource = FROMUSERBOX;
      QueryCoordSource = FROMUSERBOX;
      continue;
    } 

    if(strcasecmp(mapserv->request->ParamNames[i],"mapxy") == 0) { /* user map coordinate */
      
      if(strncasecmp(mapserv->request->ParamValues[i],"shape",5) == 0) {
        mapserv->UseShapes = MS_TRUE;    
      } else {
        tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);

        if(!tokens) {
          msSetError(MS_MEMERR, NULL, "loadForm()");
          writeError();
        }
    
        if(n != 2) {
          msSetError(MS_WEBERR, "Not enough arguments for mapxy.", "loadForm()");
          writeError();
        }
    
        mapserv->mappnt.x = getNumeric(tokens[0]);
        mapserv->mappnt.y = getNumeric(tokens[1]);
    
        msFreeCharArray(tokens, 2);

#ifdef USE_PROJ
        if(mapserv->map->projection.proj && !pj_is_latlong(mapserv->map->projection.proj)
           && (mapserv->mappnt.x >= -180.0 && mapserv->mappnt.x <= 180.0) 
           && (mapserv->mappnt.y >= -90.0 && mapserv->mappnt.y <= 90.0)) {
          msProjectPoint(&(mapserv->map->latlon), &(mapserv->map->projection), &mapserv->mappnt); /* point is a in lat/lon */
        }
#endif

        if(mapserv->CoordSource == NONE) { /* don't override previous settings (i.e. buffer or scale ) */
          mapserv->CoordSource = FROMUSERPNT;
          QueryCoordSource = FROMUSERPNT;
        }
      }
      continue;
    }

    /*
    ** Query shape consisting of map or image coordinates. It's almost identical processing so we'll do either in this block...
    */
    if(strcasecmp(mapserv->request->ParamNames[i], "mapshape") == 0 || strcasecmp(mapserv->request->ParamNames[i], "imgshape") == 0) {
      if(strcasecmp(mapserv->request->ParamNames[i],"mapshape") == 0)
        QueryCoordSource = FROMUSERSHAPE;
      else
        QueryCoordSource = FROMIMGSHAPE;

      if(strchr(mapserv->request->ParamValues[i], '(') != NULL) { /* try WKT */
        if((mapserv->map->query.shape = msShapeFromWKT(mapserv->request->ParamValues[i])) == NULL) {
          msSetError(MS_WEBERR, "WKT parse failed for mapshape/imgshape.", "loadForm()");
          writeError();
        }
      } else {
        lineObj line={0,NULL};
        char **tmp=NULL;
        int n, j;
      
        tmp = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);

        if(n%2 != 0 || n<8) { /* n must be even and be at least 8 */
          msSetError(MS_WEBERR, "Malformed polygon geometry for mapshape/imgshape.", "loadForm()");
          writeError();
        }

        line.numpoints = n/2;
        if((line.point = (pointObj *)malloc(sizeof(pointObj)*line.numpoints)) == NULL) {
          msSetError(MS_MEMERR, NULL, "loadForm()");
          writeError();
        }

        if((mapserv->map->query.shape = (shapeObj *) malloc(sizeof(shapeObj))) == NULL) {
          msSetError(MS_MEMERR, NULL, "loadForm()");
          writeError();
	}
        msInitShape(mapserv->map->query.shape);
        mapserv->map->query.shape->type = MS_SHAPE_POLYGON;

        for(j=0; j<line.numpoints; j++) {
          line.point[j].x = atof(tmp[2*j]);
          line.point[j].y = atof(tmp[2*j+1]);

#ifdef USE_PROJ
          if(QueryCoordSource == FROMUSERSHAPE && mapserv->map->projection.proj && !pj_is_latlong(mapserv->map->projection.proj)
             && (line.point[j].x >= -180.0 && line.point[j].x <= 180.0) 
             && (line.point[j].y >= -90.0 && line.point[j].y <= 90.0)) {
            msProjectPoint(&(mapserv->map->latlon), &(mapserv->map->projection), &line.point[j]); /* point is a in lat/lon */
          }
#endif
        }

        if(msAddLine(mapserv->map->query.shape, &line) == -1) writeError();

        msFree(line.point);
        msFreeCharArray(tmp, n);
      }

      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"img.x") == 0) { /* mouse click, in pieces */
      mapserv->ImgPnt.x = getNumeric(mapserv->request->ParamValues[i]);
      if((mapserv->ImgPnt.x > (2*mapserv->map->maxsize)) || (mapserv->ImgPnt.x < (-2*mapserv->map->maxsize))) {
        msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
        writeError();
      }
      mapserv->CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }
    if(strcasecmp(mapserv->request->ParamNames[i],"img.y") == 0) {
      mapserv->ImgPnt.y = getNumeric(mapserv->request->ParamValues[i]);      
      if((mapserv->ImgPnt.y > (2*mapserv->map->maxsize)) || (mapserv->ImgPnt.y < (-2*mapserv->map->maxsize))) {
        msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
        writeError();
      }
      mapserv->CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"imgxy") == 0) { /* mouse click, single variable */
      if(mapserv->CoordSource == FROMIMGPNT)
        continue;

      tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);

      if(!tokens) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
        writeError();
      }

      if(n != 2) {
        msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
        writeError();
      }

      mapserv->ImgPnt.x = getNumeric(tokens[0]);
      mapserv->ImgPnt.y = getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);

      if((mapserv->ImgPnt.x > (2*mapserv->map->maxsize)) || (mapserv->ImgPnt.x < (-2*mapserv->map->maxsize)) || (mapserv->ImgPnt.y > (2*mapserv->map->maxsize)) || (mapserv->ImgPnt.y < (-2*mapserv->map->maxsize))) {
        msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
        writeError();
      }

      if(mapserv->CoordSource == NONE) { /* override nothing since this parameter is usually used to hold a default value */
        mapserv->CoordSource = FROMIMGPNT;
        QueryCoordSource = FROMIMGPNT;
      }
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"imgbox") == 0) { /* selection box (eg. mouse drag) */
      tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);
      
      if(!tokens) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
        writeError();
      }
      
      if(n != 4) {
        msSetError(MS_WEBERR, "Not enough arguments for imgbox.", "loadForm()");
        writeError();
      }
      
      mapserv->ImgBox.minx = getNumeric(tokens[0]);
      mapserv->ImgBox.miny = getNumeric(tokens[1]);
      mapserv->ImgBox.maxx = getNumeric(tokens[2]);
      mapserv->ImgBox.maxy = getNumeric(tokens[3]);
      
      msFreeCharArray(tokens, 4);

      if((mapserv->ImgBox.minx != mapserv->ImgBox.maxx) && (mapserv->ImgBox.miny != mapserv->ImgBox.maxy)) { /* must not degenerate into a point */
        mapserv->CoordSource = FROMIMGBOX;
        QueryCoordSource = FROMIMGBOX;
      }
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"ref.x") == 0) { /* mouse click in reference image, in pieces */
      mapserv->RefPnt.x = getNumeric(mapserv->request->ParamValues[i]);      
      if((mapserv->RefPnt.x > (2*mapserv->map->maxsize)) || (mapserv->RefPnt.x < (-2*mapserv->map->maxsize))) {
        msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
        writeError();
      }
      mapserv->CoordSource = FROMREFPNT;
      continue;
    }
    if(strcasecmp(mapserv->request->ParamNames[i],"ref.y") == 0) {
      mapserv->RefPnt.y = getNumeric(mapserv->request->ParamValues[i]); 
      if((mapserv->RefPnt.y > (2*mapserv->map->maxsize)) || (mapserv->RefPnt.y < (-2*mapserv->map->maxsize))) {
        msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
        writeError();
      }
      mapserv->CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"refxy") == 0) { /* mouse click in reference image, single variable */
      tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);

      if(!tokens) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
        writeError();
      }

      if(n != 2) {
        msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
        writeError();
      }

      mapserv->RefPnt.x = getNumeric(tokens[0]);
      mapserv->RefPnt.y = getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if((mapserv->RefPnt.x > (2*mapserv->map->maxsize)) || (mapserv->RefPnt.x < (-2*mapserv->map->maxsize)) || (mapserv->RefPnt.y > (2*mapserv->map->maxsize)) || (mapserv->RefPnt.y < (-2*mapserv->map->maxsize))) {
        msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
        writeError();
      }
      
      mapserv->CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"buffer") == 0) { /* radius (map units), actually 1/2 square side */
      mapserv->Buffer = getNumeric(mapserv->request->ParamValues[i]);      
      mapserv->CoordSource = FROMBUF;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"scale") == 0 || strcasecmp(mapserv->request->ParamNames[i],"scaledenom") == 0) { /* scale for new map */
      mapserv->ScaleDenom = getNumeric(mapserv->request->ParamValues[i]);      
      if(mapserv->ScaleDenom <= 0) {
        msSetError(MS_WEBERR, "Scale out of range.", "loadForm()");
        writeError();
      }
      mapserv->CoordSource = FROMSCALE;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }
    
    if(strcasecmp(mapserv->request->ParamNames[i],"imgsize") == 0) { /* size of existing image (pixels) */
      tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);

      if(!tokens) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
        writeError();
      }

      if(n != 2) {
        msSetError(MS_WEBERR, "Not enough arguments for imgsize.", "loadForm()");
        writeError();
      }

      mapserv->ImgCols = (int)getNumeric(tokens[0]);
      mapserv->ImgRows = (int)getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(mapserv->ImgCols > mapserv->map->maxsize || mapserv->ImgRows > mapserv->map->maxsize || mapserv->ImgCols <= 0 || mapserv->ImgRows <= 0) {
        msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
        writeError();
      }
 
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"mapsize") == 0) { /* size of new map (pixels) */
      tokens = msStringSplit(mapserv->request->ParamValues[i], ' ', &n);

      if(!tokens) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
        writeError();
      }

      if(n != 2) {
        msSetError(MS_WEBERR, "Not enough arguments for mapsize.", "loadForm()");
        writeError();
      }

      mapserv->map->width = (int)getNumeric(tokens[0]);
      mapserv->map->height = (int)getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(mapserv->map->width > mapserv->map->maxsize || mapserv->map->height > mapserv->map->maxsize || mapserv->map->width <= 0 || mapserv->map->height <= 0) {
        msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
        writeError();
      }
      continue;
    }

    if(strncasecmp(mapserv->request->ParamNames[i],"layers", 6) == 0) { /* turn a set of layers, delimited by spaces, on */

      /* If layers=all then turn on all layers */
      if (strcasecmp(mapserv->request->ParamValues[i], "all") == 0 && mapserv->map != NULL) {
        int l;

        /* Reset NumLayers=0. If individual layers were already selected then free the previous values.  */
        for(l=0; l<mapserv->NumLayers; l++)
          msFree(mapserv->Layers[l]);
        mapserv->NumLayers=0;

        for(mapserv->NumLayers=0; mapserv->NumLayers < mapserv->map->numlayers; mapserv->NumLayers++) {
          if(msGrowMapservLayers(mapserv) == MS_FAILURE)
            writeError();

          if(GET_LAYER(mapserv->map, mapserv->NumLayers)->name) {
            mapserv->Layers[mapserv->NumLayers] = msStrdup(GET_LAYER(mapserv->map, mapserv->NumLayers)->name);
          } else {
            mapserv->Layers[mapserv->NumLayers] = msStrdup("");
          }
        }
      } else {
        int num_layers=0, l;
        char **layers=NULL;

        layers = msStringSplit(mapserv->request->ParamValues[i], ' ', &(num_layers));
        for(l=0; l<num_layers; l++) {
          if(msGrowMapservLayers(mapserv) == MS_FAILURE)
            writeError();
          mapserv->Layers[mapserv->NumLayers++] = msStrdup(layers[l]);
        }

        msFreeCharArray(layers, num_layers);
        num_layers = 0;
      }

      continue;
    }

    if(strncasecmp(mapserv->request->ParamNames[i],"layer", 5) == 0) { /* turn a single layer/group on */
      if(msGrowMapservLayers(mapserv) == MS_FAILURE)
        writeError();
      mapserv->Layers[mapserv->NumLayers] = msStrdup(mapserv->request->ParamValues[i]);
      mapserv->NumLayers++;
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"qlayer") == 0) { /* layer to query (i.e search) */
      QueryLayer = msStrdup(mapserv->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"qitem") == 0) { /* attribute to query on (optional) */
      QueryItem = msStrdup(mapserv->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"qstring") == 0) { /* attribute query string */
      QueryString = msStrdup(mapserv->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"qformat") == 0) { /* format to apply to query results (shortcut instead of having to use "map.web=QUERYFORMAT+foo") */
      if(mapserv->map->web.queryformat) free(mapserv->map->web.queryformat); /* avoid leak */
      mapserv->map->web.queryformat = msStrdup(mapserv->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"slayer") == 0) { /* layer to select (for feature based search) */
      SelectLayer = msStrdup(mapserv->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"shapeindex") == 0) { /* used for index queries */
      ShapeIndex = (int)getNumeric(mapserv->request->ParamValues[i]);
      continue;
    }
    if(strcasecmp(mapserv->request->ParamNames[i],"tileindex") == 0) {
      TileIndex = (int)getNumeric(mapserv->request->ParamValues[i]);
      continue;
    }

    /* -------------------------------------------------------------------- 
     *   The following code is used to support mode=tile                    
     * -------------------------------------------------------------------- */ 

    if(strcasecmp(mapserv->request->ParamNames[i], "tilemode") == 0) { 
      /* currently, only valid tilemode is "spheremerc" */
      if( strcasecmp(mapserv->request->ParamValues[i], "gmap") == 0) {
        mapserv->TileMode = TILE_GMAP;
      } else if ( strcasecmp(mapserv->request->ParamValues[i], "ve") == 0 ) {
        mapserv->TileMode = TILE_VE;
      } else {
        msSetError(MS_WEBERR, "Invalid tilemode. Use one of: gmap, ve", "loadForm()");
        writeError();
      }
      continue;
    }

    if(strcasecmp(mapserv->request->ParamNames[i],"tile") == 0) { 

      if( strlen(mapserv->request->ParamValues[i]) < 1 ) {
        msSetError(MS_WEBERR, "Empty tile parameter.", "loadForm()");
        writeError();
      }
      mapserv->CoordSource = FROMTILE;
      mapserv->TileCoords = msStrdup(mapserv->request->ParamValues[i]);
      
      continue;
    }

    /* -------------------------------------------------------------------- */
    /*      The following code is used to support the rosa applet (for      */
    /*      more information on Rosa, please consult :                      */
    /*      http://www.maptools.org/rosa/) .                                */
    /*      This code was provided by Tim.Mackey@agso.gov.au.               */
    /*                                                                      */
    /*      For Application using it can be seen at :                       */    
    /*        http://www.agso.gov.au/map/pilbara/                           */
    /*                                                                      */
    /* -------------------------------------------------------------------- */

    if(strcasecmp(mapserv->request->ParamNames[i],"INPUT_TYPE") == 0)
    { /* Rosa input type */
        if(strcasecmp(mapserv->request->ParamValues[i],"auto_rect") == 0) 
        {
            rosa_type=1; /* rectangle */
            continue;
        }
            
        if(strcasecmp(mapserv->request->ParamValues[i],"auto_point") == 0) 
        {
            rosa_type=2; /* point */
            continue;
        }
    }
    if(strcasecmp(mapserv->request->ParamNames[i],"INPUT_COORD") == 0) 
    { /* Rosa coordinates */
 
       switch(rosa_type)
       {
         case 1:
             sscanf(mapserv->request->ParamValues[i],"%lf,%lf;%lf,%lf",
                    &mapserv->ImgBox.minx,&mapserv->ImgBox.miny,&mapserv->ImgBox.maxx,
                    &mapserv->ImgBox.maxy);
             if((mapserv->ImgBox.minx != mapserv->ImgBox.maxx) && 
                (mapserv->ImgBox.miny != mapserv->ImgBox.maxy)) 
             {
                 mapserv->CoordSource = FROMIMGBOX;
                 QueryCoordSource = FROMIMGBOX;
             }
             else 
             {
                 mapserv->CoordSource = FROMIMGPNT;
                 QueryCoordSource = FROMIMGPNT;
                 mapserv->ImgPnt.x=mapserv->ImgBox.minx;
                 mapserv->ImgPnt.y=mapserv->ImgBox.miny;
       }
           break;
         case 2:
           sscanf(mapserv->request->ParamValues[i],"%lf,%lf",&mapserv->ImgPnt.x,
                   &mapserv->ImgPnt.y);
           mapserv->CoordSource = FROMIMGPNT;
           QueryCoordSource = FROMIMGPNT;
           break;
         }
       continue;
    }    
    /* -------------------------------------------------------------------- */
    /*      end of code for Rosa support.                                   */
    /* -------------------------------------------------------------------- */

  } /* next parameter */

  if(mapserv->Mode == ZOOMIN) {
    mapserv->ZoomDirection = 1;
    mapserv->Mode = BROWSE;
  }     
  if(mapserv->Mode == ZOOMOUT) {
    mapserv->ZoomDirection = -1;
    mapserv->Mode = BROWSE;
  }

  if(ZoomSize != 0) { /* use direction and magnitude to calculate zoom */
    if(mapserv->ZoomDirection == 0) {
      mapserv->fZoom = 1;
    } else {
      mapserv->fZoom = ZoomSize*mapserv->ZoomDirection;
      if(mapserv->fZoom < 0)
        mapserv->fZoom = 1.0/MS_ABS(mapserv->fZoom);
    }
  } else { /* use single value for zoom */
    if((mapserv->Zoom >= -1) && (mapserv->Zoom <= 1)) {
      mapserv->fZoom = 1; /* pan */
    } else {
      if(mapserv->Zoom < 0)
        mapserv->fZoom = 1.0/MS_ABS(mapserv->Zoom);
      else
        mapserv->fZoom = mapserv->Zoom;
    }
  }

  if(mapserv->ImgRows == -1) mapserv->ImgRows = mapserv->map->height;
  if(mapserv->ImgCols == -1) mapserv->ImgCols = mapserv->map->width;  
  if(mapserv->map->height == -1) mapserv->map->height = mapserv->ImgRows;
  if(mapserv->map->width == -1) mapserv->map->width = mapserv->ImgCols;  
}

void setExtentFromShapes(void) {
  int found=0;
  double dx, dy, cellsize;

  rectObj tmpext={-1.0,-1.0,-1.0,-1.0};
  pointObj tmppnt={-1.0,-1.0};

  found = msGetQueryResultBounds(mapserv->map, &(tmpext));

  dx = tmpext.maxx - tmpext.minx;
  dy = tmpext.maxy - tmpext.miny;
 
  tmppnt.x = (tmpext.maxx + tmpext.minx)/2;
  tmppnt.y = (tmpext.maxy + tmpext.miny)/2;
  tmpext.minx -= dx*EXTENT_PADDING/2.0;
  tmpext.maxx += dx*EXTENT_PADDING/2.0;
  tmpext.miny -= dy*EXTENT_PADDING/2.0;
  tmpext.maxy += dy*EXTENT_PADDING/2.0;

  if(mapserv->ScaleDenom != 0) { /* apply the scale around the center point (tmppnt) */
    cellsize = (mapserv->ScaleDenom/mapserv->map->resolution)/msInchesPerUnit(mapserv->map->units,0); /* user supplied a point and a scale */
    tmpext.minx = tmppnt.x - cellsize*mapserv->map->width/2.0;
    tmpext.miny = tmppnt.y - cellsize*mapserv->map->height/2.0;
    tmpext.maxx = tmppnt.x + cellsize*mapserv->map->width/2.0;
    tmpext.maxy = tmppnt.y + cellsize*mapserv->map->height/2.0;
  } else if(mapserv->Buffer != 0) { /* apply the buffer around the center point (tmppnt) */
    tmpext.minx = tmppnt.x - mapserv->Buffer;
    tmpext.miny = tmppnt.y - mapserv->Buffer;
    tmpext.maxx = tmppnt.x + mapserv->Buffer;
    tmpext.maxy = tmppnt.y + mapserv->Buffer;
  }

  /* in case we don't get  usable extent at this point (i.e. single point result) */
  if(!MS_VALID_EXTENT(tmpext)) {
    if(mapserv->map->web.minscaledenom > 0) { /* try web object minscale first */
      cellsize = (mapserv->map->web.minscaledenom/mapserv->map->resolution)/msInchesPerUnit(mapserv->map->units,0); /* user supplied a point and a scale */
      tmpext.minx = tmppnt.x - cellsize*mapserv->map->width/2.0;
      tmpext.miny = tmppnt.y - cellsize*mapserv->map->height/2.0;
      tmpext.maxx = tmppnt.x + cellsize*mapserv->map->width/2.0;
      tmpext.maxy = tmppnt.y + cellsize*mapserv->map->height/2.0;
    } else {
      msSetError(MS_WEBERR, "No way to generate a valid map extent from selected shapes.", "mapserv()");
      writeError();
    }
  }

  mapserv->mappnt = tmppnt;
  mapserv->map->extent = mapserv->RawExt = tmpext; /* save unadjusted extent */

  return;
}


/* FIX: NEED ERROR CHECKING HERE FOR IMGPNT or MAPPNT */
void setCoordinate(void)
{
  double cellx,celly;

  cellx = MS_CELLSIZE(mapserv->ImgExt.minx, mapserv->ImgExt.maxx, mapserv->ImgCols);
  celly = MS_CELLSIZE(mapserv->ImgExt.miny, mapserv->ImgExt.maxy, mapserv->ImgRows);

  mapserv->mappnt.x = MS_IMAGE2MAP_X(mapserv->ImgPnt.x, mapserv->ImgExt.minx, cellx);
  mapserv->mappnt.y = MS_IMAGE2MAP_Y(mapserv->ImgPnt.y, mapserv->ImgExt.maxy, celly);

  return;
}

void returnCoordinate(pointObj pnt)
{
  msSetError(MS_NOERR, 
             "Your \"<i>click</i>\" corresponds to (approximately): (%g, %g).",
             NULL, mapserv->mappnt.x, mapserv->mappnt.y);

#ifdef USE_PROJ
  if(mapserv->map->projection.proj != NULL && !pj_is_latlong(mapserv->map->projection.proj) ) {
    pointObj p=mapserv->mappnt;
    msProjectPoint(&(mapserv->map->projection), &(mapserv->map->latlon), &p);
    msSetError( MS_NOERR, "%s Computed lat/lon value is (%g, %g).\n", NULL, p.x, p.y);
  }
#endif

  writeError();
}


/************************************************************************/
/*                      FastCGI cleanup functions.                      */
/************************************************************************/
#ifndef WIN32
void msCleanupOnSignal( int nInData )
{
  /* For some reason, the fastcgi message code does not seem to work */
  /* from within the signal handler on Unix.  So we force output through */
  /* normal stdio functions. */
  msIO_installHandlers( NULL, NULL, NULL );
  msIO_fprintf( stderr, "In msCleanupOnSignal.\n" );
  msCleanup();
  exit( 0 );
}
#endif

#ifdef WIN32
void msCleanupOnExit( void )
{
  /* note that stderr and stdout seem to be non-functional in the */
  /* fastcgi/win32 case.  If you really want to check functioning do */
  /* some sort of hack logging like below ... otherwise just trust it! */
       
#ifdef notdef
  FILE *fp_out = fopen( "D:\\temp\\mapserv.log", "w" );
    
  fprintf( fp_out, "In msCleanupOnExit\n" );
  fclose( fp_out );
#endif    
  msCleanup();
}
#endif

/************************************************************************/
/*                                main()                                */
/************************************************************************/
int main(int argc, char *argv[]) {
  int i,j, iArg;
  char buffer[1024];
  imageObj *img=NULL;
  int status;
  int sendheaders = MS_TRUE;
  struct mstimeval execstarttime, execendtime;
  struct mstimeval requeststarttime, requestendtime;
  char *service = NULL;

  msSetup();

  /* Use MS_ERRORFILE and MS_DEBUGLEVEL env vars if set */
  if( msDebugInitFromEnv() != MS_SUCCESS ) {
    writeError();
    msCleanup();
    exit(0);
  }

  if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING) 
      msGettimeofday(&execstarttime, NULL);

  /* -------------------------------------------------------------------- */
  /*      Process arguments.  In normal use as a cgi-bin there are no     */
  /*      commandline switches, but we provide a few for test/debug       */
  /*      purposes, and to query the version info.                        */
  /* -------------------------------------------------------------------- */
  for( iArg = 1; iArg < argc; iArg++ ) {
    /* Keep only "-v", "-nh" and "QUERY_STRING=..." enabled by default.
     * The others will require an explicit -DMS_ENABLE_CGI_CL_DEBUG_ARGS
     * at compile time.
     */
    if( strcmp(argv[iArg],"-v") == 0 ) {
      printf("%s\n", msGetVersion());
      fflush(stdout);
      exit(0);
    } else if(strcmp(argv[iArg], "-nh") == 0) {
      sendheaders = MS_FALSE;
    } else if( strncmp(argv[iArg], "QUERY_STRING=", 13) == 0 ) {
      /* Debugging hook... pass "QUERY_STRING=..." on the command-line */
      putenv( "REQUEST_METHOD=GET" );
      putenv( argv[iArg] );
    }
#ifdef MS_ENABLE_CGI_CL_DEBUG_ARGS
    else if( iArg < argc-1 && strcmp(argv[iArg], "-tmpbase") == 0) {
      msForceTmpFileBase( argv[++iArg] );
    } else if( iArg < argc-1 && strcmp(argv[iArg], "-t") == 0) {
      char **tokens;
      int numtokens=0;

      if((tokens=msTokenizeMap(argv[iArg+1], &numtokens)) != NULL) {
        int i;
        for(i=0; i<numtokens; i++)
          printf("%s\n", tokens[i]);
        msFreeCharArray(tokens, numtokens);
      } else {
        writeError();
      }
            
      exit(0);
    } else if( strncmp(argv[iArg], "MS_ERRORFILE=", 13) == 0 ) {
        msSetErrorFile( argv[iArg] + 13, NULL );
    } else if( strncmp(argv[iArg], "MS_DEBUGLEVEL=", 14) == 0) {
      msSetGlobalDebugLevel( atoi(argv[iArg] + 14) );
    } 
#endif /* MS_ENABLE_CGI_CL_DEBUG_ARGS */
    else {
      /* we don't produce a usage message as some web servers pass junk arguments */
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Setup cleanup magic, mainly for FastCGI case.                   */
  /* -------------------------------------------------------------------- */
#ifndef WIN32
  signal( SIGUSR1, msCleanupOnSignal );
  signal( SIGTERM, msCleanupOnSignal );
#endif

#ifdef USE_FASTCGI
  msIO_installFastCGIRedirect();

#ifdef WIN32
  atexit( msCleanupOnExit );
#endif    

  /* In FastCGI case we loop accepting multiple requests.  In normal CGI */
  /* use we only accept and process one request.  */
  while( FCGI_Accept() >= 0 ) {
#endif /* def USE_FASTCGI */

    /* -------------------------------------------------------------------- */
    /*      Process a request.                                              */
    /* -------------------------------------------------------------------- */
    mapserv = msAllocMapServObj();
    mapserv->sendheaders = sendheaders; /* override the default if necessary (via command line -nh switch) */

    mapserv->request->NumParams = loadParams(mapserv->request, NULL, NULL, 0, NULL);
    if( mapserv->request->NumParams == -1 ) {
#ifdef USE_FASTCGI
      /* FCGI_ --- return to top of loop */
      msResetErrorList();
      continue;
#else
      /* normal case, processing is complete */
      msCleanup();
      exit( 0 );
#endif
    }

    mapserv->map = loadMap();

    if( mapserv->map->debug >= MS_DEBUGLEVEL_TUNING) 
      msGettimeofday(&requeststarttime, NULL);

#ifdef USE_FASTCGI
    if( mapserv->map->debug ) {
      static int nRequestCounter = 1;

      msDebug( "CGI Request %d on process %d\n", nRequestCounter, getpid() );
      nRequestCounter++;
    }
#endif

    /*
     * RFC-42 HTTP Cookie Forwarding
     * Here we set the http_cookie_data metadata to handle the 
     * HTTP Cookie Forwarding. The content of this metadata is the cookie 
     * content. In the future, this metadata will probably be replaced
     * by an object that is part of the mapObject that would contain 
     * information on the application status (such as cookie).
     */
    if( mapserv->request->httpcookiedata != NULL )
    {
        msInsertHashTable( &(mapserv->map->web.metadata), "http_cookie_data",
                           mapserv->request->httpcookiedata );
    }

    /*
    ** Determine 'mode': Check for MS_MODE env. var. and mode=... CGI param
    */
    mapserv->Mode = -1; /* Not set */
    if( setMode() != MS_SUCCESS)
        writeError();

    /*
    ** Start by calling the WMS/WFS/WCS Dispatchers.  If they fail then we'll 
    ** process this as a regular MapServer request.
    */
    if((mapserv->Mode == -1 || mapserv->Mode == OWS || mapserv->Mode == WFS) &&
       (status = msOWSDispatch(mapserv->map, mapserv->request, 
                               mapserv->Mode)) != MS_DONE  )  {
      /*
      ** OWSDispatch returned either MS_SUCCESS or MS_FAILURE
      **
      ** Normally if the OWS service fails it will issue an exception,
      ** and clear the error stack but still return MS_FAILURE.  But in
      ** a few situations it can't issue the exception and will instead 
      ** just an error and let us report it. 
      */
      if( status == MS_FAILURE ) {
        errorObj *ms_error = msGetErrorObj();
        
        if( (ms_error->code != MS_NOERR) && (ms_error->isreported == MS_FALSE) )
          writeError();
      }
       
      /* 
      ** This was a WMS/WFS request... cleanup and exit 
      ** At this point any error has already been handled
      ** as an XML exception by the OGC service.
      */
      if(mapserv->map->debug >= MS_DEBUGLEVEL_TUNING) {
        msGettimeofday(&requestendtime, NULL);
        msDebug("mapserv request processing time (msLoadMap not incl.): %.3fs\n", 
                (requestendtime.tv_sec+requestendtime.tv_usec/1.0e6)-
                (requeststarttime.tv_sec+requeststarttime.tv_usec/1.0e6) );
      }
      
      if (status == MS_SUCCESS &&
          strcasecmp(mapserv->map->imagetype, "application/openlayers")==0)
      {
        for( i=0; i<mapserv->request->NumParams; i++)
        {
          if(strcasecmp(mapserv->request->ParamNames[i], "SERVICE") == 0) {
            service = mapserv->request->ParamValues[i];
            break;
          }
        }
        if (service && strcasecmp(service,"WMS")==0)
        {
          msIO_printf("Content-type: text/html%c%c",10,10);
          
          if (msReturnOpenLayersPage(mapserv) != MS_SUCCESS)
            writeError();
        }
      }

      msFreeMapServObj(mapserv);      
#ifdef USE_FASTCGI
      msResetErrorList();
      /* FCGI_ --- return to top of loop */
      continue;
#else
      /* normal case, processing is complete */
      if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING) {
        msGettimeofday(&execendtime, NULL);
        msDebug("mapserv total execution time: %.3fs\n", 
                (execendtime.tv_sec+execendtime.tv_usec/1.0e6)-
                (execstarttime.tv_sec+execstarttime.tv_usec/1.0e6) );
      }
      msCleanup();
      exit( 0 );
#endif
    } /* done OGC/OWS case */

    /*
    ** Do "traditional" mode processing.
    */
    if (mapserv->Mode == -1)
        mapserv->Mode = BROWSE;

    loadForm();
 
    if(mapserv->savemap) {
      snprintf(buffer, sizeof(buffer), "%s%s%s.map", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id);
      if(msSaveMap(mapserv->map, buffer) == -1) writeError();
    }

    if((mapserv->CoordSource == FROMIMGPNT) || (mapserv->CoordSource == FROMIMGBOX)) /* make sure extent of existing image matches shape of image */
      mapserv->map->cellsize = msAdjustExtent(&mapserv->ImgExt, mapserv->ImgCols, mapserv->ImgRows);

    /*
    ** For each layer let's set layer status
    */
    for(i=0;i<mapserv->map->numlayers;i++) {
      if((GET_LAYER(mapserv->map, i)->status != MS_DEFAULT)) {
        if(isOn(mapserv,  GET_LAYER(mapserv->map, i)->name, GET_LAYER(mapserv->map, i)->group) == MS_TRUE) /* Set layer status */
          GET_LAYER(mapserv->map, i)->status = MS_ON;
        else
          GET_LAYER(mapserv->map, i)->status = MS_OFF;
      }
    }

    if(mapserv->CoordSource == FROMREFPNT) /* force browse mode if the reference coords are set */
      mapserv->Mode = BROWSE;

    /*
    ** Tile mode:
    ** Set the projection up and test the parameters for legality.
    */
    if(mapserv->Mode == TILE) {
      if( msTileSetup(mapserv) != MS_SUCCESS ) {
        writeError();
      }
    }

    if(mapserv->Mode == BROWSE) {

      char *template =  NULL;
      for(i=0;i<mapserv->request->NumParams;i++) /* find the template param value */
          if (strcasecmp(mapserv->request->ParamNames[i], "template") == 0)
              template = mapserv->request->ParamValues[i];

      if ( (!mapserv->map->web.template) && (template==NULL || (strcasecmp(template, "openlayers")!=0)) ) {
        msSetError(MS_WEBERR, "Traditional BROWSE mode requires a TEMPLATE in the WEB section, but none was provided.", "mapserv()");
        writeError();
      }

      if(QueryFile) {
        status = msLoadQuery(mapserv->map, QueryFile);
        if(status != MS_SUCCESS) writeError();
      }
      
      setExtent(mapserv);
      checkWebScale(mapserv);
       
      /* -------------------------------------------------------------------- */
      /*      generate map, legend, scalebar and refernce images.             */
      /* -------------------------------------------------------------------- */
      if(msGenerateImages(mapserv, MS_FALSE, MS_TRUE) != MS_SUCCESS)
        writeError();

      if ( (template != NULL) && (strcasecmp(template, "openlayers")==0) ) {
        msIO_printf("Content-type: text/html%c%c",10,10);
        if (msReturnOpenLayersPage(mapserv) != MS_SUCCESS)
          writeError();
      }
      else if(QueryFile) {
        if(msReturnTemplateQuery(mapserv, mapserv->map->web.queryformat, NULL) != MS_SUCCESS)
          writeError();
      } else {
        if(TEMPLATE_TYPE(mapserv->map->web.template) == MS_FILE) { /* if thers's an html template, then use it */
          if(mapserv->sendheaders) msIO_printf("Content-type: %s%c%c",  mapserv->map->web.browseformat, 10, 10); /* write MIME header */
          /* msIO_printf("<!-- %s -->\n", msGetVersion()); */
          fflush(stdout);
          if(msReturnPage(mapserv, mapserv->map->web.template, BROWSE, NULL) != MS_SUCCESS)
            writeError();
        } else {    
          if(msReturnURL(mapserv, mapserv->map->web.template, BROWSE) != MS_SUCCESS)
            writeError();
        }
      }

    } else if(mapserv->Mode == MAP || mapserv->Mode == SCALEBAR || mapserv->Mode == REFERENCE || mapserv->Mode == TILE) { /* "image" only modes */

      setExtent(mapserv);
      checkWebScale(mapserv);
      
      switch(mapserv->Mode) {
      case MAP:
        if(QueryFile) {
          status = msLoadQuery(mapserv->map, QueryFile);
          if(status != MS_SUCCESS) writeError();
          img = msDrawMap(mapserv->map, MS_TRUE);
        } else
          img = msDrawMap(mapserv->map, MS_FALSE);
        break;
      case REFERENCE:
        mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height);
        img = msDrawReferenceMap(mapserv->map);
        break;      
      case SCALEBAR:
        img = msDrawScalebar(mapserv->map);
        break;
      case TILE:
        msTileSetExtent(mapserv);
        img = msTileDraw(mapserv);
        break;
      }
      
      if(!img) writeError();
      
      /*
      ** Set the Cache control headers if the option is set. 
      */
      if( msLookupHashTable(&(mapserv->map->web.metadata), "http_max_age") ) {
        msIO_printf("Cache-Control: max-age=%s%c", msLookupHashTable(&(mapserv->map->web.metadata), "http_max_age"), 10);
      }

      if(mapserv->sendheaders)  {
        const char *attachment = msGetOutputFormatOption(mapserv->map->outputformat, "ATTACHMENT", NULL ); 
        if(attachment) msIO_printf("Content-disposition: attachment; filename=%s\n", attachment);
        msIO_printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(mapserv->map->outputformat), 10,10);
      }
            
      if( mapserv->Mode == MAP || mapserv->Mode == TILE )
        status = msSaveImage(mapserv->map, img, NULL);
      else
        status = msSaveImage(NULL,img, NULL);
          
      if(status != MS_SUCCESS) writeError();
      
      msFreeImage(img);
    } else if(mapserv->Mode == LEGEND) {
      if(mapserv->map->legend.template) {
        char *legendTemplate;

        legendTemplate = generateLegendTemplate(mapserv);
        if(legendTemplate) {
          if(mapserv->sendheaders) msIO_printf("Content-type: %s%c%c", mapserv->map->web.legendformat, 10, 10);
          msIO_fwrite(legendTemplate, strlen(legendTemplate), 1, stdout);

          free(legendTemplate);
        } else /* error already generated by (generateLegendTemplate()) */
          writeError();
      } else {
        img = msDrawLegend(mapserv->map, MS_FALSE);
        if(!img) writeError();

        if(mapserv->sendheaders) msIO_printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(mapserv->map->outputformat), 10,10);
        status = msSaveImage(NULL, img, NULL);
        if(status != MS_SUCCESS) writeError();

        msFreeImage(img);
      }
    } else if(mapserv->Mode == LEGENDICON) {
			char **tokens;
      int numtokens=0;
      int layerindex=-1, classindex=0;
      outputFormatObj *format = NULL;

      /* TODO: do we want to set scale here? */

      /* do we have enough information */
      if(!mapserv->icon) {
        msSetError(MS_WEBERR, "Mode=LEGENDICON requires an icon parameter.", "mapserv()");
        writeError();
      }

      /* process the icon definition */
      tokens = msStringSplit(mapserv->icon, ',', &numtokens);

      if(numtokens != 1 && numtokens != 2) {
        msSetError(MS_WEBERR, "%d Malformed icon parameter, should be 'layer,class' or just 'layer' if the layer has only 1 class defined.", "mapserv()", numtokens);
        writeError();
      }

      if((layerindex = msGetLayerIndex(mapserv->map, tokens[0])) == -1) {
        msSetError(MS_WEBERR, "Icon layer=%s not found in mapfile.", "mapserv()", tokens[0]);
        writeError();
      }

      if(numtokens == 2) { /* check the class index */
        classindex = atoi(tokens[1]);
        if(classindex >= GET_LAYER(mapserv->map, layerindex)->numclasses) {
          msSetError(MS_WEBERR, "Icon class=%d not found in layer=%s.", "mapserv()", classindex, GET_LAYER(mapserv->map, layerindex)->name);
          writeError();
        }
      }

      /* ensure we have an image format representing the options for the legend. */
      msApplyOutputFormat(&format, mapserv->map->outputformat, mapserv->map->legend.transparent, mapserv->map->legend.interlace, MS_NOOVERRIDE);

      /* initialize the legend image */
      if( ! MS_RENDERER_PLUGIN(format) ) {
    	  msSetError(MS_RENDERERERR, "unsupported renderer for legend icon", "mapserv main()");
    	  writeError();
      }
      img = msImageCreate(mapserv->map->legend.keysizex, mapserv->map->legend.keysizey, format,
    		  mapserv->map->web.imagepath, mapserv->map->web.imageurl, mapserv->map->resolution, mapserv->map->defresolution,
    		  &(mapserv->map->legend.imagecolor));

      /* drop this reference to output format */
      msApplyOutputFormat(&format, NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE);

      if(msDrawLegendIcon(mapserv->map, GET_LAYER(mapserv->map, layerindex), GET_LAYER(mapserv->map, layerindex)->class[classindex], mapserv->map->legend.keysizex,  mapserv->map->legend.keysizey, img, 0, 0) != MS_SUCCESS)
        writeError();

      if(mapserv->sendheaders) msIO_printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(mapserv->map->outputformat), 10,10);
      status = msSaveImage(NULL, img, NULL);
      if(status != MS_SUCCESS) writeError();

      msFreeCharArray(tokens, numtokens);
      msFreeImage(img);

    } else if(mapserv->Mode >= QUERY) { /* query modes */
      if(QueryFile) { /* already got a completed query */
        status = msLoadQuery(mapserv->map, QueryFile);
        if(status != MS_SUCCESS) writeError();
      } else {

        if((QueryLayerIndex = msGetLayerIndex(mapserv->map, QueryLayer)) != -1) /* force the query layer on */
          GET_LAYER(mapserv->map, QueryLayerIndex)->status = MS_ON;

        switch(mapserv->Mode) {
        case ITEMFEATUREQUERY:
        case ITEMFEATURENQUERY:
          if((SelectLayerIndex = msGetLayerIndex(mapserv->map, SelectLayer)) == -1) { /* force the selection layer on */
            msSetError(MS_WEBERR, "Selection layer not set or references an invalid layer.", "mapserv()"); 
            writeError();
          }
          GET_LAYER(mapserv->map, SelectLayerIndex)->status = MS_ON;

          /* validate the qstring parameter */
          if(msValidateParameter(QueryString, msLookupHashTable(&(GET_LAYER(mapserv->map, SelectLayerIndex)->validation), "qstring"), 
                                              msLookupHashTable(&(mapserv->map->web.validation), "qstring"), 
                                              msLookupHashTable(&(GET_LAYER(mapserv->map, SelectLayerIndex)->metadata), "qstring_validation_pattern"), NULL) != MS_SUCCESS) {
	    msSetError(MS_WEBERR, "Parameter 'qstring' value fails to validate.", "mapserv()");
	    writeError();
          }

          if(QueryCoordSource != NONE && !mapserv->UseShapes)
            setExtent(mapserv); /* set user area of interest */

	  mapserv->map->query.type = MS_QUERY_BY_ATTRIBUTE;
          if(QueryItem) mapserv->map->query.item = msStrdup(QueryItem);
          if(QueryString) mapserv->map->query.str = msStrdup(QueryString);

          mapserv->map->query.rect = mapserv->map->extent;

          mapserv->map->query.mode = MS_QUERY_MULTIPLE;
          if(mapserv->Mode == ITEMFEATUREQUERY)
            mapserv->map->query.mode = MS_QUERY_SINGLE;

          mapserv->map->query.layer = QueryLayerIndex;
	  mapserv->map->query.slayer = SelectLayerIndex; /* this will trigger the feature query eventually */
          break;
        case FEATUREQUERY:
        case FEATURENQUERY:
          if((SelectLayerIndex = msGetLayerIndex(mapserv->map, SelectLayer)) == -1) { /* force the selection layer on */
            msSetError(MS_WEBERR, "Selection layer not set or references an invalid layer.", "mapserv()"); 
            writeError();
          }
          GET_LAYER(mapserv->map, SelectLayerIndex)->status = MS_ON;
      
          if(mapserv->Mode == FEATUREQUERY) {
            switch(QueryCoordSource) {
            case FROMIMGPNT:
              mapserv->map->extent = mapserv->ImgExt; /* use the existing map extent */    
              setCoordinate();
              break;
            case FROMUSERPNT:
              break;
            default:
              msSetError(MS_WEBERR, "No way to perform the initial search, not enough information.", "mapserv()");
              writeError();
              break;
            }      

            mapserv->map->query.type = MS_QUERY_BY_POINT;
            mapserv->map->query.mode = MS_QUERY_SINGLE;

            mapserv->map->query.point = mapserv->mappnt;
	    mapserv->map->query.buffer = mapserv->Buffer;

            mapserv->map->query.layer = QueryLayerIndex;
	    mapserv->map->query.slayer = SelectLayerIndex; /* this will trigger the feature query eventually */
          } else { /* FEATURENQUERY */
            switch(QueryCoordSource) {
            case FROMIMGPNT:
              mapserv->map->extent = mapserv->ImgExt; /* use the existing map extent */    
              setCoordinate();
              mapserv->map->query.type = MS_QUERY_BY_POINT;
              break;     
            case FROMIMGBOX:
              /* TODO: this option was present but with no code to leverage the image box... */
              break;
            case FROMUSERPNT:
              mapserv->map->query.type = MS_QUERY_BY_POINT;
            default:
              setExtent(mapserv);
              mapserv->map->query.type = MS_QUERY_BY_RECT;
              break;
            }
          }

          mapserv->map->query.mode = MS_QUERY_MULTIPLE;

          mapserv->map->query.rect = mapserv->map->extent;
          mapserv->map->query.point = mapserv->mappnt;
          mapserv->map->query.buffer = mapserv->Buffer;

          mapserv->map->query.layer = QueryLayerIndex;
	  mapserv->map->query.slayer = SelectLayerIndex;
          break;
        case ITEMQUERY:
        case ITEMNQUERY:
          if(QueryLayerIndex < 0 || QueryLayerIndex >= mapserv->map->numlayers) {
            msSetError(MS_WEBERR, "Query layer not set or references an invalid layer.", "mapserv()"); 
            writeError();
          }

	  /* validate the qstring parameter */
          if(msValidateParameter(QueryString, msLookupHashTable(&(GET_LAYER(mapserv->map, QueryLayerIndex)->validation), "qstring"),
				 msLookupHashTable(&(mapserv->map->web.validation), "qstring"),
				 msLookupHashTable(&(GET_LAYER(mapserv->map, QueryLayerIndex)->metadata), "qstring_validation_pattern"), NULL) != MS_SUCCESS) {
            msSetError(MS_WEBERR, "Parameter 'qstring' value fails to validate.", "mapserv()");
            writeError();
          }

          if(QueryCoordSource != NONE && !mapserv->UseShapes)
            setExtent(mapserv); /* set user area of interest */

	  mapserv->map->query.type = MS_QUERY_BY_ATTRIBUTE;
	  mapserv->map->query.layer = QueryLayerIndex;
          if(QueryItem) mapserv->map->query.item = msStrdup(QueryItem);
          if(QueryString) mapserv->map->query.str = msStrdup(QueryString);

	  mapserv->map->query.rect = mapserv->map->extent;

	  mapserv->map->query.mode = MS_QUERY_MULTIPLE;
          if(mapserv->Mode == ITEMQUERY) mapserv->map->query.mode = MS_QUERY_SINGLE;
          break;
        case NQUERY:
          mapserv->map->query.mode = MS_QUERY_MULTIPLE; /* all of these cases return multiple results */
          mapserv->map->query.layer = QueryLayerIndex;

          switch(QueryCoordSource) {
          case FROMIMGPNT:      
            setCoordinate();
            
            if(SearchMap) { /* compute new extent, pan etc then search that extent */
              setExtent(mapserv);
              mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height);
              if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();
              mapserv->map->query.rect = mapserv->map->extent;
	      mapserv->map->query.type = MS_QUERY_BY_RECT; 
            } else {
              mapserv->map->extent = mapserv->ImgExt; /* use the existing image parameters */
              mapserv->map->width = mapserv->ImgCols;
              mapserv->map->height = mapserv->ImgRows;
              if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();     
              mapserv->map->query.point = mapserv->mappnt;
              mapserv->map->query.type = MS_QUERY_BY_POINT;
            }

            break;      
          case FROMIMGBOX:      
            if(SearchMap) { /* compute new extent, pan etc then search that extent */
              setExtent(mapserv);
              if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();
              mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height);
              mapserv->map->query.rect = mapserv->map->extent;
              mapserv->map->query.type = MS_QUERY_BY_RECT;
            } else {
              double cellx, celly;
        
              mapserv->map->extent = mapserv->ImgExt; /* use the existing image parameters */
              mapserv->map->width = mapserv->ImgCols;
              mapserv->map->height = mapserv->ImgRows;
              if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();    
              cellx = MS_CELLSIZE(mapserv->ImgExt.minx, mapserv->ImgExt.maxx, mapserv->ImgCols); /* calculate the new search extent */
              celly = MS_CELLSIZE(mapserv->ImgExt.miny, mapserv->ImgExt.maxy, mapserv->ImgRows);
              mapserv->RawExt.minx = MS_IMAGE2MAP_X(mapserv->ImgBox.minx, mapserv->ImgExt.minx, cellx);          
              mapserv->RawExt.maxx = MS_IMAGE2MAP_X(mapserv->ImgBox.maxx, mapserv->ImgExt.minx, cellx);
              mapserv->RawExt.maxy = MS_IMAGE2MAP_Y(mapserv->ImgBox.miny, mapserv->ImgExt.maxy, celly); /* y's are flip flopped because img/map coordinate systems are */
              mapserv->RawExt.miny = MS_IMAGE2MAP_Y(mapserv->ImgBox.maxy, mapserv->ImgExt.maxy, celly);

              mapserv->map->query.rect = mapserv->RawExt;
              mapserv->map->query.type = MS_QUERY_BY_RECT;
            }
            break;
          case FROMIMGSHAPE:
            mapserv->map->extent = mapserv->ImgExt; /* use the existing image parameters */
            mapserv->map->width = mapserv->ImgCols;
            mapserv->map->height = mapserv->ImgRows;
            mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height);
            if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();
      
            /* convert from image to map coordinates here (see setCoordinate) */
            for(i=0; i<mapserv->map->query.shape->numlines; i++) {
              for(j=0; j<mapserv->map->query.shape->line[i].numpoints; j++) {
                mapserv->map->query.shape->line[i].point[j].x = MS_IMAGE2MAP_X(mapserv->map->query.shape->line[i].point[j].x, mapserv->map->extent.minx, mapserv->map->cellsize);
                mapserv->map->query.shape->line[i].point[j].y = MS_IMAGE2MAP_Y(mapserv->map->query.shape->line[i].point[j].y, mapserv->map->extent.maxy, mapserv->map->cellsize);             
              }
            }

            mapserv->map->query.type = MS_QUERY_BY_SHAPE;
            break;      
          case FROMUSERPNT:
            if(mapserv->Buffer == 0) { /* do a *pure* point query */
	      mapserv->map->query.point = mapserv->mappnt;
              mapserv->map->query.type = MS_QUERY_BY_POINT;
              setExtent(mapserv);
            } else {
              setExtent(mapserv);
              if(SearchMap) { /* the extent should be tied to a map, so we need to "adjust" it */
                if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();
                mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height); 
              }
              mapserv->map->query.rect = mapserv->map->extent;
              mapserv->map->query.type = MS_QUERY_BY_RECT;
            }
            break;
          case FROMUSERSHAPE:
            setExtent(mapserv);
            mapserv->map->query.type = MS_QUERY_BY_SHAPE;
            break;
          default: /* from an extent of some sort */
            setExtent(mapserv);
            if(SearchMap) { /* the extent should be tied to a map, so we need to "adjust" it */
              if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();
              mapserv->map->cellsize = msAdjustExtent(&(mapserv->map->extent), mapserv->map->width, mapserv->map->height);
            }

	    mapserv->map->query.rect = mapserv->map->extent;
	    mapserv->map->query.type = MS_QUERY_BY_RECT;
            break;
	  }
          break;
        case QUERY:
          switch(QueryCoordSource) {
          case FROMIMGPNT:
            setCoordinate();
            mapserv->map->extent = mapserv->ImgExt; /* use the existing image parameters */
            mapserv->map->width = mapserv->ImgCols;
            mapserv->map->height = mapserv->ImgRows;
            if((status = msCalculateScale(mapserv->map->extent, mapserv->map->units, mapserv->map->width, mapserv->map->height, mapserv->map->resolution, &mapserv->map->scaledenom)) != MS_SUCCESS) writeError();
            break;
          case FROMUSERPNT: /* only a buffer makes sense, DOES IT? */    
            setExtent(mapserv);    
            break;
          default:
            msSetError(MS_WEBERR, "Query mode needs a point, imgxy and mapxy are not set.", "mapserv()");
            writeError();
            break;
          }

          mapserv->map->query.type = MS_QUERY_BY_POINT;
          mapserv->map->query.mode = MS_QUERY_SINGLE;
          mapserv->map->query.layer = QueryLayerIndex;
          mapserv->map->query.point = mapserv->mappnt;
          mapserv->map->query.buffer = mapserv->Buffer;          
          break;
        case INDEXQUERY:
          mapserv->map->query.type = MS_QUERY_BY_INDEX;
          mapserv->map->query.mode = MS_QUERY_SINGLE;
          mapserv->map->query.layer = QueryLayerIndex;
          mapserv->map->query.shapeindex = ShapeIndex;
          mapserv->map->query.tileindex = TileIndex;
          break;
        } /* end mode switch */

        /* finally execute the query */
        if((status = msExecuteQuery(mapserv->map)) != MS_SUCCESS) writeError();
      }
      
      if(mapserv->map->querymap.width != -1) mapserv->map->width = mapserv->map->querymap.width; /* make sure we use the right size */
      if(mapserv->map->querymap.height != -1) mapserv->map->height = mapserv->map->querymap.height;

      if(mapserv->UseShapes)
        setExtentFromShapes();

      if(msReturnTemplateQuery(mapserv, mapserv->map->web.queryformat, NULL) != MS_SUCCESS) writeError();
          
      if(mapserv->savequery) {
        snprintf(buffer, sizeof(buffer), "%s%s%s%s", mapserv->map->web.imagepath, mapserv->map->name, mapserv->Id, MS_QUERY_EXTENSION);
        if((status = msSaveQuery(mapserv->map, buffer, MS_FALSE)) != MS_SUCCESS) return status;
      }
      
    } else if(mapserv->Mode == COORDINATE) {
      setCoordinate(); /* mouse click => map coord */
      returnCoordinate(mapserv->mappnt);
    }

    writeLog(MS_FALSE);
   
    /* Clean-up (the following are not stored as part of the mapserv) */
    if(QueryItem) free(QueryItem);
    if(QueryString) free(QueryString);
    if(QueryLayer) free(QueryLayer);
    if(SelectLayer) free(SelectLayer);
    if(QueryFile) free(QueryFile);

    if(mapserv->map->debug >= MS_DEBUGLEVEL_TUNING) {
      msGettimeofday(&requestendtime, NULL);
      msDebug("mapserv request processing time (loadmap not incl.): %.3fs\n", 
              (requestendtime.tv_sec+requestendtime.tv_usec/1.0e6)-
              (requeststarttime.tv_sec+requeststarttime.tv_usec/1.0e6) );
    }

    msFreeMapServObj(mapserv);

#ifdef USE_FASTCGI
    msResetErrorList();
  }
#endif

  if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&execendtime, NULL);
    msDebug("mapserv total execution time: %.3fs\n", 
            (execendtime.tv_sec+execendtime.tv_usec/1.0e6)-
            (execstarttime.tv_sec+execstarttime.tv_usec/1.0e6) );
  }

  msCleanup();

  exit(0); /* end MapServer */
} 
