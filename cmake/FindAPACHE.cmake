#
#  APACHE_FOUND - System has APACHE
#  APACHE_INCLUDE_DIR - The APACHE include directory
#
#  APACHE_LOCATION
#   setting this enables search for apache libraries / headers in this location

#
# Include directories
#
find_path(APACHE_INCLUDE_DIR
          NAMES httpd.h 
          PATH_SUFFIXES httpd apache apache2
)

find_program(APXS_BIN NAMES apxs apxs2
          PATH_SUFFIXES httpd apache apache2
)

find_program(HTTPD_EXECUTABLE NAMES httpd
          PATHS
            /usr/sbin
          PATH_SUFFIXES httpd apache apache2
)

if(APXS_BIN)
    EXECUTE_PROCESS(COMMAND ${APXS_BIN} -q LIBEXECDIR
	    OUTPUT_VARIABLE APACHE_MODULE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(APXS_BIN)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set APACHE_FOUND to TRUE if 
# all listed variables are TRUE
find_package_handle_standard_args(APACHE DEFAULT_MSG APACHE_INCLUDE_DIR )
mark_as_advanced(APACHE_INCLUDE_DIR)
