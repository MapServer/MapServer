# Look for the header file.
find_path(LIBEXEMPI_INCLUDE_DIR NAMES xmp.h
    PATH_SUFFIXES exempi-2.0 exempi-2.0/exempi exempi 
)
find_library(LIBEXEMPI_LIBRARY NAMES exempi)

set(LIBEXEMPI_LIBRARIES ${LIBEXEMPI_LIBRARY})
set(LIBEXEMPI_INCLUDE_DIRS ${LIBEXEMPI_INCLUDE_DIR})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Exempi DEFAULT_MSG LIBEXEMPI_LIBRARY LIBEXEMPI_INCLUDE_DIR)
mark_as_advanced(LIBEXEMPI_INCLUDE_DIR LIBEXEMPI_LIBRARY)
