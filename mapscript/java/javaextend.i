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

%extend imageObj
{

    gdBuffer getBytes() 
    {
        gdBuffer buffer;
        
        buffer.data = msSaveImageBufferGD(self->img.gd, &buffer.size,
                                          self->format);
        if( buffer.size == 0 )
        {
            buffer.data = NULL;
            msSetError(MS_MISCERR, "Failed to get image buffer", "getBytes");
            return buffer;
        }

        return buffer;
    }
    
}

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
	    p->z = z;
        p->m = m;
        return p;
    }

    pointObj(double x, double y, double z)
    {
        pointObj *p;
        p = (pointObj *)calloc(1,sizeof(pointObj));
        if (!p) return NULL;
        p->x = x;
        p->y = y;
        p->z = z;
        p->m = -2e38;
        return p;
    }

}
