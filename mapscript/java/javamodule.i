
%include arrays_java.i

/* Mapscript library loader */

%pragma(java) jniclasscode=%{
    static {
        String  library = System.getProperty("mapserver.library.name", "mapscript");

        System.loadLibrary(library);
        /* TODO Throw when return value not MS_SUCCESS? */
        edu.umn.gis.mapscript.mapscript.msSetup();
    }
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


