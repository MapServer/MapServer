#include "map.h"
#include "mapproject.h"

#ifdef USE_PROJ

int msCompareProjections(PJ *pj1, PJ *pj2) 
{

}

void msProjectPoint(PJ *in, PJ *out, pointObj *point)
{
  projUV p;

  p.u = point->x;
  p.v = point->y;

  if(in==NULL) { /* input coordinates are lat/lon */
    p.u *= DEG_TO_RAD; /* convert to radians */
    p.v *= DEG_TO_RAD;  
    p = pj_fwd(p, out);
  } else {
    if(out==NULL) { /* output coordinates are lat/lon */
      p = pj_inv(p, in);
      p.u *= RAD_TO_DEG; /* convert to decimal degrees */
      p.v *= RAD_TO_DEG;
    } else { /* need to go from one projection to another */
      p = pj_inv(p, in);
      p = pj_fwd(p, out);
    }
  }

  point->x = p.u;
  point->y = p.v;
  return;
}

void msProjectRect(PJ *in, PJ *out, rectObj *rect)
{
  pointObj prj_point;
  rectObj prj_rect;

  prj_point.x = rect->minx;
  prj_point.y = rect->miny;
  msProjectPoint(in, out, &prj_point);
  prj_rect.minx = prj_point.x;
  prj_rect.miny = prj_point.y;

  prj_point.x = rect->maxx;
  prj_point.y = rect->miny;
  msProjectPoint(in, out, &prj_point);
  prj_rect.miny = MS_MIN(prj_rect.miny, prj_point.y);
  prj_rect.maxx = prj_point.x;

  prj_point.x = rect->maxx;
  prj_point.y = rect->maxy;
  msProjectPoint(in, out, &prj_point);
  prj_rect.maxx = MS_MAX(prj_rect.maxx, prj_point.x);
  prj_rect.maxy = prj_point.y;

  prj_point.x = rect->minx;
  prj_point.y = rect->maxy;
  msProjectPoint(in, out, &prj_point);
  prj_rect.minx = MS_MIN(prj_rect.minx, prj_point.x);
  prj_rect.maxy = MS_MAX(prj_rect.maxy, prj_point.y);

  rect->minx = prj_rect.minx;
  rect->miny = prj_rect.miny;
  rect->maxx = prj_rect.maxx;
  rect->maxy = prj_rect.maxy;

  return;
}

void msProjectPolyline(PJ *in, PJ *out, shapeObj *poly)
{
  int i,j;

  for(i=0; i<poly->numlines; i++)
    for(j=0; j<poly->line[i].numpoints; j++)
      msProjectPoint(in, out, &(poly->line[i].point[j]));
}

void msProjectLine(PJ *in, PJ *out, lineObj *line)
{
  int i;
  
  for(i=0; i<line->numpoints; i++)
    msProjectPoint(in, out, &(line->point[i]));
}
#endif
