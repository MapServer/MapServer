/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of utility functions related to the PROJ4 library
 * Author:   DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2001,  Y. Assefa, DM Solutions Group Inc
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ****************************************************************************/

#include <string.h>
#include "mapserver.h"

/************************************************************************/
/*                        ConvertProjUnitStringToMS                     */
/*                                                                      */
/*       Returns mapserver's unit code corresponding to the proj        */
/*      unit passed as argument.                                        */
/*       Please refer to ./src/pj_units.c file in the Proj.4 module.    */
/************************************************************************/
#ifdef USE_PROJ
static int ConvertProjUnitStringToMS(const char *pszProjUnit)
{
  if (strcmp(pszProjUnit, "m") ==0) {
    return MS_METERS;
  } else if (strcmp(pszProjUnit, "km") ==0) {
    return MS_KILOMETERS;
  } else if (strcmp(pszProjUnit, "mi") ==0 || strcmp(pszProjUnit, "us-mi") ==0) {
    return MS_MILES;
  } else if (strcmp(pszProjUnit, "in") ==0 || strcmp(pszProjUnit, "us-in") ==0 ) {
    return MS_INCHES;
  } else if (strcmp(pszProjUnit, "ft") ==0 || strcmp(pszProjUnit, "us-ft") ==0) {
    return MS_FEET;
  } else if (strcmp(pszProjUnit, "kmi") == 0) {
    return MS_NAUTICALMILES;
  }

  return -1;
}
#endif /* def USE_PROJ */

/************************************************************************/
/*           int GetMapserverUnitUsingProj(projectionObj *psProj)       */
/*                                                                      */
/*      Return a mapserver unit corresponding to the projection         */
/*      passed. Retunr -1 on failure                                    */
/************************************************************************/
int GetMapserverUnitUsingProj(projectionObj *psProj)
{
#ifdef USE_PROJ
  char *proj_str;

  if( pj_is_latlong( psProj->proj ) )
    return MS_DD;

  proj_str = pj_get_def( psProj->proj, 0 );

  /* -------------------------------------------------------------------- */
  /*      Handle case of named units.                                     */
  /* -------------------------------------------------------------------- */
  if( strstr(proj_str,"units=") != NULL ) {
    char units[32];
    char *blank;

    strlcpy( units, (strstr(proj_str,"units=")+6), sizeof(units) );
    pj_dalloc( proj_str );

    blank = strchr(units, ' ');
    if( blank != NULL )
      *blank = '\0';

    return ConvertProjUnitStringToMS( units );
  }

  /* -------------------------------------------------------------------- */
  /*      Handle case of to_meter value.                                  */
  /* -------------------------------------------------------------------- */
  if( strstr(proj_str,"to_meter=") != NULL ) {
    char to_meter_str[32];
    char *blank;
    double to_meter;

    strlcpy(to_meter_str,(strstr(proj_str,"to_meter=")+9),
            sizeof(to_meter_str));
    pj_dalloc( proj_str );

    blank = strchr(to_meter_str, ' ');
    if( blank != NULL )
      *blank = '\0';

    to_meter = atof(to_meter_str);

    if( fabs(to_meter-1.0) < 0.0000001 )
      return MS_METERS;
    else if( fabs(to_meter-1000.0) < 0.00001 )
      return MS_KILOMETERS;
    else if( fabs(to_meter-0.3048) < 0.0001 )
      return MS_FEET;
    else if( fabs(to_meter-0.0254) < 0.0001 )
      return MS_INCHES;
    else if( fabs(to_meter-1609.344) < 0.001 )
      return MS_MILES;
    else if( fabs(to_meter-1852.0) < 0.1 )
      return MS_NAUTICALMILES;
    else
      return -1;
  }

  pj_dalloc( proj_str );
#endif
  return -1;
}




