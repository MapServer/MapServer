/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface document file for mapscript classes
   Author:   Seth Girvin

   ===========================================================================
   Copyright (c) 1996-present Regents of the University of Minnesota.
   
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

%define CLASSDOC
      // hide parameters at the class level with an empty constructor
      %feature("autodoc", "classObj()
Class to store classification information") classObj;
%enddef

%define CLASSDOCSTRING
    %pythoncode "../swigdoc/class.py"
%enddef

%define COLORDOC
      %feature("autodoc", "colorObj()
MapServer colors are instances of colorObj. A colorObj may be a lone object or 
an attribute of other objects and have no other associations") colorObj;
%enddef

%define COLORDOCSTRING
    %pythoncode "../swigdoc/color.py"
%enddef

%define CLUSTERDOC
      %feature("autodoc", "clusterObj()
Class to define clustering") clusterObj;
%enddef

%define CLUSTERDOCSTRING
    %pythoncode "../swigdoc/cluster.py"
%enddef

%define DBFINFODOC
      %feature("autodoc", "DBFInfo()
Class to provide information about a DBF file") DBFInfo;
%enddef

%define DBFINFODOCSTRING
    %pythoncode "../swigdoc/dbfinfo.py"
%enddef

%define ERRORDOC
      %feature("autodoc", "errorObj()
This class allows inspection of the MapServer error stack. Only needed for the modules which
do not expose the error stack through exceptions (such as Python)") errorObj;
%enddef

%define ERRORDOCSTRING
    %pythoncode "../swigdoc/error.py"
%enddef

%define HASHTABLEDOC
      %feature("autodoc", "hashTableObj()
A hashTableObj is a very simple mapping of case-insensitive string keys to single string values. 
Map, Layer, and Class metadata have always been hash tables and now these are exposed 
directly. This is a limited hash that can contain no more than 41 values.") hashTableObj;
%enddef

%define HASHTABLEDOCSTRING
    %pythoncode "../swigdoc/hashtable.py"
%enddef
