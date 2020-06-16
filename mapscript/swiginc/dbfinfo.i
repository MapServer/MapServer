/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript DBFInfo extensions
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

%extend DBFInfo 
{
    /// Get the field name of a DBF using the field index ``iField``
    char *getFieldName(int iField) 
    {
        static char pszFieldName[1000];
        int pnWidth;
        int pnDecimals;
        msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, 
                          &pnDecimals);
        return pszFieldName;
    }

    /// Get the field width of a DBF using the field index ``iField``
    int getFieldWidth(int iField) 
    {
        char pszFieldName[1000];
        int pnWidth;
        int pnDecimals;
        msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth,
                          &pnDecimals);
        return pnWidth;
    }

    /// Get the field decimals of a DBF using the field index ``iField``
    int getFieldDecimals(int iField) 
    {
        char pszFieldName[1000];
        int pnWidth;
        int pnDecimals;
        msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth,
                          &pnDecimals);
        return pnDecimals;
    }

    /// Get the field type of a DBF using the field index ``iField``.
    /// Field types are one of FTString, FTInteger, FTDouble, FTInvalid
    int getFieldType(int iField) 
    {
        return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
    }   

}


