/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  shapeObj to GML output via MapServer queries.
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

  msIO_fprintf(stream, "%s\t\t<gml:coordinates>\n", tab);  
  msIO_fprintf(stream, "%s\t\t\t%.6f,%.6f %.6f,%.6f\n", tab, rect->minx, rect->miny, rect->maxx, rect->maxy );   
  msIO_fprintf(stream, "%s\t\t</gml:coordinates>\n", tab);
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

/* GML 2.1.2 */
static int gmlWriteGeometry_GML2(FILE *stream, shapeObj *shape, const char *srsname, char *tab) 
{
  int i, j, k;
  int *innerlist, *outerlist, numouters;
  char *srsname_encoded = NULL;

  if(!stream) return(MS_FAILURE);
  if(!shape) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  if(shape->numlines <= 0) return(MS_SUCCESS); /* empty shape, nothing to output */

  if(srsname)
    srsname_encoded = msEncodeHTMLEntities(srsname);

  /* feature geometry */
  switch(shape->type) {
  case(MS_SHAPE_POINT):
    if(shape->line[0].numpoints == 1) { /* write a Point */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:Point srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:Point>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[0].point[0].x, shape->line[0].point[0].y);

      msIO_fprintf(stream, "%s</gml:Point>\n", tab);
    } else { /* write a MultiPoint */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiPoint srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiPoint>\n", tab);
     
      for(i=0; i<shape->line[0].numpoints; i++) {
        msIO_fprintf(stream, "%s\t<gml:Point>\n", tab);
        msIO_fprintf(stream, "%s\t\t<gml:coordinates>%f,%f</gml:coordinates>\n", tab, shape->line[0].point[i].x, shape->line[0].point[i].y);
        msIO_fprintf(stream, "%s\t</gml:Point>\n", tab);
      }

      msIO_fprintf(stream, "%s</gml:MultiPoint>\n", tab);
    }
    break;
  case(MS_SHAPE_LINE):
    if(shape->numlines == 1) { /* write a LineString */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:LineString srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:LineString>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:coordinates>", tab);
      for(i=0; i<shape->line[0].numpoints; i++) /* was numpoints-1? */
        msIO_fprintf(stream, "%f,%f ", shape->line[0].point[i].x, shape->line[0].point[i].y);
      msIO_fprintf(stream, "</gml:coordinates>\n");

      msIO_fprintf(stream, "%s</gml:LineString>\n", tab);
    } else { /* write a MultiLineString */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiLineString srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiLineString>\n", tab);

      for(j=0; j<shape->numlines; j++) {
        msIO_fprintf(stream, "%s\t<gml:LineString>\n", tab); /* no srsname at this point */

        msIO_fprintf(stream, "%s\t\t<gml:coordinates>", tab);
        for(i=0; i<shape->line[j].numpoints; i++)
	  msIO_fprintf(stream, "%f,%f", shape->line[j].point[i].x, shape->line[j].point[i].y);
        msIO_fprintf(stream, "</gml:coordinates>\n");
        msIO_fprintf(stream, "%s\t</gml:LineString>\n", tab);
      }

      msIO_fprintf(stream, "%s</gml:MultiLineString>\n", tab);
    }
    break;
  case(MS_SHAPE_POLYGON): /* this gets nasty, since our shapes are so flexible */
    if(shape->numlines == 1) { /* write a basic Polygon (no interior rings) */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:outerBoundaryIs>\n", tab);
      msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

      msIO_fprintf(stream, "%s\t\t\t<gml:coordinates>", tab);
      for(j=0; j<shape->line[0].numpoints; j++)
	msIO_fprintf(stream, "%f,%f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
      msIO_fprintf(stream, "</gml:coordinates>\n");

      msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
      msIO_fprintf(stream, "%s\t</gml:outerBoundaryIs>\n", tab);

      msIO_fprintf(stream, "%s</gml:Polygon>\n", tab);
    } else { /* need to test for inner and outer rings */
      outerlist = msGetOuterList(shape);

      numouters = 0;
      for(i=0; i<shape->numlines; i++)
        if(outerlist[i] == MS_TRUE) numouters++;

      if(numouters == 1) { /* write a Polygon (with interior rings) */
	for(i=0; i<shape->numlines; i++) /* find the outer ring */
          if(outerlist[i] == MS_TRUE) break;

	innerlist = msGetInnerList(shape, i, outerlist);

	if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

        msIO_fprintf(stream, "%s\t<gml:outerBoundaryIs>\n", tab);
        msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

        msIO_fprintf(stream, "%s\t\t\t<gml:coordinates>", tab);
        for(j=0; j<shape->line[i].numpoints; j++)
	  msIO_fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
        msIO_fprintf(stream, "</gml:coordinates>\n");

        msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
        msIO_fprintf(stream, "%s\t</gml:outerBoundaryIs>\n", tab);

	for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
	  if(innerlist[k] == MS_TRUE) {
	    msIO_fprintf(stream, "%s\t<gml:innerBoundaryIs>\n", tab);
            msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s\t\t\t<gml:coordinates>", tab);
            for(j=0; j<shape->line[k].numpoints; j++)
	      msIO_fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
            msIO_fprintf(stream, "</gml:coordinates>\n");

            msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s\t</gml:innerBoundaryIs>\n", tab);
          }
        }

        msIO_fprintf(stream, "%s</gml:Polygon>\n", tab);
	free(innerlist);
      } else {  /* write a MultiPolygon	 */
	if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:MultiPolygon srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:MultiPolygon>\n", tab);
        
	for(i=0; i<shape->numlines; i++) { /* step through the outer rings */
          if(outerlist[i] == MS_TRUE) {
  	    innerlist = msGetInnerList(shape, i, outerlist);

            msIO_fprintf(stream, "%s<gml:polygonMember>\n", tab);

            msIO_fprintf(stream, "%s\t<gml:Polygon>\n", tab);

            msIO_fprintf(stream, "%s\t\t<gml:outerBoundaryIs>\n", tab);
            msIO_fprintf(stream, "%s\t\t\t<gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s\t\t\t\t<gml:coordinates>", tab);
            for(j=0; j<shape->line[i].numpoints; j++)
	      msIO_fprintf(stream, "%f,%f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "</gml:coordinates>\n");

            msIO_fprintf(stream, "%s\t\t\t</gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s\t\t</gml:outerBoundaryIs>\n", tab);

	    for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
	      if(innerlist[k] == MS_TRUE) {
	        msIO_fprintf(stream, "%s\t\t<gml:innerBoundaryIs>\n", tab);
                msIO_fprintf(stream, "%s\t\t\t<gml:LinearRing>\n", tab);

                msIO_fprintf(stream, "%s\t\t\t\t<gml:coordinates>", tab);
                for(j=0; j<shape->line[k].numpoints; j++)
	          msIO_fprintf(stream, "%f,%f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
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

  /* clean-up */
  msFree(srsname_encoded);

  return(MS_SUCCESS);
}

/* GML 3.1 (MapServer limits GML encoding to the level 0 profile) */
static int gmlWriteGeometry_GML3(FILE *stream, shapeObj *shape, const char *srsname, char *tab) 
{
  int i, j, k;
  int *innerlist, *outerlist, numouters;
  char *srsname_encoded = NULL;

  if(!stream) return(MS_FAILURE);
  if(!shape) return(MS_FAILURE);
  if(!tab) return(MS_FAILURE);

  if(shape->numlines <= 0) return(MS_SUCCESS); /* empty shape, nothing to output */

  if(srsname)
    srsname_encoded = msEncodeHTMLEntities(srsname);

  /* TODO: What about ID's? */

  /* feature geometry */
  switch(shape->type) {
  case(MS_SHAPE_POINT):
    if(shape->line[0].numpoints == 1) { /* write a Point */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:Point srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:Point>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:pos>%f %f</gml:pos>\n", tab, shape->line[0].point[0].x, shape->line[0].point[0].y);

      msIO_fprintf(stream, "%s</gml:Point>\n", tab);
    } else { /* write a MultiPoint */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiPoint srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiPoint>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:pointMembers>\n", tab);
      for(i=0; i<shape->line[0].numpoints; i++) {
        msIO_fprintf(stream, "%s\t\t<gml:Point>\n", tab);
        msIO_fprintf(stream, "%s\t\t\t<gml:pos>%f %f</gml:pos>\n", tab, shape->line[0].point[i].x, shape->line[0].point[i].y);
        msIO_fprintf(stream, "%s\t\t</gml:Point>\n", tab);
      }
      msIO_fprintf(stream, "%s\t</gml:pointMembers>\n", tab);

      msIO_fprintf(stream, "%s</gml:MultiPoint>\n", tab);
    }
    break;
  case(MS_SHAPE_LINE):
    if(shape->numlines == 1) { /* write a LineString */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:LineString srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:LineString>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:posList dimension=\"2\">", tab);
      for(i=0; i<shape->line[0].numpoints; i++)  /* was numpoints-1? */
        msIO_fprintf(stream, "%f %f ", shape->line[0].point[i].x, shape->line[0].point[i].y);
      msIO_fprintf(stream, "</gml:posList>\n");

      msIO_fprintf(stream, "%s</gml:LineString>\n", tab);
    } else { /* write a MultiCurve (MultiLineString is depricated in GML3) */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:MultiCurve srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:MultiCurve>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:curveMembers>\n", tab);
      for(j=0; j<shape->numlines; j++) {
        msIO_fprintf(stream, "%s\t\t<gml:LineString>\n", tab); /* no srsname at this point */

        msIO_fprintf(stream, "%s\t\t\t<gml:posList dimension=\"2\">", tab);
        for(i=0; i<shape->line[j].numpoints; i++)
	  msIO_fprintf(stream, "%f %f ", shape->line[j].point[i].x, shape->line[j].point[i].y);
        msIO_fprintf(stream, "</gml:posList>\n");
        msIO_fprintf(stream, "%s\t\t</gml:LineString>\n", tab);
      }
      msIO_fprintf(stream, "%s\t</gml:curveMembers>\n", tab);
  
      msIO_fprintf(stream, "%s</gml:MultiCurve>\n", tab);
    }
    break;
  case(MS_SHAPE_POLYGON): /* this gets nasty, since our shapes are so flexible */
    if(shape->numlines == 1) { /* write a basic polygon (no interior rings) */
      if(srsname_encoded)
        msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, srsname_encoded);
      else
        msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

      msIO_fprintf(stream, "%s\t<gml:exterior>\n", tab);
      msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

      msIO_fprintf(stream, "%s\t\t\t<gml:posList dimension=\"2\">", tab);
      for(j=0; j<shape->line[0].numpoints; j++)
	msIO_fprintf(stream, "%f %f ", shape->line[0].point[j].x, shape->line[0].point[j].y);
      msIO_fprintf(stream, "</gml:posList>\n");

      msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
      msIO_fprintf(stream, "%s\t</gml:exterior>\n", tab);

      msIO_fprintf(stream, "%s</gml:Polygon>\n", tab);
    } else { /* need to test for inner and outer rings */
      outerlist = msGetOuterList(shape);

      numouters = 0;
      for(i=0; i<shape->numlines; i++)
        if(outerlist[i] == MS_TRUE) numouters++;

      if(numouters == 1) { /* write a Polygon (with interior rings) */
	for(i=0; i<shape->numlines; i++) /* find the outer ring */
          if(outerlist[i] == MS_TRUE) break;

	innerlist = msGetInnerList(shape, i, outerlist);

	if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:Polygon srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:Polygon>\n", tab);

        msIO_fprintf(stream, "%s\t<gml:exterior>\n", tab);
        msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

        msIO_fprintf(stream, "%s\t\t\t<gml:posList dimension=\"2\">", tab);
        for(j=0; j<shape->line[i].numpoints; j++)
	  msIO_fprintf(stream, "%f %f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
        msIO_fprintf(stream, "</gml:posList>\n");

        msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
        msIO_fprintf(stream, "%s\t</gml:exterior>\n", tab);

	for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
	  if(innerlist[k] == MS_TRUE) {
	    msIO_fprintf(stream, "%s\t<gml:interior>\n", tab);
            msIO_fprintf(stream, "%s\t\t<gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s\t\t\t<gml:posList dimension=\"2\">", tab);
            for(j=0; j<shape->line[k].numpoints; j++)
	      msIO_fprintf(stream, "%f %f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
            msIO_fprintf(stream, "</gml:posList>\n");

            msIO_fprintf(stream, "%s\t\t</gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s\t</gml:interior>\n", tab);
          }
        }

        msIO_fprintf(stream, "%s</gml:Polygon>\n", tab);
	free(innerlist);
      } else {  /* write a MultiSurface (MultiPolygon is depricated in GML3) */
	if(srsname_encoded)
          msIO_fprintf(stream, "%s<gml:MultiSurface srsName=\"%s\">\n", tab, srsname_encoded);
        else
          msIO_fprintf(stream, "%s<gml:MultiSurface>\n", tab);

        msIO_fprintf(stream, "%s\t<gml:surfaceMembers>\n", tab);
	for(i=0; i<shape->numlines; i++) { /* step through the outer rings */
          if(outerlist[i] == MS_TRUE) {
  	    innerlist = msGetInnerList(shape, i, outerlist);            

            msIO_fprintf(stream, "%s\t\t<gml:Polygon>\n", tab);

            msIO_fprintf(stream, "%s\t\t\t<gml:exterior>\n", tab);
            msIO_fprintf(stream, "%s\t\t\t\t<gml:LinearRing>\n", tab);

            msIO_fprintf(stream, "%s\t\t\t\t\t<gml:posList dimension=\"2\">", tab);
            for(j=0; j<shape->line[i].numpoints; j++)
	      msIO_fprintf(stream, "%f %f ", shape->line[i].point[j].x, shape->line[i].point[j].y);
            msIO_fprintf(stream, "</gml:posList>\n");

            msIO_fprintf(stream, "%s\t\t\t\t</gml:LinearRing>\n", tab);
            msIO_fprintf(stream, "%s\t\t\t</gml:exterior>\n", tab);

	    for(k=0; k<shape->numlines; k++) { /* now step through all the inner rings */
	      if(innerlist[k] == MS_TRUE) {
	        msIO_fprintf(stream, "%s\t\t\t<gml:interior>\n", tab);
                msIO_fprintf(stream, "%s\t\t\t\t<gml:LinearRing>\n", tab);

                msIO_fprintf(stream, "%s\t\t\t\t\t<gml:posList dimension=\"2\">", tab);
                for(j=0; j<shape->line[k].numpoints; j++)
	          msIO_fprintf(stream, "%f %f ", shape->line[k].point[j].x, shape->line[k].point[j].y);
                msIO_fprintf(stream, "</gml:posList>\n");

                msIO_fprintf(stream, "%s\t\t\t\t</gml:LinearRing>\n", tab);
                msIO_fprintf(stream, "%s\t\t\t</gml:interior>\n", tab);
              }
            }

            msIO_fprintf(stream, "%s\t\t</gml:Polygon>\n", tab);           
	
	    free(innerlist);
          }
        }
	msIO_fprintf(stream, "%s\t</gml:surfaceMembers>\n", tab);
        msIO_fprintf(stream, "%s</gml:MultiSurface>\n", tab);
      }
      free(outerlist);
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

static int gmlWriteGeometry(FILE *stream, int format, shapeObj *shape, const char *srsname, char *tab)
{
  switch(format) {
  case(OWS_GML2):
    return gmlWriteGeometry_GML2(stream, shape, srsname, tab);
    break;
  case(OWS_GML3):
    return gmlWriteGeometry_GML3(stream, shape, srsname, tab);
    break;
  default:
    msSetError(MS_IOERR, "Unsupported GML format.", "gmlWriteGeometry()");
  } 

 return(MS_FAILURE);
}

typedef struct {
  char *name;     /* name of the item */
  char *alias;    /* is the item aliased for presentation? (NULL if not) */
  char *type;     /* raw type for thes item (NULL for a "string") (TODO: should this be a lookup table instead?) */
  int encode;     /* should the value be HTML encoded? Default is MS_TRUE */
  int visible;    /* should this item be output, default is MS_FALSE */
} gmlItemObj;

typedef struct {
  gmlItemObj *items;
  int numitems;
} gmlItemListObj;

typedef struct {
  char *name;          /* name of the group */
  char **items;        /* list of items in the group */
  int numitems;        /* number of items */
} gmlGroupObj;

typedef struct {
  gmlGroupObj *groups;
  int numgroups;
} gmlGroupListObj;

int msItemInGroups(gmlItemObj *item, gmlGroupListObj *groupList) {
  int i, j;
  gmlGroupObj *group;

  if(!groupList) return MS_FALSE; /* no groups */

  for(i=0; i<groupList->numgroups; i++) {
    group = &(groupList->groups[i]);
    for(j=0; j<group->numitems; j++) {
      if(strcasecmp(item->name, group->items[j]) == 0) return MS_TRUE;
    }
  }

  return MS_FALSE;
}

gmlItemListObj *msGMLGetItems(layerObj *layer) 
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
  if((value = msLookupHashTable(&(layer->metadata), "gml_include_items")) != NULL)  
    incitems = split(value, ',', &numincitems);

  /* get a list of items that should be excluded in output */
  if((value = msLookupHashTable(&(layer->metadata), "gml_exclude_items")) != NULL)  
    excitems = split(value, ',', &numexcitems);

  /* get a list of items that need don't get encoded */
  if((value = msLookupHashTable(&(layer->metadata), "gml_xml_items")) != NULL)  
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

    snprintf(tag, 64, "gml_%s_alias", layer->items[i]);
    if((value = msLookupHashTable(&(layer->metadata), tag)) != NULL) 
      item->alias = strdup(value);

    snprintf(tag, 64, "gml_%s_type", layer->items[i]);
    if((value = msLookupHashTable(&(layer->metadata), tag)) != NULL) 
      item->type = strdup(value);
  }

  msFreeCharArray(xmlitems, numxmlitems);
  msFreeCharArray(incitems, numincitems);
  msFreeCharArray(excitems, numexcitems);

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
  }

  free(itemList);
}

void msGMLWriteItem(FILE *stream, gmlItemObj *item, char *value, const char *namespace, const char *tab) 
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

  /* build from pieces - set tag name and need for a namespace */
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

  return;
}

gmlGroupListObj *msGMLGetGroups(layerObj *layer)
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
  if((value = msLookupHashTable(&(layer->metadata), "gml_groups")) != NULL) {
    names = split(value, ',', &numnames);

    /* allocation an array of gmlGroupObj's */
    groupList->numgroups = numnames;
    groupList->groups = (gmlGroupObj *) malloc(sizeof(gmlGroupObj)*groupList->numgroups);

    for(i=0; i<groupList->numgroups; i++) {
      group = &(groupList->groups[i]);

      group->name = strdup(names[i]); /* initialize a few things */
      group->items = NULL;
      group->numitems = 0;
      
      snprintf(tag, 64, "gml_%s_group", group->name);
      if((value = msLookupHashTable(&(layer->metadata), tag)) != NULL)
	group->items = split(value, ',', &group->numitems);
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
  }

  free(groupList);
}

void msGMLWriteGroup(FILE *stream, gmlGroupObj *group, shapeObj *shape, gmlItemListObj *itemList, const char *namespace, const char *tab)
{
  int i,j;
  int add_namespace;
  char *itemtab;

  gmlItemObj *item=NULL;

  if(!stream || !group) return;

  /* setup the item tab */
  itemtab = (char *) malloc(sizeof(char)*strlen(tab)+3);
  if(!itemtab) return;
  sprintf(itemtab, "%s  ", tab);
    
  add_namespace = MS_TRUE;
  if(!namespace || strchr(group->name, ':') != NULL) add_namespace = MS_FALSE;

  /* start the group */
  if(add_namespace == MS_TRUE)
    msIO_fprintf(stream, "%s<%s:%s>\n", tab, namespace, group->name);
  else
    msIO_fprintf(stream, "%s<%s>\n", tab, group->name);
  
  /* now the items in the group */
  for(i=0; i<group->numitems; i++) {
    for(j=0; j<shape->numvalues; j++) {
      item = &(itemList->items[j]);
      if(strcasecmp(item->name, group->items[i]) == 0) { 
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
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "encoding", 
                      OWS_NOERR, "<?xml version=\"1.0\" encoding=\"%s\"?>\n\n", "ISO-8859-1");

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

  /* a schema *should* be required */
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, 
                           "description", OWS_NOERR, 
                           "\t<gml:description>%s</gml:description>\n", NULL);

  /* step through the layers looking for query results */
  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    if(lp->dump == MS_TRUE && lp->resultcache && lp->resultcache->numresults > 0) { /* found results */

      /* start this collection (layer) */
      /* if no layer name provided  */
      /* fall back on the layer name + "Layer" */
      value = (char*) malloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                 "layername", OWS_NOERR, "\t<%s>\n", value);
      msFree(value);

      /* actually open the layer */
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(status);

      /* retrieve all the item names */
      status = msLayerGetItems(lp);
      if(status != MS_SUCCESS) return(status);

      for(j=0; j<lp->resultcache->numresults; j++) {
	status = msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex);
        if(status != MS_SUCCESS) return(status);

#ifdef USE_PROJ
	/* project the shape into the map projection (if necessary), note that this projects the bounds as well */
        if(msProjectionsDiffer(&(lp->projection), &(map->projection)))
          msProjectShape(&lp->projection, &map->projection, &shape);
#endif

	/* start this feature */
        /* specify a feature name, if nothing provided  */
        /* fall back on the layer name + "Feature" */
        value = (char*) malloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                   "featurename", OWS_NOERR, 
                                   "\t\t<%s>\n", value);
        msFree(value);

	/* write the item/values */
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

	/* write the bounding box */
#ifdef USE_PROJ
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE)) /* use the map projection first */
	  gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE), "\t\t\t");
	else /* then use the layer projection and/or metadata */
	  gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), namespaces, MS_TRUE), "\t\t\t");	
#else
	gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), NULL, "\t\t\t"); /* no projection information */
#endif

	/* write the feature geometry */
#ifdef USE_PROJ
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE)) /* use the map projection first */
	  gmlWriteGeometry(stream,  OWS_GML2, &(shape), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), namespaces, MS_TRUE), "\t\t\t");
        else /* then use the layer projection and/or metadata */
	  gmlWriteGeometry(stream,  OWS_GML2, &(shape), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), namespaces, MS_TRUE), "\t\t\t");      
#else
	gmlWriteGeometry(stream,  OWS_GML2, &(shape), NULL, "\t\t\t");
#endif

	/* end this feature */
        /* specify a feature name if nothing provided */
        /* fall back on the layer name + "Feature" */
        value = (char*) malloc(strlen(lp->name)+9);
        sprintf(value, "%s_feature", lp->name);
        msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                   "featurename", OWS_NOERR, 
                                   "\t\t</%s>\n", value);
        msFree(value);

	msFreeShape(&shape); /* init too */
      }

      /* end this collection (layer) */
      /* if no layer name provided  */
      /* fall back on the layer name + "Layer" */
      value = (char*) malloc(strlen(lp->name)+7);
      sprintf(value, "%s_layer", lp->name);
      msOWSPrintValidateMetadata(stream, &(lp->metadata), namespaces, 
                                 "layername", OWS_NOERR, "\t</%s>\n", value);
      msFree(value);

      msLayerClose(lp);
    }
  } /* next layer */

  /* end this document */
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

int msGMLWriteWFSQuery(mapObj *map, FILE *stream, int maxfeatures, char *wfs_namespace)
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
  gmlItemObj *item=NULL;

  msInitShape(&shape);

  /* Need to start with BBOX of the whole resultset */
  if (msGetQueryResultBounds(map, &resultBounds) > 0)
    gmlWriteBounds(stream, OWS_GML2, &resultBounds, msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), "      ");

  /* step through the layers looking for query results */
  for(i=0; i<map->numlayers; i++) {

    lp = &(map->layers[i]);

    if(lp->dump == MS_TRUE && lp->resultcache && lp->resultcache->numresults > 0)  { /* found results */
      const char *geom_name;
      char *layer_name;
      geom_name = msWFSGetGeomElementName(map, lp);

      /* actually open the layer */
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(status);

      /* retrieve all the item names. (Note : there might be no attributes) */
      status = msLayerGetItems(lp);
      /* if(status != MS_SUCCESS) return(status); */

      /* populate item and group metadata structures */
      itemList = msGMLGetItems(lp);
      groupList = msGMLGetGroups(lp);

      for(j=0; j<lp->resultcache->numresults; j++) {
	status = msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex);
        if(status != MS_SUCCESS) return(status);

#ifdef USE_PROJ
	/* project the shape into the map projection (if necessary), note that this projects the bounds as well */
        if(msProjectionsDiffer(&(lp->projection), &(map->projection)))
          msProjectShape(&lp->projection, &map->projection, &shape);
#endif
        
	/* start this feature */
        if (wfs_namespace) {
	  layer_name = (char *) malloc(strlen(wfs_namespace)+strlen(lp->name)+2);
	  sprintf(layer_name, "%s:%s", wfs_namespace, lp->name);
        } else
	  layer_name = strdup(lp->name);


	msIO_fprintf(stream, "    <gml:featureMember>\n");
        if(msIsXMLTagValid(layer_name) == MS_FALSE)
            msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid in a XML tag context. -->\n", layer_name);
        msIO_fprintf(stream, "      <%s>\n", layer_name);


	/* write the bounding box */
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE)) // use the map projection first

#ifdef USE_PROJ
	  gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), "        ");
	else /* then use the layer projection and/or metadata */
	  gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), "        ");
#else
	  gmlWriteBounds(stream, OWS_GML2, &(shape.bounds), NULL, "        "); /* no projection information */
#endif
        
        msIO_fprintf(stream, "        <gml:%s>\n", geom_name); 
                    
	/* write the feature geometry */
#ifdef USE_PROJ
	if(msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE)) /* use the map projection first */
	  gmlWriteGeometry(stream, OWS_GML2, &(shape), msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FGO", MS_TRUE), "          ");
        else /* then use the layer projection and/or metadata */
	  gmlWriteGeometry(stream, OWS_GML2, &(shape), msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FGO", MS_TRUE), "          ");      
#else
	gmlWriteGeometry(stream, OWS_GML2, &(shape), NULL, "          ");
#endif

        msIO_fprintf(stream, "        </gml:%s>\n", geom_name); 

	/* write the item/values */
	for(k=0; k<lp->numitems; k++) {
          item = &(itemList->items[k]);  
          if(msItemInGroups(item, groupList) == MS_FALSE) 
	    msGMLWriteItem(stream, item, shape.values[k], wfs_namespace, "        ");
        }

	for(k=0; k<groupList->numgroups; k++)
	  msGMLWriteGroup(stream, &(groupList->groups[k]), &shape, itemList, wfs_namespace, "        ");

	/* end this feature */
        msIO_fprintf(stream, "      </%s>\n", layer_name);

	msIO_fprintf(stream, "    </gml:featureMember>\n");

        msFree(layer_name);

	msFreeShape(&shape); /* init too */

         features++;
         if (maxfeatures > 0 && features == maxfeatures)
           break;
         
         /* end this layer */
      }

      msGMLFreeGroups(groupList);
      msGMLFreeItems(itemList);

      msLayerClose(lp);
    }

    if (maxfeatures > 0 && features == maxfeatures)
      break;

  } /* next layer */

  return(MS_SUCCESS);

#else /* Stub for mapscript */
    msSetError(MS_MISCERR, "WMS server support not enabled",
               "msGMLWriteWFSQuery()");
    return MS_FAILURE;
#endif /* USE_WFS_SVR */
}

