/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript colorObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 1996-2001 Regents of the University of Minnesota.
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================
*/

%{
#include "../../mapserver.h"
%}

%extend colorObj 
{
  
    colorObj(int red=0, int green=0, int blue=0, int pen=MS_PEN_UNSET) 
    {
        colorObj *color;
        
        /* Check colors */
        if (red > 255 || green > 255 || blue > 255) {
            msSetError(MS_MISCERR, "Invalid color index.", "colorObj()");
            return NULL;
        }
    
        color = (colorObj *)calloc(1, sizeof(colorObj));
        if (!color)
            return(NULL);
    
        MS_INIT_COLOR(*color, red, green, blue);

        return(color);    	
    }

    ~colorObj() 
    {
        free(self);
    }
 
    int setRGB(int red, int green, int blue) 
    {
        /* Check colors */
        if (red > 255 || green > 255 || blue > 255) {
            msSetError(MS_MISCERR, "Invalid color index.", "setRGB()");
            return MS_FAILURE;
        }
    
        MS_INIT_COLOR(*self, red, green, blue);
        return MS_SUCCESS;
    }

    int setHex(char *psHexColor) 
    {
        int red, green, blue;
        if (psHexColor && strlen(psHexColor)== 7 && psHexColor[0] == '#') {
            red = msHexToInt(psHexColor+1);
            green = msHexToInt(psHexColor+3);
            blue= msHexToInt(psHexColor+5);
            if (red > 255 || green > 255 || blue > 255) {
                msSetError(MS_MISCERR, "Invalid color index.", "setHex()");
                return MS_FAILURE;
            }

            MS_INIT_COLOR(*self, red, green, blue);
            return MS_SUCCESS;
        }
        else {
            msSetError(MS_MISCERR, "Invalid hex color.", "setHex()");
            return MS_FAILURE;
        }
    }   
    
    %newobject toHex;
    char *toHex() 
    {
        char hexcolor[8] = "";

        if (!self) 
        {
            msSetError(MS_MISCERR, "Can't express NULL color as hex",
                       "toHex()");
            return NULL;
        }
        if (self->red < 0 || self->green < 0 || self->blue < 0) 
        {
            msSetError(MS_MISCERR, "Can't express invalid color as hex",
                       "toHex()");
            return NULL;
        }
        snprintf(hexcolor, 8, "#%02x%02x%02x",
                 self->red, self->green, self->blue);
        return strdup(hexcolor);
    }

}

