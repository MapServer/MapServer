/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Python-specific enhancements to MapScript
 * Author:   Sean Gillies, sgillies@frii.com
 *
 ******************************************************************************
 *
 * Python-specific mapscript code has been moved into this 
 * SWIG interface file to improve the readibility of the main
 * interface file.  The main mapscript.i file includes this
 * file when SWIGPYTHON is defined (via 'swig -python ...').
 *
 *****************************************************************************/

/******************************************************************************
 * Supporting 'None' as an argument to attribute accessor functions
 ******************************************************************************
 *
 * Typemaps to support NULL in attribute accessor functions
 * provided to Steve Lime by David Beazley and tested for Python
 * only by Sean Gillies.
 *
 * With the use of these typemaps and some filtering on the mapscript
 * wrapper code to change the argument parsing of *_set() methods from
 * "Os" to "Oz", we can execute statements like
 *
 *   layer.group = None
 *
 *****************************************************************************/

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
#endif // __cplusplus

/**************************************************************************
 * MapServer Errors and Python Exceptions
 **************************************************************************
 *
 * Translation of errors reported via the ms_error structure into
 * Python exceptions. Original code by Chris Chamberlin <cdc@aracnet.com>
 * has been updated by Sean Gillies <sgillies@frii.com> to use 
 * PyErr_SetString, %exception and $action for SWIG 1.3, do the most
 * obvious mapping of MapServer errors to Python exceptions and map all 
 * others to a new 'MapservError' exception which can be caught like this:
 * 
 *   from mapscript import *
 *   empty_map = mapObj('')
 *   try:
 *       img = empty_map.draw()
 *   except MapservError, msg:
 *       print "Caught MapservError:", msg
 *
 *************************************************************************/

%{
PyObject *MSExc_MapservError;
%}

%init %{
MSExc_MapservError = PyErr_NewException("_mapscript.MapservError", NULL, NULL);
if (MSExc_MapservError != NULL)
    PyDict_SetItemString(d, "MapservError", MSExc_MapservError);
%}

%{
  static void _raise_ms_exception(void) {
    char errbuf[256];
    errorObj *ms_error = msGetErrorObj();
    snprintf(errbuf, 255, "%s: %s %s\n", ms_error->routine,
             msGetErrorString(ms_error->code), ms_error->message);
    // Map MapServer errors to Python exceptions, will define
    // custom Python exceptions soon.
    switch (ms_error->code) {
        case MS_IOERR:
            PyErr_SetString(PyExc_IOError, errbuf);
            break;
        case MS_MEMERR:
            PyErr_SetString(PyExc_MemoryError, errbuf);
            break;
        case MS_TYPEERR:
            PyErr_SetString(PyExc_TypeError, errbuf);
            break;
        case MS_EOFERR:
            PyErr_SetString(PyExc_EOFError, errbuf);
            break;
        default:
            PyErr_SetString(MSExc_MapservError, errbuf);
            break;
    }
  }
  
%}

%exception {
    $action {
        errorObj *ms_error = msGetErrorObj();
        // Sidestep the annoying MS_IOERR raised by msSearchDiskTree()
        // in maptree.c when a layer's shapetree index can't be found
        if ((ms_error->code != MS_NOERR) && (ms_error->code != -1)
        && !((ms_error->code == MS_IOERR) &&
             (strcmp(ms_error->routine, "msSearchDiskTree()") == 0)))
        {
            _raise_ms_exception();
            // Clear the current error
            ms_error->code = MS_NOERR;
            return NULL;
        }
    }
}

%pythoncode %{
MapservError = _mapscript.MapservError
%}

