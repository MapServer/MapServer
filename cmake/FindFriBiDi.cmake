# - Find Fribidi
# Find the Fribidi includes and libraries
#
# Following variables are provided:
# FRIBIDI_FOUND
#     True if Fribidi has been found
# FRIBIDI_INCLUDE_DIR
#     The include directories of Fribidi
# FRIBIDI_LIBRARY
#     Fribidi library list

find_package(PkgConfig)
pkg_check_modules(PC_FRIBIDI QUIET fribidi>=0.19.0)

find_path(FRIBIDI_INCLUDE_DIR
   NAMES fribidi.h
   HINTS ${PC_FRIBIDI_INCLUDE_DIR} ${PC_FRIBIDI_INCLUDE_DIRS}
   PATH_SUFFIXES fribidi
)

find_library(FRIBIDI_LIBRARY
   NAME fribidi
   HINTS ${PC_FRIBIDI_LIBDIR} ${PC_FRIBIDI_LIBRARY_DIRS}
)

set(FRIBIDI_INCLUDE_DIRS ${FRIBIDI_INCLUDE_DIR})
set(FRIBIDI_LIBRARIES ${FRIBIDI_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FriBiDi DEFAULT_MSG FRIBIDI_LIBRARY FRIBIDI_INCLUDE_DIR)
mark_as_advanced(FRIBIDI_LIBRARY FRIBIDI_INCLUDE_DIR)
