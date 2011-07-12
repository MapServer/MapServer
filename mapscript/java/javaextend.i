/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Java-specific extensions to MapScript objects
 * Author:   Sean Gillies, sgillies@frii.com
 *           Jerry Pisk, jerry.pisk@gmail.com
 *
 *****************************************************************************/

/*
===============================================================================
imageObj
===============================================================================
*/

/* getBytes moved to mapscript/swiginc/image.i */

/*
==============================================================================
 pointObj
==============================================================================
*/

%extend pointObj 
{
  
    pointObj(double x, double y, double z, double m) 
    {
        pointObj *p;
        p = (pointObj *)calloc(1,sizeof(pointObj));
        if (!p) return NULL;
        p->x = x;
        p->y = y;
#ifdef USE_POINT_Z_M
	    p->z = z;
        p->m = m;
#endif
        return p;
    }

    pointObj(double x, double y, double z)
    {
        pointObj *p;
        p = (pointObj *)calloc(1,sizeof(pointObj));
        if (!p) return NULL;
        p->x = x;
        p->y = y;
#ifdef USE_POINT_Z_M
        p->z = z;
        p->m = -2e38;
#endif
        return p;
    }

}

