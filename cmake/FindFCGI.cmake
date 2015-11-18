# Look for the header file.
find_path(FCGI_INCLUDE_DIR NAMES fastcgi.h)

# Look for the library.
find_library(FCGI_LIBRARY NAMES fcgi libfcgi)

set(FCGI_INCLUDE_DIRS ${FCGI_INCLUDE_DIR})
set(FCGI_LIBRARIES ${FCGI_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FCGI DEFAULT_MSG FCGI_LIBRARY FCGI_INCLUDE_DIR)
mark_as_advanced(FCGI_LIBRARY FCGI_INCLUDE_DIR)
