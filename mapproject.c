#include "map.h"
#include "mapproject.h"

int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point)
{
#ifdef USE_PROJ
  projUV p;
  int	 error;

  if( in && in->proj && out && out->proj )
  {
      if( pj_is_latlong(in->proj) )
      {
          point->x *= DEG_TO_RAD;
          point->y *= DEG_TO_RAD;
      }

      error = pj_transform( in->proj, out->proj, 1, 0, 
                            &(point->x), &(point->y), NULL );

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
  int i,j;

#ifdef notdef
  for(i=0; i<shape->numlines; i++)
      msProjectLine(in, out, shape->line+i );
#endif
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
  int i, be_careful = 0;

  if( be_careful )
      be_careful = pj_is_latlong(out->proj);

  if( be_careful )
  {
      for(i=0; i<line->numpoints; i++)
      {
          double	dist;

          msProjectPoint(in, out, &(line->point[i]));
          if( i > 0 )
          {
              dist = line->point[i].x - line->point[i-1].x;
              if( dist > 180 )
              {
                  line->point[i].x -= 360.0;
              }
              else if( dist < -180 )
              {
                  line->point[i].x += 360.0;
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
