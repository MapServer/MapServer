/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  MapServer-GEOS integration.
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
 * Revision 1.2  2004/10/28 02:23:35  frank
 * added standard header
 *
 */

#ifdef USE_GEOS

#include <string>
#include <iostream>
#include <fstream>

#include "geos.h"
#include "map.h"

MS_CVSID("$Id$")

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
