/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  MapScript exceptions
   Author:   Umberto Unicoletti, unicoletti@prometeo.it
             Sean Gillies, sgillies@frii.com

   Note: Python exceptions are in mapscript/python
   ========================================================================= */

%include exception.i

%exception {
    $action 
    if (msGetErrorObj()!=NULL && msGetErrorObj()->code != MS_NOERR) {
        errorObj *ms_error = msGetErrorObj();
       
        switch(ms_error->code) {
            case MS_NOTFOUND:
                msResetErrorList();
                break;
            case -1:
                break;
            case MS_IOERR:
                SWIG_exception(SWIG_IOError ,ms_error->message);
                break;
            case MS_MEMERR:
                SWIG_exception(SWIG_MemoryError,ms_error->message);
                break;
            case MS_TYPEERR:
                SWIG_exception(SWIG_TypeError,ms_error->message);
                break;
            case MS_EOFERR:
                SWIG_exception(SWIG_SyntaxError,ms_error->message);
                break;
            case MS_CHILDERR:
                SWIG_exception(SWIG_SystemError,ms_error->message);
                break;
            default:
                SWIG_exception(SWIG_UnknownError,ms_error->message);
                break;
        }       
    }
}
 

