/******************************************************************************
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


/******************************************************************************
 * gdBuffer Typemaps and helpers
 *****************************************************************************/

%pragma(csharp) imclasscode=%{
  public delegate void SWIGByteArrayDelegate(IntPtr data, int size);
%}

%insert(runtime) %{
/* Callback for returning byte arrays to C# */
typedef void (SWIGSTDCALL* SWIG_CSharpByteArrayHelperCallback)(const unsigned char *, const int);
%}

%typemap(ctype) SWIG_CSharpByteArrayHelperCallback    %{SWIG_CSharpByteArrayHelperCallback%}
%typemap(imtype) SWIG_CSharpByteArrayHelperCallback  %{SWIGByteArrayDelegate%}
%typemap(cstype) SWIG_CSharpByteArrayHelperCallback %{$imclassname.SWIGByteArrayDelegate%}
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
	getBytes(new $imclassname.SWIGByteArrayDelegate(this.CreateByteArray));
	return gdbuffer;
  }
  
  public void write(System.IO.Stream stream)
  {
	getBytes(new $imclassname.SWIGByteArrayDelegate(this.CreateByteArray));
	stream.Write(gdbuffer, 0, gdbuffer.Length);
  }
%}

