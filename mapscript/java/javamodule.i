
%include arrays_java.i

/* Mapscript library loader */

%pragma(java) jniclasscode=%{
    static {
        String  library = System.getProperty("mapserver.library.name", "mapscript");

        System.loadLibrary(library);
    }
%}

/* GD Buffer Mapping */

%{

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int size;
  void *data;
} gdBuffer;

#ifdef __cplusplus
}
#endif

%}

%typemap(jni) gdBuffer    %{jbyteArray%}
%typemap(jtype) gdBuffer  %{byte[]%}
%typemap(jstype) gdBuffer %{byte[]%}

%typemap(out) gdBuffer
%{ $result = SWIG_JavaArrayOutSchar(jenv, $1.data, $1.size); gdFree($1.data); %}

%typemap(javain) gdBuffer "$javainput"
%typemap(javaout) gdBuffer {
    return $jnicall;
}


