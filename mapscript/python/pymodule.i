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

/* Translates Python None to C NULL for strings */
%typemap(in,parse="z") char * "";

/* Translate Python's built-in file object to FILE * */
%typemap(in) FILE * {
    if (!PyFile_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "Input is not file");
        return NULL;
    }
    $1 = PyFile_AsFile($input);
}

/* To support imageObj::getBytes */
%typemap(out) gdBuffer {
    $result = PyString_FromStringAndSize((const char*)$1.data, $1.size); 
    if( $1.owns_data )
       msFree($1.data);
}


/*
 *  Typemap for counted arrays of doubles <- PySequence
 */
%typemap(in,numinputs=1) (int nListSize, double* pListValues)
{
  int i;
  /* %typemap(in,numinputs=1) (int nListSize, double* pListValues)*/
  /* check if is List */
  if ( !PySequence_Check($input) ) {
    PyErr_SetString(PyExc_TypeError, "not a sequence");
    SWIG_fail;
  }
  $1 = PySequence_Size($input);
  $2 = (double*) malloc($1*sizeof(double));
  for( i = 0; i<$1; i++ ) {
    PyObject *o = PySequence_GetItem($input,i);
    if ( !PyArg_Parse(o,"d",&$2[i]) ) {
      PyErr_SetString(PyExc_TypeError, "not a number");
      Py_DECREF(o);
      SWIG_fail;
    }
    Py_DECREF(o);
  }
}


%fragment("CreateTupleFromDoubleArray","header") %{
static PyObject *
CreateTupleFromDoubleArray( double *first, unsigned int size ) {
  unsigned int i;
  PyObject *out = PyTuple_New( size );
  for( i=0; i<size; i++ ) {
    PyObject *val = PyFloat_FromDouble( *first );
    ++first;
    PyTuple_SetItem( out, i, val );
  }
  return out;
}
%}

%typemap(freearg) (int nListSize, double* pListValues)
{
  /* %typemap(freearg) (int nListSize, double* pListValues) */
  if ($2) {
    free((void*) $2);
  }
}


%typemap(in,numinputs=0) (double** argout, int* pnListSize) ( double* argout, int nListSize ) {
  /* %typemap(python,in,numinputs=0) (double *val, int*hasval) */
  $1 = &argout;
  $2 = &nListSize;
}

%typemap(argout,fragment="t_output_helper,CreateTupleFromDoubleArray") (double** argout, int* pnListSize) 
{
   /* %typemap(argout) (double* argout, int* pnListSize)  */
  PyObject *r;
  r = CreateTupleFromDoubleArray(*$1, *$2);
  free(*$1);
  $result = t_output_helper($result,r);
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

/* Module initialization: call msSetup() and register msCleanup() */
%init %{

/* See bug 1203 for discussion of race condition with GD font cache */
	if (msSetup() != MS_SUCCESS)
    {
        msSetError(MS_MISCERR, "Failed to set up threads and font cache",
                   "msSetup()");
    }

    Py_AtExit(msCleanup);

/* Define Generic MapServer error */
MSExc_MapServerError=PyErr_NewException("_mapscript.MapServerError",NULL,NULL);
if (MSExc_MapServerError != NULL)
    PyDict_SetItemString(d, "MapServerError", MSExc_MapServerError);

/* Define MapServer MS_CHILD error */
MSExc_MapServerChildError = PyErr_NewException("_mapscript.MapServerChildError", NULL, NULL);
if (MSExc_MapServerChildError != NULL)
    PyDict_SetItemString(d, "MapServerChildError", MSExc_MapServerChildError);

%}

%{

static void _raise_ms_exception( void );

static void _raise_ms_exception() {
    int errcode;
    errorObj *ms_error;
    char *errmsg;
    ms_error = msGetErrorObj();
    errcode = ms_error->code;
    errmsg = msGetErrorString("\n");
    
    switch (errcode) {
        case MS_IOERR:
            PyErr_SetString(PyExc_IOError, errmsg);
            break;
        case MS_MEMERR:
            PyErr_SetString(PyExc_MemoryError, errmsg);
            break;
        case MS_TYPEERR:
            PyErr_SetString(PyExc_TypeError, errmsg);
            break;
        case MS_EOFERR:
            PyErr_SetString(PyExc_EOFError, errmsg);
            break;
        case MS_CHILDERR:
            PyErr_SetString(MSExc_MapServerChildError, errmsg);
            break;
        default:
            PyErr_SetString(MSExc_MapServerError, errmsg);
            break;
    }

    free(errmsg);
}
  
%}

%exception {
    $action {
        errorObj *ms_error = msGetErrorObj();
       
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

/* The bogus "if 1:" is to introduce a new scope to work around indentation
   handling with pythonappend in different versions.  (#3180) */
%feature("pythonappend") layerObj %{if 1:
            self.p_map=None
            try:
                # python 2.5
                if args and len(args)!=0: 
                    self.p_map=args[0]
            except NameError:
                # python 2.6
                if map: 
                    self.p_map=map
       %}

/* The bogus "if 1:" is to introduce a new scope to work around indentation
   handling with pythonappend in different versions. (#3180) */
%feature("pythonappend") classObj %{if 1:
            self.p_layer =None
            try:
                # python 2.5
                if args and len(args)!=0: 
                    self.p_layer=args[0]
            except NameError:
                # python 2.6
                if layer: 
                    self.p_layer=layer
       %}

%feature("shadow") insertClass %{
	def insertClass(*args):
        actualIndex=$action(*args)
        args[1].p_layer=args[0]
        return actualIndex%}

%feature("shadow") getClass %{
	def getClass(*args):
		clazz = $action(*args)
		if clazz:
			if args and len(args)!=0:
				clazz.p_layer=args[0]
			else:
				clazz.p_layer=None
		return clazz%}

%feature("shadow") insertLayer %{
	def insertLayer(*args):
        actualIndex=$action(*args)
        args[1].p_map=args[0]
        return actualIndex%}

%feature("shadow") getLayer %{
	def getLayer(*args):
		layer = $action(*args)
		if layer:
			if args and len(args)!=0:
				layer.p_map=args[0]
			else:
				layer.p_map=None
		return layer%}

%feature("shadow") getLayerByName %{
	def getLayerByName(*args):
		layer = $action(*args)
		if layer:
			if args and len(args)!=0:
				layer.p_map=args[0]
			else:
				layer.p_map=None
		return layer%}

