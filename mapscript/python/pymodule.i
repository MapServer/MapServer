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
 * Simple Typemaps
 *****************************************************************************/

// Translates Python None to C NULL for strings
%typemap(in,parse="z") char * "";

// Translate Python's built-in file object to FILE *
%typemap(python,in) FILE * {
    if (!PyFile_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "Input is not file");
        return NULL;
    }
    $1 = PyFile_AsFile($input);
}

/**************************************************************************
 * MapServer Errors and Python Exceptions
 **************************************************************************
 *
 * Translation of errors reported via the ms_error structure into
 * Python exceptions. Original code by Chris Chamberlin <cdc@aracnet.com>
 * has been updated by Sean Gillies <sgillies@frii.com> to use 
 * PyErr_SetString, %exception and $action for SWIG 1.3, do the most
 * obvious mapping of MapServer errors to Python exceptions and map all 
 * others to a new 'MapServerError' exception which can be caught like this:
 * 
 *   from mapscript import *
 *   empty_map = mapObj('')
 *   try:
 *       img = empty_map.draw()
 *   except MapServerError, msg:
 *       print "Caught MapServerError:", msg
 *
 *************************************************************************/

%{
PyObject *MSExc_MapServerError;
PyObject *MSExc_MapServerChildError;
%}

%init %{

// Generic MapServer error
MSExc_MapServerError=PyErr_NewException("_mapscript.MapServerError",NULL,NULL);
if (MSExc_MapServerError != NULL)
    PyDict_SetItemString(d, "MapServerError", MSExc_MapServerError);

// MapServer query MS_CHILD error
MSExc_MapServerChildError = PyErr_NewException("_mapscript.MapServerChildError", NULL, NULL);
if (MSExc_MapServerChildError != NULL)
    PyDict_SetItemString(d, "MapServerChildError", MSExc_MapServerChildError);

%}

%{
    static void _raise_ms_exception() {
        int errcode;
        errorObj *ms_error;
        ms_error = msGetErrorObj();
        errcode = ms_error->code;
        
        // Map MapServer errors to Python exceptions, will define
        // custom Python exceptions soon.  The exception we raise
        // is based on the error at the head of the MapServer error
        // list.  All other errors appear in the error message.
        switch (errcode) {
            case MS_IOERR:
                PyErr_SetString(PyExc_IOError, msGetErrorString("\n"));
                break;
            case MS_MEMERR:
                PyErr_SetString(PyExc_MemoryError, msGetErrorString("\n"));
                break;
            case MS_TYPEERR:
                PyErr_SetString(PyExc_TypeError, msGetErrorString("\n"));
                break;
            case MS_EOFERR:
                PyErr_SetString(PyExc_EOFError, msGetErrorString("\n"));
                break;
            case MS_CHILDERR:
                PyErr_SetString(MSExc_MapServerChildError, 
                                msGetErrorString("\n"));
                break;
            default:
                PyErr_SetString(MSExc_MapServerError, msGetErrorString("\n"));
                break;
        }
    }
  
%}

%exception {
    $action {
        errorObj *ms_error = msGetErrorObj();
       
        /* There are a few bogus errors to dodge, code smell anyone? :( */
        switch(ms_error->code) {
            case MS_NOERR:
                break;
            case MS_NOTFOUND:
                msResetErrorList();
                break;
            case -1:
                break;
            case MS_IOERR:
                if (strcmp(ms_error->routine, "msSearchDiskTree()") != 0) {
                    _raise_ms_exception();
                    msResetErrorList();
                    return NULL;
                }
            default:
                _raise_ms_exception();
                msResetErrorList();
                return NULL;
        }
                
    }
}
         
%pythoncode %{
MapServerError = _mapscript.MapServerError
MapServerChildError = _mapscript.MapServerChildError
%}



