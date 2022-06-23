# - Find LibSVG and LibSVG-Cairo
#
# Following variables are provided:
# SVG_FOUND
#     True if libsvg has been found
# SVG_INCLUDE_DIR
#     The include directories of libsvg
# SVG_LIBRARY
#     libsvg library list
# Following variables are provided:
# SVGCAIRO_FOUND
#     True if libsvg-cairo has been found
# SVGCAIRO_INCLUDE_DIR
#     The include directories of libsvg-cairo
# SVGCAIRO_LIBRARY
#     libsvg-cairo library list


find_package(PkgConfig)
pkg_check_modules(PC_SVG QUIET libsvg>=0.1.4)
pkg_check_modules(PC_SVGCAIRO QUIET libsvg-cairo>=0.1.6)

find_path(SVG_INCLUDE_DIR
   NAMES svg.h
   HINTS ${PC_SVG_INCLUDE_DIR} ${PC_SVG_INCLUDE_DIRS}
)

find_library(SVG_LIBRARY
   NAME svg
   HINTS ${PC_SVG_LIBDIR} ${PC_SVG_LIBRARY_DIRS}
)

find_path(SVGCAIRO_INCLUDE_DIR
   NAMES svg-cairo.h
   HINTS ${PC_SVGCAIRO_INCLUDE_DIR} ${PC_SVGCAIRO_INCLUDE_DIRS}
)

find_library(SVGCAIRO_LIBRARY
   NAME svg-cairo
   HINTS ${PC_SVGCAIRO_LIBDIR} ${PC_SVGCAIRO_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)

set(SVG_INCLUDE_DIRS ${SVG_INCLUDE_DIR})
set(SVG_LIBRARIES ${SVG_LIBRARY})
set(FPHSA_NAME_MISMATCHED TRUE) #handle mismatched package name warning
find_package_handle_standard_args(SVG DEFAULT_MSG SVG_LIBRARY SVG_INCLUDE_DIR)
unset(FPHSA_NAME_MISMATCHED)
mark_as_advanced(SVG_LIBRARY SVG_INCLUDE_DIR)
set(SVGCAIRO_INCLUDE_DIRS ${SVGCAIRO_INCLUDE_DIR})
set(SVGCAIRO_LIBRARIES ${SVGCAIRO_LIBRARY})
find_package_handle_standard_args(SVGCairo DEFAULT_MSG SVGCAIRO_LIBRARY SVGCAIRO_INCLUDE_DIR)
mark_as_advanced(SVGCAIRO_LIBRARY SVGCAIRO_INCLUDE_DIR)
