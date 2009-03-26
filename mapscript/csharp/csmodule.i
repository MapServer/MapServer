/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  C#-specific enhancements to MapScript
 * Author:   Tamas Szekeres, szekerest@gmail.com
 *
 ******************************************************************************
 *
 * C#-specific mapscript code has been moved into this 
 * SWIG interface file to improve the readibility of the main
 * interface file.  The main mapscript.i file includes this
 * file when SWIGCSHARP is defined (via 'swig -csharp ...').
 *
 *****************************************************************************/

/*Uncomment the following lines if you want to receive subsequent exceptions as
inner exceptions. Otherwise the exception message will be concatenated*/
//#if SWIG_VERSION >= 0x010329
//#define ALLOW_INNER_EXCEPTIONS
//#endif

%ignore fp;

/******************************************************************************
 * Module initialization helper (bug 1665)
 *****************************************************************************/

%pragma(csharp) imclasscode=%{
  protected class $moduleHelper 
	{
		static $moduleHelper() 
		{
			$module.msSetup();
		}
		 ~$moduleHelper() 
		{
			//$module.msCleanup();
		}
	}
	protected static $moduleHelper the$moduleHelper = new $moduleHelper();
%}

/******************************************************************************
 * C# exception redefinition
 *****************************************************************************/
#ifdef ALLOW_INNER_EXCEPTIONS
%exception
{
	errorObj *ms_error;
	$action
    ms_error = msGetErrorObj();
    if (ms_error != NULL && ms_error->code != MS_NOERR) {
	    if (ms_error->code != MS_NOTFOUND && ms_error->code != -1) {
            int ms_errorcode = ms_error->code;
            while (ms_error!=NULL && ms_error->code != MS_NOERR) {
                char* msg =  msAddErrorDisplayString(NULL, ms_error);
                if (msg) {
			        SWIG_CSharpException(SWIG_SystemError, msg);
			        free(msg);
		        }
                else SWIG_CSharpException(SWIG_SystemError, "MapScript unknown error");
                ms_error = ms_error->next;	  
            }
            msResetErrorList();
            return $null;
        }
        msResetErrorList();
    }
}
#else
%exception
{
	errorObj *ms_error;
	$action
    ms_error = msGetErrorObj();
    if (ms_error != NULL && ms_error->code != MS_NOERR) {
	    if (ms_error->code != MS_NOTFOUND && ms_error->code != -1) {
            char* msg = msGetErrorString(";"); 
		    if (msg) {
			    SWIG_CSharpException(SWIG_SystemError, msg);
			    free(msg);
		    }
            else SWIG_CSharpException(SWIG_SystemError, "MapScript unknown error");
            msResetErrorList();
		    return $null;
        }
        msResetErrorList();
    }
}
#endif

/******************************************************************************
 * typemaps for string arrays (for supporting map templates)
 *****************************************************************************/
 
%pragma(csharp) imclasscode=%{
  public class StringArrayMarshal : IDisposable {
    public readonly IntPtr[] _ar;
    public StringArrayMarshal(string[] ar) {
      _ar = new IntPtr[ar.Length];
      for (int cx = 0; cx < _ar.Length; cx++) {
	      _ar[cx] = System.Runtime.InteropServices.Marshal.StringToHGlobalAnsi(ar[cx]);
      }
    }
    public virtual void Dispose() {
	  for (int cx = 0; cx < _ar.Length; cx++) {
          System.Runtime.InteropServices.Marshal.FreeHGlobal(_ar[cx]);
      }
      GC.SuppressFinalize(this);
    }
  }
%}

%typemap(imtype, out="IntPtr") char** "IntPtr[]"
%typemap(cstype) char** %{string[]%}
%typemap(in) char** %{ $1 = ($1_ltype)$input; %}
%typemap(out) char** %{ $result = $1; %}
%typemap(csin) char** "new $modulePINVOKE.StringArrayMarshal($csinput)._ar"
%typemap(csout, excode=SWIGEXCODE) char** {
    $excode
    throw new System.NotSupportedException("Returning string arrays is not implemented yet.");
}
%typemap(csvarin, excode=SWIGEXCODE2) char** %{
    set {
      $excode
	  throw new System.NotSupportedException("Setting string arrays is not supported now.");
    } 
%}
/* specializations */
%typemap(csvarout, excode=SWIGEXCODE2) char** formatoptions %{
    get {
        IntPtr cPtr = $imcall;
        IntPtr objPtr;
	    string[] ret = new string[this.numformatoptions];
        for(int cx = 0; cx < this.numformatoptions; cx++) {
            objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(IntPtr)));
            ret[cx]= (objPtr == IntPtr.Zero) ? null : System.Runtime.InteropServices.Marshal.PtrToStringAnsi(objPtr);
        }
        $excode
        return ret;
    }
%}
%typemap(csvarout, excode=SWIGEXCODE2) char** values %{
    get {
        IntPtr cPtr = $imcall;
        IntPtr objPtr;
	    string[] ret = new string[this.numvalues];
        for(int cx = 0; cx < this.numvalues; cx++) {
            objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(IntPtr)));
            ret[cx]= (objPtr == IntPtr.Zero) ? null : System.Runtime.InteropServices.Marshal.PtrToStringAnsi(objPtr);
        }
        $excode
        return ret;
    }
%}

/******************************************************************************
 * typemaps for outputFormatObj arrays
 *****************************************************************************/

%typemap(ctype) outputFormatObj** "void*"
%typemap(imtype) outputFormatObj** "IntPtr"
%typemap(cstype) outputFormatObj** "outputFormatObj[]"
%typemap(out) outputFormatObj** %{ $result = $1; %}
%typemap(csout, excode=SWIGEXCODE) outputFormatObj** {
      IntPtr cPtr = $imcall;
	  IntPtr objPtr;
      outputFormatObj[] ret = new outputFormatObj[this.numoutputformats];
      for(int cx = 0; cx < this.numoutputformats; cx++) {
          objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(IntPtr)));
          ret[cx] = (objPtr == IntPtr.Zero) ? null : new outputFormatObj(objPtr, false);
      }
      $excode
      return ret;
}

%typemap(csvarout, excode=SWIGEXCODE2) outputFormatObj** %{
    get {
	  IntPtr cPtr = $imcall;
	  IntPtr objPtr;
      outputFormatObj[] ret = new outputFormatObj[this.numoutputformats];
      for(int cx = 0; cx < this.numoutputformats; cx++) {
          objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(IntPtr)));
          ret[cx] = (objPtr == IntPtr.Zero) ? null : new outputFormatObj(objPtr, false);
      }
      $excode
      return ret;
    }
%}

/******************************************************************************
 * gdBuffer Typemaps and helpers
 *****************************************************************************/

%pragma(csharp) imclasscode=%{
  public delegate void SWIGByteArrayDelegate(IntPtr data, int size);
%}

%insert(runtime) %{
/* Callback for returning byte arrays to C# */
typedef void (SWIGSTDCALL* SWIG_CSharpByteArrayHelperCallback)(const unsigned char *, const int);
/* Default callback interface */
static SWIG_CSharpByteArrayHelperCallback SWIG_csharp_bytearray_callback = NULL;
%}

%typemap(ctype) SWIG_CSharpByteArrayHelperCallback    %{SWIG_CSharpByteArrayHelperCallback%}
%typemap(imtype) SWIG_CSharpByteArrayHelperCallback  %{SWIGByteArrayDelegate%}
%typemap(cstype) SWIG_CSharpByteArrayHelperCallback %{$modulePINVOKE.SWIGByteArrayDelegate%}
%typemap(in) SWIG_CSharpByteArrayHelperCallback %{ $1 = ($1_ltype)$input; %}
%typemap(csin) SWIG_CSharpByteArrayHelperCallback "$csinput"

%csmethodmodifiers getBytes "private";
%ignore imageObj::getBytes();
%extend imageObj 
{
	void getBytes(SWIG_CSharpByteArrayHelperCallback callback) {
		gdBuffer buffer;
        
        buffer.data = msSaveImageBufferGD(self->img.gd, &buffer.size,
                                          self->format);
        if( buffer.size == 0 )
        {
            msSetError(MS_MISCERR, "Failed to get image buffer", "getBytes");
            return;
        }
        
        callback(buffer.data, buffer.size);
        gdFree(buffer.data);
	}
}

%ignore imageObj::write;

%typemap(cscode) imageObj %{
  private byte[] gdbuffer;
  private void CreateByteArray(IntPtr data, int size)
  {
      gdbuffer = new byte[size];
      Marshal.Copy(data, gdbuffer, 0, size);
  }
  
  public byte[] getBytes()
  {
	getBytes(new $modulePINVOKE.SWIGByteArrayDelegate(this.CreateByteArray));
	return gdbuffer;
  }
  
  public void write(System.IO.Stream stream)
  {
	getBytes(new $modulePINVOKE.SWIGByteArrayDelegate(this.CreateByteArray));
	stream.Write(gdbuffer, 0, gdbuffer.Length);
  }
%}


%typemap(ctype) gdBuffer    %{void%}
%typemap(imtype) gdBuffer  %{void%}
%typemap(cstype) gdBuffer %{byte[]%}

%typemap(out, null="") gdBuffer
%{ SWIG_csharp_bytearray_callback($1.data, $1.size);
	if( $1.owns_data ) gdFree($1.data); %}

// SWIGEXCODE is a macro used by many other csout typemaps
#ifdef SWIGEXCODE
%typemap(csout, excode=SWIGEXCODE) gdBuffer {
    $imcall;$excode
    return $modulePINVOKE.GetBytes();
}
#else
%typemap(csout) gdBuffer {
    $imcall;
    return $modulePINVOKE.GetBytes();
}
#endif

%pragma(csharp) imclasscode=%{
  protected class SWIGByteArrayHelper 
	{
		public delegate void SWIGByteArrayDelegate(IntPtr data, int size);
		static SWIGByteArrayDelegate bytearrayDelegate = new SWIGByteArrayDelegate(CreateByteArray);

		[DllImport("$dllimport", EntryPoint="SWIGRegisterByteArrayCallback_$module")]
		public static extern void SWIGRegisterByteArrayCallback_mapscript(SWIGByteArrayDelegate bytearrayDelegate);

		static void CreateByteArray(IntPtr data, int size) 
		{
			arraybuffer = new byte[size];
			Marshal.Copy(data, arraybuffer, 0, size);
		}

		static SWIGByteArrayHelper() 
		{
			SWIGRegisterByteArrayCallback_$module(bytearrayDelegate);
		}
	}
	protected static SWIGByteArrayHelper bytearrayHelper = new SWIGByteArrayHelper();
	[ThreadStatic] 
	private static byte[] arraybuffer;

	internal static byte[] GetBytes()
	{
		return arraybuffer;
	}
%}

%insert(runtime) %{
#ifdef __cplusplus
extern "C" 
#endif
#ifdef SWIGEXPORT
SWIGEXPORT void SWIGSTDCALL SWIGRegisterByteArrayCallback_$module(SWIG_CSharpByteArrayHelperCallback callback) {
  SWIG_csharp_bytearray_callback = callback;
}
#else
DllExport void SWIGSTDCALL SWIGRegisterByteArrayCallback_$module(SWIG_CSharpByteArrayHelperCallback callback) {
  SWIG_csharp_bytearray_callback = callback;
}
#endif
%}

/******************************************************************************
 * Preventing to take ownership of the memory when constructing objects 
 * with parent objects (causing nullreference exception, Bug 1743)
 *****************************************************************************/

%typemap(csconstruct, excode=SWIGEXCODE) layerObj(mapObj map) %{: this($imcall, true) {
  if (map != null) this.swigCMemOwn = false;$excode
}
%}
%typemap(csconstruct, excode=SWIGEXCODE) classObj(layerObj layer) %{: this($imcall, true) {
  if (layer != null) this.swigCMemOwn = false;$excode
}
%}
%typemap(csconstruct, excode=SWIGEXCODE) styleObj(classObj parent_class) %{: this($imcall, true) {
  if (parent_class != null) this.swigCMemOwn = false;$excode
}
%}
