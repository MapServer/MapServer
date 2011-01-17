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

enum MS_UNITS {MS_INCHES, MS_FEET, MS_MILES, MS_METERS, MS_KILOMETERS,
               MS_DD, MS_PIXELS, MS_NAUTICALMILES};


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
    else if (strcmp(pszProjUnit, "kmi") == 0)
    {
        return MS_NAUTICALMILES;
    }
    
    return -1;              
}
#endif /* def USE_PROJ */

/************************************************************************/
/*       pj_units[] copy.  It is safer for win32 builds to include a    */
/*      copy of pj_units instead of trying to get the variable from     */
/*      the proj dll.                                                   */
/************************************************************************/
#ifdef USE_PROJ 
struct PJ_UNITS
pj_units_copy[] = {
       {"km",  "1000.",        "Kilometer"},
       {"m",   "1.",           "Meter"},
       {"dm",  "1/10",         "Decimeter"},
       {"cm",  "1/100",        "Centimeter"},
       {"mm",  "1/1000",       "Millimeter"},
       {"kmi", "1852.0",       "International Nautical Mile"},
       {"in",  "0.0254",       "International Inch"},
       {"ft",  "0.3048",       "International Foot"},
       {"yd",  "0.9144",       "International Yard"},
       {"mi",  "1609.344",     "International Statute Mile"},
       {"fath",        "1.8288",       "International Fathom"},
       {"ch",  "20.1168",      "International Chain"},
       {"link",        "0.201168",     "International Link"},
       {"us-in",       "1./39.37",     "U.S. Surveyor's Inch"},
       {"us-ft",       "0.304800609601219",    "U.S. Surveyor's Foot"},
       {"us-yd",       "0.914401828803658",    "U.S. Surveyor's Yard"},
       {"us-ch",       "20.11684023368047",    "U.S. Surveyor's Chain"},
       {"us-mi",       "1609.347218694437",    "U.S. Surveyor's Statute Mile"},
       {"ind-yd",      "0.91439523",   "Indian Yard"},
       {"ind-ft",      "0.30479841",   "Indian Foot"},
       {"ind-ch",      "20.11669506",  "Indian Chain"},
       {(char *)0, (char *)0, (char *)0}
};
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
    struct PJ_UNITS *lu;
    if (psProj && psProj->proj)
    {
        if (psProj->proj->is_latlong)
            return MS_DD;

        /* psProj->proj->to_meter; */
        for (lu = pj_units_copy; lu->id ; ++lu)
        {
            if (strtod(lu->to_meter, NULL) == psProj->proj->to_meter)
                return ConvertProjUnitStringToMS(lu->id);
        }
    }
#endif
    return -1;
}




