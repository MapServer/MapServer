#include "map.h"
#include "mapproject.h"

#ifdef USE_PROJ
static int msTestNeedWrap( pointObj pt1, pointObj pt2, pointObj pt2_geo,
                           projectionObj *src_proj, 
                           projectionObj *dst_proj );
#endif

int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point)
{
#ifdef USE_PROJ
  projUV p;
  int	 error;

  if( in && in->proj && out && out->proj )
  {
      double	z = 0.0;

      if( pj_is_latlong(in->proj) )
      {
          point->x *= DEG_TO_RAD;
          point->y *= DEG_TO_RAD;
      }

      error = pj_transform( in->proj, out->proj, 1, 0, 
                            &(point->x), &(point->y), &z );

      if( error )
          return MS_FAILURE;

      if( pj_is_latlong(out->proj) )
      {
          point->x *= RAD_TO_DEG;
          point->y *= RAD_TO_DEG;
      }
  }
  else
  {
      p.u = point->x;
      p.v = point->y;

      if(in==NULL || in->proj==NULL) { /* input coordinates are lat/lon */
          p.u *= DEG_TO_RAD; /* convert to radians */
          p.v *= DEG_TO_RAD;  
          p = pj_fwd(p, out->proj);
      } else {
          if(out==NULL || out->proj==NULL) { /* output coordinates are lat/lon */
              p = pj_inv(p, in->proj);
              p.u *= RAD_TO_DEG; /* convert to decimal degrees */
              p.v *= RAD_TO_DEG;
          } else { /* need to go from one projection to another */
              p = pj_inv(p, in->proj);
              p = pj_fwd(p, out->proj);
          }
      }

      point->x = p.u;
      point->y = p.v;
  }
  return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectPoint()");
  return(MS_FAILURE);
#endif
}

#ifdef USE_PROJ
static void msProjectGrowRect(projectionObj *in, projectionObj *out, 
                              rectObj *prj_rect, int *rect_initialized, 
                              pointObj *prj_point, int *failure )

{
    if( msProjectPoint(in, out, prj_point) == MS_SUCCESS )
    {
        if( *rect_initialized )
        {
            prj_rect->miny = MS_MIN(prj_rect->miny, prj_point->y);
            prj_rect->maxy = MS_MAX(prj_rect->maxy, prj_point->y);
            prj_rect->minx = MS_MIN(prj_rect->minx, prj_point->x);
            prj_rect->maxx = MS_MAX(prj_rect->maxx, prj_point->x);
        }
        else
        {
            prj_rect->minx = prj_rect->maxx = prj_point->x;
            prj_rect->miny = prj_rect->maxy = prj_point->y;
            *rect_initialized = MS_TRUE;
        }
    }
    else
        (*failure)++;
}
#endif /* def USE_PROJ */

#define NUMBER_OF_SAMPLE_POINTS 100

int msProjectRect(projectionObj *in, projectionObj *out, rectObj *rect) 
{
#ifdef USE_PROJ
  pointObj prj_point;
  rectObj prj_rect;
  int	  rect_initialized = MS_FALSE, failure=0;

  double dx, dy;
  double x, y;

  dx = (rect->maxx - rect->minx)/NUMBER_OF_SAMPLE_POINTS;
  dy = (rect->maxy - rect->miny)/NUMBER_OF_SAMPLE_POINTS;

  if(dx > 0) {
    for(x=rect->minx; x<=rect->maxx; x+=dx) {
      prj_point.x = x;
      prj_point.y = rect->miny;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);
      
      prj_point.x = x;
      prj_point.y = rect->maxy;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);
    }
  }

  if(dy > 0) {
    for(y=rect->miny; y<=rect->maxy; y+=dy) {
      prj_point.y = y;
      prj_point.x = rect->minx;    
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);
      
      prj_point.x = rect->maxx;
      prj_point.y = y;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);
    }
  }

  /*
  ** If there have been any failures around the edges, then we had better
  ** try and fill in the interior to get a close bounds. 
  */
  if( failure > 0 )
  {
      msDebug( "msProjectRect(): some points failed to reproject, doing internal sampling.\n" );
      for(x=rect->minx + dx; x<=rect->maxx; x+=dx) {
          for(y=rect->miny + dy; y<=rect->maxy; y+=dy) {
              prj_point.x = x;
              prj_point.y = y;
              msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                                &failure);
          }
      }
  }

  rect->minx = prj_rect.minx;
  rect->miny = prj_rect.miny;
  rect->maxx = prj_rect.maxx;
  rect->maxy = prj_rect.maxy;

  if( !rect_initialized )
      return MS_FAILURE;
  else
      return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectRect()");
  return(MS_FAILURE);
#endif
}

int msProjectShape(projectionObj *in, projectionObj *out, shapeObj *shape)
{
#ifdef USE_PROJ
  int i;

  for(i=0; i<shape->numlines; i++)
      msProjectLine(in, out, shape->line+i );

  return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectShape()");
  return(MS_FAILURE);
#endif
}

int msProjectLine(projectionObj *in, projectionObj *out, lineObj *line)
{
#ifdef USE_PROJ
  int i, be_careful = 1;

  if( be_careful )
      be_careful = out->proj != NULL && pj_is_latlong(out->proj)
          && !pj_is_latlong(in->proj);

  if( be_careful )
  {
      pointObj	startPoint, thisPoint; /* locations in projected space */

      startPoint = line->point[0];

      for(i=0; i<line->numpoints; i++)
      {
          double	dist;

          thisPoint = line->point[i];

          /* 
          ** Read comments before msTestNeedWrap() to better understand
          ** this dateline wrapping logic. 
          */
          msProjectPoint(in, out, &(line->point[i]));
          if( i > 0 )
          {
              dist = line->point[i].x - line->point[0].x;
              if( fabs(dist) > 180.0 )
              {
                  if( msTestNeedWrap( thisPoint, startPoint, 
                                      line->point[0], in, out ) )
                  {
                      if( dist > 0.0 )
                      {
                          line->point[i].x -= 360.0;
                      }
                      else if( dist < 0.0 )
                      {
                          line->point[i].x += 360.0;
                      }
                  }
              }

          }
      }
  }
  else
  {
      for(i=0; i<line->numpoints; i++)
          msProjectPoint(in, out, &(line->point[i]));
  }

  return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectLine()");
  return(MS_FAILURE);
#endif
}

/*
** Compare two projections, and return MS_TRUE if they differ. 
**
** For now this is implemented by exact comparison of the projection
** arguments, but eventually this should call a PROJ.4 function with
** more awareness of the issues.
**
** NOTE: MS_FALSE is returned if either of the projection objects
** has no arguments, since reprojection won't work anyways.
*/

int msProjectionsDiffer( projectionObj *proj1, projectionObj *proj2 )

{
    int		i;

    if( proj1->numargs == 0 || proj2->numargs == 0 )
        return MS_FALSE;

    if( proj1->numargs != proj2->numargs )
        return MS_TRUE;

    for( i = 0; i < proj1->numargs; i++ )
    {
        if( strcmp(proj1->args[i],proj2->args[i]) != 0 )
            return MS_TRUE;
    }

    return MS_FALSE;
}

/*

Frank Warmerdam, Nov, 2001. 

See Also: 

http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=15

Proposal:

Modify msProjectLine() so that it "dateline wraps" objects when necessary
in order to preserve their shape when reprojecting to lat/long.  This
will be accomplished by:

1) As each vertex is reprojected, compare the X distance between that 
   vertex and the previous vertex.  If it is less than 180 then proceed to
   the next vertex without any special logic, otherwise:

2) Reproject the center point of the line segment from the last vertex to
   the current vertex into lat/long.  Does it's longitude lie between the
   longitudes of the start and end point.  If yes, return to step 1) for
   the next vertex ... everything is fine. 

3) We have determined that this line segment is suffering from 360 degree
   wrap to keep in the -180 to +180 range.  Now add or subtract 360 degrees
   as determined from the original sign of the distances.  

This is similar to the code there now (though disabled in CVS); however, 
it will ensure that big boxes will remain big, and not get dateline wrapped
because of the extra test in step 2).  However step 2 is invoked only very
rarely so this process takes little more than the normal process.  In fact, 
if we were sufficiently concerned about performance we could do a test on the
shape MBR in lat/long space, and if the width is less than 180 we know we never
need to even do test 1). 

What doesn't this resolve:

This ensures that individual lines are kept in the proper shape when 
reprojected to geographic space.  However, it does not:

 o Ensure that all rings of a polygon will get transformed to the same "side"
   of the world.  Depending on starting points of the different rings it is
   entirely possible for one ring to end up in the -180 area and another ring
   from the same polygon to end up in the +180 area.  We might possibly be
   able to achieve this though, by treating the multi-ring polygon as a whole
   and testing the first point of each additional ring against the last
   vertex of the previous ring (or any previous vertex for that matter).

 o It does not address the need to cut up lines and polygons into distinct
   chunks to preserve the correct semantics.  Really a polygon that 
   spaces the dateline in a -180 to 180 view should get split into two 
   polygons.  We haven't addressed that, though if it were to be addressed,
   it could be done as a followon and distinct step from what we are doing
   above.  In fact this sort of improvement (split polygons based on dateline
   or view window) should be done for all lat/long shapes regardless of 
   whether they are being reprojected from another projection. 

 o It does not address issues related to viewing rectangles that go outside
   the -180 to 180 longitude range.  For instance, it is entirely reasonable
   to want a 160 to 200 longitude view to see an area on the dateline clearly.
   Currently shapes in the -180 to -160 range which should be displayed in the
   180 to 200 portion of that view will not be because there is no recogition
   that they belong there. 


*/

#ifdef USE_PROJ
static int msTestNeedWrap( pointObj pt1, pointObj pt2, pointObj pt2_geo,
                           projectionObj *in, 
                           projectionObj *out )

{
    pointObj	middle;

    middle.x = (pt1.x + pt2.x) * 0.5;
    middle.y = (pt1.y + pt2.y) * 0.5;
    
    msProjectPoint( in, out, &pt1 );
    msProjectPoint( in, out, &pt2 );
    msProjectPoint( in, out, &middle );

    /* 
     * If the last point was moved, then we are considered due for a
     * move to.
     */
    if( fabs(pt2_geo.x-pt2.x) > 180.0 )
        return 1;

    /*
     * Otherwise, test to see if the middle point transforms
     * to be between the end points. If yes, no wrapping is needed.
     * Otherwise wrapping is needed.
     */
    if( (middle.x < pt1.x && middle.x < pt2_geo.x)
        || (middle.x > pt1.x && middle.x > pt2_geo.x) )
        return 1;
    else
        return 0;
}
#endif /* def USE_PROJ */

/*
** msGetEPSGProj()
**
** Extract projection code for this layer/map.  
** First look for a wms_srs metadata, then wfs_srs and gml_srs.  If not 
** found then look for an EPSG code in projectionObj, and if not found then 
** return NULL.
**
** If bReturnOnlyFirstOne=TRUE and metadata contains multiple EPSG codes
** then only the first one (which is assumed to be the layer's default
** projection) is returned.
*/
const char *msGetEPSGProj(projectionObj *proj, hashTableObj metadata, int bReturnOnlyFirstOne)
{
#ifdef USE_PROJ
  static char epsgCode[20] ="";
  static char *value;

  if (metadata &&
      ((value = msLookupHashTable(metadata, "wms_srs")) != NULL ||
       (value = msLookupHashTable(metadata, "wfs_srs")) != NULL ||
       (value = msLookupHashTable(metadata, "gml_srs")) != NULL ) )
  {
    // Metadata value should already be in format "EPSG:n" or "AUTO:..."
    if (!bReturnOnlyFirstOne)
        return value;

    // Caller requested only first projection code.
    strncpy(epsgCode, value, 19);
    epsgCode[19] = '\0';
    if ((value=strchr(epsgCode, ' ')) != NULL)
        *value = '\0';
    return epsgCode;
  }
  else if (proj && proj->numargs > 0 && 
           (value = strstr(proj->args[0], "init=epsg:")) != NULL &&
           strlen(value) < 20)
  {
    sprintf(epsgCode, "EPSG:%s", value+10);
    return epsgCode;
  }
  else if (proj && proj->numargs > 0 && 
           strncasecmp(proj->args[0], "AUTO:", 5) == 0 )
  {
    return proj->args[0];
  }

  return NULL;
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msGetEPSGProj()");
  return(MS_FAILURE);
#endif
}
