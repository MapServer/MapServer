/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of utility functions related to the PROJ4 library
 * Author:   DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2001, DM Solutions Group Inc
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.3  2001/12/10 22:31:27  sdlime
 * Added include for string.h to mapprojhack.c to avoid a compiler warning.
 *
 * Revision 1.2  2001/10/17 14:13:49  dan
 * Hopefully fixed conflict between projects.h and proj_api.h includes.
 *
 * Revision 1.1  2001/10/17 13:12:35  assefa
 * Add functions to set the unis of the map when the projection
 * is changed.
 *
 *
 **********************************************************************/

/* ==================================================================== 
      This file includes only mapproject.h (which includes projects.h)
      and none of the windows include files. This is because of a conflict
      in names (PJ_VALUE) between the projects.h and windows include file
      winreg.h.                
 ==================================================================== */

/* Make sure we include projects.h before mapproject.h, because proj_api.h
 * may end up being included instead by mapproject.h ifdef USE_PROJ_API_H
 */
#ifdef USE_PROJ
#include <projects.h>
#endif

#include <string.h>
#include "mapproject.h"

enum MS_UNITS {MS_INCHES, MS_FEET, MS_MILES, MS_METERS, MS_KILOMETERS, MS_DD, 
               MS_PIXELS};


/************************************************************************/
/*                        ConvertProjUnitStringToMS                     */
/*                                                                      */
/*       Returns mapserver's unit code corresponding to the proj        */
/*      unit passed as argument.                                        */
/*       Please refer to ./src/pj_units.c file in the Proj.4 module.    */
/************************************************************************/
static int ConvertProjUnitStringToMS(const char *pszProjUnit)
{
    if (strcmp(pszProjUnit, "m") ==0)
    {
        return MS_METERS;
    }
    else if (strcmp(pszProjUnit, "km") ==0)
    {
        return MS_KILOMETERS;
    }
    else if (strcmp(pszProjUnit, "mi") ==0 || strcmp(pszProjUnit, "us-mi") ==0)
    {
        return MS_FEET;
    }
    else if (strcmp(pszProjUnit, "in") ==0 || strcmp(pszProjUnit, "us-in") ==0 )
    {
        return MS_INCHES;
    }
    else if (strcmp(pszProjUnit, "ft") ==0 || strcmp(pszProjUnit, "us-ft") ==0)
    {
        return MS_FEET;
    }
    
    return -1;              
}

/************************************************************************/
/*           int GetMapserverUnitUsingProj(projectionObj *psProj)       */
/*                                                                      */
/*      Return a mapserver unit corresponding to the projection         */
/*      passed. Retunr -1 on failure                                    */
/************************************************************************/
int GetMapserverUnitUsingProj(projectionObj *psProj)
{
#ifdef USE_PROJ
    struct PJ_UNITS *lu;
    if (psProj && psProj->proj)
    {
        if (psProj->proj->is_latlong)
            return MS_DD;

        //psProj->proj->to_meter;
        for (lu = pj_units; lu->id ; ++lu)
        {
            if (strtod(lu->to_meter, NULL) == psProj->proj->to_meter)
                return ConvertProjUnitStringToMS(lu->id);
        }
    }
#endif
    return -1;
}




