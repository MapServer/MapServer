%module mapscriptng;

%pragma(php) phpinfo="
  php_info_print_table_start();
  php_info_print_table_row(2, \"MapServer Version\", msGetVersion());
  php_info_print_table_end();
"


/* To support imageObj::getBytes */
%typemap(out) gdBuffer {
    RETVAL_STRINGL((const char*)$1.data, $1.size);
    if( $1.owns_data )
       msFree($1.data);
}


/* Module initialization: call msSetup() and register msCleanup() */
%init %{
    if (msSetup() != MS_SUCCESS)
    {
        msSetError(MS_MISCERR, "Failed to set up threads and font cache",
                   "msSetup()");
    }

%}

%mshutdown {
    msCleanup();
}


