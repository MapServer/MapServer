
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
  static $imclassname() {
  }
%}

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
    IntPtr cPtr = $imcall;
    $csclassname ret = (cPtr == IntPtr.Zero) ? null : new $csclassname(cPtr, $owner, ThisOwn_$owner());$excode
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
      IntPtr cPtr = $imcall;
      $csclassname ret = (cPtr == IntPtr.Zero) ? null : new $csclassname(cPtr, $owner, ThisOwn_$owner());$excode
      return ret;
    } %}
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE *& {
    /* %typemap(csout, excode=SWIGEXCODE) SWIGTYPE *& */
    IntPtr cPtr = $imcall;
    $*csclassname ret = (cPtr == IntPtr.Zero) ? null : new $*csclassname(cPtr, $owner, ThisOwn_$owner());$excode
    return ret;
  }
// Proxy classes (base classes, ie, not derived classes)
%typemap(csbody) SWIGTYPE %{
  /* %typemap(csbody) SWIGTYPE */
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;
  protected object swigParentRef;
  
  protected static object ThisOwn_true() { return null; }
  protected object ThisOwn_false() { return this; }

  internal $csclassname(IntPtr cPtr, bool cMemoryOwn, object parent) {
    swigCMemOwn = cMemoryOwn;
    swigParentRef = parent;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }
  internal static HandleRef getCPtrAndDisown($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigCMemOwn = false;
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new HandleRef(null, IntPtr.Zero);
    }
  }
  internal static HandleRef getCPtrAndSetReference($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new HandleRef(null, IntPtr.Zero);
    }
  }
%}

// Derived proxy classes
%typemap(csbody_derived) SWIGTYPE %{
  /* %typemap(csbody_derived) SWIGTYPE */
  private HandleRef swigCPtr;

  internal $csclassname(IntPtr cPtr, bool cMemoryOwn, object parent) : base($modulePINVOKE.$csclassnameUpcast(cPtr), cMemoryOwn, parent) {
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }
  internal static HandleRef getCPtrAndDisown($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigCMemOwn = false;
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new HandleRef(null, IntPtr.Zero);
    }
  }
  internal static HandleRef getCPtrAndSetReference($csclassname obj, object parent) {
    if (obj != null)
    {
      obj.swigParentRef = parent;
      return obj.swigCPtr;
    }
    else
    {
      return new HandleRef(null, IntPtr.Zero);
    }
  }
%}

// Typewrapper classes
%typemap(csbody) SWIGTYPE *, SWIGTYPE &, SWIGTYPE [], SWIGTYPE (CLASS::*) %{
  /* %typemap(csbody) SWIGTYPE *, SWIGTYPE &, SWIGTYPE [], SWIGTYPE (CLASS::*) */
  private HandleRef swigCPtr;

  internal $csclassname(IntPtr cPtr, bool futureUse, object parent) {
    swigCPtr = new HandleRef(this, cPtr);
  }

  protected $csclassname() {
    swigCPtr = new HandleRef(null, IntPtr.Zero);
  }

  internal static HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }
%}

%typemap(csfinalize) SWIGTYPE %{
  /* %typemap(csfinalize) SWIGTYPE */
  ~$csclassname() {
    Dispose();
  }
%}

%typemap(csconstruct, excode=SWIGEXCODE) SWIGTYPE %{: this($imcall, true, null) {$excode
  }
%}

%typemap(csdestruct, methodname="Dispose", methodmodifiers="public") SWIGTYPE {
  lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        $imcall;
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      swigParentRef = null;
      GC.SuppressFinalize(this);
    }
  }

%typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") TYPE {
  lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        $imcall;
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      swigParentRef = null;
      GC.SuppressFinalize(this);
      base.Dispose();
    }
  }

%typemap(csin) SWIGTYPE *DISOWN "$csclassname.getCPtrAndDisown($csinput, ThisOwn_false())"
%typemap(csin) SWIGTYPE *SETREFERENCE "$csclassname.getCPtrAndSetReference($csinput, ThisOwn_false())"

%pragma(csharp) modulecode=%{
  /* %pragma(csharp) modulecode */
  internal class $moduleObject : IDisposable {
	public virtual void Dispose() {
      
    }
  }
  internal static $moduleObject the$moduleObject = new $moduleObject();
  protected static object ThisOwn_true() { return null; }
  protected static object ThisOwn_false() { return the$moduleObject; }
  
  [DllImport("$dllimport", EntryPoint="SetEnvironmentVariable")]
  public static extern int SetEnvironmentVariable(string envstring);
%}


%insert(runtime) %{
#ifdef __cplusplus
extern "C" 
#endif
#ifdef SWIGEXPORT
SWIGEXPORT int SWIGSTDCALL SetEnvironmentVariable(const char *envstring) {
  return putenv(envstring);
}
#else
DllExport int SWIGSTDCALL SetEnvironmentVariable(const char *envstring) {
  return putenv(envstring);
}
#endif
%}
