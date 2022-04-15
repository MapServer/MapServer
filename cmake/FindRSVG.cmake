# - Find Rsvg
# Find the Rsvg includes and libraries
#
# Following variables are provided:
# RSVG_FOUND
#     True if Rsvg has been found
# RSVG_INCLUDE_DIRS
#     The include directories of Rsvg
# RSVG_LIBRARIES
#     Rsvg library list

find_package(PkgConfig)
pkg_check_modules(RSVG QUIET librsvg-2.0>=2.32.0)
pkg_check_modules(GOBJECT QUIET gobject-2.0>=2.32.0)

find_path(RSVG_INCLUDE_DIR
   NAMES rsvg-cairo.h
   HINTS ${RSVG_INCLUDE_DIRS}
   PATH_SUFFIXES librsvg
)

find_library(RSVG_LIBRARY
   NAME rsvg-2
   HINTS ${RSVG_LIBRARY_DIRS}
)

find_path(GOBJECT_INCLUDE_DIR
   NAMES glib-object.h
   HINTS ${GOBJECT_INCLUDE_DIRS}
)

find_library(GOBJECT_LIBRARY
   NAME gobject-2.0
   HINTS ${GOBJECT_LIBRARY_DIRS}
)

find_package_handle_standard_args(RSVG DEFAULT_MSG RSVG_LIBRARY RSVG_INCLUDE_DIR)
if (${CMAKE_VERSION} GREATER "3.16") #handle mismatched package name warning
    find_package_handle_standard_args(GOBJECT DEFAULT_MSG NAME_MISMATCHED GOBJECT_LIBRARY GOBJECT_INCLUDE_DIR)
else()
    find_package_handle_standard_args(GOBJECT DEFAULT_MSG GOBJECT_LIBRARY GOBJECT_INCLUDE_DIR)
endif()
mark_as_advanced(RSVG_LIBRARY RSVG_INCLUDE_DIR RSVG_LIBRARIES RSVG_INCLUDE_DIRS GOBJECT_LIBRARY GOBJECT_INCLUDE_DIR)