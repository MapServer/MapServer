#ifdef USE_GEOS

#include <string>
#include <iostream>
#include <fstream>

#include "geos.h"
#include "map.h"

using namespace geos;

extern "C" void msGEOSInit(int maxalign);
extern "C" void msGEOSDeleteGeometry(Geometry *g);

extern "C" Geometry *msGEOSShape2Geometry(shapeObj *shape);
extern "C" shapeObj *msGEOSGeometry2Shape(Geometry *g);

extern "C" shapeObj *msGEOSBuffer(shapeObj *shape, double width);

GeometryFactory *geomFactory = NULL;

void msGEOSInit()
{
  if (geomFactory == NULL)
    {
      geomFactory = new GeometryFactory( new PrecisionModel(), -1); // NOTE: SRID will have to be changed after geometry creation
    }
}

void msGEOSDeleteGeometry(Geometry *g)
{
  try{
    delete g;
  }
  catch (GEOSException *ge)
    {
      msSetError(MS_GEOSERR, "%s", "msGEOSDeleteGeometry()", (char *) ge->toString().c_str());
      delete ge;
    }

  catch(...)
    {
      // do nothing!
    }
}

/*
** Translation functions
*/

Geometry *msGEOSShape2Geometry(shapeObj *shape)
{
  return NULL;
}

shapeObj *msGEOSGeometry2Shape(Geometry *g)
{
  return NULL;
}

/*
** Analytical functions
*/

shapeObj *msGEOSBuffer(shapeObj *shape, double width)
{
  Geometry *g = (Geometry *) shape->geometry;

  try
    {
      Geometry *bg = g->buffer(width);
      return msGEOSGeometry2Shape(bg);
    }
  catch (GEOSException *ge)
    {     
      msSetError(MS_GEOSERR, "%s", "msGEOSBuffer()", (char *) ge->toString().c_str());
      delete ge;
      return NULL;
    }
  catch (...)
    {
      return NULL;
    }
}
#endif
