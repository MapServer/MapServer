/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  C#-specific enhancements to MapScript
 * Author:   Tamas Szekeres, szekerest@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

/*Uncomment the following lines if you want to receive subsequent exceptions as
inner exceptions. Otherwise the exception message will be concatenated*/
//#if SWIG_VERSION >= 0x010329
//#define ALLOW_INNER_EXCEPTIONS
//#endif

%ignore fp;

/******************************************************************************
 * Implement Equals and GetHashCode properly
 *****************************************************************************/

%typemap(cscode) SWIGTYPE %{
  public override bool Equals(object obj) {
    if (obj == null)
        return false;
    if (this.GetType() != obj.GetType())
        return false;
    return swigCPtr.Handle.Equals($csclassname.getCPtr(($csclassname)obj).Handle);
  }

  public override int GetHashCode() {
    return swigCPtr.Handle.GetHashCode();
  }
%}

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
public class StringArrayMarshal : global::System.IDisposable {
    public readonly System.IntPtr[] _ar;
    public StringArrayMarshal(string[] ar) {
      _ar = new System.IntPtr[ar.Length];
      for (int cx = 0; cx < _ar.Length; cx++) {
	      _ar[cx] = System.Runtime.InteropServices.Marshal.StringToHGlobalAnsi(ar[cx]);
      }
    }
    public virtual void Dispose() {
	  for (int cx = 0; cx < _ar.Length; cx++) {
          System.Runtime.InteropServices.Marshal.FreeHGlobal(_ar[cx]);
      }
      System.GC.SuppressFinalize(this);
    }
  }
%}

%typemap(imtype, out="System.IntPtr") char** "System.IntPtr[]"
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
        System.IntPtr cPtr = $imcall;
        System.IntPtr objPtr;
	    string[] ret = new string[this.numformatoptions];
        for(int cx = 0; cx < this.numformatoptions; cx++) {
            objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(System.IntPtr)));
            ret[cx]= (objPtr == System.IntPtr.Zero) ? null : System.Runtime.InteropServices.Marshal.PtrToStringAnsi(objPtr);
        }
        $excode
        return ret;
    }
%}
%typemap(csvarout, excode=SWIGEXCODE2) char** values %{
    get {
        System.IntPtr cPtr = $imcall;
        System.IntPtr objPtr;
	    string[] ret = new string[this.numvalues];
        for(int cx = 0; cx < this.numvalues; cx++) {
            objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(System.IntPtr)));
            ret[cx]= (objPtr == System.IntPtr.Zero) ? null : System.Runtime.InteropServices.Marshal.PtrToStringAnsi(objPtr);
        }
        $excode
        return ret;
    }
%}

/******************************************************************************
 * typemaps for outputFormatObj arrays
 *****************************************************************************/

%typemap(ctype) outputFormatObj** "void*"
%typemap(imtype) outputFormatObj** "System.IntPtr"
%typemap(cstype) outputFormatObj** "outputFormatObj[]"
%typemap(out) outputFormatObj** %{ $result = $1; %}
%typemap(csout, excode=SWIGEXCODE) outputFormatObj** {
      System.IntPtr cPtr = $imcall;
      System.IntPtr objPtr;
      outputFormatObj[] ret = new outputFormatObj[this.numoutputformats];
      for(int cx = 0; cx < this.numoutputformats; cx++) {
          objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(System.IntPtr)));
          ret[cx] = (objPtr == System.IntPtr.Zero) ? null : new outputFormatObj(objPtr, false, ThisOwn_false());
      }
      $excode
      return ret;
}

%typemap(csvarout, excode=SWIGEXCODE2) outputFormatObj** %{
    get {
      System.IntPtr cPtr = $imcall;
      System.IntPtr objPtr;
      outputFormatObj[] ret = new outputFormatObj[this.numoutputformats];
      for(int cx = 0; cx < this.numoutputformats; cx++) {
          objPtr = System.Runtime.InteropServices.Marshal.ReadIntPtr(cPtr, cx * System.Runtime.InteropServices.Marshal.SizeOf(typeof(System.IntPtr)));
          ret[cx] = (objPtr == System.IntPtr.Zero) ? null : new outputFormatObj(objPtr, false, ThisOwn_false());
      }
      $excode
      return ret;
    }
%}

/******************************************************************************
 * gdBuffer Typemaps and helpers
 *****************************************************************************/

%pragma(csharp) imclasscode=%{
  public delegate void SWIGByteArrayDelegate(System.IntPtr data, int size);
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

%typemap(imtype) (unsigned char* pixels) "System.IntPtr"
%typemap(cstype) (unsigned char* pixels) "System.IntPtr"
%typemap(in) (unsigned char* pixels) %{ $1 = ($1_ltype)$input; %}
%typemap(csin) (unsigned char* pixels) "$csinput"

%csmethodmodifiers getBytes "private";
%ignore imageObj::getBytes();
%extend imageObj 
{
	void getBytes(SWIG_CSharpByteArrayHelperCallback callback) {
        gdBuffer buffer;
        
        buffer.owns_data = MS_TRUE;
        
        buffer.data = msSaveImageBuffer(self, &buffer.size, self->format);
            
        if( buffer.data == NULL || buffer.size == 0 )
        {
            msSetError(MS_MISCERR, "Failed to get image buffer", "getBytes");
            return;
        }
        callback(buffer.data, buffer.size);
        msFree(buffer.data);
    }

    int getRawPixels(unsigned char* pixels) {
      if (MS_RENDERER_PLUGIN(self->format)) {
        rendererVTableObj *renderer = self->format->vtable;
        if(renderer->supports_pixel_buffer) {
          rasterBufferObj rb;
          int status = MS_SUCCESS;
          int size = self->width * self->height * 4 * sizeof(unsigned char);
          
          status = renderer->getRasterBufferHandle(self,&rb);
          if(MS_UNLIKELY(status == MS_FAILURE)) {
            return MS_FAILURE;
          }
          memcpy(pixels, rb.data.rgba.pixels, size);
          return status;
        }
      }
      return MS_FAILURE;
    }
}

%ignore imageObj::write;
%typemap(cscode) imageObj, struct imageObj %{
  private byte[] gdbuffer;
  private void CreateByteArray(System.IntPtr data, int size)
  {
      gdbuffer = new byte[size];
      System.Runtime.InteropServices.Marshal.Copy(data, gdbuffer, 0, size);
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


%csmethodmodifiers processTemplate "private";
%csmethodmodifiers processLegendTemplate "private";
%csmethodmodifiers processQueryTemplate "private";

%typemap(csinterfaces) mapObj "System.IDisposable, System.Runtime.Serialization.ISerializable";
%typemap(csattributes) mapObj  "[Serializable]"
%typemap(cscode) mapObj, struct mapObj %{  
  public string processTemplate(int bGenerateImages, string[] names, string[] values)
  {
	if (names.Length != values.Length)
	    throw new System.ArgumentException("Invalid array length specified!");
	return processTemplate(bGenerateImages, names, values, values.Length);
  }
  
  public string processLegendTemplate(string[] names, string[] values)
  {
	if (names.Length != values.Length)
	    throw new System.ArgumentException("Invalid array length specified!");
	return processLegendTemplate(names, values, values.Length);
  }
  
  public string processQueryTemplate(string[] names, string[] values)
  {
	if (names.Length != values.Length)
	    throw new System.ArgumentException("Invalid array length specified!");
	return processQueryTemplate(names, values, values.Length);
  }
 
  public mapObj(
      System.Runtime.Serialization.SerializationInfo info
      , System.Runtime.Serialization.StreamingContext context) : this(info.GetString("mapText"), 1)
  {       
        //this constructor is needed for ISerializable interface
  }
  
  public void GetObjectData(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context) 
    {    
        info.AddValue( "mapText", this.convertToString() );        
    }
%}


%typemap(ctype) gdBuffer    %{void%}
%typemap(imtype) gdBuffer  %{void%}
%typemap(cstype) gdBuffer %{byte[]%}

%typemap(out, null="") gdBuffer
%{ SWIG_csharp_bytearray_callback($1.data, $1.size);
	if( $1.owns_data ) msFree($1.data); %}

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
		public delegate void SWIGByteArrayDelegate(System.IntPtr data, int size);
		static SWIGByteArrayDelegate bytearrayDelegate = new SWIGByteArrayDelegate(CreateByteArray);

		[System.Runtime.InteropServices.DllImport("$dllimport", EntryPoint="SWIGRegisterByteArrayCallback_$module")]
		public static extern void SWIGRegisterByteArrayCallback_mapscript(SWIGByteArrayDelegate bytearrayDelegate);

		static void CreateByteArray(System.IntPtr data, int size)
		{
			arraybuffer = new byte[size];
			System.Runtime.InteropServices.Marshal.Copy(data, arraybuffer, 0, size);
		}

		static SWIGByteArrayHelper() 
		{
			SWIGRegisterByteArrayCallback_$module(bytearrayDelegate);
		}
	}
	protected static SWIGByteArrayHelper bytearrayHelper = new SWIGByteArrayHelper();
	[System.ThreadStatic]
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

/* Typemaps for pattern array */
%typemap(imtype) (double pattern[ANY]) "System.IntPtr"
%typemap(cstype) (double pattern[ANY]) "double[]"
%typemap(in) (double pattern[ANY]) %{ $1 = ($1_ltype)$input; %}
%typemap(csin) (double pattern[ANY]) "$csinput"
%typemap(csvarout, excode=SWIGEXCODE2) (double pattern[ANY]) %{
    get {
      System.IntPtr cPtr = $imcall;
      double[] ret = new double[patternlength];
      if (patternlength > 0) {       
	        System.Runtime.InteropServices.Marshal.Copy(cPtr, ret, 0, patternlength);
      }
      $excode
      return ret;
    } 
    set {
      System.IntPtr cPtr = $imcall;
      if (value.Length > 0) {       
	        System.Runtime.InteropServices.Marshal.Copy(value, 0, cPtr, value.Length);
      }
      patternlength = value.Length;
      $excode
    }
    %}
%typemap(csvarin, excode="") (double pattern[ANY]) %{$excode%}

/* Typemaps for int array */
%typemap(imtype, out="System.IntPtr") int *panIndexes "int[]"
%typemap(cstype) int *panIndexes %{int[]%}
%typemap(in) int *panIndexes %{ $1 = ($1_ltype)$input; %}
%typemap(csin) (int *panIndexes)  "$csinput"

/* Typemaps for device handle */
%typemap(imtype) (void* device)  %{System.IntPtr%}
%typemap(cstype) (void* device) %{System.IntPtr%}
%typemap(in) (void* device) %{ $1 = ($1_ltype)$input; %}
%typemap(csin) (void* device)  "$csinput"

/******************************************************************************
 * Preventing to take ownership of the memory when constructing objects 
 * with parent objects (causing nullreference exception, Bug 1743)
 *****************************************************************************/

%typemap(csconstruct, excode=SWIGEXCODE) layerObj(mapObj map) %{: this($imcall, true, map) {
  $excode
}
%}
%typemap(csconstruct, excode=SWIGEXCODE) classObj(layerObj layer) %{: this($imcall, true, layer) {
  $excode
}
%}
%typemap(csconstruct, excode=SWIGEXCODE) styleObj(classObj parent_class) %{: this($imcall, true, parent_class) {
  $excode
}
%}

%typemap(csout, excode=SWIGEXCODE) classObj* getClass, layerObj* getLayer, layerObj *getLayerByName, styleObj* getStyle {
    System.IntPtr cPtr = $imcall;
    $csclassname ret = (cPtr == System.IntPtr.Zero) ? null : new $csclassname(cPtr, $owner, ThisOwn_false());$excode
    return ret;
  }
