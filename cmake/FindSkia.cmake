# Look for the header file.
find_path(SKIA_INCLUDE_DIR NAMES fastcgi.h)

# Look for the library.
find_library(SKIA_LIBRARY NAMES skia libskia)

set(SKIA_INCLUDE_DIRS ${SKIA_INCLUDE_DIR})
set(SKIA_LIBRARIES ${SKIA_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Skia DEFAULT_MSG SKIA_LIBRARY SKIA_INCLUDE_DIR)
mark_as_advanced(SKIA_LIBRARY SKIA_INCLUDE_DIR)
