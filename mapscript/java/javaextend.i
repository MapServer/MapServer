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

