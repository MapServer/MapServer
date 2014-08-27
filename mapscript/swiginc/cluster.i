/* ===========================================================================
   $Id$

   Project:  MapServer
   Purpose:  SWIG interface file for mapscript clusterObj extensions
   Author:   Tamas Szekeres

   ===========================================================================
   Copyright (c) 1996-2007 Regents of the University of Minnesota.

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

%extend clusterObj
{
  int updateFromString(char *snippet)
  {
    return msUpdateClusterFromString(self, snippet);
  }
  
  %newobject convertToString;
  char* convertToString()
  {
    return msWriteClusterToString(self);
  }
  
  int setGroup(char *group) 
  {
    if (!group || strlen(group) == 0) {
       msFreeExpression(&self->group);
       return MS_SUCCESS;
    }
    else return msLoadExpressionString(&self->group, group);
  }

  %newobject getGroupString;
  char *getGroupString() {
    return msGetExpressionString(&(self->group));
  }

  int setFilter(char *filter) {
    if (!filter || strlen(filter) == 0) {
      msFreeExpression(&self->filter);
      return MS_SUCCESS;
    }	
    else return msLoadExpressionString(&self->filter, filter);
  }

  %newobject getFilterString;
  char *getFilterString() {
    return msGetExpressionString(&(self->filter));
  }
}
