/* 
===========================================================================
 $Id$
 
 Project:  MapServer
 Purpose:  SWIG interface file for the MapServer mapscript module
 Author:   Steve Lime 
           Sean Gillies, sgillies@frii.com
             
 ===========================================================================
 Copyright (c) 1996-2005 Regents of the University of Minnesota.
  
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
============================================================================
*/

%begin %{
#ifdef _MSC_VER
#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
#else
#define _GNU_SOURCE 1
#endif
%}

#ifndef SWIGPHPNG
%module mapscript
#else
%module mapscriptng
#endif

#ifdef SWIGCSHARP
%ignore frompointer;
%include swig_csharp_extensions.i
#endif

#ifdef SWIGJAVA
%ignore layerObj::extent;
#endif

%newobject msLoadMapFromString;

%{
#include "../../mapserver.h"
#include "../../maptemplate.h"
#include "../../mapogcsld.h"
#include "../../mapows.h"
#include "../../cgiutil.h"
#include "../../mapcopy.h"
#include "../../maperror.h"
#include "../../mapprimitive.h"
#include "../../mapshape.h"

#if defined(WIN32) && defined(SWIGCSHARP)
/* <windows.h> is needed for GetExceptionCode() for unhandled exception */
#include <windows.h>
#endif

%}

/* Problem with SWIG CSHARP typemap for pointers */
#ifndef SWIGCSHARP
%include typemaps.i
#endif

%include constraints.i

%include carrays.i
%array_class(int, intarray)

/* 
===========================================================================
 Supporting 'None' as an argument to attribute accessor functions

 Typemaps to support NULL in attribute accessor functions
 provided to Steve Lime by David Beazley and tested for Python
 only by Sean Gillies.
   
 With the use of these typemaps we can execute statements like

   layer.group = None

============================================================================
*/

#ifdef __cplusplus
%typemap(memberin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
#else
%typemap(memberin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
#endif /* __cplusplus */

/* GD Buffer Mapping for use with imageObj::getBytes */
%{
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *data;
    int size;
    int owns_data;
} gdBuffer;

#ifdef __cplusplus
}
#endif
%}

/* 
============================================================================
 Exceptions

 Note: Python exceptions are in pymodule.i
============================================================================
*/

#if defined(SWIGCSHARP) || defined(SWIGJAVA) || defined(SWIGRUBY) || defined(SWIGPHP7)
%include "../mserror.i"
#endif

/* 
============================================================================
 Language-specific module code
============================================================================
*/

/* C# */
#ifdef SWIGCSHARP
%include "csmodule.i"
#endif

/* Java */
#ifdef SWIGJAVA
%include "javamodule.i"
#endif

/* Python */
#ifdef SWIGPYTHON
%include "pymodule.i"
#endif /* SWIGPYTHON */

/* Ruby */
#ifdef SWIGRUBY
%include "rbmodule.i"
#endif

/* Perl */
#ifdef SWIGPERL5
%include "plmodule.i"
#endif

/* Tcl */
#ifdef SWIGTCL8
%include "tclmodule.i"
#endif /* SWIGTCL8 */

/* PHP7 */
#ifdef SWIGPHP7
%include "php7module.i"
#endif /* SWIGPHP7 */


/* 
=============================================================================
 Wrap MapServer structs into mapscript classes
=============================================================================
*/

%include "../../mapserver.h"
%include "../../mapserver-version.h"
%include "../../mapprimitive.h"
%include "../../maperror.h"
%include "../../mapshape.h"
%include "../../mapproject.h"
%include "../../mapsymbol.h"
%include "../../maphash.h"
%include "../../maperror.h"
%include "../../mapserv-config.h"

%apply Pointer NONNULL { mapObj *map };
%apply Pointer NONNULL { layerObj *layer };
%apply Pointer NONNULL { configObj *config };

/* 
=============================================================================
 Class extension methods are now included from separate interface files  
=============================================================================
*/

%include "../swiginc/error.i"
%include "../swiginc/map.i"
%include "../swiginc/mapzoom.i"
%include "../swiginc/symbol.i"
%include "../swiginc/symbolset.i"
%include "../swiginc/layer.i"
%include "../swiginc/class.i"
%include "../swiginc/cluster.i"
%include "../swiginc/style.i"
%include "../swiginc/rect.i"
%include "../swiginc/image.i"
%include "../swiginc/shape.i"
%include "../swiginc/point.i"
%include "../swiginc/line.i"
%include "../swiginc/shapefile.i"
%include "../swiginc/outputformat.i"
%include "../swiginc/projection.i"
%include "../swiginc/dbfinfo.i"
%include "../swiginc/labelcache.i"
%include "../swiginc/color.i"
%include "../swiginc/hashtable.i"
%include "../swiginc/resultcache.i"
%include "../swiginc/result.i"
%include "../swiginc/owsrequest.i"
%include "../swiginc/connpool.i"
%include "../swiginc/msio.i"
%include "../swiginc/web.i"
%include "../swiginc/label.i"
%include "../swiginc/scalebar.i"
%include "../swiginc/legend.i"
%include "../swiginc/referencemap.i"
%include "../swiginc/querymap.i"
%include "../swiginc/config.i"

/* 
=============================================================================
 Language-specific extensions to mapserver classes are included here 
=============================================================================
*/

/* Java */
#ifdef SWIGJAVA
%include "javaextend.i"
#endif

/* Python */
#ifdef SWIGPYTHON
%include "pyextend.i"
#endif /* SWIGPYTHON */

/* Ruby */
#ifdef SWIGRUBY
%include "rbextend.i"
#endif

