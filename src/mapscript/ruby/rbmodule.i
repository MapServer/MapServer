/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Ruby-specific enhancements to MapScript
 * Author:   
 *
 ******************************************************************************
 *
 * Ruby-specific mapscript code has been moved into this 
 * SWIG interface file to improve the readibility of the main
 * interface file.  The main mapscript.i file includes this
 * file when SWIGRUBY is defined (via 'swig -ruby ...').
 *
 *****************************************************************************/

/******************************************************************************
 * Simple Typemaps
 *****************************************************************************/
/* To support imageObj::getBytes */
%typemap(out) gdBuffer {
    $result = rb_str_new($1.data, $1.size);
    msFree($1.data);
}

/**************************************************************************
 * MapServer Errors and Ruby Exceptions
 **************************************************************************
 *
 * Translation of errors reported via the ms_error structure into
 * Ruby exceptions. Generally copied from Python MapScript by Jim Klassen.
 * Follows original code by Chris Chamberlin <cdc@aracnet.com> as updated
 * updated by Sean Gillies <sgillies@frii.com>. 
 * Uses rb_raise, %exception and $action for SWIG 1.3, do the most
 * obvious mapping of MapServer errors to Ruby exceptions and map all 
 * others to a new 'MapServerError' exception which can be caught like 
 * this:
 * 
 *   require 'mapscript'
 *   empty_map = Mapscript.MapObj.new('')
 *   begin:
 *       img = empty_map.draw()
 *   rescue Mapscript::MapServerError => msg
 *       print "Caught MapServerError:", msg.to_s
 *   end
 *
 *************************************************************************/

%{
    VALUE MSExc_MapServerError;
    VALUE MSExc_MapServerChildError;
%}

/* Module initialization: call msSetup() and register msCleanup() */
%init %{

/* Copied from pymodule.i to fix #3619 */
    if (msSetup() != MS_SUCCESS)
    {
        msSetError(MS_MISCERR, "Failed to set up threads and font cache",
                   "msSetup()");
    }

    /* Is there a place to hook to call msCleanup()? */
    VALUE exceptionClass = rb_eval_string("Exception");
    VALUE mapscriptModule = rb_eval_string("Mapscript");
    MSExc_MapServerError = rb_define_class_under(mapscriptModule, "MapserverError", exceptionClass);
    MSExc_MapServerChildError = rb_define_class_under(mapscriptModule, "MapserverChildErrorError", exceptionClass);
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
            rb_raise(rb_eIOError, errmsg, "%s");
            break;
        case MS_MEMERR:
            rb_raise(rb_eNoMemError, errmsg, "%s");
            break;
        case MS_TYPEERR:
            rb_raise(rb_eTypeError, errmsg, "%s");
            break;
        case MS_EOFERR:
            rb_raise(rb_eEOFError, errmsg, "%s");
            break;
        case MS_CHILDERR:
            rb_raise(MSExc_MapServerChildError, errmsg, "%s");
            break;
        default:
            rb_raise(MSExc_MapServerError, errmsg, "%s");
            break;
    }

    free(errmsg);
}
%}

%exception {
    msResetErrorList();
    $action 
    {
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
                /*
                if (strncmp(ms_error->routine, "msSearchDiskTree()", 20) != 0) {
                    _raise_ms_exception();
                    msResetErrorList();
                    return NULL;
                }
                */
            default:
                _raise_ms_exception();
                msResetErrorList();
                return 0;
        }

    }
}

