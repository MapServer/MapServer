#include "map.h"
#include "mapproject.h"

int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point)
{
#ifdef USE_PROJ
  projUV p;

  if( in && in->proj && out && out->proj )
  {
      if( pj_is_latlong(in->proj) )
      {
          point->x *= DEG_TO_RAD;
          point->y *= DEG_TO_RAD;
      }

      pj_transform( in->proj, out->proj, 1, 0, &(point->x), &(point->y), NULL );

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

#define NUMBER_OF_SAMPLE_POINTS 100

int msProjectRect(projectionObj *in, projectionObj *out, rectObj *rect) 
{
#ifdef USE_PROJ
  pointObj prj_point;
  rectObj prj_rect;

  double dx, dy;
  double x, y;

  dx = (rect->maxx - rect->minx)/NUMBER_OF_SAMPLE_POINTS;
  dy = (rect->maxy - rect->miny)/NUMBER_OF_SAMPLE_POINTS;

  prj_point.x = rect->minx;
  prj_point.y = rect->miny;
  msProjectPoint(in, out, &prj_point);
  prj_rect.minx = prj_rect.maxx = prj_point.x;
  prj_rect.miny = prj_rect.maxy = prj_point.y;

  if(dx > 0) {
    for(x=rect->minx+dx; x<=rect->maxx; x+=dx) {
      prj_point.x = x;
      prj_point.y = rect->miny;
      msProjectPoint(in, out, &prj_point);
      prj_rect.miny = MS_MIN(prj_rect.miny, prj_point.y);
      prj_rect.maxy = MS_MAX(prj_rect.maxy, prj_point.y);
      prj_rect.minx = MS_MIN(prj_rect.minx, prj_point.x);
      prj_rect.maxx = MS_MAX(prj_rect.maxx, prj_point.x);
      
      prj_point.x = x;
      prj_point.y = rect->maxy;
      msProjectPoint(in, out, &prj_point);
      prj_rect.miny = MS_MIN(prj_rect.miny, prj_point.y);
      prj_rect.maxy = MS_MAX(prj_rect.maxy, prj_point.y);
      prj_rect.minx = MS_MIN(prj_rect.minx, prj_point.x);
      prj_rect.maxx = MS_MAX(prj_rect.maxx, prj_point.x); 
    }
  }

  if(dy > 0) {
    for(y=rect->miny+dy; y<=rect->maxy; y+=dy) {
      prj_point.y = y;
      prj_point.x = rect->minx;    
      msProjectPoint(in, out, &prj_point);
      prj_rect.minx = MS_MIN(prj_rect.minx, prj_point.x);
      prj_rect.maxx = MS_MAX(prj_rect.maxx, prj_point.x);
      prj_rect.miny = MS_MIN(prj_rect.miny, prj_point.y);
      prj_rect.maxy = MS_MAX(prj_rect.maxy, prj_point.y);
      
      prj_point.x = rect->maxx;
      prj_point.y = y;
      msProjectPoint(in, out, &prj_point);
      prj_rect.minx = MS_MIN(prj_rect.minx, prj_point.x);
      prj_rect.maxx = MS_MAX(prj_rect.maxx, prj_point.x);
      prj_rect.miny = MS_MIN(prj_rect.miny, prj_point.y);
      prj_rect.maxy = MS_MAX(prj_rect.maxy, prj_point.y);
    }
  }

  rect->minx = prj_rect.minx;
  rect->miny = prj_rect.miny;
  rect->maxx = prj_rect.maxx;
  rect->maxy = prj_rect.maxy;

  return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectRect()");
  return(MS_FAILURE);
#endif
}

int msProjectShape(projectionObj *in, projectionObj *out, shapeObj *shape)
{
#ifdef USE_PROJ
  int i,j;

  for(i=0; i<shape->numlines; i++)
    for(j=0; j<shape->line[i].numpoints; j++)
      msProjectPoint(in, out, &(shape->line[i].point[j]));

  return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectShape()");
  return(MS_FAILURE);
#endif
}

int msProjectLine(projectionObj *in, projectionObj *out, lineObj *line)
{
#ifdef USE_PROJ
  int i;
  
  for(i=0; i<line->numpoints; i++)
    msProjectPoint(in, out, &(line->point[i]));

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
