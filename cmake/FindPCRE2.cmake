# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file COPYING-CMAKE-SCRIPTS or https://cmake.org/licensing for details.

#.rst
# FindPCRE2
# ~~~~~~~~~
# Copyright (C) 2017-2018, Hiroshi Miura
#
# Find the native PCRE2 headers and libraries.

find_path(PCRE2_INCLUDE_DIR NAMES pcre2posix.h)
find_library(PCRE2-POSIX_LIBRARY NAMES pcre2-posix pcre2-posixd pcre2-posix-static pcre2-posix-staticd NAMES_PER_DIR)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE2
                                  REQUIRED_VARS PCRE2-POSIX_LIBRARY PCRE2_INCLUDE_DIR)
mark_as_advanced(PCRE2_INCLUDE_DIR PCRE2-POSIX_LIBRARY)
if(PCRE2_FOUND)
    if(NOT TARGET PCRE2::PCRE2-POSIX)
        add_library(PCRE2::PCRE2-POSIX UNKNOWN IMPORTED)
        set_target_properties(PCRE2::PCRE2-POSIX PROPERTIES
                              INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIR}"
                              IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                              IMPORTED_LOCATION "${PCRE2-POSIX_LIBRARY}")
    endif()
endif()
