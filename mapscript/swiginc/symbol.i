/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript symbolObj extensions
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


%include "../../mapsymbol.h"

/* Full support for symbols and addition of them to the map symbolset
   is done to resolve MapServer bug 579
   http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=579 */

%extend symbolObj 
{
    
    symbolObj(char *symbolname, const char *imagefile=NULL) 
    {
        symbolObj *symbol;
        symbol = (symbolObj *) malloc(sizeof(symbolObj));
        initSymbol(symbol);
        symbol->name = strdup(symbolname);
        if (imagefile) {
            msLoadImageSymbol(symbol, imagefile);
        }
        return symbol;
    }

    ~symbolObj() 
    {
		if (self) {
            if (msFreeSymbol(self)==MS_SUCCESS) {
            	free(self);
				self=NULL;
			}
        }
    }

    int setImagepath(const char *imagefile) {
	return msLoadImageSymbol(self, imagefile);
    }

    int setPoints(lineObj *line) {
        int i;
        for (i=0; i<line->numpoints; i++) {
            MS_COPYPOINT(&(self->points[i]), &(line->point[i]));
        }
        self->numpoints = line->numpoints;
        return self->numpoints;
    }

    %newobject getPoints;
    lineObj *getPoints() 
    {
        int i;
        lineObj *line;
        line = (lineObj *) malloc(sizeof(lineObj));
        line->point = (pointObj *) malloc(sizeof(pointObj)*(self->numpoints));
        for (i=0; i<self->numpoints; i++) {
            line->point[i].x = self->points[i].x;
            line->point[i].y = self->points[i].y;
        }
        line->numpoints = self->numpoints;
        return line;
    }

    int setPattern(int index, int value) 
    {
        if (index < 0 || index > MS_MAXPATTERNLENGTH) {
            msSetError(MS_SYMERR, "Can't set pattern at index %d.", "setPattern()", index);
            return MS_FAILURE;
        }
        self->pattern[index] = value;
        return MS_SUCCESS;
    }

    %newobject getImage;
    imageObj *getImage(outputFormatObj *format)
    {
        return msSymbolGetImageGD(self, format);
    }

    int setImage(imageObj *image)
    {
        return msSymbolSetImageGD(self, image);
    }

}


