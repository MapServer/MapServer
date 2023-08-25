/* 
===========================================================================
 $Id$
 
 Project:  MapServer
 Purpose:  MapScript exceptions
 Author:   Umberto Unicoletti, unicoletti@prometeo.it
           Jerry Pisk, jerry.pisk@gmail.com
           Sean Gillies, sgillies@frii.com
           Tamas Szekeres, szekerest@gmail.com

 Note: Python exceptions are in mapscript/python
       C# exceptions are redefinied in mapscript/csharp
       
       This implementation truncates the error message length
       to MAX_ERROR_LEN since SWIG_exception does not allow
       to free memory allocated on the heap easily (szekerest)
============================================================================
*/
#define MAX_ERROR_LEN 8192
%include exception.i

%exception {
    errorObj *ms_error;
    $action
    ms_error = msGetErrorObj();
    if (ms_error!=NULL && ms_error->code != MS_NOERR) {
        char ms_message[MAX_ERROR_LEN];
        char* msg = msGetErrorString(";");
        int ms_errorcode = ms_error->code;
        if (msg) {
			snprintf(ms_message, MAX_ERROR_LEN, "%s", msg);
			free(msg);
		}
        else sprintf(ms_message, "Unknown message");

        msResetErrorList();
       
        switch(ms_errorcode) {
            case MS_NOTFOUND:
                break;
            case -1:
                break;
            case MS_IOERR:
                SWIG_exception(SWIG_IOError ,ms_message);
                break;
            case MS_MEMERR:
                SWIG_exception(SWIG_MemoryError,ms_message);
                break;
            case MS_TYPEERR:
                SWIG_exception(SWIG_TypeError,ms_message);
                break;
            case MS_EOFERR:
                SWIG_exception(SWIG_SyntaxError,ms_message);
                break;
            case MS_CHILDERR:
                SWIG_exception(SWIG_SystemError,ms_message);
                break;
            case MS_NULLPARENTERR:
                SWIG_exception(SWIG_SystemError,ms_message);
                break;
            default:
                SWIG_exception(SWIG_UnknownError,ms_message);
                break;
        }       
    }
}
