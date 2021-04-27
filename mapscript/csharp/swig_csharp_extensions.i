
/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Fix for the SWIG Interface problems (early GC)
 *           and implementing SWIGTYPE *DISOWN
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

// Ensure the class is not marked BeforeFieldInit causing memory corruption with CLR4 
%pragma(csharp) imclasscode=%{

  public class UTF8Marshaler : System.Runtime.InteropServices.ICustomMarshaler {
    static UTF8Marshaler static_instance;

    public System.IntPtr MarshalManagedToNative(object managedObj) {
        if (managedObj == null)
            return System.IntPtr.Zero;
        if (!(managedObj is string))
            throw new System.Runtime.InteropServices.MarshalDirectiveException(
                   "UTF8Marshaler must be used on a string.");

        // not null terminated
        byte[] strbuf = System.Text.Encoding.UTF8.GetBytes((string)managedObj);
        System.IntPtr buffer = System.Runtime.InteropServices.Marshal.AllocHGlobal(strbuf.Length + 1);
        System.Runtime.InteropServices.Marshal.Copy(strbuf, 0, buffer, strbuf.Length);

        // write the terminating null
        System.Runtime.InteropServices.Marshal.WriteByte(buffer, strbuf.Length, 0);
        return buffer;
    }

    public object MarshalNativeToManaged(System.IntPtr pNativeData) {
	    int len = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pNativeData).Length;
        byte[] utf8data = new byte[len];
        System.Runtime.InteropServices.Marshal.Copy(pNativeData, utf8data, 0, len);
        return System.Text.Encoding.UTF8.GetString(utf8data);
    }

    public void CleanUpNativeData(System.IntPtr pNativeData) {
        System.Runtime.InteropServices.Marshal.FreeHGlobal(pNativeData);
    }

    public void CleanUpManagedData(object managedObj) {
    }

    public int GetNativeDataSize() {
        return -1;
    }

    public static System.Runtime.InteropServices.ICustomMarshaler GetInstance(string cookie) {
        if (static_instance == null) {
            return static_instance = new UTF8Marshaler();
        }
        return static_instance;
    }
  }
%}

%typemap(imtype, inattributes="[System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(UTF8Marshaler))]",
  outattributes="[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(UTF8Marshaler))]")
  char *, char *&, char[ANY], char[]   "string"

%typemap(csout, excode=SWIGEXCODE) SWIGTYPE {
    /* %typemap(csout, excode=SWIGEXCODE) SWIGTYPE */
    $&csclassname ret = new $&csclassname($imcall, true, null);$excode
    return ret;
  }

%typemap(csout, excode=SWIGEXCODE, new="1") SWIGTYPE & {
    /* typemap(csout, excode=SWIGEXCODE, new="1") SWIGTYPE & */
    $csclassname ret = new $csclassname($imcall, $owner, ThisOwn_$owner());$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE, new="1") SWIGTYPE *, SWIGTYPE [], SWIGTYPE (CLASS::*) {
    /* %typemap(csout, excode=SWIGEXCODE, new="1") SWIGTYPE *, SWIGTYPE [], SWIGTYPE (CLASS::*) */
    System.IntPtr cPtr = $imcall;
    $csclassname ret = (cPtr == System.IntPtr.Zero) ? null : new $csclassname(cPtr, $owner, ThisOwn_$owner());$excode
    return ret;
  }
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE %{
    /* %typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE */
    get {
      $&csclassname ret = new $&csclassname($imcall, $owner, ThisOwn_$owner());$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE & %{
    /* %typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE & */
    get {
      $csclassname ret = new $csclassname($imcall, $owner, ThisOwn_$owner());$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE *, struct SWIGTYPE *, SWIGTYPE [], SWIGTYPE (CLASS::*) %{
    /* %typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE *, SWIGTYPE [], SWIGTYPE (CLASS::*) */
    get {
      System.IntPtr cPtr = $imcall;
      $csclassname ret = (cPtr == System.IntPtr.Zero) ? null : new $csclassname(cPtr, $owner, ThisOwn_$owner());$excode
      return ret;
    } %}
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE *& {
    /* %typemap(csout, excode=SWIGEXCODE) SWIGTYPE *& */
    System.IntPtr cPtr = $imcall;
    $*csclassname ret = (cPtr == System.IntPtr.Zero) ? null : new $*csclassname(cPtr, $owner, ThisOwn_$owner());$excode
    return ret;
  }
// Proxy classes (base classes, ie, not derived classes)
%typemap(csbody) SWIGTYPE %{
  /* %typemap(csbody) SWIGTYPE */
  private System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;
  protected object swigParentRef;
  
  protected static object ThisOwn_true() { return null; }
  protected object ThisOwn_false() { return this; }

  internal $csclassname(System.IntPtr cPtr, bool cMemoryOwn, object parent) {
    swigCMemOwn = cMemoryOwn;
    swigParentRef = parent;
    swigCPtr = new System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero) : obj.swigCPtr;
  }
  internal static System.Runtime.InteropServices.HandleRef getCPtrAndDisown($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigCMemOwn = false;
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero);
    }
  }
  internal static System.Runtime.InteropServices.HandleRef getCPtrAndSetReference($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero);
    }
  }
%}

// Derived proxy classes
%typemap(csbody_derived) SWIGTYPE %{
  /* %typemap(csbody_derived) SWIGTYPE */
  private System.Runtime.InteropServices.HandleRef swigCPtr;

  internal $csclassname(System.IntPtr cPtr, bool cMemoryOwn, object parent) : base($modulePINVOKE.$csclassnameUpcast(cPtr), cMemoryOwn, parent) {
    swigCPtr = new System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero) : obj.swigCPtr;
  }
  internal static System.Runtime.InteropServices.HandleRef getCPtrAndDisown($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigCMemOwn = false;
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero);
    }
  }
  internal static System.Runtime.InteropServices.HandleRef getCPtrAndSetReference($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero);
    }
  }
%}

// Typewrapper classes
%typemap(csbody) SWIGTYPE *, SWIGTYPE &, SWIGTYPE [], SWIGTYPE (CLASS::*) %{
  /* %typemap(csbody) SWIGTYPE *, SWIGTYPE &, SWIGTYPE [], SWIGTYPE (CLASS::*) */
  private System.Runtime.InteropServices.HandleRef swigCPtr;

  internal $csclassname(System.IntPtr cPtr, bool futureUse, object parent) {
    swigCPtr = new System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  protected $csclassname() {
    swigCPtr = new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero);
  }

  internal static System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero) : obj.swigCPtr;
  }
%}

#if SWIG_VERSION < 0x040000
%typemap(csfinalize) SWIGTYPE %{
  /* %typemap(csfinalize) SWIGTYPE */
  ~$csclassname() {
    Dispose();
  }
%}
#endif

%typemap(csconstruct, excode=SWIGEXCODE) SWIGTYPE %{: this($imcall, true, null) {$excode
  }
%}

#if SWIG_VERSION < 0x040000
%typemap(csdestruct, methodname="Dispose", methodmodifiers="public") SWIGTYPE {
  lock(this) {
      if(swigCPtr.Handle != System.IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        $imcall;
      }
      swigCPtr = new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero);
      swigParentRef = null;
      System.GC.SuppressFinalize(this);
    }
  }
#endif

%typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") TYPE {
  lock(this) {
      if(swigCPtr.Handle != System.IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        $imcall;
      }
      swigCPtr = new System.Runtime.InteropServices.HandleRef(null, System.IntPtr.Zero);
      swigParentRef = null;
      System.GC.SuppressFinalize(this);
      base.Dispose();
    }
  }

%typemap(csin) SWIGTYPE *DISOWN "$csclassname.getCPtrAndDisown($csinput, ThisOwn_false())"
%typemap(csin) SWIGTYPE *SETREFERENCE "$csclassname.getCPtrAndSetReference($csinput, ThisOwn_false())"

%pragma(csharp) modulecode=%{
  /* %pragma(csharp) modulecode */
  internal class $moduleObject : global::System.IDisposable {
	public virtual void Dispose() {
      
    }
  }
  internal static $moduleObject the$moduleObject = new $moduleObject();
  protected static object ThisOwn_true() { return null; }
  protected static object ThisOwn_false() { return the$moduleObject; }
  
  [System.Runtime.InteropServices.DllImport("$dllimport", EntryPoint="SetEnvironmentVariable")]
  public static extern int SetEnvironmentVariable(string envstring);
%}


%insert(runtime) %{
#ifdef __cplusplus
extern "C" 
#endif
#ifdef SWIGEXPORT
SWIGEXPORT int SWIGSTDCALL SetEnvironmentVariable(const char *envstring) {
  /* putenv may not make a copy, so do it ourselves despite the memory leak */
  char* envstringDup = (char*)malloc(strlen(envstring)+1);
  memcpy(envstringDup, envstring, strlen(envstring)+1);
  return putenv(envstringDup);
}
#else
DllExport int SWIGSTDCALL SetEnvironmentVariable(const char *envstring) {
  /* putenv may not make a copy, so do it ourselves despite the memory leak */
  char* envstringDup = (char*)malloc(strlen(envstring)+1);
  memcpy(envstringDup, envstring, strlen(envstring)+1);
  return putenv(envstringDup);
}
#endif
%}
