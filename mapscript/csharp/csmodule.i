
%typemap(ctype) gdBuffer    %{void%}
%typemap(imtype) gdBuffer  %{void%}
%typemap(cstype) gdBuffer %{byte[]%}

%typemap(out) gdBuffer
%{ SWIG_csharp_bytearray_callback($1.data, $1.size);
	gdFree($1.data); %}

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

%insert(runtime) %{
/* Callback for returning byte arrays to C#, the resulting array is not marshaled back, so the caller should call GetBytes to get back the results */
typedef void (SWIGSTDCALL* SWIG_CSharpByteArrayHelperCallback)(const unsigned char *, const int);
static SWIG_CSharpByteArrayHelperCallback SWIG_csharp_bytearray_callback = NULL;
%}

%pragma(csharp) imclasscode=%{
  class SWIGByteArrayHelper 
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
	static SWIGByteArrayHelper bytearrayHelper = new SWIGByteArrayHelper();
	static byte[] arraybuffer;

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
