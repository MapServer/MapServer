# Find Proj
#
# If it's found it sets PROJ_FOUND to TRUE
# and following variables are set:
#    PROJ_INCLUDE_DIR
#    PROJ_LIBRARY


FIND_PATH(PROJ_INCLUDE_DIR proj_api.h)

FIND_LIBRARY(PROJ_LIBRARY NAMES proj proj_i)

set(PROJ_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})
set(PROJ_LIBRARIES ${PROJ_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PROJ DEFAULT_MSG PROJ_LIBRARY PROJ_INCLUDE_DIR)
mark_as_advanced(PROJ_LIBRARY PROJ_INCLUDE_DIR)
