/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript colorObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             Seth Girvin

   ===========================================================================
   Copyright (c) 1996-2019 Regents of the University of Minnesota.
   
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

%extend colorObj 
{

    /// Create a new instance. The color arguments are optional. 
    colorObj(int red=0, int green=0, int blue=0, int alpha=255) 
    {
        colorObj *color;
        
        /* Check colors */
        if (red > 255 || green > 255 || blue > 255 || alpha>255 ||
            red<-1 || green<-1 || blue<-1 || alpha<0 ) {
            msSetError(MS_MISCERR, "Invalid color", "colorObj()");
            return NULL;
        }
    
        color = (colorObj *)calloc(1, sizeof(colorObj));
        if (!color)
            return(NULL);
    
        MS_INIT_COLOR(*color, red, green, blue, alpha);

        return(color);
    }

    ~colorObj() 
    {
        free(self);
    }

    /// Set all four RGBA components. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int setRGB(int red, int green, int blue, int alpha = 255) 
    {
        /* Check colors */
        if (red > 255 || green > 255 || blue > 255 || alpha > 255) {
            msSetError(MS_MISCERR, "Invalid color index.", "setRGB()");
            return MS_FAILURE;
        }
    
        MS_INIT_COLOR(*self, red, green, blue, alpha);
        return MS_SUCCESS;
    }

    /**
    Set the color to values specified in case-independent hexadecimal notation. 
    hex must start with a '#' followed by three or four hex bytes, e.g. '#ffffff' 
    or '#ffffffff'. If only three hex bytes are supplied, the alpha will be set 
    to 255. Calling setHex('#ffffff') therefore assigns values of 255 to each 
    color component, including the alpha. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int setHex(char *psHexColor) 
    {
        int red, green, blue, alpha = 255;
        if (psHexColor && (strlen(psHexColor) == 7 || strlen(psHexColor) == 9) && psHexColor[0] == '#') {
            red = msHexToInt(psHexColor+1);
            green = msHexToInt(psHexColor+3);
            blue= msHexToInt(psHexColor+5);
            if (strlen(psHexColor) == 9) {
                alpha = msHexToInt(psHexColor+7);
            }
            if (red > 255 || green > 255 || blue > 255 || alpha > 255) {
                msSetError(MS_MISCERR, "Invalid color index.", "setHex()");
                return MS_FAILURE;
            }

            MS_INIT_COLOR(*self, red, green, blue, alpha);
            return MS_SUCCESS;
        }
        else {
            msSetError(MS_MISCERR, "Invalid hex color.", "setHex()");
            return MS_FAILURE;
        }
    }   

    %newobject toHex;
    /**
    Complement to setHex, returning a hexadecimal representation of the color 
    components. If alpha is 255 then this is three hex bytes '#rrggbb', 
    otherwise four hex bytes '#rrggbbaa'
    */
    char *toHex() 
    {
        char *hexcolor;

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
        if (self->alpha == 255) {
          hexcolor = msSmallMalloc(8);
          snprintf(hexcolor, 8, "#%02x%02x%02x",
                   self->red, self->green, self->blue);
        } else if (self->alpha >= 0) {
          hexcolor = msSmallMalloc(10);
          snprintf(hexcolor, 10, "#%02x%02x%02x%02x",
                   self->red, self->green, self->blue, self->alpha);
        } else {
           msSetError(MS_MISCERR, "Can't express color with invalid alpha as hex",
                      "toHex()");
           return NULL;
        }
        return hexcolor;
    }
}
